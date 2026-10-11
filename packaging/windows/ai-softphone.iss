; AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
; https://ai-softphone.com
;
; SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
; SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
; SPDX-License-Identifier: GPL-2.0-or-later
;
; This file is part of AI Softphone. There is NO WARRANTY, to the extent
; permitted by law. See the file LICENSE for the full terms.

#ifndef AppVersion
  #error AppVersion was not given
#endif
#ifndef StagingDir
  #error StagingDir was not given
#endif
#ifndef OutputDir
  #error OutputDir was not given
#endif
#ifndef IconFile
  #error IconFile was not given
#endif

#define AppName "AI Softphone"
#define AppExe "bin\ai-softphone.exe"
#define AppPublisher "AI Softphone"
#define AppUrl "https://ai-softphone.com"

[Setup]
AppId={{F9495AEB-CFED-4525-B1E0-A1CE4C03A0A3}

AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL={#AppUrl}
AppSupportURL={#AppUrl}
AppUpdatesURL={#AppUrl}
VersionInfoVersion={#AppVersion}

PrivilegesRequired=lowest
DefaultDirName={localappdata}\Programs\ai-softphone

DefaultGroupName={#AppName}
DisableProgramGroupPage=yes

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

MinVersion=10.0.17763

LanguageDetectionMethod=uilanguage
ShowLanguageDialog=auto

UsePreviousLanguage=no

#ifdef SignWith
SignTool=softphone
SignedUninstaller=yes
#endif

OutputDir={#OutputDir}
OutputBaseFilename=ai-softphone-{#AppVersion}-x64
SetupIconFile={#IconFile}
UninstallDisplayIcon={app}\{#AppExe}
UninstallDisplayName={#AppName}

Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll
RestartApplications=no

SetupMutex=ai-softphone-setup

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"; LicenseFile: "{#StagingDir}\licence\en.txt"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"; LicenseFile: "{#StagingDir}\licence\ru.txt"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"; LicenseFile: "{#StagingDir}\licence\de.txt"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"; LicenseFile: "{#StagingDir}\licence\fr.txt"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"; LicenseFile: "{#StagingDir}\licence\es.txt"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"; LicenseFile: "{#StagingDir}\licence\it.txt"
Name: "portuguese"; MessagesFile: "compiler:Languages\Portuguese.isl"; LicenseFile: "{#StagingDir}\licence\pt.txt"
Name: "dutch"; MessagesFile: "compiler:Languages\Dutch.isl"; LicenseFile: "{#StagingDir}\licence\nl.txt"
Name: "swedish"; MessagesFile: "compiler:Languages\Swedish.isl"; LicenseFile: "{#StagingDir}\licence\sv.txt"
Name: "danish"; MessagesFile: "compiler:Languages\Danish.isl"; LicenseFile: "{#StagingDir}\licence\da.txt"
Name: "norwegian"; MessagesFile: "compiler:Languages\Norwegian.isl"; LicenseFile: "{#StagingDir}\licence\nb.txt"
Name: "finnish"; MessagesFile: "compiler:Languages\Finnish.isl"; LicenseFile: "{#StagingDir}\licence\fi.txt"
Name: "polish"; MessagesFile: "compiler:Languages\Polish.isl"; LicenseFile: "{#StagingDir}\licence\pl.txt"
Name: "czech"; MessagesFile: "compiler:Languages\Czech.isl"; LicenseFile: "{#StagingDir}\licence\cs.txt"
Name: "slovak"; MessagesFile: "compiler:Languages\Slovak.isl"; LicenseFile: "{#StagingDir}\licence\sk.txt"
Name: "hungarian"; MessagesFile: "compiler:Languages\Hungarian.isl"; LicenseFile: "{#StagingDir}\licence\hu.txt"
Name: "bulgarian"; MessagesFile: "compiler:Languages\Bulgarian.isl"; LicenseFile: "{#StagingDir}\licence\bg.txt"
Name: "slovenian"; MessagesFile: "compiler:Languages\Slovenian.isl"; LicenseFile: "{#StagingDir}\licence\sl.txt"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"; LicenseFile: "{#StagingDir}\licence\tr.txt"
Name: "ukrainian"; MessagesFile: "compiler:Languages\Ukrainian.isl"; LicenseFile: "{#StagingDir}\licence\uk.txt"
Name: "romanian"; MessagesFile: "languages\Romanian.isl"; LicenseFile: "{#StagingDir}\licence\ro.txt"
Name: "croatian"; MessagesFile: "languages\Croatian.isl"; LicenseFile: "{#StagingDir}\licence\hr.txt"
Name: "estonian"; MessagesFile: "languages\Estonian.isl"; LicenseFile: "{#StagingDir}\licence\et.txt"
Name: "latvian"; MessagesFile: "languages\Latvian.isl"; LicenseFile: "{#StagingDir}\licence\lv.txt"
Name: "lithuanian"; MessagesFile: "languages\Lithuanian.isl"; LicenseFile: "{#StagingDir}\licence\lt.txt"
Name: "greek"; MessagesFile: "languages\Greek.isl"; LicenseFile: "{#StagingDir}\licence\el.txt"
Name: "indonesian"; MessagesFile: "languages\Indonesian.isl"; LicenseFile: "{#StagingDir}\licence\id.txt"
Name: "vietnamese"; MessagesFile: "languages\Vietnamese.isl"; LicenseFile: "{#StagingDir}\licence\vi.txt"
Name: "serbian"; MessagesFile: "languages\SerbianCyrillic.isl"; LicenseFile: "{#StagingDir}\licence\sr.txt"
Name: "serbianlatin"; MessagesFile: "languages\SerbianLatin.isl"; LicenseFile: "{#StagingDir}\licence\sr_Latn.txt"

[CustomMessages]
english.LocationsTitle=Where to keep your data
russian.LocationsTitle=Где хранить ваши данные
german.LocationsTitle=Wo Ihre Daten liegen sollen
french.LocationsTitle=Où conserver vos données
spanish.LocationsTitle=Dónde guardar sus datos
italian.LocationsTitle=Dove tenere i suoi dati
portuguese.LocationsTitle=Onde guardar os seus dados
dutch.LocationsTitle=Waar uw gegevens komen te staan
swedish.LocationsTitle=Var dina data ska ligga
danish.LocationsTitle=Hvor dine data skal ligge
norwegian.LocationsTitle=Hvor dataene dine skal ligge
finnish.LocationsTitle=Minne tietosi tallennetaan
polish.LocationsTitle=Gdzie trzymać twoje dane
czech.LocationsTitle=Kde uchovávat vaše data
slovak.LocationsTitle=Kde uchovávať vaše údaje
hungarian.LocationsTitle=Hol tartsuk az adatait
bulgarian.LocationsTitle=Къде да се пазят данните ви
slovenian.LocationsTitle=Kje naj bodo vaši podatki
turkish.LocationsTitle=Verileriniz nerede tutulsun
ukrainian.LocationsTitle=Де зберігати ваші дані
romanian.LocationsTitle=Unde să fie păstrate datele dumneavoastră
croatian.LocationsTitle=Gdje čuvati vaše podatke
estonian.LocationsTitle=Kus teie andmeid hoitakse
latvian.LocationsTitle=Kur glabāt jūsu datus
lithuanian.LocationsTitle=Kur laikyti jūsų duomenis
greek.LocationsTitle=Πού θα φυλάσσονται τα δεδομένα σας
indonesian.LocationsTitle=Di mana data Anda disimpan
vietnamese.LocationsTitle=Nơi lưu dữ liệu của bạn
serbian.LocationsTitle=Где чувати ваше податке
serbianlatin.LocationsTitle=Gde čuvati vaše podatke

english.LocationsSubtitle=Settings, history, the log and your recorded conversations.
russian.LocationsSubtitle=Настройки, история, журнал и записи разговоров.
german.LocationsSubtitle=Einstellungen, Verlauf, Protokoll und Ihre aufgezeichneten Gespräche.
french.LocationsSubtitle=Réglages, historique, journal et vos conversations enregistrées.
spanish.LocationsSubtitle=Ajustes, historial, registro y sus conversaciones grabadas.
italian.LocationsSubtitle=Impostazioni, cronologia, registro e le conversazioni registrate.
portuguese.LocationsSubtitle=Definições, histórico, registo e as suas conversas gravadas.
dutch.LocationsSubtitle=Instellingen, geschiedenis, logboek en uw opgenomen gesprekken.
swedish.LocationsSubtitle=Inställningar, historik, logg och dina inspelade samtal.
danish.LocationsSubtitle=Indstillinger, historik, log og dine optagede samtaler.
norwegian.LocationsSubtitle=Innstillinger, historikk, logg og de innspilte samtalene dine.
finnish.LocationsSubtitle=Asetukset, historia, loki ja nauhoitetut keskustelusi.
polish.LocationsSubtitle=Ustawienia, historia, dziennik i nagrane rozmowy.
czech.LocationsSubtitle=Nastavení, historie, protokol a vaše nahrané hovory.
slovak.LocationsSubtitle=Nastavenia, história, denník a vaše nahraté hovory.
hungarian.LocationsSubtitle=Beállítások, előzmények, napló és a rögzített beszélgetései.
bulgarian.LocationsSubtitle=Настройки, история, дневник и записаните ви разговори.
slovenian.LocationsSubtitle=Nastavitve, zgodovina, dnevnik in vaši posneti pogovori.
turkish.LocationsSubtitle=Ayarlar, geçmiş, günlük ve kaydedilmiş görüşmeleriniz.
ukrainian.LocationsSubtitle=Налаштування, історія, журнал і записані розмови.
romanian.LocationsSubtitle=Setări, istoric, jurnal și conversațiile înregistrate.
croatian.LocationsSubtitle=Postavke, povijest, zapisnik i vaši snimljeni razgovori.
estonian.LocationsSubtitle=Sätted, ajalugu, logi ja teie salvestatud vestlused.
latvian.LocationsSubtitle=Iestatījumi, vēsture, žurnāls un jūsu ierakstītās sarunas.
lithuanian.LocationsSubtitle=Nustatymai, istorija, žurnalas ir jūsų įrašyti pokalbiai.
greek.LocationsSubtitle=Ρυθμίσεις, ιστορικό, αρχείο καταγραφής και οι ηχογραφημένες συνομιλίες σας.
indonesian.LocationsSubtitle=Pengaturan, riwayat, log, dan percakapan yang Anda rekam.
vietnamese.LocationsSubtitle=Cài đặt, lịch sử, nhật ký và các cuộc trò chuyện đã ghi âm.
serbian.LocationsSubtitle=Подешавања, историја, дневник и ваши снимљени разговори.
serbianlatin.LocationsSubtitle=Podešavanja, istorija, dnevnik i vaši snimljeni razgovori.

english.LocationsProfileHeading=Settings, history and the log
russian.LocationsProfileHeading=Настройки, история и журнал
german.LocationsProfileHeading=Einstellungen, Verlauf und Protokoll
french.LocationsProfileHeading=Réglages, historique et journal
spanish.LocationsProfileHeading=Ajustes, historial y registro
italian.LocationsProfileHeading=Impostazioni, cronologia e registro
portuguese.LocationsProfileHeading=Definições, histórico e registo
dutch.LocationsProfileHeading=Instellingen, geschiedenis en logboek
swedish.LocationsProfileHeading=Inställningar, historik och logg
danish.LocationsProfileHeading=Indstillinger, historik og log
norwegian.LocationsProfileHeading=Innstillinger, historikk og logg
finnish.LocationsProfileHeading=Asetukset, historia ja loki
polish.LocationsProfileHeading=Ustawienia, historia i dziennik
czech.LocationsProfileHeading=Nastavení, historie a protokol
slovak.LocationsProfileHeading=Nastavenia, história a denník
hungarian.LocationsProfileHeading=Beállítások, előzmények és napló
bulgarian.LocationsProfileHeading=Настройки, история и дневник
slovenian.LocationsProfileHeading=Nastavitve, zgodovina in dnevnik
turkish.LocationsProfileHeading=Ayarlar, geçmiş ve günlük
ukrainian.LocationsProfileHeading=Налаштування, історія та журнал
romanian.LocationsProfileHeading=Setări, istoric și jurnal
croatian.LocationsProfileHeading=Postavke, povijest i zapisnik
estonian.LocationsProfileHeading=Sätted, ajalugu ja logi
latvian.LocationsProfileHeading=Iestatījumi, vēsture un žurnāls
lithuanian.LocationsProfileHeading=Nustatymai, istorija ir žurnalas
greek.LocationsProfileHeading=Ρυθμίσεις, ιστορικό και αρχείο καταγραφής
indonesian.LocationsProfileHeading=Pengaturan, riwayat, dan log
vietnamese.LocationsProfileHeading=Cài đặt, lịch sử và nhật ký
serbian.LocationsProfileHeading=Подешавања, историја и дневник
serbianlatin.LocationsProfileHeading=Podešavanja, istorija i dnevnik

english.LocationsProfileKeep=Keep them where Windows puts them
russian.LocationsProfileKeep=Оставить там, куда их кладёт Windows
german.LocationsProfileKeep=Dort lassen, wo Windows sie ablegt
french.LocationsProfileKeep=Les laisser où Windows les place
spanish.LocationsProfileKeep=Dejarlos donde los pone Windows
italian.LocationsProfileKeep=Lasciarli dove li mette Windows
portuguese.LocationsProfileKeep=Deixá-los onde o Windows os põe
dutch.LocationsProfileKeep=Laten staan waar Windows ze neerzet
swedish.LocationsProfileKeep=Låt dem ligga där Windows lägger dem
danish.LocationsProfileKeep=Lad dem ligge, hvor Windows lægger dem
norwegian.LocationsProfileKeep=La dem ligge der Windows legger dem
finnish.LocationsProfileKeep=Pidä ne siellä, minne Windows ne sijoittaa
polish.LocationsProfileKeep=Zostaw je tam, gdzie umieszcza je Windows
czech.LocationsProfileKeep=Nechat je tam, kam je dává Windows
slovak.LocationsProfileKeep=Nechať ich tam, kam ich dáva Windows
hungarian.LocationsProfileKeep=Maradjanak ott, ahová a Windows teszi őket
bulgarian.LocationsProfileKeep=Да останат там, където ги слага Windows
slovenian.LocationsProfileKeep=Pustite jih tam, kamor jih da Windows
turkish.LocationsProfileKeep=Windows'un koyduğu yerde kalsın
ukrainian.LocationsProfileKeep=Залишити там, куди їх кладе Windows
romanian.LocationsProfileKeep=Lăsați-le unde le pune Windows
croatian.LocationsProfileKeep=Ostavi ih ondje gdje ih stavlja Windows
estonian.LocationsProfileKeep=Jäta need sinna, kuhu Windows need paneb
latvian.LocationsProfileKeep=Atstāt tos tur, kur tos liek Windows
lithuanian.LocationsProfileKeep=Palikti ten, kur juos padeda Windows
greek.LocationsProfileKeep=Να μείνουν εκεί που τα βάζει τα Windows
indonesian.LocationsProfileKeep=Biarkan di tempat Windows meletakkannya
vietnamese.LocationsProfileKeep=Giữ ở nơi Windows đặt chúng
serbian.LocationsProfileKeep=Остави их тамо где их ставља Windows
serbianlatin.LocationsProfileKeep=Ostavi ih tamo gde ih stavlja Windows

english.LocationsProfileNote=Settings in %1%nThe history and the log in %2
russian.LocationsProfileNote=Настройки в %1%nИстория и журнал в %2
german.LocationsProfileNote=Einstellungen in %1%nVerlauf und Protokoll in %2
french.LocationsProfileNote=Réglages dans %1%nHistorique et journal dans %2
spanish.LocationsProfileNote=Ajustes en %1%nEl historial y el registro en %2
italian.LocationsProfileNote=Impostazioni in %1%nCronologia e registro in %2
portuguese.LocationsProfileNote=Definições em %1%nO histórico e o registo em %2
dutch.LocationsProfileNote=Instellingen in %1%nDe geschiedenis en het logboek in %2
swedish.LocationsProfileNote=Inställningar i %1%nHistoriken och loggen i %2
danish.LocationsProfileNote=Indstillinger i %1%nHistorikken og loggen i %2
norwegian.LocationsProfileNote=Innstillinger i %1%nHistorikken og loggen i %2
finnish.LocationsProfileNote=Asetukset kansiossa %1%nHistoria ja loki kansiossa %2
polish.LocationsProfileNote=Ustawienia w %1%nHistoria i dziennik w %2
czech.LocationsProfileNote=Nastavení v %1%nHistorie a protokol v %2
slovak.LocationsProfileNote=Nastavenia v %1%nHistória a denník v %2
hungarian.LocationsProfileNote=Beállítások itt: %1%nAz előzmények és a napló itt: %2
bulgarian.LocationsProfileNote=Настройки в %1%nИсторията и дневникът в %2
slovenian.LocationsProfileNote=Nastavitve v %1%nZgodovina in dnevnik v %2
turkish.LocationsProfileNote=Ayarlar %1 içinde%nGeçmiş ve günlük %2 içinde
ukrainian.LocationsProfileNote=Налаштування в %1%nІсторія та журнал у %2
romanian.LocationsProfileNote=Setări în %1%nIstoricul și jurnalul în %2
croatian.LocationsProfileNote=Postavke u %1%nPovijest i zapisnik u %2
estonian.LocationsProfileNote=Sätted kaustas %1%nAjalugu ja logi kaustas %2
latvian.LocationsProfileNote=Iestatījumi mapē %1%nVēsture un žurnāls mapē %2
lithuanian.LocationsProfileNote=Nustatymai aplanke %1%nIstorija ir žurnalas aplanke %2
greek.LocationsProfileNote=Ρυθμίσεις στο %1%nΤο ιστορικό και το αρχείο καταγραφής στο %2
indonesian.LocationsProfileNote=Pengaturan di %1%nRiwayat dan log di %2
vietnamese.LocationsProfileNote=Cài đặt trong %1%nLịch sử và nhật ký trong %2
serbian.LocationsProfileNote=Подешавања у %1%nИсторија и дневник у %2
serbianlatin.LocationsProfileNote=Podešavanja u %1%nIstorija i dnevnik u %2

english.LocationsProfileMove=Put all of them in one folder of my own:
russian.LocationsProfileMove=Положить всё в свою папку:
german.LocationsProfileMove=Alles in einen eigenen Ordner legen:
french.LocationsProfileMove=Tout mettre dans un dossier à moi :
spanish.LocationsProfileMove=Ponerlo todo en una carpeta mía:
italian.LocationsProfileMove=Mettere tutto in una cartella mia:
portuguese.LocationsProfileMove=Pôr tudo numa pasta minha:
dutch.LocationsProfileMove=Alles in een eigen map zetten:
swedish.LocationsProfileMove=Lägg allt i en egen mapp:
danish.LocationsProfileMove=Læg det hele i min egen mappe:
norwegian.LocationsProfileMove=Legg alt i en egen mappe:
finnish.LocationsProfileMove=Sijoita kaikki omaan kansiooni:
polish.LocationsProfileMove=Umieść wszystko w moim własnym folderze:
czech.LocationsProfileMove=Dát vše do vlastní složky:
slovak.LocationsProfileMove=Dať všetko do vlastného priečinka:
hungarian.LocationsProfileMove=Mind kerüljön a saját mappámba:
bulgarian.LocationsProfileMove=Всичко в моя собствена папка:
slovenian.LocationsProfileMove=Vse dajte v svojo mapo:
turkish.LocationsProfileMove=Hepsi kendi klasörüme konsun:
ukrainian.LocationsProfileMove=Покласти все в мою власну теку:
romanian.LocationsProfileMove=Puneți totul într-un folder al meu:
croatian.LocationsProfileMove=Stavi sve u vlastitu mapu:
estonian.LocationsProfileMove=Pane kõik minu enda kausta:
latvian.LocationsProfileMove=Likt visu manā paša mapē:
lithuanian.LocationsProfileMove=Sudėti viską į mano paties aplanką:
greek.LocationsProfileMove=Να μπουν όλα σε δικό μου φάκελο:
indonesian.LocationsProfileMove=Letakkan semuanya di folder milik saya:
vietnamese.LocationsProfileMove=Đặt tất cả vào thư mục của riêng tôi:
serbian.LocationsProfileMove=Стави све у моју сопствену фасциклу:
serbianlatin.LocationsProfileMove=Stavi sve u moju sopstvenu fasciklu:

english.LocationsTalksHeading=Conversations
russian.LocationsTalksHeading=Разговоры
german.LocationsTalksHeading=Gespräche
french.LocationsTalksHeading=Conversations
spanish.LocationsTalksHeading=Conversaciones
italian.LocationsTalksHeading=Conversazioni
portuguese.LocationsTalksHeading=Conversas
dutch.LocationsTalksHeading=Gesprekken
swedish.LocationsTalksHeading=Samtal
danish.LocationsTalksHeading=Samtaler
norwegian.LocationsTalksHeading=Samtaler
finnish.LocationsTalksHeading=Keskustelut
polish.LocationsTalksHeading=Rozmowy
czech.LocationsTalksHeading=Hovory
slovak.LocationsTalksHeading=Hovory
hungarian.LocationsTalksHeading=Beszélgetések
bulgarian.LocationsTalksHeading=Разговори
slovenian.LocationsTalksHeading=Pogovori
turkish.LocationsTalksHeading=Görüşmeler
ukrainian.LocationsTalksHeading=Розмови
romanian.LocationsTalksHeading=Conversații
croatian.LocationsTalksHeading=Razgovori
estonian.LocationsTalksHeading=Vestlused
latvian.LocationsTalksHeading=Sarunas
lithuanian.LocationsTalksHeading=Pokalbiai
greek.LocationsTalksHeading=Συνομιλίες
indonesian.LocationsTalksHeading=Percakapan
vietnamese.LocationsTalksHeading=Cuộc trò chuyện
serbian.LocationsTalksHeading=Разговори
serbianlatin.LocationsTalksHeading=Razgovori

english.LocationsTalksKeep=Keep them beside the history, wherever that is
russian.LocationsTalksKeep=Хранить рядом с историей, где бы она ни была
german.LocationsTalksKeep=Beim Verlauf lassen, wo immer der liegt
french.LocationsTalksKeep=Les garder auprès de l'historique, où qu'il soit
spanish.LocationsTalksKeep=Guardarlas junto al historial, esté donde esté
italian.LocationsTalksKeep=Tenerle accanto alla cronologia, ovunque sia
portuguese.LocationsTalksKeep=Guardá-las junto ao histórico, onde quer que esteja
dutch.LocationsTalksKeep=Bij de geschiedenis houden, waar die ook staat
swedish.LocationsTalksKeep=Håll dem hos historiken, var den än ligger
danish.LocationsTalksKeep=Hold dem hos historikken, hvor den end ligger
norwegian.LocationsTalksKeep=Hold dem hos historikken, uansett hvor den er
finnish.LocationsTalksKeep=Pidä ne historian vieressä, olipa se missä tahansa
polish.LocationsTalksKeep=Trzymaj je przy historii, gdziekolwiek ona jest
czech.LocationsTalksKeep=Uchovávat je u historie, ať je kdekoli
slovak.LocationsTalksKeep=Uchovávať ich pri histórii, nech je kdekoľvek
hungarian.LocationsTalksKeep=Maradjanak az előzmények mellett, bárhol is legyenek
bulgarian.LocationsTalksKeep=Да се пазят до историята, където и да е тя
slovenian.LocationsTalksKeep=Hranite jih ob zgodovini, kjer koli že je
turkish.LocationsTalksKeep=Geçmişin yanında kalsın, neredeyse orada
ukrainian.LocationsTalksKeep=Зберігати поруч з історією, де б вона не була
romanian.LocationsTalksKeep=Păstrați-le lângă istoric, oriunde ar fi
croatian.LocationsTalksKeep=Čuvaj ih uz povijest, gdje god ona bila
estonian.LocationsTalksKeep=Hoia neid ajaloo kõrval, kus see ka poleks
latvian.LocationsTalksKeep=Glabāt tās pie vēstures, lai kur tā būtu
lithuanian.LocationsTalksKeep=Laikyti juos šalia istorijos, kad ir kur ji būtų
greek.LocationsTalksKeep=Να φυλάσσονται δίπλα στο ιστορικό, όπου κι αν είναι
indonesian.LocationsTalksKeep=Simpan bersama riwayat, di mana pun itu
vietnamese.LocationsTalksKeep=Giữ cạnh lịch sử, ở bất cứ đâu
serbian.LocationsTalksKeep=Чувај их уз историју, где год она била
serbianlatin.LocationsTalksKeep=Čuvaj ih uz istoriju, gde god ona bila

english.LocationsTalksMove=Keep them here instead:
russian.LocationsTalksMove=Хранить вот здесь:
german.LocationsTalksMove=Stattdessen hier ablegen:
french.LocationsTalksMove=Les garder ici plutôt :
spanish.LocationsTalksMove=Guardarlas aquí en su lugar:
italian.LocationsTalksMove=Tenerle invece qui:
portuguese.LocationsTalksMove=Guardá-las aqui:
dutch.LocationsTalksMove=In plaats daarvan hier bewaren:
swedish.LocationsTalksMove=Lägg dem här i stället:
danish.LocationsTalksMove=Læg dem her i stedet:
norwegian.LocationsTalksMove=Legg dem her i stedet:
finnish.LocationsTalksMove=Pidä ne sen sijaan täällä:
polish.LocationsTalksMove=Trzymaj je zamiast tego tutaj:
czech.LocationsTalksMove=Uchovávat je místo toho zde:
slovak.LocationsTalksMove=Uchovávať ich namiesto toho tu:
hungarian.LocationsTalksMove=Inkább itt legyenek:
bulgarian.LocationsTalksMove=Вместо това да се пазят тук:
slovenian.LocationsTalksMove=Namesto tega jih hranite tukaj:
turkish.LocationsTalksMove=Bunun yerine burada tutulsun:
ukrainian.LocationsTalksMove=Натомість зберігати тут:
romanian.LocationsTalksMove=Păstrați-le aici în schimb:
croatian.LocationsTalksMove=Umjesto toga čuvaj ih ovdje:
estonian.LocationsTalksMove=Hoia neid hoopis siin:
latvian.LocationsTalksMove=Tā vietā glabāt tās šeit:
lithuanian.LocationsTalksMove=Vietoj to laikyti juos čia:
greek.LocationsTalksMove=Να φυλάσσονται εδώ αντ' αυτού:
indonesian.LocationsTalksMove=Simpan di sini saja:
vietnamese.LocationsTalksMove=Thay vào đó giữ ở đây:
serbian.LocationsTalksMove=Уместо тога чувај их овде:
serbianlatin.LocationsTalksMove=Umesto toga čuvaj ih ovde:

english.LocationsFootnote=Uninstalling never deletes any of it, and an update never changes it.
russian.LocationsFootnote=Удаление ничего из этого не стирает, а обновление не меняет.
german.LocationsFootnote=Beim Deinstallieren wird nichts davon gelöscht, und ein Update ändert nichts daran.
french.LocationsFootnote=La désinstallation n'en supprime jamais rien, et une mise à jour n'y change rien.
spanish.LocationsFootnote=Desinstalar nunca borra nada de esto, y una actualización nunca lo cambia.
italian.LocationsFootnote=La disinstallazione non cancella nulla di tutto ciò, e un aggiornamento non lo cambia.
portuguese.LocationsFootnote=A desinstalação nunca apaga nada disto, e uma atualização nunca o altera.
dutch.LocationsFootnote=Verwijderen wist hier nooit iets van, en een update verandert er niets aan.
swedish.LocationsFootnote=Avinstallation raderar aldrig något av detta, och en uppdatering ändrar det aldrig.
danish.LocationsFootnote=Afinstallation sletter aldrig noget af det, og en opdatering ændrer det ikke.
norwegian.LocationsFootnote=Avinstallering sletter aldri noe av dette, og en oppdatering endrer det ikke.
finnish.LocationsFootnote=Ohjelman poisto ei koskaan poista mitään näistä, eikä päivitys muuta niitä.
polish.LocationsFootnote=Odinstalowanie nigdy niczego z tego nie usuwa, a aktualizacja tego nie zmienia.
czech.LocationsFootnote=Odinstalace nikdy nic z toho nesmaže a aktualizace to nemění.
slovak.LocationsFootnote=Odinštalovanie nikdy nič z toho nezmaže a aktualizácia to nemení.
hungarian.LocationsFootnote=Az eltávolítás soha nem töröl ezekből semmit, és a frissítés sem változtat rajtuk.
bulgarian.LocationsFootnote=Деинсталирането никога не изтрива нищо от това, а обновяването не го променя.
slovenian.LocationsFootnote=Odstranitev nikoli ničesar od tega ne izbriše, posodobitev pa tega ne spremeni.
turkish.LocationsFootnote=Kaldırma işlemi bunların hiçbirini silmez ve güncelleme de değiştirmez.
ukrainian.LocationsFootnote=Видалення програми ніколи нічого з цього не стирає, а оновлення не змінює.
romanian.LocationsFootnote=Dezinstalarea nu șterge niciodată nimic din toate acestea, iar o actualizare nu le schimbă.
croatian.LocationsFootnote=Deinstalacija nikad ništa od toga ne briše, a ažuriranje to ne mijenja.
estonian.LocationsFootnote=Eemaldamine ei kustuta neist kunagi midagi ja uuendus ei muuda neid.
latvian.LocationsFootnote=Atinstalēšana nekad neko no tā neizdzēš, un atjauninājums to nemaina.
lithuanian.LocationsFootnote=Pašalinimas niekada nieko iš to neištrina, o atnaujinimas to nekeičia.
greek.LocationsFootnote=Η απεγκατάσταση δεν διαγράφει ποτέ τίποτα από αυτά, και μια ενημέρωση δεν τα αλλάζει.
indonesian.LocationsFootnote=Menghapus program tidak pernah menghapus semua ini, dan pembaruan tidak mengubahnya.
vietnamese.LocationsFootnote=Gỡ cài đặt không bao giờ xoá bất cứ thứ gì trong số này, và bản cập nhật không thay đổi chúng.
serbian.LocationsFootnote=Деинсталација никада ништа од тога не брише, а ажурирање то не мења.
serbianlatin.LocationsFootnote=Deinstalacija nikada ništa od toga ne briše, a ažuriranje to ne menja.

english.LocationsPickProfile=Where to keep settings, history and the log
russian.LocationsPickProfile=Где хранить настройки, историю и журнал
german.LocationsPickProfile=Wo Einstellungen, Verlauf und Protokoll liegen sollen
french.LocationsPickProfile=Où conserver les réglages, l'historique et le journal
spanish.LocationsPickProfile=Dónde guardar los ajustes, el historial y el registro
italian.LocationsPickProfile=Dove tenere impostazioni, cronologia e registro
portuguese.LocationsPickProfile=Onde guardar definições, histórico e registo
dutch.LocationsPickProfile=Waar instellingen, geschiedenis en logboek komen te staan
swedish.LocationsPickProfile=Var inställningar, historik och logg ska ligga
danish.LocationsPickProfile=Hvor indstillinger, historik og log skal ligge
norwegian.LocationsPickProfile=Hvor innstillinger, historikk og logg skal ligge
finnish.LocationsPickProfile=Minne asetukset, historia ja loki tallennetaan
polish.LocationsPickProfile=Gdzie trzymać ustawienia, historię i dziennik
czech.LocationsPickProfile=Kde uchovávat nastavení, historii a protokol
slovak.LocationsPickProfile=Kde uchovávať nastavenia, históriu a denník
hungarian.LocationsPickProfile=Hol tartsuk a beállításokat, az előzményeket és a naplót
bulgarian.LocationsPickProfile=Къде да се пазят настройките, историята и дневникът
slovenian.LocationsPickProfile=Kje naj bodo nastavitve, zgodovina in dnevnik
turkish.LocationsPickProfile=Ayarlar, geçmiş ve günlük nerede tutulsun
ukrainian.LocationsPickProfile=Де зберігати налаштування, історію та журнал
romanian.LocationsPickProfile=Unde să fie păstrate setările, istoricul și jurnalul
croatian.LocationsPickProfile=Gdje čuvati postavke, povijest i zapisnik
estonian.LocationsPickProfile=Kus hoitakse sätteid, ajalugu ja logi
latvian.LocationsPickProfile=Kur glabāt iestatījumus, vēsturi un žurnālu
lithuanian.LocationsPickProfile=Kur laikyti nustatymus, istoriją ir žurnalą
greek.LocationsPickProfile=Πού θα φυλάσσονται οι ρυθμίσεις, το ιστορικό και το αρχείο καταγραφής
indonesian.LocationsPickProfile=Di mana pengaturan, riwayat, dan log disimpan
vietnamese.LocationsPickProfile=Nơi lưu cài đặt, lịch sử và nhật ký
serbian.LocationsPickProfile=Где чувати подешавања, историју и дневник
serbianlatin.LocationsPickProfile=Gde čuvati podešavanja, istoriju i dnevnik

english.LocationsPickTalks=Where to keep the conversations
russian.LocationsPickTalks=Где хранить разговоры
german.LocationsPickTalks=Wo die Gespräche liegen sollen
french.LocationsPickTalks=Où conserver les conversations
spanish.LocationsPickTalks=Dónde guardar las conversaciones
italian.LocationsPickTalks=Dove tenere le conversazioni
portuguese.LocationsPickTalks=Onde guardar as conversas
dutch.LocationsPickTalks=Waar de gesprekken komen te staan
swedish.LocationsPickTalks=Var samtalen ska ligga
danish.LocationsPickTalks=Hvor samtalerne skal ligge
norwegian.LocationsPickTalks=Hvor samtalene skal ligge
finnish.LocationsPickTalks=Minne keskustelut tallennetaan
polish.LocationsPickTalks=Gdzie trzymać rozmowy
czech.LocationsPickTalks=Kde uchovávat hovory
slovak.LocationsPickTalks=Kde uchovávať hovory
hungarian.LocationsPickTalks=Hol tartsuk a beszélgetéseket
bulgarian.LocationsPickTalks=Къде да се пазят разговорите
slovenian.LocationsPickTalks=Kje naj bodo pogovori
turkish.LocationsPickTalks=Görüşmeler nerede tutulsun
ukrainian.LocationsPickTalks=Де зберігати розмови
romanian.LocationsPickTalks=Unde să fie păstrate conversațiile
croatian.LocationsPickTalks=Gdje čuvati razgovore
estonian.LocationsPickTalks=Kus hoitakse vestlusi
latvian.LocationsPickTalks=Kur glabāt sarunas
lithuanian.LocationsPickTalks=Kur laikyti pokalbius
greek.LocationsPickTalks=Πού θα φυλάσσονται οι συνομιλίες
indonesian.LocationsPickTalks=Di mana percakapan disimpan
vietnamese.LocationsPickTalks=Nơi lưu các cuộc trò chuyện
serbian.LocationsPickTalks=Где чувати разговоре
serbianlatin.LocationsPickTalks=Gde čuvati razgovore

english.LocationsNeedsADirectory=That needs a directory.
russian.LocationsNeedsADirectory=Здесь нужна папка.
german.LocationsNeedsADirectory=Dafür wird ein Ordner gebraucht.
french.LocationsNeedsADirectory=Il faut un dossier ici.
spanish.LocationsNeedsADirectory=Aquí hace falta una carpeta.
italian.LocationsNeedsADirectory=Qui serve una cartella.
portuguese.LocationsNeedsADirectory=Aqui é preciso uma pasta.
dutch.LocationsNeedsADirectory=Hier is een map nodig.
swedish.LocationsNeedsADirectory=Här behövs en mapp.
danish.LocationsNeedsADirectory=Her skal der bruges en mappe.
norwegian.LocationsNeedsADirectory=Her trengs det en mappe.
finnish.LocationsNeedsADirectory=Tähän tarvitaan kansio.
polish.LocationsNeedsADirectory=Tu potrzebny jest folder.
czech.LocationsNeedsADirectory=Zde je potřeba složka.
slovak.LocationsNeedsADirectory=Tu je potrebný priečinok.
hungarian.LocationsNeedsADirectory=Ide mappa kell.
bulgarian.LocationsNeedsADirectory=Тук е нужна папка.
slovenian.LocationsNeedsADirectory=Tukaj je potrebna mapa.
turkish.LocationsNeedsADirectory=Burada bir klasör gerekiyor.
ukrainian.LocationsNeedsADirectory=Тут потрібна тека.
romanian.LocationsNeedsADirectory=Aici este nevoie de un folder.
croatian.LocationsNeedsADirectory=Ovdje je potrebna mapa.
estonian.LocationsNeedsADirectory=Siia on vaja kausta.
latvian.LocationsNeedsADirectory=Šeit ir vajadzīga mape.
lithuanian.LocationsNeedsADirectory=Čia reikia aplanko.
greek.LocationsNeedsADirectory=Εδώ χρειάζεται φάκελος.
indonesian.LocationsNeedsADirectory=Di sini diperlukan folder.
vietnamese.LocationsNeedsADirectory=Ở đây cần một thư mục.
serbian.LocationsNeedsADirectory=Овде је потребна фасцикла.
serbianlatin.LocationsNeedsADirectory=Ovde je potrebna fascikla.

english.LocationsCannotWrite=This cannot be written to:%n%n%1%n%nIt may need administrator rights, or the drive may not be there. Choose somewhere else.
russian.LocationsCannotWrite=Сюда нельзя писать:%n%n%1%n%nВозможно, нужны права администратора или диск недоступен. Выберите другую папку.
german.LocationsCannotWrite=Hierher kann nicht geschrieben werden:%n%n%1%n%nMöglicherweise sind Administratorrechte nötig, oder das Laufwerk ist nicht da. Wählen Sie einen anderen Ort.
french.LocationsCannotWrite=Impossible d'écrire ici :%n%n%1%n%nDes droits d'administrateur sont peut-être nécessaires, ou le disque est absent. Choisissez un autre endroit.
spanish.LocationsCannotWrite=Aquí no se puede escribir:%n%n%1%n%nPuede que hagan falta permisos de administrador, o que la unidad no esté. Elija otro lugar.
italian.LocationsCannotWrite=Qui non si può scrivere:%n%n%1%n%nPotrebbero servire i diritti di amministratore, oppure l'unità non c'è. Scelga un altro posto.
portuguese.LocationsCannotWrite=Não é possível escrever aqui:%n%n%1%n%nPodem ser precisos direitos de administrador, ou a unidade pode não estar presente. Escolha outro sítio.
dutch.LocationsCannotWrite=Hier kan niet naar geschreven worden:%n%n%1%n%nMogelijk zijn beheerdersrechten nodig, of het station is er niet. Kies een andere plek.
swedish.LocationsCannotWrite=Det går inte att skriva hit:%n%n%1%n%nDet kan behövas administratörsrättigheter, eller så finns inte enheten. Välj någon annan plats.
danish.LocationsCannotWrite=Der kan ikke skrives hertil:%n%n%1%n%nDer skal måske administratorrettigheder til, eller drevet er der ikke. Vælg et andet sted.
norwegian.LocationsCannotWrite=Det går ikke an å skrive hit:%n%n%1%n%nDet kan trenges administratorrettigheter, eller stasjonen er ikke der. Velg et annet sted.
finnish.LocationsCannotWrite=Tänne ei voi kirjoittaa:%n%n%1%n%nSiihen voidaan tarvita järjestelmänvalvojan oikeudet, tai asemaa ei ole. Valitse jokin muu paikka.
polish.LocationsCannotWrite=Nie można tu zapisywać:%n%n%1%n%nMogą być potrzebne uprawnienia administratora albo dysku nie ma. Wybierz inne miejsce.
czech.LocationsCannotWrite=Sem nelze zapisovat:%n%n%1%n%nMohou být potřeba práva správce, nebo disk není k dispozici. Zvolte jiné místo.
slovak.LocationsCannotWrite=Sem sa nedá zapisovať:%n%n%1%n%nMôžu byť potrebné práva správcu, alebo disk nie je k dispozícii. Zvoľte iné miesto.
hungarian.LocationsCannotWrite=Ide nem lehet írni:%n%n%1%n%nLehet, hogy rendszergazdai jog kell hozzá, vagy a meghajtó nincs jelen. Válasszon másik helyet.
bulgarian.LocationsCannotWrite=Тук не може да се пише:%n%n%1%n%nМоже да са нужни администраторски права или устройството да липсва. Изберете друго място.
slovenian.LocationsCannotWrite=Sem ni mogoče pisati:%n%n%1%n%nMorda so potrebne skrbniške pravice ali pa pogona ni. Izberite drugo mesto.
turkish.LocationsCannotWrite=Buraya yazılamıyor:%n%n%1%n%nYönetici hakları gerekebilir veya sürücü yerinde olmayabilir. Başka bir yer seçin.
ukrainian.LocationsCannotWrite=Сюди неможливо записати:%n%n%1%n%nМожуть знадобитися права адміністратора, або диска немає. Виберіть інше місце.
romanian.LocationsCannotWrite=Aici nu se poate scrie:%n%n%1%n%nPot fi necesare drepturi de administrator sau unitatea poate lipsi. Alegeți alt loc.
croatian.LocationsCannotWrite=Ovdje se ne može pisati:%n%n%1%n%nMožda su potrebna administratorska prava ili pogon nije dostupan. Odaberite drugo mjesto.
estonian.LocationsCannotWrite=Siia ei saa kirjutada:%n%n%1%n%nVõib olla vaja administraatoriõigusi või pole ketast olemas. Valige mõni teine koht.
latvian.LocationsCannotWrite=Šeit nevar rakstīt:%n%n%1%n%nVar būt vajadzīgas administratora tiesības, vai arī diska nav. Izvēlieties citu vietu.
lithuanian.LocationsCannotWrite=Čia rašyti negalima:%n%n%1%n%nGali prireikti administratoriaus teisių arba disko nėra. Pasirinkite kitą vietą.
greek.LocationsCannotWrite=Εδώ δεν είναι δυνατή η εγγραφή:%n%n%1%n%nΊσως χρειάζονται δικαιώματα διαχειριστή ή ο δίσκος να μην υπάρχει. Επιλέξτε άλλη θέση.
indonesian.LocationsCannotWrite=Tidak dapat menulis ke sini:%n%n%1%n%nMungkin diperlukan hak administrator, atau drive-nya tidak ada. Pilih tempat lain.
vietnamese.LocationsCannotWrite=Không thể ghi vào đây:%n%n%1%n%nCó thể cần quyền quản trị, hoặc ổ đĩa không có ở đó. Hãy chọn nơi khác.
serbian.LocationsCannotWrite=Овде се не може писати:%n%n%1%n%nМожда су потребна администраторска права или диск није доступан. Изаберите друго место.
serbianlatin.LocationsCannotWrite=Ovde se ne može pisati:%n%n%1%n%nMožda su potrebna administratorska prava ili disk nije dostupan. Izaberite drugo mesto.
russian.LocationsCannotWrite=Сюда нельзя писать:%n%n%1%n%nВозможно, нужны права администратора или диск недоступен. Выберите другую папку.

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
Source: "{#StagingDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#AppName}"; Filename: "{app}\{#AppExe}"
Name: "{autodesktop}\{#AppName}"; Filename: "{app}\{#AppExe}"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueName: "AI Softphone"; Flags: dontcreatekey uninsdeletevalue
Root: HKCU; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\StartupApproved\Run"; ValueName: "AI Softphone"; Flags: dontcreatekey uninsdeletevalue

Root: HKCU; Subkey: "SOFTWARE\Classes\AiSoftphone.Call"; ValueType: string; ValueName: ""; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\Classes\AiSoftphone.Call"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "SOFTWARE\Classes\AiSoftphone.Call\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"",0"
Root: HKCU; Subkey: "SOFTWARE\Classes\AiSoftphone.Call\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""

Root: HKCU; Subkey: "SOFTWARE\Classes\tel"; ValueType: string; ValueName: ""; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\Classes\tel"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "SOFTWARE\Classes\tel\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKCU; Subkey: "SOFTWARE\Classes\callto"; ValueType: string; ValueName: ""; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\Classes\callto"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "SOFTWARE\Classes\callto\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKCU; Subkey: "SOFTWARE\Classes\sip"; ValueType: string; ValueName: ""; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\Classes\sip"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "SOFTWARE\Classes\sip\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""
Root: HKCU; Subkey: "SOFTWARE\Classes\sips"; ValueType: string; ValueName: ""; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\Classes\sips"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""
Root: HKCU; Subkey: "SOFTWARE\Classes\sips\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\{#AppExe}"" ""%1"""

Root: HKCU; Subkey: "SOFTWARE\{#AppName}\Capabilities"; ValueType: string; ValueName: "ApplicationName"; ValueData: "{#AppName}"; Flags: uninsdeletekey
Root: HKCU; Subkey: "SOFTWARE\{#AppName}\Capabilities"; ValueType: string; ValueName: "ApplicationDescription"; ValueData: "Make and take SIP calls"
Root: HKCU; Subkey: "SOFTWARE\{#AppName}\Capabilities\UrlAssociations"; ValueType: string; ValueName: "tel"; ValueData: "AiSoftphone.Call"
Root: HKCU; Subkey: "SOFTWARE\{#AppName}\Capabilities\UrlAssociations"; ValueType: string; ValueName: "callto"; ValueData: "AiSoftphone.Call"
Root: HKCU; Subkey: "SOFTWARE\{#AppName}\Capabilities\UrlAssociations"; ValueType: string; ValueName: "sip"; ValueData: "AiSoftphone.Call"
Root: HKCU; Subkey: "SOFTWARE\{#AppName}\Capabilities\UrlAssociations"; ValueType: string; ValueName: "sips"; ValueData: "AiSoftphone.Call"
Root: HKCU; Subkey: "SOFTWARE\RegisteredApplications"; ValueType: string; ValueName: "{#AppName}"; ValueData: "SOFTWARE\{#AppName}\Capabilities"; Flags: uninsdeletevalue

[Run]
Filename: "{app}\{#AppExe}"; Description: "{cm:LaunchProgram,{#StringChange(AppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

Filename: "{app}\{#AppExe}"; Flags: nowait; Check: WantsRelaunch

[Code]
var
  LocationsPage: TWizardPage;

  ProfilePanel, TalksPanel: TPanel;

  ProfileStandard, ProfileElsewhere: TNewRadioButton;
  ProfileEdit: TNewEdit;
  ProfileBrowse: TNewButton;

  TalksStandard, TalksElsewhere: TNewRadioButton;
  TalksEdit: TNewEdit;
  TalksBrowse: TNewButton;

  TalksFollowNote, LocationsFootnote: TNewStaticText;

  FoundAlreadyInstalled: Boolean;

  LocationsOffered: Boolean;

function LocationsDocument: String;
begin
  Result := ExtractFilePath(ExpandConstant('{app}\{#AppExe}')) + 'paths.json';
end;

function DefaultProfileDir: String;
begin
  Result := ExpandConstant('{localappdata}\ai-softphone');
end;

function DefaultRecordingsDir: String;
begin
  Result := ExpandConstant('{localappdata}\ai-softphone\recordings');
end;

function AsJsonText(const Value: String): String;
var
  At: Integer;
  Ch: String;
begin
  Result := '';
  for At := 1 to Length(Value) do
  begin
    Ch := Copy(Value, At, 1);
    if Ch = '\' then
      Result := Result + '\\'
    else if Ch = '"' then
      Result := Result + '\"'
    else
      Result := Result + Ch;
  end;
end;

function StoredValue(const Name: String; const Fallback: String): String;
var
  Lines: TArrayOfString;
  At, Opens, Closes: Integer;
  Line: String;
begin
  Result := Fallback;
  if not FileExists(LocationsDocument) then
    Exit;
  if not LoadStringsFromFile(LocationsDocument, Lines) then
    Exit;

  for At := 0 to GetArrayLength(Lines) - 1 do
  begin
    Line := Lines[At];
    if Pos('"' + Name + '"', Line) > 0 then
    begin
      Opens := Pos(':', Line);
      if Opens = 0 then
        Continue;
      Line := Copy(Line, Opens + 1, Length(Line));
      Opens := Pos('"', Line);
      if Opens = 0 then
        Continue;
      Line := Copy(Line, Opens + 1, Length(Line));
      Closes := Pos('"', Line);
      if Closes = 0 then
        Continue;
      Line := Copy(Line, 1, Closes - 1);
      StringChangeEx(Line, '\\', '\', True);
      StringChangeEx(Line, '/', '\', True);
      if Line <> '' then
        Result := Line;
      Exit;
    end;
  end;
end;

function RecordingsBesideHistory: String;
var
  Profile: String;
begin
  Profile := '';
  if ProfileElsewhere.Checked then
    Profile := Trim(ProfileEdit.Text);

  if Profile = '' then
    Result := DefaultRecordingsDir
  else
    Result := AddBackslash(Profile) + 'data\recordings';
end;

procedure MoveWhatIsBelowTheNote(By: Integer);
begin
  if By = 0 then
    Exit;

  TalksElsewhere.Top := TalksElsewhere.Top + By;
  TalksPanel.Height := TalksPanel.Height + By;

  TalksEdit.Top := TalksEdit.Top + By;
  TalksBrowse.Top := TalksBrowse.Top + By;
  LocationsFootnote.Top := LocationsFootnote.Top + By;
end;

procedure ShowWhereTalksFollow;
var
  Wanted: String;
  Was: Integer;
begin
  if TalksFollowNote = nil then
    Exit;

  Wanted := RecordingsBesideHistory;

  if not TalksElsewhere.Checked then
    TalksEdit.Text := Wanted;

  if TalksFollowNote.Caption = Wanted then
    Exit;

  Was := TalksFollowNote.Height;
  TalksFollowNote.Caption := Wanted;
  TalksFollowNote.AdjustHeight;
  MoveWhatIsBelowTheNote(TalksFollowNote.Height - Was);
end;

procedure ProfileWhereaboutsChanged(Sender: TObject);
begin
  ShowWhereTalksFollow;
end;

procedure LocationsChoiceClicked(Sender: TObject);
begin
  ProfileEdit.Enabled := ProfileElsewhere.Checked;
  ProfileBrowse.Enabled := ProfileElsewhere.Checked;
  TalksEdit.Enabled := TalksElsewhere.Checked;
  TalksBrowse.Enabled := TalksElsewhere.Checked;

  ShowWhereTalksFollow;
end;

procedure BrowseProfileClicked(Sender: TObject);
var
  Chosen: String;
begin
  Chosen := ProfileEdit.Text;
  if BrowseForFolder(CustomMessage('LocationsPickProfile'), Chosen, True) then
    ProfileEdit.Text := Chosen;
end;

procedure BrowseTalksClicked(Sender: TObject);
var
  Chosen: String;
begin
  Chosen := TalksEdit.Text;
  if BrowseForFolder(CustomMessage('LocationsPickTalks'), Chosen, True) then
    TalksEdit.Text := Chosen;
end;

function AddLocationGroup(Top: Integer; const Title, KeepCaption, KeepNote,
                          MoveCaption: String; var Panel: TPanel;
                          var Keep, Move: TNewRadioButton; var Edit: TNewEdit;
                          var Browse: TNewButton;
                          var Note: TNewStaticText): Integer;
var
  Heading: TNewStaticText;
  GroupTop: Integer;
begin
  Heading := TNewStaticText.Create(LocationsPage);
  Heading.Parent := LocationsPage.Surface;
  Heading.Left := 0;
  Heading.Top := Top;
  Heading.Width := LocationsPage.SurfaceWidth;
  Heading.Font.Style := [fsBold];
  Heading.Caption := Title;
  Heading.AdjustHeight;
  Top := Top + Heading.Height + ScaleY(4);

  GroupTop := Top;

  Panel := TPanel.Create(LocationsPage);
  Panel.Parent := LocationsPage.Surface;
  Panel.Left := 0;
  Panel.Top := GroupTop;
  Panel.Width := LocationsPage.SurfaceWidth;
  Panel.BevelOuter := bvNone;
  Panel.ParentBackground := True;

  Keep := TNewRadioButton.Create(LocationsPage);
  Keep.Parent := Panel;
  Keep.Left := 0;
  Keep.Top := 0;
  Keep.Width := Panel.Width;
  Keep.Caption := KeepCaption;
  Keep.Checked := True;
  Keep.OnClick := @LocationsChoiceClicked;
  Top := Top + Keep.Height + ScaleY(2);

  Note := TNewStaticText.Create(LocationsPage);
  Note.Parent := LocationsPage.Surface;
  Note.Left := ScaleX(17);
  Note.Top := Top;
  Note.Width := LocationsPage.SurfaceWidth - ScaleX(17);
  Note.WordWrap := True;
  Note.Caption := KeepNote;
  Note.AdjustHeight;
  Top := Top + Note.Height + ScaleY(6);

  Move := TNewRadioButton.Create(LocationsPage);
  Move.Parent := Panel;
  Move.Left := 0;
  Move.Top := Top - GroupTop;
  Move.Width := Panel.Width;
  Move.Caption := MoveCaption;
  Move.OnClick := @LocationsChoiceClicked;
  Top := Top + Move.Height + ScaleY(2);

  Panel.Height := Top - GroupTop;

  Browse := TNewButton.Create(LocationsPage);
  Browse.Parent := LocationsPage.Surface;
  Browse.Width := ScaleX(75);
  Browse.Height := ScaleY(23);
  Browse.Left := LocationsPage.SurfaceWidth - Browse.Width;
  Browse.Top := Top;
  Browse.Caption := SetupMessage(msgButtonBrowse);

  Edit := TNewEdit.Create(LocationsPage);
  Edit.Parent := LocationsPage.Surface;
  Edit.Left := ScaleX(17);
  Edit.Top := Top + ScaleY(1);
  Edit.Width := Browse.Left - ScaleX(17) - ScaleX(8);

  Result := Top + Browse.Height + ScaleY(14);
end;

procedure InitializeWizard;
var
  Top: Integer;
  ProfileNote: TNewStaticText;
begin
  LocationsPage := CreateCustomPage(wpSelectDir,
    CustomMessage('LocationsTitle'), CustomMessage('LocationsSubtitle'));

  Top := AddLocationGroup(0,
    CustomMessage('LocationsProfileHeading'),
    CustomMessage('LocationsProfileKeep'),
    FmtMessage(CustomMessage('LocationsProfileNote'), [ExpandConstant('{userappdata}')
      + '\ai-softphone', DefaultProfileDir]),
    CustomMessage('LocationsProfileMove'),
    ProfilePanel, ProfileStandard, ProfileElsewhere, ProfileEdit, ProfileBrowse,
    ProfileNote);
  ProfileBrowse.OnClick := @BrowseProfileClicked;

  Top := AddLocationGroup(Top,
    CustomMessage('LocationsTalksHeading'),
    CustomMessage('LocationsTalksKeep'),
    DefaultRecordingsDir,
    CustomMessage('LocationsTalksMove'),
    TalksPanel, TalksStandard, TalksElsewhere, TalksEdit, TalksBrowse,
    TalksFollowNote);
  TalksBrowse.OnClick := @BrowseTalksClicked;

  ProfileEdit.OnChange := @ProfileWhereaboutsChanged;

  LocationsFootnote := TNewStaticText.Create(LocationsPage);
  LocationsFootnote.Parent := LocationsPage.Surface;
  LocationsFootnote.Left := 0;
  LocationsFootnote.Top := Top;
  LocationsFootnote.Width := LocationsPage.SurfaceWidth;
  LocationsFootnote.WordWrap := True;
  LocationsFootnote.Caption := CustomMessage('LocationsFootnote');
  LocationsFootnote.AdjustHeight;
end;

procedure CurPageChanged(CurPageID: Integer);
var
  Chosen: String;
begin
  if CurPageID <> LocationsPage.ID then
    Exit;
  if LocationsOffered then
    Exit;

  LocationsOffered := True;

  Chosen := StoredValue('profile', '');
  ProfileElsewhere.Checked := Chosen <> '';
  ProfileStandard.Checked := Chosen = '';
  if Chosen = '' then
    ProfileEdit.Text := DefaultProfileDir
  else
    ProfileEdit.Text := Chosen;

  Chosen := StoredValue('recordings', '');
  TalksElsewhere.Checked := Chosen <> '';
  TalksStandard.Checked := Chosen = '';
  if Chosen <> '' then
    TalksEdit.Text := Chosen;

  LocationsChoiceClicked(nil);
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
  Result := False;
end;

function DirectoryWillTake(const Wanted: String): Boolean;
begin
  Result := True;
  if Trim(Wanted) = '' then
  begin
    MsgBox(CustomMessage('LocationsNeedsADirectory'), mbError, MB_OK);
    Result := False;
    Exit;
  end;
  if not ForceDirectories(Trim(Wanted)) then
  begin
    MsgBox(FmtMessage(CustomMessage('LocationsCannotWrite'), [Trim(Wanted)]), mbError, MB_OK);
    Result := False;
  end;
end;

function NextButtonClick(CurPageID: Integer): Boolean;
begin
  Result := True;
  if CurPageID <> LocationsPage.ID then
    Exit;

  if ProfileElsewhere.Checked then
    Result := DirectoryWillTake(ProfileEdit.Text);
  if Result and TalksElsewhere.Checked then
    Result := DirectoryWillTake(TalksEdit.Text);
end;

procedure WriteLocations;
var
  Profile, Recordings: String;
  Document: TArrayOfString;
begin
  Profile := '';
  Recordings := '';
  if ProfileElsewhere.Checked then
    Profile := Trim(ProfileEdit.Text);
  if TalksElsewhere.Checked then
    Recordings := Trim(TalksEdit.Text);

  if (Profile = '') and (Recordings = '') then
  begin
    if FileExists(LocationsDocument) then
      DeleteFile(LocationsDocument);
    Exit;
  end;

  SetArrayLength(Document, 4);
  Document[0] := '{';
  Document[1] := '  "profile": "' + AsJsonText(Profile) + '",';
  Document[2] := '  "recordings": "' + AsJsonText(Recordings) + '"';
  Document[3] := '}';

  SaveStringsToUTF8File(LocationsDocument, Document, False);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep <> ssPostInstall then
    Exit;

  if WizardSilent then
    Exit;

  WriteLocations;
end;

function WantsRelaunch: Boolean;
begin
  Result := ExpandConstant('{param:RELAUNCH|0}') = '1';
end;

function UnderstandsQuit(const Binary: String): Boolean;
var
  Installed, Oldest: Int64;
begin
  Result := GetPackedVersion(Binary, Installed) and
            StrToVersion('0.1.2', Oldest) and
            (ComparePackedVersion(Installed, Oldest) >= 0);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  Installed: String;
  Outcome: Integer;
begin
  Result := '';

  Installed := ExpandConstant('{app}\{#AppExe}');

  FoundAlreadyInstalled := FileExists(Installed);

  if not FoundAlreadyInstalled then
    Exit;

  if not UnderstandsQuit(Installed) then
    Exit;

  if not Exec(Installed, '--quit', '', SW_HIDE, ewWaitUntilTerminated, Outcome) then
    Exit;
end;
