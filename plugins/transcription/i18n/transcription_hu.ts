<?xml version="1.0" encoding="utf-8"?>
<!--
  AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
  https://ai-softphone.com

  SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
  SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
  SPDX-License-Identifier: GPL-2.0-or-later

  This file is part of AI Softphone. There is NO WARRANTY, to the extent
  permitted by law. See the file LICENSE for the full terms.
-->
<!DOCTYPE TS>
<TS version="2.1" language="hu" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Leírás</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Ennek az átiratnak a használata</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Másolás</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Mentés másként…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Ennek az átiratnak a törlése</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Hallgatom…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Ez a beszélgetés nincs leírva. Állítson be egy felismerőt a beállításokban, aztán nyomja meg a Leírást.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>A felismerő semmit sem hallott ebben a beszélgetésben. Ez általában csend, hallhatáshoz túl rövid hívás vagy rossz nyelv — próbáljon másik felismerőt a fenti listából.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (alapértelmezett)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Szövegként</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Feliratként (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Feliratként (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Forrás (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Törli ezt az átiratot?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Törlés</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>A beszélgetés és a hangja érintetlen marad. Az, amit a felismerő csinált belőle, eltűnik, és vele a fájl melletti saját válasza is.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Az átirat mentése</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Szövegfájlok (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Feliratok (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Adatfájlok (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Minden fájl (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>A sor másolása</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>A teljes átirat másolása</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Még nincs beállítva felismerő. Vegyen fel egyet a Beállításokban, az Átirat alatt.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Átirat</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Ezt a beszélgetést épp most írjuk le</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Ezt a beszélgetést még mindig rögzítjük</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Küldje el ezt a beszélgetést a felismerőnek</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Ennek a beszélgetésnek a hangja már nincs itt</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Legyen ez az az átirat, amelyet a keresés és az összefoglalók használnak</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Átirat</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Egy beszélgetés akkor íródik le, amikor kéri a felvételek ablakában — vagy magától, ha bekapcsolta a feldolgozást a Feldolgozás résznél. A saját gépén futó felismerő semmibe sem kerül; a felhőben lévő a hang perceiért kér fizetést.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Felismerők</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Hozzáadás</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Még semmi sincs beállítva. A saját gépén futó Whisper-kiszolgáló minden beszélgetést az épületen belül tart; az OpenAI kulcsot és internetkapcsolatot igényel.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>ki</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Leírja a rögzített beszélgetéseket</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Beszélgetés közben hallgat</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>alapértelmezés szerint</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Nyomja meg a módosításhoz</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Átirat</translation>
    </message>
</context>
</TS>
