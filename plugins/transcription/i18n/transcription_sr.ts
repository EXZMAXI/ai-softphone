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
        <translation>Препиши</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Користи овај препис</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Копирај</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Сачувај као…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Обриши овај препис</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Слушам га…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Овај разговор није преписан. Подесите препознавач у подешавањима, па притисните Препиши.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Препознавач у овом разговору није чуо ништа. То је обично тишина, позив прекратак да би се чуо, или погрешан језик — испробајте други препознавач са списка изнад.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (подразумевани)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Као текст</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Као титлови (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Као титлови (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Извор (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Обрисати овај препис?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Обриши</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Разговор и његов звук остају нетакнути. Оно што је препознавач од њега направио нестаје, а с тим и његов сопствени одговор поред датотеке.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Сачувај препис</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстуалне датотеке (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Титлови (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Датотеке с подацима (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Све датотеке (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Копирај овај ред</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Копирај цео транскрипт</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Ниједан препознавач још није постављен. Додајте га у Подешавањима, у Преписивању.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Преписивање</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Овај разговор се управо преписује</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Овај разговор се још снима</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Пошаљите овај разговор препознавачу</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Звука овог разговора више нема овде</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Нека ово буде препис који користе претрага и сажеци</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Преписивање</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Разговор се преписује када то затражите у прозору снимака — или сам од себе, ако сте укључили обраду у одељку Обрада. Препознавач на сопственој машини ништа не кошта; онај у облаку наплаћује по минуту звука.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Препознавачи</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Додај</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Још ништа није подешено. Whisper сервер на сопственој машини држи сваки разговор у кући; OpenAI тражи кључ и везу с интернетом.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>искључен</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Расписује снимљене разговоре</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Слуша разговор док се води</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>подразумевано</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Притисните да промените</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Преписивање</translation>
    </message>
</context>
</TS>
