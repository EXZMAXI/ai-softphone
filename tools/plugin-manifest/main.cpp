// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/container/loader.hpp>
#include <softphone/container/loading.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

int main(int argc, char **argv)
{
    if (argc != 3) {
        std::fputs("usage: plugin-manifest <library> <output>\n", stderr);
        return 2;
    }

    namespace container = softphone::container;

    const std::filesystem::path library{argv[1]};
    void *const handle = container::dynamic::open(library);
    if (handle == nullptr) {
        std::fprintf(stderr, "%s: %s\n", argv[1], container::dynamic::lastError().c_str());
        return 1;
    }

    const auto ask = reinterpret_cast<container::EntryPoint>(
        container::dynamic::symbol(handle, container::kEntryPoint));
    if (ask == nullptr) {
        std::fprintf(stderr, "%s: no %s — is SOFTPHONE_PLUGIN missing from its plugin.cpp?\n",
                     argv[1], container::kEntryPoint);
        container::dynamic::close(handle);
        return 1;
    }

    const container::PluginEntry *const entry = ask();
    if (entry == nullptr || entry->manifest == nullptr) {
        std::fprintf(stderr, "%s: its entry point has no manifest\n", argv[1]);
        container::dynamic::close(handle);
        return 1;
    }

    const std::string document = entry->manifest();
    container::dynamic::close(handle);

    const std::filesystem::path output{argv[2]};
    std::error_code ignored;
    std::filesystem::create_directories(output.parent_path(), ignored);

    std::ofstream out{output, std::ios::trunc};
    if (!out) {
        std::fprintf(stderr, "%s: could not be written\n", argv[2]);
        return 1;
    }
    out << document << '\n';
    return out ? 0 : 1;
}
