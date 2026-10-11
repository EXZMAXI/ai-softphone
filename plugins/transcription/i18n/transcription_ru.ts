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
<TS version="2.1" language="ru">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Расшифровать</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Использовать эту расшифровку</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Копировать</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Сохранить как…</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Слушает…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Этот разговор не расшифрован. Настройте распознаватель в настройках и нажмите «Расшифровать».</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Распознаватель ничего не услышал в этом разговоре. Обычно это тишина, слишком короткий звонок или другой язык — попробуйте другой распознаватель из списка выше.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (основной)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Текстом</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Субтитрами (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Субтитрами (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Исходник (JSON)</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Удалить</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Разговор и его звук остаются на месте. Уходит то, что сделал из него распознаватель, и его собственный ответ рядом с файлом.</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Удалить эту расшифровку</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Удалить эту расшифровку?</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Сохранить расшифровку</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстовые файлы (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Субтитры (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Файлы данных (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Все файлы (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Скопировать эту строку</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Скопировать всю стенограмму</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Распознаватель ещё не настроен. Добавьте его в настройках, в разделе «Расшифровка».</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Расшифровка</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Этот разговор сейчас расшифровывается</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Этот разговор ещё записывается</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Отправить этот разговор распознавателю</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Звука этого разговора здесь больше нет</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Сделать эту расшифровку той, по которой ищут и по которой делают выводы</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Расшифровка</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Разговор расшифровывается, когда вы просите об этом в окне записей — или сам, если вы включили обработку в разделе «Обработка». Распознаватель на своей машине не стоит ничего; облачный берёт плату за минуту звука.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Распознаватели</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Добавить</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Пока ничего не настроено. Сервер Whisper на вашей машине оставляет все разговоры внутри; для OpenAI нужны ключ и интернет.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>выключен</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Расшифровывает записанные разговоры</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Слушает разговор по ходу</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>по умолчанию</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Нажмите, чтобы изменить</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Расшифровка</translation>
    </message>
</context>
</TS>
