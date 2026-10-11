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
<TS version="2.1" language="es" sourcelanguage="en">
<context>
    <name>ReadingPanel</name>
    <message>
        <source>Transcribe</source>
        <translation>Transcribir</translation>
    </message>
    <message>
        <source>Use this transcription</source>
        <translation>Usar esta transcripción</translation>
    </message>
    <message>
        <source>Copy</source>
        <extracomment>Puts the transcript on the clipboard.</extracomment>
        <translation>Copiar</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Guardar como…</translation>
    </message>
    <message>
        <source>Delete this transcription</source>
        <extracomment>Takes one reading of a conversation away. The conversation itself is untouched.</extracomment>
        <translation>Eliminar esta transcripción</translation>
    </message>
    <message>
        <source>Listening to it…</source>
        <translation>Escuchándola…</translation>
    </message>
    <message>
        <source>This conversation has not been transcribed. Set up a recogniser in the settings, then press Transcribe.</source>
        <translation>Esta conversación no se ha transcrito. Configure un reconocedor en los ajustes y luego pulse Transcribir.</translation>
    </message>
    <message>
        <source>The recogniser heard nothing in this conversation. That is usually silence, a call too short to hear, or the wrong language — try another recogniser from the list above.</source>
        <translation>El reconocedor no ha oído nada en esta conversación. Suele ser silencio, una llamada demasiado corta para oírse, o el idioma equivocado — pruebe otro reconocedor de la lista de arriba.</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>%1 (default)</source>
        <extracomment>One recogniser in the menu that asks which to use, the one everything uses when nothing says otherwise.</extracomment>
        <translation>%1 (por defecto)</translation>
    </message>
    <message>
        <source>As text</source>
        <extracomment>Plain text, the shape somebody pastes into a message.</extracomment>
        <translation>Como texto</translation>
    </message>
    <message>
        <source>As subtitles (SRT)</source>
        <extracomment>SubRip subtitles, which every video tool reads.</extracomment>
        <translation>Como subtítulos (SRT)</translation>
    </message>
    <message>
        <source>As subtitles (VTT)</source>
        <extracomment>WebVTT subtitles, which a browser reads.</extracomment>
        <translation>Como subtítulos (VTT)</translation>
    </message>
    <message>
        <source>Source (JSON)</source>
        <extracomment>The recogniser&apos;s own answer, kept exactly as it arrived.</extracomment>
        <translation>Fuente (JSON)</translation>
    </message>
    <message>
        <source>Delete this transcription?</source>
        <translation>¿Eliminar esta transcripción?</translation>
    </message>
    <message>
        <source>Delete</source>
        <translation>Eliminar</translation>
    </message>
    <message>
        <source>The conversation and its sound are untouched. What the recogniser made of it goes, and so does its own answer beside the file.</source>
        <translation>La conversación y su sonido quedan intactos. Lo que el reconocedor hizo de ella se pierde, y su propia respuesta junto al archivo también.</translation>
    </message>
    <message>
        <source>Save the transcript</source>
        <translation>Guardar la transcripción</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Archivos de texto (*.txt)</translation>
    </message>
    <message>
        <source>Subtitles (*.srt *.vtt)</source>
        <translation>Subtítulos (*.srt *.vtt)</translation>
    </message>
    <message>
        <source>Data files (*.json)</source>
        <translation>Archivos de datos (*.json)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Todos los archivos (*)</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>A line of the transcript, on the right button: put this line&apos;s words on the clipboard.</extracomment>
        <translation>Copiar esta línea</translation>
    </message>
    <message>
        <source>Copy the whole transcript</source>
        <extracomment>A line of the transcript, on the right button: put the whole conversation on the clipboard, as the button above the panel does.</extracomment>
        <translation>Copiar toda la transcripción</translation>
    </message>
    <message>
        <source>No recogniser is set up yet. Add one in Settings, under Transcription.</source>
        <translation>Todavía no hay ningún reconocedor configurado. Añada uno en Ajustes, en Transcripción.</translation>
    </message>
    <message>
        <source>Transcription</source>
        <translation>Transcripción</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now</source>
        <translation>Esta conversación se está transcribiendo ahora</translation>
    </message>
    <message>
        <source>This conversation is still being recorded</source>
        <translation>Esta conversación todavía se está grabando</translation>
    </message>
    <message>
        <source>Send this conversation to the recogniser</source>
        <translation>Enviar esta conversación al reconocedor</translation>
    </message>
    <message>
        <source>The sound of this conversation is no longer here</source>
        <translation>El sonido de esta conversación ya no está aquí</translation>
    </message>
    <message>
        <source>Make this the transcription that searching and the summaries use</source>
        <translation>Hacer que esta sea la transcripción que usan la búsqueda y los resúmenes</translation>
    </message>
</context>
<context>
    <name>SettingsTranscription</name>
    <message>
        <source>Transcription</source>
        <translation>Transcripción</translation>
    </message>
    <message>
        <source>A conversation is transcribed when you ask for it in the recordings window — or by itself, if you have turned processing on under Processing. A recogniser on your own machine costs nothing to run; one in the cloud charges by the minute of audio.</source>
        <translation>Una conversación se transcribe cuando lo pide en la ventana de grabaciones — o sola, si ha activado el procesamiento en Procesamiento. Un reconocedor en su propia máquina no cuesta nada hacerlo funcionar; uno en la nube cobra por minuto de audio.</translation>
    </message>
    <message>
        <source>Recognisers</source>
        <translation>Reconocedores</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Añadir</translation>
    </message>
    <message>
        <source>Nothing is set up yet. A Whisper server on your own machine keeps every conversation in the building; OpenAI needs a key and an internet connection.</source>
        <translation>Todavía no hay nada configurado. Un servidor Whisper en su propia máquina mantiene cada conversación dentro del edificio; OpenAI necesita una clave y conexión a internet.</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a recogniser</comment>
        <extracomment>Marks a recogniser that has been switched off. Marks a recogniser that is switched off. Disambiguated because it agrees with a different noun than the language model and the prompt this word also marks.</extracomment>
        <translation>apagado</translation>
    </message>
    <message>
        <source>Transcribes recorded conversations</source>
        <extracomment>The tip on the mark saying this recogniser can transcribe a finished recording.</extracomment>
        <translation>Transcribe conversaciones grabadas</translation>
    </message>
    <message>
        <source>Listens while a conversation is happening</source>
        <extracomment>The tip on the mark saying this recogniser can listen while a conversation is happening.</extracomment>
        <translation>Escucha una conversación en curso</translation>
    </message>
    <message>
        <source>by default</source>
        <extracomment>Marks the recogniser used for a job when nothing says otherwise. Appears after the sentence saying what the recogniser can do.</extracomment>
        <translation>por defecto</translation>
    </message>
    <message>
        <source>%1 · %2</source>
        <translation>%1 · %2</translation>
    </message>
    <message>
        <source>Press to change</source>
        <extracomment>Added to the tip on a recogniser&apos;s mark: pressing it makes this the one used by default, or stops it being that.</extracomment>
        <translation>Pulse para cambiar</translation>
    </message>
</context>
<context>
    <name>Transcription</name>
    <message>
        <source>Transcription</source>
        <translation>Transcripción</translation>
    </message>
</context>
</TS>
