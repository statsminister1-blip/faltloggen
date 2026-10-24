// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  falt.h – fältrapport som PDF till telefonen
//
//  1. Fältloggen: en post per sekund (position, bränsle, fart, om kartan loggar) i /KARTA/FALT.BIN,
//     så att tid på fältet, bränsle med stopp och körsträcka kan räknas. Töms med kartans NOLLA.
//  2. Fältet: kartans 1 m-rutnät delas i rutor om 25 m. Rutor med spår som ligger högst ca 100 m isär hör
//     ihop (grupp). Den största gruppen (flest täckta m²) är fältet; spår långt bort räknas inte.
//     "Inne på fältet" = fältets rutor + 25 m marginal (vändtegen).
//  3. Statistiken räknas ur fältloggen för poster inne på fältet. Arbete = kartan loggar, och uppehåll
//     upp till 30 s mellan två arbetssekunder (vändningar) räknas som arbete (samma regel som analysen).
//  4. PDF:en (A4) får tre kartor (färgade efter bränsle l/ha, slirning % och dragkraft kN; slirningen från v41)
//     och statistiken. Bilderna
//     komprimeras med ESP32-ROM:ens deflate (miniz) och skrivs till /RAPPORT/<datum>_<namn>.pdf.
//  5. Skärmen startar ett eget WiFi (skärmens namn, inget lösenord) med en liten webbsida där PDF:en laddas
//     ner (http://192.168.4.1). Sköts av nät-tasken (net.h). Rapporten stannar på skärmen och telefonen.
// ============================================================================
#pragma once
#include <time.h>
#include <sys/time.h>
#ifdef ESP_PLATFORM
#include "miniz.h"
#else
#include <zlib.h>
#endif

#ifdef ESP_PLATFORM
#include "esp_attr.h"
RTC_NOINIT_ATTR static uint32_t rappSteg;     // v37.1: hur långt rapporten kom (överlever en krasch, syns i enhetsinfon)
#else
static uint32_t rappSteg;
#endif
#define RAPP_MAGI   0x52A90000u
static inline void rappPaus() {
#ifdef ESP_PLATFORM
  vTaskDelay(1);
#else
  yield();
#endif
}
static inline void rappSpar(int n) { rappSteg = n ? (RAPP_MAGI | n) : 0; }

#define FALT_PATH   "/KARTA/FALT.BIN"
#define FALT_CELL   25                       // m per ruta när fältet bestäms
#define FALT_NC     ((KG_N + FALT_CELL - 1) / FALT_CELL)
#define FALT_HOP    2                        // rutor: glapp upp till ca 2 × 25 m åt varje håll = samma fält
#define FALT_VAND_S 30                       // uppehåll upp till 30 s = vändning (arbete)
#define RAPP_W      520                      // kartbildernas storlek i PDF:en (bildpunkter)
#define RAPP_H      390

// ---------------------------------------------------------------- 1. Fältloggen
struct FRec { int32_t xcm, ycm; uint16_t fuel10, kmh10; uint8_t flags, r0, r1, r2; };   // 16 byte
enum { FR_ARBETE = 1, FR_NY = 2 };           // FR_NY = första posten efter start (bryter vändningsregeln)
static FRec fBuf[32];
static int fBufN = 0;
static bool fForsta = true;

static void faltFlush() {
  if (!fBufN) return;
  if (sdOk) {
    sdLock();
    File f = SD.open(FALT_PATH, FILE_APPEND);
    if (f) { f.write((const uint8_t *)fBuf, fBufN * sizeof(FRec)); f.close(); }
    sdUnlock();
  }
  fBufN = 0;
}
// Körs en gång per sekund från huvudprogrammet
static void faltSample(uint32_t now, float kmh, float fuel, bool arbete) {
  if (demoOn || !kHasOrigin || !trd.posMs || now - trd.posMs > 3000) return;
  float x, y; kToLocal(trd.lat, trd.lon, x, y);
  FRec &r = fBuf[fBufN++];
  memset(&r, 0, sizeof(r));
  r.xcm = (int32_t)lroundf(x * 100); r.ycm = (int32_t)lroundf(y * 100);
  r.fuel10 = (isnan(fuel) || fuel < 0) ? 0 : (uint16_t)lroundf(fminf(fuel, 6000) * 10);
  r.kmh10 = (isnan(kmh) || kmh < 0) ? 0 : (uint16_t)lroundf(fminf(kmh, 6000) * 10);
  r.flags = (arbete ? FR_ARBETE : 0) | (fForsta ? FR_NY : 0);
  fForsta = false;
  if (fBufN == 32) faltFlush();
}
static void faltNolla() {
  fBufN = 0; fForsta = true;
  if (sdOk) { sdLock(); SD.remove(FALT_PATH); sdUnlock(); }
}

// ---------------------------------------------------------------- Tid (klockan)
// Klockan ställs av NTP när skärmen har internet (net.h) eller av GPS-tiden (traktor.h, 0x1F805).
static bool faltTidOk() { return time(nullptr) > 1700000000; }
static void faltGpsTid(uint16_t dag, uint32_t sek10k) {
  if (faltTidOk() || dag < 19000 || dag > 25000 || sek10k >= 864000000UL) return;
  struct timeval tv = {(time_t)dag * 86400 + (time_t)(sek10k / 10000), 0};
  settimeofday(&tv, nullptr);
}

// ---------------------------------------------------------------- 2–3. Fältet och statistiken
struct FaltRes {
  bool ok; char fel[80];
  float ha, haUtanfor;                    // bearbetad areal på fältet / loggat utanför fältet
  double lTot, lArb;                      // bränsle på fältet totalt / i arbete (inkl. vändningar)
  int sTot, sArb, sStill;                 // sekunder på fältet / i arbete / stillastående (utanför arbete)
  double km, kmhArbSum; int kmhArbN;
  float x0, y0, x1, y1;                   // fältets gränser (lokala meter)
};
static uint16_t *fOcc = nullptr;           // täckta m² per 25 m-ruta
static int16_t  *fLab = nullptr;           // grupp per ruta (-1 = ingen)
static uint8_t  *fIn = nullptr;            // 1 = inne på fältet (med marginal)
static int32_t  *fStack = nullptr;

static bool faltMinne() {
  const size_t n = (size_t)FALT_NC * FALT_NC;
  if (!fOcc) fOcc = (uint16_t *)heap_caps_malloc(n * 2, MALLOC_CAP_SPIRAM);
  if (!fLab) fLab = (int16_t *)heap_caps_malloc(n * 2, MALLOC_CAP_SPIRAM);
  if (!fIn) fIn = (uint8_t *)heap_caps_malloc(n, MALLOC_CAP_SPIRAM);
  if (!fStack) fStack = (int32_t *)heap_caps_malloc(n * 4, MALLOC_CAP_SPIRAM);
  return fOcc && fLab && fIn && fStack;
}
static inline int faltCell(float x, float y) {
  int gx = (int)floorf(x) + KG_HALF, gy = (int)floorf(y) + KG_HALF;
  if ((unsigned)gx >= KG_N || (unsigned)gy >= KG_N) return -1;
  return (gy / FALT_CELL) * FALT_NC + gx / FALT_CELL;
}

static void faltBerakna(FaltRes &r) {
  memset(&r, 0, sizeof(r));
  const int NC = FALT_NC;
  if (!kGrid || !kHasOrigin || kCells == 0) { snprintf(r.fel, sizeof(r.fel), L("Inget är loggat på kartan sedan NOLLA.", "Nothing is logged on the map since RESET.")); return; }
  if (!faltMinne()) { snprintf(r.fel, sizeof(r.fel), L("För lite minne.", "Not enough memory.")); return; }
  memset(fOcc, 0, (size_t)NC * NC * 2);
  // täckta m² per ruta
  for (int gy = 0; gy < KG_N; gy++) {
    const uint32_t *row = kGrid + ((uint32_t)gy * KG_N >> 5);
    int cy = gy / FALT_CELL;
    for (int w = 0; w < KG_N / 32; w++) {
      uint32_t v = row[w];
      while (v) {
        int b = __builtin_ctz(v); v &= v - 1;
        int gx = w * 32 + b;
        fOcc[cy * NC + gx / FALT_CELL]++;
      }
    }
    if ((gy & 255) == 0) rappPaus();
  }
  // grupper: rutor med spår som ligger högst FALT_HOP rutor isär hör ihop
  for (int i = 0; i < NC * NC; i++) fLab[i] = -1;
  int nLab = 0, best = -1; uint32_t bestSum = 0, allSum = 0;
  for (int i = 0; i < NC * NC; i++) {
    if (!fOcc[i] || fLab[i] >= 0) continue;
    if (nLab >= 32000) break;
    int lab = nLab++; uint32_t sum = 0; int sp = 0;
    fStack[sp++] = i; fLab[i] = lab;
    while (sp) {
      int c = fStack[--sp]; sum += fOcc[c];
      int cx = c % NC, cy = c / NC;
      for (int dy = -FALT_HOP; dy <= FALT_HOP; dy++)
        for (int dx = -FALT_HOP; dx <= FALT_HOP; dx++) {
          int nx = cx + dx, ny = cy + dy;
          if ((unsigned)nx >= (unsigned)NC || (unsigned)ny >= (unsigned)NC) continue;
          int n = ny * NC + nx;
          if (fOcc[n] && fLab[n] < 0) { fLab[n] = lab; fStack[sp++] = n; }
        }
    }
    allSum += sum;
    if (sum > bestSum) { bestSum = sum; best = lab; }
  }
  if (best < 0) { snprintf(r.fel, sizeof(r.fel), L("Inget fält hittat.", "No field found.")); return; }
  r.ha = bestSum / 10000.0f;
  r.haUtanfor = (allSum - bestSum) / 10000.0f;
  // inne på fältet = fältets rutor + 1 ruta marginal; gränserna
  memset(fIn, 0, (size_t)NC * NC);
  int bx0 = NC, by0 = NC, bx1 = -1, by1 = -1;
  for (int i = 0; i < NC * NC; i++) {
    if (fLab[i] != best || !fOcc[i]) continue;
    int cx = i % NC, cy = i / NC;
    bx0 = min(bx0, cx); bx1 = max(bx1, cx); by0 = min(by0, cy); by1 = max(by1, cy);
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++) {
        int nx = cx + dx, ny = cy + dy;
        if ((unsigned)nx < (unsigned)NC && (unsigned)ny < (unsigned)NC) fIn[ny * NC + nx] = 1;
      }
  }
  r.x0 = bx0 * FALT_CELL - KG_HALF; r.x1 = (bx1 + 1) * FALT_CELL - KG_HALF;
  r.y0 = by0 * FALT_CELL - KG_HALF; r.y1 = (by1 + 1) * FALT_CELL - KG_HALF;
  // statistiken ur fältloggen
  faltFlush();
  struct Gap { int s, still; double l, m; } gap = {0, 0, 0, 0};
  bool harArb = false;
  auto slapp = [&]() {                         // uppehållet var för långt (eller avbrutet): inte arbete
    r.sStill += gap.still; gap = {0, 0, 0, 0};
  };
  if (sdOk) {
    sdLock(); File f = SD.open(FALT_PATH); sdUnlock();
    if (f) {
      static FRec b[64];
      for (;;) {
        sdLock(); int n = f.read((uint8_t *)b, sizeof(b)) / sizeof(FRec); sdUnlock();
        for (int i = 0; i < n; i++) {
          const FRec &p = b[i];
          if (p.flags & FR_NY) { slapp(); harArb = false; }
          int c = faltCell(p.xcm / 100.0f, p.ycm / 100.0f);
          if (c < 0 || !fIn[c]) { slapp(); harArb = false; continue; }   // utanför fältet bryter också
          float kmh = p.kmh10 / 10.0f, l = p.fuel10 / 10.0f / 3600.0f, m = kmh / 3.6f;
          r.sTot++; r.lTot += l; r.km += m / 1000.0;
          if (p.flags & FR_ARBETE) {
            if (harArb && gap.s && gap.s <= FALT_VAND_S) { r.sArb += gap.s; r.lArb += gap.l; gap = {0, 0, 0, 0}; }
            else slapp();
            harArb = true;
            r.sArb++; r.lArb += l;
            if (kmh >= 1.0f) { r.kmhArbSum += kmh; r.kmhArbN++; }
          } else {
            gap.s++; gap.l += l; gap.m += m;
            if (kmh < 0.5f) gap.still++;
            if (gap.s > FALT_VAND_S) { slapp(); harArb = false; }
          }
        }
        if (n < 64) break;
        rappPaus();
      }
      sdLock(); f.close(); sdUnlock();
      slapp();
    }
  }
  r.ok = true;
}

// ---------------------------------------------------------------- 4. Kartbilderna
// Ritar i en bildbuffert (RGB565) i stället för på skärmen
class FaltGfx : public Arduino_GFX {
 public:
  uint16_t *fb;
  FaltGfx(uint16_t *b, int w, int h) : Arduino_GFX(w, h), fb(b) {}
  bool begin(int32_t) override { return true; }
  void writePixelPreclipped(int16_t x, int16_t y, uint16_t c) override { fb[y * _width + x] = c; }
};

// Fältets karta i valt färgläge (0 = bränsle, 1 = dragkraft, 2 = slirning), med färgstege och skalstock
static void faltKarta(uint16_t *fb, const FaltRes &r, int lage) {
  FaltGfx g(fb, RAPP_W, RAPP_H);
  g.setUTF8Print(true);
  g.setTextWrap(false);
  const int stege = 44;                          // färgstegen nederst
  float bw = r.x1 - r.x0 + 40, bh = r.y1 - r.y0 + 40;
  float mpp = fmaxf(bw / RAPP_W, bh / (RAPP_H - stege));
  if (mpp < 0.25f) mpp = 0.25f;
  float cx = (r.x0 + r.x1) / 2, cy = (r.y0 + r.y1) / 2;
  uint8_t gamlaLage = kfLage;
  kfLage = lage;
  kOvLat = kLat0 + cy / kMy; kOvLon = kLon0 + cx / kMx;
  kOvSet = true;
  // kartan centreras i rutan ovanför stegen
  kartaRender(&g, 0, 0, RAPP_W, RAPP_H - stege, mpp, false);
  kOvSet = false;
  kfLage = gamlaLage;
  // färgstegen
  g.fillRect(0, RAPP_H - stege, RAPP_W, stege, RGB(255, 255, 255));
  int sx = 12, sw = RAPP_W - 24, sy = RAPP_H - stege + 6;
  for (int i = 0; i < sw; i++) g.drawFastVLine(sx + i, sy, 12, uiSkalaFarg(i * 2.0f / (sw - 1) - 1.0f));
  char b[40], v[16];
  float a = kfSnitt(lage), sp = kfSpann(lage);
  const char *enh = lage == 2 ? "%" : (lage ? "kN" : "l/ha");
  int dec = lage ? 0 : 1;
  if (!isnan(a)) {
    uiFmt(v, sizeof(v), a - sp, dec); uiText(&g, sx, sy + 32, v, RGB(20, 20, 20), u8g2_font_dv13_tf);
    uiFmt(v, sizeof(v), a, dec); snprintf(b, sizeof(b), L("snitt %s %s", "average %s %s"), v, enh);
    uiTextCenter(&g, sx + sw / 2, sy + 32, b, RGB(20, 20, 20), u8g2_font_dv13b_tf);
    uiFmt(v, sizeof(v), a + sp, dec); uiTextRight(&g, sx + sw, sy + 32, v, RGB(20, 20, 20), u8g2_font_dv13_tf);
  } else uiTextCenter(&g, RAPP_W / 2, sy + 32, L("inga värden", "no values"), RGB(90, 90, 90), u8g2_font_dv13_tf);
  // skalstock uppe till vänster
  int sm = 10; while (sm / mpp < 60) sm = (sm == 10 || sm == 100 || sm == 1000) ? sm * 5 / 2 : sm * 2;
  int sl = (int)(sm / mpp);
  g.fillRect(8, 8, sl + 60, 22, RGB(255, 255, 255));
  g.fillRect(14, 20, sl, 3, RGB(0, 0, 0));
  g.fillRect(14, 15, 2, 10, RGB(0, 0, 0)); g.fillRect(14 + sl - 2, 15, 2, 10, RGB(0, 0, 0));
  snprintf(b, sizeof(b), sm >= 1000 ? "%d km" : "%d m", sm >= 1000 ? sm / 1000 : sm);
  uiText(&g, 20 + sl, 25, b, RGB(0, 0, 0), u8g2_font_dv11_tf);
}

// ---------------------------------------------------------------- PDF
// Text i PDF:en: Helvetica med WinAnsi-kodning (å ä ö Å Ä Ö · finns där)
static void pdfText(char *out, size_t n, const char *in) {
  size_t o = 0;
  for (const uint8_t *p = (const uint8_t *)in; *p && o + 2 < n; p++) {
    uint8_t c = *p;
    if (c == 0xC3 && p[1]) { out[o++] = (char)(p[1] + 0x40); p++; continue; }          // Latin-1-tecken
    if (c == 0xC2 && p[1]) { out[o++] = (char)p[1]; p++; continue; }
    if (c == 0xE2 && p[1] == 0x80 && p[2] == 0x93) { out[o++] = (char)0x96; p += 2; continue; }   // –
    if (c == '(' || c == ')' || c == '\\') out[o++] = '\\';
    if (c < 0x80) out[o++] = (char)c;
  }
  out[o] = 0;
}
// Komprimering i bitar (zlib-format): zStart, zAdd (sista = true på sista biten), zLen
struct ZUt { uint8_t *p; size_t n, cap; bool ok; };
#ifdef ESP_PLATFORM
static tdefl_compressor *zD = nullptr;
static ZUt zU;
static mz_bool zPut(const void *buf, int len, void *u) {
  ZUt *o = (ZUt *)u;
  if (o->n + len > o->cap) { o->ok = false; return MZ_FALSE; }
  memcpy(o->p + o->n, buf, len); o->n += len; return MZ_TRUE;
}
static bool zStart(uint8_t *out, size_t cap) {
  if (!zD) zD = (tdefl_compressor *)heap_caps_malloc(sizeof(tdefl_compressor), MALLOC_CAP_SPIRAM);
  if (!zD) return false;
  zU = {out, 0, cap, true};
  return tdefl_init(zD, zPut, &zU, TDEFL_WRITE_ZLIB_HEADER | 64) == TDEFL_STATUS_OKAY;
}
static bool zAdd(const uint8_t *in, size_t n, bool sista) {
  tdefl_status st = tdefl_compress_buffer(zD, in, n, sista ? TDEFL_FINISH : TDEFL_NO_FLUSH);
  return st >= 0 && zU.ok;
}
static size_t zLen() { return zU.ok ? zU.n : 0; }
#else
static z_stream zS; static bool zOk;
static bool zStart(uint8_t *out, size_t cap) {
  memset(&zS, 0, sizeof(zS)); zS.next_out = out; zS.avail_out = cap;
  return zOk = deflateInit(&zS, 6) == Z_OK;
}
static bool zAdd(const uint8_t *in, size_t n, bool sista) {
  zS.next_in = (Bytef *)in; zS.avail_in = n;
  int r = deflate(&zS, sista ? Z_FINISH : Z_NO_FLUSH);
  if (r == Z_STREAM_ERROR || (zS.avail_in && !zS.avail_out)) zOk = false;
  if (sista) { if (r != Z_STREAM_END) zOk = false; deflateEnd(&zS); }
  return zOk;
}
static size_t zLen() { return zOk ? zS.total_out : 0; }
#endif

struct PdfW { File f; size_t pos; size_t off[12]; bool ok; };
static void pdfPut(PdfW &w, const void *d, size_t n) {
  if (!w.ok) return;
  sdLock(); size_t k = w.f.write((const uint8_t *)d, n); sdUnlock();
  if (k != n) w.ok = false;
  w.pos += n;
}
static void pdfPrintf(PdfW &w, const char *fmt, ...) {
  char b[512];
  va_list ap; va_start(ap, fmt); int n = vsnprintf(b, sizeof(b), fmt, ap); va_end(ap);
  if (n > 0) pdfPut(w, b, min(n, (int)sizeof(b) - 1));
}

static void faltFmtTid(char *b, size_t n, int sek) {
  int h = sek / 3600, m = (sek % 3600 + 30) / 60;
  if (m == 60) { h++; m = 0; }
  if (h) snprintf(b, n, "%d h %d min", h, m); else snprintf(b, n, "%d min", m);
}

// Skapar rapporten. Fyller ExpView (sammanfattning, filnamn) och returnerar true om PDF:en skrevs.
static bool faltRapport(ExpView &e, const char *monitor, float diesel, float timKr, int lage, float breddM) {
  e.nRad = 0; e.fel[0] = 0;
  if (!sdOk) { snprintf(e.fel, sizeof(e.fel), L("SD-kortet saknas.", "No SD card.")); return false; }
  rappSpar(1);
  static FaltRes r; faltBerakna(r);
  rappSpar(2);
  if (!r.ok) { strncpy(e.fel, r.fel, sizeof(e.fel) - 1); return false; }
  // siffrorna
  float hArb = r.sArb / 3600.0f;
  float lha = r.ha > 0 ? r.lArb / r.ha : NAN;
  float lh = hArb > 0 ? r.lArb / hArb : NAN;
  float haH = hArb > 0 ? r.ha / hArb : NAN;
  float tv = AN_TIDSVIKT[(lage >= 0 && lage <= 2) ? lage : 1];
  float krha = r.ha > 0 ? (r.lArb * diesel + hArb * timKr * tv) / r.ha : NAN;
  float kmhArb = r.kmhArbN ? r.kmhArbSum / r.kmhArbN : NAN;
  char t1[24], t2[24], v1[16], v2[16], v3[16];
  // tid och filnamn
  char datum[24], fil[64], namn[40];
  snprintf(datum, sizeof(datum), "%s", L("utan datum", "no date"));
  time_t tnu = time(nullptr);
  struct tm lt;
  bool tidOk = faltTidOk();
  if (tidOk) { localtime_r(&tnu, &lt); strftime(datum, sizeof(datum), "%Y-%m-%d %H:%M", &lt); }
  {
    char ren[32]; int k = 0;                       // filnamnet: bara a-z, 0-9, - och _
    for (const uint8_t *p = (const uint8_t *)e.namn; *p && k < 24; p++) {
      uint8_t c = *p;
      if (c == 0xC3 && p[1]) { uint8_t d = p[1]; p++; c = (d == 0xA5 || d == 0xA4 || d == 0x85 || d == 0x84) ? 'a' : (d == 0xB6 || d == 0x96) ? 'o' : 'x'; }
      if (c >= 'A' && c <= 'Z') c += 32;
      if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') ren[k++] = c;
      else if ((c == ' ' || c == '_') && k && ren[k - 1] != '_') ren[k++] = '_';
    }
    ren[k] = 0;
    char d[20];
    if (tidOk) strftime(d, sizeof(d), "%Y-%m-%d_%H%M", &lt); else snprintf(d, sizeof(d), "start%lu", (unsigned long)bootNo);
    char mon[24]; int j = 0;
    for (const char *p = monitor; *p && j < 20; p++) mon[j++] = (*p == ' ') ? '-' : *p;
    mon[j] = 0;
    snprintf(fil, sizeof(fil), "%s_%s.pdf", d, ren[0] ? ren : (mon[0] ? mon : "falt"));
    if (e.namn[0]) snprintf(namn, sizeof(namn), "%s", e.namn);
    else snprintf(namn, sizeof(namn), "%s %s", tidOk ? datum : "", monitor);
  }
  snprintf(e.fil, sizeof(e.fil), "%s", fil);
  // sammanfattningen på skärmen
  uiFmt(v1, sizeof(v1), r.ha, 2);
  snprintf(e.rad[e.nRad++], 72, L("%s ha bearbetat", "%s ha worked"), v1);
  uiFmt(v1, sizeof(v1), r.lTot, 1); uiFmt(v2, sizeof(v2), r.lArb, 1);
  snprintf(e.rad[e.nRad++], 72, L("Bränsle %s l, varav %s l i arbete", "Fuel %s l, of which %s l working"), v1, v2);
  uiFmt(v1, sizeof(v1), lha, 1); uiFmt(v2, sizeof(v2), lh, 1);
  snprintf(e.rad[e.nRad++], 72, L("%s l/ha  ·  %s l/h i arbete", "%s l/ha  ·  %s l/h working"), v1, v2);
  faltFmtTid(t1, sizeof(t1), r.sTot); faltFmtTid(t2, sizeof(t2), r.sArb);
  snprintf(e.rad[e.nRad++], 72, L("Tid på fältet %s, varav %s i arbete", "Time in field %s, of which %s working"), t1, t2);
  uiFmt(v1, sizeof(v1), r.km, 1); uiFmt(v2, sizeof(v2), kmhArb, 1);
  snprintf(e.rad[e.nRad++], 72, L("Körsträcka %s km  ·  snitthastighet %s km/h", "Distance %s km  ·  average speed %s km/h"), v1, v2);
  uiFmt(v1, sizeof(v1), haH, 2); uiFmt(v2, sizeof(v2), krha, 0);
  snprintf(e.rad[e.nRad++], 72, "%s ha/h  ·  %s %s/ha", v1, v2, valuta);
  if (r.haUtanfor >= 0.01f) { uiFmt(v1, sizeof(v1), r.haUtanfor, 2); snprintf(e.rad[e.nRad++], 72, L("%s ha loggat utanför fältet räknas inte", "%s ha logged outside the field is not counted"), v1); }

  // PDF:en – kartbilderna ritas, komprimeras och skrivs en i taget (mindre minne)
  rappSpar(3);
  const size_t npx = (size_t)RAPP_W * RAPP_H;
  const int ZR = 16;                                         // rader per bit till komprimeringen
  uint16_t *fb = (uint16_t *)heap_caps_malloc(npx * 2, MALLOC_CAP_SPIRAM);
  uint8_t *rgb = (uint8_t *)heap_caps_malloc(RAPP_W * ZR * 3, MALLOC_CAP_SPIRAM);
  const size_t zcap = npx * 3 + 4096;
  uint8_t *z = (uint8_t *)heap_caps_malloc(zcap, MALLOC_CAP_SPIRAM);
  if (!fb || !rgb || !z) {
    snprintf(e.fel, sizeof(e.fel), L("För lite minne för kartbilderna.", "Not enough memory for the map images."));
    free(fb); free(rgb); free(z); return false;
  }
  char path[96]; snprintf(path, sizeof(path), "/RAPPORT/%s", fil);
  PdfW w; memset(&w.off, 0, sizeof(w.off)); w.pos = 0; w.ok = true;
  sdLock();
  if (!SD.exists("/RAPPORT")) SD.mkdir("/RAPPORT");
  w.f = SD.open(path, FILE_WRITE);
  sdUnlock();
  if (!w.f) { snprintf(e.fel, sizeof(e.fel), L("Kunde inte skriva på SD-kortet.", "Could not write to the SD card.")); free(fb); free(rgb); free(z); return false; }
  pdfPrintf(w, "%%PDF-1.4\n%%\xE5\xE4\xF6\n");
  w.off[1] = w.pos; pdfPrintf(w, "1 0 obj\n<< /Type /Catalog /Pages 2 0 R >>\nendobj\n");
  w.off[2] = w.pos; pdfPrintf(w, "2 0 obj\n<< /Type /Pages /Kids [3 0 R] /Count 1 >>\nendobj\n");
  w.off[3] = w.pos;
  pdfPrintf(w, "3 0 obj\n<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] /Contents 8 0 R "
               "/Resources << /Font << /F1 4 0 R /F2 5 0 R >> /XObject << /Im1 6 0 R /Im2 7 0 R /Im3 9 0 R >> >> >>\nendobj\n");
  w.off[4] = w.pos; pdfPrintf(w, "4 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>\nendobj\n");
  w.off[5] = w.pos; pdfPrintf(w, "5 0 obj\n<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>\nendobj\n");
  bool bildOk = true;
  // v41: tre kartor – bränsle (Im1), slirning (Im2) och dragkraft (Im3). Objekten 6, 7 och 9 (8 = sidans text).
  static const int LAGE[3] = {0, 2, 1}, OBJ[3] = {6, 7, 9};
  for (int k = 0; k < 3 && bildOk; k++) {
    rappSpar(4 + k * 2);                                     // 4/6/8 = ritar karta 1/2/3
    faltKarta(fb, r, LAGE[k]);
    rappSpar(5 + k * 2);                                     // 5/7/9 = komprimerar karta 1/2/3
    bildOk = zStart(z, zcap);
    for (int y0 = 0; y0 < RAPP_H && bildOk; y0 += ZR) {
      int nr = min(ZR, RAPP_H - y0);
      for (int i = 0; i < nr * RAPP_W; i++) {
        uint16_t c = fb[y0 * RAPP_W + i];
        rgb[i * 3] = ((c >> 11) & 31) * 255 / 31; rgb[i * 3 + 1] = ((c >> 5) & 63) * 255 / 63; rgb[i * 3 + 2] = (c & 31) * 255 / 31;
      }
      bildOk = zAdd(rgb, (size_t)nr * RAPP_W * 3, y0 + nr >= RAPP_H);
      rappPaus();
    }
    size_t zn = bildOk ? zLen() : 0;
    if (!zn) { bildOk = false; break; }
    w.off[OBJ[k]] = w.pos;
    pdfPrintf(w, "%d 0 obj\n<< /Type /XObject /Subtype /Image /Width %d /Height %d /ColorSpace /DeviceRGB "
                 "/BitsPerComponent 8 /Filter /FlateDecode /Length %u >>\nstream\n", OBJ[k], RAPP_W, RAPP_H, (unsigned)zn);
    for (size_t o = 0; o < zn; o += 4096) pdfPut(w, z + o, min((size_t)4096, zn - o));
    pdfPrintf(w, "\nendstream\nendobj\n");
  }
  free(fb); free(rgb); free(z);
  if (!bildOk) {
    sdLock(); w.f.close(); SD.remove(path); sdUnlock();
    snprintf(e.fel, sizeof(e.fel), L("Kartbilderna kunde inte komprimeras.", "The map images could not be compressed."));
    return false;
  }
  rappSpar(10);
  // sidans innehåll
  static char cs[6000];
  size_t cn = 0;
  char tx[160];
  auto add = [&](const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    int n = vsnprintf(cs + cn, sizeof(cs) - cn, fmt, ap);
    va_end(ap);
    if (n > 0) cn = min(cn + n, sizeof(cs) - 1);
  };
  auto text = [&](int font, float size, float x, float y, const char *s) {
    pdfText(tx, sizeof(tx), s);
    add("BT /F%d %.1f Tf %.1f %.1f Td (%s) Tj ET\n", font, size, x, y, tx);
  };
  char b[160];
  snprintf(b, sizeof(b), L("Fältrapport - %s", "Field report - %s"), namn);
  text(2, 20, 40, 790, b);
  snprintf(b, sizeof(b), "%s  ·  %s", monitor[0] ? monitor : "Fältloggen", datum);
  text(1, 11, 40, 770, b);
  add("0.75 0.85 0.95 rg 40 752 515 2 re f 0 g\n");
  // v41: tre kartor i rad, 165 × 123,75 pt var (samma bilder som förut, 520 × 390 bildpunkter)
  text(2, 10, 40, 730, L("Bränsle (l/ha)", "Fuel (l/ha)"));
  text(2, 10, 215, 730, L("Slirning (%)", "Wheel slip (%)"));
  text(2, 10, 390, 730, L("Dragkraft i lyften (kN)", "Draft force in hitch (kN)"));
  add("q 165 0 0 123.75 40 600 cm /Im1 Do Q\n");
  add("q 165 0 0 123.75 215 600 cm /Im2 Do Q\n");
  add("q 165 0 0 123.75 390 600 cm /Im3 Do Q\n");
  add("0.5 G 0.5 w 40 600 165 123.75 re S 215 600 165 123.75 re S 390 600 165 123.75 re S 0 G\n");
  float y = 570;
  auto rad = [&](const char *lab, const char *val) {
    text(1, 11, 40, y, lab); text(2, 11, 230, y, val); y -= 19;
    add("0.85 G 0.5 w 40 %.1f m 555 %.1f l S 0 G\n", y + 13, y + 13);
  };
  uiFmt(v1, sizeof(v1), r.ha, 2); snprintf(b, sizeof(b), "%s ha", v1); rad(L("Areal bearbetad", "Area worked"), b);
  uiFmt(v1, sizeof(v1), r.lTot, 1); uiFmt(v2, sizeof(v2), r.lArb, 1);
  snprintf(b, sizeof(b), L("%s l, varav %s l i arbete", "%s l, of which %s l working"), v1, v2); rad(L("Bränsle", "Fuel"), b);
  uiFmt(v1, sizeof(v1), lha, 1); snprintf(b, sizeof(b), "%s l/ha", v1); rad(L("Bränsle per hektar", "Fuel per hectare"), b);
  uiFmt(v1, sizeof(v1), lh, 1); snprintf(b, sizeof(b), "%s l/h", v1); rad(L("Bränsle per timme i arbete", "Fuel per hour working"), b);
  faltFmtTid(t1, sizeof(t1), r.sTot); faltFmtTid(t2, sizeof(t2), r.sArb);
  snprintf(b, sizeof(b), L("%s, varav %s i arbete", "%s, of which %s working"), t1, t2); rad(L("Tid på fältet", "Time in field"), b);
  faltFmtTid(t1, sizeof(t1), r.sStill); rad(L("Stillastående", "Standing still"), t1);
  uiFmt(v1, sizeof(v1), r.km, 1); snprintf(b, sizeof(b), "%s km", v1); rad(L("Körsträcka", "Distance"), b);
  uiFmt(v1, sizeof(v1), kmhArb, 1); snprintf(b, sizeof(b), "%s km/h", v1); rad(L("Snitthastighet i arbete", "Average speed working"), b);
  uiFmt(v1, sizeof(v1), haH, 2); snprintf(b, sizeof(b), "%s ha/h", v1); rad(L("Kapacitet", "Capacity"), b);
  uiFmt(v1, sizeof(v1), breddM, 2); snprintf(b, sizeof(b), "%s m", v1); rad(L("Arbetsbredd", "Working width"), b);
  uiFmt(v1, sizeof(v1), krha, 0); uiFmt(v2, sizeof(v2), diesel, 1); uiFmt(v3, sizeof(v3), timKr, 0);
  snprintf(b, sizeof(b), L("%s %s/ha (diesel %s %s/l, traktor + förare %s %s/h, %s)", "%s %s/ha (diesel %s %s/l, tractor + driver %s %s/h, %s)"), v1, valuta, v2, valuta, v3, valuta, anLage(lage));
  rad(L("Kostnad", "Cost"), b);
  y -= 10;
  text(1, 8.5f, 40, y, L("Arbete = när kartan loggar; uppehåll upp till 30 s (vändningar) räknas som arbete. Fältet = det största sammanhängande",
                         "Working = when the map is logging; pauses up to 30 s (headland turns) count as working. The field = the largest connected")); y -= 11;
  text(1, 8.5f, 40, y, L("området med spår (glapp upp till ca 100 m hör ihop) + 25 m marginal för vändtegen; spår längre bort räknas inte.",
                         "area with tracks (gaps up to about 100 m belong together) + 25 m margin for the headland; tracks further away do not count.")); y -= 11;
  if (r.haUtanfor >= 0.01f) {
    uiFmt(v1, sizeof(v1), r.haUtanfor, 2);
    snprintf(b, sizeof(b), L("Loggat utanför fältet (räknas inte): %s ha.", "Logged outside the field (not counted): %s ha."), v1); text(1, 8.5f, 40, y, b); y -= 11;
  }
  snprintf(b, sizeof(b), L("Kartfärg: körningens snitt = blått, -30 %% grönt, +30 %% rött. %s.", "Map colour: the run average = blue, -30 %% green, +30 %% red. %s."), kBildText());
  text(1, 8.5f, 40, y, b);
  snprintf(b, sizeof(b), L("Fältloggen v%d  ·  uppskattade värden, utan garanti", "Fältloggen v%d  ·  estimated values, no warranty"), FW_VERSION);
  text(1, 8, 40, 30, b);
  w.off[8] = w.pos;
  pdfPrintf(w, "8 0 obj\n<< /Length %u >>\nstream\n", (unsigned)cn);
  pdfPut(w, cs, cn);
  pdfPrintf(w, "\nendstream\nendobj\n");
  size_t xref = w.pos;
  pdfPrintf(w, "xref\n0 10\n0000000000 65535 f \n");
  for (int i = 1; i <= 9; i++) pdfPrintf(w, "%010u 00000 n \n", (unsigned)w.off[i]);
  pdfPrintf(w, "trailer\n<< /Size 10 /Root 1 0 R >>\nstartxref\n%u\n%%%%EOF\n", (unsigned)xref);
  sdLock(); w.f.close(); sdUnlock();
  rappSpar(0);
  if (!w.ok) { snprintf(e.fel, sizeof(e.fel), L("SD-kortet blev fullt eller slutade svara.", "The SD card became full or stopped responding.")); return false; }
  return true;
}

// ---------------------------------------------------------------- Rapporten i en egen task (v37.1)
// Skapas utanför huvudloopen: egen stor stack, och skärmen fortsätter ritas under tiden.
static volatile int rappLage = 0;            // 0 vilar, 1 arbetar, 2 klar, 3 fel
static ExpView rappE;
static char rappMon[33]; static float rappDiesel, rappTimKr, rappBredd; static int rappAnLage;
#ifdef ESP_PLATFORM
static void rappTask(void *) {
  bool ok = faltRapport(rappE, rappMon, rappDiesel, rappTimKr, rappAnLage, rappBredd);
  rappLage = ok ? 2 : 3;
  vTaskDelete(nullptr);
}
#endif
static bool rappStarta(const ExpView &e, const char *mon, float diesel, float timKr, int lage, float bredd) {
  if (rappLage == 1) return false;
  rappE = e; strncpy(rappMon, mon, sizeof(rappMon) - 1); rappMon[sizeof(rappMon) - 1] = 0;
  rappDiesel = diesel; rappTimKr = timKr; rappAnLage = lage; rappBredd = bredd;
  rappLage = 1;
#ifdef ESP_PLATFORM
  if (xTaskCreatePinnedToCore(rappTask, "rapport", 24576, nullptr, 1, nullptr, 1) != pdPASS) {
    snprintf(rappE.fel, sizeof(rappE.fel), L("Kunde inte starta (för lite minne).", "Could not start (not enough memory)."));
    rappLage = 3;
  }
#else
  rappLage = faltRapport(rappE, rappMon, rappDiesel, rappTimKr, rappAnLage, rappBredd) ? 2 : 3;
#endif
  return true;
}
