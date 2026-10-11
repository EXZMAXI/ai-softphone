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
<TS version="2.1" language="sk" sourcelanguage="en">
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
        <translation>Kopírovať</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Uložiť ako…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Zmazať</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Pracujem na tom…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Uložiť odpoveď</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textové súbory (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Všetky súbory (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Zmazať toto?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Spýtať sa</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Vaša otázka</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Model odpovedá z tohto hovoru.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Kopírovať túto odpoveď</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Tento hovor sa práve prepisuje.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Kopírovať komunikáciu s modelom</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Kopírovať tento riadok</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Spracovanie</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Model v tomto hovore nenašiel žiadne signály.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Model o tomto hovore nič nenapísal.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 sa zmaže. Hovor a jeho prepis zostanú a môžete o to požiadať znova. Zmazanie sa nedá vrátiť.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Spracovanie</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Spracovanie</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Spracovávať hovory automaticky</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Vypnuté sa nič nedeje, kým o to nepožiadate v okne nahrávok. Zapnuté bežia pravidlá nižšie — a práve to premení hovor na zhrnutie, kategóriu a všetko ostatné bez toho, aby niekto na čokoľvek klikal. Model v oblaku si za každú z tých vecí účtuje.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Hranica peňazí, mesačne</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Hranica tokenov, mesačne</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Dve hranice, v dvoch veciach, v ktorých sa mesiac dá počítať. Len čo je ktorákoľvek dosiahnutá, pravidlá sa zastavia do zmeny mesiaca. Požiadať o niečo sami sa nikdy nezastaví.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Jazykové modely</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Pridať</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>To, čo číta prepis a píše o ňom. Model na vašom vlastnom stroji drží každý hovor v budove a jeho beh nič nestojí.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>predvolený</translation>
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
        <translation>To, o čo sú modely žiadané. Každý z nich prišiel s programom a každý je váš na zmenu — aj na vrátenie.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>zmenené</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>vypnutý</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Pravidlá</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>To, čo beží samo, v tomto poradí. Každé sa spustí najviac raz na hovor.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Posunúť hore</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Posunúť dole</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Predvolené</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Vráti pokyny a pravidlá tak, ako prišli s programom. Vaše jazykové modely zostanú nedotknuté.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Obnoviť predvolené</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Výdavky</translation>
    </message>
</context>
</TS>
