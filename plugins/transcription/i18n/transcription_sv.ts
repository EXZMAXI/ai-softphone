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
<TS version="2.1" language="sv" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Transkribera</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Använd den här transkriptionen</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopiera</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Spara som…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Ta bort den här transkriptionen</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Lyssnar på det…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Det här samtalet är inte transkriberat. Sätt upp en igenkännare i inställningarna, tryck sedan Transkribera.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Igenkännaren hörde ingenting i det här samtalet. Det är oftast tystnad, ett samtal för kort för att höras, eller fel språk — pröva en annan igenkännare i listan ovan.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (standard)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Som text</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Som undertexter (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Som undertexter (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Källa (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Ta bort den här transkriptionen?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Ta bort</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Samtalet och dess ljud rörs inte. Det igenkännaren gjorde av det försvinner, och dess eget svar bredvid filen med.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Spara transkriptionen</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textfiler (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Undertexter (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Datafiler (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Alla filer (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Kopiera den här raden</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Kopiera hela utskriften</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Ingen igenkännare är uppsatt ännu. Lägg till en i Inställningar, under Transkription.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Transkription</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Det här samtalet transkriberas just nu</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Det här samtalet spelas fortfarande in</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Skicka det här samtalet till igenkännaren</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Ljudet från det här samtalet finns inte längre här</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Gör den här till den transkription som sökningen och sammanfattningarna använder</translation>
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
        <translation>Ett samtal transkriberas när du ber om det i inspelningsfönstret — eller av sig självt, om du slagit på bearbetningen under Bearbetning. En igenkännare på din egen maskin kostar ingenting att köra; en i molnet tar betalt per minut ljud.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Igenkännare</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Lägg till</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Ingenting är uppsatt ännu. En Whisper-server på din egen maskin håller varje samtal inom huset; OpenAI behöver en nyckel och en internetuppkoppling.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>av</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Skriver ut inspelade samtal</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Lyssnar på ett pågående samtal</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>som standard</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Tryck för att ändra</translation>
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
