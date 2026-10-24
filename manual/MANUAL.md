# Fältloggen – manual

Samma text finns på skärmen: Inställningar → MANUAL. Engelsk version: `MANUAL_EN.md`.
Vid bygget gör `tools/manual/md2c.py` om båda filerna till `src/Faltloggen/manual_data.h` – ändra här, inte där.
Båda språken måste ha lika många kapitel i samma ordning. Det sista kapitlet öppnas också av knappen
ANSVAR & LICENSER under Inställningar → MER.
Rader i *kursiv* är anteckningar till den som programmerar och visas inte på skärmen.

---

## 1. Vad Fältloggen är

Fältloggen är en extra skärm i traktorhytten. Den kopplas till hyttens ISOBUS-uttag och visar vad
traktorn och redskapet gör: hastighet, motor, lyft, kraftuttag, hydraulik, bränsle, slirning och var du har kört.

Skärmen **lyssnar bara**. Den skickar aldrig något på bussen och kan inte styra traktorn eller redskapet.
Allt du trycker på påverkar bara Fältloggen själv.

Fältloggen är gratis och öppen källkod. Den är gjord av en privatperson på fritiden, helt utan
inblandning från Fendt, AGCO eller någon annan tillverkare. Allt används på eget ansvar – läs
kapitel 15.

Den är provad på Fendt Vario S4 (720 och 828). Andra traktorer kan skicka andra saker; det som inte hörs
visas som "--".

---

## 2. Knapparna till vänster

Traktorsidan och redskapssidan har samma knapprad längst till vänster, uppifrån:

1. **Sidans egna knappar** (två platser) – på traktorsidan **Info⁺ Trepunktslyft** och
   **Info⁺ Hydraulventiler**, på redskapssidan **TERMINAL**.
2. **Månen / solen** – växlar mellan dag- och nattfärger. Valet sparas.
3. **Huset** – startvalet: välj TRAKTOR, ISOBUS eller TERMINAL.
4. **Skiftnyckeln** – inställningarna.

Vald knapp har röd ram.

Två saker öppnas genom att trycka direkt på rutan:
- **Kartrutan** → kartan i helskärm.
- **Bränslediagrammet** → bränsleanalysen.

---

## 3. Startvalet

När skärmen hittar ett ISOBUS-redskap frågar den vad du vill se:

- **TRAKTOR** – traktorsidan.
- **ISOBUS** – redskapssidan med redskapets egna värden.
- **TERMINAL** – redskapets egen bild, som på traktorns terminal (finns när bilden är fångad).

Svarar du inte väljer skärmen redskapet själv efter 20 sekunder. Hittas inget redskap går den direkt
till traktorsidan.

Öppnar du startvalet med **huset** väljer den aldrig själv. Finns inget redskap är ISOBUS-knappen grå
med texten "inget redskap hittat".

---

## 4. Traktorsidan

Fyra rutor:

**Uppe till vänster – Info⁺** (välj med knapparna till vänster):
- **Trepunktslyft:** läge i procent fram och bak, kraftuttagens varvtal, ARBETE när lyften är i arbetsläge.
- **Hydraulventiler:** varje uttag som traktorn skickar, med flödet ut (+) eller in (−). Namnen och färgerna
  (Gul, Blå, Röd, Grön, Brun, Front) följer Fendts ordning – stämmer de inte med din traktor, gå efter numret.

**Uppe till höger – Traktor:**
- Den blå listen: grön prick = bussen hörs, WiFi, **timräknaren**, **lutning** fram/bak och i sidled (grader).
- Traktorsymbolen: lyftarmarna rör sig med lyften (gröna i arbete), kugghjulen blir gröna när
  kraftuttagen går. Symbolen lutar med backen och visar tända lampor (strålkastare, arbetsljus,
  blinkers, bromsljus, backljus). Varningsljuset på hyttaket blir orange och "roterar" när det är på.
- Raderna: **hastighet** och **körriktning** (FRAM grön / BACK), **motorvarv** och **bränsle l/h**,
  **slirning** och **dragkraft**.

**Nere till vänster – Karta:** var du är och vad som är kört. Arealen i den vita rutan, grön ring när
kartan loggar, l/ha nere till vänster. Tryck för helskärm.

**Nere till höger – Bränsle:** l/h och l/min nu, och ett stapeldiagram med ett medelvärde per minut
för de senaste 30 minuterna. Tryck för bränsleanalysen.

---

## 5. Slirning och dragkraft

**Slirning (%)** räknas ut av Fältloggen: hur mycket fortare hjulen snurrar än traktorn faktiskt rör sig
(hjulhastighet jämfört med markhastighet). Värdet jämnas ut över några sekunder och visas bara över 1,5 km/h.
- Fältet blir **gult från 20 %** och **rött från 30 %**.
- Vid transport ligger det normalt på några procent.

**Dragkraft (kN)** kommer från kraftgivarna i baklyftens undre länkarmar. Den syns därför **bara när
redskapet hänger i lyftarmarna** – ett redskap i dragkroken eller på draget syns inte här. Använd då
slirningen i stället.
- Pilen växer med kraften. Fältet blir **gult från 35 kN** och **rött från 50 kN**.

Båda är uppskattningar från traktorns egna givare. Lita på traktorns instrument om de skiljer sig.

---

## 6. Redskapssidan och TERMINAL

**Redskapssidan** visar värdena som ett ISOBUS-redskap skickar till traktorns Task Controller, med
redskapets egna namn och enheter så som redskapet själv beskriver dem. Överst står hastighet,
arbetsbredd och arbetsläge. Är listan lång bläddrar knappen till vänster.

**TERMINAL** visar redskapets egen bild, samma som på traktorns terminal, och uppdateras hela tiden.
Det är **bara en bild** – knapparna går inte att trycka på här. Använd traktorns terminal för att styra
redskapet.

Bilden fångas första gången redskapet laddar upp den till terminalen och sparas på SD-kortet. Har
terminalen redan bilden sparad skickas den inte – radera då redskapets sparade bild i terminalen och
starta om.

---

## 7. Kartan i helskärm

**Knapparna till höger:** tillbaka · **+** / **100 m** / **−** (zoom) · **START / STOPP** (loggning för
hand) · skiftnyckeln (kartans inställningar) · **NOLLA** · **FÖLJ** (visas när kartan är flyttad).

- **Dra med fingret** för att flytta kartan. **FÖLJ** centrerar på traktorn igen.
- Norr är alltid uppåt. Traktorn är den lilla markören, redskapet ritas 3 m bakom antennen.
- **Areal-rutan:** bearbetad yta i hektar, bränsle i liter och l/ha sedan senaste NOLLA,
  och knappen **EXPORTERA RAPPORT** (kapitel 9).
- **Skiftets areal:** tryck på **SKIFTETS AREAL** och knappa in arealen i hektar med upp till två decimaler.
  Rutan visar sedan hur många hektar som är kvar, hur lång tid det är kvar och vilken klockan det blir klart.
  Tiden räknas ut löpande ur de senaste ca 20 minuterna av loggning och visas efter ca 3 minuters arbete.
  Klockslaget kräver att skärmen har tid (internet eller GPS). NOLLA tar bort skiftet.
- **Fliken med pilen** fäller in och ut rutorna så att kartan syns bättre.
- **NOLLA** tar bort spår, areal, bränsle och färger (skärmen frågar först). Gör det när du börjar på ett
  nytt fält.

Överlappande körning räknas bara en gång i arealen. Kartan behöver en GPS-position från traktorns
styrsystem på ISOBUS.

---

## 8. Färg på spåret

Rutan **Färg på spåret** under Areal väljer vad spåret färgas efter:

- **BRÄNSLE** – liter per hektar.
- **DRAG** – dragkraft i lyften (kN).
- **SLIRNING** – slirning (%).

Färgerna jämförs med **körningens eget snitt** sedan senaste NOLLA:
- **blått** = som snittet
- **grönt** = 30 % lägre
- **rött** = 30 % högre

Siffrorna under färgstegen visar vad helt grönt, snittet och helt rött är just nu. Färgerna sparas med
spåret och finns kvar efter omstart. Under 1,5 km/h målas ingen färg.

---

## 9. Fältrapport till telefonen

1. Kartan i helskärm → **EXPORTERA RAPPORT**.
2. Skriv fältets namn, eller tryck **HOPPA ÖVER** (då blir namnet datum + skärmens namn).
3. Vänta några sekunder medan rapporten skapas.
4. Skanna **QR-kod 1** med telefonen – den ansluter till skärmens eget WiFi (inget lösenord).
5. Skanna **QR-kod 2** (eller öppna http://192.168.4.1) och tryck **LADDA NER**.
6. Tryck **STÄNG** när du är klar.

Rapporten är en PDF med tre kartor (bränsle, slirning, dragkraft) och areal, bränsle, l/ha, l/h, tid på
fältet och i arbete, stillastående, körsträcka, snitthastighet, ha/h och kostnad per hektar.

**Fältet** = det största sammanhängande området du kört sedan NOLLA. Enstaka loggningar långt bort räknas
inte. Vändningar upp till 30 sekunder räknas som arbete.

Fungerar ute på fältet utan internet. Rapporten sparas också på SD-kortet (mappen RAPPORT). Den skickas
inte någon annanstans.

---

## 10. Kartans inställningar

Skiftnyckeln på helskärmskartan:

- **Trigger** – vad som startar och stoppar loggningen:
  - **ISOBUS (auto)** (förval): loggar när ISOBUS-redskapet säger att det arbetar.
  - **Ventil**, **kraftuttag** bak/fram, **lyft** bak/fram, eller **manuellt** med START/STOPP.
- **Startar / Stoppar** – vilken händelse som startar och stoppar (t.ex. lyft NER / UPP).
- **Arbetsbredd** – **AUTO** tar bredden från redskapet. **MANUELL**: ställ med − / +, eller **tryck på
  värdet** och skriv in exakt i centimeter. Manuell bredd används också när AUTO är valt men redskapet
  inte skickar någon bredd.
- **Bakgrund** – **SATELLIT** (satellitbild) eller **ENFÄRGAD** (ljusgul, som en styrbild).

Satellitbilderna hämtas automatiskt när skärmen har WiFi och GPS, 6 km runt platsen, och sparas på
SD-kortet. Källa: Sentinel-2 cloudless 2016 av EOX IT Services GmbH (CC BY 4.0).

---

## 11. Bränsleanalysen

Tryck på bränslediagrammet. Analysen gäller **de senaste 30 minuterna** av arbete.

- **Överst:** snitthastighet, l/h, l/ha, kostnad per hektar och kapacitet ha/h.
- **Staplarna:** kostnad per hektar för varje hastighet (steg om 1 km/h), med l/ha under.
  **Grönt** = lönsammast, **gult** = snålast på diesel.
- **Rekommendation** längst ner, även om motorvarv.
- **Läge** till vänster: **EKONOMI** (tiden väger hälften), **BALANS** (som den kostar), **BRÅTTOM**
  (tiden väger dubbelt, t.ex. regn på väg).
- **PRISER:** diesel och traktor + förare. Sparas. Valutan ändras med `valuta=` i CONFIG.TXT.

Arbete = när kartan loggar. Vändningar upp till 30 s räknas som arbete. Det är en uppskattning – terräng,
last och vändningar påverkar.

---

## 12. Inställningar

Skiftnyckeln:

- **Ljusstyrka** – dämpar färgerna.
- **Dag / natt** – ljusa eller mörka färger.
- **Språk** – SVENSKA eller ENGLISH.
- **Arbetsbredd** – AUTO eller MANUELL; tryck på värdet för att skriva in centimeter.
- **WiFi → VÄLJ NÄT** – välj nät och skriv lösenordet med skärmens tangentbord.
- **ISOBUS:** **TRAKTOR** (traktorsidan), **REDSKAP** (redskapssidan), **DIAGNOS** (vad bussen skickar).
- **DEMO** – visar påhittade värden för att prova skärmen. Inget sparas och kartan loggar inte.
  Stängs av med DEMO AV, vid omstart eller när riktig trafik kommer.
- **Inforaden** – skärmens namn, version och SD-kort.
- **MANUAL** – den här manualen. Tryck på ett kapitel; **<** och **>** bläddrar, **INNEHÅLL** går tillbaka.
- **ENHET** – chippets temperatur, minne, SD-kort, drifttid, varför skärmen startade senast, WiFi och
  ISOBUS-trafik.
- **MER** – uppdateringar, loggning, feedback och ansvar & licenser (kapitel 13 och 15).
- **Bocken** uppe till höger går tillbaka till sidan du kom ifrån.

Skärmens namn och fler WiFi-nät kan läggas i filen **CONFIG.TXT** på SD-kortet (se README).

---

## 13. Mer: uppdateringar, loggning och feedback

**Uppdateringar** – **FRÅGA** (förval) eller **AV**.
- När skärmen startar eller får internet kollar den om en ny version finns i det publika arkivet på GitHub.
- Finns det en visas en ruta med ansvarstext. **Ingenting installeras utan att du trycker JA, INSTALLERA.**
  **INTE NU** frågar igen nästa gång. **STÄNG AV** frågar aldrig mer.
- **AV** = skärmen letar aldrig efter nya versioner och kontaktar inte GitHub.
- Startar en ny version inte ordentligt går skärmen själv tillbaka till den förra.

**Loggning** – **PÅ** spelar in all ISOBUS-trafik på SD-kortet (filer CANnnnnn.LOG). Bra vid felsökning
eller om du vill se vad din traktor skickar. **AV** (förval) = ingen inspelning. Inget laddas upp.

**Feedback** – **SKICKA FEEDBACK** visar en QR-kod till projektets ärendesida på GitHub. Där kan du
rapportera fel, fråga och föreslå förbättringar. Det besvaras **i mån av tid** – det finns inget löfte om
svar eller support. Allt som skrivs där är offentligt.

---

## 14. Viloläge och felsökning

**Viloläge:** när bussen är tyst visar skärmen loggan och "Väntar på ISOBUS". Skärmens namn och version står
nere till höger.

**Om något inte fungerar:**
- **"Ingen ISOBUS-trafik":** kontrollera kontakten i uttaget och att CAN H och CAN L inte är förväxlade.
- **Inga värden från traktorn:** vänta några sekunder efter start. Inställningar → DIAGNOS visar vad som hörs.
- **"Ingen GPS-position" på kartan:** styrsystemet skickar ingen position på ISOBUS.
- **"Utanför kartans 4 km":** kartan täcker 4 × 4 km runt där du började – tryck NOLLA på nya platsen.
- **Slirning visar "--":** under 1,5 km/h, eller hastigheterna saknas.
- **Dragkraft nära noll i arbete:** redskapet hänger i kroken, inte i lyften – titta på slirningen.
- **Fältrapporten:** stäng och öppna telefonens WiFi om sidan inte kommer upp, och skanna QR-kod 1 igen.
- **Något annat:** Inställningar → MER → SKICKA FEEDBACK.

---

## 15. Om Fältloggen: ansvar och licenser

**Ingen koppling till tillverkare.** Fältloggen är byggd helt utan inblandning från Fendt, AGCO eller
någon annan tillverkare av traktorer, redskap eller elektronik (inte heller Waveshare, som gör skärmkortet).
Den är inte gjord, granskad, godkänd eller stödd av dem. Fendt, AGCO, Vario, ISOBUS och andra namn är
varumärken som tillhör sina ägare och nämns bara för att beskriva vad skärmen fungerar med.

**Ingen garanti – eget ansvar.** Fältloggen levereras i befintligt skick, utan garanti av något slag.
Att bygga, installera, uppdatera och använda den sker helt på ditt eget ansvar. Den som gjort Fältloggen
ansvarar inte för fel, skador på traktor, redskap, person eller egendom, förlorad data eller beslut som
fattas utifrån det som visas.
- Skärmen lyssnar bara på ISOBUS och skickar ingenting – men du ansluter den till traktorn på egen risk.
- Den ersätter inte traktorns terminal eller instrument. Alla värden (areal, bränsle, kostnad, slirning,
  dragkraft) är uppskattningar.
- Låt inte skärmen ta uppmärksamheten från körningen.

**Öppen källkod.** Programmet är fri programvara under **GNU GPL version 3 eller senare**. Du får använda,
ändra, dela och sälja det, men den som delar eller säljer det – ändrat eller inte – måste ge mottagaren
källkoden och sina ändringar under samma licens. Skalen och ritningarna är under **CERN-OHL-S v2**, manualen
under **CC BY-SA 4.0**. Hela texterna finns i arkivet på GitHub.

**Det här ingår, med egna licenser:**
- QR Code generator library – Project Nayuki (MIT)
- GFX Library for Arduino – moononournation, bygger på Adafruit GFX (BSD)
- U8g2 – olikraus (BSD 2-clause); typsnittet DejaVu Sans, omgjort till bildpunktstypsnitt (Bitstream Vera-licensen)
- ESP32 Arduino-kärnan – Espressif (LGPL 2.1) och ESP-IDF (Apache 2.0)
- Kartbild: Sentinel-2 cloudless – https://s2maps.eu av EOX IT Services GmbH (innehåller bearbetade
  Copernicus Sentinel-data 2016), CC BY 4.0

**Integritet.** Fältloggen skickar inga loggar, positioner eller rapporter någonstans. Med WiFi kontaktar
den bara GitHub (versionskoll, om uppdateringar är på) och EOX (kartbilder – EOX ser då vilket område
bilderna gäller). Feedback skickar du själv via telefonen.

*Till den som programmerar: ändras en funktion ska manualen ändras i samma version, på båda språken.*
