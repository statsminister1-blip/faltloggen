// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  ui.h – ritar Fältloggens alla sidor (800 x 480)
//
//  Allt ritas från en UiState-struktur till ett Arduino_GFX-objekt (i praktiken
//  en bildbuffert som kopieras till skärmen i ett svep, så att inget flimrar).
//  Varje knapp som ritas registreras med sin yta och sitt id, så att
//  pekhanteringen i huvudprogrammet vet vad som trycktes.
//
//  Sidor:  traktorsida (det traktorn skickar), redskapssida (värden från ISOBUS-redskapet),
//          TERMINAL (redskapets egen bild), karta, bränsleanalys, viloläge (loggan),
//          inställningar, MER (språk, uppdateringar, loggning, feedback, om), WiFi-lista,
//          tangentbord, sifferfält, ISOBUS-diagnos, manual, enhetsinfo, fältrapport.
//  Rutor:  "Ny mjukvara tillgänglig" (med ansvarstext) och "Uppdaterar programmet".
//  Alla texter som visas skrivs L("svenska", "English") (lang.h).
// ============================================================================
#pragma once
static const char *kBildText();   // definieras i karta.h (kartbildens källa och licens)
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "logo_data.h"
#include "lang.h"

#define CHART_POINTS 30
#define DIAG_ROWS    18
#define UI_SCAN_MAX  12

// Sidor
enum { PG_IDLE = 0, PG_SETTINGS, PG_WIFI, PG_KEYBOARD, PG_DIAG, PG_GENERIC, PG_TRAKTOR, PG_KARTA, PG_KARTINST, PG_VAL, PG_VT, PG_ANALYS,
       PG_SIFFROR, PG_EXPORT,     // v37: sifferfält för arbetsbredden, fältrapporten
       PG_MANUAL,                 // v42: manualen (innehåll och kapitel)
       PG_ENHET,                  // v44: enhetsinfo (temperatur, minne, SD, nät, buss)
       PG_MER,                    // fler inställningar: språk, uppdateringar, loggning, feedback, om
       PG_FEEDBACK };             // feedback: QR-kod till arkivets ärenden (GitHub Issues)

// Knapp-id
enum {
  B_NONE = 0,
  B_SETTINGS, B_CLOSE,
  B_BRI_MINUS, B_BRI_PLUS, B_W_MINUS, B_W_PLUS, B_W_MODE,
  B_WIFI, B_DIAG,
  B_WIFI_SCAN, B_WIFI_BACK, B_WIFI_FORGET,
  B_NET0,                         // B_NET0 + index i nätlistan
  B_KB_SHIFT = B_NET0 + 20, B_KB_MODE, B_KB_BACK, B_KB_SPACE, B_KB_CANCEL, B_KB_OK,
  B_DIAG_BACK,
  B_REDSKAP, B_GEN_BACK, B_GEN_PAGE, B_TRAKTOR, B_DEMO, B_TR_RAW,
  B_KARTA, B_K_BACK, B_K_ZIN, B_K_ZUT, B_K_Z100, B_K_FOLJ, B_K_START, B_K_INST, B_K_NOLLA, B_K_NOLLA_OK, B_K_NOLLA_NO,
  B_KI_TRIG_M, B_KI_TRIG_P, B_KI_S_M, B_KI_S_P, B_KI_P_M, B_KI_P_P, B_KI_WMODE, B_KI_W_M, B_KI_W_P, B_KI_KLAR,
  B_VAL_TRAKTOR, B_VAL_ISOBUS,
  B_VAL_VT, B_VT_OPEN, B_VT_BACK,
  B_TR_ANALYS, B_AN_BACK, B_AN_L0, B_AN_L1, B_AN_L2, B_AN_PRIS, B_AN_D_M, B_AN_D_P, B_AN_T_M, B_AN_T_P, B_AN_PRIS_OK,
  B_NATT,                         // dag/natt (S4-stil)
  B_TR_INFO0, B_TR_INFO1, B_TR_INFO2,   // traktorsidans Info⁺-ruta: lyft, ventiler, rådata
  B_OTA_JA, B_OTA_NEJ,            // frågan "Ny mjukvara tillgänglig" (v35)
  B_K_FARG_B, B_K_FARG_D,         // kartans färg: bränsle / dragkraft (v36)
  B_K_FARG_S,                     // kartans färg: slirning (v41)
  B_HEM,                          // huset: startvalet (v37)
  B_K_GARDIN,                     // kartans informationsrutor in/ut (v37)
  B_KI_BG_SAT, B_KI_BG_ENF,       // kartans bakgrund: satellit / enfärgad (v37)
  B_LOGG_PA, B_LOGG_AV,           // loggning PÅ/AV (v37, 10.5)
  B_W_NUM, B_KI_W_NUM,            // tryck på arbetsbredden = sifferfält i cm (v37)
  B_NUM_DEL, B_NUM_OK, B_NUM_CANCEL, B_NUM0,   // sifferfältet; B_NUM0 + siffra
  B_NUM9 = B_NUM0 + 9,
  B_NUM_DOT,                      // decimaltecknet i sifferfältet för skiftets areal (v45)
  B_K_SKIFT,                      // skiftets areal på kartan (v45, 10.19)
  B_K_EXPORT, B_EXP_STANG,        // fältrapporten (v37, 10.6)
  B_MAN_OPEN, B_MAN_BACK, B_MAN_INNEH, B_MAN_FORRA, B_MAN_NASTA,   // manualen (v42, 10.12)
  B_MAN_KAP0,                     // B_MAN_KAP0 + kapitel i innehållsförteckningen
  B_MAN_KAP_END = B_MAN_KAP0 + 24,
  B_ENH_OPEN, B_ENH_BACK,         // enhetsinfo (v44)
  B_MER_OPEN, B_MER_BACK,         // fler inställningar
  B_SPRAK_SV, B_SPRAK_EN,         // språk
  B_UPPD_PA, B_UPPD_AV, B_OTA_AV, // uppdateringar på/av (B_OTA_AV = "stäng av" i uppdateringsfrågan)
  B_FB_OPEN, B_FB_BACK,           // feedback
  B_OM_OPEN,                      // om Fältloggen: ansvar, licenser (öppnar manualens sista kapitel)
  B_KEY0 = 200                   // B_KEY0 + rad*16 + kolumn
};

static_assert(B_NUM9 < B_KEY0, "knapp-id:n räcker inte");
static_assert(B_MAN_KAP_END < B_KEY0, "knapp-id:n räcker inte (manualen)");
static_assert(B_OM_OPEN < B_KEY0, "knapp-id:n räcker inte");

struct DiagRow {
  uint8_t  sa, da;
  uint16_t element;
  uint16_t ddi;
  uint8_t  cmd;
  int32_t  value;
  uint32_t count;
  uint32_t lastMs;
};

// En rad på redskapssidan (färdigformaterad)
#define GEN_ROWS     48
#define GEN_PER_PAGE 20
struct GenRow {
  char namn[64];
  char val[20];
  char enh[16];
  bool farsk;           // uppdaterad de senaste 10 s
};

// Belysning (bitmask i TraktorData.ljus / TrView.ljus)
enum {
  LJ_POS = 1 << 0,        // positionsljus / parkeringsljus
  LJ_HALV = 1 << 1,       // halvljus
  LJ_HEL = 1 << 2,        // helljus
  LJ_ARB_FRAM = 1 << 3,   // arbetsljus fram (låga eller höga)
  LJ_ARB_BAK = 1 << 4,    // arbetsljus bak (låga eller höga)
  LJ_ARB_SIDA = 1 << 5,   // arbetsljus på sidorna
  LJ_BROMS = 1 << 6,      // bromsljus
  LJ_BLINK_V = 1 << 7,    // blinkers vänster
  LJ_BLINK_H = 1 << 8,    // blinkers höger
  LJ_ROTOR = 1 << 9,      // roterande varningsljus
  LJ_BACK = 1 << 10,      // backljus
};

// Traktorns värden (NAN / -1 = saknas)
#define TR_UI_ROWS 8
#define TR_VALVES  8
struct TrView {
  float  kmh, rpm, ptoF, ptoR, liftF, liftR, fuel, hours, km, cog;
  float  dragKN;               // dragkraft i bakre lyften (kN), + = drar bakåt (v36)
  float  slir;                 // slirning (%) = (hjulfart - markfart) / hjulfart (v41)
  float  lutFram, lutSida;     // lutning fram/bak och åt sidan (grader) från styrsystemet (v36)
  bool   ljusOk;               // belysningen hörs (0xFE40)
  uint16_t ljus;               // tända lampor (LJ_*)
  int    workF, workR;         // lyft i arbete 1/0, -1 okänt
  int    dir;                  // 1 framåt, 0 back, -1 okänt
  bool   gps, gpsFresh;
  double lat, lon;
  int    nValve;
  int8_t valveNo[TR_VALVES];
  float  valveFlow[TR_VALVES]; // + ut, - in (%)
};

// Bränsleanalys (fylls av analys.h), senaste 30 minuterna
#define AN_VBINS 30
#define AN_REK   5
struct AnView {
  bool  ok, workFromMap, ptoOn;
  int   sekOk, sekWork, sekIdle, sekVand;   // sekVand = vändningar < 30 s som räknas som arbete (v37)
  float diesel, timKr;                     // priser som användes (kr/l, kr/h)
  int   lage;                              // 0 ekonomi, 1 balans, 2 bråttom
  float avgKmh, avgLh, avgLha, avgKrHa, avgW, haPerH, idleL, totL, ptoRpm;
  float vandL;                             // liter under vändningarna (v37)
  int   nBin, nGood, best, bestL;          // best = lägst kr/ha, bestL = lägst l/ha (index i bin*, -1 = inget)
  int   binLo[AN_VBINS], binSek[AN_VBINS];
  float binLh[AN_VBINS], binLha[AN_VBINS], binKrHa[AN_VBINS], binHaH[AN_VBINS];
  float rpmLo, rpmHi, rpmGain;
  int   nRek;
  char  rek[AN_REK][128];
};

// Kartan (fylls av karta.h)
struct KartaView {
  bool   logging, hasPos, gps, widthFromBus, cfgAutoW, outside;
  bool   panned;           // helskärmskartan är flyttad (följer inte traktorn)
  float  areaHa, distKm, widthM, cfgWidthM;
  float  fuelL, lPerHa, fuelNow;   // bränsle under loggning (l), snitt (l/ha), just nu (l/h)
  int    fargLage;                 // spårets färg: 0 = bränsle (l/ha), 1 = dragkraft (kN) (v36), 2 = slirning (%) (v41)
  int    bakgrund;                 // 0 = satellitbild, 1 = enfärgad som S4-styrbilden (v37)
  bool   fargHar;                  // det finns färgade rutor i valt läge
  float  fargSnitt, fargSpann;     // körningens snitt (= blått) och ±30 % (= helt grönt / rött)
  int    trig;
  char   trigTxt[32], trigKort[16], startTxt[16], stopTxt[16], tileTxt[72];
  double lat, lon;
  float  skiftHa, skiftKvarHa, skiftTakt;   // v45 (10.19): skiftets areal, kvar, takt (ha/h)
  int    skiftMin;                          // återstående minuter, -1 = ännu ingen takt
  char   skiftKlass[8];                     // beräknat klockslag "HH:MM", tomt = ingen klocka
  bool   skiftKlart;
};
// Kartan ritas av karta.h (sätts där): rutan x,y,w,h med mpp meter per bildpunkt
// pan = true: helskärmskartan, flyttad med fingret (minikartan följer alltid traktorn)
static void (*uiKartaRender)(Arduino_GFX *, int, int, int, int, float, bool) = nullptr;
// Redskapets egen bild ritas av vtview.h: masken i rutan (x, y, storlek), mjukknapparna i rutan (kx, ky, kw, kh), kw = 0 = inga
static void (*uiVtRender)(Arduino_GFX *, int, int, int, int, int, int, int) = nullptr;
// Zoom: meter per bildpunkt. 100 m-knappen = 100 m framför och bakom traktorn på skärmens höjd.
#define K_MPP_MIN  0.25f
#define K_MPP_MAX  32.0f
#define K_MPP_100M (200.0f / 440.0f)

// Fältrapporten (v37, 10.6) – fylls av falt.h
struct ExpView {
  int    status;          // 0 = inget, 1 = skapar, 2 = klar (WiFi på), 3 = fel
  char   namn[40];        // fältets namn (frivilligt)
  char   fil[64];         // filnamnet på SD-kortet (utan /RAPPORT/)
  char   ap[33];          // skärmens WiFi-nät
  char   fel[80];
  char   rad[8][72];      // sammanfattningen
  int    nRad;
  int    stationer;       // anslutna telefoner
  int    hamtade;         // antal nedladdningar
};

// Kopia av nätstatus (fylls av huvudprogrammet)
struct NetView {
  bool     connected;
  char     ssid[33];
  char     ip[16];
  int      rssi;
  bool     hasSaved;
  char     msg[64];
  bool     scanning;
  int      nScan;
  char     scanSsid[UI_SCAN_MAX][33];
  int8_t   scanRssi[UI_SCAN_MAX];
  bool     scanSecure[UI_SCAN_MAX];
  bool     otaActive;
  int      otaPct;
  int      latestVersion;
  int      otaOffer;       // > 0 = fråga om uppdatering till denna version (v35)
  char     otaNote[48];    // uppdateringens läge
  bool     uppd;           // leta efter uppdateringar (fråga alltid innan installation)
  char     fbUrl[96];      // feedback: arkivets ärenden (GitHub Issues)
  char     devId[16];
  char     namn[33];       // skärmens namn (namn= i CONFIG.TXT), tomt = inget
  bool     sdOk;
};

// v44: enhetsinfo – fylls av Faltloggen.ino (enhFill) ungefär en gång per sekund
struct EnhView {
  float    tempC, tempMin, tempMax;          // chippets temperatur (°C), NAN = okänd
  uint32_t psFreeKB, psTotKB, psBigKB;       // PSRAM
  uint32_t inFreeKB, inTotKB, inMinKB, inBigKB;   // internt minne (minst ledigt sedan start, största bit)
  bool     sdKand;                           // SD-kortets storlek är uppmätt
  uint32_t sdTotMB, sdFriMB;
  uint32_t upSek, bootNo;
  char     omstart[96];
  char     part[17];                         // programplats (ota_0 / ota_1)
  uint32_t ramarS, ramarTot;                 // ISOBUS-ramar per sekund / sedan start
  int      canState;                         // 0 stoppad, 1 igång, 2 felläge, 3 återhämtar, -1 okänt
  uint32_t canMiss, canOverrun, canBusErr, canRxErr;
  uint32_t ritN, ritSnitt, ritMax;           // ritningen sedan förra statusen (v43:s mätning)
};

struct UiState {
  int      page;
  // buss
  bool     busOk;          // meddelanden de senaste 2 s
  bool     tcSeen;
  float    kmh;            // NAN = saknas
  float    widthM;
  bool     widthFromBus;
  // knappar
  int      pressId;        // knapp som hålls nere just nu
  uint32_t tick;
  // inställningar
  uint8_t  lang;           // 0 = svenska, 1 = English (samma som uiLang)
  int      brightness;     // 20..100 %
  bool     natt;           // nattläge (mörka färger, S4-stil)
  float    manualWidth;    // m
  bool     widthManualMode;
  int      version;
  // tangentbord
  char     kbSsid[33];
  char     kbText[65];
  int      kbMode;         // 0 små, 1 stora, 2 tecken
  // diagnos
  DiagRow  rows[DIAG_ROWS];
  int      nRows;
  uint32_t nowMs;
  uint32_t frames;
  int      tcAddr;
  // redskap (redskapssidan)
  bool     genHasImpl;     // något redskap hittat
  int      autoPage;       // sidan som bussen ger (PG_GENERIC eller PG_TRAKTOR)
  bool     demo;           // demoläge: simulerade värden
  bool     genManual;      // redskaps- eller traktorsidan öppnad från inställningarna
  bool     genDdop;        // namn/enheter från redskapets beskrivning
  char     genTitel[48];
  char     genSub[128];
  char     genWidth[16];   // arbetsbredd från redskapet
  char     genWork[8];     // arbetsläge PÅ/AV/--
  GenRow   gen[GEN_ROWS];
  int      nGen;
  int      genPage;
  // traktor
  TrView   trv;
  bool     trRawView;      // diagnossidan visar traktorns rådata i stället för processdata (v37)
  int      trInfo;         // Info⁺-rutan: 0 trepunktslyft, 1 hydraulventiler, 2 rådata
  float    trFuelMin[CHART_POINTS];   // bränsle l/min, ett minutmedel per punkt (NAN = ingen data), äldst först
  AnView   an;             // bränsleanalysen
  float    anDiesel;       // dieselpris kr/l
  float    anTimKr;        // traktor + förare kr/h
  int      anLage;         // 0 EKONOMI, 1 BALANS, 2 BRÅTTOM
  bool     anPris;         // prisrutan visas
  // karta
  KartaView karta;
  float    kartaMpp;       // kartans zoom (meter per bildpunkt)
  bool     kartaConfirm;   // "Nolla kartan?" visas
  bool     kartaGardin;    // helskärmskartans rutor Areal och Färg på spåret infällda (v37)
  // startval (traktor eller ISOBUS)
  int      startVal;       // 0 = inte frågat, 1 = frågar nu, 2 = klart
  bool     valHem;         // startvalet öppnat med huset: ingen nedräkning (v37)
  bool     loggAv;         // loggning AV (v37, 10.5)
  int      numFor;         // sifferfältet: 1 = inställningarnas bredd, 2 = kartans bredd (v37), 3 = skiftets areal (v45)
  int      numBack;        // sidan som sifferfältet återgår till
  char     numText[8];     // inskrivna cm
  int      setBack;        // v38: sidan man kom från till inställningarna (-1 = ingen), bocken går tillbaka dit
  int      manKap;         // v42: manualens kapitel (-1 = innehållsförteckningen)
  int      manSida;        // v42: sida i kapitlet
  int      manBack;        // sidan som manualens tillbaka-knapp går till (inställningar eller MER)
  int      kbFor;          // tangentbordet: 0 = WiFi-lösenord, 2 = fältets namn (v37)
  ExpView  exp;            // fältrapporten (v37)
  int      valSecs;        // sekunder kvar innan ISOBUS väljs själv
  char     trRaw[TR_UI_ROWS][64];
  int      nTrRaw;
  // redskapets egen bild (spegling av terminalen, vtview.h)
  bool     vtAvail;        // en pool för redskapet finns inläst
  bool     vtLive;         // redskapet har skickat kommandon till terminalen de senaste 3 s
  // nät
  NetView  net;
  EnhView  enh;           // v44: enhetsinfo
};

// ---------------------------------------------------------------- Färger (RGB565)
#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))
// Färgerna sätts av uiTema() (S4-stil, dag eller natt) – se avsnittet S4-STIL längre ner.
static uint16_t UI_BG;
static uint16_t UI_PANEL;
static uint16_t UI_TEXT;
static uint16_t UI_DIM;
static uint16_t UI_FAINT;
static uint16_t UI_GRID;
static uint16_t UI_GRID2;
static uint16_t UI_AMBER;
static uint16_t UI_AMBERLT;
static uint16_t UI_GREEN;
static uint16_t UI_GREENLT;
static uint16_t UI_GREENBG;
static uint16_t UI_GREENBD;
static uint16_t UI_GREYBG;
static uint16_t UI_GREYBD;
static uint16_t UI_SHADE;
static uint16_t UI_SHADEBD;
static uint16_t UI_RED;
static uint16_t UI_REDBG;
static uint16_t UI_FGREEN;
static uint16_t UI_FGREENDK;
static uint16_t UI_FGREENHL;
static uint16_t UI_TYRE;
static uint16_t UI_RIM;
static uint16_t UI_GLASS;
static uint16_t UI_STEEL;

// ---------------------------------------------------------------- Knappregister
struct UiBtn { int16_t x, y, w, h; uint16_t id; };
#define UI_MAX_BTN 96
static UiBtn uiBtns[UI_MAX_BTN];
static int uiNBtn = 0;

static void uiReg(int x, int y, int w, int h, int id) {
  if (uiNBtn < UI_MAX_BTN) uiBtns[uiNBtn++] = {(int16_t)x, (int16_t)y, (int16_t)w, (int16_t)h, (uint16_t)id};
}
// Vilken knapp ligger under (x,y)? (lite generös marginal)
static int uiHit(int x, int y) {
  for (int i = uiNBtn - 1; i >= 0; i--) {
    const UiBtn &b = uiBtns[i];
    if (x >= b.x - 6 && x < b.x + b.w + 6 && y >= b.y - 6 && y < b.y + b.h + 6) return b.id;
  }
  return B_NONE;
}

// ---------------------------------------------------------------- Hjälpfunktioner
static int uiTextWidth(Arduino_GFX *g, const char *s, const uint8_t *font) {
  // Mät bredden genom att "skriva" utanför skärmen och läsa av markören
  // (getTextBounds hanterar inte å/ä/ö rätt).
  g->setFont(font);
  g->setCursor(0, -2000);
  g->print(s);
  return g->getCursorX();
}
static void uiText(Arduino_GFX *g, int x, int y, const char *s, uint16_t col, const uint8_t *font) {
  g->setFont(font);
  g->setTextColor(col);
  g->setCursor(x, y);
  g->print(s);
}
static void uiTextRight(Arduino_GFX *g, int xr, int y, const char *s, uint16_t col, const uint8_t *font) {
  uiText(g, xr - uiTextWidth(g, s, font), y, s, col, font);
}
static void uiTextCenter(Arduino_GFX *g, int xc, int y, const char *s, uint16_t col, const uint8_t *font) {
  uiText(g, xc - uiTextWidth(g, s, font) / 2, y, s, col, font);
}
// Text som kortas med ".." om den inte får plats på maxW bildpunkter
static void uiTextFit(Arduino_GFX *g, int x, int y, const char *s, int maxW, uint16_t col, const uint8_t *font) {
  char b[128];
  strncpy(b, s, sizeof(b) - 3); b[sizeof(b) - 3] = 0;
  if (uiTextWidth(g, b, font) <= maxW) { uiText(g, x, y, b, col, font); return; }
  int n = strlen(b);
  while (n > 0) {
    n--;
    while (n > 0 && (b[n] & 0xC0) == 0x80) n--;      // hela tecken (å/ä/ö)
    b[n] = 0; strcat(b, "..");
    if (uiTextWidth(g, b, font) <= maxW) break;
    b[n] = 0;
  }
  uiText(g, x, y, b, col, font);
}
// Bara siffror (kan ritas med de stora siffertypsnitten)?
static bool uiNumeric(const char *s) {
  for (; *s; s++) if (!((*s >= '0' && *s <= '9') || *s == ',' || *s == '-' || *s == '.')) return false;
  return true;
}

// Tal med decimaltecken efter språket (komma på svenska, punkt på engelska)
static void uiFmt(char *buf, size_t n, double v, int dec) {
  if (isnan(v)) { snprintf(buf, n, "--"); return; }
  if (dec <= 0) { snprintf(buf, n, "%ld", (long)lround(v)); return; }
  snprintf(buf, n, "%.*f", dec, v);
  langDec(buf);
}
// Ritar en 4-bitars alfamask (loggan) i färgen "tint" ovanpå en enfärgad bakgrund "bg"
static void uiLogo(Arduino_GFX *g, int x0, int y0, int w, int h, const uint8_t *data,
                   uint16_t tint, uint16_t bg) {
  uint16_t pal[16];
  int tr = (tint >> 11) & 31, tg = (tint >> 5) & 63, tb = tint & 31;
  int br = (bg >> 11) & 31, bgc = (bg >> 5) & 63, bb = bg & 31;
  for (int i = 0; i < 16; i++) {
    int r = br + (tr - br) * i / 15, gg = bgc + (tg - bgc) * i / 15, b = bb + (tb - bb) * i / 15;
    pal[i] = (uint16_t)((r << 11) | (gg << 5) | b);
  }
  int stride = (w + 1) / 2;
  for (int yy = 0; yy < h; yy++) {
    const uint8_t *row = data + yy * stride;
    for (int xx = 0; xx < w; xx++) {
      uint8_t v = row[xx >> 1];
      uint8_t a = (xx & 1) ? (v & 0x0F) : (v >> 4);
      if (a) g->drawPixel(x0 + xx, y0 + yy, pal[a]);
    }
  }
}

// Kugghjul (inställningar)
static void uiGear(Arduino_GFX *g, int cx, int cy, uint16_t col, uint16_t bg) {
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    int tx = cx + (int)lroundf(cosf(a) * 11), ty = cy + (int)lroundf(sinf(a) * 11);
    g->fillCircle(tx, ty, 3, col);
  }
  g->fillCircle(cx, cy, 10, col);
  g->fillCircle(cx, cy, 4, bg);
}

// WiFi-symbol: tre bågar och en prick. Utan nät: dämpad med ett rött snedstreck.
static void uiWifiIcon(Arduino_GFX *g, int cx, int by, bool online) {
  uint16_t c = online ? UI_DIM : UI_FAINT;
  g->fillCircle(cx, by - 2, 2, c);
  g->fillArc(cx, by, 8, 6, 225, 315, c);
  g->fillArc(cx, by, 13, 11, 225, 315, c);
  g->fillArc(cx, by, 18, 16, 225, 315, c);
  if (!online) {
    for (int t = -1; t <= 1; t++) g->drawLine(cx - 11 + t, by - 18, cx + 11 + t, by + 1, UI_RED);
  }
}

// WiFi-staplar
static void uiBars(Arduino_GFX *g, int x, int y, int rssi, uint16_t col) {
  int lvl = rssi > -55 ? 4 : rssi > -65 ? 3 : rssi > -75 ? 2 : 1;
  for (int i = 0; i < 4; i++) {
    int h = 6 + i * 5;
    if (i < lvl) g->fillRect(x + i * 8, y - h, 5, h, col);
    else g->drawRect(x + i * 8, y - h, 5, h, UI_GRID2);
  }
}

// Båge: vinklar i grader, 0 = höger, 90 = nedåt (medurs). a1 får vara > 360.
static void uiArc(Arduino_GFX *g, int cx, int cy, int r1, int r2, float a0, float a1, uint16_t c) {
  if (a1 - a0 < 0.5f) return;
  if (a1 > 360.0f) {
    if (a0 < 360.0f) g->fillArc(cx, cy, r1, r2, a0, 360.0f, c);
    g->fillArc(cx, cy, r1, r2, a0 < 360.0f ? 0.0f : a0 - 360.0f, a1 - 360.0f, c);
  } else g->fillArc(cx, cy, r1, r2, a0, a1, c);
}
// Tjock linje med runda ändar
static void uiThick(Arduino_GFX *g, int x0, int y0, int x1, int y1, int w, uint16_t c) {
  float dx = x1 - x0, dy = y1 - y0, L = sqrtf(dx * dx + dy * dy);
  if (L < 1) { g->fillCircle(x0, y0, w / 2, c); return; }
  float nx = -dy / L * w / 2.0f, ny = dx / L * w / 2.0f;
  g->fillTriangle(x0 + nx, y0 + ny, x1 + nx, y1 + ny, x1 - nx, y1 - ny, c);
  g->fillTriangle(x0 + nx, y0 + ny, x1 - nx, y1 - ny, x0 - nx, y0 - ny, c);
  g->fillCircle(x0, y0, w / 2, c);
  g->fillCircle(x1, y1, w / 2, c);
}


// ================================================================ S4-STIL (Fendt Vario Terminal S4)
// Utseendet följer S4-terminalens stil (Fendt 828/720 S4):
//  - mörk bakgrund, ljusa rutor med 2 px mörka glipor
//  - rubrikrad: "Info⁺" på ljusblå platta + rubrik på ljusblå list som tonar ut åt höger
//  - värdefält = vinkellinje (tunn linje ovanför och till vänster), värdet högerställt, enheten efter
//  - staplar = vinkellinje med blå fyllning (mörkblå = läge, ljusblå = övrigt) och procent
//  - knappar: ljusgrå med rundade hörn, stora mörka ikoner; vald knapp har röd ram
//  - vänsterkolumnen väljer vad Info⁺-rutan visar (som S4: Trepunktslyft, Hydraulventiler, ...)
//  - typsnitt DejaVu Sans (s4font.h)
//  Två färgscheman: DAG och NATT (knappen B_NATT).
#include "s4font.h"

struct S4Pal {
  uint16_t scr, pan, txt, dim, faint, lin, sep, hdr, hdr2, barM, barL, grn, grnTxt,
           keyHi, keyLo, keyBd, ico, sel, amb, mapTop, white;
};
static const S4Pal S4_DAG = {
  RGB(20, 22, 24),      // scr     bakgrund / glipor
  RGB(238, 240, 241),   // pan     rutorna
  RGB(14, 14, 14),      // txt
  RGB(64, 68, 72),      // dim
  RGB(140, 144, 148),   // faint
  RGB(54, 56, 58),      // lin     vinkellinjer
  RGB(178, 182, 186),   // sep     skiljelinjer
  RGB(196, 220, 240),   // hdr     ljusblå list
  RGB(164, 200, 228),   // hdr2    "Info⁺"-plattan
  RGB(0, 0, 196),       // barM    mörkblå stapel
  RGB(0, 138, 230),     // barL    ljusblå stapel
  RGB(36, 150, 40),     // grn
  RGB(255, 255, 255),   // grnTxt
  RGB(240, 241, 242),   // keyHi
  RGB(206, 208, 210),   // keyLo
  RGB(132, 136, 140),   // keyBd
  RGB(46, 48, 50),      // ico     ikonernas färg
  RGB(232, 28, 24),     // sel     vald knapp
  RGB(236, 150, 16),    // amb
  RGB(18, 18, 18),      // mapTop  kartans svarta list
  RGB(255, 255, 255),   // white
};
static const S4Pal S4_NATT = {
  RGB(0, 0, 0),
  RGB(34, 36, 38),
  RGB(214, 216, 218),
  RGB(160, 164, 168),
  RGB(100, 104, 108),
  RGB(150, 154, 158),
  RGB(70, 74, 78),
  RGB(34, 62, 96),
  RGB(44, 80, 122),
  RGB(52, 74, 236),
  RGB(28, 120, 210),
  RGB(32, 128, 36),
  RGB(236, 255, 236),
  RGB(76, 78, 81),
  RGB(50, 52, 55),
  RGB(18, 19, 20),
  RGB(206, 208, 210),
  RGB(214, 40, 32),
  RGB(214, 136, 16),
  RGB(0, 0, 0),
  RGB(10, 11, 12),
};
static S4Pal P = S4_DAG;


#define S4F_S  u8g2_font_dv13_tf       // små texter
#define S4F_T  u8g2_font_dv15_tf       // rubriker, enheter
#define S4F_TB u8g2_font_dv15b_tf      // "Info⁺"
#define S4F_V  u8g2_font_dv20_tf       // värden
#define S4F_M  u8g2_font_dv17_tf       // mellanstora värden
#define S4F_MB u8g2_font_dv17b_tf

static uint16_t uiMix(uint16_t a, uint16_t b, int t, int n) {
  int ar = (a >> 11) & 31, ag = (a >> 5) & 63, ab = a & 31;
  int br = (b >> 11) & 31, bg = (b >> 5) & 63, bb = b & 31;
  return (uint16_t)(((ar + (br - ar) * t / n) << 11) | ((ag + (bg - ag) * t / n) << 5) | (ab + (bb - ab) * t / n));
}
// De gamla färgnamnen (UI_*) följer S4-paletten, så att all kod ritar i samma stil
static void uiTema(bool natt) {
  P = natt ? S4_NATT : S4_DAG;
  UI_BG = P.pan;  UI_PANEL = P.pan;  UI_TEXT = P.txt;  UI_DIM = P.dim;  UI_FAINT = P.faint;
  UI_GRID = P.sep;  UI_GRID2 = P.lin;
  UI_AMBER = P.amb;  UI_AMBERLT = natt ? RGB(240, 180, 90) : RGB(150, 90, 0);
  UI_GREEN = P.grn;  UI_GREENLT = natt ? RGB(120, 220, 120) : RGB(20, 120, 30);
  UI_GREENBG = P.grn;  UI_GREENBD = P.grn;
  UI_GREYBG = P.keyLo;  UI_GREYBD = P.keyBd;
  UI_SHADE = uiMix(P.pan, P.amb, 1, 4);  UI_SHADEBD = P.amb;
  UI_RED = P.sel;  UI_REDBG = uiMix(P.pan, P.sel, 1, 5);
  UI_FGREEN = P.grn;  UI_FGREENDK = uiMix(P.grn, P.pan, 1, 2);  UI_FGREENHL = P.grn;
  UI_TYRE = P.txt;  UI_RIM = P.faint;  UI_GLASS = P.hdr;  UI_STEEL = P.lin;
}
static struct UiTemaStart { UiTemaStart() { uiTema(false); } } uiTemaStart;   // färger redan före första ritningen

static void s4VGrad(Arduino_GFX *g, int x, int y, int w, int h, uint16_t c0, uint16_t c1) {
  for (int i = 0; i < h; i++) g->drawFastHLine(x, y + i, w, uiMix(c0, c1, i, h > 1 ? h - 1 : 1));
}
static void s4HGrad(Arduino_GFX *g, int x, int y, int w, int h, uint16_t c0, uint16_t c1) {
  for (int i = 0; i < w; i++) g->drawFastVLine(x + i, y, h, uiMix(c0, c1, i, w > 1 ? w - 1 : 1));
}
// Fylld polygon (godtycklig form, jämn/udda-regeln), punkter som x0,y0,x1,y1,...
static void s4Poly(Arduino_GFX *g, const float *p, int n, uint16_t c) {
  float y0 = 1e9f, y1 = -1e9f;
  for (int i = 0; i < n; i++) { y0 = fminf(y0, p[2 * i + 1]); y1 = fmaxf(y1, p[2 * i + 1]); }
  for (int y = (int)ceilf(y0); y <= (int)floorf(y1); y++) {
    float xs[24]; int k = 0;
    float yc = y + 0.01f;
    for (int i = 0; i < n && k < 24; i++) {
      float ax = p[2 * i], ay = p[2 * i + 1], bx = p[2 * ((i + 1) % n)], by = p[2 * ((i + 1) % n) + 1];
      if ((ay <= yc && by > yc) || (by <= yc && ay > yc)) xs[k++] = ax + (yc - ay) * (bx - ax) / (by - ay);
    }
    for (int a = 1; a < k; a++) for (int b = a; b > 0 && xs[b] < xs[b - 1]; b--) { float t = xs[b]; xs[b] = xs[b - 1]; xs[b - 1] = t; }
    for (int a = 0; a + 1 < k; a += 2) {
      int xa = (int)lroundf(xs[a]), xb = (int)lroundf(xs[a + 1]);
      if (xb >= xa) g->drawFastHLine(xa, y, xb - xa + 1, c);
    }
  }
}

// ---------------------------------------------------------------- Byggstenar
// Ruta (ljus) med 2 px mörk glipa runt (skärmens bakgrund syns mellan rutorna)
static void s4Box(Arduino_GFX *g, int x, int y, int w, int h) { g->fillRect(x, y, w, h, P.pan); }

// Rubrikrad som i S4: [Info⁺] Rubrik på ljusblå list. icon != null: ikon i stället för "Info⁺".
typedef void (*S4Icon)(Arduino_GFX *, int, int, int, uint16_t);       // (cx, cy, storlek, färg)
#define S4_HDR 25
static void s4Head(Arduino_GFX *g, int x, int y, int w, const char *title, bool info, S4Icon icon = nullptr) {
  int tx = x + 6;
  int bandEnd = x + w - 4;
  int fade = w / 3;
  g->fillRect(x + 4, y + 3, bandEnd - (x + 4) - fade, S4_HDR - 4, P.hdr);
  s4HGrad(g, bandEnd - fade, y + 3, fade, S4_HDR - 4, P.hdr, P.pan);
  if (info) {
    int iw = uiTextWidth(g, "Info⁺", S4F_TB);
    g->fillRect(x + 4, y + 3, iw + 6, S4_HDR - 4, P.hdr2);
    uiText(g, x + 7, y + 19, "Info⁺", P.txt, S4F_TB);
    tx = x + 4 + iw + 12;
  } else if (icon) {
    g->fillRect(x + 4, y + 3, 34, S4_HDR - 4, P.hdr2);
    icon(g, x + 21, y + 13, 26, P.txt);
    tx = x + 44;
  }
  uiText(g, tx, y + 19, title, P.txt, S4F_T);
}

// Vinkellinje: tunn linje ovanför och till vänster (S4:s värdefält)
static void s4L(Arduino_GFX *g, int x, int y, int w, int h) {
  g->drawFastHLine(x, y, w, P.lin);
  g->drawFastVLine(x, y, h, P.lin);
}
// Värdefält: vinkellinje, värdet högerställt. bg != 0: fylld (t.ex. grön)
static void s4Field(Arduino_GFX *g, int x, int y, int w, int h, const char *val, uint16_t fg,
                    const uint8_t *font = S4F_V, uint16_t bg = 0) {
  if (bg) g->fillRect(x + 2, y + 2, w - 2, h - 3, bg);
  s4L(g, x, y, w, h);
  int asc = font == S4F_V ? 15 : (font == S4F_M || font == S4F_MB ? 12 : 11);
  uiTextRight(g, x + w - 5, y + h / 2 + asc / 2 + 1, val, fg, font);
}
// Stapel som i S4: vinkellinje, blå fyllning, procenten mitt i (vit där stapeln ligger under)
static void s4Bar(Arduino_GFX *g, int x, int y, int w, int h, float pct, const char *txt, bool mork) {
  s4L(g, x, y, w, h);
  int bw = 0;
  if (!isnan(pct)) {
    float f = pct / 100.0f; if (f < 0) f = 0; if (f > 1) f = 1;
    bw = (int)lroundf((w - 3) * f);
    if (bw > 0) g->fillRect(x + 3, y + 3, bw, h - 5, mork ? P.barM : P.barL);
  }
  int tw = uiTextWidth(g, txt, S4F_M);
  int tx = x + 3 + (w - 3 - tw) / 2;
  uint16_t c = isnan(pct) ? P.faint : P.txt;
  if (!isnan(pct) && x + 3 + bw > tx + tw / 2) c = RGB(255, 255, 255);
  uiText(g, tx, y + h / 2 + 7, txt, c, S4F_M);
}
// Skiljelinje mellan raderna
static void s4Sep(Arduino_GFX *g, int x, int y, int w) { g->drawFastHLine(x + 6, y, w - 12, P.sep); }

// Knapp som i S4: ljusgrå, rundade hörn, mörk ikon. Vald = röd ram.
static void s4Key(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h, int id, bool sel,
                  S4Icon icon, int isz, const char *info = nullptr, const char *lab = nullptr) {
  bool down = s.pressId == id && id != B_NONE;
  if (sel) g->fillRoundRect(x - 3, y - 3, w + 6, h + 6, 9, P.sel);
  g->fillRoundRect(x, y, w, h, 7, P.keyBd);
  g->fillRoundRect(x + 1, y + 1, w - 2, h - 2, 6, down ? P.keyLo : P.keyHi);
  s4VGrad(g, x + 4, y + 5, w - 8, h - 10, down ? P.keyLo : P.keyHi, down ? P.keyHi : P.keyLo);
  int cy = y + h / 2 + (down ? 1 : 0), cx = x + w / 2 + (down ? 1 : 0);
  if (info) {
    uiTextCenter(g, cx, y + 23, info, P.ico, S4F_TB);
    cy = y + h / 2 + 12;
  }
  if (lab) {
    uiTextCenter(g, cx, icon ? y + 22 : cy + 6, lab, P.ico, icon ? S4F_TB : S4F_MB);
    cy = y + h / 2 + 12;
  }
  if (icon) icon(g, cx, cy, isz, P.ico);
  if (id != B_NONE) uiReg(x, y, w, h, id);
}

// ---------------------------------------------------------------- Symboler (ritade efter S4)
// Traktorn ur S4-ikonen (fronten åt vänster). Ritad i ikonens egna mått 640 × 415 och skalad.
// (cx, cy) = mitten, w = bredd i bildpunkter, bg = bakgrund (vita ringar runt hjulen)
// rot = lutning i radianer (+ = fronten upp), vriden runt (cx, cy) (v36)
static void s4Traktor(Arduino_GFX *g, int cx, int cy, int w, uint16_t c, uint16_t bg, float rot = 0) {
  float k = w / 640.0f, cr = cosf(rot), sr = sinf(rot);
  auto PX = [&](float u, float v) { float dx = (u - 430) * k, dy = (v - 337) * k; return cx + dx * cr - dy * sr; };
  auto PY = [&](float u, float v) { float dx = (u - 430) * k, dy = (v - 337) * k; return cy + dx * sr + dy * cr; };
  auto poly = [&](const float *uv, int n) {
    float q[24];
    for (int i = 0; i < n; i++) { q[2 * i] = PX(uv[2 * i], uv[2 * i + 1]); q[2 * i + 1] = PY(uv[2 * i], uv[2 * i + 1]); }
    s4Poly(g, q, n, c);
  };
  static const float hood[] = {135, 292, 405, 238, 428, 238, 428, 332, 472, 332, 472, 446, 362, 450, 330, 420, 140, 346};
  poly(hood, 9);
  static const float tak[] = {400, 128, 606, 126, 608, 150, 402, 152};
  poly(tak, 4);
  static const float a[] = {404, 140, 428, 140, 428, 334, 404, 334};
  poly(a, 4);
  static const float b[] = {588, 138, 608, 138, 630, 256, 608, 256};
  poly(b, 4);
  int rR = (int)lroundf(135 * k), fR = (int)lroundf(105 * k), gap = max(1, (int)lroundf(13 * k));
  int rx = (int)lroundf(PX(610, 410)), ry = (int)lroundf(PY(610, 410)), fx = (int)lroundf(PX(215, 425)), fy = (int)lroundf(PY(215, 425));
  float rd = rot * 57.29578f;
  g->fillArc(rx, ry, rR + gap + max(2, (int)lroundf(20 * k)), rR + gap, 200 + rd, 332 + rd, c);   // stänkskärm bak
  g->fillCircle(rx, ry, rR + gap, bg);
  g->fillCircle(fx, fy, fR + gap, bg);
  g->fillCircle(rx, ry, rR, c);
  g->fillCircle(fx, fy, fR, c);
}
// Punkt (dx, dy) från (cx, cy), vriden rot radianer som traktorsymbolen
static void s4Rot(int cx, int cy, float rot, float dx, float dy, int &x, int &y) {
  x = (int)lroundf(cx + dx * cosf(rot) - dy * sinf(rot));
  y = (int)lroundf(cy + dx * sinf(rot) + dy * cosf(rot));
}
// Belysningen på traktorsymbolen (v36): ljuskäglor och lampor. Samma mått och vridning som s4Traktor.
// Ritas FÖRE traktorn, så att traktorn ligger ovanpå käglornas början.
static void s4Ljus(Arduino_GFX *g, int cx, int cy, int w, float rot, uint16_t ljus, bool natt) {
  if (!ljus) return;
  float k = w / 640.0f;
  uint16_t kagla = natt ? RGB(150, 130, 60) : RGB(255, 224, 110);
  auto pt = [&](float u, float v, int &x, int &y) { s4Rot(cx, cy, rot, (u - 430) * k, (v - 337) * k, x, y); };
  // kägla från lampan (u,v): längd L och halv öppning sp (bildpunkter), riktning (dxr, dyr) i traktorns system
  auto kon = [&](float u, float v, float dxr, float dyr, float L, float sp) {
    int lx, ly; pt(u, v, lx, ly);
    float n = sqrtf(dxr * dxr + dyr * dyr); dxr /= n; dyr /= n;
    float ex = dxr * L, ey = dyr * L, px = -dyr * sp, py = dxr * sp;     // slut och tvärs (traktorns system)
    float ax = ex + px, ay = ey + py, bx = ex - px, by = ey - py;
    float cs = cosf(rot), sn = sinf(rot);
    float q[] = {(float)lx, (float)ly,
                 lx + ax * cs - ay * sn, ly + ax * sn + ay * cs,
                 lx + bx * cs - by * sn, ly + bx * sn + by * cs};
    s4Poly(g, q, 3, kagla);
  };
  auto prick = [&](float u, float v, int r, uint16_t c) { int x, y; pt(u, v, x, y); g->fillCircle(x, y, r, c); };
  if (ljus & LJ_HEL) kon(140, 300, -1, 0.02f, 36, 8);
  else if (ljus & LJ_HALV) kon(140, 305, -1, 0.22f, 26, 6);
  if (ljus & LJ_ARB_FRAM) kon(402, 140, -1, 0.55f, 30, 9);
  if (ljus & LJ_ARB_BAK) kon(606, 138, 1, 0.75f, 26, 8);
  if (ljus & LJ_BACK) kon(640, 330, 1, 0.1f, 14, 4);
  // lampor (prickar), ritade efter traktorn i s4LjusPrickar
}
static void s4LjusPrickar(Arduino_GFX *g, int cx, int cy, int w, float rot, uint16_t ljus, uint32_t tick) {
  float k = w / 640.0f;
  auto prick = [&](float u, float v, int r, uint16_t c) { int x, y; s4Rot(cx, cy, rot, (u - 430) * k, (v - 337) * k, x, y); g->fillCircle(x, y, r, c); };
  const uint16_t gul = RGB(255, 210, 60), orange = RGB(255, 140, 0), rod = RGB(230, 30, 20);
  // v37 (10.7): varningsljuset (saftblandaren) som en kupa mitt på hyttaket, alltid synlig – grå när den är av,
  // orange med strålar åt sidorna när den är på; strålarna byter sida vid varje omritning så att den ser ut att rotera
  {
    int x, y; s4Rot(cx, cy, rot, (505 - 430) * k, (124 - 337) * k, x, y);
    int r = max(3, (int)lroundf(20 * k));
    bool pa = ljus & LJ_ROTOR;
    uint16_t c = pa ? orange : RGB(150, 150, 150);
    g->fillArc(x, y, r, 0, 180, 360, c);                                  // kupan (övre halvan)
    g->drawFastHLine(x - r - 1, y, 2 * r + 3, P.txt);                    // foten mot taket
    if (pa) {
      int sid = (tick & 1) ? 1 : -1, L = r * 3;
      for (int i = -1; i <= 1; i++)
        uiThick(g, x + sid * (r + 1), y - r / 2, x + sid * (r + 1 + L), y - r / 2 + i * r, 2, orange);
    }
  }
  if (!ljus) return;
  if (ljus & (LJ_HALV | LJ_HEL)) prick(150, 300, 2, gul);
  if (ljus & LJ_ARB_FRAM) prick(408, 132, 2, gul);
  if (ljus & LJ_ARB_BAK) prick(600, 132, 2, gul);
  if (ljus & LJ_POS) { prick(160, 285, 1, RGB(255, 245, 200)); prick(620, 270, 1, rod); }
  if (ljus & LJ_BROMS) prick(624, 268, 3, rod);
  if (ljus & (LJ_BLINK_V | LJ_BLINK_H)) prick(418, 160, 3, orange);   // bussens bit blinkar själv
}
// Liten bakvy av traktorn (två bakhjul, hytt), vriden rot radianer (+ = höger sida ner) – sidolutningen (v36)
static float s4BakvyRot = 0;
static void icBakvy(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  float rot = s4BakvyRot, cs = cosf(rot), sn = sinf(rot), k = sz / 26.0f;
  auto rect = [&](float x0, float y0, float x1, float y1, uint16_t col) {
    float q[8] = {x0, y0, x1, y0, x1, y1, x0, y1};
    for (int i = 0; i < 4; i++) {
      float dx = q[2 * i] * k, dy = q[2 * i + 1] * k;
      q[2 * i] = cx + dx * cs - dy * sn; q[2 * i + 1] = cy + dx * sn + dy * cs;
    }
    s4Poly(g, q, 4, col);
  };
  // marken (vågrät referens) och traktorns markplan
  g->drawFastHLine(cx - sz * 7 / 10, cy + (int)lroundf(10 * k), sz * 14 / 10, P.faint);
  rect(-13, 1, -7, 10, c);          // vänster bakhjul
  rect(7, 1, 13, 10, c);            // höger bakhjul
  rect(-7, 2, 7, 6, c);             // bakaxel / kropp
  rect(-6, -10, 6, -8, c);          // hyttak
  rect(-6, -8, -4, 2, c);           // hyttstolpar
  rect(4, -8, 6, 2, c);
  rect(-15, 10, 15, 11, c);         // traktorns markplan (lutar med traktorn)
}
// Dragkraft: krok (lyftarmen) och pil bakåt som växer med kraften (0-50 kN), v36
#define TR_DRAG_GUL 35.0f            // kN: gult fält
#define TR_DRAG_ROD 50.0f            // kN: rött fält
static float s4DragKN = NAN;
static void icDrag(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  int x0 = cx - sz / 2;
  g->fillRect(x0, cy - sz / 3, 3, sz * 2 / 3, c);
  float f = isnan(s4DragKN) ? 0.5f : fminf(1.0f, fabsf(s4DragKN) / 50.0f);
  int L = 9 + (int)(f * (sz - 12));
  int xs = x0 + 4, xe = xs + L;
  g->fillRect(xs, cy - 1, L - 6, 3, c);
  g->fillTriangle(xe, cy, xe - 7, cy - 5, xe - 7, cy + 5, c);
}

// Slirning (v41): hjul med ekrar och fartstreck bakom; fältet gult från TR_SLIR_GUL och rött från TR_SLIR_ROD %
#define TR_SLIR_GUL 20.0f            // %: gult fält
#define TR_SLIR_ROD 30.0f            // %: rött fält
static void icSlir(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  int r = sz * 4 / 10, hx = cx + sz / 8;
  g->fillCircle(hx, cy, r, c);
  g->fillCircle(hx, cy, r - 3, P.pan);
  g->fillCircle(hx, cy, 2, c);
  for (int i = 0; i < 4; i++) {
    float a = i * 0.785398f;
    g->drawLine(hx - (int)lroundf(cosf(a) * (r - 3)), cy - (int)lroundf(sinf(a) * (r - 3)),
                hx + (int)lroundf(cosf(a) * (r - 3)), cy + (int)lroundf(sinf(a) * (r - 3)), c);
  }
  // fartstreck bakom hjulet (hjulet snurrar fortare än det rullar)
  int x0 = cx - sz / 2;
  g->fillRect(x0, cy + r - 2, sz / 4, 2, c);
  g->fillRect(x0 + 2, cy + r - 7, sz / 5, 2, c);
}

// Lyftarm som S4:s länk: vit kapsel med svart kant och två hål. (px,py) = inre änden (vid traktorn),
// dir -1 = fram (ut åt vänster), +1 = bak. Läget 0-100 % vrider armen från nedåt till uppåt.
static void s4Link(Arduino_GFX *g, int px, int py, int dir, float pos, int work, int L) {
  bool ok = !isnan(pos);
  float p = ok ? pos : 50.0f;
  if (p < 0) p = 0; if (p > 100) p = 100;
  float ang = (25.0f - p * 0.85f) * 0.0174533f;
  int ex = px + (int)lroundf(dir * cosf(ang) * L), ey = py + (int)lroundf(sinf(ang) * L);
  int w = max(8, L / 3);
  uint16_t kant = ok ? P.txt : P.faint;
  uiThick(g, px, py, ex, ey, w + 4, kant);
  uiThick(g, px, py, ex, ey, w, ok && work == 1 ? P.grn : P.white);
  int hr = max(1, w / 5);
  g->fillCircle(px + (ex - px) / 6, py + (ey - py) / 6, hr, kant);
  g->fillCircle(ex - (ex - px) / 6, ey - (ey - py) / 6, hr, kant);
}
// Kugghjul (kraftuttag) som i S4: helsvart med fyrkantiga kuggar. Grönt när uttaget går.
static void s4Kugg(Arduino_GFX *g, int cx, int cy, int r, uint16_t c) {
  g->fillCircle(cx, cy, r, c);
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f + 0.39f, ca = cosf(a), sa = sinf(a);
    float hw = r * 0.30f, r0 = r * 0.7f, r1 = r * 1.32f;
    float q[] = {cx + ca * r0 - sa * hw, cy + sa * r0 + ca * hw, cx + ca * r1 - sa * hw, cy + sa * r1 + ca * hw,
                 cx + ca * r1 + sa * hw, cy + sa * r1 - ca * hw, cx + ca * r0 + sa * hw, cy + sa * r0 - ca * hw};
    s4Poly(g, q, 4, c);
  }
}

// Ikoner (cx, cy, storlek, färg)
static void icTraktorS4(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) { s4Traktor(g, cx, cy, sz, c, P.keyHi); }
static void icTraktorHdr(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) { s4Traktor(g, cx, cy + 1, sz, c, P.hdr2); }
static void icLyft(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {        // Info⁺ Trepunktslyft
  s4Traktor(g, cx, cy + 4, sz * 45 / 100, c, P.keyHi);
  int L = sz * 28 / 100;
  uiThick(g, cx - sz * 22 / 100, cy + 2, cx - sz * 22 / 100 - L, cy - L / 2, 7, c);
  uiThick(g, cx - sz * 22 / 100, cy + 2, cx - sz * 22 / 100 - L, cy - L / 2, 3, P.keyHi);
  uiThick(g, cx + sz * 20 / 100, cy + 2, cx + sz * 20 / 100 + L, cy - L / 2, 7, c);
  uiThick(g, cx + sz * 20 / 100, cy + 2, cx + sz * 20 / 100 + L, cy - L / 2, 3, P.keyHi);
}
static void s4Cyl(Arduino_GFX *g, int x, int y, int w, int h, uint16_t col) {    // hydraulcylinder (S4)
  uint16_t kant = RGB(30, 30, 30);
  g->fillRoundRect(x, y, w, h, h / 2, kant);
  g->fillRoundRect(x + 1, y + 1, w - 2, h - 2, h / 2 - 1, col);
  g->drawFastHLine(x + 3, y + 2, w - 8, uiMix(col, RGB(255, 255, 255), 2, 3));
  g->fillRect(x + w - h / 2, y + h / 2 - 1, h / 2 + 2, 3, kant);
  g->drawCircle(x + w + h / 2 + 1, y + h / 2, h / 2 - 1, kant);
  g->drawCircle(x + w + h / 2 + 1, y + h / 2, h / 2 - 2, kant);
}
static void icVentiler(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  int w = sz * 55 / 100, h = max(7, sz / 6);
  s4Cyl(g, cx - w / 2 + 4, cy - h - 4, w, h, RGB(244, 200, 60));
  s4Cyl(g, cx - w / 2 - 2, cy - 2, w, h, RGB(90, 130, 230));
  s4Cyl(g, cx - w / 2 - 8, cy + h, w, h, RGB(236, 96, 66));
}
static void icRadata(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  int bw = sz / 6;
  for (int r = 0; r < 3; r++) for (int k = 0; k < 4; k++) g->fillRect(cx - sz / 3 + k * (bw + 3), cy - 10 + r * 8, bw, 5, c);
}
static void icFlytta(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {      // S4:s fyrvägspil (karta)
  int a = sz / 2, t = sz / 6;
  g->fillRect(cx - t / 2, cy - a + t, t, 2 * a - 2 * t, c);
  g->fillRect(cx - a + t, cy - t / 2, 2 * a - 2 * t, t, c);
  g->fillTriangle(cx, cy - a, cx - t * 3 / 2, cy - a + t + 2, cx + t * 3 / 2, cy - a + t + 2, c);
  g->fillTriangle(cx, cy + a, cx - t * 3 / 2, cy + a - t - 2, cx + t * 3 / 2, cy + a - t - 2, c);
  g->fillTriangle(cx - a, cy, cx - a + t + 2, cy - t * 3 / 2, cx - a + t + 2, cy + t * 3 / 2, c);
  g->fillTriangle(cx + a, cy, cx + a - t - 2, cy - t * 3 / 2, cx + a - t - 2, cy + t * 3 / 2, c);
}
static void icStyrning(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {    // S4: satellit + körspår
  int s = sz;
  // fält med körspår (trianglar mot horisonten)
  int by = cy + s * 34 / 100, ty = cy - s * 2 / 100;
  float f1[] = {(float)cx - s * 45 / 100, (float)by, (float)cx - s * 4 / 100, (float)ty, (float)cx + 0, (float)ty, (float)cx - s * 22 / 100, (float)by};
  float f2[] = {(float)cx - s * 14 / 100, (float)by, (float)cx + 3, (float)ty, (float)cx + 7, (float)ty, (float)cx + s * 10 / 100, (float)by};
  float f3[] = {(float)cx + s * 20 / 100, (float)by, (float)cx + 10, (float)ty, (float)cx + 14, (float)ty, (float)cx + s * 44 / 100, (float)by};
  s4Poly(g, f1, 4, c); s4Poly(g, f2, 4, c); s4Poly(g, f3, 4, c);
  // satellit
  int sx = cx + s * 8 / 100, sy = cy - s * 30 / 100;
  uiThick(g, sx - s * 34 / 100, sy - s * 10 / 100, sx - 5, sy - 2, 5, c);
  uiThick(g, sx + 5, sy + 2, sx + s * 34 / 100, sy + s * 10 / 100, 5, c);
  g->fillCircle(sx, sy, 4, c);
  g->fillArc(sx, sy + 2, 12, 10, 40, 140, c);
  g->fillArc(sx, sy + 2, 17, 15, 50, 130, c);
}
static void icAnalys(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {     // papper med penna och staplar
  int w = sz * 60 / 100, h = sz * 48 / 100, x = cx - w / 2 + 4, y = cy - h / 2 + 4;
  float pap[] = {(float)x + 8, (float)y, (float)x + w + 4, (float)y, (float)x + w - 4, (float)y + h, (float)x, (float)y + h};
  s4Poly(g, pap, 4, c);
  float in[] = {(float)x + 10, (float)y + 2, (float)x + w + 1, (float)y + 2, (float)x + w - 6, (float)y + h - 2, (float)x + 3, (float)y + h - 2};
  s4Poly(g, in, 4, P.keyHi);
  static const int H[4] = {6, 12, 9, 16};
  for (int i = 0; i < 4; i++) g->fillRect(x + 8 + i * (w / 5), y + h - 4 - H[i] * h / 30, w / 8, H[i] * h / 30, c);
  uiThick(g, cx - w / 2 - 2, cy - h / 2 - 10, cx - w / 2 + 10, cy + 6, 6, c);
}
static void icMane(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  int r = sz * 28 / 100;
  g->fillCircle(cx, cy, r, c);
  g->fillCircle(cx + r * 6 / 10, cy - r * 4 / 10, r * 9 / 10, uiMix(P.keyHi, P.keyLo, 1, 2));
}
static void icSol(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {
  int r = sz * 16 / 100;
  g->fillCircle(cx, cy, r, c);
  for (int i = 0; i < 8; i++) {
    float a = i * 0.785398f;
    uiThick(g, cx + cosf(a) * (r + 5), cy + sinf(a) * (r + 5), cx + cosf(a) * (r + 11), cy + sinf(a) * (r + 11), 3, c);
  }
}
static void icInst(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {       // S4: skiftnyckel
  int s = sz;
  uiThick(g, cx - s * 18 / 100, cy + s * 22 / 100, cx + s * 10 / 100, cy - s * 6 / 100, max(6, s / 7), c);
  g->fillCircle(cx + s * 16 / 100, cy - s * 12 / 100, s * 17 / 100, c);
  float m[] = {(float)cx + s * 16 / 100, (float)cy - s * 12 / 100, (float)cx + s * 34 / 100, (float)cy - s * 18 / 100,
               (float)cx + s * 22 / 100, (float)cy - s * 30 / 100};
  s4Poly(g, m, 3, uiMix(P.keyHi, P.keyLo, 1, 2));
  g->fillCircle(cx + s * 16 / 100, cy - s * 12 / 100, s * 6 / 100, uiMix(P.keyHi, P.keyLo, 1, 2));
}
static void icBok(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {         // v42: uppslagen bok (manualen)
  int w = sz / 2 - 2, h = sz * 6 / 10, y0 = cy - h / 2;
  for (int sgn = -1; sgn <= 1; sgn += 2) {
    int x0 = sgn < 0 ? cx - 2 - w : cx + 2;
    g->fillRect(x0, y0, w, h, c);
    g->fillRect(x0 + 2, y0 + 2, w - 4, h - 4, P.keyHi);
    for (int r = 1; r <= 3; r++) g->fillRect(x0 + 4, y0 + r * h / 4 - 1, w - 8, 2, c);
  }
  g->fillRect(cx - 2, y0 - 2, 4, h + 4, c);
}
static void icChip(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {        // v44: kretschip (enhetsinfo)
  int h = sz * 52 / 100, x0 = cx - h / 2, y0 = cy - h / 2;
  g->fillRoundRect(x0, y0, h, h, max(2, sz / 16), c);
  g->fillRect(x0 + sz / 10, y0 + sz / 10, h - sz / 5, h - sz / 5, P.keyHi);
  g->fillRect(x0 + sz * 17 / 100, y0 + sz * 17 / 100, h - sz * 34 / 100, h - sz * 34 / 100, c);
  int pl = max(3, sz / 9), pw = max(2, sz / 22);
  for (int i = 0; i < 4; i++) {
    int o = y0 + h * (2 * i + 1) / 8 - pw / 2;
    g->fillRect(x0 - pl, o, pl, pw, c); g->fillRect(x0 + h, o, pl, pw, c);
    int ox = x0 + h * (2 * i + 1) / 8 - pw / 2;
    g->fillRect(ox, y0 - pl, pw, pl, c); g->fillRect(ox, y0 + h, pw, pl, c);
  }
}
static void icLista(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {       // v42: innehållsförteckning
  int w = sz * 8 / 10, x0 = cx - w / 2;
  for (int r = -1; r <= 1; r++) {
    int y = cy + r * sz / 4;
    g->fillRect(x0, y - 2, 5, 5, c);
    g->fillRect(x0 + 9, y - 1, w - 9, 3, c);
  }
}
static void icTillbaka(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {   // S4:s returpil
  int s = sz;
  g->fillArc(cx + s * 6 / 100, cy, s * 26 / 100, s * 26 / 100 - max(4, s / 10), 270, 450, c);
  g->fillRect(cx - s * 14 / 100, cy - s * 26 / 100, s * 20 / 100, max(4, s / 10), c);
  g->fillRect(cx - s * 14 / 100, cy + s * 26 / 100 - max(4, s / 10) + 1, s * 20 / 100, max(4, s / 10), c);
  g->fillTriangle(cx - s * 30 / 100, cy + s * 21 / 100, cx - s * 12 / 100, cy + s * 8 / 100, cx - s * 12 / 100, cy + s * 34 / 100, c);
}
static void icHem(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {        // huset: startvalet (v37)
  int w = sz * 56 / 100, hb = sz * 30 / 100;                 // husets kropp
  int x0 = cx - w / 2, yb = cy + sz * 30 / 100, yt = yb - hb;
  int tak = sz * 30 / 100, over = sz * 9 / 100, t = max(3, sz / 14);
  g->fillRect(x0 + w * 64 / 100, yt - tak * 80 / 100, max(4, sz / 10), tak * 60 / 100, c);   // skorsten
  float roof[] = {(float)cx, (float)(yt - tak), (float)(x0 + w + over), (float)(yt + 1), (float)(x0 - over), (float)(yt + 1)};
  s4Poly(g, roof, 3, c);
  uint16_t bg = uiMix(P.keyHi, P.keyLo, 1, 2);
  float inner[] = {(float)cx, (float)(yt - tak + t * 2), (float)(x0 + w + over - t * 2), (float)(yt + 1 - t / 2),
                   (float)(x0 - over + t * 2), (float)(yt + 1 - t / 2)};
  s4Poly(g, inner, 3, bg);
  g->fillRect(x0, yt, w, hb, c);
  g->fillRect(x0 + t, yt, w - 2 * t, hb - t, bg);
  int dw = w * 30 / 100;                                    // dörr
  g->fillRect(cx - dw / 2, yb - hb * 62 / 100, dw, hb * 62 / 100, c);
}
static void icFart(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {       // hastighetsmätare (S4 "C1")
  int r = sz / 2;
  g->fillArc(cx, cy, r, r - 2, 150, 390, c);
  for (int i = 0; i <= 4; i++) {
    float a = (150 + i * 60) * 0.0174533f;
    g->drawLine(cx + cosf(a) * (r - 2), cy + sinf(a) * (r - 2), cx + cosf(a) * (r - 5), cy + sinf(a) * (r - 5), c);
  }
  uiThick(g, cx, cy, cx + r * 5 / 10, cy + r * 3 / 10, 2, c);
  g->fillCircle(cx, cy, 2, c);
}
static void icVarv(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {       // varvräknare
  int r = sz / 2;
  g->fillArc(cx, cy, r, r - 2, 150, 390, c);
  uiThick(g, cx, cy, cx - r * 4 / 10, cy - r * 5 / 10, 2, c);
  g->fillCircle(cx, cy, 2, c);
  uiTextCenter(g, cx, cy + r - 1, "rpm", c, u8g2_font_dv11_tf);
}
static void icPump(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {       // bränslepump (S4)
  int w = sz * 42 / 100, h = sz * 70 / 100, x = cx - sz / 2 + 2, y = cy - h / 2;
  g->fillRoundRect(x, y, w, h, 3, c);
  g->fillRect(x + 3, y + 3, w - 6, h / 3, P.pan);
  g->fillRect(x - 2, y + h - 3, w + 4, 3, c);
  uiThick(g, x + w, y + h / 3, x + w + sz * 22 / 100, y + h / 3 + 4, 3, c);
  uiThick(g, x + w + sz * 22 / 100, y + h / 3 + 4, x + w + sz * 22 / 100, y + h - 6, 3, c);
  uiThick(g, x + w + sz * 22 / 100, y + h - 6, x + w + 4, y + h - 6, 3, c);
}
static void icRiktn(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {      // körriktning
  int s = sz / 2;
  g->fillRect(cx - s, cy - 3, s, 6, c);
  g->fillTriangle(cx, cy - s / 2 - 2, cx, cy + s / 2 + 2, cx + s, cy, c);
}
static void icTimmar(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {     // klocka
  int r = sz * 40 / 100;
  g->fillCircle(cx, cy, r, c);
  g->fillCircle(cx, cy, r - 3, P.pan);
  g->fillRect(cx - 1, cy - r + 5, 3, r - 4, c);
  g->fillRect(cx, cy - 1, r - 5, 3, c);
}

// ---------------------------------------------------------------- Ramverk: kolumner och rutor
#define S4_LX   4         // vänsterkolumnen (Info⁺-knappar)
#define S4_RX   708       // högerkolumnen
#define S4_KW   88
#define S4_CX0  98        // rutornas område 98..798 (v37: ingen högerkolumn)
#define S4_QW   349
#define S4_QH   236
#define S4_QX(c) (S4_CX0 + (c) * (S4_QW + 2))
#define S4_QY(r) (2 + (r) * (S4_QH + 2))

// Vänsterkolumnen (samma på traktor- och redskapssidan): fem platser uppifrån.
// Plats 0 och 1 är sidans egna (Info⁺, TERMINAL ...), 2 = månen/solen, 3 = huset, 4 = skiftnyckeln.
#define S4_VKH  88
#define S4_VY(i) (4 + (i) * 95)
static void s4VKol(Arduino_GFX *g, const UiState &s) {
  s4Key(g, s, S4_LX, S4_VY(2), S4_KW, S4_VKH, B_NATT, false, s.natt ? icSol : icMane, 54);
  s4Key(g, s, S4_LX, S4_VY(3), S4_KW, S4_VKH, B_HEM, false, icHem, 66);
  s4Key(g, s, S4_LX, S4_VY(4), S4_KW, S4_VKH, B_SETTINGS, false, icInst, 66);
}


// ---------------------------------------------------------------- S4: knappar med text, sidram, rutor
// Typsnittets versalhöjd (för att centrera text lodrätt)
static int s4Asc(const uint8_t *f) {
  if (f == u8g2_font_dv11_tf) return 8;
  if (f == u8g2_font_dv13_tf || f == u8g2_font_dv13b_tf) return 9;
  if (f == u8g2_font_dv15_tf || f == u8g2_font_dv15b_tf) return 11;
  if (f == u8g2_font_dv17_tf || f == u8g2_font_dv17b_tf) return 12;
  if (f == u8g2_font_dv20_tf || f == u8g2_font_dv20b_tf) return 15;
  if (f == u8g2_font_dv24_tf || f == u8g2_font_dv24b_tf) return 17;
  return 11;
}
// Knapp med text i S4-stil. style 0 = vanlig, 1 = varning (röd text), 2 = grön (aktiv/OK)
static void s4KeyT(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h, int id, const char *label,
                   const uint8_t *font = u8g2_font_dv17b_tf, int style = 0, bool sel = false) {
  bool down = s.pressId == id && id != B_NONE;
  if (sel) g->fillRoundRect(x - 3, y - 3, w + 6, h + 6, 9, P.sel);
  g->fillRoundRect(x, y, w, h, 7, P.keyBd);
  uint16_t hi = P.keyHi, lo = P.keyLo, fg = P.ico;
  if (style == 2) { hi = uiMix(P.grn, RGB(255, 255, 255), 1, 4); lo = P.grn; fg = P.grnTxt; }
  if (style == 1) fg = P.sel;
  g->fillRoundRect(x + 1, y + 1, w - 2, h - 2, 6, down ? lo : hi);
  s4VGrad(g, x + 4, y + 4, w - 8, h - 8, down ? lo : hi, down ? hi : lo);
  int dd = down ? 1 : 0;
  uiTextCenter(g, x + w / 2 + dd, y + h / 2 + s4Asc(font) / 2 + dd, label, fg, font);
  if (id != B_NONE) uiReg(x, y, w, h, id);
}
// Den gamla knappfunktionen ritar nu i S4-stil (används av tangentbordet m.m.)
static void uiButton(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h, int id,
                     const char *label, const uint8_t *font, int style = 0) {
  s4KeyT(g, s, x, y, w, h, id, label, font, style);
}
// Hel sida: mörk bakgrund, en stor ruta med rubrik (x 2..702) och högerkolumnen fri för knappar
static void s4Page(Arduino_GFX *g, const char *title, bool info = false) {
  g->fillScreen(P.scr);
  s4Box(g, 2, 2, 700, 476);
  s4Head(g, 2, 2, 700, title, info);
}
// Liten lampa (buss) och WiFi-symbol, högerställd i en rubrikrad
static void s4Lampa(Arduino_GFX *g, int xr, int y, bool busOk, bool fresh, bool online) {
  uint16_t dot = !busOk ? P.sel : (fresh ? P.grn : P.amb);
  g->fillCircle(xr - 9, y + 13, 6, dot);
  g->drawCircle(xr - 9, y + 13, 6, P.lin);
  int cx = xr - 34, by = y + 21;
  uint16_t c = online ? P.txt : P.faint;
  g->fillCircle(cx, by - 2, 2, c);
  g->fillArc(cx, by, 8, 6, 225, 315, c);
  g->fillArc(cx, by, 13, 11, 225, 315, c);
  if (!online) g->drawLine(cx - 9, by - 14, cx + 9, by + 1, P.sel);
}
// Ruta över sidan (nolla, priser, uppdatering): rutig slöja, ruta med rubrik
static void uiDim(Arduino_GFX *g) {
  for (int y = 0; y < 480; y += 2)
    for (int x = (y / 2) & 1; x < 800; x += 2) g->drawPixel(x, y, P.scr);
}
static void s4Dialog(Arduino_GFX *g, int x, int y, int w, int h, const char *title) {
  uiDim(g);
  g->fillRect(x - 3, y - 3, w + 6, h + 6, P.scr);
  s4Box(g, x, y, w, h);
  s4Head(g, x, y, w, title, true);
}

// Fler ikoner
static void icRedskap(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {     // allmänt redskap med ISOBUS
  int w = sz * 64 / 100, h = sz * 30 / 100, x = cx - w / 2 + 4, y = cy - h / 2 - 6;
  g->fillRect(x, y, w, h, c);
  g->fillRect(x - sz * 16 / 100, y + h - 4, sz * 18 / 100, 3, c);
  for (int i = 0; i < 5; i++) g->fillTriangle(x + 4 + i * w / 5, y + h, x + 10 + i * w / 5, y + h, x + 7 + i * w / 5, y + h + 10, c);
  g->fillCircle(x + w - 8, y + h + 6, 6, c);
}
static void icTerminal(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {    // terminalen (S4-skärmen)
  int w = sz * 62 / 100, h = sz * 44 / 100, x = cx - w / 2 - 4, y = cy - h / 2;
  g->fillRoundRect(x, y, w, h, 3, c);
  g->fillRect(x + 4, y + 4, w - 8, h - 8, P.keyHi);
  g->fillRect(x + 7, y + 7, (w - 14) / 2, h - 14, uiMix(c, P.keyHi, 1, 2));
  g->fillRect(x + w, y + 4, sz * 10 / 100, h - 8, c);
}
static void icKlar(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {        // bock
  uiThick(g, cx - sz * 22 / 100, cy, cx - sz * 6 / 100, cy + sz * 16 / 100, max(5, sz / 9), c);
  uiThick(g, cx - sz * 6 / 100, cy + sz * 16 / 100, cx + sz * 24 / 100, cy - sz * 20 / 100, max(5, sz / 9), c);
}
static void icSok(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {         // sök igen (pil i ring)
  int r = sz * 26 / 100;
  g->fillArc(cx, cy, r, r - 5, 30, 320, c);
  g->fillTriangle(cx + r + 4, cy - 6, cx + r - 10, cy - 6, cx + r - 3, cy + 5, c);
}
static void icSidaNer(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {     // S4:s pil nedåt (Info⁺ ↓)
  int w = sz * 14 / 100;
  g->fillRect(cx - w / 2, cy - sz * 30 / 100, w, w, c);
  g->fillRect(cx - w / 2, cy - sz * 10 / 100, w, sz * 16 / 100, c);
  g->fillTriangle(cx - sz * 26 / 100, cy + sz * 6 / 100, cx + sz * 26 / 100, cy + sz * 6 / 100, cx, cy + sz * 32 / 100, c);
}
static void icBredd(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {       // arbetsbredd |<->|
  int a = sz * 40 / 100;
  g->fillRect(cx - a, cy - 8, 2, 16, c); g->fillRect(cx + a - 1, cy - 8, 2, 16, c);
  g->fillRect(cx - a + 4, cy - 1, 2 * a - 7, 2, c);
  g->fillTriangle(cx - a + 2, cy, cx - a + 8, cy - 5, cx - a + 8, cy + 5, c);
  g->fillTriangle(cx + a - 2, cy, cx + a - 8, cy - 5, cx + a - 8, cy + 5, c);
}
// Varningsruta "ingen ISOBUS" (S4: vit ruta, röd kant, varningstriangel)
static void uiNoBus(Arduino_GFX *g) {
  int w = 300, x = 400 - w / 2, y = 436;
  g->fillRect(x - 2, y - 2, w + 4, 38, P.sel);
  g->fillRect(x, y, w, 34, P.pan);
  g->fillTriangle(x + 12, y + 28, x + 36, y + 28, x + 24, y + 6, P.sel);
  g->fillTriangle(x + 16, y + 26, x + 32, y + 26, x + 24, y + 11, RGB(255, 220, 0));
  g->fillRect(x + 23, y + 15, 3, 6, RGB(20, 20, 20)); g->fillRect(x + 23, y + 23, 3, 2, RGB(20, 20, 20));
  uiText(g, x + 48, y + 23, L("Ingen ISOBUS-trafik", "No ISOBUS traffic"), P.txt, u8g2_font_dv15b_tf);
}
// "DEMO" när demoläget är på
static void uiDemoBadge(Arduino_GFX *g) {
  int w = 300, x = 400 - w / 2, y = 440;
  g->fillRect(x, y, w, 32, P.amb);
  g->drawRect(x, y, w, 32, uiMix(P.amb, RGB(0, 0, 0), 1, 3));
  uiTextCenter(g, 400, y + 22, L("DEMO - simulerade värden", "DEMO - simulated values"), RGB(20, 20, 20), u8g2_font_dv15b_tf);
}

// ---------------------------------------------------------------- Startbild
static void uiDrawSplash(Arduino_GFX *g) {
  g->setTextWrap(false);
  g->fillScreen(P.pan);
  uiLogo(g, (800 - LOGO_BIG_W) / 2, 50, LOGO_BIG_W, LOGO_BIG_H, LOGO_BIG, P.txt, P.pan);
  uiTextCenter(g, 400, 432, L("startar ...", "starting ..."), P.faint, u8g2_font_dv15_tf);
}

// ---------------------------------------------------------------- Uppdateringsruta
static void uiOta(Arduino_GFX *g, const UiState &s) {
  uiNBtn = 0;
  s4Dialog(g, 150, 140, 500, 200, L("Uppdaterar programmet", "Updating the software"));
  char b[48];
  snprintf(b, sizeof(b), "version %d", s.net.latestVersion);
  uiText(g, 176, 200, b, P.txt, u8g2_font_dv20b_tf);
  snprintf(b, sizeof(b), "%d%%", s.net.otaPct);
  s4Bar(g, 176, 220, 448, 32, (float)s.net.otaPct, b, true);
  uiTextCenter(g, 400, 300, L("skärmen startar om av sig själv", "the display restarts by itself"), P.dim, u8g2_font_dv15_tf);
}

// Radbryter text i en ruta (ord för ord, \n = ny rad). Returnerar y efter sista raden.
static int uiWrap(Arduino_GFX *g, int x, int y, int w, int radH, const char *txt, uint16_t col, const uint8_t *font) {
  char rad[160]; int n = 0;
  const char *p = txt;
  while (*p) {
    const char *q = p;
    while (*q && *q != ' ' && *q != '\n') q++;
    char test[160];
    int len = (int)(q - p);
    if (n + 1 + len >= (int)sizeof(rad)) len = sizeof(rad) - n - 2;
    memcpy(test, rad, n); int tn = n;
    if (tn) test[tn++] = ' ';
    memcpy(test + tn, p, len); tn += len; test[tn] = 0;
    if (n && uiTextWidth(g, test, font) > w) {
      rad[n] = 0; uiText(g, x, y, rad, col, font); y += radH; n = 0;
      memcpy(rad, p, len); n = len;
    } else { memcpy(rad, test, tn); n = tn; }
    p = q;
    if (*p == '\n') { rad[n] = 0; if (n) uiText(g, x, y, rad, col, font); y += radH; n = 0; p++; }
    else if (*p == ' ') p++;
  }
  if (n) { rad[n] = 0; uiText(g, x, y, rad, col, font); y += radH; }
  return y;
}

// ---------------------------------------------------------------- Frågan "Ny mjukvara tillgänglig"
// Visas när skärmen startat eller fått internet och en nyare version finns. Står kvar tills föraren
// svarar. Ingenting installeras utan JA. Rutan säger tydligt att uppdateringen är frivillig, görs på eget
// ansvar och att den som gjort programmet inte ansvarar för något. STÄNG AV = fråga aldrig mer
// (kan slås på igen i inställningarna -> MER).
static void uiOtaFraga(Arduino_GFX *g, const UiState &s) {
  uiNBtn = 0;                       // bara rutans knappar gäller
  s4Dialog(g, 60, 20, 680, 440, L("Uppdatering", "Update"));
  char b[80];
  snprintf(b, sizeof(b), L("Ny version %d finns (du har %d)", "New version %d is available (you have %d)"), s.net.otaOffer, s.version);
  uiTextCenter(g, 400, 80, b, P.txt, u8g2_font_dv20b_tf);
  int y = uiWrap(g, 84, 116, 632, 21, L(
    "Uppdateringen är frivillig. Fältloggen är gratis, öppen källkod (GPLv3) och görs på fritiden. "
    "Programmet levereras i befintligt skick, utan garanti av något slag. Den som gjort programmet "
    "ansvarar inte för fel, skador, förlorad data eller andra följder av att det installeras eller används.\n"
    "Genom att trycka JA godkänner du att installationen och all användning sker helt på ditt eget ansvar. "
    "Skärmen lyssnar bara på ISOBUS och skickar ingenting till traktorn. Startar den nya versionen inte "
    "ordentligt går skärmen själv tillbaka till den gamla.",
    "The update is optional. Fältloggen is free, open-source software (GPLv3) made in spare time. "
    "It is provided as is, without warranty of any kind. The person who made it is not responsible for "
    "errors, damage, lost data or any other consequences of installing or using it.\n"
    "By pressing YES you accept that the installation and all use are entirely at your own risk. "
    "The display only listens to ISOBUS and never sends anything to the tractor. If the new version does "
    "not start properly, the display goes back to the old one by itself."), P.txt, u8g2_font_dv15_tf);
  (void)y;
  s4KeyT(g, s, 84, 372, 196, 68, B_OTA_AV, L("STÄNG AV", "TURN OFF"), u8g2_font_dv17b_tf);
  s4KeyT(g, s, 302, 372, 196, 68, B_OTA_NEJ, L("INTE NU", "NOT NOW"), u8g2_font_dv17b_tf);
  s4KeyT(g, s, 520, 372, 196, 68, B_OTA_JA, L("JA, INSTALLERA", "YES, INSTALL"), u8g2_font_dv17b_tf, 2);
  uiTextCenter(g, 182, 456, L("frågar aldrig mer", "never ask again"), P.faint, u8g2_font_dv11_tf);
}

// ---------------------------------------------------------------- Redskapssida i S4-stil
static void uiDrawGeneric(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(P.scr);
  char b[64];
  int pages = (s.nGen + GEN_PER_PAGE - 1) / GEN_PER_PAGE;
  int page = pages ? s.genPage % pages : 0;
  // vänsterkolumnen: TERMINAL, sida (om listan är lång), månen, huset, skiftnyckeln
  if (s.vtAvail) s4Key(g, s, S4_LX, S4_VY(0), S4_KW, S4_VKH, B_VT_OPEN, false, icTerminal, 60, "Info⁺");
  if (pages > 1) {
    snprintf(b, sizeof(b), "%d/%d", page + 1, pages);
    s4Key(g, s, S4_LX, S4_VY(1), S4_KW, S4_VKH, B_GEN_PAGE, false, icSidaNer, 50, b);
  }
  s4VKol(g, s);

  const int X = S4_CX0, W = 2 * S4_QW + 2;
  // överst: redskapets namn, beskrivning och fyra värdefält
  int y = 2, h = 128;
  s4Box(g, X, y, W, h);
  s4Head(g, X, y, W, s.genTitel, true);
  bool anyFresh = false;
  for (int i = 0; i < s.nGen; i++) if (s.gen[i].farsk) anyFresh = true;
  s4Lampa(g, X + W - 6, y, s.busOk, anyFresh, s.net.connected);
  uiTextFit(g, X + 12, y + S4_HDR + 18, s.genSub, W - 24, P.dim, u8g2_font_dv13_tf);
  s4Sep(g, X, y + S4_HDR + 26, W);
  {
    const int cw = W / 4, fy = y + S4_HDR + 32;
    auto ruta = [&](int i, const char *lab, const char *val, const char *unit, bool ok) {
      int cx = X + i * cw;
      if (i) g->drawFastVLine(cx, fy + 2, h - (fy - y) - 8, P.sep);
      uiTextFit(g, cx + 10, fy + 14, lab, cw - 20, P.dim, u8g2_font_dv13_tf);
      int uw = unit && unit[0] ? uiTextWidth(g, unit, u8g2_font_dv13_tf) + 6 : 0;
      s4Field(g, cx + 10, fy + 22, cw - 20 - uw, 36, val, ok ? P.txt : P.faint, u8g2_font_dv20_tf);
      if (uw) uiText(g, cx + cw - 6 - uw + 4, fy + 48, unit, P.txt, u8g2_font_dv13_tf);
    };
    uiFmt(b, sizeof(b), s.kmh, 1);
    ruta(0, L("Hastighet", "Speed"), b, "km/h", !isnan(s.kmh));
    ruta(1, L("Arbetsbredd", "Working width"), s.genWidth, "m", strcmp(s.genWidth, "--") != 0);
    ruta(2, L("Arbetsläge", "Work state"), s.genWork, "", strcmp(s.genWork, "--") != 0);
    snprintf(b, sizeof(b), "%d", s.nGen); ruta(3, L("Värden", "Values"), b, L("st", "pcs"), s.nGen > 0);
  }
  // lista med redskapets värden, två spalter
  y += h + 2; h = 478 - y;
  s4Box(g, X, y, W, h);
  s4Head(g, X, y, W, L("Värden från redskapet", "Values from the implement"), true);
  if (s.genHasImpl)
    uiTextRight(g, X + W - 10, y + 18, s.genDdop ? L("namn och enheter från redskapets beskrivning", "names and units from the implement description")
                                                 : L("beskrivning saknas - standardnamn", "no description - standard names"), P.dim, u8g2_font_dv11_tf);
  if (!s.genHasImpl) {
    uiTextCenter(g, X + W / 2, y + 140, L("Inget ISOBUS-redskap hittat", "No ISOBUS implement found"), P.dim, u8g2_font_dv20b_tf);
    uiTextCenter(g, X + W / 2, y + 170, L("bara traktorns egna enheter hörs på bussen", "only the tractor's own units are heard on the bus"), P.faint, u8g2_font_dv15_tf);
  } else if (s.nGen == 0) {
    uiTextCenter(g, X + W / 2, y + 150, L("Redskapet har inte skickat några värden än", "The implement has not sent any values yet"), P.dim, u8g2_font_dv15_tf);
  }
  const int colW = (W - 24) / 2, rowH = 30;
  for (int k = 0; k < GEN_PER_PAGE; k++) {
    int i = page * GEN_PER_PAGE + k;
    if (i >= s.nGen) break;
    const GenRow &r = s.gen[i];
    int cx = X + 10 + (k / 10) * (colW + 4);
    int ry = y + S4_HDR + 4 + (k % 10) * rowH;
    if (k % 10) g->drawFastHLine(cx, ry, colW - 6, P.sep);
    int valR = cx + colW - 52;
    uiTextFit(g, cx + 2, ry + 20, r.namn, colW - 146, r.farsk ? P.txt : P.faint, u8g2_font_dv13_tf);
    s4Field(g, valR - 84, ry + 3, 84, rowH - 5, r.val, r.farsk ? P.txt : P.faint, u8g2_font_dv15_tf);
    uiTextFit(g, valR + 4, ry + 20, r.enh, 46, r.farsk ? P.dim : P.faint, u8g2_font_dv13_tf);
  }
  if (!s.busOk) uiNoBus(g);
}

// Hydraulventilernas namn och färger (ventil n = PGN 0xFE10 + n). Färgerna är de som Fendt S4 använder
// för ventil 1-5 bak; ventil 7 är främre ventilen. Kontrollera mot din egen traktor.
static const char *ventilNamn(int n) {
  static const char *const SV[] = {"0", "Gul", "Blå", "Röd", "Grön", "Brun", "6", "Front"};
  static const char *const EN[] = {"0", "Yellow", "Blue", "Red", "Green", "Brown", "6", "Front"};
  static char b[4];
  if (n >= 0 && n < 8) return L(SV[n], EN[n]);
  snprintf(b, sizeof(b), "%d", n); return b;
}
static uint16_t ventilFarg(int n) {
  switch (n) {
    case 1: return RGB(250, 210, 40);    // gul
    case 2: return RGB(70, 140, 255);    // blå
    case 3: return RGB(255, 80, 70);     // röd
    case 4: return RGB(90, 210, 90);     // grön
    case 5: return RGB(176, 120, 70);    // brun
    default: return RGB(220, 224, 228);
  }
}

// ---------------------------------------------------------------- Karta (helskärm) i S4-stil
static void uiAreaText(char *b, int n, float ha) {
  char t[20]; uiFmt(t, sizeof(t), ha, ha < 100 ? 2 : 1);
  snprintf(b, n, "%s ha", t);
}

// Spårets färgskala (v36): t = -1 grönt (lågt), 0 blått (körningens snitt), +1 rött (högt)
static uint16_t uiSkalaFarg(float t) {
  if (t < -1) t = -1; else if (t > 1) t = 1;
  const float B[3] = {40, 110, 235}, G[3] = {40, 205, 50}, R[3] = {235, 35, 25};
  const float *m = t < 0 ? G : R; float a = fabsf(t);
  return RGB((int)(B[0] + (m[0] - B[0]) * a), (int)(B[1] + (m[1] - B[1]) * a), (int)(B[2] + (m[2] - B[2]) * a));
}

// Ruta "Färg på spåret": BRÄNSLE / DRAGKRAFT / SLIRNING (v41) och färgstegen med körningens siffror
static void uiKartaFargRuta(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h) {
  const KartaView &k = s.karta;
  s4Box(g, x, y, w, h);
  s4Head(g, x, y, w, L("Färg på spåret", "Track colour"), true);
  int kw = (w - 28) / 3;
  s4KeyT(g, s, x + 8, y + S4_HDR + 6, kw, 34, B_K_FARG_B, L("BRÄNSLE", "FUEL"), u8g2_font_dv13b_tf, 0, k.fargLage == 0);
  s4KeyT(g, s, x + 14 + kw, y + S4_HDR + 6, kw, 34, B_K_FARG_D, L("DRAG", "DRAFT"), u8g2_font_dv13b_tf, 0, k.fargLage == 1);
  s4KeyT(g, s, x + 20 + 2 * kw, y + S4_HDR + 6, kw, 34, B_K_FARG_S, L("SLIRNING", "SLIP"), u8g2_font_dv13b_tf, 0, k.fargLage == 2);
  int sy = y + S4_HDR + 48, sx = x + 12, sw = w - 24;
  for (int i = 0; i < sw; i++) g->drawFastVLine(sx + i, sy, 12, uiSkalaFarg(i * 2.0f / (sw - 1) - 1.0f));
  g->drawRect(sx - 1, sy - 1, sw + 2, 14, P.sep);
  const char *enh = k.fargLage == 2 ? "%" : (k.fargLage ? "kN" : "l/ha");
  char b[24], v[16];
  if (!k.fargHar || isnan(k.fargSnitt)) {
    uiTextCenter(g, x + w / 2, sy + 30, L("inget värde än - loggas under körning", "no value yet - logged while driving"), P.faint, u8g2_font_dv11_tf);
    return;
  }
  int dec = k.fargLage ? 0 : 1;
  uiFmt(v, sizeof(v), k.fargSnitt - k.fargSpann, dec); uiText(g, sx, sy + 30, v, P.txt, u8g2_font_dv13_tf);
  uiFmt(v, sizeof(v), k.fargSnitt, dec); snprintf(b, sizeof(b), L("snitt %s %s", "avg %s %s"), v, enh);
  uiTextCenter(g, sx + sw / 2, sy + 30, b, P.txt, u8g2_font_dv13b_tf);
  uiFmt(v, sizeof(v), k.fargSnitt + k.fargSpann, dec); uiTextRight(g, sx + sw, sy + 30, v, P.txt, u8g2_font_dv13_tf);
}

// Fliken som fäller in/ut kartans informationsrutor (v37). ut = rutorna är infällda (pil åt höger = fäll ut)
static void s4KeyGardin(Arduino_GFX *g, const UiState &s, int x, int y, bool ut) {
  const int w = 40, h = 104;
  bool down = s.pressId == B_K_GARDIN;
  g->fillRoundRect(x, y, w, h, 6, P.keyBd);
  g->fillRoundRect(x + 1, y + 1, w - 2, h - 2, 5, down ? P.keyLo : P.keyHi);
  s4VGrad(g, x + 3, y + 4, w - 6, h - 8, down ? P.keyLo : P.keyHi, down ? P.keyHi : P.keyLo);
  int cx = x + w / 2, cy = y + 26;
  if (ut) g->fillTriangle(cx - 7, cy - 11, cx - 7, cy + 11, cx + 9, cy, P.ico);
  else    g->fillTriangle(cx + 7, cy - 11, cx + 7, cy + 11, cx - 9, cy, P.ico);
  const char *t = "INFO";
  for (int i = 0; i < 4; i++) { char c[2] = {t[i], 0}; uiTextCenter(g, cx, y + 54 + i * 13, c, P.ico, u8g2_font_dv11_tf); }
  uiReg(x, y, w + 6, h, B_K_GARDIN);
}

static void uiDrawKarta(Arduino_GFX *g, const UiState &s) {
  const KartaView &k = s.karta;
  float mpp = s.kartaMpp;
  g->fillScreen(P.scr);
  const int MW = 702;                                    // kartan 0..702, knapparna till höger
  if (uiKartaRender) uiKartaRender(g, 0, 0, MW, 480, mpp, true);
  else g->fillRect(0, 0, MW, 480, RGB(228, 230, 178));
  char b[64];

  // uppe till vänster: areal, läge och bränsle (S4-ruta). v37: rutorna Areal och Färg på spåret kan fällas in
  // med fliken (B_K_GARDIN); den undre rutan med bredd, sträcka och skalstock syns alltid.
  const int ix = 6, iy = 6, iw = 300, ih = 244;
  if (s.kartaGardin) {
    s4KeyGardin(g, s, 0, iy, true);
  } else {
  s4KeyGardin(g, s, ix + iw + 2, iy, false);
  s4Box(g, ix, iy, iw, ih);
  s4Head(g, ix, iy, iw, L("Areal", "Area"), true);
  if (k.logging) s4Field(g, ix + iw - 92, iy + 3, 84, 20, L("LOGGAR", "LOGGING"), P.grnTxt, u8g2_font_dv13b_tf, P.grn);
  else uiTextRight(g, ix + iw - 10, iy + 18, L("PAUS", "PAUSED"), P.dim, u8g2_font_dv15b_tf);
  char t[20]; uiFmt(t, sizeof(t), k.areaHa, k.areaHa < 100 ? 2 : 1);
  s4Field(g, ix + 10, iy + 32, 196, 52, t, P.txt, u8g2_font_dv20_tf);
  uiText(g, ix + 214, iy + 68, "ha", P.txt, u8g2_font_dv20_tf);
  {
    char f[20], l[20];
    uiFmt(f, sizeof(f), k.fuelL, k.fuelL < 100 ? 1 : 0);
    uiFmt(l, sizeof(l), k.lPerHa, 1);
    icPump(g, ix + 22, iy + 106, 24, P.txt);
    snprintf(b, sizeof(b), "%s l", f);
    s4Field(g, ix + 38, iy + 92, 90, 28, b, P.txt, u8g2_font_dv15_tf);
    snprintf(b, sizeof(b), "%s l/ha", l);
    s4Field(g, ix + 136, iy + 92, 154, 28, b, isnan(k.lPerHa) ? P.faint : P.txt, u8g2_font_dv15_tf);
  }
  if (k.trig == 0) snprintf(b, sizeof(b), "%s", L("start/stopp med knappen", "start/stop with the button"));
  else snprintf(b, sizeof(b), L("%s: start %s, stopp %s", "%s: start %s, stop %s"), k.trigKort, k.startTxt, k.stopTxt);
  // v45 (10.19): skiftets areal, återstående tid och beräknat klockslag. Tryck = sifferfält.
  {
    const int sy = iy + 126;
    if (k.skiftHa <= 0) {
      s4KeyT(g, s, ix + 8, sy, iw - 16, 40, B_K_SKIFT, L("SKIFTETS AREAL", "FIELD AREA"), u8g2_font_dv15b_tf);
    } else {
      char a[16], q[16], sb[64];
      uiFmt(a, sizeof(a), k.skiftHa, 2);
      uiFmt(q, sizeof(q), k.skiftKvarHa, 2);
      s4Box(g, ix + 8, sy, iw - 16, 46);
      snprintf(sb, sizeof(sb), L("skifte %s ha  ·  kvar %s ha", "field %s ha  ·  left %s ha"), a, q);
      uiTextFit(g, ix + 16, sy + 18, sb, iw - 32, P.txt, u8g2_font_dv13_tf);
      if (k.skiftKlart) snprintf(sb, sizeof(sb), "%s", L("skiftet är klart", "the field is done"));
      else if (k.skiftMin < 0) snprintf(sb, sizeof(sb), "%s", L("räknar ut tid ...", "calculating time ..."));
      else {
        int h = k.skiftMin / 60, m = k.skiftMin % 60;
        if (h) snprintf(sb, sizeof(sb), L("%d h %02d min kvar", "%d h %02d min left"), h, m); else snprintf(sb, sizeof(sb), L("%d min kvar", "%d min left"), m);
        if (k.skiftKlass[0]) { size_t n = strlen(sb); snprintf(sb + n, sizeof(sb) - n, L("  ·  klart %s", "  ·  done %s"), k.skiftKlass); }
      }
      uiTextFit(g, ix + 16, sy + 38, sb, iw - 32, k.skiftKlart ? P.grnTxt : P.txt, u8g2_font_dv15b_tf);
      uiReg(ix + 8, sy, iw - 16, 46, B_K_SKIFT);
    }
  }
  uiTextFit(g, ix + 10, iy + 188, b, iw - 20, P.dim, u8g2_font_dv11_tf);
  s4KeyT(g, s, ix + 8, iy + 196, iw - 16, 40, B_K_EXPORT, L("EXPORTERA RAPPORT", "EXPORT REPORT"), u8g2_font_dv15b_tf);
  uiKartaFargRuta(g, s, ix, iy + ih + 6, iw, 118);
  }

  // höger kolumn: tillbaka, zoom, start/stopp, inställningar, nolla, följ
  const int kx = S4_RX, kw = S4_KW;
  bool z100 = fabsf(mpp - K_MPP_100M) < 0.01f;
  s4Key(g, s, kx, 4, kw, 62, B_K_BACK, false, icTillbaka, 50);
  s4KeyT(g, s, kx, 72, kw, 54, B_K_ZIN, "+", u8g2_font_dv24b_tf);
  s4KeyT(g, s, kx, 132, kw, 54, B_K_Z100, "100 m", u8g2_font_dv15b_tf, 0, z100);
  s4KeyT(g, s, kx, 192, kw, 54, B_K_ZUT, "-", u8g2_font_dv24b_tf);
  s4KeyT(g, s, kx, 256, kw, 66, B_K_START, k.logging ? L("STOPP", "STOP") : "START", u8g2_font_dv17b_tf, k.logging ? 1 : 2);
  s4Key(g, s, kx, 330, kw, 66, B_K_INST, false, icInst, 54);
  s4KeyT(g, s, kx, 404, kw, 70, B_K_NOLLA, L("NOLLA", "RESET"), u8g2_font_dv15b_tf, 1);

  // nere till vänster: bredd, sträcka, skalstock och bildkälla
  const int by = 404, bw = 430;
  s4Box(g, 6, by, bw, 70);
  char w[20]; uiFmt(w, sizeof(w), k.widthM, 1);
  char d[20]; uiFmt(d, sizeof(d), k.distKm, 1);
  snprintf(b, sizeof(b), L("bredd %s m (%s)  ·  körd sträcka %s km", "width %s m (%s)  ·  distance %s km"), w, k.widthFromBus ? "ISOBUS" : L("manuell", "manual"), d);
  uiTextFit(g, 16, by + 20, b, bw - 20, P.txt, u8g2_font_dv13_tf);
  int sm = 10; while (sm / mpp < 60) sm = (sm == 10 || sm == 100 || sm == 1000) ? sm * 5 / 2 : sm * 2;
  int sl = (int)(sm / mpp);
  g->fillRect(16, by + 36, sl, 4, P.txt);
  g->fillRect(16, by + 32, 2, 12, P.txt); g->fillRect(16 + sl - 2, by + 32, 2, 12, P.txt);
  snprintf(b, sizeof(b), sm >= 1000 ? "%d km" : "%d m", sm >= 1000 ? sm / 1000 : sm);
  uiText(g, 24 + sl, by + 44, b, P.dim, u8g2_font_dv13_tf);
  if (k.bakgrund == 0) uiTextFit(g, 16, by + 64, kBildText(), bw - 20, P.faint, u8g2_font_dv11_tf);
  // kompass (norr uppåt)
  int cx = MW - 24, cy = 440;
  g->fillTriangle(cx - 7, cy, cx + 7, cy, cx, cy - 22, RGB(220, 30, 30));
  g->fillTriangle(cx - 7, cy, cx + 7, cy, cx, cy + 22, RGB(250, 250, 250));
  g->drawLine(cx - 7, cy, cx, cy + 22, RGB(90, 90, 90)); g->drawLine(cx + 7, cy, cx, cy + 22, RGB(90, 90, 90));

  if (k.panned) s4KeyT(g, s, 560, 404, 110, 54, B_K_FOLJ, L("FÖLJ", "FOLLOW"), u8g2_font_dv17b_tf, 2);
  else {
    const char *dra = L("dra för att flytta", "drag to move");
    int tw = uiTextWidth(g, dra, u8g2_font_dv13_tf) + 12;
    g->fillRect(MW - 50 - tw, 458, tw, 20, P.pan);
    uiTextRight(g, MW - 56, 473, dra, P.dim, u8g2_font_dv13_tf);
  }
  if (!k.gps) {
    s4Box(g, 200, 206, 300, 66);
    uiTextCenter(g, 350, 234, L("Ingen GPS-position", "No GPS position"), P.txt, u8g2_font_dv17b_tf);
    uiTextCenter(g, 350, 258, L("styrsystemet skickar inte position", "the guidance system sends no position"), P.dim, u8g2_font_dv13_tf);
  }
  if (k.outside) {
    g->fillRect(180, 170, 340, 30, P.amb);
    uiTextCenter(g, 350, 191, L("Utanför kartans 4 km - nolla för ny plats", "Outside the 4 km map - reset for a new place"), RGB(20, 20, 20), u8g2_font_dv13b_tf);
  }
  if (s.kartaConfirm) {
    uiNBtn = 0;
    s4Dialog(g, 150, 110, 500, 260, L("Nolla kartan", "Reset the map"));
    uiTextCenter(g, 400, 178, L("Nolla kartan?", "Reset the map?"), P.txt, u8g2_font_dv24b_tf);
    char t[20]; uiAreaText(t, sizeof(t), k.areaHa);
    snprintf(b, sizeof(b), L("Spår, areal (%s) och bränsle tas bort", "Tracks, area (%s) and fuel are removed"), t);
    uiTextCenter(g, 400, 214, b, P.dim, u8g2_font_dv17_tf);
    s4KeyT(g, s, 176, 278, 210, 72, B_K_NOLLA_NO, L("AVBRYT", "CANCEL"), u8g2_font_dv20b_tf);
    s4KeyT(g, s, 414, 278, 210, 72, B_K_NOLLA_OK, L("NOLLA", "RESET"), u8g2_font_dv20b_tf, 1);
  }
}

// Rad i en inställningssida: rubrik, förklaring och skiljelinje under
static void uiRowPanel(Arduino_GFX *g, int y, int h, const char *title, const char *sub) {
  uiText(g, 18, y + (sub ? 30 : h / 2 + 7), title, P.txt, u8g2_font_dv17b_tf);
  if (sub) uiTextFit(g, 18, y + 54, sub, 380, P.dim, u8g2_font_dv13_tf);
  s4Sep(g, 2, y + h, 700);
}
// Väljare: "<" värde ">"
static void uiChooser(Arduino_GFX *g, const UiState &s, int y, int idM, int idP, const char *val, bool on) {
  s4KeyT(g, s, 410, y + 10, 60, 54, idM, "<", u8g2_font_dv24b_tf);
  s4Field(g, 478, y + 14, 140, 46, val, on ? P.txt : P.faint, u8g2_font_dv17_tf);
  s4KeyT(g, s, 630, y + 10, 60, 54, idP, ">", u8g2_font_dv24b_tf);
}

static void uiDrawKartInst(Arduino_GFX *g, const UiState &s) {
  const KartaView &k = s.karta;
  s4Page(g, L("Karta - inställningar", "Map - settings"), true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_KI_KLAR, false, icKlar, 70);
  bool man = k.trig == 0, iso = k.trig == 12;   // 0 = manuellt, 12 = ISOBUS (auto)
  int y = 30;
  uiRowPanel(g, y, 76, "Trigger", L("vad som startar och stoppar loggningen", "what starts and stops logging"));
  uiChooser(g, s, y, B_KI_TRIG_M, B_KI_TRIG_P, k.trigTxt, true);
  y += 80;
  uiRowPanel(g, y, 76, L("Startar", "Starts"), man ? L("START-knappen på kartan", "the START button on the map") : iso ? L("när redskapet arbetar", "when the implement is working") : L("loggningen börjar vid", "logging starts at"));
  uiChooser(g, s, y, B_KI_S_M, B_KI_S_P, (man || iso) ? "-" : k.startTxt, !(man || iso));
  y += 80;
  uiRowPanel(g, y, 76, L("Stoppar", "Stops"), man ? L("STOPP-knappen på kartan", "the STOP button on the map") : iso ? L("när redskapet slutar arbeta", "when the implement stops working") : L("samma som start = varannan gång", "same as start = every other time"));
  uiChooser(g, s, y, B_KI_P_M, B_KI_P_P, (man || iso) ? "-" : k.stopTxt, !(man || iso));
  y += 80;
  char b[48], w[16];
  uiFmt(w, sizeof(w), k.widthM, 1);
  if (k.cfgAutoW) snprintf(b, sizeof(b), k.widthFromBus ? L("från ISOBUS: %s m", "from ISOBUS: %s m") : L("ISOBUS saknas, använder %s m", "no ISOBUS, using %s m"), w);
  else snprintf(b, sizeof(b), "%s", L("manuell", "manual"));
  uiRowPanel(g, y, 76, L("Arbetsbredd", "Working width"), b);
  s4KeyT(g, s, 260, y + 10, 140, 54, B_KI_WMODE, k.cfgAutoW ? "AUTO" : L("MANUELL", "MANUAL"), u8g2_font_dv17b_tf, k.cfgAutoW ? 2 : 0);
  uiFmt(w, sizeof(w), k.cfgWidthM, 2); strcat(w, " m");
  s4KeyT(g, s, 410, y + 10, 60, 54, B_KI_W_M, "-", u8g2_font_dv24b_tf);
  s4Field(g, 478, y + 14, 140, 46, w, P.txt, u8g2_font_dv20_tf);
  uiReg(478, y + 10, 140, 54, B_KI_W_NUM);                                // v37: tryck = sifferfält i cm
  s4KeyT(g, s, 630, y + 10, 60, 54, B_KI_W_P, "+", u8g2_font_dv24b_tf);
  y += 80;
  uiRowPanel(g, y, 70, L("Bakgrund", "Background"), k.bakgrund ? L("enfärgad, som en styrbild", "plain, like a guidance screen") : L("satellitbild (Sentinel-2)", "satellite image (Sentinel-2)"));
  s4KeyT(g, s, 410, y + 8, 136, 54, B_KI_BG_SAT, L("SATELLIT", "SATELLITE"), u8g2_font_dv17b_tf, 0, k.bakgrund == 0);
  s4KeyT(g, s, 556, y + 8, 136, 54, B_KI_BG_ENF, L("ENFÄRGAD", "PLAIN"), u8g2_font_dv17b_tf, 0, k.bakgrund == 1);
  uiTextFit(g, 18, 448, k.tileTxt, 670, P.faint, u8g2_font_dv13_tf);
  uiTextFit(g, 18, 468, L("Manuell bredd används när AUTO är valt men ISOBUS inte skickar någon bredd.", "The manual width is used when AUTO is chosen but ISOBUS sends no width."), 670, P.faint, u8g2_font_dv13_tf);
}

// ---------------------------------------------------------------- Startval: traktor, ISOBUS eller terminal
static void uiValKnapp(Arduino_GFX *g, const UiState &s, int x, int id, const char *rubrik, const char *sub, int w,
                       S4Icon icon, bool av = false) {
  const int y = 100, h = 300;
  s4Key(g, s, x, y, w, h, av ? B_NONE : id, false, nullptr, 0);
  uint16_t c = av ? P.faint : P.ico;
  if (icon) icon(g, x + w / 2, y + 110, w * 55 / 100, c);
  uiTextCenter(g, x + w / 2, y + h - 52, rubrik, c, u8g2_font_dv24b_tf);
  uiTextCenter(g, x + w / 2, y + h - 22, sub, av ? P.faint : P.dim, u8g2_font_dv15_tf);
}
static void uiDrawVal(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(P.scr);
  s4Box(g, 2, 2, 796, 476);
  s4Head(g, 2, 2, 796, L("Vad vill du se?", "What do you want to see?"), true);
  char b[80];
  bool redskap = s.genHasImpl;
  if (redskap) snprintf(b, sizeof(b), L("ISOBUS-redskap hittat: %s", "ISOBUS implement found: %s"), s.genTitel);
  else snprintf(b, sizeof(b), "%s", L("Inget ISOBUS-redskap hittat", "No ISOBUS implement found"));
  uiTextCenter(g, 400, 66, b, P.dim, u8g2_font_dv17_tf);
  bool tre = redskap && s.vtAvail && uiVtRender;
  const int w = tre ? 240 : 360;
  const int x0 = tre ? 20 : 30, st = tre ? 260 : 380;
  uiValKnapp(g, s, x0, B_VAL_TRAKTOR, L("TRAKTOR", "TRACTOR"), L("hastighet, motor, lyft, karta", "speed, engine, hitch, map"), w, icTraktorS4);
  if (redskap) uiValKnapp(g, s, x0 + st, B_VAL_ISOBUS, "ISOBUS", L("redskapets värden", "implement values"), w, icRedskap);
  else uiValKnapp(g, s, x0 + st, B_VAL_ISOBUS, "ISOBUS", L("inget redskap hittat", "no implement found"), w, icRedskap, true);
  if (tre) {
    uiValKnapp(g, s, x0 + 2 * st, B_VAL_VT, "TERMINAL", L("redskapets egen bild", "the implement's own screen"), w, nullptr);
    uiVtRender(g, x0 + 2 * st + (w - 160) / 2, 124, 160, 0, 0, 0, 0);
  }
  if (!s.valHem) {                                     // v37: öppnat med huset = ingen nedräkning
    snprintf(b, sizeof(b), L("ISOBUS väljs själv om %d s", "ISOBUS is chosen automatically in %d s"), s.valSecs);
    uiTextCenter(g, 400, 450, b, P.dim, u8g2_font_dv15_tf);
  }
}

// ---------------------------------------------------------------- Rutorna på traktorsidan
// Info⁺ Trepunktslyft: FRAM till vänster, BAK till höger, som S4:s staplar och symboler
static void s4InfoLyft(Arduino_GFX *g, const TrView &v, int x, int y, int w, int h) {
  s4Head(g, x, y, w, L("Trepunktslyft", "Three-point hitch"), true);
  int mid = x + w / 2;
  g->drawFastVLine(mid, y + S4_HDR + 10, h - S4_HDR - 20, P.lin);
  char b[24];
  for (int side = 0; side < 2; side++) {
    bool bak = side == 1;
    int x0 = bak ? mid + 8 : x + 8, cw = w / 2 - 16;
    float lift = bak ? v.liftR : v.liftF, pto = bak ? v.ptoR : v.ptoF;
    int work = bak ? v.workR : v.workF;
    int dir = bak ? 1 : -1;
    // rad 1: lyftens läge (pil upp + arm), mörkblå stapel
    int ry = y + S4_HDR + 12;
    int icx = x0 + 22;
    g->fillRect(icx - 2 + dir * 6, ry + 2, 3, 26, P.txt);
    g->fillTriangle(icx - 7 + dir * 6, ry + 9, icx + 5 + dir * 6, ry + 9, icx - 1 + dir * 6, ry, P.txt);
    g->fillRect(icx - 9 + dir * 6, ry, 16, 2, P.txt);
    s4Link(g, icx - dir * 4, ry + 26, -dir, 80, 0, 20);
    if (isnan(lift)) snprintf(b, sizeof(b), "--"); else snprintf(b, sizeof(b), "%d%%", (int)lroundf(lift));
    s4Bar(g, x0 + 48, ry, cw - 48, 30, lift, b, true);
    // rad 2: kraftuttaget (kugghjul), varv
    ry += 50;
    bool on = !isnan(pto) && pto >= 1;
    s4Kugg(g, icx, ry + 15, 9, on ? P.grn : P.txt);
    if (isnan(pto)) snprintf(b, sizeof(b), "--");
    else if (!on) snprintf(b, sizeof(b), "%s", L("AV", "OFF"));
    else snprintf(b, sizeof(b), "%d", (int)lroundf(pto));
    s4Field(g, x0 + 48, ry, cw - 48, 30, b, on ? P.grnTxt : (isnan(pto) ? P.faint : P.txt), S4F_V, on ? P.grn : 0);
    // rad 3: i arbete
    ry += 50;
    s4Link(g, icx - dir * 10, ry + 22, dir, 0, work == 1 ? 1 : 0, 22);
    if (work == 1) s4Field(g, x0 + 48, ry, cw - 48, 30, L("ARBETE", "WORK"), P.grnTxt, S4F_MB, P.grn);
    else s4Field(g, x0 + 48, ry, cw - 48, 30, work == 0 ? "" : "--", P.faint, S4F_M);
    // nere: FRAM / BAK
    uiTextCenter(g, x0 + cw / 2, y + h - 12, bak ? L("bak", "rear") : L("fram", "front"), P.dim, S4F_T);
  }
}

// Info⁺ Hydraulventiler: S4:s rutor - pastellfärgad platta, cylinder i ventilens färg, svart ruta
static void s4InfoVentiler(Arduino_GFX *g, const TrView &v, int x, int y, int w, int h) {
  s4Head(g, x, y, w, L("Hydraulventiler", "Hydraulic valves"), true);
  int n = v.nValve;
  if (n == 0) { uiTextCenter(g, x + w / 2, y + h / 2 + 6, L("inga ventiler hörs", "no valves heard"), P.faint, S4F_T); return; }
  const int cols = 4;
  int tw = (w - 12) / cols, th = (h - S4_HDR - 10) / 2;
  for (int i = 0; i < n && i < 8; i++) {
    int tx = x + 6 + (i % cols) * tw, ty = y + S4_HDR + 6 + (i / cols) * th;
    uint16_t vc = ventilFarg(v.valveNo[i]);
    g->fillRect(tx + 2, ty, tw - 4, th - 4, uiMix(vc, P.pan, 3, 5));
    int cw = tw - 26;
    s4Cyl(g, tx + 6, ty + 6, cw, 13, vc);
    int bx = tx + 7, by = ty + 26, bw = tw - 18, bh = th - 36;
    g->fillRect(bx, by, bw, bh, RGB(14, 14, 14));
    // flödet: grönt ut (uppåt), gult in (nedåt), mittlinje
    int mid = by + bh / 2, half = bh / 2 - 4;
    float f = v.valveFlow[i];
    int hh = (int)(fabsf(f) / 100.0f * half); if (hh > half) hh = half;
    if (f > 0.5f) g->fillRect(bx + 8, mid - hh, bw - 16, hh, RGB(70, 200, 80));
    else if (f < -0.5f) g->fillRect(bx + 8, mid, bw - 16, hh, P.amb);
    g->drawFastHLine(bx + 4, mid, bw - 8, RGB(120, 120, 120));
    char b[12];
    if (fabsf(f) > 0.5f) { snprintf(b, sizeof(b), "%d", (int)lroundf(fabsf(f))); uiTextCenter(g, bx + bw / 2, by + bh - 4, b, RGB(255, 255, 255), S4F_S); }
    uiText(g, bx + 3, by + 14, ventilNamn(v.valveNo[i]), RGB(255, 255, 255), S4F_S);
  }
}

// Info⁺ Rådata: övriga meddelanden från bussen
static void s4InfoRadata(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h) {
  s4Head(g, x, y, w, L("Rådata", "Raw data"), true);
  uiText(g, x + 10, y + S4_HDR + 20, L("PGN  från  data", "PGN  from  data"), P.dim, S4F_S);
  if (s.nTrRaw == 0) uiText(g, x + 10, y + S4_HDR + 48, L("inga", "none"), P.faint, S4F_T);
  for (int i = 0; i < s.nTrRaw && i < 8; i++) {
    int ry = y + S4_HDR + 44 + i * 21;
    uiTextFit(g, x + 10, ry, s.trRaw[i], w - 20, P.txt, S4F_S);
    if (i) s4Sep(g, x, ry - 15, w);
  }
}

// Traktor (som S4:s ruta "Traktor"): symbolen med lyft och kraftuttag, sedan rader med värdefält
static void s4RutaTraktor(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h) {
  const TrView &v = s.trv;
  s4Head(g, x, y, w, L("Traktor", "Tractor"), false, icTraktorHdr);
  // andra listen (S4 har en blå list under rubriken): status för buss och WiFi
  g->fillRect(x + 4, y + S4_HDR + 1, 34, 18, P.hdr2);
  g->fillRect(x + 42, y + S4_HDR + 1, w - 46 - w / 3, 18, P.hdr);
  s4HGrad(g, x + w - 4 - w / 3, y + S4_HDR + 1, w / 3, 18, P.hdr, P.pan);
  uint16_t dot = !s.busOk ? P.sel : ((!isnan(v.kmh) || !isnan(v.rpm)) ? P.grn : P.amb);
  g->fillCircle(x + 21, y + S4_HDR + 10, 6, dot);
  uiText(g, x + 48, y + S4_HDR + 15, s.net.connected ? "WiFi" : L("ingen WiFi", "no WiFi"), P.dim, S4F_S);
  // v41: timmarna i listen (raden behövdes till slirningen)
  if (!isnan(v.hours)) {
    char t[20]; uiFmt(t, sizeof(t), v.hours, 0);
    icTimmar(g, x + 140, y + S4_HDR + 10, 15, P.txt);
    snprintf(t + strlen(t), sizeof(t) - strlen(t), " h");
    uiText(g, x + 152, y + S4_HDR + 15, t, P.txt, S4F_S);
  }

  // lutningen (v36) längst till höger i listen: sluttning fram/bak och liten bakvy för sidolutningen
  char b[24];
  {
    int ly = y + S4_HDR + 1, lx = x + w - 124;
    if (!isnan(v.lutFram)) {
      int hh = 3 + (int)lroundf(fminf(fabsf(v.lutFram), 10.0f) * 0.8f);
      if (v.lutFram >= 0) g->fillTriangle(lx, ly + 15, lx + 16, ly + 15, lx, ly + 15 - hh, P.txt);   // fronten (vänster) upp
      else g->fillTriangle(lx, ly + 15, lx + 16, ly + 15, lx + 16, ly + 15 - hh, P.txt);
      snprintf(b, sizeof(b), "%d°", (int)lroundf(fabsf(v.lutFram)));
      uiText(g, lx + 20, ly + 15, b, P.txt, S4F_S);
    }
    if (!isnan(v.lutSida)) {
      s4BakvyRot = fmaxf(-20.0f, fminf(20.0f, v.lutSida)) * 0.0174533f;
      icBakvy(g, lx + 70, ly + 8, 15, P.txt);
      snprintf(b, sizeof(b), "%d°", (int)lroundf(fabsf(v.lutSida)));
      uiText(g, lx + 84, ly + 15, b, P.txt, S4F_S);
    }
  }

  // symbolen: arm fram, traktorn, arm bak; kugghjul under armarna, värden i vinkelfälten.
  // v36: symbolen lutar med backen (fram/bak) och visar tända lampor.
  int sy = y + S4_HDR + 22;                     // överkant symbolområdet
  int cx = x + w / 2 + 2, cyS = sy + 30;
  float rot = isnan(v.lutFram) ? 0 : fmaxf(-15.0f, fminf(15.0f, v.lutFram)) * 0.0174533f;
  s4Ljus(g, cx, cyS, 70, rot, v.ljus, s.natt);
  s4Traktor(g, cx, cyS, 70, P.txt, P.pan, rot);
  s4LjusPrickar(g, cx, cyS, 70, rot, v.ljus, s.tick);
  int lfx, lfy, lbx, lby, kfx, kfy, kbx, kby;
  s4Rot(cx, cyS, rot, -48, 0, lfx, lfy);
  s4Rot(cx, cyS, rot, 50, 0, lbx, lby);
  s4Rot(cx, cyS, rot, -56, 18, kfx, kfy);
  s4Rot(cx, cyS, rot, 58, 18, kbx, kby);
  s4Link(g, lfx, lfy, -1, v.liftF, v.workF, 26);
  s4Link(g, lbx, lby, +1, v.liftR, v.workR, 26);
  bool onF = !isnan(v.ptoF) && v.ptoF >= 1, onR = !isnan(v.ptoR) && v.ptoR >= 1;
  s4Kugg(g, kfx, kfy, 7, onF ? P.grn : P.txt);
  s4Kugg(g, kbx, kby, 7, onR ? P.grn : P.txt);
  auto lyft = [&](float l) { if (isnan(l)) snprintf(b, sizeof(b), "--"); else snprintf(b, sizeof(b), "%d%%", (int)lroundf(l)); };
  auto pto = [&](float p) { if (isnan(p)) snprintf(b, sizeof(b), "--"); else if (p < 1) snprintf(b, sizeof(b), "%s", L("AV", "OFF")); else snprintf(b, sizeof(b), "%d", (int)lroundf(p)); };
  lyft(v.liftF); s4Field(g, x + 6, sy, 58, 24, b, isnan(v.liftF) ? P.faint : P.txt, S4F_M);
  pto(v.ptoF);   s4Field(g, x + 6, sy + 28, 58, 24, b, onF ? P.grnTxt : (isnan(v.ptoF) ? P.faint : P.txt), S4F_M, onF ? P.grn : 0);
  lyft(v.liftR); s4Field(g, x + w - 64, sy, 58, 24, b, isnan(v.liftR) ? P.faint : P.txt, S4F_M);
  pto(v.ptoR);   s4Field(g, x + w - 64, sy + 28, 58, 24, b, onR ? P.grnTxt : (isnan(v.ptoR) ? P.faint : P.txt), S4F_M, onR ? P.grn : 0);

  // värderader med skiljelinjer (S4: ikon, vinkelfält, enhet)
  int ry = sy + 60;
  const int rh = (y + h - ry) / 3;
  const int half = w / 2;
  auto rad = [&](int col, int row, S4Icon ic, const char *val, const char *unit, bool ok, bool green = false) {
    int bx = x + col * half, by = ry + row * rh;
    ic(g, bx + 20, by + rh / 2 + 1, 26, P.txt);
    int fw = half - 36 - 38;
    s4Field(g, bx + 36, by + 6, fw, rh - 10, val, green ? P.grnTxt : (ok ? P.txt : P.faint), S4F_V, green ? P.grn : 0);
    if (unit) uiText(g, bx + 36 + fw + 4, by + rh / 2 + 6, unit, P.txt, S4F_S);
  };
  for (int r = 0; r < 3; r++) s4Sep(g, x, ry + r * rh, w);
  uiFmt(b, sizeof(b), v.kmh, 1);   rad(0, 0, icFart, b, "km/h", !isnan(v.kmh));
  if (v.dir == 1) rad(1, 0, icRiktn, L("FRAM", "FWD"), nullptr, true, true);
  else if (v.dir == 0) rad(1, 0, icRiktn, L("BACK", "REV"), nullptr, true, false);
  else rad(1, 0, icRiktn, "--", nullptr, false);
  if (isnan(v.rpm)) snprintf(b, sizeof(b), "--"); else snprintf(b, sizeof(b), "%d", (int)lroundf(v.rpm / 10.0f) * 10);
  rad(0, 1, icVarv, b, L("v/m", "rpm"), !isnan(v.rpm));
  uiFmt(b, sizeof(b), v.fuel, 1);  rad(1, 1, icPump, b, "l/h", !isnan(v.fuel));
  // slirning (v41): (hjulfart - markfart) / hjulfart, gul från TR_SLIR_GUL och röd från TR_SLIR_ROD %
  {
    int bx = x, by = ry + 2 * rh, fw = half - 36 - 38;
    icSlir(g, bx + 20, by + rh / 2 + 1, 26, P.txt);
    uint16_t fill = 0, tc = isnan(v.slir) ? P.faint : P.txt;
    if (!isnan(v.slir) && v.slir >= TR_SLIR_ROD) { fill = P.sel; tc = RGB(255, 255, 255); }
    else if (!isnan(v.slir) && v.slir >= TR_SLIR_GUL) { fill = P.amb; tc = RGB(20, 20, 20); }
    uiFmt(b, sizeof(b), v.slir, 0);
    s4Field(g, bx + 36, by + 6, fw, rh - 10, b, tc, S4F_V, fill);
    uiText(g, bx + 36 + fw + 4, by + rh / 2 + 6, "%", P.txt, S4F_S);
  }
  // dragkraft i bakre lyften (v36): pilen växer med kraften, gul från TR_DRAG_GUL och röd från TR_DRAG_ROD kN
  {
    int bx = x + half, by = ry + 2 * rh, fw = half - 36 - 38;
    s4DragKN = v.dragKN;
    icDrag(g, bx + 20, by + rh / 2 + 1, 26, P.txt);
    uint16_t fill = 0, tc = isnan(v.dragKN) ? P.faint : P.txt;
    if (!isnan(v.dragKN) && v.dragKN >= TR_DRAG_ROD) { fill = P.sel; tc = RGB(255, 255, 255); }
    else if (!isnan(v.dragKN) && v.dragKN >= TR_DRAG_GUL) { fill = P.amb; tc = RGB(20, 20, 20); }
    uiFmt(b, sizeof(b), v.dragKN, 0);
    s4Field(g, bx + 36, by + 6, fw, rh - 10, b, tc, S4F_V, fill);
    uiText(g, bx + 36 + fw + 4, by + rh / 2 + 6, "kN", P.txt, S4F_S);
  }
}

// Karta som S4:s styrbild: svart list överst (GPS, areal i vit ruta, loggning), kartan, kompass
static void s4RutaKarta(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h) {
  const KartaView &k = s.karta;
  const int th = 34;
  g->fillRect(x, y, w, th, P.mapTop);
  // GPS-staplar (gröna som S4)
  for (int i = 0; i < 5; i++) {
    int bh = 4 + i * 4;
    uint16_t c = k.gps ? RGB(90, 210, 60) : RGB(80, 80, 80);
    g->fillRect(x + 8 + i * 7, y + th - 6 - bh, 5, bh, c);
  }
  uiText(g, x + 48, y + 22, k.gps ? "GPS" : L("ingen GPS", "no GPS"), RGB(230, 230, 230), S4F_TB);
  // arealen i en vit ruta (som S4:s "0,03")
  char b[32];
  uiAreaText(b, sizeof(b), k.areaHa);
  int bw = uiTextWidth(g, b, S4F_M) + 16, bx = x + w / 2 - bw / 2 + 20;
  g->fillRect(bx, y + 5, bw, 24, RGB(236, 238, 240));
  g->drawRect(bx, y + 5, bw, 24, RGB(120, 120, 120));
  uiTextCenter(g, bx + bw / 2, y + 23, b, RGB(20, 20, 20), S4F_M);
  // loggning: grön ring med ratt som S4:s styrsymbol
  int rx = x + w - 22, ryy = y + th / 2;
  uint16_t rc = k.logging ? RGB(190, 220, 40) : RGB(90, 90, 90);
  g->fillCircle(rx, ryy, 13, rc);
  g->fillCircle(rx, ryy, 10, P.mapTop);
  g->fillCircle(rx, ryy, 7, rc);
  g->fillRect(rx - 7, ryy - 1, 14, 3, P.mapTop);
  g->fillRect(rx - 1, ryy, 3, 8, P.mapTop);
  // kartan
  int mx = x, my = y + th, mw = w, mh = h - th;
  g->fillRect(mx, my, mw, mh, RGB(228, 230, 178));
  if (uiKartaRender) uiKartaRender(g, mx, my, mw, mh, 2.0f, false);
  // nere till vänster: bränsle per hektar (som S4:s "487 m")
  char l[20]; uiFmt(l, sizeof(l), k.lPerHa, 1);
  snprintf(b, sizeof(b), "%s l/ha", l);
  int lw = uiTextWidth(g, b, S4F_M) + 16;
  g->fillRect(mx + 4, my + mh - 30, lw, 26, uiMix(RGB(236, 238, 240), RGB(228, 230, 178), 1, 3));
  uiText(g, mx + 12, my + mh - 11, b, RGB(30, 30, 30), S4F_M);
  // kompass nere till höger (norr uppåt)
  int kx = mx + mw - 16, ky = my + mh - 24;
  g->fillTriangle(kx - 5, ky, kx + 5, ky, kx, ky - 16, RGB(220, 30, 30));
  g->fillTriangle(kx - 5, ky, kx + 5, ky, kx, ky + 16, RGB(250, 250, 250));
  g->drawLine(kx - 5, ky, kx, ky + 16, RGB(90, 90, 90));
  g->drawLine(kx + 5, ky, kx, ky + 16, RGB(90, 90, 90));
  if (!k.gps) {
    g->fillRect(mx + mw / 2 - 84, my + mh / 2 - 15, 168, 28, RGB(240, 240, 240));
    uiTextCenter(g, mx + mw / 2, my + mh / 2 + 5, L("ingen GPS-position", "no GPS position"), RGB(60, 60, 60), S4F_T);
  }
  uiReg(x, y, w, h, B_KARTA);
}

// Bränsle: l/h och l/min, staplar per minut de senaste 30 minuterna, ANALYS
static void s4RutaBransle(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h) {
  s4Head(g, x, y, w, L("Bränsle", "Fuel"), false, icPump);
  char b[24];
  int ry = y + S4_HDR + 6;
  uiFmt(b, sizeof(b), s.trv.fuel, 1);
  s4Field(g, x + 10, ry, 84, 30, b, isnan(s.trv.fuel) ? P.faint : P.txt);
  uiText(g, x + 98, ry + 21, "l/h", P.txt, S4F_S);
  float now = isnan(s.trv.fuel) ? NAN : s.trv.fuel / 60.0f;
  uiFmt(b, sizeof(b), now, 2);
  s4Field(g, x + 140, ry, 84, 30, b, isnan(now) ? P.faint : P.txt);
  uiText(g, x + 228, ry + 21, "l/min", P.txt, S4F_S);
  s4Sep(g, x, ry + 38, w);

  float hi = 0; int n = 0;
  for (int i = 0; i < CHART_POINTS; i++) if (!isnan(s.trFuelMin[i])) { hi = fmaxf(hi, s.trFuelMin[i]); n++; }
  static const float STEG[] = {0.25f, 0.5f, 1, 1.5f, 2, 3, 4, 5, 8, 10, 15, 20};
  float top = STEG[11];
  for (float st : STEG) if (hi <= st) { top = st; break; }
  const int px = x + 40, py = ry + 52, pw = w - 50, ph = y + h - 26 - py;
  s4L(g, px - 3, py - 3, pw + 3, ph + 4);
  g->drawFastHLine(px, py + ph / 2, pw, P.sep);
  uiFmt(b, sizeof(b), top, top < 1 ? 2 : (top < 2 ? 1 : 0));
  uiTextRight(g, px - 7, py + 9, b, P.dim, S4F_S);
  uiTextRight(g, px - 7, py + ph, "0", P.dim, S4F_S);
  uiText(g, x + 6, py + ph / 2 + 4, "l/min", P.dim, u8g2_font_dv11_tf);
  float bw = pw / (float)CHART_POINTS;
  for (int i = 0; i < CHART_POINTS; i++) {
    float v = s.trFuelMin[i];
    if (isnan(v)) continue;
    int bh = (int)(fminf(v, top) / top * ph + 0.5f);
    g->fillRect(px + (int)(i * bw) + 1, py + ph - bh, max(1, (int)bw - 2), bh, i == CHART_POINTS - 1 ? P.grn : P.barM);
  }
  if (n == 0) uiTextCenter(g, px + pw / 2, py + ph / 2 + 5, L("väntar på första minuten ...", "waiting for the first minute ..."), P.faint, S4F_S);
  uiText(g, px, y + h - 8, "-30 min", P.dim, S4F_S);
  uiTextRight(g, px + pw, y + h - 8, L("nu", "now"), P.dim, S4F_S);
  uiReg(x, py - 4, w, y + h - py + 4, B_TR_ANALYS);       // tryck på diagrammet = analys
}

// ---------------------------------------------------------------- Traktorsidan
static void uiDrawTraktor(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(P.scr);
  // vänsterkolumnen (v37): Info⁺ Trepunktslyft, Info⁺ Hydraulventiler, månen, huset, skiftnyckeln.
  // Kartan och analysen nås genom att trycka på kartrutan och bränslediagrammet; rådata finns under Diagnos.
  int info = s.trInfo == 1 ? 1 : 0;
  s4Key(g, s, S4_LX, S4_VY(0), S4_KW, S4_VKH, B_TR_INFO0, info == 0, icLyft, 58, "Info⁺");
  s4Key(g, s, S4_LX, S4_VY(1), S4_KW, S4_VKH, B_TR_INFO1, info == 1, icVentiler, 58, "Info⁺");
  s4VKol(g, s);

  // fyra rutor
  for (int r = 0; r < 2; r++) for (int c = 0; c < 2; c++) s4Box(g, S4_QX(c), S4_QY(r), S4_QW, S4_QH);
  if (info == 1) s4InfoVentiler(g, s.trv, S4_QX(0), S4_QY(0), S4_QW, S4_QH);
  else s4InfoLyft(g, s.trv, S4_QX(0), S4_QY(0), S4_QW, S4_QH);
  s4RutaTraktor(g, s, S4_QX(1), S4_QY(0), S4_QW, S4_QH);
  s4RutaKarta(g, s, S4_QX(0), S4_QY(1), S4_QW, S4_QH);
  s4RutaBransle(g, s, S4_QX(1), S4_QY(1), S4_QW, S4_QH);
  if (!s.busOk) uiNoBus(g);
}

// ---------------------------------------------------------------- Bränsleanalys i S4-stil
// Nås från traktorsidan: högerkolumnens dokumentationsknapp eller tryck på bränslediagrammet.
// Vänster kolumn: läge (EKONOMI / BALANS / BRÅTTOM) och PRISER. Höger kolumn som på traktorsidan.
static void icKr(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {           // prislapp med valutan
  float p[] = {(float)cx - 22, (float)cy - 12, (float)cx + 12, (float)cy - 12, (float)cx + 24, (float)cy,
               (float)cx + 12, (float)cy + 12, (float)cx - 22, (float)cy + 12};
  s4Poly(g, p, 5, c);
  g->fillCircle(cx + 12, cy, 3, P.keyHi);
  uiTextCenter(g, cx - 6, cy + 6, valuta, P.keyHi, S4F_TB);
}
static void icTidHalv(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {      // EKONOMI: klocka, halv
  icTimmar(g, cx, cy, sz, c);
  g->fillArc(cx, cy, sz * 40 / 100 - 3, 0, 270, 450, uiMix(c, P.keyHi, 1, 2));
}
static void icTidHel(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) { icTimmar(g, cx, cy, sz, c); }
static void icTidDubbel(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {    // BRÅTTOM: två klockor
  icTimmar(g, cx - 9, cy, sz * 8 / 10, c);
  icTimmar(g, cx + 11, cy, sz * 8 / 10, c);
}

static void uiDrawAnalys(Arduino_GFX *g, const UiState &s) {
  const AnView &a = s.an;
  g->fillScreen(P.scr);
  static const S4Icon LIK[3] = {icTidHalv, icTidHel, icTidDubbel};
  // vänster kolumn (v37, ingen högerkolumn): läget, priserna och tillbaka till traktorsidan
  for (int i = 0; i < 3; i++)
    s4Key(g, s, S4_LX, S4_VY(0) + i * 79, S4_KW, 74, B_AN_L0 + i, s.anLage == i, LIK[i], 34, nullptr, anLage(i));
  s4Key(g, s, S4_LX, S4_VY(0) + 3 * 79 + 4, S4_KW, 74, B_AN_PRIS, false, icKr, 50);
  s4Key(g, s, S4_LX, S4_VY(4), S4_KW, S4_VKH, B_AN_BACK, false, icTillbaka, 60);

  const int X = S4_CX0, W = 2 * S4_QW + 2;
  char b[96];
  // ---- nyckeltal (senaste 30 minuterna)
  int y = 2, h = 92;
  s4Box(g, X, y, W, h);
  s4Head(g, X, y, W, L("Bränsleanalys - senaste 30 min", "Fuel analysis - last 30 min"), true);
  {
    char v[5][16];
    uiFmt(v[0], 16, a.avgKmh, 1); uiFmt(v[1], 16, a.avgLh, 1); uiFmt(v[2], 16, a.avgLha, 1);
    uiFmt(v[3], 16, a.avgKrHa, 0); uiFmt(v[4], 16, a.haPerH, 2);
    static const char *const LSV[5] = {"Snitthast.", "Bränsle", "Per hektar", "Kostnad", "Kapacitet"};
    static const char *const LEN[5] = {"Avg speed", "Fuel", "Per hectare", "Cost", "Capacity"};
    const char *const *LAB = uiLang ? LEN : LSV;
    char krha[16]; snprintf(krha, sizeof(krha), "%s/ha", valuta);
    const char *ENH[5] = {"km/h", "l/h", "l/ha", krha, "ha/h"};
    static const S4Icon IK[5] = {icFart, icPump, icPump, icKr, icTimmar};
    int cw = W / 5;
    for (int i = 0; i < 5; i++) {
      int cx = X + i * cw;
      if (i) g->drawFastVLine(cx, y + S4_HDR + 8, h - S4_HDR - 14, P.sep);
      uiText(g, cx + 10, y + S4_HDR + 19, LAB[i], P.dim, S4F_S);
      bool ok = v[i][0] != '-';
      bool kost = i == 3;
      s4Field(g, cx + 8, y + S4_HDR + 27, cw - 52, 32, v[i], kost && ok ? P.grnTxt : (ok ? P.txt : P.faint), S4F_V, kost && ok ? P.grn : 0);
      uiText(g, cx + cw - 41, y + S4_HDR + 49, ENH[i], P.txt, S4F_S);
      (void)IK;
    }
  }
  // ---- kostnad per hektar vid olika fart
  y += h + 2; h = 202;
  s4Box(g, X, y, W, h);
  s4Head(g, X, y, W, L("Kostnad per hektar vid olika hastighet", "Cost per hectare at different speeds"), true);
  {
    // förklaring i rubrikraden: grön = lönsammast, gul = snålast
    int lx = X + W - 252;
    g->fillRect(lx, y + 8, 12, 12, P.grn); uiText(g, lx + 16, y + 19, L("lönsammast", "best value"), P.txt, S4F_S);
    g->fillRect(lx + 116, y + 8, 12, 12, P.amb); uiText(g, lx + 132, y + 19, L("snålast", "lowest fuel"), P.txt, S4F_S);
  }
  const int px = X + 56, py = y + S4_HDR + 28, pw = W - 70, ph = h - S4_HDR - 28 - 50;
  float hi = 0;
  for (int i = 0; i < a.nBin; i++) if (!isnan(a.binKrHa[i])) hi = fmaxf(hi, a.binKrHa[i]);
  s4L(g, px - 3, py - 3, pw + 3, ph + 4);
  if (a.nBin == 0 || hi <= 0) {
    uiTextCenter(g, px + pw / 2, py + ph / 2 + 6, L("inget arbete med hastighet och bränsle de senaste 30 minuterna", "no work with speed and fuel in the last 30 minutes"), P.faint, S4F_T);
  } else {
    static const float STEG[] = {50, 100, 200, 300, 400, 500, 750, 1000, 1500, 2000, 3000, 5000, 10000};
    float top = 10000;
    for (float st : STEG) if (hi <= st) { top = st; break; }
    g->drawFastHLine(px, py + ph / 2, pw, P.sep);
    uiFmt(b, sizeof(b), top, 0);     uiTextRight(g, px - 8, py + 8, b, P.dim, S4F_S);
    uiFmt(b, sizeof(b), top / 2, 0); uiTextRight(g, px - 8, py + ph / 2 + 5, b, P.dim, S4F_S);
    uiTextRight(g, px - 8, py + ph, "0", P.dim, S4F_S);
    { char kh[16]; snprintf(kh, sizeof(kh), "%s/ha", valuta); uiText(g, X + 6, py - 8, kh, P.dim, S4F_S); }
    uiText(g, X + 6, py + ph + 20, "km/h", P.dim, S4F_S);
    uiText(g, X + 6, py + ph + 41, "l/ha", P.dim, S4F_S);
    int slot = pw / max(a.nBin, 6);
    int bw = min(slot - 8, 66);
    for (int i = 0; i < a.nBin; i++) {
      int bx = px + i * slot + (slot - bw) / 2;
      bool good = a.binSek[i] >= 60;
      snprintf(b, sizeof(b), "%d-%d", a.binLo[i], a.binLo[i] + 1);
      uiTextCenter(g, bx + bw / 2, py + ph + 20, b, good ? P.txt : P.faint, S4F_T);
      if (isnan(a.binKrHa[i])) continue;
      uiFmt(b, sizeof(b), a.binLha[i], 1);
      if (i == a.bestL) { int tw = uiTextWidth(g, b, S4F_MB) + 10; g->fillRect(bx + bw / 2 - tw / 2, py + ph + 26, tw, 21, P.amb); }
      uiTextCenter(g, bx + bw / 2, py + ph + 42, b, i == a.bestL ? RGB(20, 20, 20) : (good ? P.txt : P.faint), i == a.bestL ? S4F_MB : S4F_M);
      int hh = (int)(fminf(a.binKrHa[i], top) / top * (ph - 4) + 0.5f);
      uint16_t c = i == a.best ? P.grn : (i == a.bestL ? P.amb : (good ? P.barL : P.sep));
      g->fillRect(bx, py + ph - hh, bw, hh, c);
      uiFmt(b, sizeof(b), a.binKrHa[i], 0);
      if (hh > 26) uiTextCenter(g, bx + bw / 2, py + ph - hh + 19, b, i == a.bestL ? RGB(20, 20, 20) : RGB(255, 255, 255), S4F_MB);
      else uiTextCenter(g, bx + bw / 2, py + ph - hh - 4, b, P.txt, S4F_MB);
    }
  }
  // ---- rekommendationer
  y += h + 2; h = 480 - 2 - y;
  s4Box(g, X, y, W, h);
  s4Head(g, X, y, W, L("Rekommendation", "Recommendation"), true);
  {
    char pd[16], pt[16];
    uiFmt(pd, sizeof(pd), s.anDiesel, 1); uiFmt(pt, sizeof(pt), s.anTimKr, 0);
    snprintf(b, sizeof(b), "%s %s/l  ·  %s %s/h  ·  %s", pd, valuta, pt, valuta, anLage(s.anLage));
    uiTextRight(g, X + W - 10, y + 19, b, P.dim, S4F_S);
  }
  int ry = y + S4_HDR + 4;
  const int rh2 = 21;
  for (int i = 0; i < a.nRek && i < 5; i++) {
    int yy = ry + i * rh2;
    if (i) s4Sep(g, X, yy, W);
    if (i == 0) {
      g->fillRect(X + 6, yy + 2, 6, rh2 - 3, P.grn);
      uiTextFit(g, X + 18, yy + 16, a.rek[i], W - 28, P.txt, S4F_TB);
    } else uiTextFit(g, X + 18, yy + 16, a.rek[i], W - 28, P.dim, S4F_S);
  }
  if (a.nRek == 0) uiText(g, X + 18, ry + 18, L("Ingen data från traktorn än.", "No data from the tractor yet."), P.faint, S4F_T);
  char wb[16]; uiFmt(wb, sizeof(wb), a.avgW, 1);
  char vl[16]; uiFmt(vl, sizeof(vl), a.vandL, 1);
  snprintf(b, sizeof(b), L("Arbete %d av 30 min (%s), varav vändningar och stopp %d min (%s l).", "Working %d of 30 min (%s), of which turns and stops %d min (%s l)."),
           a.sekWork / 60, a.workFromMap ? L("kartan loggar", "map logging") : L("över 1 km/h", "above 1 km/h"), a.sekVand / 60, vl);
  uiTextFit(g, X + 10, y + h - 26, b, W - 20, P.faint, S4F_S);
  snprintf(b, sizeof(b), L("Uppehåll upp till 30 s räknas som arbete. Bredd %s m. Tiden räknas %s.", "Pauses up to 30 s count as work. Width %s m. Time counts %s."), wb,
           s.anLage == 0 ? L("till halva kostnaden", "at half cost") : (s.anLage == 2 ? L("dubbelt", "double") : L("som den kostar", "at its cost")));
  uiTextFit(g, X + 10, y + h - 7, b, W - 20, P.faint, S4F_S);

  // ---- prisrutan
  if (s.anPris) {
    for (int yy = 0; yy < 480; yy += 2)
      for (int xx = (yy / 2) & 1; xx < 800; xx += 2) g->drawPixel(xx, yy, P.scr);
    const int dx = 180, dy = 100, dw = 440, dh = 280;
    g->fillRect(dx - 2, dy - 2, dw + 4, dh + 4, P.scr);
    s4Box(g, dx, dy, dw, dh);
    s4Head(g, dx, dy, dw, L("Priser", "Prices"), true);
    char pd[16], pt[16];
    uiFmt(pd, sizeof(pd), s.anDiesel, 1); uiFmt(pt, sizeof(pt), s.anTimKr, 0);
    icPump(g, dx + 28, dy + 72, 30, P.txt);
    uiText(g, dx + 50, dy + 66, "Diesel", P.txt, S4F_T);
    { char t[24]; snprintf(t, sizeof(t), L("%s per liter", "%s per litre"), valuta); uiText(g, dx + 50, dy + 86, t, P.dim, S4F_S); }
    s4Key(g, s, dx + 170, dy + 46, 60, 52, B_AN_D_M, false, nullptr, 0, nullptr, "-");
    s4Field(g, dx + 238, dy + 52, 110, 38, pd, P.txt);
    s4Key(g, s, dx + 360, dy + 46, 60, 52, B_AN_D_P, false, nullptr, 0, nullptr, "+");
    s4Sep(g, dx, dy + 112, dw);
    s4Traktor(g, dx + 28, dy + 142, 36, P.txt, P.pan);
    uiText(g, dx + 50, dy + 136, L("Traktor + förare", "Tractor + driver"), P.txt, S4F_T);
    { char t[24]; snprintf(t, sizeof(t), L("%s per timme", "%s per hour"), valuta); uiText(g, dx + 50, dy + 156, t, P.dim, S4F_S); }
    s4Key(g, s, dx + 200, dy + 160, 60, 52, B_AN_T_M, false, nullptr, 0, nullptr, "-");
    s4Field(g, dx + 268, dy + 166, 80, 38, pt, P.txt);
    s4Key(g, s, dx + 360, dy + 160, 60, 52, B_AN_T_P, false, nullptr, 0, nullptr, "+");
    s4Key(g, s, dx + dw - 130, dy + dh - 56, 120, 46, B_AN_PRIS_OK, false, nullptr, 0, nullptr, L("KLAR", "DONE"));
  }
  if (!s.busOk) uiNoBus(g);
}

// ---------------------------------------------------------------- Redskapets egen bild (spegling av terminalen)
static void uiDrawVt(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(RGB(20, 20, 20));
  if (s.vtAvail && uiVtRender) {
    uiVtRender(g, 0, 0, 480, 486, 0, 244, 480);
  } else {
    s4Box(g, 20, 140, 440, 200);
    s4Head(g, 20, 140, 440, "Terminal", true);
    uiTextCenter(g, 240, 196, L("Redskapets bild finns inte än", "The implement's screen is not captured yet"), P.txt, u8g2_font_dv17b_tf);
    uiTextCenter(g, 240, 226, L("Den fångas när redskapet laddar upp", "It is captured when the implement uploads"), P.dim, u8g2_font_dv15_tf);
    uiTextCenter(g, 240, 248, L("sin bild till traktorns terminal.", "its screen to the tractor terminal."), P.dim, u8g2_font_dv15_tf);
    uiTextCenter(g, 240, 284, L("Radera redskapets sparade bild i terminalen", "Delete the implement's stored pool in the terminal"), P.faint, u8g2_font_dv13_tf);
    uiTextCenter(g, 240, 304, L("och starta om, så skickas den på nytt.", "and restart, so it is sent again."), P.faint, u8g2_font_dv13_tf);
  }
  // TILLBAKA (smal kolumn längst till höger)
  int x = 736, y = 6, w = 58, h = 100;
  s4Key(g, s, x, y, w, h, B_VT_BACK, false, icTillbaka, 50);
  // lampa: grön = redskapet uppdaterar bilden just nu
  g->fillCircle(x + w / 2, 140, 7, !s.busOk ? P.sel : (s.vtLive ? P.grn : P.amb));
  uiTextCenter(g, x + w / 2, 166, "LIVE", RGB(170, 170, 170), u8g2_font_dv11_tf);
  uiTextCenter(g, x + w / 2, 440, L("bara", "view"), RGB(150, 150, 150), u8g2_font_dv11_tf);
  uiTextCenter(g, x + w / 2, 456, L("visning", "only"), RGB(150, 150, 150), u8g2_font_dv11_tf);
  if (!s.busOk) uiNoBus(g);
}

// ---------------------------------------------------------------- Viloläge
static void uiDrawIdle(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(P.pan);
  uiLogo(g, (800 - LOGO_BIG_W) / 2, 30, LOGO_BIG_W, LOGO_BIG_H, LOGO_BIG, P.txt, P.pan);
  char w[48], full[48];
  snprintf(w, sizeof(w), "%s", L("Väntar på ISOBUS", "Waiting for ISOBUS"));
  snprintf(full, sizeof(full), "%s . . .", w);
  int nd = (s.tick / 3) % 4;
  for (int i = 0; i < nd; i++) strcat(w, " .");
  uiText(g, 400 - uiTextWidth(g, full, u8g2_font_dv20b_tf) / 2, 400, w, P.dim, u8g2_font_dv20b_tf);
  if (s.net.connected) {
    uiBars(g, 16, 462, s.net.rssi, P.faint);
    uiText(g, 56, 460, s.net.ssid, P.faint, u8g2_font_dv13_tf);
  }
  char vb[48];
  if (s.net.namn[0]) snprintf(vb, sizeof(vb), "%s  ·  v%d", s.net.namn, s.version);
  else snprintf(vb, sizeof(vb), "v%d", s.version);
  uiTextRight(g, 784, 460, vb, P.faint, u8g2_font_dv13_tf);
  if (s.net.namn[0]) uiText(g, 18, 44, s.net.namn, P.dim, u8g2_font_dv24b_tf);   // vilken skärm det är (v35)
  s4Key(g, s, 722, 6, 72, 64, B_SETTINGS, false, icInst, 54);
}

// ---------------------------------------------------------------- Inställningar
static void uiStepper(Arduino_GFX *g, const UiState &s, int y, int idMinus, int idPlus, const char *val, int idVal = B_NONE) {
  s4KeyT(g, s, 470, y + 8, 60, 48, idMinus, "-", u8g2_font_dv24b_tf);
  s4Field(g, 538, y + 10, 92, 44, val, P.txt, u8g2_font_dv17_tf);
  if (idVal != B_NONE) uiReg(538, y + 8, 92, 48, idVal);               // v37: tryck på värdet = sifferfält
  s4KeyT(g, s, 638, y + 8, 60, 48, idPlus, "+", u8g2_font_dv24b_tf);
}
static void icMer(Arduino_GFX *g, int cx, int cy, int sz, uint16_t c) {         // tre prickar (mer)
  int r = max(3, sz / 9);
  for (int i = -1; i <= 1; i++) g->fillCircle(cx + i * r * 3, cy, r, c);
}
static void uiDrawSettings(Arduino_GFX *g, const UiState &s) {
  s4Page(g, L("Inställningar", "Settings"), true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_CLOSE, false, icKlar, 70);
  s4Key(g, s, S4_RX, 122, S4_KW, 108, B_MAN_OPEN, false, icBok, 46, nullptr, "MANUAL");
  s4Key(g, s, S4_RX, 238, S4_KW, 108, B_ENH_OPEN, false, icChip, 46, nullptr, L("ENHET", "DEVICE"));
  s4Key(g, s, S4_RX, 354, S4_KW, 108, B_MER_OPEN, false, icMer, 46, nullptr, L("MER", "MORE"));
  char b[96];
  const int rh = 64;
  int y = 28;
  // Ljusstyrka
  uiRowPanel(g, y, rh, L("Ljusstyrka", "Brightness"), L("dämpar färgerna, bakgrundsljuset är alltid på", "dims the colours, the backlight is always on"));
  snprintf(b, sizeof(b), "%d %%", s.brightness);
  uiStepper(g, s, y, B_BRI_MINUS, B_BRI_PLUS, b);
  // Dag / natt
  y += rh;
  uiRowPanel(g, y, rh, L("Dag / natt", "Day / night"), L("ljusa färger på dagen, mörka på natten", "light colours by day, dark at night"));
  s4KeyT(g, s, 470, y + 8, 106, 48, s.natt ? B_NATT : B_NONE, L("DAG", "DAY"), u8g2_font_dv17b_tf, 0, !s.natt);
  s4KeyT(g, s, 592, y + 8, 106, 48, s.natt ? B_NONE : B_NATT, L("NATT", "NIGHT"), u8g2_font_dv17b_tf, 0, s.natt);
  // Språk
  y += rh;
  uiRowPanel(g, y, rh, L("Språk", "Language"), L("svenska eller engelska", "Swedish or English"));
  s4KeyT(g, s, 448, y + 8, 120, 48, B_SPRAK_SV, "SVENSKA", u8g2_font_dv17b_tf, 0, s.lang == 0);
  s4KeyT(g, s, 578, y + 8, 120, 48, B_SPRAK_EN, "ENGLISH", u8g2_font_dv17b_tf, 0, s.lang == 1);
  // Arbetsbredd (tryck på värdet för att skriva in centimeter)
  y += rh;
  uiRowPanel(g, y, rh, L("Arbetsbredd", "Working width"), s.widthManualMode ? L("alltid manuell · tryck på värdet för cm", "always manual · tap the value for cm")
                                                                         : L("från ISOBUS om den finns", "from ISOBUS when available"));
  s4KeyT(g, s, 300, y + 8, 150, 48, B_W_MODE, s.widthManualMode ? L("MANUELL", "MANUAL") : "AUTO", u8g2_font_dv17b_tf,
         s.widthManualMode ? 0 : 2);
  uiFmt(b, sizeof(b), s.manualWidth, 2);
  strcat(b, " m");
  uiStepper(g, s, y, B_W_MINUS, B_W_PLUS, b, B_W_NUM);
  // WiFi
  y += rh;
  if (s.net.connected) snprintf(b, sizeof(b), L("Ansluten: %s", "Connected: %s"), s.net.ssid);
  else if (s.net.hasSaved) snprintf(b, sizeof(b), "%s", L("Inte ansluten - söker sparade nät", "Not connected - looking for saved networks"));
  else snprintf(b, sizeof(b), "%s", L("Inget nät valt", "No network chosen"));
  uiRowPanel(g, y, rh, "WiFi", b);
  if (s.net.connected) uiBars(g, 420, y + 48, s.net.rssi, P.grn);
  s4KeyT(g, s, 498, y + 8, 200, 48, B_WIFI, L("VÄLJ NÄT", "CHOOSE"), u8g2_font_dv17b_tf);
  // ISOBUS
  y += rh;
  snprintf(b, sizeof(b), "TC %s · %s", s.tcSeen ? L("hittad", "found") : L("saknas", "missing"), s.genHasImpl ? s.genTitel : L("inget redskap", "no implement"));
  uiText(g, 18, y + 30, "ISOBUS", P.txt, u8g2_font_dv17b_tf);
  uiTextFit(g, 18, y + 54, b, 200, P.dim, u8g2_font_dv13_tf);
  s4KeyT(g, s, 226, y + 8, 152, 48, B_TRAKTOR, L("TRAKTOR", "TRACTOR"), u8g2_font_dv17b_tf);
  s4KeyT(g, s, 386, y + 8, 152, 48, B_REDSKAP, L("REDSKAP", "IMPLEMENT"), u8g2_font_dv17b_tf);
  s4KeyT(g, s, 546, y + 8, 152, 48, B_DIAG, L("DIAGNOS", "DIAGNOSTICS"), u8g2_font_dv15b_tf);
  s4Sep(g, 2, y + rh, 700);
  // Info och demo
  y += rh;
  if (s.net.namn[0])
    snprintf(b, sizeof(b), L("%s  ·  version %d  ·  SD-kort %s", "%s  ·  version %d  ·  SD card %s"), s.net.namn, s.version, s.net.sdOk ? "OK" : L("SAKNAS", "MISSING"));
  else snprintf(b, sizeof(b), L("Version %d  ·  SD-kort %s", "Version %d  ·  SD card %s"), s.version, s.net.sdOk ? "OK" : L("SAKNAS", "MISSING"));
  if (s.net.latestVersion > s.version && s.net.uppd) {      // en nyare version finns men har inte installerats
    char nb[32]; snprintf(nb, sizeof(nb), L("  ·  v%d finns", "  ·  v%d available"), s.net.latestVersion);
    strncat(b, nb, sizeof(b) - strlen(b) - 1);
  }
  uiTextFit(g, 18, y + 24, b, 520, P.dim, u8g2_font_dv13_tf);
  uiTextFit(g, 18, y + 46, L("Gratis, öppen källkod (GPLv3) · används på eget ansvar · se MER", "Free, open source (GPLv3) · use at your own risk · see MORE"), 520, P.faint, u8g2_font_dv13_tf);
  s4KeyT(g, s, 558, y + 6, 140, 48, B_DEMO, s.demo ? L("DEMO AV", "DEMO OFF") : "DEMO", u8g2_font_dv17b_tf, s.demo ? 1 : 0);
}

// ---------------------------------------------------------------- MER: uppdateringar, loggning, feedback, om
static void uiDrawMer(Arduino_GFX *g, const UiState &s) {
  s4Page(g, L("Inställningar - mer", "Settings - more"), true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_MER_BACK, false, icTillbaka, 60);
  char b[96];
  const int rh = 80;
  int y = 28;
  // Uppdateringar
  if (!s.net.uppd) snprintf(b, sizeof(b), "%s", L("AV - letar aldrig efter nya versioner", "OFF - never looks for new versions"));
  else if (s.net.latestVersion > s.version) snprintf(b, sizeof(b), L("v%d finns - frågas när skärmen får internet", "v%d available - asked when the display gets internet"), s.net.latestVersion);
  else snprintf(b, sizeof(b), "%s", L("frågar alltid innan något installeras", "always asks before installing anything"));
  uiRowPanel(g, y, rh, L("Uppdateringar", "Updates"), b);
  s4KeyT(g, s, 470, y + 14, 106, 52, B_UPPD_PA, L("FRÅGA", "ASK"), u8g2_font_dv17b_tf, 0, s.net.uppd);
  s4KeyT(g, s, 592, y + 14, 106, 52, B_UPPD_AV, L("AV", "OFF"), u8g2_font_dv17b_tf, 0, !s.net.uppd);
  // Loggning
  y += rh;
  uiRowPanel(g, y, rh, L("Loggning", "Logging"), s.loggAv ? L("AV - ingen inspelning av bussen", "OFF - the bus is not recorded")
                                                          : L("PÅ - all ISOBUS-trafik sparas på SD-kortet", "ON - all ISOBUS traffic is saved on the SD card"));
  s4KeyT(g, s, 470, y + 14, 106, 52, B_LOGG_PA, L("PÅ", "ON"), u8g2_font_dv17b_tf, 0, !s.loggAv);
  s4KeyT(g, s, 592, y + 14, 106, 52, B_LOGG_AV, L("AV", "OFF"), u8g2_font_dv17b_tf, 0, s.loggAv);
  // Feedback
  y += rh;
  uiRowPanel(g, y, rh, "Feedback", L("fel, frågor och förslag - besvaras i mån av tid", "bugs, questions and ideas - answered when time allows"));
  s4KeyT(g, s, 470, y + 14, 228, 52, B_FB_OPEN, L("SKICKA FEEDBACK", "SEND FEEDBACK"), u8g2_font_dv17b_tf);
  // Om
  y += rh;
  uiRowPanel(g, y, rh, L("Om Fältloggen", "About Fältloggen"), L("ansvar, licenser och vem som inte står bakom", "liability, licences and who is not behind it"));
  s4KeyT(g, s, 470, y + 14, 228, 52, B_OM_OPEN, L("ANSVAR & LICENSER", "LIABILITY & LICENCES"), u8g2_font_dv15b_tf);
  // fast text längst ner
  y += rh + 18;
  uiWrap(g, 18, y, 666, 19, L(
    "Fältloggen är gratis och öppen källkod (GPLv3). Den är byggd helt utan inblandning från Fendt, AGCO "
    "eller någon annan tillverkare och är inte godkänd eller stödd av dem. Allt används på eget ansvar.",
    "Fältloggen is free and open source (GPLv3). It was built entirely without involvement from Fendt, AGCO "
    "or any other manufacturer and is not approved or supported by them. Everything is used at your own risk."),
    P.dim, u8g2_font_dv13_tf);
}

// ---------------------------------------------------------------- Feedback: QR-kod till arkivets ärenden
static void uiQr(Arduino_GFX *g, int x, int y, int sz, const char *text);   // längre ner
static void uiDrawFeedback(Arduino_GFX *g, const UiState &s) {
  s4Page(g, "Feedback", true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_FB_BACK, false, icTillbaka, 60);
  uiQr(g, 24, 44, 260, s.net.fbUrl);
  int y = uiWrap(g, 306, 66, 380, 22, L(
    "Skanna koden med telefonens kamera.\n"
    "Där kan du rapportera fel, ställa frågor och föreslå förbättringar. Du behöver ett gratis GitHub-konto.\n"
    "Skriv gärna vilken version du har och vilken traktor och vilka redskap det gäller.",
    "Scan the code with your phone camera.\n"
    "There you can report bugs, ask questions and suggest improvements. You need a free GitHub account.\n"
    "Please mention your version and which tractor and implements it concerns."), P.txt, u8g2_font_dv15_tf);
  char b[48]; snprintf(b, sizeof(b), L("Din version: %d", "Your version: %d"), s.version);
  uiText(g, 306, y + 10, b, P.txt, u8g2_font_dv15b_tf);
  uiTextFit(g, 24, 336, s.net.fbUrl, 660, P.dim, u8g2_font_dv13_tf);
  s4Sep(g, 2, 352, 700);
  uiWrap(g, 24, 382, 660, 20, L(
    "Fältloggen görs på fritiden. Feedback läses och besvaras i mån av tid - det finns inget löfte om svar, "
    "rättning eller support. Skriv aldrig lösenord eller annat känsligt i ett ärende: allt där är offentligt.",
    "Fältloggen is made in spare time. Feedback is read and answered when time allows - there is no promise "
    "of an answer, a fix or support. Never write passwords or anything sensitive in an issue: it is all public."),
    P.dim, u8g2_font_dv13_tf);
}

// ---------------------------------------------------------------- Enhetsinfo (v44)
// Temperatur, minne, SD-kort, nät och buss. Värdena fylls av Faltloggen.ino (enhFill) och ritas om två gånger
// i sekunden som alla sidor. Gränserna för temperaturen gäller chippet, som alltid är varmare än luften runt kortet.
#define ENH_TEMP_GUL 70.0f
#define ENH_TEMP_ROD 80.0f
static void uiEnhRad(Arduino_GFX *g, int x, int y, int w, const char *lab, const char *val, uint16_t vc = 0) {
  uiText(g, x + 10, y + 19, lab, P.dim, S4F_S);
  int lw = uiTextWidth(g, lab, S4F_S);
  int vw = uiTextWidth(g, val, S4F_MB);
  if (vw <= w - 30 - lw) uiTextRight(g, x + w - 10, y + 20, val, vc ? vc : P.txt, S4F_MB);
  else { int fw = w - 30 - lw; uiTextFit(g, x + w - 10 - fw, y + 19, val, fw, vc ? vc : P.txt, S4F_S); }
}
static void uiEnhStapel(Arduino_GFX *g, int x, int y, int w, const char *lab, float anvPct, const char *txt) {
  uiText(g, x + 10, y + 19, lab, P.dim, S4F_S);
  int bx = x + 96, bw = w - 106, bh = 26;           // stapeln = använd del, ljus så att texten syns över den
  s4L(g, bx, y + 2, bw, bh);
  if (!isnan(anvPct)) {
    float f = anvPct / 100.0f; if (f < 0) f = 0; if (f > 1) f = 1;
    int fw = (int)lroundf((bw - 3) * f);
    uint16_t c = anvPct >= 90 ? uiMix(P.sel, P.pan, 1, 2) : uiMix(P.barL, P.pan, 1, 2);
    if (fw > 0) g->fillRect(bx + 3, y + 5, fw, bh - 5, c);
  }
  int tw = uiTextWidth(g, txt, S4F_S);
  uiText(g, bx + 3 + (bw - 3 - tw) / 2, y + 2 + bh / 2 + 6, txt, P.txt, S4F_S);
}
static void uiFmtMB(char *b, int n, uint32_t mb) {        // 950 MB / 12,3 GB
  if (mb < 1000) snprintf(b, n, "%lu MB", (unsigned long)mb);
  else { char t[16]; uiFmt(t, sizeof(t), mb / 1024.0f, mb < 102400 ? 1 : 0); snprintf(b, n, "%s GB", t); }
}
static void uiDrawEnhet(Arduino_GFX *g, const UiState &s) {
  const EnhView &e = s.enh;
  char b[128], t1[24], t2[24];
  g->fillScreen(P.scr);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_ENH_BACK, false, icTillbaka, 50);
  const int xL = 2, xR = 354, w = 346, yT = 2, hT = 234, yB = 240, hB = 238;

  // ---- Temperatur
  s4Box(g, xL, yT, w, hT);
  s4Head(g, xL, yT, w, L("Temperatur", "Temperature"), false, icChip);
  uint16_t bg = P.grn, fg = RGB(255, 255, 255);
  if (isnan(e.tempC)) { bg = 0; fg = P.faint; strcpy(b, "--"); }
  else {
    uiFmt(t1, sizeof(t1), e.tempC, 1); snprintf(b, sizeof(b), "%s °C", t1);
    if (e.tempC >= ENH_TEMP_ROD) bg = P.sel; else if (e.tempC >= ENH_TEMP_GUL) bg = P.amb;
  }
  if (isnan(e.tempC)) s4Field(g, xL + 70, yT + 40, w - 140, 64, b, fg, S4F_V);
  else {                                      // stora siffror (dv40 har bara siffror), "°C" med vanligt typsnitt
    s4Field(g, xL + 70, yT + 40, w - 140, 64, "", fg, S4F_V, bg);
    int tw = uiTextWidth(g, t1, u8g2_font_dv40_tn), uw = uiTextWidth(g, " °C", S4F_V);
    int tx = xL + 70 + (w - 140 - tw - uw) / 2;
    uiText(g, tx, yT + 91, t1, fg, u8g2_font_dv40_tn);
    uiText(g, tx + tw, yT + 91, " °C", fg, S4F_V);
  }
  int y = yT + 116;
  if (isnan(e.tempMin)) strcpy(b, "--");
  else { uiFmt(t1, sizeof(t1), e.tempMin, 1); uiFmt(t2, sizeof(t2), e.tempMax, 1); snprintf(b, sizeof(b), "%s / %s °C", t1, t2); }
  uiEnhRad(g, xL, y, w, L("Lägst / högst sedan start", "Lowest / highest since start"), b);
  s4Sep(g, xL, y + 30, w);
  uiTextFit(g, xL + 10, y + 52, L("Chippets egen temperatur - den är alltid", "The chip's own temperature - always"), w - 20, P.faint, S4F_S);
  uiTextFit(g, xL + 10, y + 70, L("högre än luften runt skärmen. Kortet är", "higher than the air around the display."), w - 20, P.faint, S4F_S);
  snprintf(b, sizeof(b), L("gjort för 0-65 °C luft. Gult från %d, rött från %d °C.", "Board rated 0-65 °C. Yellow %d, red %d °C."), (int)ENH_TEMP_GUL, (int)ENH_TEMP_ROD);
  uiTextFit(g, xL + 10, y + 88, b, w - 20, P.faint, S4F_S);

  // ---- Minne
  s4Box(g, xR, yT, w, hT);
  s4Head(g, xR, yT, w, L("Minne", "Memory"), false);
  y = yT + 34;
  float p = e.psTotKB ? 100.0f * (e.psTotKB - e.psFreeKB) / e.psTotKB : NAN;
  uiFmt(t1, sizeof(t1), e.psFreeKB / 1024.0f, 1); uiFmt(t2, sizeof(t2), e.psTotKB / 1024.0f, 1);
  snprintf(b, sizeof(b), L("%s av %s MB ledigt", "%s of %s MB free"), t1, t2);
  uiEnhStapel(g, xR, y, w, "PSRAM", p, b);
  y += 36;
  p = e.inTotKB ? 100.0f * (e.inTotKB - e.inFreeKB) / e.inTotKB : NAN;
  snprintf(b, sizeof(b), L("%lu av %lu kB ledigt", "%lu of %lu kB free"), (unsigned long)e.inFreeKB, (unsigned long)e.inTotKB);
  uiEnhStapel(g, xR, y, w, L("Internt", "Internal"), p, b);
  y += 36;
  if (!s.net.sdOk) uiEnhRad(g, xR, y, w, L("SD-kort", "SD card"), L("SAKNAS", "MISSING"), P.sel);
  else if (!e.sdKand) uiEnhRad(g, xR, y, w, L("SD-kort", "SD card"), L("mäter ...", "measuring ..."), P.faint);
  else {
    p = e.sdTotMB ? 100.0f * (e.sdTotMB - e.sdFriMB) / e.sdTotMB : NAN;
    uiFmtMB(t1, sizeof(t1), e.sdFriMB); uiFmtMB(t2, sizeof(t2), e.sdTotMB);
    snprintf(b, sizeof(b), L("%s av %s ledigt", "%s of %s free"), t1, t2);
    uiEnhStapel(g, xR, y, w, L("SD-kort", "SD card"), p, b);
  }
  y += 38;
  s4Sep(g, xR, y, w);
  snprintf(b, sizeof(b), "%lu kB", (unsigned long)e.inMinKB);
  uiEnhRad(g, xR, y + 2, w, L("Internt, minst ledigt sedan start", "Internal, least free since start"), b);
  s4Sep(g, xR, y + 32, w);
  snprintf(b, sizeof(b), "%lu / %lu kB", (unsigned long)e.psBigKB, (unsigned long)e.inBigKB);
  uiEnhRad(g, xR, y + 34, w, L("Största lediga bit PSRAM / internt", "Largest free block PSRAM / internal"), b);

  // ---- Skärmen
  s4Box(g, xL, yB, w, hB);
  s4Head(g, xL, yB, w, L("Skärmen", "Display"), false);
  y = yB + S4_HDR + 4;
  const int rh = 30;
  snprintf(b, sizeof(b), "%d", s.version);
  uiEnhRad(g, xL, y, w, "Version", b); s4Sep(g, xL, y + rh - 1, w); y += rh;
  snprintf(b, sizeof(b), "%s%s%s", s.net.namn[0] ? s.net.namn : "", s.net.namn[0] ? "  ·  " : "", s.net.devId);
  uiEnhRad(g, xL, y, w, L("Namn / enhet", "Name / unit"), b); s4Sep(g, xL, y + rh - 1, w); y += rh;
  uint32_t m = e.upSek / 60;
  if (m < 60) snprintf(b, sizeof(b), "%lu min", (unsigned long)m);
  else if (m < 48 * 60) snprintf(b, sizeof(b), "%lu h %lu min", (unsigned long)(m / 60), (unsigned long)(m % 60));
  else snprintf(b, sizeof(b), L("%lu dygn %lu h", "%lu days %lu h"), (unsigned long)(m / 1440), (unsigned long)((m / 60) % 24));
  uiEnhRad(g, xL, y, w, L("Drifttid", "Uptime"), b); s4Sep(g, xL, y + rh - 1, w); y += rh;
  snprintf(b, sizeof(b), "%lu", (unsigned long)e.bootNo);
  uiEnhRad(g, xL, y, w, L("Startnummer", "Boot number"), b); s4Sep(g, xL, y + rh - 1, w); y += rh;
  bool krasch = strstr(e.omstart, "KRASCH") || strstr(e.omstart, "VAKTHUND") || strstr(e.omstart, "spänning") ||
                strstr(e.omstart, "CRASH") || strstr(e.omstart, "WATCHDOG") || strstr(e.omstart, "brownout");
  uiEnhRad(g, xL, y, w, L("Senaste omstart", "Last restart"), e.omstart, krasch ? P.sel : 0); s4Sep(g, xL, y + rh - 1, w); y += rh;
  uiEnhRad(g, xL, y, w, L("Programplats", "App partition"), e.part[0] ? e.part : "-"); y += rh;

  // ---- Nät och ISOBUS
  s4Box(g, xR, yB, w, hB);
  s4Head(g, xR, yB, w, L("Nät och ISOBUS", "Network and ISOBUS"), false);
  y = yB + S4_HDR + 4;
  if (s.net.connected) snprintf(b, sizeof(b), "%s  %d dBm", s.net.ssid, s.net.rssi);
  else strcpy(b, L("inte ansluten", "not connected"));
  uiEnhRad(g, xR, y, w, "WiFi", b, s.net.connected ? 0 : P.faint); s4Sep(g, xR, y + rh - 1, w); y += rh;
  uiEnhRad(g, xR, y, w, L("IP-adress", "IP address"), s.net.connected && s.net.ip[0] ? s.net.ip : "-"); s4Sep(g, xR, y + rh - 1, w); y += rh;
  snprintf(b, sizeof(b), L("%lu /s  ·  %lu totalt", "%lu /s  ·  %lu total"), (unsigned long)e.ramarS, (unsigned long)e.ramarTot);
  uiEnhRad(g, xR, y, w, L("ISOBUS-ramar", "ISOBUS frames"), b, s.busOk ? 0 : P.faint); s4Sep(g, xR, y + rh - 1, w); y += rh;
  static const char *const CS[] = {"stoppad", "lyssnar", "FEL (buss av)", "återhämtar"};
  static const char *const CE[] = {"stopped", "listening", "ERROR (bus off)", "recovering"};
  const char *cs = e.canState >= 0 && e.canState <= 3 ? L(CS[e.canState], CE[e.canState]) : "-";
  uiEnhRad(g, xR, y, w, "CAN", cs, e.canState == 1 ? 0 : (e.canState >= 2 ? P.sel : 0)); s4Sep(g, xR, y + rh - 1, w); y += rh;
  snprintf(b, sizeof(b), L("%lu missade  ·  %lu fel", "%lu missed  ·  %lu errors"), (unsigned long)(e.canMiss + e.canOverrun), (unsigned long)e.canBusErr);
  uiEnhRad(g, xR, y, w, L("Bussen sedan start", "Bus since start"), b, (e.canMiss + e.canOverrun) ? P.amb : 0); s4Sep(g, xR, y + rh - 1, w); y += rh;
  if (e.ritN) snprintf(b, sizeof(b), L("snitt %lu ms  ·  max %lu ms", "avg %lu ms  ·  max %lu ms"), (unsigned long)e.ritSnitt, (unsigned long)e.ritMax);
  else strcpy(b, "-");
  uiEnhRad(g, xR, y, w, L("Ritning", "Drawing"), b); y += rh;
}

// ---------------------------------------------------------------- Manualen
// Texten kommer från manual/MANUAL.md och manual/MANUAL_EN.md via tools/manual/md2c.py (manual_data.h). Varje kapitel bryts om och delas
// i sidor här; < och > bläddrar sida för sida och vidare till nästa kapitel.
#include "manual_data.h"
static int uiManSidor = 1, uiManSidaVisad = 0;     // senast ritade kapitlets sidantal och sidan som visades
#define MAN_X0 18
#define MAN_X1 690
#define MAN_Y0 58
#define MAN_Y1 470
#define MAN_RAD 22
// Ett ord med **fet** i: ritar (om rita) och returnerar bredden. fet = läget före ordet, uppdateras efter ordet.
static int uiManOrd(Arduino_GFX *g, const char *w, int n, int x, int y, bool &fet, bool rita) {
  char seg[96]; int sn = 0, bredd = 0;
  auto flush = [&]() {
    if (!sn) return;
    seg[sn] = 0;
    const uint8_t *f = fet ? u8g2_font_dv15b_tf : u8g2_font_dv15_tf;
    if (rita) uiText(g, x + bredd, y, seg, P.txt, f);
    bredd += uiTextWidth(g, seg, f);
    sn = 0;
  };
  for (int i = 0; i < n; i++) {
    if (w[i] == '*' && i + 1 < n && w[i + 1] == '*') { flush(); fet = !fet; i++; continue; }
    if (sn < (int)sizeof(seg) - 1) seg[sn++] = w[i];
  }
  flush();
  return bredd;
}
// Bryter om kapitlet och ritar sidan "sida" (rita = true). Returnerar antalet sidor.
static int uiManLayout(Arduino_GFX *g, int kap, int sida, bool rita) {
  const char *t = MANUAL[kap].text;
  const int mell = uiTextWidth(g, " ", u8g2_font_dv15_tf);
  int sid = 0, y = MAN_Y0;
  while (*t) {
    const char *slut = strchr(t, '\n'); if (!slut) slut = t + strlen(t);
    char typ = *t++;
    if (typ == 'G') {
      if (y != MAN_Y0) y += 10;
      t = *slut ? slut + 1 : slut; continue;
    }
    int ind = 0, hang = 0;                         // indrag för första raden och följande rader
    if (typ == 'B') { ind = hang = 22; }
    else if (typ == 'b') { ind = hang = 46; }
    else if (typ == 'N') { ind = 0; hang = 26; }
    // punkten / numret
    bool forsta = true, fet = false;
    int x = MAN_X0 + ind;
    auto nyRad = [&]() {
      y += MAN_RAD;
      if (y > MAN_Y1) { sid++; y = MAN_Y0; }
      x = MAN_X0 + hang;
    };
    if (y > MAN_Y1) { sid++; y = MAN_Y0; }
    if (rita && sid == sida && (typ == 'B' || typ == 'b')) g->fillCircle(MAN_X0 + ind - 11, y - 6, 3, P.txt);
    const char *p = t;
    while (p < slut) {
      while (p < slut && *p == ' ') p++;
      if (p >= slut) break;
      const char *e = p; while (e < slut && *e != ' ') e++;
      bool fetFore = fet;
      int w = uiManOrd(g, p, e - p, 0, 0, fet, false);
      if (!forsta && x + w > MAN_X1) nyRad();
      bool f2 = fetFore;
      uiManOrd(g, p, e - p, x, y, f2, rita && sid == sida);
      x += w + mell;
      forsta = false;
      if (typ == 'N' && x < MAN_X0 + hang && forsta == false) x = MAN_X0 + hang;   // numret: hängande indrag
      p = e;
    }
    y += MAN_RAD;
    t = *slut ? slut + 1 : slut;
  }
  if (y == MAN_Y0 && sid > 0) sid--;                 // sista sidan blev tom
  return sid + 1;
}
static void uiDrawManual(Arduino_GFX *g, const UiState &s) {
  char b[80];
  if (s.manKap < 0 || s.manKap >= MAN_N) {
    // Innehållsförteckningen: kapitlen i två kolumner
    s4Page(g, L("Manual - Fältloggen", "Manual - Fältloggen"), true);
    const int rader = (MAN_N + 1) / 2, kh = 48, gap = 5;
    for (int i = 0; i < MAN_N; i++) {
      int col = i / rader, row = i % rader;
      int x = 14 + col * 342, y = 36 + row * (kh + gap);
      s4KeyT(g, s, x, y, 332, kh, B_MAN_KAP0 + i, "", u8g2_font_dv15b_tf);
      uiTextFit(g, x + 14, y + kh / 2 + 6, MANUAL[i].titel, 304, P.ico, u8g2_font_dv15b_tf);
    }
    s4Key(g, s, S4_RX, 6, S4_KW, 108, B_MAN_BACK, false, icTillbaka, 50);
    uiManSidor = 1; uiManSidaVisad = 0;
    return;
  }
  int sidor = uiManLayout(g, s.manKap, -1, false);
  int sida = s.manSida < 0 ? 0 : (s.manSida >= sidor ? sidor - 1 : s.manSida);
  if (sidor > 1) snprintf(b, sizeof(b), "%s  (%d/%d)", MANUAL[s.manKap].titel, sida + 1, sidor);
  else snprintf(b, sizeof(b), "%s", MANUAL[s.manKap].titel);
  s4Page(g, b, true);
  uiManLayout(g, s.manKap, sida, true);
  uiManSidor = sidor; uiManSidaVisad = sida;
  // högerkolumnen: tillbaka till inställningarna, innehåll, föregående, nästa
  s4Key(g, s, S4_RX, 6, S4_KW, 90, B_MAN_BACK, false, icTillbaka, 46);
  s4Key(g, s, S4_RX, 102, S4_KW, 90, B_MAN_INNEH, false, icLista, 40);
  uiTextCenter(g, S4_RX + S4_KW / 2, 102 + 20, L("INNEHÅLL", "CONTENTS"), P.ico, u8g2_font_dv11_tf);
  bool forst = s.manKap == 0 && sida == 0, sist = s.manKap == MAN_N - 1 && sida == sidor - 1;
  s4KeyT(g, s, S4_RX, 198, S4_KW, 134, forst ? B_NONE : B_MAN_FORRA, "<", u8g2_font_dv24b_tf);
  s4KeyT(g, s, S4_RX, 338, S4_KW, 134, sist ? B_NONE : B_MAN_NASTA, ">", u8g2_font_dv24b_tf);
}

// ---------------------------------------------------------------- Sifferfält: arbetsbredd i cm (v37)
static void uiDrawSiffror(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(P.scr);
  s4Box(g, 2, 2, 796, 476);
  s4Head(g, 2, 2, 796, s.numFor == 3 ? L("Skiftets areal i hektar", "Field area in hectares") : (s.numFor == 2 ? L("Kartans arbetsbredd i centimeter", "Map working width in centimetres") : L("Arbetsbredd i centimeter", "Working width in centimetres")), true);
  // värdet
  s4L(g, 40, 50, 300, 70);
  g->fillRect(42, 52, 296, 66, P.white);
  char vt[10]; snprintf(vt, sizeof(vt), "%s", s.numText[0] ? s.numText : "0");
  langDec(vt);                                                       // decimaltecken efter språket
  uiTextRight(g, 300, 102, vt, P.txt, u8g2_font_dv40_tn);
  uiText(g, 306, 102, s.numFor == 3 ? "ha" : "cm", P.txt, u8g2_font_dv20_tf);
  int cm = atoi(s.numText);
  float haV = (float)atof(s.numText);
  char b[48];
  bool skift = s.numFor == 3;                                    // v45: skiftets areal, 125 = 12,5 ha
  if (!skift && cm > 0) { char m[16]; uiFmt(m, sizeof(m), cm / 100.0f, 2); snprintf(b, sizeof(b), "= %s m", m); }
  else if (skift) snprintf(b, sizeof(b), "%s", L("skriv arealen, t.ex. 35,65", "type the area, e.g. 35.65"));
  else snprintf(b, sizeof(b), "%s", L("skriv in bredden, t.ex. 1050", "type the width, e.g. 1050"));
  uiText(g, 44, 150, b, P.dim, u8g2_font_dv17_tf);
  bool ok = skift ? (haV >= 0.01f && haV <= 999.99f) : (cm >= 100 && cm <= 6000);
  if (s.numText[0] && !ok) uiText(g, 44, 176, skift ? L("mellan 0,01 och 999,99 ha", "between 0.01 and 999.99 ha") : L("mellan 100 och 6000 cm", "between 100 and 6000 cm"), P.sel, u8g2_font_dv15_tf);
  // siffertangenterna
  const int kx = 420, ky = 40, kw = 110, kh = 80, gx = 10;
  const char *const T[12] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", L("RADERA", "DELETE"), "0", L(",", ".")};
  for (int i = 0; i < (skift ? 12 : 11); i++) {
    int x = kx + (i % 3) * (kw + gx), y = ky + (i / 3) * (kh + gx);
    int id = i == 9 ? B_NUM_DEL : (i == 11 ? B_NUM_DOT : B_NUM0 + (i == 10 ? 0 : i + 1));
    s4KeyT(g, s, x, y, kw, kh, id, T[i], i == 9 ? u8g2_font_dv15b_tf : u8g2_font_dv24b_tf);
  }
  s4KeyT(g, s, 40, 390, 160, 70, B_NUM_CANCEL, L("AVBRYT", "CANCEL"), u8g2_font_dv20b_tf);
  s4KeyT(g, s, 220, 390, 160, 70, ok ? B_NUM_OK : B_NONE, "OK", u8g2_font_dv20b_tf, ok ? 2 : 0);
}

// ---------------------------------------------------------------- WiFi-lista
static void uiDrawWifi(Arduino_GFX *g, const UiState &s) {
  s4Page(g, L("Välj WiFi-nät", "Choose WiFi network"), true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_WIFI_BACK, false, icTillbaka, 64);
  s4Key(g, s, S4_RX, 125, S4_KW, 108, B_WIFI_SCAN, false, icSok, 64, nullptr, nullptr);
  uiTextCenter(g, S4_RX + S4_KW / 2, 125 + 100, L("SÖK", "SCAN"), P.ico, u8g2_font_dv13b_tf);
  if (s.net.hasSaved) s4KeyT(g, s, S4_RX, 362, S4_KW, 108, B_WIFI_FORGET, L("GLÖM", "FORGET"), u8g2_font_dv15b_tf, 1);
  if (s.net.scanning) uiTextCenter(g, 352, 240, L("Söker efter nät ...", "Looking for networks ..."), P.dim, u8g2_font_dv20b_tf);
  else if (s.net.nScan == 0) uiTextCenter(g, 352, 240, L("Inga nät hittade. Tryck SÖK.", "No networks found. Press SCAN."), P.dim, u8g2_font_dv20b_tf);
  int n = s.net.scanning ? 0 : s.net.nScan;
  if (n > 8) n = 8;
  for (int i = 0; i < n; i++) {
    int y = 30 + i * 55;
    bool down = s.pressId == B_NET0 + i;
    bool cur = s.net.connected && !strcmp(s.net.ssid, s.net.scanSsid[i]);
    if (down) g->fillRect(4, y, 696, 52, P.hdr);
    if (i) s4Sep(g, 2, y, 700);
    uiBars(g, 20, y + 40, s.net.scanRssi[i], cur ? P.grn : P.dim);
    if (cur) s4Field(g, 66, y + 10, 360, 34, s.net.scanSsid[i], P.grnTxt, u8g2_font_dv17b_tf, P.grn);
    else uiText(g, 70, y + 34, s.net.scanSsid[i], P.txt, u8g2_font_dv17b_tf);
    uiTextRight(g, 686, y + 33, cur ? L("ansluten", "connected") : (s.net.scanSecure[i] ? L("lösenord", "password") : L("öppet", "open")), P.dim, u8g2_font_dv15_tf);
    uiReg(4, y, 696, 52, B_NET0 + i);
  }
}

// ---------------------------------------------------------------- Fältrapporten
#include "qrcodegen.h"
// QR-kod (svart på vitt) med kvadratisk storlek sz bildpunkter
static void uiQr(Arduino_GFX *g, int x, int y, int sz, const char *text) {
  static uint8_t qr[qrcodegen_BUFFER_LEN_FOR_VERSION(10)], tmp[qrcodegen_BUFFER_LEN_FOR_VERSION(10)];
  g->fillRect(x, y, sz, sz, RGB(255, 255, 255));
  if (!qrcodegen_encodeText(text, tmp, qr, qrcodegen_Ecc_MEDIUM, 1, 10, qrcodegen_Mask_AUTO, true)) return;
  int n = qrcodegen_getSize(qr), m = (sz - 8) / (n + 2);           // bildpunkter per modul, tyst zon 1 modul
  int ox = x + (sz - m * n) / 2, oy = y + (sz - m * n) / 2;
  for (int r = 0; r < n; r++)
    for (int c = 0; c < n; c++)
      if (qrcodegen_getModule(qr, c, r)) g->fillRect(ox + c * m, oy + r * m, m, m, RGB(0, 0, 0));
}
static void uiDrawExport(Arduino_GFX *g, const UiState &s) {
  const ExpView &e = s.exp;
  g->fillScreen(P.scr);
  s4Box(g, 2, 2, 700, 476);
  char b[96];
  snprintf(b, sizeof(b), "%s%s%s", L("Fältrapport", "Field report"), e.namn[0] ? " - " : "", e.namn);
  s4Head(g, 2, 2, 700, b, true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_EXP_STANG, false, icKlar, 70);
  uiTextCenter(g, S4_RX + S4_KW / 2, 136, L("STÄNG", "CLOSE"), P.dim, u8g2_font_dv13b_tf);
  if (e.status == 1) {
    uiTextCenter(g, 352, 220, L("Skapar rapporten ...", "Creating the report ..."), P.txt, u8g2_font_dv24b_tf);
    uiTextCenter(g, 352, 256, L("kartorna ritas och PDF:en skrivs till SD-kortet", "the maps are drawn and the PDF is written to the SD card"), P.dim, u8g2_font_dv15_tf);
    return;
  }
  if (e.status == 3) {
    uiTextCenter(g, 352, 210, L("Rapporten kunde inte skapas", "The report could not be created"), P.sel, u8g2_font_dv20b_tf);
    uiTextCenter(g, 352, 244, e.fel, P.txt, u8g2_font_dv15_tf);
    return;
  }
  // vänster: sammanfattningen
  for (int i = 0; i < e.nRad && i < 8; i++) {
    int y = 52 + i * 26;
    if (i) s4Sep(g, 2, y - 18, 330);
    uiTextFit(g, 14, y, e.rad[i], 312, i == 0 ? P.txt : P.dim, i == 0 ? u8g2_font_dv15b_tf : u8g2_font_dv13_tf);
  }
  snprintf(b, sizeof(b), L("Fil: %s", "File: %s"), e.fil);
  uiTextFit(g, 14, 270, b, 312, P.faint, u8g2_font_dv11_tf);
  // höger: två QR-koder – 1) anslut till skärmens WiFi, 2) öppna sidan
  char wifi[80]; snprintf(wifi, sizeof(wifi), "WIFI:T:nopass;S:%s;;", e.ap);
  uiText(g, 346, 50, L("1. Anslut telefonen", "1. Connect phone"), P.txt, u8g2_font_dv15b_tf);
  uiQr(g, 352, 60, 160, wifi);
  uiText(g, 526, 50, L("2. Öppna rapporten", "2. Open report"), P.txt, u8g2_font_dv15b_tf);
  uiQr(g, 530, 60, 160, "http://192.168.4.1/");
  snprintf(b, sizeof(b), L("WiFi: %s (inget lösenord)", "WiFi: %s (no password)"), e.ap);
  uiTextFit(g, 346, 246, b, 350, P.txt, u8g2_font_dv13b_tf);
  uiTextFit(g, 346, 266, L("Webbläsaren: 192.168.4.1", "Browser: 192.168.4.1"), 350, P.txt, u8g2_font_dv13_tf);
  s4Sep(g, 2, 282, 700);
  uiText(g, 14, 308, L("Så här gör du:", "How to do it:"), P.txt, u8g2_font_dv15b_tf);
  uiTextFit(g, 14, 332, L("1. Skanna den vänstra koden med kameran och anslut till nätet (telefonen kan säga att det",
                          "1. Scan the left code with the camera and join the network (the phone may say there is"), 680, P.dim, u8g2_font_dv13_tf);
  uiTextFit(g, 14, 350, L("    saknas internet - välj att behålla anslutningen).", "    no internet - choose to keep the connection)."), 680, P.dim, u8g2_font_dv13_tf);
  uiTextFit(g, 14, 372, L("2. Skanna den högra koden och tryck på LADDA NER. PDF:en hamnar bland nedladdningarna.",
                          "2. Scan the right code and press DOWNLOAD. The PDF ends up among your downloads."), 680, P.dim, u8g2_font_dv13_tf);
  uiTextFit(g, 14, 394, L("3. Tryck STÄNG när du är klar. Rapporten sparas också på skärmens SD-kort.",
                          "3. Press CLOSE when done. The report is also kept on the display's SD card."), 680, P.dim, u8g2_font_dv13_tf);
  snprintf(b, sizeof(b), L("Anslutna telefoner: %d   ·   nedladdningar: %d", "Connected phones: %d   ·   downloads: %d"), e.stationer, e.hamtade);
  s4Field(g, 14, 420, 676, 40, b, e.hamtade ? P.grnTxt : P.txt, u8g2_font_dv15b_tf, e.hamtade ? P.grn : 0);
}

// ---------------------------------------------------------------- Tangentbord
// Tangenter per rad, per läge (0 = små, 1 = stora, 2 = tecken)
static const char *const KB_ROWS[3][4][11] = {
  { {"1","2","3","4","5","6","7","8","9","0",0},
    {"q","w","e","r","t","y","u","i","o","p","å"},
    {"a","s","d","f","g","h","j","k","l","ö","ä"},
    {"z","x","c","v","b","n","m",".",0} },
  { {"1","2","3","4","5","6","7","8","9","0",0},
    {"Q","W","E","R","T","Y","U","I","O","P","Å"},
    {"A","S","D","F","G","H","J","K","L","Ö","Ä"},
    {"Z","X","C","V","B","N","M",",",0} },
  { {"1","2","3","4","5","6","7","8","9","0",0},
    {"!","@","#","$","%","&","*","(",")","=","+"},
    {"?","/","\\",":",";","\"","'","<",">","[","]"},
    {"-","_","{","}","~","^","|","`",0} },
};

static int kbRowLen(int mode, int r) {
  int n = 0;
  while (n < 11 && KB_ROWS[mode][r][n]) n++;
  return n;
}
static const char *uiKeyStr(int mode, int id) {
  int k = id - B_KEY0, r = k / 16, c = k % 16;
  if (r < 0 || r > 3 || c >= kbRowLen(mode, r)) return "";
  return KB_ROWS[mode][r][c];
}

// Tangent i S4-stil. style 0 = vanlig, 2 = grön (ANSLUT), sel = röd ram (skift på)
static void uiKey(Arduino_GFX *g, const UiState &s, int x, int y, int w, int h, int id, const char *label,
                  int style = 0, const uint8_t *font = u8g2_font_dv24_tf, bool sel = false) {
  s4KeyT(g, s, x, y, w, h, id, label, font, style, sel);
}

static void uiDrawKeyboard(Arduino_GFX *g, const UiState &s) {
  g->fillScreen(P.scr);
  s4Box(g, 2, 2, 796, 476);
  char b[64];
  if (s.kbFor == 2) snprintf(b, sizeof(b), "%s", L("Fältets namn (frivilligt) - står i rapportens rubrik", "Field name (optional) - shown in the report title"));
  else snprintf(b, sizeof(b), L("Lösenord för %s", "Password for %s"), s.kbSsid);
  s4Head(g, 2, 2, 796, b, true);
  // textfält
  s4L(g, 12, 34, 776, 56);
  g->fillRect(14, 36, 772, 52, P.white);
  uiText(g, 24, 72, s.kbText, P.txt, u8g2_font_dv24_tf);
  int cx = 24 + uiTextWidth(g, s.kbText, u8g2_font_dv24_tf) + 2;
  if ((s.tick / 3) % 2 == 0) g->fillRect(cx, 44, 3, 36, P.barM);

  const int KW = 65, KH = 64, GAP = 6, X0 = 12;
  int m = s.kbMode;
  for (int r = 0; r < 4; r++) {
    int n = kbRowLen(m, r);
    int y = 100 + r * (KH + GAP);
    int x;
    if (r == 3) {
      uiKey(g, s, X0, y, 96, KH, B_KB_SHIFT, m == 1 ? "abc" : "ABC", 0, u8g2_font_dv17b_tf, m == 1);
      x = X0 + 96 + GAP;
      int kw = (776 - 2 * 96 - 2 * GAP - (n - 1) * GAP) / n;
      for (int c = 0; c < n; c++) {
        uiKey(g, s, x, y, kw, KH, B_KEY0 + r * 16 + c, KB_ROWS[m][r][c]);
        x += kw + GAP;
      }
      uiKey(g, s, X0 + 776 - 96, y, 96, KH, B_KB_BACK, L("RADERA", "DELETE"), 0, u8g2_font_dv15b_tf);
    } else {
      int total = n * KW + (n - 1) * GAP;
      x = X0 + (776 - total) / 2;
      for (int c = 0; c < n; c++) {
        uiKey(g, s, x, y, KW, KH, B_KEY0 + r * 16 + c, KB_ROWS[m][r][c]);
        x += KW + GAP;
      }
    }
  }
  int y = 100 + 4 * (KH + GAP);
  uiKey(g, s, X0, y, 130, KH, B_KB_MODE, m == 2 ? "ABC" : "?123", 0, u8g2_font_dv17b_tf);
  uiKey(g, s, X0 + 136, y, 290, KH, B_KB_SPACE, L("mellanslag", "space"), 0, u8g2_font_dv17_tf);
  uiKey(g, s, X0 + 432, y, 160, KH, B_KB_CANCEL, s.kbFor == 2 ? L("HOPPA ÖVER", "SKIP") : L("AVBRYT", "CANCEL"), 0, u8g2_font_dv17b_tf);
  uiKey(g, s, X0 + 598, y, 178, KH, B_KB_OK, s.kbFor == 2 ? L("SKAPA", "CREATE") : L("ANSLUT", "CONNECT"), 2, u8g2_font_dv17b_tf);
}

// ---------------------------------------------------------------- Diagnossida
static void uiDrawDiag(Arduino_GFX *g, const UiState &s) {
  s4Page(g, s.trRawView ? L("ISOBUS - traktorns övriga meddelanden", "ISOBUS - other tractor messages") : L("ISOBUS - processdata", "ISOBUS - process data"), true);
  s4Key(g, s, S4_RX, 6, S4_KW, 108, B_DIAG_BACK, false, icTillbaka, 64);
  // v37: traktorns rådata (förut Info⁺ Rådata på traktorsidan) visas här
  s4Key(g, s, S4_RX, 125, S4_KW, 108, B_TR_RAW, false, s.trRawView ? nullptr : icRadata, 56, nullptr,
        s.trRawView ? "PROCESS" : nullptr);
  if (!s.trRawView) uiTextCenter(g, S4_RX + S4_KW / 2, 125 + 100, L("RÅDATA", "RAW"), P.ico, u8g2_font_dv13b_tf);
  else uiTextCenter(g, S4_RX + S4_KW / 2, 125 + 80, "DATA", P.ico, u8g2_font_dv15b_tf);
  if (s.trRawView) {
    uiText(g, 14, 50, L("PGN  från  data  (en rad per sort, senast hörd)", "PGN  from  data  (one row per kind, last heard)"), P.dim, u8g2_font_dv13_tf);
    s4L(g, 10, 60, 684, 400);
    if (s.nTrRaw == 0) uiText(g, 20, 92, L("inga", "none"), P.faint, u8g2_font_dv15_tf);
    for (int i = 0; i < s.nTrRaw && i < TR_UI_ROWS; i++) {
      int ry = 88 + i * 30;
      if (i) s4Sep(g, 4, ry - 20, 696);
      uiTextFit(g, 20, ry, s.trRaw[i], 660, P.txt, u8g2_font_dv15_tf);
    }
    return;
  }
  char b[96];
  char tc[8] = "?";
  if (s.tcAddr >= 0) snprintf(tc, sizeof(tc), "%d", s.tcAddr);
  snprintf(b, sizeof(b), L("TC-adress: %s   meddelanden: %lu", "TC address: %s   messages: %lu"), tc, (unsigned long)s.frames);
  uiText(g, 14, 48, b, P.dim, u8g2_font_dv13_tf);
  const int x0 = 14, y0 = 74;
  const int cx[] = {x0, x0 + 60, x0 + 120, x0 + 190, x0 + 260, x0 + 320, x0 + 470, x0 + 580};
  const char *hd[] = {L("FRÅN", "FROM"), L("TILL", "TO"), "ELEM", "DDI", "CMD", L("VÄRDE", "VALUE"), L("ANTAL", "COUNT"), L("SEDAN", "AGO")};
  for (int i = 0; i < 8; i++) uiText(g, cx[i], y0, hd[i], P.txt, u8g2_font_dv13b_tf);
  g->drawFastHLine(x0, y0 + 6, 680, P.lin);
  for (int r = 0; r < s.nRows && r < DIAG_ROWS; r++) {
    const DiagRow &d = s.rows[r];
    int y = y0 + 26 + r * 21;
    uint32_t age = (s.nowMs - d.lastMs) / 1000;
    uint16_t c = age < 5 ? P.txt : P.faint;
    if (d.ddi == 2 || d.ddi == 7) c = age < 5 ? P.barM : P.faint;
    const uint8_t *f = u8g2_font_dv13_tf;
    snprintf(b, sizeof(b), "%u", d.sa);         uiText(g, cx[0], y, b, c, f);
    snprintf(b, sizeof(b), "%u", d.da);         uiText(g, cx[1], y, b, c, f);
    snprintf(b, sizeof(b), "%u", d.element);    uiText(g, cx[2], y, b, c, f);
    snprintf(b, sizeof(b), "%u", d.ddi);        uiText(g, cx[3], y, b, c, f);
    snprintf(b, sizeof(b), "%u", d.cmd);        uiText(g, cx[4], y, b, c, f);
    snprintf(b, sizeof(b), "%ld", (long)d.value); uiText(g, cx[5], y, b, c, f);
    snprintf(b, sizeof(b), "%lu", (unsigned long)d.count); uiText(g, cx[6], y, b, c, f);
    snprintf(b, sizeof(b), "%lu s", (unsigned long)age); uiText(g, cx[7], y, b, c, f);
  }
  if (s.nRows == 0) uiText(g, x0, y0 + 40, L("Ingen processdata hörd än.", "No process data heard yet."), P.faint, u8g2_font_dv15_tf);
}

// ---------------------------------------------------------------- Rita aktuell sida
static void uiDraw(Arduino_GFX *g, const UiState &s) {
  g->setTextWrap(false);
  uiNBtn = 0;
  switch (s.page) {
    case PG_IDLE:     uiDrawIdle(g, s); break;
    case PG_SETTINGS: uiDrawSettings(g, s); break;
    case PG_WIFI:     uiDrawWifi(g, s); break;
    case PG_KEYBOARD: uiDrawKeyboard(g, s); break;
    case PG_DIAG:     uiDrawDiag(g, s); break;
    case PG_GENERIC:  uiDrawGeneric(g, s); break;
    case PG_TRAKTOR:  uiDrawTraktor(g, s); break;
    case PG_KARTA:    uiDrawKarta(g, s); break;
    case PG_KARTINST: uiDrawKartInst(g, s); break;
    case PG_VAL:      uiDrawVal(g, s); break;
    case PG_VT:       uiDrawVt(g, s); break;
    case PG_ANALYS:   uiDrawAnalys(g, s); break;
    case PG_SIFFROR:  uiDrawSiffror(g, s); break;
    case PG_EXPORT:   uiDrawExport(g, s); break;
    case PG_MANUAL:   uiDrawManual(g, s); break;
    case PG_ENHET:    uiDrawEnhet(g, s); break;
    case PG_MER:      uiDrawMer(g, s); break;
    case PG_FEEDBACK: uiDrawFeedback(g, s); break;
    default:          uiDrawTraktor(g, s); break;
  }
  if (s.demo && (s.page == PG_GENERIC || s.page == PG_TRAKTOR)) uiDemoBadge(g);
  if (s.net.otaActive) uiOta(g, s);
  else if (s.net.otaOffer > 0) uiOtaFraga(g, s);
}
