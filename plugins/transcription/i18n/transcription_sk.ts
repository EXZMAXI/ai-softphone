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
<TS version="2.1" language="sk" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Prepísať</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Použiť tento prepis</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopírovať</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Uložiť ako…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Zmazať tento prepis</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Počúvam ho…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Tento hovor nebol prepísaný. Nastavte v nastaveniach rozpoznávač a potom stlačte Prepísať.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Rozpoznávač v tomto hovore nič nepočul. Zvyčajne je to ticho, hovor príliš krátky na počutie alebo zlý jazyk — skúste iný rozpoznávač zo zoznamu vyššie.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (predvolený)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Ako text</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Ako titulky (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Ako titulky (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Zdroj (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Zmazať tento prepis?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Zmazať</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Hovor a jeho zvuk zostanú nedotknuté. To, čo z neho rozpoznávač urobil, zmizne a s tým aj jeho vlastná odpoveď vedľa súboru.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Uložiť prepis</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textové súbory (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Titulky (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Dátové súbory (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Všetky súbory (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Kopírovať tento riadok</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Kopírovať celý prepis</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Zatiaľ nie je nastavený žiadny rozpoznávač. Pridajte ho v Nastaveniach, v Prepise.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Prepis</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Tento hovor sa práve prepisuje</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Tento hovor sa stále nahráva</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Poslať tento hovor rozpoznávaču</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Zvuk tohto hovoru už tu nie je</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Urobiť tento prepis tým, ktorý používa vyhľadávanie a zhrnutia</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Prepis</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Hovor sa prepisuje, keď o to požiadate v okne nahrávok — alebo sám, ak ste zapli spracovanie v časti Spracovanie. Rozpoznávač na vašom vlastnom stroji nič nestojí; ten v oblaku si účtuje za minútu zvuku.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Rozpoznávače</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Pridať</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Zatiaľ nie je nič nastavené. Server Whisper na vašom vlastnom stroji drží každý hovor v budove; OpenAI vyžaduje kľúč a pripojenie k internetu.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>vypnutý</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Prepisuje nahrané rozhovory</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Počúva rozhovor počas jeho priebehu</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>predvolený</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Stlačením zmeníte</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Prepis</translation>
    </message>
</context>
</TS>
