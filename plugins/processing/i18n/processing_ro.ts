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
<TS version="2.1" language="ro" sourcelanguage="en">
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
        <translation>Copiază</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Salvează ca…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Șterge</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Lucrez la asta…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Salvează răspunsul</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Fișiere text (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Toate fișierele (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Ștergeți asta?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Întreabă</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Întrebarea dumneavoastră</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Modelul răspunde pornind de la această conversație.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Copiază acest răspuns</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Această conversație se transcrie chiar acum.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Copiază schimbul cu modelul</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Copiază acest rând</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Prelucrare</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Modelul nu a găsit semnale în această conversație.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Modelul nu a scris nimic despre această conversație.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 va fi șters. Conversația și transcrierea ei rămân, iar dumneavoastră puteți cere din nou. Ștergerea nu se poate anula.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Prelucrare</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Prelucrare</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Prelucrează conversațiile automat</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Oprit, nu se întâmplă nimic până nu cereți în fereastra înregistrărilor. Pornit, rulează regulile de mai jos — și tocmai asta transformă o conversație într-un rezumat, o categorie și tot restul fără ca cineva să apese ceva. Un model din nor taxează fiecare dintre ele.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Limită de bani, lunar</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Limită de tokenuri, lunar</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Două limite, în cele două lucruri în care se poate număra o lună. Când oricare este atinsă, regulile se opresc până la schimbarea lunii. A cere ceva dumneavoastră nu se oprește niciodată.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Modele de limbă</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Adaugă</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Ceea ce citește o transcriere și scrie despre ea. Un model pe propria mașină ține fiecare conversație în clădire și nu costă nimic să meargă.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>implicit</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>oprit</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Instrucțiuni</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Ceea ce li se cere modelelor. Fiecare dintre acestea a venit cu programul și fiecare este a dumneavoastră ca să o schimbați — și ca să o puneți la loc.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>schimbată</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>oprită</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Reguli</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Ceea ce rulează de la sine, în această ordine. Fiecare se declanșează cel mult o dată pe conversație.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Mută mai sus</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Mută mai jos</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Valori implicite</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Pune instrucțiunile și regulile la loc așa cum au venit cu programul. Modelele dumneavoastră de limbă rămân neatinse.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Restabilește valorile implicite</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Cheltuieli</translation>
    </message>
</context>
</TS>
