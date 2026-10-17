# AI Softphone

**[ai-softphone.com](https://ai-softphone.com)**

A cross-platform desktop SIP softphone with an AI enrichment layer. It places and takes calls and
records them, turns the recordings into text and writes up what was said; and while a call is still
going on it recognises speech as it is spoken, subtitles or translates it, and prompts the person
taking the call. Windows, Linux and macOS, from one tree.

## What it looks like

Linux, the dark theme, three of the set — the phone photographing itself off a world seeded for
the purpose, rather than anything drawn for this file.

**The prompter, while the call is still going on.** The other party's words arrive on the left as
they are said; a model reads them and writes what is happening and what to say next on the right,
and the newest of those is repeated across the top where it can be read without looking away.

![The prompter during a sales call](images/prompter-sales.png)

**A conversation after it has ended.** The library at the top, both directions of the audio drawn
as their own channel, and under them the transcript with the speakers kept apart, following the
playhead as it moves.

![The recordings window with a transcript](images/transcript.png)

**Three calls at once.** One talking, one on hold and one ringing in: the one ringing is offered on
the card, the list under it marks the conversation the microphone is on, and the lines the phone is
registered on sit along the foot.

![The phone with three calls](images/calls.png)

## What it does

- **Any conversation, however it arrived.** Calls this phone places and takes, meetings held in
  Zoom, Teams or anything else the machine plays out loud, and sound files that already existed.
  One library afterwards, and the same machinery over all three.
- **Everything expected of a business telephone.** Several accounts and several calls at once,
  hold, mute and DTMF, blind and attended transfer, a local conference, auto-answer, global
  hotkeys and a tray icon; TLS for signalling and SRTP for media.
- **Every recording becomes text**, each side on its own channel, with the line lit as the sound
  reaches it and a search that folds case in every alphabet the interface is written in.
- **And then the write-up**: summaries, what each side promised, topics and entities, tags, red
  flags, scoring against a rubric and translation — from prompts that ship with the product and
  that anybody can edit.
- **While the call is still going on**: subtitles, translation, and a prompter that reads what the
  other party is saying and offers what to say next.
- **Eleven speech recognisers**, each of which both listens live and transcribes a file; and
  language models through the OpenAI chat shape — one on your own machine behind Ollama or vLLM,
  OpenAI itself, most of the vendors in between — or Anthropic's own.
- **It can all stay on your machine.** A recogniser and a model on your own hardware, the database
  and the recordings local, nothing in the middle.
- **Thirty languages**, each of them a complete interface rather than a set of labels.
- **It fits what you already run.** `tel:` and `sip:` links from a browser or a CRM, webhooks, a
  local HTTP API, and a diagnostics window showing the SIP on the wire.
- **Only the parts you want.** Every module has a tick of its own, the telephone included.
- **GPL-2.0-or-later** — free software, and no licence per seat.

**The "or later" above is an offer**, made here and in the application's own About page: `LICENSE`
holds the text of version 2, and a recipient may take this work under that version or under any
later one at their option.

## About this tree

The product's source, laid out for building: the C++, the QML, the CMake that assembles them, the
generated sounds and icons, the thirty translation catalogues and everything each package format
needs. One tree builds all three operating systems; which parts of the product a build carries is
decided by the `SOFTPHONE_BUILD_*` options in `CMakeLists.txt`, and every one of them defaults to on.

## Building on Linux

Reference platform: Ubuntu 24.04 LTS or a derivative on the same base, GCC 13, Qt 6.8 LTS.
Published packages are built on Ubuntu 22.04 so that they run on anything newer — a binary's floor
is the glibc it was compiled against.

### 1. System packages

```bash
./scripts/install-build-deps.sh
```

Two optional extras: `pipx`, which installs Qt in the next step, and `libsecret-tools`, whose
`secret-tool` command shows what the keyring is holding.

```bash
sudo apt install -y pipx libsecret-tools
```

### 2. Qt 6.8 LTS

The Qt in the Ubuntu 24.04 archive is 6.4, which predates the `colorScheme` API the interface uses
to follow the system's light and dark theme. Install 6.8 alongside it, touching no system package:

```bash
pipx install aqtinstall && aqt install-qt linux desktop 6.8.3 linux_gcc_64 --outputdir "$HOME/Qt"
export QT_ROOT_DIR="$HOME/Qt/6.8.3/gcc_64"
```

Put that `export` in your shell profile; the build reads `QT_ROOT_DIR` through the `base` preset.

Qt 6.8.3 rather than whatever is newest: every measurement this product's layout rests on — Material
control insets, which signal a view fires when it scrolls, what `grabToImage` refuses — was taken
against that version.

### 3. pjproject

Built from a pinned, checksum-verified release into `~/.cache/softphone-ai-deps`, because what the
distributions ship varies in build options between releases and some builds omit pjsua2 altogether.
About a minute; afterwards the script is a no-op.

```bash
./scripts/build-pjproject.sh
```

### 4. Configure and build

```bash
cmake --preset dev && cmake --build --preset dev
```

Three presets: `dev` for everyday work, `package` for what a release is cut from, and `base`, which
the other two inherit. Add `-D SOFTPHONE_WARNINGS_AS_ERRORS=ON` to turn the compiler's warnings
into errors, which is the standard this code is kept at.

### 5. Installing

```bash
cmake --install build/dev --prefix ~/.local
```

Puts the two binaries, a desktop entry and an icon where a desktop menu will find them. Optional:
everything runs from the build directory.

Three details of it are each worth knowing, because each was a defect first. The install carries an
**RPATH** to the Qt it was built against, since Qt 6.8 sits beside the distribution's 6.4 and the
loader would otherwise find the older one and refuse to start. The desktop entry's `Exec` is written
**at install time with the real prefix**, because `--prefix` overrides whatever the configure step
knew, and a bare binary name would depend on the PATH of the desktop *session* rather than of the
terminal. The icon is installed as **PNG as well as SVG**, because Ubuntu 24.04 ships librsvg
without its gdk-pixbuf loader, and a desktop asked to draw an SVG icon through gdk-pixbuf draws
nothing at all.

## Building on Windows

### 1. Toolchain

**Visual Studio 2022 Build Tools** with the "Desktop development with C++" workload, which brings
MSVC v143 and a current Windows SDK. Visual Studio 2019 is not enough: `/std:c++20` arrived in 16.11
and Qt 6.8 is built with the 2022 toolset.

CMake and Ninja must be on the `PATH`, and the ones bundled with Visual Studio are too old — install
them separately. **Python 3** is needed by the asset scripts.

### 2. Qt 6.8 LTS

```powershell
pip install aqtinstall
aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 --outputdir D:\Qt6
setx QT_ROOT_DIR D:\Qt6\6.8.3\msvc2022_64
```

### 3. pjproject

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-pjproject.ps1
```

Same source archive, same checksum and same `config_site.h` as the Linux build — the compile-time
limits are one file read by both scripts. It installs into `%LOCALAPPDATA%\softphone-ai-deps` and is
a no-op afterwards; `-Force` rebuilds.

It builds two configurations, and the reason saves an afternoon: pjproject's Visual Studio
configurations are named for the C runtime they link rather than for the kind of library they
produce, and all of them produce static `.lib` files. CMake's default for MSVC is `/MD` and `/MDd`,
so the `Release-Dynamic` and `Debug-Dynamic` pair is what can be linked here at all. The Static pair
builds perfectly and then fails at link time on `_ITERATOR_DEBUG_LEVEL`.

### 4. OpenSSL

```powershell
powershell -ExecutionPolicy Bypass -File scripts\build-openssl.ps1
```

Linux needs no such step — the distribution ships one and `find_package` finds it. Windows ships
none, and this product wants one for `https` and for the Ed25519 check over a release manifest.
Without it a development build comes up with no update service at all, and `cmake --preset package`
stops rather than producing a package in that state.

Built from source, pinned by checksum, into `%LOCALAPPDATA%\softphone-ai-deps`. Perl is OpenSSL's
build system and there is no way around it; the script uses whatever Perl it finds and fetches a
portable one when it finds none.

### 5. Configure and build

From a **Developer Command Prompt**, or any shell that has run `vcvars64.bat`, because the presets
use Ninja and Ninja needs `cl` on the `PATH`:

```cmd
call scripts\windows-build-environment.cmd
cmake --preset dev
cmake --build --preset dev
```

## Building on macOS

Reference platform: macOS 14.5 on Apple silicon, Apple clang 16, Qt 6.8 LTS. The product needs
**14.4 or newer** on this system, because the audio taps it records other applications with were
introduced there.

### 1. The toolchain

```bash
xcode-select --install
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
brew install ninja pkgconf bash
```

`pkgconf` rather than `pkg-config`, whose formula is deprecated and which it provides anyway. And
`bash`, because the `/bin/bash` Apple ships is **3.2.57** — a 2007 release kept at that version for
licensing reasons — and some of these scripts use builtins that arrived in bash 4.

### 2. CMake 3.31.5, which Homebrew does not have

Homebrew's `cmake` is 4.x, and CMake 4 removed compatibility with any `cmake_minimum_required` below
3.5 — which a pinned dependency of this build declares, so `FetchContent` fails before a line of the
project is compiled. There is no `cmake@3` formula, so take the official universal build:

```bash
curl -sSLO https://github.com/Kitware/CMake/releases/download/v3.31.5/cmake-3.31.5-macos-universal.tar.gz
tar -xzf cmake-3.31.5-macos-universal.tar.gz -C "$HOME"
```

### 3. Qt 6.8 LTS

```bash
python3 -m pip install --user aqtinstall
aqt install-qt mac desktop 6.8.3 clang_64 -m qtshadertools --outputdir "$HOME/Qt"
export QT_ROOT_DIR="$HOME/Qt/6.8.3/clang_64"
```

Qt's own macOS build is already universal. **Not Homebrew's Qt**, which is 6.9 — see the note about
6.8.3 in the Linux section.

### 4. The rest of the dependencies

```bash
./scripts/build-macos-deps.sh
```

### 5. Configure and build

```bash
cmake --preset dev && cmake --build --preset dev
```

## Running

The desktop application:

```bash
./build/dev/ui/ai-softphone
```

The headless client, which is standing proof that the engine runs with no interface at all:

```bash
./build/dev/cli/ai-softphone-cli --help
```

Both read and write one configuration directory, so they can be used against the same accounts.

## Packaging

```bash
./scripts/make-package.sh          # Linux: a .deb and an AppImage into dist/linux
./scripts/build-in-container.sh    # the same, on the old base a published package is built on
scripts\make-package.ps1           # Windows: an Inno Setup installer
./scripts/make-macos-dmg.sh        # macOS: a disk image
```

Each checks the staged tree before it seals it: every QML module the product imports has to be in
there, and every name the code loads a translation by has to reach a file. A package is the one
place where a reading taken in a build directory is not a fact about what ships.

The signing scripts beside them want credentials this tree does not carry — a code-signing
certificate, an Apple Developer identity, a release key. Unsigned packages build and install; what
signing buys is that the operating system stops warning about an unknown publisher, and that an
update manifest can be proved to have come from whoever published it.

## Translations

Thirty languages, in `ui/i18n/` and `plugins/*/i18n/`. They are compiled into the binaries as
resources rather than installed as files, so a build has them all and `SOFTPHONE_LANG=<code>`
chooses one for a single run. The language a person picks in the settings is a separate and shorter
list: a catalogue that exists is not the same thing as a language the product offers, and
`cmake/Languages.cmake` is where the second list lives.

## Layout

| Path           | Contents                                                                      |
| -------------- | ----------------------------------------------------------------------------- |
| `core/`        | The engine. C++20, no Qt and no SIP stack — what a headless server would link |
| `sip/`         | The SIP backend, on pjsua2. Separate so the engine never needs a stack        |
| `media/`       | Decoding, encoding and the waveform                                           |
| `capture/`     | Recording other applications' calls, one backend per operating system         |
| `storage/`     | The local database                                                            |
| `secrets/`     | The platform keychain, and a sealed file for desktops that have none          |
| `http/`        | The client and the local control API                                          |
| `container/`   | The plugin host: what loads the modules below and what they may ask of it     |
| `plugins/`     | The modules the product is assembled from, each switchable                    |
| `shell/`       | What holds the engine and the plugins together outside the interface          |
| `ui/`          | Qt 6 Quick. Presentation only, no telephony logic                             |
| `cli/`         | The headless client                                                           |
| `cmake/`       | The build's own modules: languages, telemetry, update hosts, platform checks  |
| `packaging/`   | What a package format needs and the build does not                            |
| `scripts/`     | Dependency builds, packaging and signing                                      |
| `third_party/` | Vendored work, with its licence beside it                                     |

## License

**GPL-2.0-or-later.** `LICENSE` holds the text of version 2, and the "or later" is an offer this
project makes on top of it — made here, in the application's own About page and in the AppStream
metadata. A recipient may take this work under version 2 or under any later version at their
option.

**Why "or later" and not version 2 alone.** Version 2 was the original choice, and it is the more
constrained of the two. What broke it was OpenSSL, which this build links for `https` and for
verifying a release manifest: OpenSSL 3 is **Apache-2.0**, and the Free Software Foundation's
position — the one that matters, since it is their licence — is that Apache-2.0 is *not* compatible
with GPL v2, because its patent-termination clause is a restriction v2 does not permit to be added.
It is compatible with GPL v3. On Linux that could be argued around, since GPL v2 §3 excuses
components that normally accompany the operating system, and a `.deb` depends on the distribution's
`libssl` rather than carrying it. It could not be argued around on Windows, where the installer
carries `libcrypto-3-x64.dll` inside it. Adding "or later" costs nothing and settles it: every
component below permits it, and a recipient who needs v3 has it.

**What this product ships that somebody else wrote.** The table is written from
`third_party/components.json`, which is also what the About page and the notices file are held to;
the versions are not in it on purpose, because the build scripts are where they are pinned.

| Component                                                                                  | Offered under                                    | Verdict                                                                      |
| ------------------------------------------------------------------------------------------ | ------------------------------------------------ | ---------------------------------------------------------------------------- |
| Qt 6 — The interface, and the libraries under it                                           | Commercial **or LGPL-3.0 or GPL-2.0 or GPL-3.0** | both GPL versions are on offer                                               |
| pjproject (PJSIP) — The SIP stack and its media engine                                     | GPL-2.0-**or-later**                             | compatible with either                                                       |
| speex and speexdsp, inside pjproject — The echo canceller and the resampler pjproject uses | BSD-3-Clause                                     | compatible                                                                   |
| RNNoise, inside pjproject's WebRTC echo canceller — Noise suppression                      | BSD-3-Clause                                     | compatible                                                                   |
| resample, inside pjproject — Sample-rate conversion                                        | LGPL-**2.1**                                     | compatible                                                                   |
| libsrtp, inside pjproject — SRTP, which is how media is encrypted                          | BSD-3-Clause                                     | compatible                                                                   |
| WebRTC, inside pjproject — The echo canceller                                              | BSD-3-Clause                                     | compatible                                                                   |
| GSM 06.10, inside pjproject — The GSM codec                                                | permissive                                       | compatible                                                                   |
| iLBC, inside pjproject — The iLBC codec, from RFC 3951                                     | IETF RFC 3951 terms                              | compatible                                                                   |
| Opus — The Opus codec, which pjproject links                                               | BSD-3-Clause                                     | compatible                                                                   |
| OpenSSL — TLS for signalling, for https and for verifying a release                        | **Apache-2.0**                                   | compatible with GPL-3.0 only - which is why the offer is "or later"          |
| SQLite — The local database                                                                | public domain                                    | compatible                                                                   |
| nlohmann/json — Reading and writing JSON                                                   | MIT                                              | compatible                                                                   |
| cpp-httplib — The HTTP client and the local control API                                    | MIT                                              | compatible                                                                   |
| IXWebSocket — The socket ten streaming recognisers are carried on                          | BSD-3                                            | compatible                                                                   |
| nghttp2 — HTTP/2, for the one streaming recogniser that is gRPC                            | MIT                                              | compatible; the library only, `ENABLE_LIB_ONLY`                              |
| LAME — The MP3 encoder every recording is written with                                     | LGPL-**2.0-or-later**                            | compatible; LGPL-2.0 §3 permits taking it under the GPL                      |
| minimp3 — The MP3 decoder the player and the waveform read with                            | CC0                                              | compatible                                                                   |
| Fluent UI System Icons — The pictures on the buttons and in the rows                       | MIT                                              | compatible; the drawings themselves carry their notice                       |
| libsecret — The desktop keyring, where one answers                                         | LGPL-2.1+                                        | compatible                                                                   |
| ALSA — The sound devices the telephone opens                                               | LGPL-2.1+                                        | compatible                                                                   |
| PulseAudio (client) — The sound server, where the machine runs one                         | LGPL-2.1+                                        | compatible - the GPL parts of PulseAudio are the server, which is not linked |

**The constraint that survives.** Nothing **LGPL-v3-only** may ever be linked — LGPL v3 is
compatible with GPL v3 but not with GPL v2, so linking it would take the "or version 2" half of the
offer away. GPL-v3-only is permissible, which is a real change: Qt Wayland **Compositor** stops
being forbidden. It is still not used, since this is a Wayland client.

**The copyright and the licence of each of those, in full, is in `THIRD-PARTY-LICENCES`**, with the
texts themselves in `third_party/licences/` — carried verbatim from each component's own tree, or
from the Free Software Foundation's and the Apache Software Foundation's own wording for the long
canonical ones. A built package carries the same file beside this product's own licence, because
most of those licences ask for it by name rather than for a link.

**Two things that file does not try to restate**, and they are written into it as well. A Linux
package also carries about thirty shared libraries copied off the machine it was built on — FLAC,
opus, sndfile, Kerberos, ICU, ogg, vorbis and the xcb family among them — whose copyright is stated
by the distribution that built them, and those statements travel verbatim beside it rather than
being summarised. And Qt carries third-party code of its own, under some twenty-five further
licences, which Qt enumerates in the SPDX SBOM shipped inside the Qt installation a build is made
against.

**The Qt figures come from that SBOM** rather than from Qt's website, which is the difference
between knowing what *this* build uses and what Qt offers in general. It is two commands, and they
are here so that a reader can repeat them rather than take the table on trust:

```bash
ldd build/dev/ui/ai-softphone | grep -o 'libQt6[A-Za-z]*'   # what is actually linked
grep -A6 '^FileName: ./lib/libQt6Core.so' "$QT_ROOT_DIR"/sbom/*.spdx
```
