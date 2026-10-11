// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include "api_handlers.hpp"

#include "api_settings_json.hpp"

#include "api_resources.hpp"

#include <softphone/core/api_json.hpp>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <span>
#include <utility>
#include <vector>

namespace softphone::ui {
namespace {

[[nodiscard]] std::string refuse(std::string_view why)
{
    return "!" + std::string{why};
}

[[nodiscard]] core::ApiResponse answered(std::optional<std::string> body,
                                         std::string contentType = "application/json")
{
    if (!body) {
        return core::apiError(503, "the phone did not answer in time");
    }

    core::ApiResponse response;
    response.contentType = std::move(contentType);
    response.body = std::move(*body);
    return response;
}

[[nodiscard]] core::ApiResponse accepted(std::optional<std::string> body, int status = 200)
{
    if (!body) {
        return core::apiError(503, "the phone did not answer in time");
    }
    if (!body->empty() && body->front() == '!') {
        return core::apiError(400, body->substr(1));
    }

    core::ApiResponse response;
    response.status = status;
    response.body = body->empty() ? std::string{"{}"} : *body;
    return response;
}

}

void registerApiHandlers(core::ApiRouter &router, ApiDependencies dependencies)
{
    const auto askStorage = dependencies.askStorage;
    const auto askTaxonomy = dependencies.askTaxonomy;
    const auto askEngine = dependencies.askEngine;
    const auto calls = dependencies.calls;
    const auto configuration = dependencies.configuration;
    const auto registrationState = dependencies.registrationState;
    const auto settings = dependencies.settings;
    const auto contactsChanged = dependencies.contactsChanged;
    const auto historyChanged = dependencies.historyChanged;

    router.add(core::ApiRoute{"GET", "/contacts", core::ApiAccess::Read, resource::kContacts,
                              "The address book"},
               [askStorage](const core::ApiRequest &) {
                   return answered(askStorage([](core::IContactStore &stored, core::ICallLog &) {
                       std::vector<core::Contact> found;
                       static_cast<void>(stored.contacts(found));
                       return core::toJson(std::span{found});
                   }));
               });

    router.add(core::ApiRoute{"GET", "/contacts/{id}", core::ApiAccess::Read, resource::kContacts,
                              "One contact"},
               [askStorage](const core::ApiRequest &request) {
                   const auto body =
                       askStorage([id = request.id](core::IContactStore &stored, core::ICallLog &) {
                           std::vector<core::Contact> found;
                           static_cast<void>(stored.contacts(found));
                           for (const auto &contact : found) {
                               if (contact.id == id) {
                                   return core::toJson(contact);
                               }
                           }
                           return std::string{};
                       });

                   if (!body) {
                       return core::apiError(503, "the phone did not answer in time");
                   }
                   if (body->empty()) {
                       return core::apiError(404, "no contact with that id");
                   }

                   core::ApiResponse response;
                   response.body = *body;
                   return response;
               });

    router.add(core::ApiRoute{"GET", "/history", core::ApiAccess::Read, resource::kHistory,
                              "Call history, newest first. ?limit=, ?missed=true and "
                              "?declined=true"},
               [askStorage](const core::ApiRequest &request) {
                   core::CallLogQuery query;

                   const auto limit = core::queryValue(request.query, "limit");
                   query.limit = limit.empty() ? 100 : std::max(1, std::atoi(limit.c_str()));
                   query.missedOnly = core::queryValue(request.query, "missed") == "true";

                   query.declinedOnly = core::queryValue(request.query, "declined") == "true";

                   return answered(askStorage([query](core::IContactStore &, core::ICallLog &log) {
                       std::vector<core::CallRecord> found;
                       static_cast<void>(log.calls(query, found));
                       return core::toJson(std::span{found});
                   }));
               });

    router.add(core::ApiRoute{"GET", "/calls", core::ApiAccess::Read, resource::kCalls,
                              "The calls happening now"},
               [askEngine, calls, configuration](const core::ApiRequest &) {
                   return answered(askEngine([calls, configuration]() {

                       std::vector<core::ApiCallView> live;
                       for (const auto &call : calls().calls()) {
                           const auto *account = core::find(configuration(), call.accountId);
                           live.push_back(
                               core::ApiCallView{call, account ? account->uuid : std::string{}});
                       }
                       return core::toJson(std::span{std::as_const(live)},
                                           std::chrono::system_clock::now());
                   }));
               });

    router.add(core::ApiRoute{"GET", "/accounts", core::ApiAccess::Read, resource::kAccounts,
                              "The SIP accounts and their registration state. Never any password"},

               [askEngine, configuration, registrationState](const core::ApiRequest &) {
                   return answered(askEngine([configuration, registrationState]() {
                       std::vector<core::ApiAccountView> lines;
                       for (const auto &account : configuration().accounts) {
                           lines.push_back(
                               core::ApiAccountView{account, registrationState(account.id)});
                       }
                       return core::toJson(std::span{lines});
                   }));
               });

    router.add(core::ApiRoute{"GET", "/settings", core::ApiAccess::Read, resource::kSettings,
                              "Everything configurable except the secrets"},
               [askEngine, settings](const core::ApiRequest &) {
                   return answered(askEngine([settings]() {
                       auto *controller = settings();
                       return controller ? settingsToJson(controller->effective())
                                         : std::string{"{}"};
                   }));
               });

    router.add(
        core::ApiRoute{"GET", "/taxonomy", core::ApiAccess::Read, resource::kSettings,
                       "The categories, tags and red flags, by code"},
        [askTaxonomy](const core::ApiRequest &request) {
            if (!askTaxonomy) {
                return core::apiError(503, "this build has no dictionaries");
            }

            const auto wanted = core::parseTaxonomyKind(core::queryValue(request.query, "kind"));

            return answered(askTaxonomy([wanted](core::ITaxonomyStore &stored) {
                std::vector<core::TaxonomyEntry> found;
                for (const auto kind : {core::TaxonomyKind::Category, core::TaxonomyKind::Tag,
                                        core::TaxonomyKind::RedFlag}) {
                    if (wanted.has_value() && *wanted != kind) {
                        continue;
                    }

                    static_cast<void>(stored.entries(kind, true, found));
                }
                return core::toJson(std::span{std::as_const(found)});
            }));
        });

    router.add(core::ApiRoute{"POST", "/calls", core::ApiAccess::Control, resource::kCalls,
                              "Place a call: {number, account_id?}"},
               [askEngine, calls, configuration](const core::ApiRequest &request) {
                   const auto number = core::stringFromJson(request.body, "number");
                   if (number.empty()) {
                       return core::apiError(400, "a call needs a number");
                   }
                   const auto wanted = core::stringFromJson(request.body, "account_id");

                   return accepted(askEngine([calls, configuration, number, wanted]() {

                       const auto &stored = configuration();
                       auto line = core::dialingAccount(stored);
                       for (const auto &account : stored.accounts) {
                           if (!wanted.empty() && account.enabled && account.uuid == wanted) {
                               line = account.id;
                           }
                       }
                       if (line.empty()) {
                           return refuse("this phone has no account to call from");
                       }

                       const auto *account = core::find(stored, line);
                       if (account == nullptr) {
                           return refuse("this phone has no account to call from");
                       }

                       const auto id =
                           calls().place(line, core::dialTarget(number, *account), number);
                       if (id.empty()) {
                           return refuse("the call could not be placed");
                       }

                       for (const auto &call : calls().calls()) {
                           if (call.id == id) {
                               return R"({"id":")" + call.uuid + R"("})";
                           }
                       }
                       return refuse("the call could not be placed");
                   }));
               });

    const auto onCall =
        [askEngine, calls](const std::string &id,
                           std::function<void(core::CallController &, const std::string &)> work) {
            return accepted(askEngine([calls, id, work]() {

                for (const auto &call : calls().calls()) {
                    if (call.uuid == id) {
                        work(calls(), call.id);
                        return std::string{};
                    }
                }
                return refuse("no call with that id");
            }));
        };

    router.add(core::ApiRoute{"POST", "/calls/{id}/answer", core::ApiAccess::Control,
                              resource::kCalls, "Answer it"},
               [onCall](const core::ApiRequest &request) {
                   return onCall(request.id, [](core::CallController &controller, const auto &id) {
                       controller.answer(id);
                   });
               });
    router.add(core::ApiRoute{"POST", "/calls/{id}/hangup", core::ApiAccess::Control,
                              resource::kCalls, "Hang it up"},
               [onCall](const core::ApiRequest &request) {
                   return onCall(request.id, [](core::CallController &controller, const auto &id) {
                       controller.hangUp(id);
                   });
               });
    router.add(core::ApiRoute{"POST", "/calls/{id}/hold", core::ApiAccess::Control,
                              resource::kCalls, "Put it on hold"},
               [onCall](const core::ApiRequest &request) {
                   return onCall(request.id, [](core::CallController &controller, const auto &id) {
                       controller.hold(id);
                   });
               });
    router.add(core::ApiRoute{"POST", "/calls/{id}/resume", core::ApiAccess::Control,
                              resource::kCalls, "Take it off hold"},
               [onCall](const core::ApiRequest &request) {
                   return onCall(request.id, [](core::CallController &controller, const auto &id) {
                       controller.activate(id);
                   });
               });

    router.add(core::ApiRoute{"POST", "/calls/{id}/dtmf", core::ApiAccess::Control,
                              resource::kCalls, "Send digits: {digits}"},
               [onCall](const core::ApiRequest &request) {
                   const auto digits = core::stringFromJson(request.body, "digits");
                   if (digits.empty()) {
                       return core::apiError(400, "no digits to send");
                   }
                   return onCall(request.id,
                                 [digits](core::CallController &controller, const auto &id) {
                                     for (const char digit : digits) {
                                         controller.sendDtmf(id, digit);
                                     }
                                 });
               });

    router.add(core::ApiRoute{"POST", "/calls/{id}/transfer", core::ApiAccess::Control,
                              resource::kCalls, "Transfer it: {target}"},
               [askEngine, calls, configuration](const core::ApiRequest &request) {
                   const auto target = core::stringFromJson(request.body, "target");
                   if (target.empty()) {
                       return core::apiError(400, "a transfer needs a target");
                   }
                   const auto id = request.id;
                   return accepted(askEngine([calls, configuration, id, target]() {
                       for (const auto &call : calls().calls()) {
                           if (call.uuid != id) {
                               continue;
                           }

                           const auto *account = core::find(configuration(), call.accountId);
                           if (account == nullptr) {
                               return refuse("the account this call is on is no longer set up");
                           }
                           calls().transferBlind(call.id, core::dialTarget(target, *account));
                           return std::string{};
                       }
                       return refuse("no call with that id");
                   }));
               });

    const auto saveContact = [askStorage, contactsChanged](core::Contact contact, int status) {
        auto answer = askStorage([contact](core::IContactStore &stored, core::ICallLog &) {
            auto saved = contact;
            if (stored.save(saved) != core::StoreStatus::Ok) {
                return refuse("the database would not take it");
            }
            return core::toJson(saved);
        });

        if (contactsChanged) {
            contactsChanged();
        }
        return accepted(std::move(answer), status);
    };

    router.add(core::ApiRoute{"POST", "/contacts", core::ApiAccess::Write, resource::kContacts,
                              "Add a contact: {name, number, account_id?}"},
               [saveContact](const core::ApiRequest &request) {
                   std::string problem;
                   auto contact = core::contactFromJson(request.body, problem);
                   if (!contact) {
                       return core::apiError(400, problem);
                   }

                   contact->id.clear();
                   return saveContact(*contact, 201);
               });

    router.add(core::ApiRoute{"PUT", "/contacts/{id}", core::ApiAccess::Write, resource::kContacts,
                              "Replace a contact"},
               [saveContact](const core::ApiRequest &request) {
                   std::string problem;
                   auto contact = core::contactFromJson(request.body, problem);
                   if (!contact) {
                       return core::apiError(400, problem);
                   }
                   contact->id = request.id;
                   return saveContact(*contact, 200);
               });

    router.add(core::ApiRoute{"DELETE", "/contacts/{id}", core::ApiAccess::Write,
                              resource::kContacts, "Remove a contact"},
               [askStorage, contactsChanged](const core::ApiRequest &request) {
                   auto answer =
                       askStorage([id = request.id](core::IContactStore &stored, core::ICallLog &) {
                           return stored.removeContact(id) == core::StoreStatus::Ok
                                      ? std::string{}
                                      : refuse("no contact with that id");
                       });
                   if (contactsChanged) {
                       contactsChanged();
                   }
                   return accepted(std::move(answer));
               });

    router.add(core::ApiRoute{"DELETE", "/history/{id}", core::ApiAccess::Write, resource::kHistory,
                              "Forget one call"},
               [askStorage, historyChanged](const core::ApiRequest &request) {
                   auto answer =
                       askStorage([id = request.id](core::IContactStore &, core::ICallLog &log) {
                           return log.removeCall(id) == core::StoreStatus::Ok
                                      ? std::string{}
                                      : refuse("no call with that id");
                       });
                   if (historyChanged) {
                       historyChanged();
                   }
                   return accepted(std::move(answer));
               });

    router.add(core::ApiRoute{"DELETE", "/history", core::ApiAccess::Write, resource::kHistory,
                              "Forget the whole history"},
               [askStorage, historyChanged](const core::ApiRequest &) {
                   auto answer = askStorage([](core::IContactStore &, core::ICallLog &log) {
                       return log.clearCalls() == core::StoreStatus::Ok
                                  ? std::string{}
                                  : refuse("the history could not be cleared");
                   });
                   if (historyChanged) {
                       historyChanged();
                   }
                   return accepted(std::move(answer));
               });

    router.add(core::ApiRoute{"PUT", "/accounts/{id}", core::ApiAccess::Write, resource::kAccounts,
                              "Replace an account"},
               [askEngine, calls, settings](const core::ApiRequest &request) {
                   return accepted(askEngine([calls, settings, request]() {
                       auto *controller = settings();
                       if (controller == nullptr) {
                           return refuse("the phone is not ready");
                       }

                       auto wanted = controller->desired();

                       const auto at =
                           std::ranges::find_if(wanted.accounts, [&request](const auto &account) {
                               return account.uuid == request.id;
                           });
                       if (at == wanted.accounts.end()) {
                           return refuse("no account with that id");
                       }

                       std::string problem;
                       auto account = core::accountFromJson(request.body, *at, problem);
                       if (!account) {
                           return refuse(problem);
                       }

                       core::upsert(wanted, *account);
                       static_cast<void>(controller->request(std::move(wanted), calls().calls()));
                       return std::string{};
                   }));
               });

    router.add(core::ApiRoute{"PUT", "/settings", core::ApiAccess::Write, resource::kSettings,
                              "Replace the settings"},
               [askEngine, calls, settings](const core::ApiRequest &request) {
                   return accepted(askEngine([calls, settings, request]() {
                       auto *controller = settings();
                       if (controller == nullptr) {
                           return refuse("the phone is not ready");
                       }

                       auto wanted = controller->desired();
                       std::string problem;
                       auto replacement = settingsFromJson(request.body, wanted, problem);
                       if (!replacement) {
                           return refuse(problem);
                       }

                       wanted = std::move(*replacement);

                       static_cast<void>(controller->request(std::move(wanted), calls().calls()));
                       return std::string{};
                   }));
               });
}

}
