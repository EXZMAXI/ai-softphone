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
<TS version="2.1" language="bg" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Запиши</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Използвай тази стенограма</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Копиране</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Запази като…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Изтрий тази стенограма</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Слушам я…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Този разговор не е записан. Настройте разпознавач в настройките и после натиснете Запиши.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Разпознавачът не чу нищо в този разговор. Обикновено това е тишина, твърде кратък разговор, за да се чуе, или грешен език — опитайте друг разпознавач от списъка по-горе.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (по подразбиране)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Като текст</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Като субтитри (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Като субтитри (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Източник (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Да се изтрие ли тази стенограма?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Изтрий</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Разговорът и звукът му остават недокоснати. Това, което разпознавачът направи от него, изчезва, а с него и собственият му отговор до файла.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Запази стенограмата</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстови файлове (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Субтитри (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Файлове с данни (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Всички файлове (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Копиране на този ред</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Копиране на цялата стенограма</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Още няма настроен разпознавател. Добавете такъв в Настройки, в Стенограма.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Стенограма</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Този разговор се записва точно сега</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Този разговор още се записва</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Изпрати този разговор на разпознавача</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Звукът на този разговор вече не е тук</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Направи тази стенограмата, която търсенето и обобщенията използват</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Стенограма</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Разговор се записва, когато поискате в прозореца със записите — или сам, ако сте включили обработката в раздел Обработка. Разпознавач на вашата собствена машина не струва нищо да работи; този в облака таксува на минута звук.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Разпознавачи</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Добави</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Още нищо не е настроено. Whisper сървър на вашата собствена машина държи всеки разговор в сградата; OpenAI изисква ключ и интернет връзка.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>изключен</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Разчита записани разговори</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Слуша разговор по време на провеждането му</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>по подразбиране</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Натиснете, за да промените</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Стенограма</translation>
    </message>
</context>
</TS>
