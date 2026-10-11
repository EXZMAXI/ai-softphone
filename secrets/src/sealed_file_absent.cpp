// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/secrets/sealed_file_store.hpp>

namespace softphone::secrets {

std::unique_ptr<ICredentialStore> makeSealedFileStore(std::filesystem::path, std::string)
{
    return {};
}

std::string sealedBinding(std::string_view installationId)
{
    return std::string{installationId};
}

}
