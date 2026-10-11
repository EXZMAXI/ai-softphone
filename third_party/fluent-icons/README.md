# Fluent UI System Icons

The pictures on the buttons and in the rows of every window — and the application's own
icon, which is two of these drawings placed on a coloured square rather than anything
drawn here.

|           |                                                                                                                      |
| --------- | -------------------------------------------------------------------------------------------------------------------- |
| Upstream  | https://github.com/microsoft/fluentui-system-icons                                                                   |
| Licence   | **MIT** — the full text is in `LICENSE` beside this file                                                             |
| Copyright | © 2020 Microsoft Corporation                                                                                         |
| Taken at  | `84e8a2ae0e55b3cbe176b5cc33154fe82ef363cc`                                                                           |
| What      | 21 drawings at size 24 in both weights, and 2 at size 48 for the application icon                                    |
| Where     | `ui/icons/glyphs/`, compiled into the binary by `ui/CMakeLists.txt`; `ui/icons/parts/`, which the icon is drawn from |

## Why this set and not a prettier one

The product was **GPL version 2** when this was chosen, and that decided more
than taste did. Apache-2.0 is not compatible with GPLv2 — the Apache Software
Foundation says so itself, citing the patent-termination and indemnification
clauses — which ruled out **Material Symbols** and **Remix Icon**, the two sets
a person reaches for first. What was left is the MIT and ISC families: Fluent,
Phosphor, Lucide, Tabler, Bootstrap Icons.

**The constraint has since gone and the choice stands.** M18 amended the licence
to GPL-2.0-**or-later**, because OpenSSL is Apache-2.0 and had to be linked; an
Apache-2.0 icon set would now be permissible too. Nothing is going to be redrawn
over it — the reason below is the one that actually decided this, and it has not
changed.

Fluent is the one whose vocabulary is telephony. `Call Transfer` and
`Call Park` exist in it as themselves, drawn by people who were drawing a
telephone, rather than as an arrow the user has to interpret. That mattered
more than it sounds: until this landed, the transfer button wore a
**mail-forward arrow**, the conference button a **group of people** and the
split button a **pair of scissors**, because those are the nearest things a
*desktop* icon theme has to a telephone.

## Why the files are here rather than fetched by the build

Forty-four files of about two kilobytes each. A build that reaches the network to draw a button
is a build that fails on an aeroplane, and the pinned commit above is only
honest if what it pins is sitting in the tree where anyone can read it.

They were fetched from the commit above and committed exactly as they arrived,
so the committed file is itself the record: any drift from upstream is a
difference anybody can measure against that commit.

## What MIT asks of us

The licence text and the copyright notice must travel with anything we ship.
`LICENSE` beside this file is that text.

The About section in the settings window closed the gap this file used to end
by naming: every component is listed there beside the licence it is used under,
this set included. It arrived in the same increment as the drawings.
