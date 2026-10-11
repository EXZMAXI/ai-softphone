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
<TS version="2.1" language="de" sourcelanguage="en">
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
        <translation>Kopieren</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Speichern unter…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Löschen</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Wird bearbeitet…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Die Antwort speichern</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Textdateien (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Alle Dateien (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Dies löschen?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Fragen</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Ihre Frage</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Das Modell antwortet aus diesem Gespräch.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Diese Antwort kopieren</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Dieses Gespräch wird gerade transkribiert.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Austausch mit dem Modell kopieren</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Diese Zeile kopieren</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>Verarbeitung</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Das Modell hat in diesem Gespräch keine Auffälligkeiten gefunden.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Das Modell hat nichts zu diesem Gespräch geschrieben.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 wird gelöscht. Das Gespräch und sein Transkript bleiben erhalten, und Sie können erneut danach fragen. Das Löschen lässt sich nicht rückgängig machen.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>Verarbeitung</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>Verarbeitung</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Gespräche automatisch verarbeiten</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Aus geschieht nichts, bis Sie es im Aufnahmenfenster verlangen. An laufen die Regeln unten — das ist es, was aus einem Gespräch eine Zusammenfassung, eine Kategorie und alles Übrige macht, ohne dass jemand etwas drückt. Ein Modell in der Cloud berechnet jedes davon.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Geldgrenze, monatlich</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Token-Grenze, monatlich</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Zwei Grenzen, in den zwei Größen, in denen sich ein Monat zählen lässt. Ist eine erreicht, ruhen die Regeln bis zum Monatswechsel. Selbst etwas anzufordern wird nie angehalten.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Sprachmodelle</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Hinzufügen</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Was ein Transkript liest und darüber schreibt. Ein Modell auf Ihrem eigenen Rechner behält jedes Gespräch im Haus und kostet im Betrieb nichts.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>Vorgabe</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Prompts</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Was die Modelle gefragt werden. Jeder davon kam mit dem Programm, und jeder ist Ihrer, um ihn zu ändern — und zurückzusetzen.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>geändert</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Regeln</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Was von selbst läuft, in dieser Reihenfolge. Jede greift höchstens einmal je Gespräch.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Nach oben</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Nach unten</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Voreinstellungen</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Setzt die Prompts und die Regeln so zurück, wie sie mit dem Programm kamen. Ihre Sprachmodelle bleiben unberührt.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Voreinstellungen wiederherstellen</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>aus</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>aus</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Ausgaben</translation>
    </message>
</context>
</TS>
