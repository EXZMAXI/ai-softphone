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
<TS version="2.1" language="sr_RS">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Prepiši</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Koristi ovaj prepis</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopiraj</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Sačuvaj kao…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Obriši ovaj prepis</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Slušam ga…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Ovaj razgovor nije prepisan. Podesite prepoznavač u podešavanjima, pa pritisnite Prepiši.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Prepoznavač u ovom razgovoru nije čuo ništa. To je obično tišina, poziv prekratak da bi se čuo, ili pogrešan jezik — isprobajte drugi prepoznavač sa spiska iznad.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (podrazumevani)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Kao tekst</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Kao titlovi (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Kao titlovi (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Izvor (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Obrisati ovaj prepis?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Obriši</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Razgovor i njegov zvuk ostaju netaknuti. Ono što je prepoznavač od njega napravio nestaje, a s tim i njegov sopstveni odgovor pored datoteke.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Sačuvaj prepis</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Tekstualne datoteke (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Titlovi (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Datoteke s podacima (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Sve datoteke (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Kopiraj ovaj red</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Kopiraj ceo transkript</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Nijedan prepoznavač još nije postavljen. Dodajte ga u Podešavanjima, u Prepisivanju.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Prepisivanje</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Ovaj razgovor se upravo prepisuje</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Ovaj razgovor se još snima</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Pošaljite ovaj razgovor prepoznavaču</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Zvuka ovog razgovora više nema ovde</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Neka ovo bude prepis koji koriste pretraga i sažeci</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Prepisivanje</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Razgovor se prepisuje kada to zatražite u prozoru snimaka — ili sam od sebe, ako ste uključili obradu u odeljku Obrada. Prepoznavač na sopstvenoj mašini ništa ne košta; onaj u oblaku naplaćuje po minutu zvuka.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Prepoznavači</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Dodaj</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Još ništa nije podešeno. Whisper server na sopstvenoj mašini drži svaki razgovor u kući; OpenAI traži ključ i vezu s internetom.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>isključen</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Raspisuje snimljene razgovore</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Sluša razgovor dok se vodi</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>podrazumevano</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Pritisnite da promenite</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Prepisivanje</translation>
    </message>
</context>
</TS>
