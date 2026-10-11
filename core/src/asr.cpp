// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/core/asr.hpp>

#include <string>
#include <string_view>

namespace softphone::core {
namespace {

constexpr std::string_view kStemPrefix{"stem:"};

}

std::string_view toString(Voice voice) noexcept
{
    switch (voice) {
    case Voice::Near:
        return "near";
    case Voice::Far:
        return "far";
    case Voice::Mixed:
        return "mixed";
    case Voice::Stem:
        return "stem";
    }
    return "mixed";
}

std::string_view speakerWord(Voice voice, std::string_view language) noexcept
{

    struct Said {
        std::string_view code;
        std::string_view ours;
        std::string_view theirs;
        std::string_view guest;
    };

    static constexpr Said kSaid[] = {
        {"en", "Operator", "Caller", "Participant"},
        {"ru", "Оператор", "Собеседник", "Участник"},
        {"de", "Agent", "Gesprächspartner", "Teilnehmer"},
        {"fr", "Opérateur", "Interlocuteur", "Participant"},
        {"es", "Operador", "Interlocutor", "Participante"},
        {"it", "Operatore", "Interlocutore", "Partecipante"},
        {"pt", "Operador", "Interlocutor", "Participante"},
        {"nl", "Medewerker", "Gesprekspartner", "Deelnemer"},
        {"sv", "Operatör", "Motpart", "Deltagare"},
        {"da", "Medarbejder", "Modpart", "Deltager"},
        {"nb", "Operatør", "Motpart", "Deltaker"},
        {"fi", "Operaattori", "Keskustelukumppani", "Osallistuja"},
        {"pl", "Operator", "Rozmówca", "Uczestnik"},
        {"cs", "Operátor", "Protistrana", "Účastník"},
        {"sk", "Operátor", "Protistrana", "Účastník"},
        {"hu", "Ügyintéző", "Beszélgetőpartner", "Résztvevő"},
        {"ro", "Operator", "Interlocutor", "Participant"},
        {"bg", "Оператор", "Събеседник", "Участник"},
        {"hr", "Operater", "Sugovornik", "Sudionik"},
        {"sl", "Operater", "Sogovornik", "Udeleženec"},
        {"et", "Operaator", "Vestluskaaslane", "Osaleja"},
        {"lv", "Operators", "Sarunu partneris", "Dalībnieks"},
        {"lt", "Operatorius", "Pašnekovas", "Dalyvis"},
        {"el", "Χειριστής", "Συνομιλητής", "Συμμετέχων"},
        {"tr", "Operatör", "Görüşülen kişi", "Katılımcı"},
        {"uk", "Оператор", "Співрозмовник", "Учасник"},
        {"id", "Operator", "Lawan bicara", "Peserta"},
        {"vi", "Nhân viên", "Người đối thoại", "Người tham gia"},
        {"sr", "Оператер", "Саговорник", "Учесник"},
        {"sr_Latn", "Operater", "Sagovornik", "Učesnik"},
        {"zh", "接线员", "对话者", "参与者"},
        {"ja", "オペレーター", "相手", "参加者"},
        {"ko", "상담원", "상대방", "참가자"},
        {"ar", "الموظف", "المتحدث الآخر", "المشارك"},
        {"he", "נציג", "הצד השני", "משתתף"},
        {"hi", "ऑपरेटर", "वार्ताकार", "प्रतिभागी"},
    };

    const Said *said = &kSaid[0];
    for (const auto &one : kSaid) {
        if (one.code == language) {
            said = &one;
            break;
        }
    }

    switch (voice) {
    case Voice::Near:
        return said->ours;
    case Voice::Far:
    case Voice::Mixed:
        return said->theirs;
    case Voice::Stem:
        return said->guest;
    }
    return said->theirs;
}

std::string voiceText(Voice voice, std::string_view stemId)
{
    if (voice == Voice::Stem) {
        return std::string{kStemPrefix} + std::string{stemId};
    }
    return std::string{toString(voice)};
}

std::optional<std::pair<Voice, std::string>> parseVoice(std::string_view text)
{
    if (text == "near") {
        return std::pair{Voice::Near, std::string{}};
    }
    if (text == "far") {
        return std::pair{Voice::Far, std::string{}};
    }
    if (text == "mixed") {
        return std::pair{Voice::Mixed, std::string{}};
    }
    if (text.size() > kStemPrefix.size() && text.substr(0, kStemPrefix.size()) == kStemPrefix) {
        return std::pair{Voice::Stem, std::string{text.substr(kStemPrefix.size())}};
    }
    return std::nullopt;
}

std::string_view toString(AsrStatus status) noexcept
{
    switch (status) {
    case AsrStatus::Ok:
        return "ok";
    case AsrStatus::Unauthorised:
        return "unauthorised";
    case AsrStatus::Unsupported:
        return "unsupported";
    case AsrStatus::TooLarge:
        return "too-large";
    case AsrStatus::Unavailable:
        return "unavailable";
    case AsrStatus::Timeout:
        return "timeout";
    case AsrStatus::Malformed:
        return "malformed";
    case AsrStatus::Cancelled:
        return "cancelled";
    case AsrStatus::Failed:
        return "failed";
    }
    return "failed";
}

bool worthRetrying(AsrStatus status) noexcept
{
    switch (status) {
    case AsrStatus::Unavailable:
    case AsrStatus::Timeout:
        return true;
    case AsrStatus::Ok:
    case AsrStatus::Unauthorised:
    case AsrStatus::Unsupported:
    case AsrStatus::TooLarge:
    case AsrStatus::Malformed:
    case AsrStatus::Cancelled:
    case AsrStatus::Failed:
        return false;
    }
    return false;
}

}
