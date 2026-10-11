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
<TS version="2.1" language="pl" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Przepisz</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Użyj tej transkrypcji</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopiuj</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Zapisz jako…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Usuń tę transkrypcję</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Odsłuchuję…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Ta rozmowa nie została przepisana. Skonfiguruj rozpoznawacz w ustawieniach, a potem naciśnij Przepisz.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Rozpoznawacz nic nie usłyszał w tej rozmowie. Zwykle to cisza, zbyt krótka rozmowa albo zły język — spróbuj innego rozpoznawacza z listy powyżej.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (domyślny)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Jako tekst</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Jako napisy (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Jako napisy (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Źródło (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Usunąć tę transkrypcję?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Usuń</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Rozmowa i jej dźwięk zostają nietknięte. To, co zrobił z niej rozpoznawacz, znika, a wraz z tym jego własna odpowiedź obok pliku.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Zapisz transkrypcję</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Pliki tekstowe (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Napisy (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Pliki danych (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Wszystkie pliki (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Kopiuj ten wiersz</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Kopiuj całą transkrypcję</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Nie ustawiono jeszcze żadnego rozpoznawacza. Dodaj go w Ustawieniach, w Transkrypcji.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Transkrypcja</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Ta rozmowa jest właśnie przepisywana</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Ta rozmowa jest wciąż nagrywana</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Wyślij tę rozmowę do rozpoznawacza</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Dźwięku tej rozmowy już tu nie ma</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Uczyń tę transkrypcję tą, której używa wyszukiwanie i podsumowania</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Transkrypcja</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Rozmowa jest przepisywana, gdy poprosisz o to w oknie nagrań — albo sama, jeśli włączyłeś przetwarzanie w sekcji Przetwarzanie. Rozpoznawacz na Twojej własnej maszynie nic nie kosztuje w działaniu; ten w chmurze liczy za minutę dźwięku.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Rozpoznawacze</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Dodaj</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Nic jeszcze nie skonfigurowano. Serwer Whisper na Twojej własnej maszynie trzyma każdą rozmowę w budynku; OpenAI wymaga klucza i połączenia z internetem.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>wyłączony</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Rozpisuje nagrane rozmowy</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Słucha rozmowy w jej trakcie</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>domyślnie</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Naciśnij, aby zmienić</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Transkrypcja</translation>
    </message>
</context>
</TS>
