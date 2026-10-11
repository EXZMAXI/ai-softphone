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
    <name>AnswerPanel</name>
    <message>
        <source>%1…</source>
        <extracomment>A prompt that needs a question typed before it can run. %1 is the prompt&apos;s own name.</extracomment>
        <translation>%1…</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts what a model wrote on the clipboard.</extracomment>
        <translation>Копиране</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Запази като…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Изтрий</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Работя по това…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Запази отговора</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстови файлове (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Всички файлове (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Да се изтрие ли това?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Попитай</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Вашият въпрос</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Моделът отговаря въз основа на този разговор.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Копиране на този отговор</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Този разговор се записва точно сега.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Копиране на обмена с модела</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Копиране на този ред</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Обработка</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Моделът не откри сигнали в този разговор.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Моделът не написа нищо за този разговор.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 ще бъде изтрито. Разговорът и стенограмата му остават и можете да поискате отново. Изтриването не може да се отмени.</translation>
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
        <translation>Обработвай разговорите автоматично</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Изключено — не се случва нищо, докато не поискате в прозореца със записите. Включено — работят правилата по-долу, и точно това превръща разговора в обобщение, категория и всичко останало, без някой да натиска нищо. Модел в облака таксува всяко от тези неща.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Парична граница, месечно</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Граница за токени, месечно</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Две граници, в двете неща, в които може да се брои един месец. Щом която и да е бъде достигната, правилата спират до смяната на месеца. Да поискате нещо сами никога не се спира.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Езикови модели</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Добави</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Това, което чете стенограма и пише за нея. Модел на вашата собствена машина държи всеки разговор в сградата и работата му не струва нищо.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>по подразбиране</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>изключен</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Указания</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Това, което се иска от моделите. Всяко от тях дойде с програмата и всяко е ваше, за да го промените — и да го върнете.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>променено</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>изключено</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Правила</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Това, което работи само, в този ред. Всяко се задейства най-много веднъж на разговор.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Премести нагоре</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Премести надолу</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>По подразбиране</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Връща указанията и правилата такива, каквито дойдоха с програмата. Вашите езикови модели остават недокоснати.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Възстанови по подразбиране</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Разходи</translation>
    </message>
</context>
</TS>
