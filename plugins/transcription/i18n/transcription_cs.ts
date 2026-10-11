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
<TS version="2.1" language="cs" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Přepsat</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Použít tento přepis</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopírovat</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Uložit jako…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Smazat tento přepis</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Poslouchám ho…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Tento hovor nebyl přepsán. Nastavte v nastavení rozpoznávač a pak stiskněte Přepsat.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Rozpoznávač v tomto hovoru nic neslyšel. Obvykle je to ticho, hovor příliš krátký na slyšení nebo špatný jazyk — zkuste jiný rozpoznávač ze seznamu výše.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (výchozí)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Jako text</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Jako titulky (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Jako titulky (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Zdroj (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Smazat tento přepis?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Smazat</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Hovor a jeho zvuk zůstanou nedotčené. To, co z něj rozpoznávač udělal, zmizí a s tím i jeho vlastní odpověď vedle souboru.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Uložit přepis</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textové soubory (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Titulky (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Datové soubory (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Všechny soubory (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Kopírovat tento řádek</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Kopírovat celý přepis</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Zatím není nastaven žádný rozpoznávač. Přidejte ho v Nastavení, v Přepisu.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Přepis</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Tento hovor se právě přepisuje</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Tento hovor se stále nahrává</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Poslat tento hovor rozpoznávači</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Zvuk tohoto hovoru už tu není</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Učinit tento přepis tím, který používá vyhledávání a shrnutí</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Přepis</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Hovor se přepisuje, když o to požádáte v okně nahrávek — nebo sám, pokud jste zapnuli zpracování v části Zpracování. Rozpoznávač na vašem vlastním stroji nic nestojí; ten v oblaku si účtuje za minutu zvuku.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Rozpoznávače</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Přidat</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Zatím není nic nastaveno. Server Whisper na vašem vlastním stroji drží každý hovor v budově; OpenAI vyžaduje klíč a připojení k internetu.</translation>
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
        <translation>Přepisuje nahrané rozhovory</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Poslouchá rozhovor v jeho průběhu</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>výchozí</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Stisknutím změníte</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Přepis</translation>
    </message>
</context>
</TS>
