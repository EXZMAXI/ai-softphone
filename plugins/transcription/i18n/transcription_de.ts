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
<TS version="2.1" language="de" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Transkribieren</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Diese Transkription verwenden</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopieren</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Speichern unter…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Diese Transkription löschen</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Höre hinein…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Dieses Gespräch wurde nicht transkribiert. Richten Sie in den Einstellungen einen Spracherkenner ein und drücken Sie dann Transkribieren.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Der Spracherkenner hat in diesem Gespräch nichts gehört. Das ist meist Stille, ein zu kurzes Gespräch oder die falsche Sprache — versuchen Sie einen anderen Spracherkenner aus der Liste oben.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (Vorgabe)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Als Text</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Als Untertitel (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Als Untertitel (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Quelle (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Diese Transkription löschen?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Löschen</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Das Gespräch und sein Ton bleiben unberührt. Was der Spracherkenner daraus gemacht hat, geht, und seine eigene Antwort neben der Datei ebenso.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Das Transkript speichern</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textdateien (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Untertitel (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Datendateien (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Alle Dateien (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Diese Zeile kopieren</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Ganzes Transkript kopieren</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Es ist noch kein Spracherkenner eingerichtet. Fügen Sie in den Einstellungen unter Transkription einen hinzu.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Transkription</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Dieses Gespräch wird gerade transkribiert</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Dieses Gespräch wird noch aufgenommen</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Dieses Gespräch an den Spracherkenner senden</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Der Ton dieses Gesprächs liegt nicht mehr hier</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Diese zu der Transkription machen, die die Suche und die Zusammenfassungen verwenden</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Transkription</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Ein Gespräch wird transkribiert, wenn Sie es im Aufnahmenfenster verlangen — oder von selbst, wenn Sie die Verarbeitung unter Verarbeitung eingeschaltet haben. Ein Spracherkenner auf Ihrem eigenen Rechner kostet im Betrieb nichts; einer in der Cloud berechnet je Minute Ton.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Spracherkenner</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Hinzufügen</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Es ist noch nichts eingerichtet. Ein Whisper-Server auf Ihrem eigenen Rechner behält jedes Gespräch im Haus; OpenAI braucht einen Schlüssel und eine Internetverbindung.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>aus</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Erkennt aufgezeichnete Gespräche</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Hört ein laufendes Gespräch mit</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>standardmäßig</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Zum Ändern drücken</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Transkription</translation>
    </message>
</context>
</TS>
