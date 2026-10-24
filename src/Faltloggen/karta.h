// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  karta.h – karta över körd yta, med areal
//
//  - Loggningen startas och stoppas av en vald signal från traktorn (trigger):
//    hydraulventil 1-7 (UT/IN), kraftuttag bak/fram (PÅ/AV), lyft bak/fram (NER/UPP),
//    ISOBUS-redskapets arbetsläge (auto) eller manuellt med START/STOPP-knappen.
//  - Körspåret målas med arbetsbredden (auto från ISOBUS eller manuell, i cm)
//    i ett rutnät med 1 m rutor (4 x 4 km runt platsen där loggningen startade).
//    Arealen = antal målade rutor, så överlapp räknas inte två gånger.
//  - Spåret sparas på SD-kortet (/KARTA/SPAR.BIN) och läses in igen vid start.
//  - Bakgrund: Sentinel-2 cloudless från EOX (10 m upplösning), se K_KALLOR nedan för källa och licens.
//    Skärmen hämtar rutorna själv via WiFi (6 km runt den första GPS-positionen och nya platser),
//    avkodar JPEG med ESP32-S3:ans inbyggda avkodare och sparar dem på SD-kortet.
//    Finns ingen bild blir bakgrunden mörk – spåret och fordonet syns ändå.
//  - Bränsle: traktorns förbrukning (l/h, 0xFEF2) summeras medan loggningen är på -> liter och l/ha.
//  - Spårets färg: bränsle (l/ha), dragkraft (kN) eller slirning (%) – se "Färg på spåret".
//  - Redskapet antas sitta K_BAK_M (3 m) bakom GPS-antennen; spåret målas där.
//  Positionen kommer från traktorns styrsystem (NMEA 2000 0x1F801 på ISOBUS).
// ============================================================================
#pragma once
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "rom/tjpgd.h"
#include <time.h>
#include "traktor_ovan.h"

// ---------------------------------------------------------------- Inställningar
enum { KT_MANUELL = 0, KT_VENTIL1 = 1, KT_VENTIL7 = 7, KT_PTO_BAK = 8, KT_PTO_FRAM = 9,
       KT_LYFT_BAK = 10, KT_LYFT_FRAM = 11, KT_ISOBUS = 12, KT_N = 13 };
// KT_ISOBUS = autoläge: loggar så länge ISOBUS-redskapet arbetar
// (redskapets arbetsläge DDI 141 = PÅ)

struct KartaCfg { uint8_t trig, startEv, stopEv; bool autoW; float widthM; };
static KartaCfg kcfg = {KT_ISOBUS, 0, 1, true, 10.5f};
static int kIsoWork = -1;            // ISOBUS-redskapet arbetar: 1/0, -1 = inget redskap
static Preferences kPrefs;

static void kartaTrigNamn(int t, char *b, int n) {
  if (t == KT_MANUELL) snprintf(b, n, "%s", L("Manuellt (knapp)", "Manual (button)"));
  else if (t == KT_ISOBUS) snprintf(b, n, "ISOBUS (auto)");
  else if (t <= KT_VENTIL7) snprintf(b, n, L("Ventil %s", "Valve %s"), ventilNamn(t));
  else if (t == KT_PTO_BAK) snprintf(b, n, "%s", L("Kraftuttag bak", "Rear PTO"));
  else if (t == KT_PTO_FRAM) snprintf(b, n, "%s", L("Kraftuttag fram", "Front PTO"));
  else if (t == KT_LYFT_BAK) snprintf(b, n, "%s", L("Lyft bak", "Rear hitch"));
  else snprintf(b, n, "%s", L("Lyft fram", "Front hitch"));
}
static const char *kartaEvNamn(int t, int ev) {
  if (t == KT_MANUELL) return L("knapp", "button");
  if (t == KT_ISOBUS) return ev == 0 ? L("arbetar", "working") : L("slutar", "stops");
  if (t <= KT_VENTIL7) return ev == 0 ? L("UT (+)", "OUT (+)") : L("IN (-)", "IN (-)");
  if (t <= KT_PTO_FRAM) return ev == 0 ? L("PÅ", "ON") : L("AV", "OFF");
  return ev == 0 ? L("NER", "DOWN") : L("UPP", "UP");
}

static void kartaSaveCfg() {
  kPrefs.putUChar("trig", kcfg.trig);
  kPrefs.putUChar("sEv", kcfg.startEv);
  kPrefs.putUChar("pEv", kcfg.stopEv);
  kPrefs.putBool("autoW", kcfg.autoW);
  kPrefs.putFloat("wM", kcfg.widthM);
}

// ---------------------------------------------------------------- Koordinater
// Lokala meter (x = öster, y = norr) runt en origo, och Web Mercator för kartrutorna.
static double kMperDegLat(double lat) {
  double p = lat * M_PI / 180.0;
  return 111132.954 - 559.822 * cos(2 * p) + 1.175 * cos(4 * p);
}
static double kMperDegLon(double lat) {
  double p = lat * M_PI / 180.0;
  return 111412.84 * cos(p) - 93.5 * cos(3 * p);
}
static void kTileXY(double lat, double lon, int z, double &X, double &Y) {     // global bildpunkt
  double n = 256.0 * (1 << z);
  X = (lon + 180.0) / 360.0 * n;
  double p = lat * M_PI / 180.0;
  Y = (1.0 - log(tan(p) + 1.0 / cos(p)) / M_PI) / 2.0 * n;
}
static double kTileMpp(double lat, int z) { return 156543.03392 * cos(lat * M_PI / 180.0) / (1 << z); }

// ---------------------------------------------------------------- Rutnät och spår
#define KG_N    4096                 // 4096 x 4096 rutor à 1 m
#define KG_HALF 2048
static uint32_t *kGrid = nullptr;    // 1 bit per ruta, PSRAM (2 MB)
static bool   kHasOrigin = false;
static double kLat0 = 0, kLon0 = 0, kMx = 1, kMy = 1;   // origo och meter per grad
static volatile bool kLogging = false;
static uint32_t kCells = 0;          // målade rutor = m²
static double kDistM = 0;
static bool   kHasLast = false;
static float  kLastX = 0, kLastY = 0;
static uint32_t kLastPosMs = 0;
static float  kWidthNow = 10.5f;
#define K_BAK_M 3.0f                 // redskapet sitter så här långt bakom antennen (m)
static float  kFwdX = 0, kFwdY = 1;  // traktorns framåtriktning (lokala meter, enhetsvektor)
static bool   kHasFwd = false;
static double kAntLat = 0, kAntLon = 0; static bool kHasAnt = false;
static bool   kWidthFromBus = false;
static int    kOutside = 0;          // punkter utanför rutnätet
#define K_VAND_MS 30000UL             // v37: uppehåll i loggningen upp till 30 s = vändning (räknas som arbete)
static double kFuelL = 0;            // bränsle under loggning (liter)
static double kFuelSaved = 0;
static uint32_t kFuelMs = 0;

struct KRec { uint8_t typ; uint8_t nytt; uint16_t wcm; int32_t xcm, ycm; uint32_t res; };   // 16 byte
#define KBUF_N 128
static KRec kBuf[KBUF_N];
static int kBufN = 0;
static uint32_t kLastFlush = 0;
static File kReplay;
static bool kReplaying = false;

// ---------------------------------------------------------------- Färg på spåret (v36)
// Bränsle (l/ha), dragkraft (kN) och slirning (%, v41) där redskapet gått, i rutor om 2 × 2 m. Kanten på
// spåret kommer fortfarande från 1 m-rutnätet; bara färgen är grövre. Glest: block om 64 × 64 rutor
// (128 × 128 m, 12 kB) tas ur PSRAM först när något målas där, högst KF_MAXB block (≈ 200 ha). Senaste varvet vinner.
// Koder (0 = inget värde): bränsle 1 + l/ha × 4 (0..63,5 l/ha), dragkraft 1 + (kN + 20) × 2 (-20..107 kN),
// slirning 1 + (% + 20) × 2 (-20..107 %).
#define KF_CELL 2
#define KF_N    (KG_N / KF_CELL)     // 2048 rutor per sida
#define KF_B    64                   // rutor per block och sida
#define KF_NB   (KF_N / KF_B)        // 32 block per sida
#define KF_MAXB 160
#define KF_V    3                    // värden per ruta: bränsle, dragkraft, slirning
static uint8_t  kfDir[KF_NB * KF_NB];   // 0 = inget block, annars index + 1
static uint8_t *kfBlk[KF_MAXB];         // [rad][kolumn][KF_V]: bränslekod, dragkod, slirkod
static int      kfNB = 0;
static double   kfSum[KF_V] = {0, 0, 0};   // summa och antal rutor med värde (för körningens snitt)
static uint32_t kfCnt[KF_V] = {0, 0, 0};
static uint8_t  kBakgrund = 0;          // v37: 0 = satellitbild, 1 = enfärgad som S4-styrbilden (NVS "karta"/"bg")
static uint8_t  kfLage = 0;             // vad kartan färgas efter: 0 = bränsle, 1 = dragkraft, 2 = slirning (NVS "karta"/"farg")
static float    kfFuel = NAN, kfKmh = NAN, kfKN = NAN, kfSl = NAN;   // glidande medel medan kartan loggar
static uint32_t kfMs = 0;

static inline float kfVal(int k, uint8_t c) { return k ? (c - 1) * 0.5f - 20.0f : (c - 1) * 0.25f; }
static inline uint8_t kfKod(int k, float v) {
  if (isnan(v)) return 0;
  float q = k ? 1 + (v + 20.0f) * 2.0f : 1 + v * 4.0f;
  return q < 1 ? 1 : (q > 255 ? 255 : (uint8_t)lroundf(q));
}
static inline uint8_t *kfCell(int cx, int cy, bool skapa) {
  int b = (cy / KF_B) * KF_NB + cx / KF_B;
  int i = kfDir[b];
  if (!i) {
    if (!skapa || kfNB >= KF_MAXB) return nullptr;
    uint8_t *p = (uint8_t *)heap_caps_calloc(KF_B * KF_B * KF_V, 1, MALLOC_CAP_SPIRAM);
    if (!p) return nullptr;
    kfBlk[kfNB++] = p; kfDir[b] = i = kfNB;
  }
  return kfBlk[i - 1] + ((cy % KF_B) * KF_B + cx % KF_B) * KF_V;
}
static inline void kfSet(int gx, int gy, uint8_t f, uint8_t d, uint8_t sl) {
  if ((unsigned)gx >= KG_N || (unsigned)gy >= KG_N) return;
  uint8_t *c = kfCell(gx / KF_CELL, gy / KF_CELL, true);
  if (!c) return;
  for (int k = 0; k < KF_V; k++) {
    uint8_t v = k == 2 ? sl : (k ? d : f);
    if (!v || c[k] == v) continue;
    if (c[k]) { kfSum[k] -= kfVal(k, c[k]); kfCnt[k]--; }
    c[k] = v; kfSum[k] += kfVal(k, v); kfCnt[k]++;
  }
}
static void kfNolla() {
  for (int i = 0; i < kfNB; i++) { heap_caps_free(kfBlk[i]); kfBlk[i] = nullptr; }
  kfNB = 0; memset(kfDir, 0, sizeof(kfDir));
  for (int k = 0; k < KF_V; k++) { kfSum[k] = 0; kfCnt[k] = 0; }
}
// Färgskalan: körningens snitt = blått, -30 % = grönt, +30 % = rött (dragkraft minst ±1,5 kN, slirning minst
// ±1,5 procentenheter, bränsle minst ±0,3 l/ha)
static float kfSnitt(int k) { return kfCnt[k] ? (float)(kfSum[k] / kfCnt[k]) : NAN; }
static float kfSpann(int k) { float a = kfSnitt(k); return 0.3f * fmaxf(fabsf(a), k ? 5.0f : 1.0f); }
static uint16_t kfFarg(float t) { return uiSkalaFarg(t); }   // t = -1 (grönt) .. 0 (blått) .. +1 (rött)

static inline bool kGet(int gx, int gy) {
  if ((unsigned)gx >= KG_N || (unsigned)gy >= KG_N) return false;
  uint32_t i = (uint32_t)gy * KG_N + gx;
  return (kGrid[i >> 5] >> (i & 31)) & 1;
}
static inline void kSet(int gx, int gy) {
  if ((unsigned)gx >= KG_N || (unsigned)gy >= KG_N) return;
  uint32_t i = (uint32_t)gy * KG_N + gx, m = 1u << (i & 31);
  if (!(kGrid[i >> 5] & m)) { kGrid[i >> 5] |= m; kCells++; }
}
// Målar en "kapsel": sträckan (x0,y0)-(x1,y1) med bredden w (meter, lokala koordinater).
// f, d, sl = färgkoder för bränsle, dragkraft och slirning (0 = inget värde, rutans gamla värde står kvar)
static void kPaint(float x0, float y0, float x1, float y1, float w, uint8_t f = 0, uint8_t d = 0, uint8_t sl = 0) {
  if (!kGrid) return;
  float r = w / 2.0f, r2 = r * r;
  float dx = x1 - x0, dy = y1 - y0, L2 = dx * dx + dy * dy;
  int gx0 = (int)floorf(fminf(x0, x1) - r) + KG_HALF, gx1 = (int)floorf(fmaxf(x0, x1) + r) + KG_HALF;
  int gy0 = (int)floorf(fminf(y0, y1) - r) + KG_HALF, gy1 = (int)floorf(fmaxf(y0, y1) + r) + KG_HALF;
  if (gx1 < 0 || gy1 < 0 || gx0 >= KG_N || gy0 >= KG_N) { kOutside++; return; }
  for (int gy = gy0; gy <= gy1; gy++) {
    float py = gy - KG_HALF + 0.5f;
    for (int gx = gx0; gx <= gx1; gx++) {
      float px = gx - KG_HALF + 0.5f;
      float t = L2 > 0 ? ((px - x0) * dx + (py - y0) * dy) / L2 : 0;
      if (t < 0) t = 0; else if (t > 1) t = 1;
      float ex = x0 + t * dx - px, ey = y0 + t * dy - py;
      if (ex * ex + ey * ey <= r2) { kSet(gx, gy); if (f | d | sl) kfSet(gx, gy, f, d, sl); }
    }
  }
}

static void kSetOrigin(double lat, double lon) {
  kLat0 = lat; kLon0 = lon; kHasOrigin = true;
  kMx = kMperDegLon(lat); kMy = kMperDegLat(lat);
}
static inline void kToLocal(double lat, double lon, float &x, float &y) {
  x = (float)((lon - kLon0) * kMx);
  y = (float)((lat - kLat0) * kMy);
}

static void kFlush() {
  if (!kBufN || !sdOk) { kBufN = 0; return; }
  sdLock();
  File f = SD.open("/KARTA/SPAR.BIN", FILE_APPEND);
  if (f) { f.write((uint8_t *)kBuf, sizeof(KRec) * kBufN); f.close(); }
  sdUnlock();
  kBufN = 0;
}
// res: b0 = bränslekod, b1 = dragkod (v36), b2 = slirkod (v41) för sträckan fram till punkten (äldre spår har 0 = ingen färg)
static void kRecord(uint8_t nytt, float x, float y, float w, uint8_t f = 0, uint8_t d = 0, uint8_t sl = 0) {
  if (kBufN >= KBUF_N) kFlush();
  KRec &r = kBuf[kBufN++];
  r.typ = 'P'; r.nytt = nytt; r.wcm = (uint16_t)lroundf(w * 100);
  r.xcm = (int32_t)lroundf(x * 100); r.ycm = (int32_t)lroundf(y * 100); r.res = f | ((uint32_t)d << 8) | ((uint32_t)sl << 16);
}

static int kTrigState(uint32_t now);
static void kartaKnapp(uint32_t now);
static void kartaStartLog() {
  if (kLogging) return;
  kLogging = true;
  kHasLast = false;
}
static void kartaStopLog() {
  if (!kLogging) return;
  kLogging = false;
  kHasLast = false;
  kFlush();
  if (fabs(kFuelL - kFuelSaved) > 0.01) { kPrefs.putDouble("fuel", kFuelL); kFuelSaved = kFuelL; }
}

// ---------------------------------------------------------------- Skiftets areal (v45, 10.19)
// Föraren knappar in skiftets areal (hektar). Takten (ha per arbetad timme) räknas ur de senaste ~20 minuterna
// av loggning (vändningar ingår, uppehåll då loggningen är av räknas inte), återstående tid = kvar / takt.
static float kSkiftHa = 0;                       // 0 = inget skifte inlagt
#define KS_N 80                                  // prov var 15:e sekund = 20 minuter
static float kSArea[KS_N]; static uint32_t kSSek[KS_N];   // areal (ha) och arbetade sekunder vid provet
static int kSN = 0, kSHead = 0;
static uint32_t kSWork = 0, kSLastMs = 0, kSLastSample = 0;
static void kSkiftNolla() { kSN = 0; kSHead = 0; kSWork = 0; kSLastSample = 0; }
static void kartaSkift(float ha) { kSkiftHa = (ha > 0 && ha < 1000) ? ha : 0; kPrefs.putFloat("skift", kSkiftHa); }
static void kSkiftTick(uint32_t now) {           // anropas från kartaLoop
  uint32_t dt = now - kSLastMs; kSLastMs = now;
  if (dt > 5000) dt = 0;
  if (!kLogging) return;
  static uint32_t rest = 0; rest += dt; kSWork += rest / 1000; rest %= 1000;
  if (kSWork - kSLastSample >= 15 || (kSN == 0 && kSWork > 0)) {
    kSLastSample = kSWork;
    kSArea[kSHead] = kCells / 10000.0f; kSSek[kSHead] = kSWork;
    kSHead = (kSHead + 1) % KS_N; if (kSN < KS_N) kSN++;
  }
}
// Takt i ha/h, NAN tills det finns minst 3 minuters arbete att räkna på
static float kSkiftTakt() {
  if (kSN < 13) return NAN;
  int nyast = (kSHead + KS_N - 1) % KS_N, aldst = (kSHead + KS_N - kSN) % KS_N;
  uint32_t ds = kSSek[nyast] - kSSek[aldst];
  float dA = kSArea[nyast] - kSArea[aldst];
  if (ds < 180 || dA <= 0.0005f) return NAN;
  return dA / (ds / 3600.0f);
}

// Nollar kartan, arealen och spåret på SD-kortet
static void kartaNolla() {
  kSkiftHa = 0; kPrefs.putFloat("skift", 0); kSkiftNolla();
  kLogging = false; kHasLast = false; kBufN = 0;
  if (kGrid) memset(kGrid, 0, KG_N / 8 * KG_N);
  kfNolla();
  kCells = 0; kDistM = 0; kOutside = 0; kHasOrigin = false;
  kFuelL = 0; kFuelSaved = 0; kPrefs.putDouble("fuel", 0);
  if (kReplaying) { kReplay.close(); kReplaying = false; }
  if (sdOk) {
    sdLock();
    SD.remove("/KARTA/SPAR.BIN");
    SD.remove("/KARTA/SPAR.ORG");
    sdUnlock();
  }
}

// ---------------------------------------------------------------- Kartrutor (satellitbild)
#define KT_SIZE   256
#define KC_N      16                 // rutor i minnet (128 kB var, PSRAM)
struct KTile { int8_t z; int32_t x, y; uint16_t *px; uint32_t used; bool ok; };
static KTile kCache[KC_N];
static uint32_t kUseTick = 0;
static int kLoadBudget = 0;

static void kTilePath(char *p, int n, int z, int x, int y);   // längre ner (beror på kartkällan)

// Rutan (z,x,y) ur minnet eller från SD-kortet. nullptr = finns inte (än).
static const uint16_t *kTileGet(int z, int x, int y) {
  for (int i = 0; i < KC_N; i++)
    if (kCache[i].px && kCache[i].z == z && kCache[i].x == x && kCache[i].y == y) {
      kCache[i].used = ++kUseTick;
      return kCache[i].ok ? kCache[i].px : nullptr;
    }
  if (kLoadBudget <= 0 || !sdOk) return nullptr;
  kLoadBudget--;
  int v = 0;
  for (int i = 1; i < KC_N; i++) if (kCache[i].used < kCache[v].used) v = i;
  KTile &t = kCache[v];
  if (!t.px) t.px = (uint16_t *)heap_caps_malloc(KT_SIZE * KT_SIZE * 2, MALLOC_CAP_SPIRAM);
  if (!t.px) return nullptr;
  t.z = z; t.x = x; t.y = y; t.used = ++kUseTick; t.ok = false;
  char p[40]; kTilePath(p, sizeof(p), z, x, y);
  sdLock();
  File f = SD.open(p, FILE_READ);
  if (f) { t.ok = f.read((uint8_t *)t.px, KT_SIZE * KT_SIZE * 2) == KT_SIZE * KT_SIZE * 2; f.close(); }
  sdUnlock();
  return t.ok ? t.px : nullptr;
}
static void kTileForget(int z, int x, int y) {      // ny ruta nedladdad: läs om nästa gång
  for (int i = 0; i < KC_N; i++)
    if (kCache[i].px && kCache[i].z == z && kCache[i].x == x && kCache[i].y == y) kCache[i].used = 0, kCache[i].z = -1;
}

// ---------------------------------------------------------------- Rita kartan
static inline uint16_t kBlend(uint16_t a, uint16_t b) { return ((a & 0xF7DE) >> 1) + ((b & 0xF7DE) >> 1); }
static const uint16_t K_COVER = RGB(120, 230, 60);
static const uint16_t K_EMPTY = RGB(22, 27, 32);

// Panorering av helskärmskartan (meter öster/norr om traktorn). kFollow = följ traktorn.
static float kPanE = 0, kPanN = 0;
static bool  kFollow = true;
static void kartaFolj() { kPanE = kPanN = 0; kFollow = true; }
static void kartaPan(float dE, float dN) { kPanE += dE; kPanN += dN; kFollow = false; }

// Vad kartan ska visas runt (fordonet om position finns)
// v37: fältrapporten ritar kartan centrerad på fältet och utan fordonet
static bool kOvSet = false;
static double kOvLat = 0, kOvLon = 0;
static bool kViewCenter(double &lat, double &lon) {
  if (kOvSet) { lat = kOvLat; lon = kOvLon; return false; }
  if (trd.posMs) { lat = trd.lat; lon = trd.lon; return true; }
  if (kHasOrigin) { lat = kLat0; lon = kLon0; return false; }
  lat = lon = 0; return false;                 // ingen position än: mörk bakgrund
}

// Ritar kartan i rutan (x,y,w,h) med mpp meter per bildpunkt, centrerad på fordonet
static inline int cxr(int c, float f, float d) { return c - (int)lroundf(f * d); }   // punkt d bakom c

static void kartaRender(Arduino_GFX *g, int x0, int y0, int w, int h, float mpp, bool pan) {
  static uint16_t row[800];
  if (w > 800) w = 800;
  double clat, clon;
  bool hasPos = kViewCenter(clat, clon);
  float offE = 0, offN = 0;                    // vyn är flyttad så här mycket från traktorn
  if (pan && !kFollow) {
    offE = kPanE; offN = kPanN;
    clat += offN / kMperDegLat(clat);
    clon += offE / kMperDegLon(clat);
  }
  int z = mpp >= 12 ? 12 : (mpp >= 6 ? 13 : 14);
  double CX, CY; kTileXY(clat, clon, z, CX, CY);
  double tm = kTileMpp(clat, z);
  double sc = mpp / tm;                                   // rutbildpunkter per skärmbildpunkt
  float cxm = 0, cym = 0;
  if (kHasOrigin) kToLocal(clat, clon, cxm, cym);
  kLoadBudget = kOvSet ? 400 : 4;                 // fältrapporten får läsa alla rutor den behöver
  // Heltalsräkning i den inre loopen (16.16 fast komma) – ESP32-S3 saknar snabb double
  const int32_t stepT = (int32_t)lround(sc * 65536.0);              // rutbildpunkter per skärmbildpunkt
  const int32_t stepG = (int32_t)lroundf(mpp * 65536.0f);           // rutnätsrutor (m) per skärmbildpunkt
  const bool cover = kHasOrigin && kGrid;
  // Färgen för varje kod i valt läge (blandad 3/4 över satellitbilden)
  static uint16_t lut[256];
  const int fk = kfLage;
  const bool farg = kfNB > 0 && kfCnt[fk] > 0;
  if (farg) {
    float a = kfSnitt(fk), sp = kfSpann(fk);
    lut[0] = K_COVER;
    for (int c = 1; c < 256; c++) lut[c] = kfFarg((kfVal(fk, (uint8_t)c) - a) / sp);
  }
  // v37: enfärgad bakgrund som S4-styrbilden – ljusgul (mörk på natten) med tunna linjer var 25:e meter
  const bool enf = kBakgrund == 1;
  const bool natt = P.pan == S4_NATT.pan;
  const uint16_t plain = natt ? RGB(58, 60, 40) : RGB(232, 232, 184);
  const uint16_t plainL = natt ? RGB(80, 82, 58) : RGB(206, 206, 158);
  const int32_t lineM = mpp > 4 ? 200 : (mpp > 1 ? 100 : 25);
  for (int sy = 0; sy < h; sy++) {
    double ty = CY + (sy - h / 2) * sc;
    int tyi = (int)floor(ty / KT_SIZE), py = (int)floor(ty) - tyi * KT_SIZE;
    double tx0 = CX - (w / 2) * sc;
    int tx0i = (int)floor(tx0 / KT_SIZE);
    int32_t fx = (int32_t)lround((tx0 - (double)tx0i * KT_SIZE) * 65536.0);
    int gy = (int)floorf(cym - (sy - h / 2) * mpp) + KG_HALF;
    bool rowCov = cover && (unsigned)gy < KG_N;
    int32_t fg = (int32_t)lroundf((cxm - (w / 2) * mpp + KG_HALF) * 65536.0f);
    const uint32_t *grow = rowCov ? kGrid + ((uint32_t)gy * KG_N >> 5) : nullptr;
    int curT = INT32_MIN; const uint16_t *tp = nullptr;
    for (int sx = 0; sx < w; sx++, fx += stepT, fg += stepG) {
      int pxa = fx >> 16;
      uint16_t c;
      if (enf) {
        int gxm = (fg >> 16) - KG_HALF, gym = gy - KG_HALF;
        bool linje = (((gxm % lineM) + lineM) % lineM) < (int)ceilf(mpp) || (((gym % lineM) + lineM) % lineM) < (int)ceilf(mpp);
        c = linje ? plainL : plain;
      } else {
        int ti = tx0i + (pxa >> 8);
        if (ti != curT) { curT = ti; tp = kTileGet(z, ti, tyi); }
        c = tp ? tp[py * KT_SIZE + (pxa & 255)] : K_EMPTY;
      }
      if (grow) {
        int gx = fg >> 16;
        if ((unsigned)gx < KG_N && ((grow[gx >> 5] >> (gx & 31)) & 1)) {
          const uint8_t *fc = farg ? kfCell(gx / KF_CELL, gy / KF_CELL, false) : nullptr;
          if (fc && fc[fk]) { uint16_t lc = lut[fc[fk]]; c = kBlend(kBlend(c, lc), lc); }
          else c = kBlend(c, K_COVER);
        }
      }
      row[sx] = c;
    }
    g->draw16bitRGBBitmap(x0, y0 + sy, row, w, 1);
  }
  // Fordonet: pil i kursriktningen och redskapsbredden tvärs över
  int vx = x0 + w / 2 - (int)lroundf(offE / mpp), vy = y0 + h / 2 + (int)lroundf(offN / mpp);
  if (hasPos && vx > x0 - 60 && vx < x0 + w + 60 && vy > y0 - 60 && vy < y0 + h + 60) {
    int cx = vx, cy = vy;
    float fx = kFwdX, fy = -kFwdY;                              // skärm: y nedåt
    if (!kHasFwd && !trd.kursMs) { fx = 0; fy = -1; }
    float rx = -fy, ry = fx;                                     // höger om färdriktningen
    float half = kWidthNow / 2.0f / mpp;
    if (half < 6) half = 6;
    float bk = K_BAK_M / mpp;                                    // redskapet bakom antennen
    int bxc = cx - (int)lroundf(fx * bk), byc = cy - (int)lroundf(fy * bk);
    uint16_t bc = kLogging ? K_COVER : UI_TEXT;
    // Markörens storlek: ungefär skalenlig (5,5 m lång), minst 18 och högst 64 bildpunkter
    float Lpx = 5.5f / mpp;
    if (Lpx < 18) Lpx = 18;
    if (Lpx > TROVAN_W) Lpx = TROVAN_W;
    float msc = Lpx / TROVAN_W;                                  // bildens skala
    // Redskapsbalken bakom traktorn (minst så långt bak att den syns bakom markören)
    float bkp = fmaxf(bk, (TROVAN_W - 37) * msc + 3);
    bxc = cx - (int)lroundf(fx * bkp); byc = cy - (int)lroundf(fy * bkp);
    uiThick(g, cxr(cx, fx, (TROVAN_W - 37) * msc), cxr(cy, fy, (TROVAN_W - 37) * msc), bxc, byc, 2, UI_DIM);
    uiThick(g, bxc - rx * half, byc - ry * half, bxc + rx * half, byc + ry * half, 3, bc);
    // Traktorn ovanifrån, vriden i färdriktningen. Bilden har fronten åt vänster;
    // antennen (hyttaket) ligger vid PX,PY i bilden.
    const float PX = 37, PY = TROVAN_H / 2.0f;
    int R = (int)ceilf(40 * msc) + 1;
    for (int dy = -R; dy <= R; dy++) {
      int yy = cy + dy;
      if (yy < y0 || yy >= y0 + h) continue;
      for (int dx = -R; dx <= R; dx++) {
        int xx = cx + dx;
        if (xx < x0 || xx >= x0 + w) continue;
        float u = (dx * fx + dy * fy) / msc;  // framåt (i bildens bildpunkter)
        float v = (dx * rx + dy * ry) / msc;  // åt höger
        int sx = (int)lroundf(PX - u), sy = (int)lroundf(PY - v);
        if ((unsigned)sx >= TROVAN_W || (unsigned)sy >= TROVAN_H) continue;
        uint16_t c = TROVAN[sy * TROVAN_W + sx];
        if (c != TROVAN_KEY) g->drawPixel(xx, yy, c);
      }
    }
  }
}

// ---------------------------------------------------------------- Trigger
static int kPrevState = -2;

// Läget för vald trigger: 0 = händelse 0 (UT/PÅ/NER), 1 = händelse 1 (IN/AV/UPP), -1 = neutral/okänt
static int kTrigState(uint32_t now) {
  int t = kcfg.trig;
  if (t == KT_ISOBUS) return kIsoWork == 1 ? 0 : (kIsoWork == 0 ? 1 : -1);
  if (t >= KT_VENTIL1 && t <= KT_VENTIL7) {
    if (!trd.ventMs[t] || now - trd.ventMs[t] > 3000) return -1;
    float ut = trd.ventUt[t], in = trd.ventIn[t];
    if (ut >= 10 && ut >= in) return 0;
    if (in >= 10) return 1;
    return -1;
  }
  if (t == KT_PTO_BAK || t == KT_PTO_FRAM) {
    const TrVal &p = t == KT_PTO_BAK ? trd.ptoBak : trd.ptoFram;
    bool on = p.ms && now - p.ms < 3000 && p.v > 50;
    return on ? 0 : 1;
  }
  if (t == KT_LYFT_BAK || t == KT_LYFT_FRAM) {
    const TrVal &l = t == KT_LYFT_BAK ? trd.lyftBak : trd.lyftFram;
    int arb = t == KT_LYFT_BAK ? trd.arbBak : trd.arbFram;
    if (!l.ms || now - l.ms > 3000) return -1;
    if (arb == 1) return 0;
    if (arb == 0) return 1;
    return l.v < 50 ? 0 : 1;
  }
  return -1;
}

// ---------------------------------------------------------------- Nedladdning av kartrutor (körs i nät-tasken)
#define KC_CENTRA 6
struct KCentrum { float lat, lon; };
static KCentrum kCentra[KC_CENTRA];
static int kNCentra = 0;
static volatile bool kNewPass = true;
static int kPassIdx = 0, kPassTotal = 0, kTilesOk = 0, kTilesFail = 0, kLastCode = 0;
static volatile bool kDlDone = false;
static uint32_t kPassDoneMs = 0;
// Kartbildens källa. Sentinel-2 cloudless 2016 är CC BY 4.0 (fri även i yrkesmässig användning, källan måste anges).
// Senare årgångar (2018 och framåt) är CC BY-NC-SA 4.0 – INTE tillåtna i kommersiell användning utan licens från EOX.
// Ett lantbruksföretag räknas troligen som kommersiellt, därför används 2016 som förval.
struct KKalla { const char *url; const char *mapp; int ar; const char *licens; };
static const KKalla K_KALLOR[] = {
  { "https://tiles.maps.eox.at/wmts/1.0.0/s2cloudless_3857/default/g/%d/%d/%d.jpg", "/KARTA16", 2016, "CC BY 4.0" },
};
static const int kKalla = 0;
static int kBildAr = 2016;
static char kBildTxt[96];
static const char *kBildText() {           // bildtexten som ska stå på skärmen och i rapporten
  snprintf(kBildTxt, sizeof(kBildTxt), L("Satellitbild: Sentinel-2 cloudless %d av EOX IT Services GmbH (%s)",
                                         "Satellite image: Sentinel-2 cloudless %d by EOX IT Services GmbH (%s)"),
           K_KALLOR[kKalla].ar, K_KALLOR[kKalla].licens);
  return kBildTxt;
}
#define K_RADIE_M 6000.0
static const int K_ZOOM[] = {12, 13, 14};

// Rutan nr i i nuvarande omgång -> (z,x,y). Returnerar false när listan är slut.
static bool kPassTile(int i, int &z, int &x, int &y) {
  for (int c = 0; c < kNCentra; c++) {
    double lat = kCentra[c].lat, lon = kCentra[c].lon;
    double dLat = K_RADIE_M / kMperDegLat(lat), dLon = K_RADIE_M / kMperDegLon(lat);
    for (int zi = 0; zi < 3; zi++) {
      int zz = K_ZOOM[zi];
      double X0, Y0, X1, Y1;
      kTileXY(lat + dLat, lon - dLon, zz, X0, Y0);
      kTileXY(lat - dLat, lon + dLon, zz, X1, Y1);
      int tx0 = (int)(X0 / 256), tx1 = (int)(X1 / 256), ty0 = (int)(Y0 / 256), ty1 = (int)(Y1 / 256);
      int cnt = (tx1 - tx0 + 1) * (ty1 - ty0 + 1);
      if (i < cnt) { z = zz; x = tx0 + i % (tx1 - tx0 + 1); y = ty0 + i / (tx1 - tx0 + 1); return true; }
      i -= cnt;
    }
  }
  return false;
}

// JPEG -> RGB565 med ESP32-S3:ans inbyggda avkodare
struct KJpg { const uint8_t *d; uint32_t n, pos; uint16_t *out; };
static UINT kJpgIn(JDEC *jd, BYTE *buf, UINT n) {
  KJpg *j = (KJpg *)jd->device;
  if (j->pos + n > j->n) n = j->n - j->pos;
  if (buf) memcpy(buf, j->d + j->pos, n);
  j->pos += n;
  return n;
}
static UINT kJpgOut(JDEC *jd, void *bm, JRECT *r) {
  KJpg *j = (KJpg *)jd->device;
  const uint8_t *s = (const uint8_t *)bm;
  for (int y = r->top; y <= r->bottom; y++)
    for (int x = r->left; x <= r->right; x++, s += 3)
      if (x < KT_SIZE && y < KT_SIZE)
        j->out[y * KT_SIZE + x] = ((s[0] & 0xF8) << 8) | ((s[1] & 0xFC) << 3) | (s[2] >> 3);
  return 1;
}

static uint8_t *kDlBuf = nullptr;
static uint16_t *kDlPx = nullptr;

static void kTilePath(char *p, int n, int z, int x, int y) { snprintf(p, n, "%s/%d/%d_%d.RGB", K_KALLOR[kKalla].mapp, z, x, y); }

static int kDownload(int z, int x, int y) {
  char url[160];
  int code = -1;
  {
    snprintf(url, sizeof(url), K_KALLOR[kKalla].url, z, y, x);
    WiFiClientSecure c;
    c.setInsecure();                         // bara bilder (avkodas och kontrolleras), inget program
    HTTPClient http;
    http.setTimeout(15000);
    if (!http.begin(c, url)) return -10;
    http.setUserAgent("Faltloggen/1.0 (ESP32; +https://github.com/statsminister1-blip/faltloggen)");
    code = http.GET();
    if (code != 200) { http.end(); return code; }
    WiFiClient *st = http.getStreamPtr();
    int len = http.getSize();
    uint32_t n = 0, t0 = millis();
    while (http.connected() && (len < 0 || (int)n < len) && n < 262144 && millis() - t0 < 20000) {
      int a = st->available();
      if (a > 0) n += st->readBytes(kDlBuf + n, (size_t)min(a, (int)(262144 - n)));
      else delay(5);
    }
    http.end();
    if (n < 100) return -11;
    // avkoda
    static uint8_t pool[3200];
    JDEC jd; KJpg j = {kDlBuf, n, 0, kDlPx};
    memset(kDlPx, 0, KT_SIZE * KT_SIZE * 2);
    if (jd_prepare(&jd, kJpgIn, pool, sizeof(pool), &j) != JDR_OK || jd_decomp(&jd, kJpgOut, 0) != JDR_OK) return -12;
    char p[40];
    snprintf(p, sizeof(p), "%s/%d", K_KALLOR[kKalla].mapp, z);
    kTilePath(url, sizeof(url), z, x, y);
    sdLock();
    if (!SD.exists("/KARTA")) SD.mkdir("/KARTA");
    if (!SD.exists(K_KALLOR[kKalla].mapp)) SD.mkdir(K_KALLOR[kKalla].mapp);
    if (!SD.exists(p)) SD.mkdir(p);
    File f = SD.open(url, FILE_WRITE);
    bool ok = f && f.write((uint8_t *)kDlPx, KT_SIZE * KT_SIZE * 2) == KT_SIZE * KT_SIZE * 2;
    if (f) f.close();
    sdUnlock();
    if (!ok) return -13;
    kTileForget(z, x, y);
    return 200;
  }
}

// Ett steg: hoppa över rutor som finns, hämta högst en ny. Anropas från nät-tasken.
static void kartaNetStep() {
  if (!sdOk) return;
  if (!kDlBuf) kDlBuf = (uint8_t *)heap_caps_malloc(262144, MALLOC_CAP_SPIRAM);
  if (!kDlPx) kDlPx = (uint16_t *)heap_caps_malloc(KT_SIZE * KT_SIZE * 2, MALLOC_CAP_SPIRAM);
  if (!kDlBuf || !kDlPx) return;
  if (kNewPass) {
    kNewPass = false; kPassIdx = 0; kTilesOk = 0; kTilesFail = 0; kDlDone = false;
    int z, x, y; kPassTotal = 0;
    while (kPassTile(kPassTotal, z, x, y)) kPassTotal++;
  }
  if (kDlDone) {
    if (millis() - kPassDoneMs > 6UL * 3600 * 1000 && kTilesFail) kNewPass = true;   // försök igen senare
    return;
  }
  for (int k = 0; k < 40; k++) {
    int z, x, y;
    if (!kPassTile(kPassIdx, z, x, y)) { kDlDone = true; kPassDoneMs = millis(); return; }
    char p[40]; kTilePath(p, sizeof(p), z, x, y);
    sdLock(); bool finns = SD.exists(p); sdUnlock();
    if (finns) { kTilesOk++; kPassIdx++; continue; }
    int code = kDownload(z, x, y);
    kLastCode = code;
    if (code == 200) kTilesOk++; else kTilesFail++;
    kPassIdx++;
    return;                                  // en nedladdning per steg
  }
}

static void kAddCentrum(double lat, double lon) {
  for (int i = 0; i < kNCentra; i++) {
    double dy = (lat - kCentra[i].lat) * 111000.0, dx = (lon - kCentra[i].lon) * 57000.0;
    if (dx * dx + dy * dy < 4000.0 * 4000.0) return;     // redan täckt
  }
  if (kNCentra >= KC_CENTRA) return;
  kCentra[kNCentra].lat = lat; kCentra[kNCentra].lon = lon; kNCentra++;
  kPrefs.putBytes("centra", kCentra, sizeof(KCentrum) * kNCentra);
  kNewPass = true;
}

// ---------------------------------------------------------------- Uppstart och loop
static void kartaBegin() {
  kGrid = (uint32_t *)heap_caps_calloc(KG_N / 32 * KG_N, 4, MALLOC_CAP_SPIRAM);
  kPrefs.begin("karta", false);
  kcfg.trig = kPrefs.getUChar("trig", KT_ISOBUS);
  kcfg.startEv = kPrefs.getUChar("sEv", 0);
  kcfg.stopEv = kPrefs.getUChar("pEv", 1);
  kcfg.autoW = kPrefs.getBool("autoW", true);
  kcfg.widthM = kPrefs.getFloat("wM", 10.5f);
  if (kcfg.trig >= KT_N) kcfg.trig = KT_ISOBUS;
  kfLage = kPrefs.getUChar("farg", 0); if (kfLage > 2) kfLage = 0;
  kBakgrund = kPrefs.getUChar("bg", 0) ? 1 : 0;
  kSkiftHa = kPrefs.getFloat("skift", 0);
  kFuelL = kFuelSaved = kPrefs.getDouble("fuel", 0);
  kNCentra = kPrefs.getBytes("centra", kCentra, sizeof(kCentra)) / sizeof(KCentrum);
  // inga platser än: den första läggs till när GPS-positionen kommer (kartaLoop)
  // Spåret från förra körningen läses in lite i taget i kartaLoop
  if (sdOk) {
    sdLock();
    if (!SD.exists("/KARTA")) SD.mkdir("/KARTA");
    File o = SD.open("/KARTA/SPAR.ORG", FILE_READ);
    if (o) {
      char b[64] = ""; int n = o.read((uint8_t *)b, sizeof(b) - 1); b[n > 0 ? n : 0] = 0; o.close();
      double la, lo;
      if (sscanf(b, "%lf %lf", &la, &lo) == 2) {
        kSetOrigin(la, lo);
        kReplay = SD.open("/KARTA/SPAR.BIN", FILE_READ);
        kReplaying = (bool)kReplay;
      }
    }
    sdUnlock();
  }
}

static int kOverride = -3;            // autoläget: START/STOPP-knappen gäller tills läget ändras

static void kartaLoop(uint32_t now, float busWidth, int isoWork) {
  kIsoWork = isoWork;
  kSkiftTick(now);
  // Spåret från SD-kortet (500 punkter per varv)
  if (kReplaying) {
    static KRec r[64]; static bool har = false; static float lx, ly;
    for (int k = 0; k < 8 && kReplaying; k++) {
      sdLock(); int n = kReplay.read((uint8_t *)r, sizeof(r)); sdUnlock();
      int m = n / sizeof(KRec);
      for (int i = 0; i < m; i++) {
        if (r[i].typ != 'P') continue;
        float x = r[i].xcm / 100.0f, y = r[i].ycm / 100.0f;
        if (!r[i].nytt && har) { kPaint(lx, ly, x, y, r[i].wcm / 100.0f, r[i].res & 0xFF, (r[i].res >> 8) & 0xFF, (r[i].res >> 16) & 0xFF); kDistM += sqrtf((x - lx) * (x - lx) + (y - ly) * (y - ly)); }
        lx = x; ly = y; har = true;
      }
      if (m < 64) { sdLock(); kReplay.close(); sdUnlock(); kReplaying = false; }
    }
  }

  // Bränsle medan loggningen är på (l/h * tid).
  // v37: ett uppehåll i loggningen på högst K_VAND_MS (vändning på vändtegen) räknas som arbete.
  // Bränslet under uppehållet hålls i väntan och läggs till när loggningen börjar igen inom tiden; annars släpps det.
  static bool kVarLog = false;
  static uint32_t kStoppMs = 0;
  static double kVanta = 0;
  if (kLogging != kVarLog) {
    if (!kLogging) { kStoppMs = now; kVanta = 0; }                       // loggningen stoppade: börja hålla i väntan
    else if (kStoppMs && now - kStoppMs <= K_VAND_MS) kFuelL += kVanta;   // igång igen inom 30 s = vändning
    kVanta = 0;
    kVarLog = kLogging;
  }
  if (trd.bransle.ms && now - trd.bransle.ms < 5000 && kFuelMs) {
    float dt = (now - kFuelMs) / 1000.0f;
    if (dt > 0 && dt < 5) {
      double l = trd.bransle.v * dt / 3600.0;
      if (kLogging) kFuelL += l;
      else if (kStoppMs && now - kStoppMs <= K_VAND_MS) kVanta += l;
    }
  }
  kFuelMs = now;
  static uint32_t lastFuelSave = 0;
  if (kLogging && now - lastFuelSave > 60000) {
    lastFuelSave = now;
    if (fabs(kFuelL - kFuelSaved) > 0.01) { kPrefs.putDouble("fuel", kFuelL); kFuelSaved = kFuelL; }
  }

  // Arbetsbredd
  kWidthFromBus = kcfg.autoW && !isnan(busWidth) && busWidth >= 1 && busWidth <= 60;
  kWidthNow = kWidthFromBus ? busWidth : kcfg.widthM;

  // Demoläget får aldrig hamna på den riktiga kartan
  if (demoOn) { if (kLogging) kartaStopLog(); kPrevState = -2; kOverride = -3; }
  // Trigger: start/stopp på kanter (när läget ändras)
  else
  if (kcfg.trig == KT_ISOBUS) {
    // Autoläge: loggar så länge redskapet arbetar (nivå, inte kant). Knappen gäller tills läget ändras.
    int st = kTrigState(now);
    if (st != kOverride) {
      kOverride = -3;
      if (st == 0 && !kLogging) kartaStartLog();
      else if (st != 0 && kLogging) kartaStopLog();
    }
  } else if (kcfg.trig != KT_MANUELL) {
    int st = kTrigState(now);
    if (st != kPrevState) {
      if (st >= 0 && kPrevState != -2) {
        if (!kLogging && st == kcfg.startEv) kartaStartLog();
        else if (kLogging && st == kcfg.stopEv) kartaStopLog();
      }
      kPrevState = st;
    }
  }

  // Ny GPS-position
  if (trd.posMs && trd.posMs != kLastPosMs) {
    kLastPosMs = trd.posMs;
    double lat = trd.lat, lon = trd.lon;
    // Framåtriktning ur rörelsen (vänd vid backning), annars GPS-kursen
    {
      double mx = kMperDegLon(lat), my = kMperDegLat(lat);
      if (kHasAnt) {
        float dx = (float)((lon - kAntLon) * mx), dy = (float)((lat - kAntLat) * my);
        float d = sqrtf(dx * dx + dy * dy);
        if (d >= 0.3f && d < 50) {
          float s = (trd.riktning == 0) ? -1.0f : 1.0f;            // backar: traktorn pekar åt andra hållet
          kFwdX = s * dx / d; kFwdY = s * dy / d; kHasFwd = true;
          kAntLat = lat; kAntLon = lon;
        } else if (d >= 50) { kAntLat = lat; kAntLon = lon; }
      } else { kAntLat = lat; kAntLon = lon; kHasAnt = true; }
      if (!kHasFwd && trd.kursMs) {
        float a = trd.kurs * 0.0174533f; kFwdX = sinf(a); kFwdY = cosf(a);
      }
      // redskapets position: K_BAK_M bakom antennen
      lat -= kFwdY * K_BAK_M / my;
      lon -= kFwdX * K_BAK_M / mx;
    }
    if (kLogging) {
      if (!kHasOrigin) {
        kSetOrigin(lat, lon);
        if (sdOk) {
          sdLock();
          File o = SD.open("/KARTA/SPAR.ORG", FILE_WRITE);
          if (o) { o.printf("%.8f %.8f\n", lat, lon); o.close(); }
          sdUnlock();
        }
      }
      // Färgen (v36): glidande medel över ca 3 s av bränsle (l/h), fart och dragkraft -> l/ha och kN.
      // v41: slirningen (redan utjämnad i traktor.h) bara vid samma fart som bränslet (>= 1,5 km/h).
      uint8_t fKod = 0, dKod = 0, sKod = 0;
      {
        float dt = kfMs ? (now - kfMs) / 1000.0f : 0; kfMs = now;
        float a = (dt > 0 && dt < 2) ? dt / 3.0f : 1.0f;
        auto ema = [&](float &m, float v) { if (isnan(v)) m = NAN; else m = isnan(m) ? v : m + (v - m) * a; };
        ema(kfFuel, trGet(trd.bransle, now));
        float kmh = trGet(trd.kmhMark, now); if (isnan(kmh)) kmh = trGet(trd.kmhHjul, now);
        ema(kfKmh, kmh);
        ema(kfKN, trGet(trd.dragKN, now));
        if (!isnan(kfFuel) && !isnan(kfKmh) && kfKmh >= 1.5f && kWidthNow >= 1)
          fKod = kfKod(0, kfFuel / (kfKmh * kWidthNow / 10.0f));
        dKod = kfKod(1, kfKN);
        kfSl = trGet(trd.slir, now, 1500);
        if (!isnan(kfSl) && !isnan(kfKmh) && kfKmh >= 1.5f) sKod = kfKod(2, kfSl);
      }
      float x, y; kToLocal(lat, lon, x, y);
      if (!kHasLast) { kRecord(1, x, y, kWidthNow); kLastX = x; kLastY = y; kHasLast = true; }
      else {
        float d = sqrtf((x - kLastX) * (x - kLastX) + (y - kLastY) * (y - kLastY));
        if (d > 80) { kRecord(1, x, y, kWidthNow); kLastX = x; kLastY = y; }        // hopp i GPS: nytt spår
        else if (d >= 0.5f) {
          kPaint(kLastX, kLastY, x, y, kWidthNow, fKod, dKod, sKod);
          kRecord(0, x, y, kWidthNow, fKod, dKod, sKod);
          kDistM += d; kLastX = x; kLastY = y;
        }
      }
    }
    static uint32_t lastC = 0;
    if (now - lastC > 30000) { lastC = now; kAddCentrum(lat, lon); }   // ny plats långt bort: hämta karta där
  }
  if (kBufN && now - kLastFlush > 10000) { kLastFlush = now; kFlush(); }

}

// Fyller kartans del av UiState
static void kartaFill(UiState &u, uint32_t now) {
  KartaView &k = u.karta;
  k.logging = kLogging;
  k.areaHa = kCells / 10000.0f;
  k.fuelL = kFuelL;
  k.lPerHa = k.areaHa >= 0.05f ? kFuelL / k.areaHa : NAN;
  k.fuelNow = (trd.bransle.ms && now - trd.bransle.ms < 5000) ? trd.bransle.v : NAN;
  k.distKm = kDistM / 1000.0f;
  k.hasPos = trd.posMs && now - trd.posMs < 5000;
  k.widthM = kWidthNow;
  k.widthFromBus = kWidthFromBus;
  k.cfgAutoW = kcfg.autoW;
  k.cfgWidthM = kcfg.widthM;
  k.trig = kcfg.trig;
  kartaTrigNamn(kcfg.trig, k.trigTxt, sizeof(k.trigTxt));
  {
    int t = kcfg.trig;
    if (t == KT_MANUELL) snprintf(k.trigKort, sizeof(k.trigKort), "%s", L("Knapp", "Button"));
    else if (t == KT_ISOBUS) snprintf(k.trigKort, sizeof(k.trigKort), "ISOBUS auto");
    else if (t <= KT_VENTIL7) snprintf(k.trigKort, sizeof(k.trigKort), L("Ventil %s", "Valve %s"), ventilNamn(t));
    else snprintf(k.trigKort, sizeof(k.trigKort), "%s %s", t <= KT_PTO_FRAM ? "PTO" : L("Lyft", "Hitch"),
                  (t == KT_PTO_BAK || t == KT_LYFT_BAK) ? L("bak", "rear") : L("fram", "front"));
  }
  snprintf(k.startTxt, sizeof(k.startTxt), "%s", kartaEvNamn(kcfg.trig, kcfg.startEv));
  snprintf(k.stopTxt, sizeof(k.stopTxt), "%s", kartaEvNamn(kcfg.trig, kcfg.stopEv));
  if (kPassTotal == 0) snprintf(k.tileTxt, sizeof(k.tileTxt), "%s", L("Satellitbild: hämtas när skärmen har WiFi och GPS", "Satellite image: downloaded when the display has WiFi and GPS"));
  else if (kDlDone && kTilesFail == 0) snprintf(k.tileTxt, sizeof(k.tileTxt), L("Satellitbild: alla %d rutor finns", "Satellite image: all %d tiles present"), kPassTotal);
  else snprintf(k.tileTxt, sizeof(k.tileTxt), L("Satellitbild: %d av %d rutor%s", "Satellite image: %d of %d tiles%s"), kTilesOk, kPassTotal,
                kTilesFail ? L(" (några misslyckades)", " (some failed)") : "");
  k.fargLage = kfLage;
  k.bakgrund = kBakgrund;
  k.fargHar = kfCnt[kfLage] > 0;
  k.fargSnitt = kfSnitt(kfLage);
  k.fargSpann = kfSpann(kfLage);
  k.lat = trd.lat; k.lon = trd.lon; k.gps = trd.posMs != 0;
  k.outside = kOutside > 0;
  k.panned = !kFollow;
  // skiftet (v45): kvar, återstående tid och klockslag
  k.skiftHa = kSkiftHa; k.skiftKvarHa = NAN; k.skiftMin = -1; k.skiftKlass[0] = 0; k.skiftKlart = false;
  if (kSkiftHa > 0) {
    float kvar = kSkiftHa - k.areaHa;
    if (kvar <= 0.005f) { k.skiftKvarHa = 0; k.skiftKlart = true; }
    else {
      k.skiftKvarHa = kvar;
      float takt = kSkiftTakt();
      if (!isnan(takt)) {
        k.skiftTakt = takt;
        k.skiftMin = (int)lroundf(kvar / takt * 60.0f);
        time_t t = time(nullptr);
        if (t > 1700000000) {
          t += (time_t)(k.skiftMin * 60L);
          struct tm lt; localtime_r(&t, &lt);
          snprintf(k.skiftKlass, sizeof(k.skiftKlass), "%02d:%02d", lt.tm_hour, lt.tm_min);
        }
      }
    }
  }
}

// Kartans bakgrund (v37): 0 = satellitbild, 1 = enfärgad. Sparas.
static void kartaBakgrund(int b) {
  kBakgrund = b ? 1 : 0;
  kPrefs.putUChar("bg", kBakgrund);
}

// Vad spåret färgas efter: 0 = bränsle (l/ha), 1 = dragkraft (kN), 2 = slirning (%, v41). Sparas.
static void kartaFarg(int lage) {
  kfLage = (lage >= 0 && lage <= 2) ? lage : 0;
  kPrefs.putUChar("farg", kfLage);
}

// START/STOPP-knappen på kartan (i autoläget gäller den tills redskapets läge ändras)
static void kartaKnapp(uint32_t now) {
  if (demoOn) return;                       // ingen loggning i demoläget
  if (kLogging) kartaStopLog(); else kartaStartLog();
  if (kcfg.trig == KT_ISOBUS) kOverride = kTrigState(now);
}
