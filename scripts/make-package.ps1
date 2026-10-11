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
    [switch] $SkipCheck,
    [switch] $KeepBuildTree,

    [switch] $Sign,
    [string] $SigningHost = $env:SOFTPHONE_SIGNING_HOST
)

$ErrorActionPreference = 'Stop'

$signingArguments = @()
if ($SigningHost) { $signingArguments += @('-SigningHost', $SigningHost) }
$ProgressPreference = 'SilentlyContinue'

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Build = Join-Path $Root 'build\package'
$Dist = Join-Path $Root 'dist\windows'
$Staging = Join-Path $Build 'staging\windows'

$InnoVersion = '6.7.3'
$InnoSha256 = '9c73c3bae7ed48d44112a0f48e66742c00090bdb5bef71d9d3c056c66e97b732'
$InnoUrl = "https://github.com/jrsoftware/issrc/releases/download/is-6_7_3/innosetup-$InnoVersion.exe"

if ($env:SOFTPHONE_CACHE_DIR) {
    $CacheRoot = $env:SOFTPHONE_CACHE_DIR
} else {
    $CacheRoot = Join-Path $env:LOCALAPPDATA 'softphone-ai-deps'
}

function Write-Log([string] $Message) {
    Write-Host "[package] $Message" -ForegroundColor White
}

if (-not $env:VCINSTALLDIR) {
    Write-Log 'bringing in the compiler environment'

    $environment = & cmd /c "call `"$Root\scripts\windows-build-environment.cmd`" >nul && set"
    if ($LASTEXITCODE -ne 0) {
        Write-Error 'No Visual Studio with the C++ build tools on this machine.'
        exit 1
    }

    foreach ($line in $environment) {
        if ($line -match '^([^=]+)=(.*)$') {
            [Environment]::SetEnvironmentVariable($Matches[1], $Matches[2])
        }
    }

    if (-not $env:VCINSTALLDIR) {
        Write-Error 'The compiler environment did not set VCINSTALLDIR.'
        exit 1
    }
}

function Resolve-Iscc {
    $candidates = @()
    $onPath = Get-Command ISCC.exe -ErrorAction SilentlyContinue
    if ($onPath) { $candidates += $onPath.Source }
    $candidates += (Join-Path $CacheRoot "innosetup-$InnoVersion\ISCC.exe")
    $candidates += "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe"
    $candidates += "$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe"

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) { return $candidate }
    }
    return $null
}

$Iscc = Resolve-Iscc

if (-not $Iscc) {
    Write-Log "no Inno Setup on this machine; fetching $InnoVersion"

    $downloads = Join-Path $CacheRoot 'downloads'
    New-Item -ItemType Directory -Force $downloads | Out-Null
    $installer = Join-Path $downloads "innosetup-$InnoVersion.exe"

    if (-not (Test-Path $installer)) {
        Invoke-WebRequest $InnoUrl -OutFile $installer -UseBasicParsing
    }

    $actual = (Get-FileHash $installer -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actual -ne $InnoSha256) {
        Write-Error @"
Checksum mismatch for $installer.
  expected $InnoSha256
  got      $actual
Delete it and re-run, or update `$InnoSha256 if the pin changed.
"@
        exit 1
    }

    $target = Join-Path $CacheRoot "innosetup-$InnoVersion"
    $process = Start-Process -FilePath $installer -PassThru -ArgumentList `
        '/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-', '/CURRENTUSER', '/NOICONS',
        "/DIR=`"$target`""
    if (-not $process.WaitForExit(300000)) {
        $process.Kill()
        Write-Error 'The Inno Setup installer did not finish in five minutes.'
        exit 1
    }

    $Iscc = Resolve-Iscc
    if (-not $Iscc) {
        Write-Error "Installed Inno Setup to $target and still cannot find ISCC.exe."
        exit 1
    }
}

Write-Log "compiler: $Iscc"

Write-Log 'staging the tree'

if (Test-Path $Staging) { Remove-Item $Staging -Recurse -Force }

if (-not $KeepBuildTree -and (Test-Path $Build)) {
    Write-Log 'emptying the build tree a release is cut from'
    Remove-Item $Build -Recurse -Force
}

if (-not (Test-Path (Join-Path $Build 'CMakeCache.txt'))) {
    & cmake --preset package | Out-Null
    if ($LASTEXITCODE -ne 0) { Write-Error 'configure failed'; exit 1 }
}

& cmake -S $Root -B $Build `
    -DCMAKE_INSTALL_BINDIR=bin `
    -DSOFTPHONE_PACKAGE_FORMAT=installer | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error 'configure failed'; exit 1 }

& cmake --build $Build | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error 'build failed'; exit 1 }

& cmake --install $Build --prefix $Staging | Out-Null
if ($LASTEXITCODE -ne 0) { Write-Error 'install failed'; exit 1 }

$notes = Join-Path $Root 'packaging\windows\licence-note'
$licences = Join-Path $Staging 'licence'
New-Item -ItemType Directory -Force $licences | Out-Null

$gpl = Get-Content (Join-Path $Staging 'LICENSE.txt') -Raw -Encoding UTF8
$made = 0
foreach ($note in Get-ChildItem $notes -Filter *.txt) {
    $text = (Get-Content $note.FullName -Raw -Encoding UTF8) + "`r`n" + ('-' * 78) + "`r`n`r`n" + $gpl
    Set-Content -Path (Join-Path $licences $note.Name) -Value $text -Encoding utf8
    $made++
}
if ($made -eq 0) { Write-Error "no licence notes under $notes"; exit 1 }
Write-Log "$made licence files, one per language"

if ($Sign) {
    & powershell -NoProfile -ExecutionPolicy Bypass `
        -File (Join-Path $Root 'scripts\sign-windows.ps1') `
        -Files $Staging @signingArguments
    if ($LASTEXITCODE -ne 0) { Write-Error 'signing the staged tree failed'; exit 1 }
}

$Version = (Select-String -Path (Join-Path $Build 'CMakeCache.txt') `
    -Pattern '^CMAKE_PROJECT_VERSION:STATIC=(.+)$').Matches[0].Groups[1].Value
if (-not $Version) {
    Write-Error "Could not read the version out of $Build\CMakeCache.txt"
    exit 1
}
Write-Log "version $Version"

$Tree = $Staging
if (-not (Test-Path (Join-Path $Tree 'bin\ai-softphone.exe'))) {
    Write-Error "The install produced no bin\ai-softphone.exe under $Tree"
    exit 1
}

if (-not $SkipCheck) {
    & powershell -ExecutionPolicy Bypass -File (Join-Path $Root 'scripts\check-package-tree.ps1') $Tree
    if ($LASTEXITCODE -ne 0) {
        Write-Error 'the staged tree did not pass its check; nothing was packaged'
        exit 1
    }
}

New-Item -ItemType Directory -Force $Dist | Out-Null

Write-Log 'compiling the installer'

$script = Join-Path $Root 'packaging\windows\ai-softphone.iss'
$icon = Join-Path $Root 'ui\icons\ai-softphone.ico'

$innoSigning = @()
if ($Sign) {
    $signer = Join-Path $Root 'scripts\sign-windows.ps1'
    $line = ('powershell.exe -NoProfile -ExecutionPolicy Bypass -File $q' + $signer +
             '$q -Files $f')
    if ($SigningHost) { $line += " -SigningHost $q$SigningHost$q" }
    $innoSigning = @('/DSignWith=1', "/Ssoftphone=$line")
}

& $Iscc `
    @innoSigning `
    "/DAppVersion=$Version" `
    "/DStagingDir=$Tree" `
    "/DOutputDir=$Dist" `
    "/DIconFile=$icon" `
    $script
if ($LASTEXITCODE -ne 0) { Write-Error 'ISCC failed'; exit 1 }

$package = Join-Path $Dist "ai-softphone-$Version-x64.exe"
if (-not (Test-Path $package)) {
    Write-Error "ISCC reported success but $package is not there"
    exit 1
}

Write-Log $package
Get-ChildItem $Dist | Select-Object Name, Length, LastWriteTime
