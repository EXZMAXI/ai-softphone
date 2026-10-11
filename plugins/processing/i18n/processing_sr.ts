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
    <name>AnswerPanel</name>
    <message>
        <source>%1…</source>
        <extracomment>A prompt that needs a question typed before it can run. %1 is the prompt&apos;s own name.</extracomment>
        <translation>%1…</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts what a model wrote on the clipboard.</extracomment>
        <translation>Копирај</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Сачувај као…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Обриши</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Радим на томе…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Сачувај одговор</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Текстуалне датотеке (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Све датотеке (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Обрисати ово?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Питај</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Ваше питање</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Модел одговара из овог разговора.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Копирај овај одговор</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Овај разговор се управо преписује.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Копирај размену са моделом</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Копирај овај ред</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Обрада</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Модел није пронашао упозорења у овом разговору.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Модел није ништа написао о овом разговору.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 ће бити обрисано. Разговор и његов препис остају, и можете то поново затражити. Брисање се не може опозвати.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Обрада</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Обрада</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Обрађуј разговоре самостално</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Искључено — ништа се не дешава док не затражите у прозору снимака. Укључено — раде правила испод, а управо то претвара разговор у сажетак, категорију и све остало а да нико ништа не притиска. Модел у облаку наплаћује сваку од тих ствари.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Новчана граница, месечно</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Граница жетона, месечно</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Две границе, у оне две ствари којима се месец може мерити. Чим се достигне било која, правила стају до промене месеца. Кад сами нешто затражите, то се никада не зауставља.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Језички модели</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Додај</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Оно што чита препис и пише о њему. Модел на сопственој машини држи сваки разговор у кући и његов рад ништа не кошта.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>подразумевани</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>искључен</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Упутства</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Оно што се тражи од модела. Свако од њих дошло је с програмом и свако је ваше да га измените — и да га вратите.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>измењено</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>искључен</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Правила</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Оно што ради само, овим редом. Свако се окида највише једном по разговору.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Помери горе</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Помери доле</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Подразумевано</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Враћа упутства и правила онаква каква су дошла с програмом. Ваши језички модели остају нетакнути.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Врати подразумевано</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Трошење</translation>
    </message>
</context>
</TS>
