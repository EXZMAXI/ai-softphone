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
    <name>AnswerPanel</name>
    <message>
        <source>%1…</source>
        <extracomment>A prompt that needs a question typed before it can run. %1 is the prompt&apos;s own name.</extracomment>
        <translation>%1…</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts what a model wrote on the clipboard.</extracomment>
        <translation>Копировать</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Сохранить как…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Удалить</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Удалить это?</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Модель отвечает по этому разговору.</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Работаю…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Сохранить ответ</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстовые файлы (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Все файлы (*)</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Ваш вопрос</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Спросить</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Скопировать этот ответ</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Этот разговор сейчас расшифровывается.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Скопировать обмен с моделью</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Скопировать эту строку</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Обработка</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Модель не нашла в этом разговоре красных флагов.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Модель ничего не написала об этом разговоре.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 будет удалено. Разговор и его стенограмма остаются, и попросить снова можно. Удаление отменить невозможно.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Обработка</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Обработка</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Обрабатывать разговоры автоматически</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Выключено — ничего не происходит, пока вы не попросите в окне записей. Включено — работают правила ниже, и разговор сам становится содержанием, категорией и всем остальным. Облачная модель берёт плату за каждое такое действие.</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Лимит токенов в месяц</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Языковые модели</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Добавить</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Что читает стенограмму и пишет по ней. Модель на вашей машине оставляет каждый разговор внутри и ничего не стоит в работе.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>основной</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Промпты</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>изменён</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Правила</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Лимит денег в месяц</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Два предела — в том, в чём можно посчитать месяц. Когда достигнут любой из них, правила останавливаются до начала следующего месяца. То, что вы просите сами, не останавливается никогда.</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>О чём просят модели. Всё это пришло вместе с программой, и всё это вы вправе изменить — и вернуть обратно.</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Что выполняется само, в этом порядке. Каждое срабатывает не более одного раза на разговор.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Выше</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Ниже</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>По умолчанию</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Возвращает промпты и правила в том виде, в каком они пришли с программой. Языковые модели не трогает.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Восстановить по умолчанию</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>выключена</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>выключен</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Расходы</translation>
    </message>
</context>
</TS>
