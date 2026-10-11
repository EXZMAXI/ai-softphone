// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/progress.hpp>

#include <softphone/core/logging.hpp>
#include <softphone/core/paths.hpp>

#include <nlohmann/json.hpp>

#include <fstream>
#include <system_error>
#include <utility>

namespace softphone::core {
namespace {

constexpr std::string_view kArea{"progress"};

constexpr std::size_t kMostDeeds = 512;

}

ProgressStore::ProgressStore(std::filesystem::path file) : file_{std::move(file)}
{
    std::ifstream in{file_};
    if (!in) {
        return;
    }

    nlohmann::json document = nlohmann::json::parse(in, nullptr, false);
    if (document.is_discarded() || !document.is_object() || !document.contains("done") ||
        !document["done"].is_array()) {
        Logger::instance().log(LogLevel::Debug, kArea,
                               "the progress file could not be read; starting again");
        return;
    }

    for (const auto &entry : document["done"]) {
        if (!entry.is_string() || done_.size() >= kMostDeeds) {
            continue;
        }
        auto code = entry.get<std::string>();
        if (!code.empty()) {
            done_.insert(std::move(code));
        }
    }
}

bool ProgressStore::done(const std::string &code) const
{
    return done_.contains(code);
}

void ProgressStore::note(const std::string &code)
{
    if (code.empty() || done_.size() >= kMostDeeds || !done_.insert(code).second) {
        return;
    }

    write();
}

std::vector<std::string> ProgressStore::all() const
{
    return {done_.begin(), done_.end()};
}

void ProgressStore::write()
{
    nlohmann::json document;
    document["done"] = all();

    std::error_code error;
    std::filesystem::create_directories(file_.parent_path(), error);

    const auto temporary = std::filesystem::path{file_}.concat(".new");
    {
        std::ofstream out{temporary, std::ios::trunc};
        if (!out) {

            Logger::instance().log(LogLevel::Debug, kArea, "the progress file cannot be written");
            return;
        }
        out << document.dump(2) << '\n';
        if (!out) {
            return;
        }
    }

    static_cast<void>(restrictToOwner(temporary));
    std::filesystem::rename(temporary, file_, error);
    if (error) {
        std::filesystem::remove(temporary, error);
    }
}

}
