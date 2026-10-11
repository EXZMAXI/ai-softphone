// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/loading.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::container {
namespace {

struct Field {
    std::string_view name;
    std::string BuildKey::*where;
};

const std::array<Field, 6> kFields{{
    {"container", &BuildKey::container},
    {"compiler", &BuildKey::compiler},
    {"stdlib", &BuildKey::standardLibrary},
    {"system", &BuildKey::system},
    {"machine", &BuildKey::machine},
    {"qt", &BuildKey::qt},
}};

}

std::string toString(const BuildKey &key)
{
    std::string text;
    for (const auto &field : kFields) {
        if (!text.empty()) {
            text.push_back(';');
        }
        text.append(field.name);
        text.push_back('=');
        text.append(key.*field.where);
    }
    return text;
}

BuildKey parseBuildKey(std::string_view text)
{
    BuildKey key;
    while (!text.empty()) {
        const auto end = text.find(';');
        const auto pair = text.substr(0, end);
        text = end == std::string_view::npos ? std::string_view{} : text.substr(end + 1);

        const auto equals = pair.find('=');
        if (equals == std::string_view::npos) {
            continue;
        }
        const auto name = pair.substr(0, equals);
        const auto value = pair.substr(equals + 1);

        const auto known = std::ranges::find(kFields, name, &Field::name);
        if (known != kFields.end()) {
            key.*(known->where) = std::string{value};
        }
    }
    return key;
}

std::string mismatch(const BuildKey &host, const BuildKey &plugin)
{
    std::vector<std::string> differences;
    for (const auto &field : kFields) {
        const auto &ours = host.*field.where;
        const auto &theirs = plugin.*field.where;
        if (ours == theirs) {
            continue;
        }

        differences.push_back(std::string{field.name} + " is " +
                              (theirs.empty() ? std::string{"missing"} : theirs) +
                              " and this phone is " + (ours.empty() ? std::string{"none"} : ours));
    }

    if (differences.empty()) {
        return {};
    }

    std::string sentence = "it was built somewhere else: ";
    for (std::size_t index = 0; index < differences.size(); ++index) {
        if (index != 0) {
            sentence += index + 1 == differences.size() ? ", and " : ", ";
        }
        sentence += differences[index];
    }
    return sentence;
}

}
