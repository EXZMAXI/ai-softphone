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
<TS version="2.1" language="cs" sourcelanguage="en">
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
        <translation>Kopírovat</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Uložit jako…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Smazat</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Pracuji na tom…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Uložit odpověď</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textové soubory (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Všechny soubory (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Smazat toto?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Zeptat se</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Vaše otázka</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Model odpovídá z tohoto hovoru.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Kopírovat tuto odpověď</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Tento hovor se právě přepisuje.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Kopírovat komunikaci s modelem</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Kopírovat tento řádek</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Zpracování</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Model v tomto hovoru nenašel žádné signály.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Model o tomto hovoru nic nenapsal.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 se smaže. Hovor a jeho přepis zůstanou a můžete o to požádat znovu. Smazání nelze vrátit.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Zpracování</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Zpracování</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Zpracovávat hovory automaticky</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Vypnuto se nic neděje, dokud o to nepožádáte v okně nahrávek. Zapnuto běží pravidla níže — a právě to promění hovor ve shrnutí, kategorii a všechno ostatní, aniž by někdo na cokoli klikal. Model v oblaku si za každou z těch věcí účtuje.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Mez peněz, měsíčně</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Mez tokenů, měsíčně</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Dvě meze, ve dvou věcech, ve kterých se měsíc dá počítat. Jakmile je kterákoli dosažena, pravidla se zastaví do přelomu měsíce. Požádat o něco sami se nikdy nezastaví.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Jazykové modely</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Přidat</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>To, co čte přepis a píše o něm. Model na vašem vlastním stroji drží každý hovor v budově a jeho běh nic nestojí.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>výchozí</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>vypnutý</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Pokyny</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>To, oč jsou modely žádány. Každý z nich přišel s programem a každý je váš ke změně — i k vrácení.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>změněno</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>vypnutý</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Pravidla</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>To, co běží samo, v tomto pořadí. Každé se spustí nejvýše jednou na hovor.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Posunout nahoru</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Posunout dolů</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Výchozí</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Vrátí pokyny a pravidla tak, jak přišly s programem. Vaše jazykové modely zůstanou nedotčené.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Obnovit výchozí</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Výdaje</translation>
    </message>
</context>
</TS>
