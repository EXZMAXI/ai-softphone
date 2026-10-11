#!/usr/bin/env bash
#
# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

set -euo pipefail

destination=${1:-release-signing-key.pem}

if [[ -e ${destination} ]]; then
    echo "${destination} already exists. Refusing to overwrite a signing key." >&2
    exit 1
fi

umask 077
openssl genpkey -algorithm ed25519 -out "${destination}"
chmod 600 "${destination}"

public=$(openssl pkey -in "${destination}" -pubout -outform DER | tail -c 32 | base64 -w0)

echo "Private key: ${destination}  (mode 600 — keep it that way)"
echo
echo "Public key, for cmake/Updates.cmake or the packaging build:"
echo
echo "  ${public}"
echo
echo "Check that the pair works before you rely on it:"
echo "  scripts/sign-release.sh ${destination} --base-url https://example.test/"
