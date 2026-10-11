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
<TS version="2.1" language="fi" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Litteroi</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Käytä tätä litterointia</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Kopioi</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Tallenna nimellä…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Poista tämä litterointi</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Kuunnellaan sitä…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Tätä keskustelua ei ole litteroitu. Määritä tunnistin asetuksista ja paina sitten Litteroi.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>Tunnistin ei kuullut tässä keskustelussa mitään. Se on yleensä hiljaisuutta, liian lyhyt puhelu kuultavaksi, tai väärä kieli — kokeile toista tunnistinta yllä olevasta listasta.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (oletus)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Tekstinä</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Tekstityksinä (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Tekstityksinä (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Lähde (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>Poistetaanko tämä litterointi?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Poista</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>Keskustelu ja sen ääni jäävät koskematta. Se, mitä tunnistin siitä teki, katoaa, ja sen oma vastaus tiedoston vierestä samoin.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Tallenna litterointi</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Tekstitiedostot (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Tekstitykset (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Datatiedostot (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Kaikki tiedostot (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Kopioi tämä rivi</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Kopioi koko litterointi</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Yhtään tunnistinta ei ole vielä määritetty. Lisää sellainen Asetuksissa, kohdassa Litterointi.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Litterointi</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Tätä keskustelua litteroidaan juuri nyt</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Tätä keskustelua tallennetaan yhä</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Lähetä tämä keskustelu tunnistimelle</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>Tämän keskustelun ääntä ei ole enää täällä</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Tee tästä se litterointi, jota haku ja tiivistelmät käyttävät</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Litterointi</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Keskustelu litteroidaan, kun pyydät sitä tallenneikkunassa — tai itsestään, jos olet kytkenyt käsittelyn päälle kohdasta Käsittely. Omalla koneellasi oleva tunnistin ei maksa mitään ajaa; pilvessä oleva veloittaa äänen minuutilta.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Tunnistimet</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Lisää</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Mitään ei ole vielä määritetty. Omalla koneellasi oleva Whisper-palvelin pitää jokaisen keskustelun talon sisällä; OpenAI vaatii avaimen ja internet-yhteyden.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>pois</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Purkaa tallennetut keskustelut tekstiksi</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Kuuntelee keskustelua sen aikana</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>oletuksena</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Muuta painamalla</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Litterointi</translation>
    </message>
</context>
</TS>
