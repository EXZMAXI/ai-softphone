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
<TS version="2.1" language="hu" sourcelanguage="en">
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
        <translation>Másolás</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Mentés másként…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Törlés</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Dolgozom rajta…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>A válasz mentése</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Szövegfájlok (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Minden fájl (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Törli ezt?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Kérdés</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Az Ön kérdése</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>A modell ebből a beszélgetésből válaszol.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>A válasz másolása</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Ezt a beszélgetést épp most írjuk le.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>A modellel folytatott kommunikáció másolása</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>A sor másolása</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Feldolgozás</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>A modell nem talált jelzéseket ebben a beszélgetésben.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>A modell semmit sem írt erről a beszélgetésről.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 törlődik. A beszélgetés és az átirata megmarad, és újra kérheti. A törlést nem lehet visszavonni.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Feldolgozás</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Feldolgozás</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>A beszélgetések automatikus feldolgozása</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Kikapcsolva semmi sem történik, amíg nem kéri a felvételek ablakában. Bekapcsolva az alábbi szabályok futnak — és épp ez az, ami a beszélgetésből összefoglalót, kategóriát és minden mást csinál anélkül, hogy bárki bármit megnyomna. A felhőben lévő modell mindegyikért fizetni kér.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Pénzkorlát, havonta</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Tokenkorlát, havonta</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Két korlát, abban a két dologban, amelyben egy hónap számolható. Amint bármelyik elérésre kerül, a szabályok megállnak a hónapfordulóig. Az, hogy Ön kér valamit, soha nem áll meg.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Nyelvi modellek</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Hozzáadás</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Az, ami elolvassa az átiratot és ír róla. A saját gépén futó modell minden beszélgetést az épületen belül tart, és a futtatása semmibe sem kerül.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>alapértelmezett</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>ki</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Utasítások</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Az, amit a modellektől kérünk. Mindegyik a programmal érkezett, és mindegyik az Öné, hogy megváltoztassa — és hogy visszaállítsa.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>módosítva</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>ki</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Szabályok</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Az, ami magától fut, ebben a sorrendben. Mindegyik legfeljebb egyszer sül el beszélgetésenként.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Feljebb</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Lejjebb</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Alapértékek</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Visszaállítja az utasításokat és a szabályokat úgy, ahogy a programmal érkeztek. A nyelvi modelljei érintetlenek maradnak.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Alapértékek visszaállítása</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Kiadás</translation>
    </message>
</context>
</TS>
