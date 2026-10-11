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
<TS version="2.1" language="uk" sourcelanguage="en">
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
        <translation>Копіювати</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Зберегти як…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Видалити</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Працюю над цим…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Зберегти відповідь</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстові файли (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Усі файли (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Видалити це?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Запитати</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Ваше питання</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Модель відповідає з цієї розмови.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Скопіювати цю відповідь</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Ця розмова розшифровується просто зараз.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Скопіювати обмін з моделлю</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Скопіювати цей рядок</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Обробка</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Модель не знайшла в цій розмові червоних прапорців.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Модель нічого не написала про цю розмову.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 буде видалено. Розмова і її розшифровка зберігаються, і ви можете попросити це знову. Видалення не скасувати.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Обробка</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Обробка</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Обробляти розмови автоматично</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Вимкнено — нічого не відбувається, доки ви не попросите у вікні записів. Увімкнено — працюють правила нижче, і саме це перетворює розмову на підсумок, категорію і все інше без того, щоб хтось щось натискав. Модель у хмарі бере плату за кожне з цього.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Грошова межа, на місяць</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Межа токенів, на місяць</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Дві межі, у двох речах, якими можна виміряти місяць. Щойно досягнуто будь-якої, правила спиняються до зміни місяця. Попросити щось самому не спиняється ніколи.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Мовні моделі</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Додати</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Те, що читає розшифровку і пише про неї. Модель на вашій власній машині тримає кожну розмову в будівлі, і її робота нічого не коштує.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>типовий</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>вимкнено</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Вказівки</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Те, про що просять моделі. Кожна з них прийшла з програмою, і кожну ви можете змінити — і повернути.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>змінено</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>вимкнено</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Правила</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Те, що працює саме, у цьому порядку. Кожне спрацьовує щонайбільше раз на розмову.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Пересунути вгору</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Пересунути вниз</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Типові</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Повертає вказівки і правила такими, якими вони прийшли з програмою. Ваші мовні моделі лишаються недоторканими.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Відновити типові</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Витрати</translation>
    </message>
</context>
</TS>
