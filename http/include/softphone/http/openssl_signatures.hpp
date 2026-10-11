// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <softphone/core/updates.hpp>

namespace softphone::http {

class OpenSslSignatures final : public core::IUpdateSignatures {
public:
    OpenSslSignatures();
    ~OpenSslSignatures() override;

    [[nodiscard]] bool verify(std::string_view document, std::string_view signature,
                              std::string_view publicKey) override;

    [[nodiscard]] std::string digest(const std::filesystem::path &file) override;
};

}
