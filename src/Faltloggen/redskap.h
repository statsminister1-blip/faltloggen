// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  redskap.h – vilket ISOBUS-redskap sitter i traktorn?
//
//  1. ISOBUS-namn: varje enhet talar om vem den är (adressanspråk, PGN 0xEE00).
//     Namnet innehåller tillverkarkod, enhetsklass (t.ex. 6 = spruta), funktion
//     och ett serienummer. Namnen sparas i minnet så att de är kända även om
//     skärmen startar efter redskapet.
//  2. Redskapet = den enhet som pratar med traktorns Task Controller (TC) eller
//     terminalen (VT) och inte är traktorn själv (enhetsklass 0/1).
//  3. Redskapets beskrivning (DDOP) fångas när redskapet laddar upp den till
//     TC (TP/ETP, processdata 0x61). Den ger namn, enheter och skala för varje
//     värde. Beskrivningen sparas på SD-kortet (/DDOP/<namn>.BIN).
//
//  Allt är bara lyssning – inget skickas på bussen.
// ============================================================================
#pragma once
#include <SD.h>
#include <Preferences.h>
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"
#include "lang.h"

// Enhetsklasser (ISO 11783-1, jordbruk)
static const char *klassNamn(int k) {
  static const char *const SV[] = {
    "ospecificerad", "traktor", "jordbearbetning", "efterbearbetning", "sådd/plantering",
    "gödselspridare", "spruta", "skördare", "rotfruktsskördare", "vallskörd",
    "bevattning", "transport/vagn", "gårdsarbete", "hjälpaggregat" };
  static const char *const EN[] = {
    "unspecified", "tractor", "primary tillage", "secondary tillage", "planter/seeder",
    "fertilizer", "sprayer", "harvester", "root harvester", "forage",
    "irrigation", "transport/trailer", "farmyard work", "powered auxiliary" };
  if (k >= 0 && k < (int)(sizeof(SV) / sizeof(SV[0]))) return L(SV[k], EN[k]);
  return nullptr;
}

// ---------------------------------------------------------------- ISOBUS-namn
static inline uint32_t nmIdent(uint64_t n) { return n & 0x1FFFFF; }
static inline int nmTillv(uint64_t n)  { return (n >> 21) & 0x7FF; }
static inline int nmFunk(uint64_t n)   { return (n >> 40) & 0xFF; }
static inline int nmKlass(uint64_t n)  { return (n >> 49) & 0x7F; }
static inline int nmGrupp(uint64_t n)  { return (n >> 60) & 0x7; }

#define NK_NEJ    0
#define NK_HORT   1   // hört adressanspråket
#define NK_MINNE  2   // ihågkommet från förra körningen
#define NK_DDOP   3   // stod i redskapets beskrivning


struct BusDev {
  uint64_t name;
  uint32_t tcMs;      // senast enheten skickade processdata till TC
  uint32_t vtMs;      // senast enheten pratade med VT (terminalen)
  uint8_t  known;
  uint8_t  flags;
};
static BusDev busDev[256];
static volatile bool rdNamesChanged = false;
static portMUX_TYPE rdMux = portMUX_INITIALIZER_UNLOCKED;

// ---------------------------------------------------------------- Redskapets värden
#define IMPL_VALS 48
struct ImplVal { uint16_t el, ddi; int32_t v; uint32_t ms; };
static ImplVal implVals[IMPL_VALS];
static int nImplVals = 0;

static volatile int rdImplAddr = -1;    // valt redskap (-1 = inget)

// ---------------------------------------------------------------- Redskapsbeskrivning (DDOP)
#define DDOP_MAX      65536
#define DDOP_MAX_OBJ  1024
#define DDOP_MAX_VAL  256
#define DDOP_MAX_DVP  96

struct DdopVal { uint16_t el, ddi; char elNamn[33]; char namn[33]; int16_t dvp; };
struct DdopDvp { uint16_t id; int32_t off; float skala; uint8_t dec; char enhet[17]; };
struct Ddop {
  bool     ok;
  uint64_t name;            // ISOBUS-namnet från beskrivningen
  char     namn[33];        // redskapets namn
  char     ver[33];         // programversion
  char     serie[33];
  int      nVal, nDvp, nObj;
  DdopVal *val;
  DdopDvp *dvp;
};
static Ddop ddop;

static uint8_t *ddopRaw = nullptr;      // hela beskrivningen (utan 0x61-byten), PSRAM
static uint32_t ddopRawLen = 0;
static uint64_t ddopRawName = 0;
static uint64_t ddopTriedName = 0;      // har försökt läsa från SD-kortet

// Överföring på väg (en åt gången)
struct TpSess {
  bool     on, etp;
  uint8_t  sa, da;
  uint32_t size, maxPos, etpOff, ms;
};
static TpSess tpS;
static uint8_t *tpBuf = nullptr;        // PSRAM
static uint8_t *readyBuf = nullptr;     // färdig överföring som väntar på att tolkas
static volatile uint32_t readyLen = 0;
static volatile int readySa = -1;
static volatile bool readyFlag = false;
static volatile bool readyIsDemo = false;  // beskrivningen kommer från demoläget: spara inte


// ---------------------------------------------------------------- Uppstart
static Preferences rdPrefs;

struct NamnMinne { uint8_t addr; uint8_t pad[7]; uint64_t name; };

static void redskapBegin() {
  tpBuf    = (uint8_t *)heap_caps_malloc(DDOP_MAX, MALLOC_CAP_SPIRAM);
  readyBuf = (uint8_t *)heap_caps_malloc(DDOP_MAX, MALLOC_CAP_SPIRAM);
  ddopRaw  = (uint8_t *)heap_caps_malloc(DDOP_MAX, MALLOC_CAP_SPIRAM);
  ddop.val = (DdopVal *)heap_caps_malloc(sizeof(DdopVal) * DDOP_MAX_VAL, MALLOC_CAP_SPIRAM);
  ddop.dvp = (DdopDvp *)heap_caps_malloc(sizeof(DdopDvp) * DDOP_MAX_DVP, MALLOC_CAP_SPIRAM);
  memset(busDev, 0, sizeof(busDev));
  rdPrefs.begin("redskap", false);
  NamnMinne m[16];
  size_t n = rdPrefs.getBytes("namn", m, sizeof(m)) / sizeof(NamnMinne);
  for (size_t i = 0; i < n; i++) {
    busDev[m[i].addr].name = m[i].name;
    busDev[m[i].addr].known = NK_MINNE;
  }
}

// Sparar kända namn (anropas från loop, inte oftare än var 30:e sekund)
static void redskapSaveNames() {
  NamnMinne m[16];
  int n = 0;
  portENTER_CRITICAL(&rdMux);
  for (int a = 0; a < 254 && n < 16; a++)
    if (busDev[a].known == NK_HORT || busDev[a].known == NK_DDOP) {
      memset(&m[n], 0, sizeof(NamnMinne));
      m[n].addr = a; m[n].name = busDev[a].name; n++;
    }
  portEXIT_CRITICAL(&rdMux);
  if (n) rdPrefs.putBytes("namn", m, n * sizeof(NamnMinne));
}

// ---------------------------------------------------------------- Från CAN-tasken
static void tpDone() {
  // Hela överföringen mottagen: lämna över till loop om det är en beskrivning
  if (tpS.maxPos >= tpS.size && tpS.size > 1 && tpBuf[0] == 0x61 && !readyFlag) {
    memcpy(readyBuf, tpBuf, tpS.size);
    readyLen = tpS.size;
    readySa = tpS.sa;
    readyFlag = true;
  }
  tpS.on = false;
}

static void redskapFrame(uint32_t pgn, uint8_t sa, uint8_t da, const uint8_t *d, uint8_t dlc,
                         uint32_t now, int tcAddr) {
  switch (pgn) {
    case 0xEE00:                      // adressanspråk
      if (dlc == 8 && sa < 254) {
        uint64_t n = 0;
        for (int i = 7; i >= 0; i--) n = (n << 8) | d[i];
        portENTER_CRITICAL(&rdMux);
        if (busDev[sa].known != NK_HORT || busDev[sa].name != n) rdNamesChanged = true;
        // samma namn på en annan adress -> den gamla adressen gäller inte längre
        for (int a = 0; a < 254; a++) if (a != sa && busDev[a].known && busDev[a].name == n) busDev[a].known = NK_NEJ;
        busDev[sa].name = n; busDev[sa].known = NK_HORT;
        portEXIT_CRITICAL(&rdMux);
      }
      return;

    case 0xCB00: {                    // processdata
      if (dlc < 8 || sa >= 254) return;
      uint8_t cmd = d[0] & 0x0F;
      if (tcAddr < 0 || sa == tcAddr) return;
      if (da != tcAddr && da != 0xFF) return;
      busDev[sa].tcMs = now;
      if (cmd != 0x03) return;
      uint16_t el = (d[0] >> 4) | ((uint16_t)d[1] << 4);
      uint16_t ddi = d[2] | (d[3] << 8);
      int32_t v = (int32_t)((uint32_t)d[4] | ((uint32_t)d[5] << 8) | ((uint32_t)d[6] << 16) | ((uint32_t)d[7] << 24));
      if (sa != rdImplAddr || ddi >= 0xDFFF) return;   // 0xDFFF.. = förfrågningar m.m.
      portENTER_CRITICAL(&rdMux);
      int i;
      for (i = 0; i < nImplVals; i++) if (implVals[i].el == el && implVals[i].ddi == ddi) break;
      if (i == nImplVals) {
        if (nImplVals < IMPL_VALS) nImplVals++;
        else {
          int old = 0;
          for (int k = 1; k < IMPL_VALS; k++) if (implVals[k].ms < implVals[old].ms) old = k;
          i = old;
        }
        implVals[i].el = el; implVals[i].ddi = ddi;
      }
      implVals[i].v = v; implVals[i].ms = now;
      portEXIT_CRITICAL(&rdMux);
      return;
    }

    case 0xE600: case 0xE700:         // VT-trafik (terminalen)
      if (pgn == 0xE700 && sa < 254) busDev[sa].vtMs = now;
      return;

    case 0xEC00: case 0xC800: {       // transportprotokoll: styrmeddelanden
      if (!tpBuf || dlc < 8) return;
      bool etp = (pgn == 0xC800);
      uint32_t tpPgn = d[5] | (d[6] << 8) | ((uint32_t)d[7] << 16);
      if (tpPgn != 0xCB00) return;
      uint8_t c = d[0];
      if ((!etp && c == 0x10) || (etp && c == 0x14)) {            // RTS: ny överföring
        if (tcAddr >= 0 && da != tcAddr) return;
        uint32_t size = etp ? (d[1] | (d[2] << 8) | ((uint32_t)d[3] << 16) | ((uint32_t)d[4] << 24))
                            : (uint32_t)(d[1] | (d[2] << 8));
        if (size > DDOP_MAX) { tpS.on = false; return; }
        tpS = {true, etp, sa, da, size, 0, 0, now};
      } else if (!tpS.on) {
        return;
      } else if (etp && c == 0x16 && sa == tpS.sa && da == tpS.da) {   // ETP: dataoffset
        tpS.etpOff = d[2] | (d[3] << 8) | ((uint32_t)d[4] << 16);
      } else if ((c == 0x13 || c == 0x17) && sa == tpS.da && da == tpS.sa) {   // klart
        tpDone();
      } else if (c == 0xFF && ((sa == tpS.sa && da == tpS.da) || (sa == tpS.da && da == tpS.sa))) {
        tpS.on = false;                                                     // avbruten
      }
      return;
    }

    case 0xEB00: case 0xC700: {       // transportprotokoll: data
      if (!tpS.on || sa != tpS.sa || da != tpS.da || dlc < 8) return;
      if ((pgn == 0xC700) != tpS.etp) return;
      if (d[0] == 0) return;
      uint32_t pos = ((tpS.etp ? tpS.etpOff : 0) + d[0] - 1) * 7;
      for (int i = 0; i < 7; i++) if (pos + i < tpS.size) tpBuf[pos + i] = d[1 + i];
      if (pos + 7 > tpS.maxPos) tpS.maxPos = pos + 7;
      tpS.ms = now;
      return;
    }
  }
}

// ---------------------------------------------------------------- Tolka beskrivningen
// Text från beskrivningen: UTF-8 om den är giltig, annars Latin-1 -> UTF-8.
static void ddopStr(const uint8_t *s, int n, char *out, int outSz) {
  bool utf8 = true;
  for (int i = 0; i < n && utf8; i++) {
    uint8_t c = s[i];
    if (c < 0x80) continue;
    int k = (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : (c & 0xF8) == 0xF0 ? 3 : -1;
    if (k < 0 || i + k >= n + 0) { utf8 = false; break; }
    for (int j = 1; j <= k; j++) if (i + j >= n || (s[i + j] & 0xC0) != 0x80) utf8 = false;
    i += k;
  }
  int o = 0;
  for (int i = 0; i < n; i++) {
    uint8_t c = s[i];
    if (c == 0) break;
    if (utf8 || c < 0x80) {
      int k = c < 0x80 ? 0 : (c & 0xE0) == 0xC0 ? 1 : (c & 0xF0) == 0xE0 ? 2 : 3;
      if (o + k + 1 >= outSz) break;
      for (int j = 0; j <= k && i + j < n; j++) out[o++] = s[i + j];
      i += k;
    } else {
      if (o + 2 >= outSz) break;
      out[o++] = 0xC0 | (c >> 6);
      out[o++] = 0x80 | (c & 0x3F);
    }
  }
  // ta bort mellanslag på slutet
  while (o > 0 && out[o - 1] == ' ') o--;
  out[o] = 0;
}

struct DdopObj { char typ; uint16_t id; uint32_t pos; };

static inline bool ddopTag(const uint8_t *p, const char *t) { return p[0] == t[0] && p[1] == t[1] && p[2] == t[2]; }
static inline bool ddopAnyTag(const uint8_t *p) {
  return ddopTag(p, "DVC") || ddopTag(p, "DET") || ddopTag(p, "DPD") || ddopTag(p, "DPT") || ddopTag(p, "DVP");
}
static inline uint16_t rd16(const uint8_t *p) { return p[0] | (p[1] << 8); }
static inline int32_t rd32(const uint8_t *p) { return (int32_t)((uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24)); }

// Tolkar objekten i buf (utan 0x61). Returnerar antal objekt.
static int ddopParse(const uint8_t *buf, uint32_t len) {
  static DdopObj *obj = nullptr;
  if (!obj) obj = (DdopObj *)heap_caps_malloc(sizeof(DdopObj) * DDOP_MAX_OBJ, MALLOC_CAP_SPIRAM);
  if (!obj || !ddop.val || !ddop.dvp) return 0;
  ddop.ok = false; ddop.nVal = ddop.nDvp = 0; ddop.namn[0] = ddop.ver[0] = ddop.serie[0] = 0; ddop.name = 0;
  int nObj = 0;
  uint32_t p = 0;
  // Pass 1: hitta alla objekt
  while (p + 5 <= len && nObj < DDOP_MAX_OBJ) {
    const uint8_t *o = buf + p;
    uint32_t q = p + 5;
    char typ;
    #define NEED(k) if (q + (k) > len) goto slut
    if (ddopTag(o, "DVC")) {
      typ = 'C';
      NEED(1); q += 1 + buf[q];               // namn
      NEED(1); q += 1 + buf[q];               // programversion
      NEED(8); q += 8;                        // ISOBUS-namn
      NEED(1); q += 1 + buf[q];               // serienummer
      NEED(14); q += 14;                      // struktur- och språketikett
      // version 4: utökad strukturetikett (längd + data) innan nästa objekt
      if (q + 3 <= len && !ddopAnyTag(buf + q)) { NEED(1); q += 1 + buf[q]; }
    } else if (ddopTag(o, "DET")) {
      typ = 'E';
      NEED(1); q += 1;                        // typ
      NEED(1); q += 1 + buf[q];               // namn
      NEED(6); uint16_t nc = rd16(buf + q + 4); q += 6;
      q += 2 * nc;
    } else if (ddopTag(o, "DPD")) {
      typ = 'D';
      NEED(4); q += 4;                        // DDI, egenskaper, utlösning
      NEED(1); q += 1 + buf[q];
      q += 2;                                 // DVP
    } else if (ddopTag(o, "DPT")) {
      typ = 'T';
      NEED(6); q += 6;                        // DDI, värde
      NEED(1); q += 1 + buf[q];
      q += 2;
    } else if (ddopTag(o, "DVP")) {
      typ = 'V';
      NEED(9); q += 9;                        // offset, skala, decimaler
      NEED(1); q += 1 + buf[q];
    } else {
      break;                                  // okänt objekt: sluta här
    }
    if (q > len) break;
    obj[nObj++] = {typ, rd16(o + 3), p};
    p = q;
    #undef NEED
  }
slut:
  // Pass 2: enheten, värdepresentationer
  for (int i = 0; i < nObj; i++) {
    const uint8_t *o = buf + obj[i].pos + 5;
    if (obj[i].typ == 'C') {
      int n = o[0]; ddopStr(o + 1, n, ddop.namn, sizeof(ddop.namn)); o += 1 + n;
      n = o[0]; ddopStr(o + 1, n, ddop.ver, sizeof(ddop.ver)); o += 1 + n;
      uint64_t nm = 0;
      for (int k = 7; k >= 0; k--) nm = (nm << 8) | o[k];
      ddop.name = nm; o += 8;
      n = o[0]; ddopStr(o + 1, n, ddop.serie, sizeof(ddop.serie));
    } else if (obj[i].typ == 'V' && ddop.nDvp < DDOP_MAX_DVP) {
      DdopDvp &v = ddop.dvp[ddop.nDvp++];
      v.id = obj[i].id;
      v.off = rd32(o);
      memcpy(&v.skala, o + 4, 4);
      v.dec = o[8];
      ddopStr(o + 10, o[9], v.enhet, sizeof(v.enhet));
    }
  }
  // Pass 3: element och deras värden
  for (int i = 0; i < nObj; i++) {
    if (obj[i].typ != 'E') continue;
    const uint8_t *o = buf + obj[i].pos + 5;
    char elNamn[33];
    int n = o[1];
    ddopStr(o + 2, n, elNamn, sizeof(elNamn));
    const uint8_t *r = o + 2 + n;
    uint16_t elNum = rd16(r), nc = rd16(r + 4);
    const uint8_t *ch = r + 6;
    for (int c = 0; c < nc; c++) {
      uint16_t cid = rd16(ch + 2 * c);
      for (int j = 0; j < nObj; j++) {
        if (obj[j].id != cid || obj[j].typ != 'D') continue;
        if (ddop.nVal >= DDOP_MAX_VAL) break;
        const uint8_t *dp = buf + obj[j].pos + 5;
        DdopVal &v = ddop.val[ddop.nVal++];
        v.el = elNum;
        v.ddi = rd16(dp);
        strcpy(v.elNamn, elNamn);
        int dn = dp[4];
        ddopStr(dp + 5, dn, v.namn, sizeof(v.namn));
        uint16_t dvpId = rd16(dp + 5 + dn);
        v.dvp = -1;
        for (int k = 0; k < ddop.nDvp; k++) if (ddop.dvp[k].id == dvpId) { v.dvp = k; break; }
        break;
      }
    }
  }
  ddop.nObj = nObj;
  ddop.ok = nObj > 0;
  return nObj;
}

// ---------------------------------------------------------------- Inbyggda namn (utan beskrivning)
struct DdiInfo { uint16_t ddi; const char *sv, *en; const char *enhet; float skala; uint8_t dec; };
static const DdiInfo DDI_INFO[] = {
  {   1, "Börvärde volym/yta",   "Setpoint volume/area",  "l/ha", 0.01f,      1 },
  {   2, "Giva volym/yta",       "Actual volume/area",    "l/ha", 0.01f,      1 },
  {   6, "Börvärde massa/yta",   "Setpoint mass/area",    "kg/ha", 0.01f,     1 },
  {   7, "Giva massa/yta",       "Actual mass/area",      "kg/ha", 0.01f,     1 },
  {  67, "Arbetsbredd",          "Working width",         "m",    0.001f,     2 },
  {  70, "Max arbetsbredd",      "Max working width",     "m",    0.001f,     2 },
  {  80, "Total volym",          "Total volume",          "l",    1.0f,       0 },
  {  81, "Total massa",          "Total mass",            "kg",   1.0f,       0 },
  { 116, "Total yta",            "Total area",            "ha",   0.0001f,    2 },
  { 117, "Total sträcka",        "Total distance",        "km",   0.000001f,  2 },
  { 119, "Total tid i arbete",   "Total working time",    "h",    1.0f / 3600, 1 },
  { 134, "Offset X",             "Offset X",              "m",    0.001f,     2 },
  { 135, "Offset Y",             "Offset Y",              "m",    0.001f,     2 },
  { 141, "Arbetsläge",           "Work state",            "",     1.0f,       0 },
  { 157, "Kopplingstyp",         "Connector type",        "",     1.0f,       0 },
  { 160, "Sektionsstyrning",     "Section control",       "",     1.0f,       0 },
  { 161, "Sektioner 1-16",       "Sections 1-16",         "",     1.0f,       0 },
};

static const DdopVal *ddopFind(uint16_t el, uint16_t ddi) {
  if (!ddop.ok) return nullptr;
  for (int i = 0; i < ddop.nVal; i++) if (ddop.val[i].el == el && ddop.val[i].ddi == ddi) return &ddop.val[i];
  return nullptr;
}
static const DdiInfo *ddiFind(uint16_t ddi) {
  for (auto &d : DDI_INFO) if (d.ddi == ddi) return &d;
  return nullptr;
}

// ---------------------------------------------------------------- Välja redskap
struct RedskapInfo {
  int      addr;          // -1 = inget redskap
  bool     nameKnown;
  uint64_t name;
};
static RedskapInfo rdInfo = {-1, false, 0};

static bool isTractorSide(const BusDev &b) {
  // traktorns egna enheter: enhetsklass 0 (ospecificerad) eller 1 (traktor)
  return b.known && nmGrupp(b.name) == 2 && nmKlass(b.name) <= 1;
}

// Körs från loop några gånger per sekund
static void redskapSelect(uint32_t now, int tcAddr) {
  int best = -1, bestScore = 0;
  int tcTillv = (tcAddr >= 0 && busDev[tcAddr].known) ? nmTillv(busDev[tcAddr].name) : -1;
  for (int a = 0; a < 254; a++) {
    const BusDev &b = busDev[a];
    if (a == tcAddr) continue;
    bool tc = b.tcMs && now - b.tcMs < 8000;
    bool vt = b.vtMs && now - b.vtMs < 8000;
    if (!tc && !vt) continue;
    if (isTractorSide(b)) continue;
    int score = 0;
    if (tc) score += b.known ? 8 : 2;
    else if (b.known && nmKlass(b.name) >= 2 && nmTillv(b.name) != tcTillv) score += 4;   // bara VT
    if (!score) continue;
    if (score > bestScore) { bestScore = score; best = a; }
  }
  if (best != rdImplAddr) {
    portENTER_CRITICAL(&rdMux);
    nImplVals = 0;
    portEXIT_CRITICAL(&rdMux);
    rdImplAddr = best;
  }
  RedskapInfo r = {best, false, 0};
  if (best >= 0) {
    const BusDev &b = busDev[best];
    r.nameKnown = b.known != NK_NEJ;
    r.name = b.name;
  }
  rdInfo = r;
}

// ---------------------------------------------------------------- SD-kortet
static void ddopPath(char *p, int n, uint64_t name) {
  snprintf(p, n, "/DDOP/%08lX%08lX.BIN", (unsigned long)(name >> 32), (unsigned long)(name & 0xFFFFFFFF));
}

static void ddopSave() {
  if (!sdOk || !ddopRawLen || !ddopRawName) return;
  char p[40];
  ddopPath(p, sizeof(p), ddopRawName);
  sdLock();
  if (!SD.exists("/DDOP")) SD.mkdir("/DDOP");
  File f = SD.open(p, FILE_WRITE);
  if (f) { f.write(ddopRaw, ddopRawLen); f.close(); }
  sdUnlock();
}

static bool ddopLoad(uint64_t name) {
  if (!sdOk) return false;
  char p[40];
  ddopPath(p, sizeof(p), name);
  bool ok = false;
  sdLock();
  File f = SD.open(p, FILE_READ);
  if (f) {
    uint32_t n = f.size();
    if (n > 0 && n <= DDOP_MAX) { ddopRawLen = f.read(ddopRaw, n); ok = ddopRawLen == n; }
    f.close();
  }
  sdUnlock();
  if (ok) { ddopRawName = name; ddopParse(ddopRaw, ddopRawLen); }
  return ok;
}

// ---------------------------------------------------------------- Körs från loop
static void redskapLoop(uint32_t now, int tcAddr) {
  static uint32_t lastSel = 0, lastNameSave = 0;
  // Ny beskrivning mottagen
  if (readyFlag) {
    int sa = readySa;
    const uint8_t *b = readyBuf + 1;          // hoppa över 0x61
    uint32_t n = readyLen - 1;
    bool full = n >= 3 && ddopTag(b, "DVC");  // hel beskrivning (annars en del som läggs till)
    if (full) ddopRawLen = 0;
    if (ddopRawLen + n <= DDOP_MAX) { memcpy(ddopRaw + ddopRawLen, b, n); ddopRawLen += n; }
    readyFlag = false;
    bool demoD = readyIsDemo;
    readyIsDemo = false;
    if (ddopParse(ddopRaw, ddopRawLen) > 0) {
      ddopRawName = ddop.name;
      ddopTriedName = ddop.name;
      if (ddop.name && sa >= 0 && sa < 254) {
        portENTER_CRITICAL(&rdMux);
        if (busDev[sa].known != NK_HORT) { busDev[sa].name = ddop.name; busDev[sa].known = NK_DDOP; rdNamesChanged = true; }
        portEXIT_CRITICAL(&rdMux);
      }
      Serial.printf("Redskapsbeskrivning från 0x%02X: %s, %d objekt, %d värden\n", sa, ddop.namn, ddop.nObj, ddop.nVal);
      if (!demoD) ddopSave();
    }
  }
  // Överföring som aldrig blev klar
  if (tpS.on && now - tpS.ms > 5000) tpS.on = false;

  if (now - lastSel >= 250) {
    lastSel = now;
    redskapSelect(now, tcAddr);
    // Beskrivning från SD-kortet om redskapet är känt sedan tidigare
    if (rdInfo.addr >= 0 && rdInfo.nameKnown && rdInfo.name != ddopRawName && rdInfo.name != ddopTriedName) {
      ddopTriedName = rdInfo.name;
      if (ddopLoad(rdInfo.name)) Serial.printf("Redskapsbeskrivning från SD: %s\n", ddop.namn);
      else { ddop.ok = false; ddopRawLen = 0; ddopRawName = 0; }
    }
  }
  if (rdNamesChanged && now - lastNameSave > 30000) {
    lastNameSave = now;
    rdNamesChanged = false;
    redskapSaveNames();
  }
}

// Beskrivningen hör till valt redskap?
static bool redskapHasDdop() {
  return ddop.ok && rdInfo.addr >= 0 && rdInfo.nameKnown && ddopRawName == rdInfo.name;
}

// Formaterar ett värde: namn, värde, enhet (decimaltecken efter språket)
static void redskapFormat(uint16_t el, uint16_t ddi, int32_t raw, char *namn, int nn,
                          char *val, int vn, char *enh, int en) {
  const DdopVal *dv = redskapHasDdop() ? ddopFind(el, ddi) : nullptr;
  const DdiInfo *di = ddiFind(ddi);
  double x = raw; int dec = 0; enh[0] = 0;
  if (dv) {
    if (dv->elNamn[0] && strcmp(dv->elNamn, ddop.namn)) snprintf(namn, nn, "%s · %s", dv->elNamn, dv->namn);
    else snprintf(namn, nn, "%s", dv->namn);
  } else if (di) snprintf(namn, nn, "%s (el %u)", L(di->sv, di->en), el);
  else snprintf(namn, nn, "DDI %u (el %u)", ddi, el);
  if (ddi == 141) {                             // arbetsläge: text
    snprintf(val, vn, "%s", raw == 1 ? L("PÅ", "ON") : raw == 0 ? L("AV", "OFF") : raw == 2 ? L("FEL", "ERROR") : "-");
    return;
  }
  if (dv && dv->dvp >= 0) {
    const DdopDvp &p = ddop.dvp[dv->dvp];
    x = ((double)raw + p.off) * p.skala; dec = p.dec;
    snprintf(enh, en, "%s", p.enhet);
  } else if (di) {
    x = raw * (double)di->skala; dec = di->dec;
    snprintf(enh, en, "%s", di->enhet);
  }
  if (dec > 3) dec = 3;
  if (dec <= 0) snprintf(val, vn, "%lld", (long long)llround(x));
  else snprintf(val, vn, "%.*f", dec, x);
  langDec(val);
}
