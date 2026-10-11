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
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $Tree
)

$ErrorActionPreference = 'Stop'

$Tree = (Resolve-Path $Tree).Path
$failures = 0

function Complain([string] $Message) {
    Write-Host "  FAIL  $Message" -ForegroundColor Red
    $script:failures++
}

function Approve([string] $Message) {
    Write-Host "  ok    $Message"
}

Write-Host "Checking $Tree"

$binDirectory = $Tree
if (Test-Path (Join-Path $Tree 'bin\ai-softphone.exe')) {
    $binDirectory = Join-Path $Tree 'bin'
}

$sideDirectory = $binDirectory
if (Test-Path (Join-Path $Tree 'plugins')) {
    $sideDirectory = $Tree
}

function Invoke-InAnEmptyEnvironment([string] $Executable) {
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $Executable
    $info.Arguments = '--version'
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true

    $info.WorkingDirectory = [System.IO.Path]::GetTempPath()

    $info.EnvironmentVariables.Clear()
    $info.EnvironmentVariables['SystemRoot'] = $env:SystemRoot
    $info.EnvironmentVariables['windir'] = $env:SystemRoot
    $info.EnvironmentVariables['PATH'] = "$env:SystemRoot\system32;$env:SystemRoot"
    $info.EnvironmentVariables['TEMP'] = [System.IO.Path]::GetTempPath()
    $info.EnvironmentVariables['TMP'] = [System.IO.Path]::GetTempPath()

    $process = [System.Diagnostics.Process]::Start($info)
    $out = $process.StandardOutput.ReadToEnd()
    $err = $process.StandardError.ReadToEnd()
    $process.WaitForExit()

    return [pscustomobject]@{
        ExitCode = $process.ExitCode
        Output   = ($out + $err).Trim()
    }
}

foreach ($binary in @('ai-softphone.exe', 'ai-softphone-cli.exe')) {
    $path = Join-Path $binDirectory $binary
    if (-not (Test-Path $path)) {
        Complain "$binary is missing"
        continue
    }

    $run = Invoke-InAnEmptyEnvironment $path
    if ($run.ExitCode -ne 0) {
        Complain "$binary --version failed with $($run.ExitCode): $($run.Output)"
    } else {
        Approve "$binary starts and says: $($run.Output)"
    }
}

foreach ($library in @('Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Qml.dll', 'Qt6Quick.dll',
                       'Qt6Network.dll', 'Qt6Widgets.dll')) {
    if (Test-Path (Join-Path $binDirectory $library)) {
        Approve $library
    } else {
        Complain "$library is missing: this tree borrows Qt from the machine it is on"
    }
}

foreach ($library in @('softphone_container.dll', 'softphone_core.dll',
                       'softphone_http.dll', 'softphone_media.dll',
                       'softphone_secrets.dll', 'softphone_sip.dll',
                       'softphone_storage.dll',
                       'softphone_plugin_diagnostics.dll', 'softphone_plugin_integration.dll',
                       'softphone_plugin_monolith.dll', 'softphone_plugin_realtime.dll',
                       'softphone_plugin_softphone.dll', 'softphone_plugin_storage.dll',
                       'softphone_plugin_diagnostics_ui.dll', 'softphone_plugin_integration_ui.dll',
                       'softphone_plugin_softphone_ui.dll')) {
    if (Test-Path (Join-Path $binDirectory $library)) {
        Approve $library
    } else {
        Complain "$library is missing: the phone will not start on a machine that never built it"
    }
}

$pluginNames = & python3 (Join-Path $PSScriptRoot 'plugins-in-files.py')
if ($LASTEXITCODE -ne 0 -or -not $pluginNames) {
    Complain 'could not work out which plugins ship, so none of them was checked'
    $pluginNames = @()
}
foreach ($plugin in $pluginNames) {
    $directory = Join-Path $Tree "lib\ai-softphone\plugins\$plugin"
    $manifest = Join-Path $directory 'manifest.json'
    $library = Join-Path $directory "softphone_plugin_$plugin.dll"

    $face = Join-Path $directory "softphone_plugin_${plugin}_ui.dll"

    if (-not (Test-Path $manifest)) {
        Complain "the $plugin plugin has no manifest.json"
    } elseif (-not (Test-Path $library)) {
        Complain "the $plugin plugin has a manifest and no library"
    } elseif (-not (Test-Path $face)) {
        Complain "the $plugin plugin has no interface half: it will contribute nothing to any window"
    } else {
        Approve "the $plugin plugin is a pair of files the phone can open"
    }
}

foreach ($plugin in @('platforms\qwindows.dll',
                      'styles\qmodernwindowsstyle.dll',
                      'tls\qschannelbackend.dll',
                      'imageformats\qsvg.dll',
                      'iconengines\qsvgicon.dll')) {
    if (Test-Path (Join-Path $sideDirectory "plugins\$plugin")) {
        Approve "plugins\$plugin"
    } else {
        Complain "$plugin is missing"
    }
}

$moduleCheck = Join-Path $PSScriptRoot 'check-packaged-qml-modules.py'
& python3 $moduleCheck $sideDirectory
if ($LASTEXITCODE -ne 0) {
    Complain 'a QML module the product imports is not in the tree: the phone would not start'
} else {
    Approve 'every QML module the product imports is in the tree'
}

foreach ($carried in @('vcruntime140.dll', 'vcruntime140_1.dll', 'msvcp140.dll',
                       'libcrypto-3-x64.dll', 'libssl-3-x64.dll',
                       'dxcompiler.dll', 'dxil.dll')) {
    if (Test-Path (Join-Path $binDirectory $carried)) {
        Approve $carried
    } else {
        Complain "$carried is missing"
    }
}

$symbols = Join-Path $binDirectory 'ai-softphone.pdb'
if (Test-Path $symbols) {
    Approve ("ai-softphone.pdb ({0:N0} bytes)" -f (Get-Item $symbols).Length)
} else {
    Complain 'ai-softphone.pdb is missing: a crash dump from this build would name nothing'
}

if ((Test-Path (Join-Path $Tree 'LICENSE.txt')) -or
    (Test-Path (Join-Path $binDirectory 'LICENSE.txt'))) {
    Approve 'LICENSE.txt'
} else {
    Complain 'LICENSE.txt is missing'
}

$qtConf = Join-Path $binDirectory 'qt.conf'
if (Test-Path $qtConf) {
    $contents = Get-Content $qtConf -Raw
    if ($contents -match '(?m)^\s*Prefix\s*=\s*[A-Za-z]:') {
        Complain "qt.conf names an absolute prefix: $($contents.Trim())"
    } else {
        Approve 'qt.conf is beside the binaries and names nothing absolute'
    }
} else {
    Complain 'qt.conf is missing: the bundled Qt is not private to this tree'
}

$catalogueSource = Join-Path $PSScriptRoot '..\ui\src\translations.cpp'
$translationsDirectory = Join-Path $Tree 'translations'

if (-not (Test-Path $translationsDirectory)) {
    Complain 'translations/ is missing: every standard dialog button would be English'
} elseif ((Get-Content $catalogueSource -Raw) -notmatch 'kQtCatalogues\[\]\s*=\s*\{([^}]*)\}') {
    Complain "could not read kQtCatalogues out of $catalogueSource"
} else {
    $asked = [regex]::Matches($Matches[1], '"([^"]+)"') | ForEach-Object { $_.Groups[1].Value }
    $found = @{}
    foreach ($name in $asked) {
        $found[$name] = @(Get-ChildItem $translationsDirectory -Filter "$name`_*.qm" `
                              -ErrorAction SilentlyContinue).Count
    }
    $total = ($found.Values | Measure-Object -Sum).Sum

    if ($total -eq 0) {
        $there = @(Get-ChildItem $translationsDirectory -Filter '*.qm' |
                   Select-Object -First 4 -ExpandProperty Name) -join ', '
        Complain ("no catalogue here is under a name the phone loads. It asks for {0}; the tree has {1}" -f `
                  (($asked | ForEach-Object { "${_}_<language>.qm" }) -join ', '), $there)
    } else {
        $counted = ($asked | ForEach-Object { "$_ $($found[$_])" }) -join ', '
        Approve "Qt's own catalogues are here under a name the phone loads ($counted)"
    }
}

Write-Host ''
if ($failures -eq 0) {
    Write-Host 'The tree is one somebody else can run.'
} else {
    Write-Host "$failures problem(s)." -ForegroundColor Red
    exit 1
}
