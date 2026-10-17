# AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
# https://ai-softphone.com
#
# SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
# SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This file is part of AI Softphone. There is NO WARRANTY, to the extent
# permitted by law. See the file LICENSE for the full terms.

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string[]]$Files,
    [string]$SigningHost = $env:SOFTPHONE_SIGNING_HOST,
    [string]$IdentityFile = (Join-Path $env:USERPROFILE '.ssh\id_ed25519'),

    [string]$Pkcs11Module = '/opt/SimplySignDesktop/SimplySignPKCS_64-MS-1.0.20.so',

    [string]$Pin = '1234',

    [string]$TimestampUrl = 'http://time.certum.pl/',
    [string]$Description = 'AI Softphone',
    [string]$DescriptionUrl = 'https://ai-softphone.com'
)

$ErrorActionPreference = 'Stop'

function Write-Log([string]$text) { Write-Host "[sign] $text" }

if (-not $SigningHost) {
    throw @'
No signing machine. Set SOFTPHONE_SIGNING_HOST to user@host of the machine that
holds the SimplySign session, or pass -SigningHost.

The key is in Certum's cloud, the card is only reachable from that machine, and
the session is opened there by hand.
'@
}

function Resolve-Tool([string]$name) {
    foreach ($where in @("$env:SystemRoot\System32\OpenSSH\$name",
                         "$env:SystemRoot\Sysnative\OpenSSH\$name")) {
        if (Test-Path $where) { return $where }
    }
    $onPath = Get-Command $name -ErrorAction SilentlyContinue
    if ($onPath) { return $onPath.Source }
    throw "no $name on this machine"
}

$ssh = Resolve-Tool 'ssh.exe'
$scp = Resolve-Tool 'scp.exe'
if (-not (Test-Path $IdentityFile)) { throw "no ssh key at $IdentityFile" }

function Remote([string]$script) {
    $unix = ("exec 2>&1`n" + $script) -replace "`r`n", "`n"

    $packed = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes($unix))
    & $ssh -i $IdentityFile -o BatchMode=yes $SigningHost "echo $packed | base64 -d | sh -s"
}

$wanted = New-Object System.Collections.ArrayList
foreach ($one in $Files) {
    if (Test-Path $one -PathType Container) {
        Get-ChildItem $one -Recurse -Include *.exe, *.dll -File | ForEach-Object {
            [void]$wanted.Add($_.FullName)
        }
    } elseif (Test-Path $one) {
        [void]$wanted.Add((Resolve-Path $one).Path)
    } else {
        throw "nothing at $one"
    }
}
if ($wanted.Count -eq 0) { throw 'nothing to sign' }

$prepare = @'
set -e
mkdir -p ~/.softphone-signing
MOD=__MODULE__
SOCK=/run/user/$(id -u)/p11-kit/softphone-signing.sock

if [ ! -S "$SOCK" ]; then
    mkdir -p "$(dirname "$SOCK")"
    setsid p11-kit server --provider "$MOD" -n "$SOCK" 'pkcs11:' </dev/null >/tmp/p11-softphone.log 2>&1 &
    sleep 3
fi
[ -S "$SOCK" ] || { echo "no p11-kit socket at $SOCK"; exit 1; }

if [ ! -s /tmp/ccsca2021.pem ]; then
    curl -sSfL -o /tmp/ccsca2021.pem https://repository.certum.pl/ccsca2021.pem
fi
openssl x509 -in /tmp/ccsca2021.pem -noout -subject

P11_KIT_SERVER_ADDRESS="unix:path=$SOCK"; export P11_KIT_SERVER_ADDRESS
pkcs11-tool --module /usr/lib/x86_64-linux-gnu/pkcs11/p11-kit-client.so -O --type cert 2>/dev/null |
    grep -E 'subject|ID:' | head -4 || true
'@ -replace '__MODULE__', $Pkcs11Module

Write-Log "the signing machine is $SigningHost"
$ready = Remote $prepare
$ready | ForEach-Object { Write-Log "  $_" }
if ($LASTEXITCODE -ne 0) { throw 'the signing machine is not ready; see above' }
if (-not ($ready -match 'Certificate Object|subject')) {
    throw @'
The card holds no certificate this run can see. The SimplySign session has
probably expired: open SimplySign Desktop on that machine and log in again.
'@
}

$signOne = (
    'set -e; ' +
    'export P11_KIT_SERVER_ADDRESS="unix:path=/run/user/$(id -u)/p11-kit/softphone-signing.sock"; ' +
    'W=$HOME/.softphone-signing/__STEM__; ' +
    'osslsigncode sign ' +
        '-pkcs11module /usr/lib/x86_64-linux-gnu/pkcs11/p11-kit-client.so ' +
        "-pkcs11cert 'pkcs11:token=Code%20Signing;type=cert' " +
        "-key 'pkcs11:token=Code%20Signing;type=private' " +
        '-ac /tmp/ccsca2021.pem -h sha256 -ts __TIMESTAMP__ ' +
        "-n '__NAME__' -i '__URL__' -pass __PIN__ " +
        '-in "$W" -out "$W.signed" >/dev/null; ' +
    'mv "$W.signed" "$W"; ' +
    'osslsigncode verify -in "$W" 2>/dev/null | grep -q Pleshchinskiy'
)

Write-Log "$($wanted.Count) file(s) to sign"
$signed = 0
$already = 0
$began = Get-Date

foreach ($file in $wanted) {
    if ((Get-AuthenticodeSignature $file).Status -eq 'Valid') { $already++; continue }

    $stem = [System.Guid]::NewGuid().ToString('N')
    $there = ".softphone-signing/$stem"

    & $scp -q -i $IdentityFile -o BatchMode=yes $file "${SigningHost}:$there"
    if ($LASTEXITCODE -ne 0) { throw "could not send $file" }

    $line = $signOne -replace '__STEM__', $stem -replace '__TIMESTAMP__', $TimestampUrl `
                     -replace '__NAME__', $Description -replace '__URL__', $DescriptionUrl `
                     -replace '__PIN__', $Pin
    $said = & $ssh -i $IdentityFile -o BatchMode=yes $SigningHost "exec 2>&1; $line"
    if ($LASTEXITCODE -ne 0) {
        $said | ForEach-Object { Write-Log "  $_" }
        throw "signing failed for $file"
    }

    & $scp -q -i $IdentityFile -o BatchMode=yes "${SigningHost}:$there" $file
    if ($LASTEXITCODE -ne 0) { throw "could not bring $file back" }
    & $ssh -i $IdentityFile -o BatchMode=yes $SigningHost "rm -f `$HOME/$there" | Out-Null

    $signed++
    if ($signed % 10 -eq 0) { Write-Log "  $signed signed" }
}

Write-Log ("$signed signed, $already already carried a signature, " +
           "$([int]((Get-Date) - $began).TotalSeconds) s")

$bad = @()
foreach ($file in $wanted) {
    if ((Get-AuthenticodeSignature $file).Status -ne 'Valid') { $bad += $file }
}
if ($bad.Count -gt 0) {
    $bad | ForEach-Object { Write-Log "  does not verify: $_" }
    throw "signed, and $($bad.Count) file(s) do not verify on this machine"
}

Write-Log "all $($wanted.Count) verify as Valid to Windows"
