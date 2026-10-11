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
<TS version="2.1" language="et" sourcelanguage="en">
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
        <translation>Kopeeri</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Salvesta kui…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Kustuta</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Töötan selle kallal…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Salvesta vastus</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Tekstifailid (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Kõik failid (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Kas kustutada see?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Küsi</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Teie küsimus</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Mudel vastab sellest vestlusest.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Kopeeri see vastus</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Seda vestlust kirjutatakse praegu üles.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Kopeeri suhtlus mudeliga</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Kopeeri see rida</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Töötlemine</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Mudel ei leidnud sellest vestlusest signaale.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Mudel ei kirjutanud sellest vestlusest midagi.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 kustutatakse. Vestlus ja tema ülestähendus jäävad alles ja te võite seda uuesti küsida. Kustutamist ei saa tagasi võtta.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Töötlemine</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Töötlemine</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Töötle vestlusi automaatselt</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Väljas ei juhtu midagi, kuni te seda salvestiste aknas ei palu. Sees töötavad allolevad reeglid — ja just see teeb vestlusest kokkuvõtte, kategooria ja kõik muu, ilma et keegi midagi vajutaks. Pilves olev mudel võtab igaühe eest tasu.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Rahapiir, kuus</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Märkide piir, kuus</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Kaks piiri, kahes asjas, milles kuud saab lugeda. Niipea kui kumbki saavutatakse, peatuvad reeglid kuu vahetuseni. Ise millegi palumist ei peatata kunagi.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Keelemudelid</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Lisa</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>See, mis loeb ülestähendust ja kirjutab sellest. Mudel teie oma masinal hoiab iga vestluse majas ja tema käitamine ei maksa midagi.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>vaikimisi</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>väljas</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Juhised</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>See, mida mudelitelt palutakse. Igaüks neist tuli programmiga ja igaüks on teie oma, et seda muuta — ja tagasi panna.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>muudetud</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>väljas</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Reeglid</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>See, mis töötab iseenesest, selles järjekorras. Igaüks käivitub kõige rohkem korra vestluse kohta.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Liiguta üles</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Liiguta alla</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Vaikeväärtused</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Paneb juhised ja reeglid tagasi selliseks, nagu need programmiga tulid. Teie keelemudelid jäävad puutumata.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Taasta vaikeväärtused</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Kulud</translation>
    </message>
</context>
</TS>
