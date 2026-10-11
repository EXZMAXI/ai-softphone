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

#include <softphone/core/http.hpp>

namespace softphone::http {

class HttplibHttpClient final : public core::IHttpClient {
public:
    HttplibHttpClient();
    ~HttplibHttpClient() override;

    [[nodiscard]] core::HttpResponse send(const core::HttpRequest &request,
                                          const core::HttpCancel &cancelled) override;

    [[nodiscard]] static bool supportsTls() noexcept;
};

}
