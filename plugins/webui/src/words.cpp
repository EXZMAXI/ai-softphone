// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#include <softphone/plugins/webui/words.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace softphone::plugins::webui {
namespace {

constexpr Words kEnglish{
    .recordings = "Recordings",
    .when = "When",
    .who = "Who",
    .length = "Length",
    .number = "Number",
    .parties = "Parties",
    .format = "Format",
    .transcript = "Transcript",
    .unknownParty = "unknown",
    .nothingRecorded = "Nothing has been recorded yet.",
    .notTranscribed = "This conversation has not been transcribed.",
    .notHere = "Not here",
    .noSuchRecording = "There is no recording with that identifier.",
    .notAvailable = "Not available",
    .noLibrary = "There is no recordings library in this phone.",
    .couldNotRead = "The recordings could not be read.",
    .soundGone = "The sound of this recording is no longer on disk.",
    .back = "Recordings",
    .date = Words::Date::YearFirst,
    .tag = "en",
};

constexpr Words kRussian{
    .recordings = "Записи",
    .when = "Когда",
    .who = "Кто",
    .length = "Длительность",
    .number = "Номер",
    .parties = "Участников",
    .format = "Формат",
    .transcript = "Стенограмма",
    .unknownParty = "неизвестно",
    .nothingRecorded = "Пока ничего не записано.",
    .notTranscribed = "Этот разговор ещё не расшифрован.",
    .notHere = "Здесь ничего нет",
    .noSuchRecording = "Записи с таким идентификатором нет.",
    .notAvailable = "Недоступно",
    .noLibrary = "В этом телефоне нет библиотеки записей.",
    .couldNotRead = "Не удалось прочитать записи.",
    .soundGone = "Звук этой записи больше не хранится на диске.",
    .back = "Записи",

    .date = Words::Date::DayFirst,
    .tag = "ru",
};

constexpr Words kGerman{
    .recordings = "Aufnahmen",
    .when = "Wann",
    .who = "Wer",
    .length = "Dauer",
    .number = "Nummer",
    .parties = "Teilnehmer",
    .format = "Format",
    .transcript = "Transkript",
    .unknownParty = "unbekannt",
    .nothingRecorded = "Es wurde noch nichts aufgenommen.",
    .notTranscribed = "Dieses Gespräch wurde nicht transkribiert.",
    .notHere = "Nicht hier",
    .noSuchRecording = "Es gibt keine Aufnahme mit dieser Kennung.",
    .notAvailable = "Nicht verfügbar",
    .noLibrary = "In diesem Telefon gibt es keine Aufnahmenbibliothek.",
    .couldNotRead = "Die Aufnahmen konnten nicht gelesen werden.",
    .soundGone = "Der Ton dieser Aufnahme liegt nicht mehr auf der Festplatte.",
    .back = "Aufnahmen",
    .date = Words::Date::DayFirst,
    .tag = "de",
};

constexpr Words kFrench{
    .recordings = "Enregistrements",
    .when = "Quand",
    .who = "Qui",
    .length = "Durée",
    .number = "Numéro",
    .parties = "Participants",
    .format = "Format",
    .transcript = "Transcription",
    .unknownParty = "inconnu",
    .nothingRecorded = "Rien n'a encore été enregistré.",
    .notTranscribed = "Cette conversation n'a pas été transcrite.",
    .notHere = "Rien ici",
    .noSuchRecording = "Il n'y a pas d'enregistrement avec cet identifiant.",
    .notAvailable = "Indisponible",
    .noLibrary = "Il n'y a pas de bibliothèque d'enregistrements dans ce téléphone.",
    .couldNotRead = "Les enregistrements n'ont pas pu être lus.",
    .soundGone = "Le son de cet enregistrement n'est plus sur le disque.",
    .back = "Enregistrements",
    .date = Words::Date::DayFirst,
    .tag = "fr",
};

constexpr Words kSpanish{
    .recordings = "Grabaciones",
    .when = "Cuándo",
    .who = "Quién",
    .length = "Duración",
    .number = "Número",
    .parties = "Participantes",
    .format = "Formato",
    .transcript = "Transcripción",
    .unknownParty = "desconocido",
    .nothingRecorded = "Todavía no se ha grabado nada.",
    .notTranscribed = "Esta conversación no se ha transcrito.",
    .notHere = "Aquí no hay nada",
    .noSuchRecording = "No hay ninguna grabación con ese identificador.",
    .notAvailable = "No disponible",
    .noLibrary = "En este teléfono no hay biblioteca de grabaciones.",
    .couldNotRead = "Las grabaciones no se han podido leer.",
    .soundGone = "El sonido de esta grabación ya no está en el disco.",
    .back = "Grabaciones",
    .date = Words::Date::DayFirst,
    .tag = "es",
};

constexpr Words kItalian{
    .recordings = "Registrazioni",
    .when = "Quando",
    .who = "Chi",
    .length = "Durata",
    .number = "Numero",
    .parties = "Partecipanti",
    .format = "Formato",
    .transcript = "Trascrizione",
    .unknownParty = "sconosciuto",
    .nothingRecorded = "Non è stato ancora registrato nulla.",
    .notTranscribed = "Questa conversazione non è stata trascritta.",
    .notHere = "Qui non c'è nulla",
    .noSuchRecording = "Non c'è alcuna registrazione con quell'identificatore.",
    .notAvailable = "Non disponibile",
    .noLibrary = "In questo telefono non c'è una libreria di registrazioni.",
    .couldNotRead = "Le registrazioni non si sono potute leggere.",
    .soundGone = "Il suono di questa registrazione non è più sul disco.",
    .back = "Registrazioni",
    .date = Words::Date::DayFirst,
    .tag = "it",
};

constexpr Words kPortuguese{
    .recordings = "Gravações",
    .when = "Quando",
    .who = "Quem",
    .length = "Duração",
    .number = "Número",
    .parties = "Participantes",
    .format = "Formato",
    .transcript = "Transcrição",
    .unknownParty = "desconhecido",
    .nothingRecorded = "Ainda não foi gravado nada.",
    .notTranscribed = "Esta conversa não foi transcrita.",
    .notHere = "Aqui não há nada",
    .noSuchRecording = "Não há nenhuma gravação com esse identificador.",
    .notAvailable = "Indisponível",
    .noLibrary = "Neste telefone não há biblioteca de gravações.",
    .couldNotRead = "As gravações não puderam ser lidas.",
    .soundGone = "O som desta gravação já não está no disco.",
    .back = "Gravações",
    .date = Words::Date::DayFirst,
    .tag = "pt",
};

constexpr Words kDutch{
    .recordings = "Opnames",
    .when = "Wanneer",
    .who = "Wie",
    .length = "Duur",
    .number = "Nummer",
    .parties = "Deelnemers",
    .format = "Formaat",
    .transcript = "Transcriptie",
    .unknownParty = "onbekend",
    .nothingRecorded = "Er is nog niets opgenomen.",
    .notTranscribed = "Dit gesprek is niet getranscribeerd.",
    .notHere = "Hier is niets",
    .noSuchRecording = "Er is geen opname met die identificatie.",
    .notAvailable = "Niet beschikbaar",
    .noLibrary = "In deze telefoon is geen opnamebibliotheek.",
    .couldNotRead = "De opnames konden niet worden gelezen.",
    .soundGone = "Het geluid van deze opname staat niet meer op de schijf.",
    .back = "Opnames",
    .date = Words::Date::DayFirst,
    .tag = "nl",
};

constexpr Words kSwedish{
    .recordings = "Inspelningar",
    .when = "När",
    .who = "Vem",
    .length = "Längd",
    .number = "Nummer",
    .parties = "Deltagare",
    .format = "Format",
    .transcript = "Transkription",
    .unknownParty = "okänd",
    .nothingRecorded = "Ingenting är inspelat ännu.",
    .notTranscribed = "Det här samtalet är inte transkriberat.",
    .notHere = "Ingenting här",
    .noSuchRecording = "Det finns ingen inspelning med den identifieraren.",
    .notAvailable = "Inte tillgänglig",
    .noLibrary = "Det finns inget inspelningsbibliotek i den här telefonen.",
    .couldNotRead = "Inspelningarna gick inte att läsa.",
    .soundGone = "Ljudet från den här inspelningen finns inte längre på disken.",
    .back = "Inspelningar",
    .date = Words::Date::YearFirst,
    .tag = "sv",
};

constexpr Words kDanish{
    .recordings = "Optagelser",
    .when = "Hvornår",
    .who = "Hvem",
    .length = "Længde",
    .number = "Nummer",
    .parties = "Deltagere",
    .format = "Format",
    .transcript = "Transskription",
    .unknownParty = "ukendt",
    .nothingRecorded = "Der er endnu ikke optaget noget.",
    .notTranscribed = "Denne samtale er ikke transskriberet.",
    .notHere = "Intet her",
    .noSuchRecording = "Der er ingen optagelse med den identifikator.",
    .notAvailable = "Ikke tilgængelig",
    .noLibrary = "Der er intet optagelsesbibliotek i denne telefon.",
    .couldNotRead = "Optagelserne kunne ikke læses.",
    .soundGone = "Lyden fra denne optagelse ligger ikke længere på disken.",
    .back = "Optagelser",
    .date = Words::Date::DayFirst,
    .tag = "da",
};

constexpr Words kNorwegian{
    .recordings = "Opptak",
    .when = "Når",
    .who = "Hvem",
    .length = "Lengde",
    .number = "Nummer",
    .parties = "Deltakere",
    .format = "Format",
    .transcript = "Transkripsjon",
    .unknownParty = "ukjent",
    .nothingRecorded = "Ingenting er tatt opp ennå.",
    .notTranscribed = "Denne samtalen er ikke transkribert.",
    .notHere = "Ingenting her",
    .noSuchRecording = "Det finnes ikke noe opptak med den identifikatoren.",
    .notAvailable = "Ikke tilgjengelig",
    .noLibrary = "Det finnes ikke noe opptaksbibliotek i denne telefonen.",
    .couldNotRead = "Opptakene kunne ikke leses.",
    .soundGone = "Lyden fra dette opptaket ligger ikke lenger på disken.",
    .back = "Opptak",
    .date = Words::Date::DayFirst,
    .tag = "nb",
};

constexpr Words kFinnish{
    .recordings = "Tallenteet",
    .when = "Milloin",
    .who = "Kuka",
    .length = "Kesto",
    .number = "Numero",
    .parties = "Osallistujat",
    .format = "Muoto",
    .transcript = "Litterointi",
    .unknownParty = "tuntematon",
    .nothingRecorded = "Mitään ei ole vielä tallennettu.",
    .notTranscribed = "Tätä keskustelua ei ole litteroitu.",
    .notHere = "Täällä ei ole mitään",
    .noSuchRecording = "Tuolla tunnisteella ei ole tallennetta.",
    .notAvailable = "Ei saatavilla",
    .noLibrary = "Tässä puhelimessa ei ole tallennekirjastoa.",
    .couldNotRead = "Tallenteita ei saatu luettua.",
    .soundGone = "Tämän tallenteen ääni ei ole enää levyllä.",
    .back = "Tallenteet",
    .date = Words::Date::DayFirst,
    .tag = "fi",
};

constexpr Words kPolish{
    .recordings = "Nagrania",
    .when = "Kiedy",
    .who = "Kto",
    .length = "Czas",
    .number = "Numer",
    .parties = "Uczestnicy",
    .format = "Format",
    .transcript = "Zapis",
    .unknownParty = "nieznany",
    .nothingRecorded = "Nic jeszcze nie nagrano.",
    .notTranscribed = "Ta rozmowa nie została zapisana.",
    .notHere = "Nic tu nie ma",
    .noSuchRecording = "Nie ma nagrania o tym identyfikatorze.",
    .notAvailable = "Niedostępne",
    .noLibrary = "Ten telefon nie ma biblioteki nagrań.",
    .couldNotRead = "Nie udało się odczytać nagrań.",
    .soundGone = "Dźwięku tego nagrania już nie ma na dysku.",
    .back = "Nagrania",
    .date = Words::Date::DayFirst,
    .tag = "pl",
};

constexpr Words kCzech{
    .recordings = "Nahrávky",
    .when = "Kdy",
    .who = "Kdo",
    .length = "Délka",
    .number = "Číslo",
    .parties = "Účastníci",
    .format = "Formát",
    .transcript = "Přepis",
    .unknownParty = "neznámý",
    .nothingRecorded = "Zatím není nic nahráno.",
    .notTranscribed = "Tento hovor není přepsán.",
    .notHere = "Tady nic není",
    .noSuchRecording = "Nahrávka s tímto identifikátorem neexistuje.",
    .notAvailable = "Nedostupné",
    .noLibrary = "Tento telefon nemá knihovnu nahrávek.",
    .couldNotRead = "Nahrávky se nepodařilo přečíst.",
    .soundGone = "Zvuk této nahrávky už na disku není.",
    .back = "Nahrávky",
    .date = Words::Date::DayFirst,
    .tag = "cs",
};

constexpr Words kSlovak{
    .recordings = "Nahrávky",
    .when = "Kedy",
    .who = "Kto",
    .length = "Dĺžka",
    .number = "Číslo",
    .parties = "Účastníci",
    .format = "Formát",
    .transcript = "Prepis",
    .unknownParty = "neznámy",
    .nothingRecorded = "Zatiaľ nie je nič nahraté.",
    .notTranscribed = "Tento hovor nie je prepísaný.",
    .notHere = "Tu nič nie je",
    .noSuchRecording = "Nahrávka s týmto identifikátorom neexistuje.",
    .notAvailable = "Nedostupné",
    .noLibrary = "Tento telefón nemá knižnicu nahrávok.",
    .couldNotRead = "Nahrávky sa nepodarilo prečítať.",
    .soundGone = "Zvuk tejto nahrávky už na disku nie je.",
    .back = "Nahrávky",
    .date = Words::Date::DayFirst,
    .tag = "sk",
};

constexpr Words kHungarian{
    .recordings = "Felvételek",
    .when = "Mikor",
    .who = "Ki",
    .length = "Hossz",
    .number = "Szám",
    .parties = "Résztvevők",
    .format = "Formátum",
    .transcript = "Leirat",
    .unknownParty = "ismeretlen",
    .nothingRecorded = "Még nincs semmi felvéve.",
    .notTranscribed = "Ez a beszélgetés nincs leírva.",
    .notHere = "Itt nincs semmi",
    .noSuchRecording = "Nincs felvétel ezzel az azonosítóval.",
    .notAvailable = "Nem érhető el",
    .noLibrary = "Ezen a telefonon nincs felvételtár.",
    .couldNotRead = "A felvételeket nem sikerült beolvasni.",
    .soundGone = "Ennek a felvételnek a hangja már nincs a lemezen.",
    .back = "Felvételek",
    .date = Words::Date::YearFirst,
    .tag = "hu",
};

constexpr Words kRomanian{
    .recordings = "Înregistrări",
    .when = "Când",
    .who = "Cine",
    .length = "Durată",
    .number = "Număr",
    .parties = "Participanți",
    .format = "Format",
    .transcript = "Transcriere",
    .unknownParty = "necunoscut",
    .nothingRecorded = "Nu s-a înregistrat încă nimic.",
    .notTranscribed = "Această convorbire nu este transcrisă.",
    .notHere = "Aici nu este nimic",
    .noSuchRecording = "Nu există nicio înregistrare cu acest identificator.",
    .notAvailable = "Indisponibil",
    .noLibrary = "Acest telefon nu are o bibliotecă de înregistrări.",
    .couldNotRead = "Înregistrările nu au putut fi citite.",
    .soundGone = "Sunetul acestei înregistrări nu mai este pe disc.",
    .back = "Înregistrări",
    .date = Words::Date::DayFirst,
    .tag = "ro",
};

constexpr Words kBulgarian{
    .recordings = "Записи",
    .when = "Кога",
    .who = "Кой",
    .length = "Времетраене",
    .number = "Номер",
    .parties = "Участници",
    .format = "Формат",
    .transcript = "Разшифровка",
    .unknownParty = "неизвестен",
    .nothingRecorded = "Още нищо не е записано.",
    .notTranscribed = "Този разговор не е разшифрован.",
    .notHere = "Тук няма нищо",
    .noSuchRecording = "Няма запис с този идентификатор.",
    .notAvailable = "Не е налично",
    .noLibrary = "Този телефон няма библиотека със записи.",
    .couldNotRead = "Записите не можаха да бъдат прочетени.",
    .soundGone = "Звукът на този запис вече не е на диска.",
    .back = "Записи",
    .date = Words::Date::DayFirst,
    .tag = "bg",
};

constexpr Words kCroatian{
    .recordings = "Snimke",
    .when = "Kada",
    .who = "Tko",
    .length = "Trajanje",
    .number = "Broj",
    .parties = "Sudionici",
    .format = "Oblik",
    .transcript = "Prijepis",
    .unknownParty = "nepoznat",
    .nothingRecorded = "Još ništa nije snimljeno.",
    .notTranscribed = "Ovaj razgovor nije prepisan.",
    .notHere = "Ovdje nema ničega",
    .noSuchRecording = "Nema snimke s tom oznakom.",
    .notAvailable = "Nedostupno",
    .noLibrary = "Ovaj telefon nema knjižnicu snimaka.",
    .couldNotRead = "Snimke nije bilo moguće pročitati.",
    .soundGone = "Zvuk ove snimke više nije na disku.",
    .back = "Snimke",
    .date = Words::Date::DayFirst,
    .tag = "hr",
};

constexpr Words kSlovenian{
    .recordings = "Posnetki",
    .when = "Kdaj",
    .who = "Kdo",
    .length = "Trajanje",
    .number = "Številka",
    .parties = "Udeleženci",
    .format = "Oblika",
    .transcript = "Prepis",
    .unknownParty = "neznan",
    .nothingRecorded = "Še nič ni posneto.",
    .notTranscribed = "Ta pogovor ni prepisan.",
    .notHere = "Tu ni ničesar",
    .noSuchRecording = "Posnetka s to oznako ni.",
    .notAvailable = "Ni na voljo",
    .noLibrary = "Ta telefon nima knjižnice posnetkov.",
    .couldNotRead = "Posnetkov ni bilo mogoče prebrati.",
    .soundGone = "Zvoka tega posnetka ni več na disku.",
    .back = "Posnetki",
    .date = Words::Date::DayFirst,
    .tag = "sl",
};

constexpr Words kEstonian{
    .recordings = "Salvestised",
    .when = "Millal",
    .who = "Kes",
    .length = "Kestus",
    .number = "Number",
    .parties = "Osalejad",
    .format = "Vorming",
    .transcript = "Üleskirjutus",
    .unknownParty = "tundmatu",
    .nothingRecorded = "Midagi ei ole veel salvestatud.",
    .notTranscribed = "See vestlus ei ole üles kirjutatud.",
    .notHere = "Siin ei ole midagi",
    .noSuchRecording = "Selle tunnusega salvestist ei ole.",
    .notAvailable = "Ei ole saadaval",
    .noLibrary = "Sellel telefonil ei ole salvestiste kogu.",
    .couldNotRead = "Salvestisi ei õnnestunud lugeda.",
    .soundGone = "Selle salvestise heli ei ole enam kettal.",
    .back = "Salvestised",
    .date = Words::Date::DayFirst,
    .tag = "et",
};

constexpr Words kLatvian{
    .recordings = "Ieraksti",
    .when = "Kad",
    .who = "Kurš",
    .length = "Ilgums",
    .number = "Numurs",
    .parties = "Dalībnieki",
    .format = "Formāts",
    .transcript = "Atšifrējums",
    .unknownParty = "nezināms",
    .nothingRecorded = "Vēl nekas nav ierakstīts.",
    .notTranscribed = "Šī saruna nav atšifrēta.",
    .notHere = "Šeit nekā nav",
    .noSuchRecording = "Nav ieraksta ar šo identifikatoru.",
    .notAvailable = "Nav pieejams",
    .noLibrary = "Šim tālrunim nav ierakstu krātuves.",
    .couldNotRead = "Ierakstus neizdevās nolasīt.",
    .soundGone = "Šī ieraksta skaņas diskā vairs nav.",
    .back = "Ieraksti",
    .date = Words::Date::DayFirst,
    .tag = "lv",
};

constexpr Words kLithuanian{
    .recordings = "Įrašai",
    .when = "Kada",
    .who = "Kas",
    .length = "Trukmė",
    .number = "Numeris",
    .parties = "Dalyviai",
    .format = "Formatas",
    .transcript = "Užrašas",
    .unknownParty = "nežinomas",
    .nothingRecorded = "Kol kas nieko neįrašyta.",
    .notTranscribed = "Šis pokalbis neužrašytas.",
    .notHere = "Čia nieko nėra",
    .noSuchRecording = "Įrašo su tokiu atpažinimo ženklu nėra.",
    .notAvailable = "Neprieinama",
    .noLibrary = "Šis telefonas neturi įrašų bibliotekos.",
    .couldNotRead = "Įrašų nepavyko perskaityti.",
    .soundGone = "Šio įrašo garso diske nebėra.",
    .back = "Įrašai",
    .date = Words::Date::YearFirst,
    .tag = "lt",
};

constexpr Words kGreek{
    .recordings = "Εγγραφές",
    .when = "Πότε",
    .who = "Ποιος",
    .length = "Διάρκεια",
    .number = "Αριθμός",
    .parties = "Συμμετέχοντες",
    .format = "Μορφή",
    .transcript = "Απομαγνητοφώνηση",
    .unknownParty = "άγνωστος",
    .nothingRecorded = "Τίποτα δεν έχει ηχογραφηθεί ακόμη.",
    .notTranscribed = "Αυτή η συνομιλία δεν έχει απομαγνητοφωνηθεί.",
    .notHere = "Εδώ δεν υπάρχει τίποτα",
    .noSuchRecording = "Δεν υπάρχει εγγραφή με αυτό το αναγνωριστικό.",
    .notAvailable = "Μη διαθέσιμο",
    .noLibrary = "Αυτό το τηλέφωνο δεν έχει βιβλιοθήκη εγγραφών.",
    .couldNotRead = "Οι εγγραφές δεν διαβάστηκαν.",
    .soundGone = "Ο ήχος αυτής της εγγραφής δεν είναι πια στον δίσκο.",
    .back = "Εγγραφές",
    .date = Words::Date::DayFirst,
    .tag = "el",
};

constexpr Words kTurkish{
    .recordings = "Kayıtlar",
    .when = "Ne zaman",
    .who = "Kim",
    .length = "Süre",
    .number = "Numara",
    .parties = "Katılanlar",
    .format = "Biçim",
    .transcript = "Döküm",
    .unknownParty = "bilinmiyor",
    .nothingRecorded = "Henüz hiçbir şey kaydedilmedi.",
    .notTranscribed = "Bu görüşme yazıya dökülmedi.",
    .notHere = "Burada bir şey yok",
    .noSuchRecording = "Bu tanımlayıcıyla bir kayıt yok.",
    .notAvailable = "Kullanılamıyor",
    .noLibrary = "Bu telefonda kayıt kitaplığı yok.",
    .couldNotRead = "Kayıtlar okunamadı.",
    .soundGone = "Bu kaydın sesi artık diskte değil.",
    .back = "Kayıtlar",
    .date = Words::Date::DayFirst,
    .tag = "tr",
};

constexpr Words kUkrainian{
    .recordings = "Записи",
    .when = "Коли",
    .who = "Хто",
    .length = "Тривалість",
    .number = "Номер",
    .parties = "Учасники",
    .format = "Формат",
    .transcript = "Розшифровка",
    .unknownParty = "невідомий",
    .nothingRecorded = "Ще нічого не записано.",
    .notTranscribed = "Цю розмову не розшифровано.",
    .notHere = "Тут нічого немає",
    .noSuchRecording = "Запису з таким ідентифікатором немає.",
    .notAvailable = "Недоступно",
    .noLibrary = "Цей телефон не має бібліотеки записів.",
    .couldNotRead = "Записи не вдалося прочитати.",
    .soundGone = "Звуку цього запису вже немає на диску.",
    .back = "Записи",
    .date = Words::Date::DayFirst,
    .tag = "uk",
};

constexpr Words kIndonesian{
    .recordings = "Rekaman",
    .when = "Kapan",
    .who = "Siapa",
    .length = "Durasi",
    .number = "Nomor",
    .parties = "Peserta",
    .format = "Format",
    .transcript = "Transkrip",
    .unknownParty = "tidak diketahui",
    .nothingRecorded = "Belum ada yang direkam.",
    .notTranscribed = "Percakapan ini belum ditranskripsikan.",
    .notHere = "Tidak ada apa-apa di sini",
    .noSuchRecording = "Tidak ada rekaman dengan pengenal itu.",
    .notAvailable = "Tidak tersedia",
    .noLibrary = "Telepon ini tidak punya pustaka rekaman.",
    .couldNotRead = "Rekaman tidak dapat dibaca.",
    .soundGone = "Suara rekaman ini sudah tidak ada di disk.",
    .back = "Rekaman",
    .date = Words::Date::DayFirst,
    .tag = "id",
};

constexpr Words kVietnamese{
    .recordings = "Bản ghi",
    .when = "Khi nào",
    .who = "Ai",
    .length = "Độ dài",
    .number = "Số máy",
    .parties = "Người tham gia",
    .format = "Định dạng",
    .transcript = "Bản gỡ băng",
    .unknownParty = "không rõ",
    .nothingRecorded = "Chưa có gì được ghi.",
    .notTranscribed = "Cuộc trò chuyện này chưa được gỡ băng.",
    .notHere = "Ở đây không có gì",
    .noSuchRecording = "Không có bản ghi nào mang định danh đó.",
    .notAvailable = "Không có",
    .noLibrary = "Điện thoại này không có thư viện bản ghi.",
    .couldNotRead = "Không đọc được các bản ghi.",
    .soundGone = "Âm thanh của bản ghi này không còn trên đĩa.",
    .back = "Bản ghi",
    .date = Words::Date::DayFirst,
    .tag = "vi",
};

constexpr Words kSerbian{
    .recordings = "Снимци",
    .when = "Када",
    .who = "Ко",
    .length = "Трајање",
    .number = "Број",
    .parties = "Учесници",
    .format = "Формат",
    .transcript = "Препис",
    .unknownParty = "непознат",
    .nothingRecorded = "Још ништа није снимљено.",
    .notTranscribed = "Овај разговор није преписан.",
    .notHere = "Овде нема ничега",
    .noSuchRecording = "Нема снимка с том ознаком.",
    .notAvailable = "Није доступно",
    .noLibrary = "Овај телефон нема библиотеку снимака.",
    .couldNotRead = "Снимке није било могуће прочитати.",
    .soundGone = "Звука овог снимка више нема на диску.",
    .back = "Снимци",
    .date = Words::Date::DayFirst,
    .tag = "sr",
};

constexpr Words kSerbianLatin{
    .recordings = "Snimci",
    .when = "Kada",
    .who = "Ko",
    .length = "Trajanje",
    .number = "Broj",
    .parties = "Učesnici",
    .format = "Format",
    .transcript = "Prepis",
    .unknownParty = "nepoznat",
    .nothingRecorded = "Još ništa nije snimljeno.",
    .notTranscribed = "Ovaj razgovor nije prepisan.",
    .notHere = "Ovde nema ničega",
    .noSuchRecording = "Nema snimka s tom oznakom.",
    .notAvailable = "Nije dostupno",
    .noLibrary = "Ovaj telefon nema biblioteku snimaka.",
    .couldNotRead = "Snimke nije bilo moguće pročitati.",
    .soundGone = "Zvuka ovog snimka više nema na disku.",
    .back = "Snimci",
    .date = Words::Date::DayFirst,
    .tag = "sr-Latn",
};

constexpr std::array kLanguages{kEnglish,   kRussian,    kGerman,     kFrench,   kSpanish,
                                kItalian,   kPortuguese, kDutch,      kSwedish,  kDanish,
                                kNorwegian, kFinnish,    kPolish,     kCzech,    kSlovak,
                                kHungarian, kRomanian,   kBulgarian,  kCroatian, kSlovenian,
                                kEstonian,  kLatvian,    kLithuanian, kGreek,    kTurkish,
                                kUkrainian, kIndonesian, kVietnamese, kSerbian,  kSerbianLatin};

[[nodiscard]] std::string normalisedTag(std::string_view header)
{
    const auto comma = header.find(',');
    auto first = header.substr(0, comma == std::string_view::npos ? header.size() : comma);

    if (const auto quality = first.find(';'); quality != std::string_view::npos) {
        first = first.substr(0, quality);
    }

    while (!first.empty() && std::isspace(static_cast<unsigned char>(first.front())) != 0) {
        first.remove_prefix(1);
    }
    while (!first.empty() && std::isspace(static_cast<unsigned char>(first.back())) != 0) {
        first.remove_suffix(1);
    }

    std::string tag;
    tag.reserve(first.size());
    for (const char c : first) {
        tag.push_back(c == '_' ? '-'
                               : static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return tag;
}

[[nodiscard]] bool answersTo(const Words &language, std::string_view tag)
{
    if (language.tag.size() != tag.size()) {
        return false;
    }
    return std::ranges::equal(language.tag, tag, [](char a, char b) {
        return std::tolower(static_cast<unsigned char>(a)) ==
               std::tolower(static_cast<unsigned char>(b));
    });
}

}

const Words *wordsIn(std::string_view tag)
{
    const auto wanted = normalisedTag(tag);
    if (wanted.empty()) {
        return nullptr;
    }

    for (auto candidate = std::string_view{wanted}; !candidate.empty();) {
        const auto found = std::ranges::find_if(kLanguages, [candidate](const Words &language) {
            return answersTo(language, candidate);
        });
        if (found != kLanguages.end()) {
            return &*found;
        }
        const auto cut = candidate.rfind('-');
        if (cut == std::string_view::npos) {
            break;
        }
        candidate = candidate.substr(0, cut);
    }
    return nullptr;
}

const Words &wordsFor(std::string_view header, const Words *phone)
{
    if (const auto *asked = wordsIn(header); asked != nullptr) {
        return *asked;
    }

    if (phone != nullptr) {
        return *phone;
    }
    return kEnglish;
}

std::span<const Words> everyLanguage()
{
    return kLanguages;
}

}
