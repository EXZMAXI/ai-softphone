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
<TS version="2.1" language="tr" sourcelanguage="en">
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
        <translation>Kopyala</translation>
    </message>
    <message>
        <source>Save as…</source>
        <translation>Farklı kaydet…</translation>
    </message>
    <message>
        <source>Delete</source>
        <extracomment>Throws away what a model wrote about this conversation.</extracomment>
        <translation>Sil</translation>
    </message>
    <message>
        <source>Working on it…</source>
        <extracomment>Shown while a model is still working on this conversation.</extracomment>
        <translation>Üzerinde çalışıyorum…</translation>
    </message>
    <message>
        <source>Save the answer</source>
        <extracomment>Keeping what a model wrote about one conversation.</extracomment>
        <translation>Yanıtı kaydet</translation>
    </message>
    <message>
        <source>Text files (*.txt)</source>
        <translation>Metin dosyaları (*.txt)</translation>
    </message>
    <message>
        <source>All files (*)</source>
        <translation>Tüm dosyalar (*)</translation>
    </message>
    <message>
        <source>Delete this?</source>
        <extracomment>Asked before throwing away what a model wrote.</extracomment>
        <translation>Bu silinsin mi?</translation>
    </message>
    <message>
        <source>Ask</source>
        <translation>Sor</translation>
    </message>
    <message>
        <source>Your question</source>
        <extracomment>The box a question about one conversation is typed into.</extracomment>
        <translation>Sorunuz</translation>
    </message>
    <message>
        <source>The model answers from this conversation.</source>
        <translation>Model bu görüşmeden yanıtlar.</translation>
    </message>
    <message>
        <source>Copy this answer</source>
        <extracomment>An answer a model wrote, on the right button: put the whole of it on the clipboard, as the button at the top does.</extracomment>
        <translation>Bu yanıtı kopyala</translation>
    </message>
    <message>
        <source>This conversation is being transcribed now.</source>
        <translation>Bu görüşme şu anda yazıya dökülüyor.</translation>
    </message>
    <message>
        <source>Copy the exchange with the model</source>
        <extracomment>On the right button over a model&apos;s answer: puts the request this phone sent and the answer it got back, as JSON, on the clipboard. Kept for an hour.</extracomment>
        <translation>Modelle yapılan yazışmayı kopyala</translation>
    </message>
    <message>
        <source>Copy this line</source>
        <extracomment>One entry of a model&apos;s answer — a red flag, an action, a quotation — on the right button: put that one on the clipboard.</extracomment>
        <translation>Bu satırı kopyala</translation>
    </message>
    <message>
        <source>Processing</source>
        <translation>İşleme</translation>
    </message>
    <message>
        <source>The model raised no red flags in this conversation.</source>
        <extracomment>A model&apos;s red flags came back with none in them.</extracomment>
        <translation>Model bu görüşmede uyarı işareti bulmadı.</translation>
    </message>
    <message>
        <source>The model wrote nothing about this conversation.</source>
        <extracomment>A model&apos;s answer about a conversation came back with nothing in it.</extracomment>
        <translation>Model bu görüşme hakkında hiçbir şey yazmadı.</translation>
    </message>
    <message>
        <source>%1 will be deleted. The conversation and its transcript are kept, and you can ask for it again. Deleting cannot be undone.</source>
        <translation>%1 silinecek. Görüşme ve dökümü korunur, bunu yeniden isteyebilirsiniz. Silme geri alınamaz.</translation>
    </message>
</context>
<context>
    <name>Processing</name>
    <message>
        <source>Processing</source>
        <translation>İşleme</translation>
    </message>
</context>
<context>
    <name>SettingsProcessing</name>
    <message>
        <source>Processing</source>
        <translation>İşleme</translation>
    </message>
    <message>
        <source>Process conversations automatically</source>
        <extracomment>The one switch that lets the rules below run.</extracomment>
        <translation>Görüşmeleri kendiliğinden işle</translation>
    </message>
    <message>
        <source>Off, nothing happens until you ask for it in the recordings window. On, the rules below run — which is what turns a conversation into a summary, a category and everything else without anybody pressing anything. A model in the cloud charges for each of those.</source>
        <translation>Kapalıyken, kayıtlar penceresinde siz istemedikçe hiçbir şey olmaz. Açıkken aşağıdaki kurallar çalışır — bir görüşmeyi kimse bir şeye basmadan bir özete, bir kategoriye ve geri kalan her şeye dönüştüren budur. Buluttaki bir model bunların her biri için ücret alır.</translation>
    </message>
    <message>
        <source>Money limit, monthly</source>
        <translation>Para sınırı, aylık</translation>
    </message>
    <message>
        <source>Token limit, monthly</source>
        <translation>Belirteç sınırı, aylık</translation>
    </message>
    <message>
        <source>Two limits, in the two things a month can be counted in. When either is reached the rules stop until the month turns. Asking for something yourself is never stopped.</source>
        <translation>Bir ayın sayılabileceği iki şeyde iki sınır. Hangisine ulaşılırsa, ay dönene kadar kurallar durur. Kendiniz bir şey istemek hiçbir zaman durdurulmaz.</translation>
    </message>
    <message>
        <source>Language models</source>
        <translation>Dil modelleri</translation>
    </message>
    <message>
        <source>Add</source>
        <translation>Ekle</translation>
    </message>
    <message>
        <source>What reads a transcript and writes about it. A model on your own machine keeps every conversation in the building and costs nothing to run.</source>
        <translation>Bir dökümü okuyup onun hakkında yazan şey. Kendi makinenizdeki bir model her görüşmeyi bina içinde tutar ve çalışması hiçbir şeye mal olmaz.</translation>
    </message>
    <message>
        <source>default</source>
        <extracomment>Marks the model everything uses when nothing says otherwise.</extracomment>
        <translation>öntanımlı</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a language model</comment>
        <extracomment>Marks a language model that has been switched off. Disambiguated from the prompt&apos;s own &quot;off&quot; below: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>kapalı</translation>
    </message>
    <message>
        <source>Prompts</source>
        <translation>Yönergeler</translation>
    </message>
    <message>
        <source>What the models are asked. Every one of these came with the program and every one of them is yours to change — and to put back.</source>
        <translation>Modellerden istenenler. Bunların her biri programla birlikte geldi ve her birini değiştirmek — ve geri koymak — size kalmış.</translation>
    </message>
    <message>
        <source>changed</source>
        <extracomment>Marks a shipped prompt whose words somebody has changed.</extracomment>
        <translation>değiştirildi</translation>
    </message>
    <message>
        <source>off</source>
        <comment>a prompt</comment>
        <extracomment>Marks a prompt that has been switched off. Disambiguated from the language model&apos;s own &quot;off&quot; above: the two agree with different nouns in every Romance and Slavic language M16 ships.</extracomment>
        <translation>kapalı</translation>
    </message>
    <message>
        <source>Rules</source>
        <translation>Kurallar</translation>
    </message>
    <message>
        <source>What runs by itself, in this order. Each one fires at most once per conversation.</source>
        <translation>Kendi kendine bu düzende çalışanlar. Her biri görüşme başına en çok bir kez ateşlenir.</translation>
    </message>
    <message>
        <source>Move up</source>
        <translation>Yukarı taşı</translation>
    </message>
    <message>
        <source>Move down</source>
        <translation>Aşağı taşı</translation>
    </message>
    <message>
        <source>Defaults</source>
        <translation>Öntanımlılar</translation>
    </message>
    <message>
        <source>Puts the prompts and the rules back as they came with the program. Your language models are left alone.</source>
        <translation>Yönergeleri ve kuralları programla geldikleri biçime döndürür. Dil modelleriniz olduğu gibi bırakılır.</translation>
    </message>
    <message>
        <source>Restore defaults</source>
        <translation>Öntanımlılara dön</translation>
    </message>
    <message>
        <source>Spending</source>
        <extracomment>A heading: the two monthly ceilings below.</extracomment>
        <translation>Harcama</translation>
    </message>
</context>
</TS>
