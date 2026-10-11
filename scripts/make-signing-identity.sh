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

NAME="${SOFTPHONE_SIGNING_IDENTITY:-AI Softphone Development}"
KEYCHAIN="${HOME}/Library/Keychains/login.keychain-db"
YEARS=10

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This is a macOS script: code signing is what macOS calls what the other" >&2
    echo "two systems do not have." >&2
    exit 1
fi

if security find-identity -v -p codesigning | grep -qF "$NAME"; then
    echo "\"$NAME\" is already an identity on this machine."
    security find-identity -v -p codesigning | grep -F "$NAME"
    echo
    echo "Nothing to do. Configure with it by hand if a build is not using it:"
    echo "  cmake --preset dev -D SOFTPHONE_CODESIGN_IDENTITY=\"$NAME\""
    exit 0
fi

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT

cat > "$work/openssl.cnf" <<CONFIG
[ req ]
distinguished_name = subject
x509_extensions    = codesign
prompt             = no

[ subject ]
CN = ${NAME}

[ codesign ]
basicConstraints       = critical,CA:false
keyUsage               = critical,digitalSignature
extendedKeyUsage       = critical,codeSigning
subjectKeyIdentifier   = hash
CONFIG

echo "Making a self-signed code-signing certificate for \"$NAME\"…"
openssl req -x509 -newkey rsa:2048 -sha256 -nodes \
    -days "$((YEARS * 365))" \
    -config "$work/openssl.cnf" \
    -keyout "$work/key.pem" -out "$work/cert.pem" >/dev/null 2>&1

passphrase="$(LC_ALL=C tr -dc 'A-Za-z0-9' </dev/urandom | head -c 32)"
openssl pkcs12 -export \
    -inkey "$work/key.pem" -in "$work/cert.pem" \
    -name "$NAME" -out "$work/identity.p12" -passout "pass:$passphrase"

echo "Importing it into the login keychain…"
security import "$work/identity.p12" -k "$KEYCHAIN" -f pkcs12 -P "$passphrase" -A >/dev/null

echo "Trusting it for code signing — macOS will ask for your login password."
security add-trusted-cert -p codeSign -k "$KEYCHAIN" "$work/cert.pem"

echo
if security find-identity -v -p codesigning | grep -qF "$NAME"; then
    security find-identity -v -p codesigning | grep -F "$NAME"
    echo
    echo "Configure a build directory again and it will use it:"
    echo "  cmake --preset dev && cmake --build --preset dev"
else
    echo "The certificate went in but is not being offered as a code-signing" >&2
    echo "identity. Check Keychain Access for \"$NAME\" and what it is trusted for." >&2
    exit 1
fi
