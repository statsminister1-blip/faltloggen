// SPDX-License-Identifier: CC-BY-SA-4.0
// Skapad av tools/manual/md2c.py ur manual/MANUAL.md och manual/MANUAL_EN.md – ändra inte här.
// Poster: P stycke, B punkt, b underpunkt, N numrerad rad, G luft; **fet**.
#pragma once
#include "lang.h"

struct ManKap { const char *titel; const char *text; };
static const ManKap MANUAL_SV[] = {
  {"1. Vad F\303\244ltloggen \303\244r",
    "PF\303\244ltloggen \303\244r en extra sk\303\244rm i traktorhytten. Den kopplas till hyttens ISOBUS-uttag och visar vad traktorn och redskapet g\303\266r: hastighet, motor, lyft, kraftuttag, hydraulik, br\303\244nsle, slirning och var du har k\303\266rt.\n"
    "G\n"
    "PSk\303\244rmen **lyssnar bara**. Den skickar aldrig n\303\245got p\303\245 bussen och kan inte styra traktorn eller redskapet. Allt du trycker p\303\245 p\303\245verkar bara F\303\244ltloggen sj\303\244lv.\n"
    "G\n"
    "PF\303\244ltloggen \303\244r gratis och \303\266ppen k\303\244llkod. Den \303\244r gjord av en privatperson p\303\245 fritiden, helt utan inblandning fr\303\245n Fendt, AGCO eller n\303\245gon annan tillverkare. Allt anv\303\244nds p\303\245 eget ansvar \342\200\223 l\303\244s kapitel 15.\n"
    "G\n"
    "PDen \303\244r provad p\303\245 Fendt Vario S4 (720 och 828). Andra traktorer kan skicka andra saker; det som inte h\303\266rs visas som \"--\"."},
  {"2. Knapparna till v\303\244nster",
    "PTraktorsidan och redskapssidan har samma knapprad l\303\244ngst till v\303\244nster, uppifr\303\245n:\n"
    "G\n"
    "N1. **Sidans egna knappar** (tv\303\245 platser) \342\200\223 p\303\245 traktorsidan **Info\342\201\272 Trepunktslyft** och **Info\342\201\272 Hydraulventiler**, p\303\245 redskapssidan **TERMINAL**.\n"
    "N2. **M\303\245nen / solen** \342\200\223 v\303\244xlar mellan dag- och nattf\303\244rger. Valet sparas.\n"
    "N3. **Huset** \342\200\223 startvalet: v\303\244lj TRAKTOR, ISOBUS eller TERMINAL.\n"
    "N4. **Skiftnyckeln** \342\200\223 inst\303\244llningarna.\n"
    "G\n"
    "PVald knapp har r\303\266d ram.\n"
    "G\n"
    "PTv\303\245 saker \303\266ppnas genom att trycka direkt p\303\245 rutan:\n"
    "B**Kartrutan** \342\206\222 kartan i helsk\303\244rm.\n"
    "B**Br\303\244nslediagrammet** \342\206\222 br\303\244nsleanalysen."},
  {"3. Startvalet",
    "PN\303\244r sk\303\244rmen hittar ett ISOBUS-redskap fr\303\245gar den vad du vill se:\n"
    "G\n"
    "B**TRAKTOR** \342\200\223 traktorsidan.\n"
    "B**ISOBUS** \342\200\223 redskapssidan med redskapets egna v\303\244rden.\n"
    "B**TERMINAL** \342\200\223 redskapets egen bild, som p\303\245 traktorns terminal (finns n\303\244r bilden \303\244r f\303\245ngad).\n"
    "G\n"
    "PSvarar du inte v\303\244ljer sk\303\244rmen redskapet sj\303\244lv efter 20 sekunder. Hittas inget redskap g\303\245r den direkt till traktorsidan.\n"
    "G\n"
    "P\303\226ppnar du startvalet med **huset** v\303\244ljer den aldrig sj\303\244lv. Finns inget redskap \303\244r ISOBUS-knappen gr\303\245 med texten \"inget redskap hittat\"."},
  {"4. Traktorsidan",
    "PFyra rutor:\n"
    "G\n"
    "P**Uppe till v\303\244nster \342\200\223 Info\342\201\272** (v\303\244lj med knapparna till v\303\244nster):\n"
    "B**Trepunktslyft:** l\303\244ge i procent fram och bak, kraftuttagens varvtal, ARBETE n\303\244r lyften \303\244r i arbetsl\303\244ge.\n"
    "B**Hydraulventiler:** varje uttag som traktorn skickar, med fl\303\266det ut (+) eller in (-). Namnen och f\303\244rgerna (Gul, Bl\303\245, R\303\266d, Gr\303\266n, Brun, Front) f\303\266ljer Fendts ordning \342\200\223 st\303\244mmer de inte med din traktor, g\303\245 efter numret.\n"
    "G\n"
    "P**Uppe till h\303\266ger \342\200\223 Traktor:**\n"
    "BDen bl\303\245 listen: gr\303\266n prick = bussen h\303\266rs, WiFi, **timr\303\244knaren**, **lutning** fram/bak och i sidled (grader).\n"
    "BTraktorsymbolen: lyftarmarna r\303\266r sig med lyften (gr\303\266na i arbete), kugghjulen blir gr\303\266na n\303\244r kraftuttagen g\303\245r. Symbolen lutar med backen och visar t\303\244nda lampor (str\303\245lkastare, arbetsljus, blinkers, bromsljus, backljus). Varningsljuset p\303\245 hyttaket blir orange och \"roterar\" n\303\244r det \303\244r p\303\245.\n"
    "BRaderna: **hastighet** och **k\303\266rriktning** (FRAM gr\303\266n / BACK), **motorvarv** och **br\303\244nsle l/h**, **slirning** och **dragkraft**.\n"
    "G\n"
    "P**Nere till v\303\244nster \342\200\223 Karta:** var du \303\244r och vad som \303\244r k\303\266rt. Arealen i den vita rutan, gr\303\266n ring n\303\244r kartan loggar, l/ha nere till v\303\244nster. Tryck f\303\266r helsk\303\244rm.\n"
    "G\n"
    "P**Nere till h\303\266ger \342\200\223 Br\303\244nsle:** l/h och l/min nu, och ett stapeldiagram med ett medelv\303\244rde per minut f\303\266r de senaste 30 minuterna. Tryck f\303\266r br\303\244nsleanalysen."},
  {"5. Slirning och dragkraft",
    "P**Slirning (%)** r\303\244knas ut av F\303\244ltloggen: hur mycket fortare hjulen snurrar \303\244n traktorn faktiskt r\303\266r sig (hjulhastighet j\303\244mf\303\266rt med markhastighet). V\303\244rdet j\303\244mnas ut \303\266ver n\303\245gra sekunder och visas bara \303\266ver 1,5 km/h.\n"
    "BF\303\244ltet blir **gult fr\303\245n 20 %** och **r\303\266tt fr\303\245n 30 %**.\n"
    "BVid transport ligger det normalt p\303\245 n\303\245gra procent.\n"
    "G\n"
    "P**Dragkraft (kN)** kommer fr\303\245n kraftgivarna i baklyftens undre l\303\244nkarmar. Den syns d\303\244rf\303\266r **bara n\303\244r redskapet h\303\244nger i lyftarmarna** \342\200\223 ett redskap i dragkroken eller p\303\245 draget syns inte h\303\244r. Anv\303\244nd d\303\245 slirningen i st\303\244llet.\n"
    "BPilen v\303\244xer med kraften. F\303\244ltet blir **gult fr\303\245n 35 kN** och **r\303\266tt fr\303\245n 50 kN**.\n"
    "G\n"
    "PB\303\245da \303\244r uppskattningar fr\303\245n traktorns egna givare. Lita p\303\245 traktorns instrument om de skiljer sig."},
  {"6. Redskapssidan och TERMINAL",
    "P**Redskapssidan** visar v\303\244rdena som ett ISOBUS-redskap skickar till traktorns Task Controller, med redskapets egna namn och enheter s\303\245 som redskapet sj\303\244lv beskriver dem. \303\226verst st\303\245r hastighet, arbetsbredd och arbetsl\303\244ge. \303\204r listan l\303\245ng bl\303\244ddrar knappen till v\303\244nster.\n"
    "G\n"
    "P**TERMINAL** visar redskapets egen bild, samma som p\303\245 traktorns terminal, och uppdateras hela tiden. Det \303\244r **bara en bild** \342\200\223 knapparna g\303\245r inte att trycka p\303\245 h\303\244r. Anv\303\244nd traktorns terminal f\303\266r att styra redskapet.\n"
    "G\n"
    "PBilden f\303\245ngas f\303\266rsta g\303\245ngen redskapet laddar upp den till terminalen och sparas p\303\245 SD-kortet. Har terminalen redan bilden sparad skickas den inte \342\200\223 radera d\303\245 redskapets sparade bild i terminalen och starta om."},
  {"7. Kartan i helsk\303\244rm",
    "P**Knapparna till h\303\266ger:** tillbaka \302\267 **+** / **100 m** / **-** (zoom) \302\267 **START / STOPP** (loggning f\303\266r hand) \302\267 skiftnyckeln (kartans inst\303\244llningar) \302\267 **NOLLA** \302\267 **F\303\226LJ** (visas n\303\244r kartan \303\244r flyttad).\n"
    "G\n"
    "B**Dra med fingret** f\303\266r att flytta kartan. **F\303\226LJ** centrerar p\303\245 traktorn igen.\n"
    "BNorr \303\244r alltid upp\303\245t. Traktorn \303\244r den lilla mark\303\266ren, redskapet ritas 3 m bakom antennen.\n"
    "B**Areal-rutan:** bearbetad yta i hektar, br\303\244nsle i liter och l/ha sedan senaste NOLLA, och knappen **EXPORTERA RAPPORT** (kapitel 9).\n"
    "B**Skiftets areal:** tryck p\303\245 **SKIFTETS AREAL** och knappa in arealen i hektar med upp till tv\303\245 decimaler. Rutan visar sedan hur m\303\245nga hektar som \303\244r kvar, hur l\303\245ng tid det \303\244r kvar och vilken klockan det blir klart. Tiden r\303\244knas ut l\303\266pande ur de senaste ca 20 minuterna av loggning och visas efter ca 3 minuters arbete. Klockslaget kr\303\244ver att sk\303\244rmen har tid (internet eller GPS). NOLLA tar bort skiftet.\n"
    "B**Fliken med pilen** f\303\244ller in och ut rutorna s\303\245 att kartan syns b\303\244ttre.\n"
    "B**NOLLA** tar bort sp\303\245r, areal, br\303\244nsle och f\303\244rger (sk\303\244rmen fr\303\245gar f\303\266rst). G\303\266r det n\303\244r du b\303\266rjar p\303\245 ett nytt f\303\244lt.\n"
    "G\n"
    "P\303\226verlappande k\303\266rning r\303\244knas bara en g\303\245ng i arealen. Kartan beh\303\266ver en GPS-position fr\303\245n traktorns styrsystem p\303\245 ISOBUS."},
  {"8. F\303\244rg p\303\245 sp\303\245ret",
    "PRutan **F\303\244rg p\303\245 sp\303\245ret** under Areal v\303\244ljer vad sp\303\245ret f\303\244rgas efter:\n"
    "G\n"
    "B**BR\303\204NSLE** \342\200\223 liter per hektar.\n"
    "B**DRAG** \342\200\223 dragkraft i lyften (kN).\n"
    "B**SLIRNING** \342\200\223 slirning (%).\n"
    "G\n"
    "PF\303\244rgerna j\303\244mf\303\266rs med **k\303\266rningens eget snitt** sedan senaste NOLLA:\n"
    "B**bl\303\245tt** = som snittet\n"
    "B**gr\303\266nt** = 30 % l\303\244gre\n"
    "B**r\303\266tt** = 30 % h\303\266gre\n"
    "G\n"
    "PSiffrorna under f\303\244rgstegen visar vad helt gr\303\266nt, snittet och helt r\303\266tt \303\244r just nu. F\303\244rgerna sparas med sp\303\245ret och finns kvar efter omstart. Under 1,5 km/h m\303\245las ingen f\303\244rg."},
  {"9. F\303\244ltrapport till telefonen",
    "N1. Kartan i helsk\303\244rm \342\206\222 **EXPORTERA RAPPORT**.\n"
    "N2. Skriv f\303\244ltets namn, eller tryck **HOPPA \303\226VER** (d\303\245 blir namnet datum + sk\303\244rmens namn).\n"
    "N3. V\303\244nta n\303\245gra sekunder medan rapporten skapas.\n"
    "N4. Skanna **QR-kod 1** med telefonen \342\200\223 den ansluter till sk\303\244rmens eget WiFi (inget l\303\266senord).\n"
    "N5. Skanna **QR-kod 2** (eller \303\266ppna http://192.168.4.1) och tryck **LADDA NER**.\n"
    "N6. Tryck **ST\303\204NG** n\303\244r du \303\244r klar.\n"
    "G\n"
    "PRapporten \303\244r en PDF med tre kartor (br\303\244nsle, slirning, dragkraft) och areal, br\303\244nsle, l/ha, l/h, tid p\303\245 f\303\244ltet och i arbete, stillast\303\245ende, k\303\266rstr\303\244cka, snitthastighet, ha/h och kostnad per hektar.\n"
    "G\n"
    "P**F\303\244ltet** = det st\303\266rsta sammanh\303\244ngande omr\303\245det du k\303\266rt sedan NOLLA. Enstaka loggningar l\303\245ngt bort r\303\244knas inte. V\303\244ndningar upp till 30 sekunder r\303\244knas som arbete.\n"
    "G\n"
    "PFungerar ute p\303\245 f\303\244ltet utan internet. Rapporten sparas ocks\303\245 p\303\245 SD-kortet (mappen RAPPORT). Den skickas inte n\303\245gon annanstans."},
  {"10. Kartans inst\303\244llningar",
    "PSkiftnyckeln p\303\245 helsk\303\244rmskartan:\n"
    "G\n"
    "B**Trigger** \342\200\223 vad som startar och stoppar loggningen:\n"
    "b**ISOBUS (auto)** (f\303\266rval): loggar n\303\244r ISOBUS-redskapet s\303\244ger att det arbetar.\n"
    "b**Ventil**, **kraftuttag** bak/fram, **lyft** bak/fram, eller **manuellt** med START/STOPP.\n"
    "B**Startar / Stoppar** \342\200\223 vilken h\303\244ndelse som startar och stoppar (t.ex. lyft NER / UPP).\n"
    "B**Arbetsbredd** \342\200\223 **AUTO** tar bredden fr\303\245n redskapet. **MANUELL**: st\303\244ll med - / +, eller **tryck p\303\245 v\303\244rdet** och skriv in exakt i centimeter. Manuell bredd anv\303\244nds ocks\303\245 n\303\244r AUTO \303\244r valt men redskapet inte skickar n\303\245gon bredd.\n"
    "B**Bakgrund** \342\200\223 **SATELLIT** (satellitbild) eller **ENF\303\204RGAD** (ljusgul, som en styrbild).\n"
    "G\n"
    "PSatellitbilderna h\303\244mtas automatiskt n\303\244r sk\303\244rmen har WiFi och GPS, 6 km runt platsen, och sparas p\303\245 SD-kortet. K\303\244lla: Sentinel-2 cloudless 2016 av EOX IT Services GmbH (CC BY 4.0)."},
  {"11. Br\303\244nsleanalysen",
    "PTryck p\303\245 br\303\244nslediagrammet. Analysen g\303\244ller **de senaste 30 minuterna** av arbete.\n"
    "G\n"
    "B**\303\226verst:** snitthastighet, l/h, l/ha, kostnad per hektar och kapacitet ha/h.\n"
    "B**Staplarna:** kostnad per hektar f\303\266r varje hastighet (steg om 1 km/h), med l/ha under. **Gr\303\266nt** = l\303\266nsammast, **gult** = sn\303\245last p\303\245 diesel.\n"
    "B**Rekommendation** l\303\244ngst ner, \303\244ven om motorvarv.\n"
    "B**L\303\244ge** till v\303\244nster: **EKONOMI** (tiden v\303\244ger h\303\244lften), **BALANS** (som den kostar), **BR\303\205TTOM** (tiden v\303\244ger dubbelt, t.ex. regn p\303\245 v\303\244g).\n"
    "B**PRISER:** diesel och traktor + f\303\266rare. Sparas. Valutan \303\244ndras med `valuta=` i CONFIG.TXT.\n"
    "G\n"
    "PArbete = n\303\244r kartan loggar. V\303\244ndningar upp till 30 s r\303\244knas som arbete. Det \303\244r en uppskattning \342\200\223 terr\303\244ng, last och v\303\244ndningar p\303\245verkar."},
  {"12. Inst\303\244llningar",
    "PSkiftnyckeln:\n"
    "G\n"
    "B**Ljusstyrka** \342\200\223 d\303\244mpar f\303\244rgerna.\n"
    "B**Dag / natt** \342\200\223 ljusa eller m\303\266rka f\303\244rger.\n"
    "B**Spr\303\245k** \342\200\223 SVENSKA eller ENGLISH.\n"
    "B**Arbetsbredd** \342\200\223 AUTO eller MANUELL; tryck p\303\245 v\303\244rdet f\303\266r att skriva in centimeter.\n"
    "B**WiFi \342\206\222 V\303\204LJ N\303\204T** \342\200\223 v\303\244lj n\303\244t och skriv l\303\266senordet med sk\303\244rmens tangentbord.\n"
    "B**ISOBUS:** **TRAKTOR** (traktorsidan), **REDSKAP** (redskapssidan), **DIAGNOS** (vad bussen skickar).\n"
    "B**DEMO** \342\200\223 visar p\303\245hittade v\303\244rden f\303\266r att prova sk\303\244rmen. Inget sparas och kartan loggar inte. St\303\244ngs av med DEMO AV, vid omstart eller n\303\244r riktig trafik kommer.\n"
    "B**Inforaden** \342\200\223 sk\303\244rmens namn, version och SD-kort.\n"
    "B**MANUAL** \342\200\223 den h\303\244r manualen. Tryck p\303\245 ett kapitel; **<** och **>** bl\303\244ddrar, **INNEH\303\205LL** g\303\245r tillbaka.\n"
    "B**ENHET** \342\200\223 chippets temperatur, minne, SD-kort, drifttid, varf\303\266r sk\303\244rmen startade senast, WiFi och ISOBUS-trafik.\n"
    "B**MER** \342\200\223 uppdateringar, loggning, feedback och ansvar & licenser (kapitel 13 och 15).\n"
    "B**Bocken** uppe till h\303\266ger g\303\245r tillbaka till sidan du kom ifr\303\245n.\n"
    "G\n"
    "PSk\303\244rmens namn och fler WiFi-n\303\244t kan l\303\244ggas i filen **CONFIG.TXT** p\303\245 SD-kortet (se README)."},
  {"13. Mer: uppdateringar, loggning och feedback",
    "P**Uppdateringar** \342\200\223 **FR\303\205GA** (f\303\266rval) eller **AV**.\n"
    "BN\303\244r sk\303\244rmen startar eller f\303\245r internet kollar den om en ny version finns i det publika arkivet p\303\245 GitHub.\n"
    "BFinns det en visas en ruta med ansvarstext. **Ingenting installeras utan att du trycker JA, INSTALLERA.** **INTE NU** fr\303\245gar igen n\303\244sta g\303\245ng. **ST\303\204NG AV** fr\303\245gar aldrig mer.\n"
    "B**AV** = sk\303\244rmen letar aldrig efter nya versioner och kontaktar inte GitHub.\n"
    "BStartar en ny version inte ordentligt g\303\245r sk\303\244rmen sj\303\244lv tillbaka till den f\303\266rra.\n"
    "G\n"
    "P**Loggning** \342\200\223 **P\303\205** spelar in all ISOBUS-trafik p\303\245 SD-kortet (filer CANnnnnn.LOG). Bra vid fels\303\266kning eller om du vill se vad din traktor skickar. **AV** (f\303\266rval) = ingen inspelning. Inget laddas upp.\n"
    "G\n"
    "P**Feedback** \342\200\223 **SKICKA FEEDBACK** visar en QR-kod till projektets \303\244rendesida p\303\245 GitHub. D\303\244r kan du rapportera fel, fr\303\245ga och f\303\266resl\303\245 f\303\266rb\303\244ttringar. Det besvaras **i m\303\245n av tid** \342\200\223 det finns inget l\303\266fte om svar eller support. Allt som skrivs d\303\244r \303\244r offentligt."},
  {"14. Vilol\303\244ge och fels\303\266kning",
    "P**Vilol\303\244ge:** n\303\244r bussen \303\244r tyst visar sk\303\244rmen loggan och \"V\303\244ntar p\303\245 ISOBUS\". Sk\303\244rmens namn och version st\303\245r nere till h\303\266ger.\n"
    "G\n"
    "P**Om n\303\245got inte fungerar:**\n"
    "B**\"Ingen ISOBUS-trafik\":** kontrollera kontakten i uttaget och att CAN H och CAN L inte \303\244r f\303\266rv\303\244xlade.\n"
    "B**Inga v\303\244rden fr\303\245n traktorn:** v\303\244nta n\303\245gra sekunder efter start. Inst\303\244llningar \342\206\222 DIAGNOS visar vad som h\303\266rs.\n"
    "B**\"Ingen GPS-position\" p\303\245 kartan:** styrsystemet skickar ingen position p\303\245 ISOBUS.\n"
    "B**\"Utanf\303\266r kartans 4 km\":** kartan t\303\244cker 4 \303\227 4 km runt d\303\244r du b\303\266rjade \342\200\223 tryck NOLLA p\303\245 nya platsen.\n"
    "B**Slirning visar \"--\":** under 1,5 km/h, eller hastigheterna saknas.\n"
    "B**Dragkraft n\303\244ra noll i arbete:** redskapet h\303\244nger i kroken, inte i lyften \342\200\223 titta p\303\245 slirningen.\n"
    "B**F\303\244ltrapporten:** st\303\244ng och \303\266ppna telefonens WiFi om sidan inte kommer upp, och skanna QR-kod 1 igen.\n"
    "B**N\303\245got annat:** Inst\303\244llningar \342\206\222 MER \342\206\222 SKICKA FEEDBACK."},
  {"15. Om F\303\244ltloggen: ansvar och licenser",
    "P**Ingen koppling till tillverkare.** F\303\244ltloggen \303\244r byggd helt utan inblandning fr\303\245n Fendt, AGCO eller n\303\245gon annan tillverkare av traktorer, redskap eller elektronik (inte heller Waveshare, som g\303\266r sk\303\244rmkortet). Den \303\244r inte gjord, granskad, godk\303\244nd eller st\303\266dd av dem. Fendt, AGCO, Vario, ISOBUS och andra namn \303\244r varum\303\244rken som tillh\303\266r sina \303\244gare och n\303\244mns bara f\303\266r att beskriva vad sk\303\244rmen fungerar med.\n"
    "G\n"
    "P**Ingen garanti \342\200\223 eget ansvar.** F\303\244ltloggen levereras i befintligt skick, utan garanti av n\303\245got slag. Att bygga, installera, uppdatera och anv\303\244nda den sker helt p\303\245 ditt eget ansvar. Den som gjort F\303\244ltloggen ansvarar inte f\303\266r fel, skador p\303\245 traktor, redskap, person eller egendom, f\303\266rlorad data eller beslut som fattas utifr\303\245n det som visas.\n"
    "BSk\303\244rmen lyssnar bara p\303\245 ISOBUS och skickar ingenting \342\200\223 men du ansluter den till traktorn p\303\245 egen risk.\n"
    "BDen ers\303\244tter inte traktorns terminal eller instrument. Alla v\303\244rden (areal, br\303\244nsle, kostnad, slirning, dragkraft) \303\244r uppskattningar.\n"
    "BL\303\245t inte sk\303\244rmen ta uppm\303\244rksamheten fr\303\245n k\303\266rningen.\n"
    "G\n"
    "P**\303\226ppen k\303\244llkod.** Programmet \303\244r fri programvara under **GNU GPL version 3 eller senare**. Du f\303\245r anv\303\244nda, \303\244ndra, dela och s\303\244lja det, men den som delar eller s\303\244ljer det \342\200\223 \303\244ndrat eller inte \342\200\223 m\303\245ste ge mottagaren k\303\244llkoden och sina \303\244ndringar under samma licens. Skalen och ritningarna \303\244r under **CERN-OHL-S v2**, manualen under **CC BY-SA 4.0**. Hela texterna finns i arkivet p\303\245 GitHub.\n"
    "G\n"
    "P**Det h\303\244r ing\303\245r, med egna licenser:**\n"
    "BQR Code generator library \342\200\223 Project Nayuki (MIT)\n"
    "BGFX Library for Arduino \342\200\223 moononournation, bygger p\303\245 Adafruit GFX (BSD)\n"
    "BU8g2 \342\200\223 olikraus (BSD 2-clause); typsnittet DejaVu Sans, omgjort till bildpunktstypsnitt (Bitstream Vera-licensen)\n"
    "BESP32 Arduino-k\303\244rnan \342\200\223 Espressif (LGPL 2.1) och ESP-IDF (Apache 2.0)\n"
    "BKartbild: Sentinel-2 cloudless \342\200\223 https://s2maps.eu av EOX IT Services GmbH (inneh\303\245ller bearbetade Copernicus Sentinel-data 2016), CC BY 4.0\n"
    "G\n"
    "P**Integritet.** F\303\244ltloggen skickar inga loggar, positioner eller rapporter n\303\245gonstans. Med WiFi kontaktar den bara GitHub (versionskoll, om uppdateringar \303\244r p\303\245) och EOX (kartbilder \342\200\223 EOX ser d\303\245 vilket omr\303\245de bilderna g\303\244ller). Feedback skickar du sj\303\244lv via telefonen."},
};
static const ManKap MANUAL_EN[] = {
  {"1. What F\303\244ltloggen is",
    "PF\303\244ltloggen (\"the field log\") is an extra display for the tractor cab. It connects to the cab's ISOBUS socket and shows what the tractor and the implement are doing: speed, engine, hitch, PTO, hydraulics, fuel, wheel slip and where you have driven.\n"
    "G\n"
    "PThe display **only listens**. It never sends anything on the bus and cannot control the tractor or the implement. Everything you press only affects F\303\244ltloggen itself.\n"
    "G\n"
    "PF\303\244ltloggen is free and open source. It is made by a private person in spare time, entirely without involvement from Fendt, AGCO or any other manufacturer. Everything is used at your own risk \342\200\223 read chapter 15.\n"
    "G\n"
    "PIt has been tested on Fendt Vario S4 (720 and 828). Other tractors may send other things; whatever is not heard is shown as \"--\"."},
  {"2. The buttons on the left",
    "PThe tractor page and the implement page have the same column of buttons on the far left, from the top:\n"
    "G\n"
    "N1. **The page's own buttons** (two places) \342\200\223 on the tractor page **Info\342\201\272 Three-point hitch** and **Info\342\201\272 Hydraulic valves**, on the implement page **TERMINAL**.\n"
    "N2. **The moon / sun** \342\200\223 switches between day and night colours. Saved.\n"
    "N3. **The house** \342\200\223 the start choice: TRACTOR, ISOBUS or TERMINAL.\n"
    "N4. **The spanner** \342\200\223 settings.\n"
    "G\n"
    "PThe chosen button has a red frame.\n"
    "G\n"
    "PTwo things open by tapping the box itself:\n"
    "B**The map box** \342\206\222 full-screen map.\n"
    "B**The fuel chart** \342\206\222 fuel analysis."},
  {"3. The start choice",
    "PWhen the display finds an ISOBUS implement it asks what you want to see:\n"
    "G\n"
    "B**TRACTOR** \342\200\223 the tractor page.\n"
    "B**ISOBUS** \342\200\223 the implement page with the implement's own values.\n"
    "B**TERMINAL** \342\200\223 the implement's own screen, as on the tractor terminal (when it has been captured).\n"
    "G\n"
    "PIf you do not answer, the display picks the implement after 20 seconds. If no implement is found it goes straight to the tractor page.\n"
    "G\n"
    "PIf you open the start choice with **the house** it never picks by itself. Without an implement the ISOBUS button is grey with the text \"no implement found\"."},
  {"4. The tractor page",
    "PFour boxes:\n"
    "G\n"
    "P**Top left \342\200\223 Info\342\201\272** (chosen with the buttons on the left):\n"
    "B**Three-point hitch:** position in percent front and rear, PTO speeds, WORK when the hitch is in work position.\n"
    "B**Hydraulic valves:** every valve the tractor sends, with the flow out (+) or in (-). The names and colours (Yellow, Blue, Red, Green, Brown, Front) follow Fendt's order \342\200\223 if they do not match your tractor, go by the number.\n"
    "G\n"
    "P**Top right \342\200\223 Tractor:**\n"
    "BThe blue strip: green dot = the bus is heard, WiFi, **hour meter**, **slope** front/rear and sideways (degrees).\n"
    "BThe tractor symbol: the hitch arms move with the hitch (green when working), the gears turn green when the PTOs run. The symbol tilts with the slope and shows lights that are on (headlights, work lights, indicators, brake lights, reversing light). The beacon on the cab roof turns orange and \"rotates\" when on.\n"
    "BThe rows: **speed** and **direction** (FWD green / REV), **engine speed** and **fuel l/h**, **wheel slip** and **draft force**.\n"
    "G\n"
    "P**Bottom left \342\200\223 Map:** where you are and what has been covered. The area in the white box, green ring when the map is logging, l/ha at the bottom left. Tap for full screen.\n"
    "G\n"
    "P**Bottom right \342\200\223 Fuel:** l/h and l/min now, and a bar chart with one average per minute for the last 30 minutes. Tap for the fuel analysis."},
  {"5. Wheel slip and draft force",
    "P**Wheel slip (%)** is calculated by F\303\244ltloggen: how much faster the wheels turn than the tractor really moves (wheel speed compared with ground speed). It is smoothed over a few seconds and only shown above 1.5 km/h.\n"
    "BThe field turns **yellow from 20 %** and **red from 30 %**.\n"
    "BDuring transport it is normally a few percent.\n"
    "G\n"
    "P**Draft force (kN)** comes from the force sensors in the rear hitch lower links. It is therefore **only seen when the implement hangs in the hitch** \342\200\223 an implement on the drawbar or hitch hook is not seen here. Use the wheel slip instead.\n"
    "BThe arrow grows with the force. The field turns **yellow from 35 kN** and **red from 50 kN**.\n"
    "G\n"
    "PBoth are estimates from the tractor's own sensors. Trust the tractor's instruments if they differ."},
  {"6. The implement page and TERMINAL",
    "P**The implement page** shows the values an ISOBUS implement sends to the tractor's Task Controller, with the implement's own names and units as the implement itself describes them. At the top are speed, working width and work state. If the list is long the button on the left turns the page.\n"
    "G\n"
    "P**TERMINAL** shows the implement's own screen, the same as on the tractor terminal, updated all the time. It is **only a picture** \342\200\223 the buttons cannot be pressed here. Use the tractor terminal to control the implement.\n"
    "G\n"
    "PThe picture is captured the first time the implement uploads it to the terminal and is saved on the SD card. If the terminal already has it stored it is not sent \342\200\223 delete the implement's stored pool in the terminal and restart."},
  {"7. The full-screen map",
    "P**Buttons on the right:** back \302\267 **+** / **100 m** / **-** (zoom) \302\267 **START / STOP** (manual logging) \302\267 the spanner (map settings) \302\267 **RESET** \302\267 **FOLLOW** (shown when the map has been moved).\n"
    "G\n"
    "B**Drag with a finger** to move the map. **FOLLOW** centres on the tractor again.\n"
    "BNorth is always up. The tractor is the small marker; the implement is drawn 3 m behind the antenna.\n"
    "B**The Area box:** area worked in hectares, fuel in litres and l/ha since the last RESET, and the button **EXPORT REPORT** (chapter 9).\n"
    "B**Field area:** press **FIELD AREA** and type the area in hectares with up to two decimals. The box then shows how many hectares are left, how long it will take and when it will be done. The time is calculated from the last approx. 20 minutes of logging and is shown after about 3 minutes of work. The clock time needs the display to know the time (internet or GPS). RESET removes the field area.\n"
    "B**The tab with the arrow** folds the boxes in and out so the map is easier to see.\n"
    "B**RESET** removes tracks, area, fuel and colours (the display asks first). Do it when you start a new field.\n"
    "G\n"
    "POverlapping passes are only counted once in the area. The map needs a GPS position from the tractor's guidance system on ISOBUS."},
  {"8. Track colour",
    "PThe **Track colour** box under Area chooses what the track is coloured by:\n"
    "G\n"
    "B**FUEL** \342\200\223 litres per hectare.\n"
    "B**DRAFT** \342\200\223 draft force in the hitch (kN).\n"
    "B**SLIP** \342\200\223 wheel slip (%).\n"
    "G\n"
    "PThe colours are compared with **the run's own average** since the last RESET:\n"
    "B**blue** = like the average\n"
    "B**green** = 30 % lower\n"
    "B**red** = 30 % higher\n"
    "G\n"
    "PThe numbers under the scale show what fully green, the average and fully red are right now. The colours are saved with the track and remain after a restart. Below 1.5 km/h no colour is painted."},
  {"9. Field report to the phone",
    "N1. Full-screen map \342\206\222 **EXPORT REPORT**.\n"
    "N2. Type the field name, or press **SKIP** (the name is then the date + the display's name).\n"
    "N3. Wait a few seconds while the report is created.\n"
    "N4. Scan **QR code 1** with the phone \342\200\223 it joins the display's own WiFi (no password).\n"
    "N5. Scan **QR code 2** (or open http://192.168.4.1) and press **DOWNLOAD**.\n"
    "N6. Press **CLOSE** when you are done.\n"
    "G\n"
    "PThe report is a PDF with three maps (fuel, wheel slip, draft force) and area, fuel, l/ha, l/h, time in the field and working, standing still, distance, average speed, ha/h and cost per hectare.\n"
    "G\n"
    "P**The field** = the largest connected area you have driven since RESET. Odd logging far away does not count. Turns up to 30 seconds count as work.\n"
    "G\n"
    "PWorks out in the field without internet. The report is also saved on the SD card (folder RAPPORT). It is not sent anywhere else."},
  {"10. Map settings",
    "PThe spanner on the full-screen map:\n"
    "G\n"
    "B**Trigger** \342\200\223 what starts and stops logging:\n"
    "b**ISOBUS (auto)** (default): logs when the ISOBUS implement says it is working.\n"
    "b**Valve**, rear/front **PTO**, rear/front **hitch**, or **manual** with START/STOP.\n"
    "B**Starts / Stops** \342\200\223 which event starts and stops (e.g. hitch DOWN / UP).\n"
    "B**Working width** \342\200\223 **AUTO** takes the width from the implement. **MANUAL**: set with - / +, or **tap the value** and type it exactly in centimetres. The manual width is also used when AUTO is chosen but the implement sends no width.\n"
    "B**Background** \342\200\223 **SATELLITE** (satellite image) or **PLAIN** (light yellow, like a guidance screen).\n"
    "G\n"
    "PThe satellite images are downloaded automatically when the display has WiFi and GPS, 6 km around the place, and saved on the SD card. Source: Sentinel-2 cloudless 2016 by EOX IT Services GmbH (CC BY 4.0)."},
  {"11. The fuel analysis",
    "PTap the fuel chart. The analysis covers **the last 30 minutes** of work.\n"
    "G\n"
    "B**Top:** average speed, l/h, l/ha, cost per hectare and capacity ha/h.\n"
    "B**The bars:** cost per hectare for every speed (1 km/h steps), with l/ha below. **Green** = best value, **yellow** = lowest fuel use.\n"
    "B**Recommendation** at the bottom, also about engine speed.\n"
    "B**Mode** on the left: **ECONOMY** (time counts half), **BALANCE** (at its cost), **HURRY** (time counts double, e.g. rain on the way).\n"
    "B**PRICES:** diesel and tractor + driver. Saved. The currency is set with `valuta=` in CONFIG.TXT.\n"
    "G\n"
    "PWorking = when the map is logging. Turns up to 30 s count as work. It is an estimate \342\200\223 terrain, load and turns all matter."},
  {"12. Settings",
    "PThe spanner:\n"
    "G\n"
    "B**Brightness** \342\200\223 dims the colours.\n"
    "B**Day / night** \342\200\223 light or dark colours.\n"
    "B**Language** \342\200\223 SVENSKA or ENGLISH.\n"
    "B**Working width** \342\200\223 AUTO or MANUAL; tap the value to type centimetres.\n"
    "B**WiFi \342\206\222 CHOOSE** \342\200\223 choose a network and type the password on the display's keyboard.\n"
    "B**ISOBUS:** **TRACTOR** (tractor page), **IMPLEMENT** (implement page), **DIAGNOSTICS** (what the bus sends).\n"
    "B**DEMO** \342\200\223 shows made-up values to try the display. Nothing is saved and the map does not log. Turned off with DEMO OFF, by a restart or when real traffic arrives.\n"
    "B**The info line** \342\200\223 the display's name, version and SD card.\n"
    "B**MANUAL** \342\200\223 this manual. Tap a chapter; **<** and **>** turn pages, **CONTENTS** goes back.\n"
    "B**DEVICE** \342\200\223 the chip temperature, memory, SD card, uptime, why the display last restarted, WiFi and ISOBUS traffic.\n"
    "B**MORE** \342\200\223 updates, logging, feedback and liability & licences (chapters 13 and 15).\n"
    "B**The tick** at the top right goes back to the page you came from.\n"
    "G\n"
    "PThe display's name and more WiFi networks can be put in the file **CONFIG.TXT** on the SD card (see README)."},
  {"13. More: updates, logging and feedback",
    "P**Updates** \342\200\223 **ASK** (default) or **OFF**.\n"
    "BWhen the display starts or gets internet it checks whether a new version is in the public repository on GitHub.\n"
    "BIf there is, a box with a liability text is shown. **Nothing is installed unless you press YES, INSTALL.** **NOT NOW** asks again next time. **TURN OFF** never asks again.\n"
    "B**OFF** = the display never looks for new versions and does not contact GitHub.\n"
    "BIf a new version does not start properly the display goes back to the previous one by itself.\n"
    "G\n"
    "P**Logging** \342\200\223 **ON** records all ISOBUS traffic on the SD card (files CANnnnnn.LOG). Useful for fault finding or to see what your tractor sends. **OFF** (default) = no recording. Nothing is uploaded.\n"
    "G\n"
    "P**Feedback** \342\200\223 **SEND FEEDBACK** shows a QR code to the project's issue page on GitHub. There you can report bugs, ask and suggest improvements. It is answered **when time allows** \342\200\223 there is no promise of an answer or support. Everything written there is public."},
  {"14. Idle mode and troubleshooting",
    "P**Idle mode:** when the bus is quiet the display shows the logo and \"Waiting for ISOBUS\". The display's name and version are at the bottom right.\n"
    "G\n"
    "P**If something does not work:**\n"
    "B**\"No ISOBUS traffic\":** check the plug in the socket and that CAN H and CAN L are not swapped.\n"
    "B**No values from the tractor:** wait a few seconds after starting. Settings \342\206\222 DIAGNOSTICS shows what is heard.\n"
    "B**\"No GPS position\" on the map:** the guidance system sends no position on ISOBUS.\n"
    "B**\"Outside the 4 km map\":** the map covers 4 \303\227 4 km around where you started \342\200\223 press RESET at the new place.\n"
    "B**Wheel slip shows \"--\":** below 1.5 km/h, or the speeds are missing.\n"
    "B**Draft force near zero while working:** the implement is on the drawbar, not in the hitch \342\200\223 look at the wheel slip.\n"
    "B**Field report:** turn the phone's WiFi off and on if the page does not appear, and scan QR code 1 again.\n"
    "B**Anything else:** Settings \342\206\222 MORE \342\206\222 SEND FEEDBACK."},
  {"15. About F\303\244ltloggen: liability and licences",
    "P**No connection to manufacturers.** F\303\244ltloggen was built entirely without involvement from Fendt, AGCO or any other manufacturer of tractors, implements or electronics (nor Waveshare, who make the display board). It is not made, reviewed, approved or supported by them. Fendt, AGCO, Vario, ISOBUS and other names are trademarks of their owners and are only mentioned to describe what the display works with.\n"
    "G\n"
    "P**No warranty \342\200\223 your own risk.** F\303\244ltloggen is provided as is, without warranty of any kind. Building, installing, updating and using it is entirely at your own risk. The person who made F\303\244ltloggen is not responsible for errors, damage to tractor, implement, person or property, lost data or decisions made from what is shown.\n"
    "BThe display only listens to ISOBUS and sends nothing \342\200\223 but you connect it to the tractor at your own risk.\n"
    "BIt does not replace the tractor's terminal or instruments. All values (area, fuel, cost, slip, draft) are estimates.\n"
    "BDo not let the display take your attention away from driving.\n"
    "G\n"
    "P**Open source.** The software is free software under the **GNU GPL version 3 or later**. You may use, change, share and sell it, but whoever shares or sells it \342\200\223 changed or not \342\200\223 must give the recipient the source code and their changes under the same licence. The housings and drawings are under **CERN-OHL-S v2**, the manual under **CC BY-SA 4.0**. The full texts are in the repository on GitHub.\n"
    "G\n"
    "P**Included, with their own licences:**\n"
    "BQR Code generator library \342\200\223 Project Nayuki (MIT)\n"
    "BGFX Library for Arduino \342\200\223 moononournation, based on Adafruit GFX (BSD)\n"
    "BU8g2 \342\200\223 olikraus (BSD 2-clause); the DejaVu Sans font, converted to a bitmap font (Bitstream Vera licence)\n"
    "BESP32 Arduino core \342\200\223 Espressif (LGPL 2.1) and ESP-IDF (Apache 2.0)\n"
    "BMap image: Sentinel-2 cloudless \342\200\223 https://s2maps.eu by EOX IT Services GmbH (contains modified Copernicus Sentinel data 2016), CC BY 4.0\n"
    "G\n"
    "P**Privacy.** F\303\244ltloggen sends no logs, positions or reports anywhere. With WiFi it only contacts GitHub (version check, if updates are on) and EOX (map images \342\200\223 EOX then sees which area the images cover). You send feedback yourself from your phone."},
};
static const int MAN_N = sizeof(MANUAL_SV) / sizeof(MANUAL_SV[0]);
static_assert(sizeof(MANUAL_SV) == sizeof(MANUAL_EN), "manualerna har olika antal kapitel");
#define MANUAL (uiLang ? MANUAL_EN : MANUAL_SV)
