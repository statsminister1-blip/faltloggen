// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  vtview.h – visar redskapets egen bild (spegling av VT-terminalen)
//
//  Redskapets objektpool (fångad av vtpool.h och sparad på SD-kortet) läses in i
//  PSRAM. Kommandona som redskapet skickar till terminalen (nytt värde, visa/dölj,
//  byt mask, ny text m.m.) följs live och skrivs direkt in i poolen. Sedan ritas
//  den aktiva masken och mjukknapparna, uppskalade till skärmen.
//
//  Det är en SPEGEL: inget skickas på bussen, och knapparna går inte att trycka på.
//  Redskapet styrs fortfarande från traktorns terminal.
//
//  Kan provas på datorn med tools/vt/pc (vtview_test).
// ============================================================================
#pragma once
#include <SD.h>
#include <Preferences.h>
#include "esp_heap_caps.h"

// ---------------------------------------------------------------- Tillstånd
static uint8_t  *vvPool = nullptr;      // poolen (PSRAM), ändras av kommandona
static uint32_t  vvLen = 0, vvCap = 0;
static uint32_t *vvIdx = nullptr;       // objekt-id -> position + 1 (0 = finns inte), 65536 st (PSRAM)
static bool      vvOk = false;          // en pool är inläst
static uint64_t  vvName = 0;            // redskapet poolen hör till
static uint32_t  vvCrc = 0;
static uint16_t  vvMask = 0xFFFF;       // aktiv mask
static int       vvM = 480;             // datamaskens storlek som poolen är gjord för
static int       vvKW = 80, vvKH = 52;  // mjukknapparnas storlek som poolen är gjord för
static int       vvKeys = 12;           // fysiska mjukknappar
static int       vvAddr = -1;           // redskapets adress just nu
static uint32_t  vvCmdCount = 0, vvLastCmdMs = 0;
static volatile bool vvDirty = false;   // något har ändrats sedan bilden ritades
static char      vvStatus[96] = "ingen pool inläst";
static uint64_t  vvTriedName = 0;
static uint32_t  vvTriedGen = 0xFFFFFFFF;

static inline uint16_t vvR16(const uint8_t *p) { return p[0] | (p[1] << 8); }
static inline int16_t  vvS16(const uint8_t *p) { return (int16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t vvR32(const uint8_t *p) { return p[0] | (p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
static inline void vvW16(uint8_t *p, uint16_t v) { p[0] = v & 0xFF; p[1] = v >> 8; }
static inline void vvW32(uint8_t *p, uint32_t v) { p[0] = v; p[1] = v >> 8; p[2] = v >> 16; p[3] = v >> 24; }

static inline uint8_t *vvObj(uint16_t id) {
  if (!vvOk || id == 0xFFFF) return nullptr;
  uint32_t o = vvIdx[id];
  return o ? vvPool + o - 1 : nullptr;
}

// ---------------------------------------------------------------- Objektens längd (ISO 11783-6, VT v3/v4)
// Returnerar längden på objektet som börjar vid p, eller -1 om typen är okänd / data saknas.
static int32_t vvObjLen(const uint8_t *d, uint32_t p, uint32_t n) {
  if (p + 3 > n) return -1;
  const uint8_t *o = d + p;
  uint32_t q = 3;
  #define NEED(k) do { if (p + q + (k) > n) return -1; } while (0)
  #define U8(off)  (o[off])
  #define U16(off) (o[off] | (o[(off) + 1] << 8))
  switch (o[2]) {
    case 0:  NEED(7);  q = 10 + 6 * U8(7) + 2 * U8(8) + 2 * U8(9); break;                   // Working Set
    case 1:  NEED(5);  q = 8 + 6 * U8(6) + 2 * U8(7); break;                                // Data Mask
    case 2:  NEED(7);  q = 10 + 6 * U8(8) + 2 * U8(9); break;                               // Alarm Mask
    case 3:  NEED(7);  q = 10 + 6 * U8(8) + 2 * U8(9); break;                               // Container
    case 4:  NEED(3);  q = 6 + 2 * U8(4) + 2 * U8(5); break;                                // Soft Key Mask
    case 5:  NEED(4);  q = 7 + 6 * U8(5) + 2 * U8(6); break;                                // Key
    case 6:  NEED(10); q = 13 + 6 * U8(11) + 2 * U8(12); break;                             // Button
    case 7:  NEED(10); q = 13 + 2 * U8(12); break;                                          // Input Boolean
    case 8:  NEED(14); { uint32_t l = U8(16); q = 17 + l; NEED(2); q += 2 + 2 * o[17 + l + 1]; } break;   // Input String
    case 9:  NEED(35); q = 38 + 2 * U8(37); break;                                          // Input Number
    case 10: NEED(10); q = 13 + 2 * U8(10) + 2 * U8(12); break;                             // Input List
    case 11: NEED(13); { uint32_t l = U16(14); q = 16 + l; NEED(1); q += 1 + 2 * o[16 + l]; } break;   // Output String
    case 12: NEED(26); q = 29 + 2 * U8(28); break;                                          // Output Number
    case 13: NEED(8);  q = 11 + 2 * U8(10); break;                                          // Output Line
    case 14: NEED(10); q = 13 + 2 * U8(12); break;                                          // Output Rectangle
    case 15: NEED(12); q = 15 + 2 * U8(14); break;                                          // Output Ellipse
    case 16: NEED(11); q = 14 + 4 * U8(12) + 2 * U8(13); break;                             // Output Polygon
    case 17: NEED(18); q = 21 + 2 * U8(20); break;                                          // Output Meter
    case 18: NEED(21); q = 24 + 2 * U8(23); break;                                          // Output Linear Bar Graph
    case 19: NEED(24); q = 27 + 2 * U8(26); break;                                          // Output Arched Bar Graph
    case 20: NEED(14); q = 17 + vvR32(o + 12) + 2 * U8(16); break;                          // Picture Graphic
    case 21: q = 7; break;                                                                  // Number Variable
    case 22: NEED(2);  q = 5 + U16(3); break;                                               // String Variable
    case 23: NEED(5);  q = 8 + 2 * U8(7); break;                                            // Font Attributes
    case 24: NEED(5);  q = 8 + 2 * U8(7); break;                                            // Line Attributes
    case 25: NEED(5);  q = 8 + 2 * U8(7); break;                                            // Fill Attributes
    case 26: NEED(2);  { uint32_t l = U8(4); q = 5 + l; NEED(1); q += 1 + 2 * o[5 + l]; } break;   // Input Attributes
    case 27: q = 5; break;                                                                  // Object Pointer
    case 28: NEED(2);  q = 5 + U16(3); break;                                               // Macro
    case 29: case 31: case 32: NEED(3); q = 6 + 6 * U8(5); break;                           // Aux Function / Input 2
    case 30: NEED(4);  q = 7 + 6 * U8(6); break;                                            // Aux Input 1
    case 33: q = 6; break;                                                                  // Aux Control Designator Pointer
    case 35: NEED(7);  q = 10 + 2 * U8(8) + 2 * U8(9); break;                               // Key Group
    case 37: NEED(9);  q = 12 + 2 * U8(10) + 2 * U8(11); break;                             // Output List
    case 39: NEED(2);  q = 5 + U16(3); break;                                               // Colour Map
    case 40: NEED(2);  q = 5 + 7 * U16(3); break;                                           // Object Label Reference List
    default: return -1;
  }
  #undef NEED
  #undef U8
  #undef U16
  if (p + q > n) return -1;
  return (int32_t)q;
}

// Barnlista (id, x, y) för objekt som har en
static bool vvKids(uint8_t *o, uint8_t *&kids, int &n) {
  switch (o[2]) {
    case 0: n = o[7];  kids = o + 10; return true;
    case 1: n = o[6];  kids = o + 8;  return true;
    case 2: n = o[8];  kids = o + 10; return true;
    case 3: n = o[8];  kids = o + 10; return true;
    case 5: n = o[5];  kids = o + 7;  return true;
    case 6: n = o[11]; kids = o + 13; return true;
  }
  return false;
}

// ---------------------------------------------------------------- Inläsning
// Bygger index. Returnerar antal objekt.
static int vvBuildIndex() {
  memset(vvIdx, 0, 65536 * sizeof(uint32_t));
  uint32_t p = 0; int n = 0;
  vvMask = 0xFFFF;
  while (p < vvLen) {
    int32_t l = vvObjLen(vvPool, p, vvLen);
    if (l <= 0) { Serial.printf("vtview: okänt objekt typ %d vid %lu\n", p + 2 < vvLen ? vvPool[p + 2] : -1, (unsigned long)p); break; }
    uint16_t id = vvR16(vvPool + p);
    vvIdx[id] = p + 1;
    if (vvPool[p + 2] == 0 && vvMask == 0xFFFF) vvMask = vvR16(vvPool + p + 5);   // Working Set: aktiv mask
    p += l; n++;
  }
  return n;
}

// Datamaskens storlek gissas om den inte är känd: bredden på det som ligger i (0,0) i en mask.
static int vvGuessM() {
  int best = 0;
  for (uint32_t p = 0; p < vvLen;) {
    int32_t l = vvObjLen(vvPool, p, vvLen);
    if (l <= 0) break;
    uint8_t *o = vvPool + p;
    if (o[2] == 1) {
      uint8_t *k; int nk;
      vvKids(o, k, nk);
      for (int i = 0; i < nk; i++) {
        if (vvR16(k + 6 * i + 2) || vvR16(k + 6 * i + 4)) continue;
        uint8_t *c = vvObj(vvR16(k + 6 * i));
        if (!c) continue;
        int w = 0;
        if (c[2] == 20 || c[2] == 3 || c[2] == 14) w = vvR16(c + 3 + (c[2] == 14 ? 2 : 0));
        if (w > best) best = w;
      }
    }
    p += l;
  }
  return (best >= 100 && best <= 1024) ? best : 480;
}

static void vvKey(char *k, char c, uint64_t name) { snprintf(k, 12, "%c%08lX", c, (unsigned long)(name & 0xFFFFFFFF)); }

// Läser en fil från SD-kortet till en buffert (växer vid behov). Returnerar längden, 0 = fel.
static uint32_t vvReadFile(const char *path, uint8_t *&buf, uint32_t &cap) {
  uint32_t got = 0;
  sdLock();
  File f = SD.open(path, FILE_READ);
  if (f) {
    uint32_t n = f.size();
    if (n > 0 && n <= 2 * 1024 * 1024) {
      if (n > cap) {
        uint8_t *b = (uint8_t *)heap_caps_realloc(buf, n, MALLOC_CAP_SPIRAM);
        if (b) { buf = b; cap = n; }
      }
      if (buf && n <= cap && f.read(buf, n) == n) got = n;
    }
    f.close();
  }
  sdUnlock();
  return got;
}

// Lägger in ett tillägg (objekt som ersätter eller kompletterar poolens) och bygger om indexet
static bool vvMerge(const uint8_t *add, uint32_t alen) {
  static uint8_t *bits = nullptr;
  if (!bits) bits = (uint8_t *)heap_caps_malloc(8192, MALLOC_CAP_SPIRAM);
  if (!bits) return false;
  memset(bits, 0, 8192);
  for (uint32_t p = 0; p < alen;) {
    int32_t l = vvObjLen(add, p, alen);
    if (l <= 0) { alen = p; break; }
    uint16_t id = vvR16(add + p); bits[id >> 3] |= 1 << (id & 7);
    p += l;
  }
  if (!alen) return false;
  uint8_t *nb = (uint8_t *)heap_caps_malloc(vvLen + alen, MALLOC_CAP_SPIRAM);
  if (!nb) return false;
  uint32_t o = 0;
  for (uint32_t p = 0; p < vvLen;) {
    int32_t l = vvObjLen(vvPool, p, vvLen);
    if (l <= 0) break;
    uint16_t id = vvR16(vvPool + p);
    if (!(bits[id >> 3] & (1 << (id & 7)))) { memcpy(nb + o, vvPool + p, l); o += l; }
    p += l;
  }
  memcpy(nb + o, add, alen); o += alen;
  free(vvPool);
  vvPool = nb; vvLen = o; vvCap = o;
  return vvBuildIndex() > 0;
}

// Läser in poolen för redskapet med ISOBUS-namnet name. Filerna hittas på NAME:s låga 32 bitar
// (tillverkare + identitet), så att poolen hittas även om redskapet bytt klass (t.ex. 5 <-> 6).
// Först den som senast sparades som hel pool, annars den största hela poolen. Sedan ett eventuellt tillägg.
static bool vvLoad(uint64_t name) {
  vvOk = false;
  if (!sdOk || !name) { snprintf(vvStatus, sizeof(vvStatus), "ingen pool (SD/namn saknas)"); return false; }
  if (!vvIdx) vvIdx = (uint32_t *)heap_caps_malloc(65536 * sizeof(uint32_t), MALLOC_CAP_SPIRAM);
  if (!vvIdx) { snprintf(vvStatus, sizeof(vvStatus), "inget minne för index"); return false; }
  char low[9], k[12];
  snprintf(low, sizeof(low), "%08lX", (unsigned long)(name & 0xFFFFFFFF));
  Preferences pr;
  pr.begin("vtpool", true);
  vvKey(k, 'n', name); uint32_t crcN = pr.getUInt(k, 0);
  vvKey(k, 'a', name); uint32_t crcA = pr.getUInt(k, 0);
  vvKey(k, 'm', name); uint32_t info = pr.getUInt(k, 0);
  vvKey(k, 'k', name); int keys = pr.getUInt(k, 0);
  pr.end();
  // Kandidater i /VTPOOL: <16 hex>_<8 hex>.BIN med samma låga 32 bitar
  struct Cand { char fn[32]; uint32_t crc, size; };
  static Cand cand[24];
  int nc = 0;
  sdLock();
  File dir = SD.open("/VTPOOL");
  if (dir) {
    for (File e = dir.openNextFile(); e && nc < 24; e = dir.openNextFile()) {
      const char *nm = e.name();
      const char *b = strrchr(nm, '/'); b = b ? b + 1 : nm;
      if (!e.isDirectory() && strlen(b) == 29 && b[16] == '_' && !strncmp(b + 8, low, 8) && !strcasecmp(b + 25, ".BIN")) {
        strcpy(cand[nc].fn, b);
        cand[nc].crc = strtoul(b + 17, nullptr, 16);
        cand[nc].size = e.size();
        nc++;
      }
      e.close();
    }
    dir.close();
  }
  sdUnlock();
  // ordning: senast sparade hela poolen först, sedan störst först
  for (int a = 0; a < nc; a++) for (int b = a + 1; b < nc; b++) {
    bool swap = (cand[b].crc == crcN && cand[a].crc != crcN) ||
                (cand[a].crc != crcN && cand[b].crc != crcN && cand[b].size > cand[a].size);
    if (swap) { Cand t = cand[a]; cand[a] = cand[b]; cand[b] = t; }
  }
  int nObj = 0, used = -1;
  char path[48];
  for (int c = 0; c < nc && used < 0; c++) {
    snprintf(path, sizeof(path), "/VTPOOL/%s", cand[c].fn);
    vvLen = vvReadFile(path, vvPool, vvCap);
    if (!vvLen) continue;
    vvOk = true;                                    // (vvObj kräver vvOk under indexeringen)
    nObj = vvBuildIndex();
    if (nObj > 0 && vvObj(vvMask)) used = c;        // hel pool: har Working Set och aktiv mask
    else vvOk = false;
  }
  if (used < 0) {
    snprintf(vvStatus, sizeof(vvStatus), nc ? "ingen hel pool för redskapet (%d filer)" : "ingen pool för redskapet på SD-kortet", nc);
    return false;
  }
  uint32_t crc = cand[used].crc;
  // tillägg (t.ex. texter efter att redskapet laddat sin bild ur terminalens minne)
  bool merged = false;
  if (crcA && crcA != crc) {
    for (int c = 0; c < nc; c++) if (cand[c].crc == crcA) {
      static uint8_t *ab = nullptr; static uint32_t acap = 0;
      snprintf(path, sizeof(path), "/VTPOOL/%s", cand[c].fn);
      uint32_t al = vvReadFile(path, ab, acap);
      if (al && vvMerge(ab, al)) { merged = true; nObj = vvBuildIndex(); }
      break;
    }
  }
  vvName = name; vvCrc = crc;
  vvM = info & 0xFFFF; vvKW = (info >> 16) & 0xFF; vvKH = info >> 24;
  if (vvM < 100 || vvM > 1024) vvM = vvGuessM();
  if (vvKW < 20 || vvKH < 20) { vvKW = 80; vvKH = 52; }
  vvKeys = (keys >= 2 && keys <= 64) ? keys : 12;
  snprintf(vvStatus, sizeof(vvStatus), "pool %08lX%s, %d objekt, mask %d px, %d mjukknappar", (unsigned long)crc,
           merged ? " + tillägg" : "", nObj, vvM, vvKeys);
  Serial.printf("vtview: %s (%s)\n", vvStatus, cand[used].fn);
  vvDirty = true;
  return true;
}

// Finns en pool för redskapet? (utan att läsa in den)
static bool vvAvailable() { return vvOk && vvName == rdInfo.name && rdInfo.nameKnown; }

// ---------------------------------------------------------------- Kommandon från redskapet
// Attributnummer (Change Attribute, 0xAF) -> position i objektet och storlek, för de vanligaste typerna.
static bool vvAttr(uint8_t type, uint8_t aid, int &off, int &sz) {
  struct A { uint8_t t, a; uint8_t off, sz; };
  static const A T[] = {
    {1,1,3,1},{1,2,4,2}, {2,1,3,1},{2,2,4,2},{2,3,6,1},{2,4,7,1},
    {3,1,3,2},{3,2,5,2},
    {5,1,3,1},{5,2,4,1},
    {6,1,3,2},{6,2,5,2},{6,3,7,1},{6,4,8,1},{6,5,9,1},{6,6,10,1},
    {7,1,3,1},{7,2,4,2},{7,3,6,2},{7,4,8,2},
    {8,1,3,2},{8,2,5,2},{8,3,7,1},{8,4,8,2},{8,5,10,2},{8,6,12,1},{8,7,13,2},{8,8,15,1},
    {9,1,3,2},{9,2,5,2},{9,3,7,1},{9,4,8,2},{9,5,10,1},{9,6,11,2},{9,7,17,4},{9,8,21,4},{9,9,25,4},{9,10,29,4},{9,11,33,1},{9,12,34,1},{9,13,35,1},{9,14,36,1},
    {10,1,3,2},{10,2,5,2},{10,3,7,2},{10,4,11,1},
    {11,1,3,2},{11,2,5,2},{11,3,7,1},{11,4,8,2},{11,5,10,1},{11,6,11,2},{11,7,13,1},
    {12,1,3,2},{12,2,5,2},{12,3,7,1},{12,4,8,2},{12,5,10,1},{12,6,11,2},{12,7,17,4},{12,8,21,4},{12,9,25,1},{12,10,26,1},{12,11,27,1},
    {13,1,3,2},{13,2,5,2},{13,3,7,2},{13,4,9,1},
    {14,1,3,2},{14,2,5,2},{14,3,7,2},{14,4,9,1},{14,5,10,2},
    {15,1,3,2},{15,2,5,2},{15,3,7,2},{15,4,9,1},{15,5,10,1},{15,6,11,1},{15,7,12,2},
    {16,1,3,2},{16,2,5,2},{16,3,7,2},{16,4,9,2},{16,5,11,1},
    {17,1,3,2},{17,2,5,1},{17,3,6,1},{17,4,7,1},{17,5,8,1},{17,6,9,1},{17,7,10,1},{17,8,11,1},{17,9,12,2},{17,10,14,2},{17,11,16,2},
    {18,1,3,2},{18,2,5,2},{18,3,7,1},{18,4,8,1},{18,5,9,1},{18,6,10,1},{18,7,11,2},{18,8,13,2},{18,9,15,2},{18,10,19,2},{18,11,21,2},
    {20,1,3,2},{20,2,10,1},{20,3,11,1},
    {23,1,3,1},{23,2,4,1},{23,3,5,1},{23,4,6,1},
    {24,1,3,1},{24,2,4,1},{24,3,5,2},
    {25,1,3,1},{25,2,4,1},{25,3,5,2},
  };
  for (auto &x : T) if (x.t == type && x.a == aid) { off = x.off; sz = x.sz; return true; }
  return false;
}

static void vvApply(const uint8_t *b, int n) {
  if (!vvOk || n < 3) return;
  uint8_t f = b[0];
  uint8_t *o = vvObj(vvR16(b + 1));
  switch (f) {
    case 0xA0:                                            // visa/dölj container
      if (o && o[2] == 3 && n >= 4) o[7] = b[3] ? 0 : 1;
      break;
    case 0xA5: {                                          // flytta barn relativt
      if (!o || n < 7) break;
      uint8_t *k; int nk;
      if (!vvKids(o, k, nk)) break;
      uint16_t c = vvR16(b + 3);
      for (int i = 0; i < nk; i++) if (vvR16(k + 6 * i) == c) {
        vvW16(k + 6 * i + 2, vvS16(k + 6 * i + 2) + (int)b[5] - 127);
        vvW16(k + 6 * i + 4, vvS16(k + 6 * i + 4) + (int)b[6] - 127);
      }
      break;
    }
    case 0xA6:                                            // ny storlek
      if (!o || n < 7) break;
      if (o[2] == 20) vvW16(o + 3, vvR16(b + 3));
      else if (o[2] == 3 || o[2] == 6 || o[2] == 8 || o[2] == 9 || o[2] == 10 || o[2] == 11 || o[2] == 12 || o[2] == 16 || o[2] == 18 || o[2] == 19) {
        vvW16(o + 3, vvR16(b + 3)); vvW16(o + 5, vvR16(b + 5));
      } else if (o[2] == 13 || o[2] == 14 || o[2] == 15) { vvW16(o + 5, vvR16(b + 3)); vvW16(o + 7, vvR16(b + 5)); }
      break;
    case 0xA7:                                            // ny bakgrundsfärg
      if (!o || n < 4) break;
      if (o[2] == 1 || o[2] == 2 || o[2] == 4 || o[2] == 5 || o[2] == 7) o[3] = b[3];
      else if (o[2] == 6 || o[2] == 8 || o[2] == 9 || o[2] == 11 || o[2] == 12) o[7] = b[3];
      break;
    case 0xA8: {                                          // nytt värde (tal, objektpekare, lista ...)
      if (!o || n < 8) break;
      uint32_t v = vvR32(b + 4);
      switch (o[2]) {
        case 7:  o[10] = v; break;
        case 9:  case 12: vvW32(o + 13, v); break;
        case 10: o[9] = v; break;
        case 17: vvW16(o + 18, v); break;
        case 18: vvW16(o + 17, v); break;
        case 19: vvW16(o + 20, v); break;
        case 21: vvW32(o + 3, v); break;
        case 27: vvW16(o + 3, v); break;
        case 37: o[9] = v; break;
      }
      break;
    }
    case 0xAA: if (o && o[2] == 23 && n >= 7) memcpy(o + 3, b + 3, 4); break;   // teckensnitt
    case 0xAB: if (o && o[2] == 24 && n >= 7) memcpy(o + 3, b + 3, 4); break;   // linje
    case 0xAC: if (o && o[2] == 25 && n >= 7) memcpy(o + 3, b + 3, 4); break;   // fyllning
    case 0xAD: if (n >= 5 && vvObj(vvR16(b + 3))) vvMask = vvR16(b + 3); break;  // byt aktiv mask
    case 0xAE: {                                          // byt mjukknappsmask
      if (n < 6) break;
      uint8_t *m = vvObj(vvR16(b + 2));
      if (m && (m[2] == 1 || m[2] == 2)) vvW16(m + 4, vvR16(b + 4));
      break;
    }
    case 0xAF: {                                          // ändra attribut
      if (!o || n < 8) break;
      int off, sz;
      if (!vvAttr(o[2], b[3], off, sz)) break;
      uint32_t v = vvR32(b + 4);
      if (sz == 1) o[off] = v; else if (sz == 2) vvW16(o + off, v); else vvW32(o + off, v);
      break;
    }
    case 0xB1: {                                          // byt listpost
      if (!o || n < 6) break;
      int i = b[3];
      if (o[2] == 10 && i < o[10]) vvW16(o + 13 + 2 * i, vvR16(b + 4));
      if (o[2] == 37 && i < o[10]) vvW16(o + 12 + 2 * i, vvR16(b + 4));
      break;
    }
    case 0xB3: {                                          // ny text
      if (!o || n < 5) break;
      int l = vvR16(b + 3); if (l > n - 5) l = n - 5;
      uint8_t *dst = nullptr; int cap = 0;
      if (o[2] == 22) { dst = o + 5; cap = vvR16(o + 3); }
      else if (o[2] == 11) { dst = o + 16; cap = vvR16(o + 14); }
      else if (o[2] == 8) { dst = o + 17; cap = o[16]; }
      if (!dst) break;
      for (int i = 0; i < cap; i++) dst[i] = i < l ? b[5 + i] : ' ';
      break;
    }
    case 0xB4: {                                          // ny position för barn
      if (!o || n < 9) break;
      uint8_t *k; int nk;
      if (!vvKids(o, k, nk)) break;
      uint16_t c = vvR16(b + 3);
      for (int i = 0; i < nk; i++) if (vvR16(k + 6 * i) == c) { vvW16(k + 6 * i + 2, vvR16(b + 5)); vvW16(k + 6 * i + 4, vvR16(b + 7)); }
      break;
    }
    default: return;
  }
  vvDirty = true;
}

// ---------------------------------------------------------------- Rita
// VT-standardens 256 färger -> RGB565
static uint16_t vvPal[256];
static void vvPalInit() {
  static const uint8_t B[16][3] = {{0,0,0},{255,255,255},{0,153,0},{0,153,153},{153,0,0},{153,0,153},{153,153,0},{204,204,204},
                                   {153,153,153},{0,0,255},{0,255,0},{0,255,255},{255,0,0},{255,0,255},{255,255,0},{0,0,153}};
  auto c565 = [](int r, int g, int b) { return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)); };
  for (int i = 0; i < 16; i++) vvPal[i] = c565(B[i][0], B[i][1], B[i][2]);
  for (int i = 16; i < 232; i++) { int j = i - 16; vvPal[i] = c565((j / 36) * 51, ((j / 6) % 6) * 51, (j % 6) * 51); }
  for (int i = 232; i < 256; i++) vvPal[i] = 0;
}

// Teckensnitt: ISO-storlek -> cell (bredd x höjd) i VT-bildpunkter
static const uint8_t VV_FONT[15][2] = {{6,8},{8,8},{8,12},{12,16},{16,16},{16,24},{24,32},{32,32},{32,48},{48,64},{64,64},{64,96},{96,128},{128,128},{128,192}};
struct VvFont { const uint8_t *f; uint8_t w, h, asc; bool bold; };
static const VvFont VV_U8[] = {
  {u8g2_font_4x6_tf, 4, 6, 5, false},     {u8g2_font_5x8_tf, 5, 7, 6, false},     {u8g2_font_6x10_tf, 6, 9, 7, false},
  {u8g2_font_6x13_tf, 6, 11, 9, false},   {u8g2_font_6x13B_tf, 6, 11, 9, true},   {u8g2_font_7x14_tf, 7, 12, 10, false},
  {u8g2_font_7x14B_tf, 7, 12, 10, true},  {u8g2_font_8x13_tf, 8, 11, 9, false},   {u8g2_font_8x13B_tf, 8, 12, 10, true},
  {u8g2_font_9x15_tf, 9, 13, 10, false},  {u8g2_font_9x15B_tf, 9, 13, 10, true},  {u8g2_font_10x20_tf, 10, 17, 13, false},
  {u8g2_font_profont22_tf, 12, 18, 14, false}, {u8g2_font_spleen12x24_mf, 12, 20, 15, false},
  {u8g2_font_profont29_tf, 16, 24, 19, false}, {u8g2_font_spleen16x32_mf, 16, 26, 20, false},
  {u8g2_font_inr24_mf, 21, 31, 25, false}, {u8g2_font_inb24_mf, 21, 30, 24, true},
  {u8g2_font_inr33_mf, 28, 42, 33, false}, {u8g2_font_inb33_mf, 28, 42, 33, true},
  {u8g2_font_inb46_mf, 38, 58, 46, true},
};
static const VvFont *vvPickFont(int cw, int ch, bool bold) {
  const VvFont *best = &VV_U8[0];
  int bestScore = -1;
  for (auto &f : VV_U8) {
    if (f.w > cw || f.h > ch + 1) continue;
    int score = f.asc * 4 + (f.bold == bold ? 2 : 0);
    if (score > bestScore) { bestScore = score; best = &f; }
  }
  return best;
}

// Ritkontext
static Arduino_GFX *vvG = nullptr;
static uint16_t *vvFb = nullptr;             // bildbufferten (800 bred), om den finns
static int vvOx, vvOy;                       // skärmens origo för VT (0,0)
static int32_t vvK;                          // skala 16.16
static int vvCx0, vvCy0, vvCx1, vvCy1;       // klipprektangel (skärmbildpunkter, x1/y1 exklusiva)
static inline int vvSX(int v) { return vvOx + (int)(((int64_t)v * vvK) >> 16); }
static inline int vvSY(int v) { return vvOy + (int)(((int64_t)v * vvK) >> 16); }

static void vvFill(int x0, int y0, int x1, int y1, uint16_t c) {
  if (x0 < vvCx0) x0 = vvCx0; if (y0 < vvCy0) y0 = vvCy0;
  if (x1 > vvCx1) x1 = vvCx1; if (y1 > vvCy1) y1 = vvCy1;
  if (x0 >= x1 || y0 >= y1) return;
  if (vvFb) {
    for (int y = y0; y < y1; y++) { uint16_t *p = vvFb + y * 800 + x0; for (int x = x0; x < x1; x++) *p++ = c; }
  } else vvG->fillRect(x0, y0, x1 - x0, y1 - y0, c);
}
static inline void vvPix(int x, int y, uint16_t c) {
  if (x < vvCx0 || y < vvCy0 || x >= vvCx1 || y >= vvCy1) return;
  if (vvFb) vvFb[y * 800 + x] = c; else vvG->drawPixel(x, y, c);
}
// Ram med tjocklek t (bitar i sup: 1 topp, 2 höger, 4 botten, 8 vänster = utelämnas)
static void vvFrame(int x0, int y0, int x1, int y1, int t, uint16_t c, int sup) {
  if (!(sup & 1)) vvFill(x0, y0, x1, y0 + t, c);
  if (!(sup & 4)) vvFill(x0, y1 - t, x1, y1, c);
  if (!(sup & 8)) vvFill(x0, y0, x0 + t, y1, c);
  if (!(sup & 2)) vvFill(x1 - t, y0, x1, y1, c);
}
static void vvLine(int x0, int y0, int x1, int y1, int t, uint16_t c) {
  int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1, dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1, e = dx + dy;
  int h = t / 2;
  for (;;) {
    vvFill(x0 - h, y0 - h, x0 - h + t, y0 - h + t, c);
    if (x0 == x1 && y0 == y1) break;
    int e2 = 2 * e;
    if (e2 >= dy) { e += dy; x0 += sx; }
    if (e2 <= dx) { e += dx; y0 += sy; }
  }
}
// Fylld polygon (jämn-udda), punkter i skärmbildpunkter
static void vvPolyFill(const int *px, const int *py, int n, uint16_t c) {
  if (n < 3) return;
  int ymin = py[0], ymax = py[0];
  for (int i = 1; i < n; i++) { ymin = min(ymin, py[i]); ymax = max(ymax, py[i]); }
  ymin = max(ymin, vvCy0); ymax = min(ymax, vvCy1 - 1);
  static int xs[64];
  for (int y = ymin; y <= ymax; y++) {
    int m = 0;
    for (int i = 0, j = n - 1; i < n && m < 64; j = i++) {
      int yi = py[i], yj = py[j];
      if ((yi <= y && yj > y) || (yj <= y && yi > y))
        xs[m++] = px[i] + (int)((int64_t)(y - yi) * (px[j] - px[i]) / (yj - yi));
    }
    for (int a = 1; a < m; a++) { int v = xs[a], b = a - 1; while (b >= 0 && xs[b] > v) { xs[b + 1] = xs[b]; b--; } xs[b + 1] = v; }
    for (int a = 0; a + 1 < m; a += 2) vvFill(xs[a], y, xs[a + 1] + 1, y + 1, c);
  }
}
static void vvEllipse(int x0, int y0, int x1, int y1, int t, uint16_t lc, bool fill, uint16_t fc, bool line) {
  float cx = (x0 + x1 - 1) / 2.0f, cy = (y0 + y1 - 1) / 2.0f, rx = (x1 - x0) / 2.0f, ry = (y1 - y0) / 2.0f;
  if (rx < 0.5f || ry < 0.5f) return;
  for (int y = y0; y < y1; y++) {
    float dy = (y - cy) / ry;
    if (dy * dy > 1) continue;
    float hw = rx * sqrtf(1 - dy * dy);
    int a = (int)lroundf(cx - hw), b = (int)lroundf(cx + hw);
    if (fill) vvFill(a, y, b + 1, y + 1, fc);
    if (line) {
      float rxi = rx - t, ryi = ry - t;
      float dyi = ryi > 0 ? (y - cy) / ryi : 2;
      if (dyi * dyi >= 1 || rxi <= 0) vvFill(a, y, b + 1, y + 1, lc);
      else {
        float hwi = rxi * sqrtf(1 - dyi * dyi);
        int ai = (int)lroundf(cx - hwi), bi = (int)lroundf(cx + hwi);
        vvFill(a, y, ai, y + 1, lc); vvFill(bi + 1, y, b + 1, y + 1, lc);
      }
    }
  }
}

// Bild (Picture Graphic): avkodas rad för rad (RLE eller rå) och skalas till skärmen
static uint8_t vvRow[2048];
static void vvPicture(uint8_t *o, int x, int y) {
  int w = vvR16(o + 3), aw = vvR16(o + 5), ah = vvR16(o + 7);
  int fmt = o[9], opt = o[10], tc = o[11];
  uint32_t nraw = vvR32(o + 12);
  const uint8_t *raw = o + 17;
  if (aw <= 0 || ah <= 0 || w <= 0 || aw > 2048) return;
  int bpp = fmt == 0 ? 1 : fmt == 1 ? 4 : 8;
  int rowb = (aw * bpp + 7) / 8;
  int h = (int)((int64_t)ah * w / aw);
  int X0 = vvSX(x), X1 = vvSX(x + w), Y0 = vvSY(y), Y1 = vvSY(y + h);
  int DW = X1 - X0, DH = Y1 - Y0;
  if (DW <= 0 || DH <= 0) return;
  bool rle = opt & 4, transp = opt & 1;
  uint32_t rp = 0; int runLeft = 0; uint8_t runVal = 0;
  int curRow = -1;
  for (int dy = 0; dy < DH; dy++) {
    int sy = (int)((int64_t)dy * ah / DH);
    while (curRow < sy) {                             // läs fram till rad sy
      for (int i = 0; i < rowb; i++) {
        uint8_t v = 0;
        if (rle) {
          if (runLeft == 0) {
            if (rp + 1 >= nraw) { runLeft = 1 << 30; runVal = 0; }
            else { runLeft = raw[rp]; runVal = raw[rp + 1]; rp += 2; if (runLeft == 0) { i--; continue; } }
          }
          v = runVal; runLeft--;
        } else {
          uint32_t q = (uint32_t)(curRow + 1) * rowb + i;
          v = q < nraw ? raw[q] : 0;
        }
        vvRow[i] = v;
      }
      curRow++;
    }
    int Y = Y0 + dy;
    if (Y < vvCy0 || Y >= vvCy1) continue;
    for (int dx = 0; dx < DW; dx++) {
      int X = X0 + dx;
      if (X < vvCx0 || X >= vvCx1) continue;
      int sx = (int)((int64_t)dx * aw / DW);
      int v;
      if (bpp == 8) v = vvRow[sx];
      else if (bpp == 4) v = (sx & 1) ? (vvRow[sx >> 1] & 15) : (vvRow[sx >> 1] >> 4);
      else v = (vvRow[sx >> 3] >> (7 - (sx & 7))) & 1;
      if (transp && v == tc) continue;
      vvPix(X, Y, vvPal[v]);
    }
  }
}

// Text i ett fält (VT-koordinater). Latin-1 in.
static void vvText(int x, int y, int w, int h, const uint8_t *s, int len, uint16_t fontId, uint8_t just,
                   int bg, bool transp, bool wrap) {
  uint8_t *fa = vvObj(fontId);
  int col = fa ? fa[3] : 0, size = fa ? fa[4] : 0, style = fa ? fa[6] : 0;
  if (size > 14) size = 14;
  if (style & 0x10) { int t = col; col = bg; bg = t; transp = false; }   // inverterad
  int X0 = vvSX(x), Y0 = vvSY(y), X1 = vvSX(x + w), Y1 = vvSY(y + h);
  if (!transp) vvFill(X0, Y0, X1, Y1, vvPal[bg & 255]);
  int cw = VV_FONT[size][0], ch = VV_FONT[size][1];
  float CW = cw * vvK / 65536.0f, CH = ch * vvK / 65536.0f;
  const VvFont *F = vvPickFont((int)CW, (int)CH, style & 1);
  // dela upp i rader
  static char lines[8][128]; int nl = 0;            // statisk: sparar stack (anropas inte rekursivt)
  {
    int maxc = cw > 0 ? w / cw : 1; if (maxc < 1) maxc = 1; if (maxc > 127) maxc = 127;
    static char cur[128]; int cl = 0;
    for (int i = 0; i <= len && nl < 8; i++) {
      char c = i < len ? (char)s[i] : '\n';
      if (c == '\r' || c == '\n' || c == 0) {
        if (c == '\r' && i + 1 < len && s[i + 1] == '\n') i++;
        cur[cl] = 0; memcpy(lines[nl++], cur, cl + 1); cl = 0;
        if (c == 0) break;
        continue;
      }
      if (wrap && cl >= maxc) {
        int cut = cl; while (cut > 0 && cur[cut - 1] != ' ') cut--;
        if (cut == 0) cut = cl;
        static char rest[128]; int rl = cl - cut; memcpy(rest, cur + cut, rl);
        cur[cut] = 0; memcpy(lines[nl++], cur, cut + 1);
        memcpy(cur, rest, rl); cl = rl;
        if (nl >= 8) break;
      }
      if (cl < 127) cur[cl++] = c;
    }
  }
  int hj = just & 3, vj = (just >> 2) & 3;
  int tot = nl * ch;
  int ty = vj == 0 ? 0 : vj == 1 ? (h - tot) / 2 : h - tot;
  int ocx0 = vvCx0, ocy0 = vvCy0, ocx1 = vvCx1, ocy1 = vvCy1;    // klipp till fältet
  vvCx0 = max(vvCx0, X0); vvCy0 = max(vvCy0, Y0); vvCx1 = min(vvCx1, X1 + 1); vvCy1 = min(vvCy1, Y1 + 1);
  vvG->setFont(F->f);
  vvG->setTextColor(vvPal[col & 255]);
  for (int li = 0; li < nl; li++) {
    char *ln = lines[li];
    int n = strlen(ln);
    if (hj) { while (n > 0 && ln[n - 1] == ' ') n--; }
    int start = 0;
    if (hj) while (start < n && ln[start] == ' ') start++;
    int tw = (n - start) * cw;
    int tx = hj == 0 ? 0 : hj == 1 ? (w - tw) / 2 : w - tw;
    int cy = y + ty + li * ch;
    float bx = vvOx + (x + tx) * vvK / 65536.0f;
    int baseY = vvSY(cy) + (int)((CH - F->h) / 2 + F->asc + 0.5f);
    for (int i = start; i < n; i++) {
      uint8_t c = (uint8_t)ln[i];
      if (c == ' ') continue;
      float cx = bx + (i - start) * CW;
      int px = (int)(cx + (CW - F->w) / 2 + 0.5f);
      if (px < vvCx0 - 1 || px + F->w > vvCx1 + 1) continue;
      if (baseY - F->asc < vvCy0 - 2 || baseY > vvCy1 + 2) continue;
      char u[3];
      if (c < 0x80) { u[0] = c; u[1] = 0; } else { u[0] = 0xC0 | (c >> 6); u[1] = 0x80 | (c & 0x3F); u[2] = 0; }
      vvG->setCursor(px, baseY);
      vvG->print(u);
    }
  }
  vvCx0 = ocx0; vvCy0 = ocy0; vvCx1 = ocx1; vvCy1 = ocy1;
}

static void vvNumber(uint8_t *o, int x, int y, bool input) {
  int w = vvR16(o + 3), h = vvR16(o + 5), bg = o[7];
  uint16_t font = vvR16(o + 8);
  int opt = o[10];
  uint16_t var = vvR16(o + 11);
  uint32_t raw = vvR32(o + 13);
  int32_t off; float scale; int dec, fmt, just;
  if (input) { off = (int32_t)vvR32(o + 25); memcpy(&scale, o + 29, 4); dec = o[33]; fmt = o[34]; just = o[35]; }
  else { off = (int32_t)vvR32(o + 17); memcpy(&scale, o + 21, 4); dec = o[25]; fmt = o[26]; just = o[27]; }
  uint8_t *vo = vvObj(var);
  if (vo && vo[2] == 21) raw = vvR32(vo + 3);
  double v = ((double)(int32_t)raw + off) * scale;
  if (dec > 7) dec = 7;
  char b[40];
  if ((opt & 4) && v == 0) b[0] = 0;
  else {
    if (opt & 8) { double m = pow(10, dec); v = trunc(v * m) / m; }
    if (fmt == 1) snprintf(b, sizeof(b), "%.*e", dec, v); else snprintf(b, sizeof(b), "%.*f", dec, v);
    if (opt & 2) {                                    // inledande nollor till fältets bredd
      uint8_t *fa = vvObj(font);
      int cw = VV_FONT[fa ? min((int)fa[4], 14) : 0][0];
      int want = cw ? w / cw : 0, l = strlen(b);
      if (want > l && want < 39) {
        bool neg = b[0] == '-';
        char t[40]; int z = want - l, o2 = 0;
        if (neg) t[o2++] = '-';
        for (int i = 0; i < z; i++) t[o2++] = '0';
        strcpy(t + o2, b + (neg ? 1 : 0));
        strcpy(b, t);
      }
    }
  }
  vvText(x, y, w, h, (const uint8_t *)b, strlen(b), font, just, bg, opt & 1, false);
}

static void vvDraw(uint16_t id, int x, int y, int depth);

static void vvDrawKids(uint8_t *o, int x, int y, int depth) {
  uint8_t *k; int n;
  if (!vvKids(o, k, n)) return;
  for (int i = 0; i < n; i++) vvDraw(vvR16(k + 6 * i), x + vvS16(k + 6 * i + 2), y + vvS16(k + 6 * i + 4), depth + 1);
}

static int vvRegionW = 0, vvRegionH = 0;     // maskens / knappens storlek i VT-bildpunkter (för bakgrunden)

static void vvDraw(uint16_t id, int x, int y, int depth) {
  uint8_t *o = vvObj(id);
  if (!o || depth > 24) return;
  switch (o[2]) {
    case 1: case 2:                                    // mask
      vvFill(vvSX(x), vvSY(y), vvSX(x + vvRegionW), vvSY(y + vvRegionH), vvPal[o[3]]);
      vvDrawKids(o, x, y, depth);
      break;
    case 3:                                            // container
      if (!o[7]) vvDrawKids(o, x, y, depth);
      break;
    case 5:                                            // mjukknapp
      vvFill(vvSX(x), vvSY(y), vvSX(x + vvRegionW), vvSY(y + vvRegionH), vvPal[o[3]]);
      vvDrawKids(o, x, y, depth);
      break;
    case 6: {                                          // knapp
      int w = vvR16(o + 3), h = vvR16(o + 5), opt = o[10];
      int X0 = vvSX(x), Y0 = vvSY(y), X1 = vvSX(x + w), Y1 = vvSY(y + h);
      if (!(opt & 8)) vvFill(X0, Y0, X1, Y1, vvPal[o[7]]);
      bool border = !(opt & 0x24);
      if (border) vvFrame(X0, Y0, X1, Y1, max(2, (int)(2 * vvK >> 16)), vvPal[o[8]], 0);
      vvDrawKids(o, x + (border ? 4 : 0), y + (border ? 4 : 0), depth);
      break;
    }
    case 7: {                                          // in: ja/nej
      int w = vvR16(o + 4);
      uint8_t *fa = vvObj(vvR16(o + 6));
      uint16_t fg = vvPal[fa ? fa[3] : 0];
      int v = o[10];
      uint8_t *vo = vvObj(vvR16(o + 8));
      if (vo && vo[2] == 21) v = vvR32(vo + 3);
      int X0 = vvSX(x), Y0 = vvSY(y), X1 = vvSX(x + w), Y1 = vvSY(y + w);
      vvFill(X0, Y0, X1, Y1, vvPal[o[3]]);
      vvFrame(X0, Y0, X1, Y1, 1, fg, 0);
      if (v) {
        int t = max(2, (int)(2 * vvK >> 16));
        vvLine(vvSX(x + 2), vvSY(y + w / 2), vvSX(x + w / 2), vvSY(y + w - 3), t, fg);
        vvLine(vvSX(x + w / 2), vvSY(y + w - 3), vvSX(x + w - 2), vvSY(y + 2), t, fg);
      }
      break;
    }
    case 8: {                                          // in: text
      uint8_t *so = vvObj(vvR16(o + 13));
      const uint8_t *s = o + 17; int l = o[16];
      if (so && so[2] == 22) { s = so + 5; l = vvR16(so + 3); }
      vvText(x, y, vvR16(o + 3), vvR16(o + 5), s, l, vvR16(o + 8), o[15], o[7], o[12] & 1, o[12] & 2);
      break;
    }
    case 9:  vvNumber(o, x, y, true); break;           // in: tal
    case 12: vvNumber(o, x, y, false); break;          // ut: tal
    case 10: case 37: {                                // lista: visa vald post
      int v = o[9];
      uint8_t *vo = vvObj(vvR16(o + 7));
      if (vo && vo[2] == 21) v = vvR32(vo + 3);
      int ni = o[10];
      uint8_t *items = o + (o[2] == 10 ? 13 : 12);
      if (v >= 0 && v < ni) vvDraw(vvR16(items + 2 * v), x, y, depth + 1);
      break;
    }
    case 11: {                                         // ut: text
      uint8_t *so = vvObj(vvR16(o + 11));
      const uint8_t *s = o + 16; int l = vvR16(o + 14);
      if (so && so[2] == 22) { s = so + 5; l = vvR16(so + 3); }
      vvText(x, y, vvR16(o + 3), vvR16(o + 5), s, l, vvR16(o + 8), o[13], o[7], o[10] & 1, o[10] & 2);
      break;
    }
    case 13: {                                         // linje
      uint8_t *la = vvObj(vvR16(o + 3));
      int w = vvR16(o + 5), h = vvR16(o + 7);
      int t = max(1, (int)((la ? la[4] : 1) * vvK >> 16));
      uint16_t c = vvPal[la ? la[3] : 0];
      if (o[9] == 0) vvLine(vvSX(x), vvSY(y), vvSX(x + w) - 1, vvSY(y + h) - 1, t, c);
      else vvLine(vvSX(x), vvSY(y + h) - 1, vvSX(x + w) - 1, vvSY(y), t, c);
      break;
    }
    case 14: {                                         // rektangel
      uint8_t *la = vvObj(vvR16(o + 3)), *fa = vvObj(vvR16(o + 10));
      int X0 = vvSX(x), Y0 = vvSY(y), X1 = vvSX(x + vvR16(o + 5)), Y1 = vvSY(y + vvR16(o + 7));
      uint16_t lc = vvPal[la ? la[3] : 0];
      if (fa && fa[3] == 1) vvFill(X0, Y0, X1, Y1, lc);
      else if (fa && (fa[3] == 2 || fa[3] == 3)) vvFill(X0, Y0, X1, Y1, vvPal[fa[4]]);
      int lw = la ? la[4] : 1;
      if (lw) vvFrame(X0, Y0, X1, Y1, max(1, (int)(lw * vvK >> 16)), lc, o[9]);
      break;
    }
    case 15: {                                         // ellips
      uint8_t *la = vvObj(vvR16(o + 3)), *fa = vvObj(vvR16(o + 12));
      int lw = la ? la[4] : 1;
      uint16_t lc = vvPal[la ? la[3] : 0];
      bool fill = fa && fa[3] >= 1 && fa[3] <= 3;
      uint16_t fc = fa ? (fa[3] == 1 ? lc : vvPal[fa[4]]) : 0;
      vvEllipse(vvSX(x), vvSY(y), vvSX(x + vvR16(o + 5)), vvSY(y + vvR16(o + 7)), max(1, (int)(lw * vvK >> 16)), lc, fill, fc, lw > 0);
      break;
    }
    case 16: {                                         // polygon
      uint8_t *la = vvObj(vvR16(o + 7)), *fa = vvObj(vvR16(o + 9));
      int n = min((int)o[12], 64);
      static int px[64], py[64];                     // statiska: vvDraw är rekursiv, stacken är liten
      for (int i = 0; i < n; i++) { px[i] = vvSX(x + vvR16(o + 14 + 4 * i)); py[i] = vvSY(y + vvR16(o + 16 + 4 * i)); }
      uint16_t lc = vvPal[la ? la[3] : 0];
      if (fa && fa[3] >= 1 && fa[3] <= 3) vvPolyFill(px, py, n, fa[3] == 1 ? lc : vvPal[fa[4]]);
      int lw = la ? la[4] : 1;
      if (lw) {
        int t = max(1, (int)(lw * vvK >> 16));
        for (int i = 0; i + 1 < n; i++) vvLine(px[i], py[i], px[i + 1], py[i + 1], t, lc);
        if (o[11] != 3 && n > 2) vvLine(px[n - 1], py[n - 1], px[0], py[0], t, lc);
      }
      break;
    }
    case 18: {                                         // stapel
      int w = vvR16(o + 3), h = vvR16(o + 5), opt = o[9];
      int mn = vvR16(o + 11), mx = vvR16(o + 13), v = vvR16(o + 17);
      uint8_t *vo = vvObj(vvR16(o + 15));
      if (vo && vo[2] == 21) v = vvR32(vo + 3);
      float fr = mx > mn ? (float)(v - mn) / (mx - mn) : 0; fr = fr < 0 ? 0 : fr > 1 ? 1 : fr;
      int X0 = vvSX(x), Y0 = vvSY(y), X1 = vvSX(x + w), Y1 = vvSY(y + h);
      uint16_t c = vvPal[o[7]];
      if (opt & 0x10) { int xe = (opt & 0x20) ? X1 - (int)((X1 - X0) * fr) : X0 + (int)((X1 - X0) * fr);
                        if (opt & 0x20) vvFill(xe, Y0, X1, Y1, c); else vvFill(X0, Y0, xe, Y1, c); }
      else { int ye = (opt & 0x20) ? Y0 + (int)((Y1 - Y0) * fr) : Y1 - (int)((Y1 - Y0) * fr);
             if (opt & 0x20) vvFill(X0, Y0, X1, ye, c); else vvFill(X0, ye, X1, Y1, c); }
      if (opt & 1) vvFrame(X0, Y0, X1, Y1, 1, c, 0);
      break;
    }
    case 20: vvPicture(o, x, y); break;                 // bild
    case 27: vvDraw(vvR16(o + 3), x, y, depth + 1); break;   // objektpekare
    default: break;                                    // mätare, bågstapel m.m. ritas inte än
  }
}

// Ritar aktiv mask i rutan (x, y, storlek) och returnerar mjukknappsmaskens id
static uint16_t vvRenderMask(Arduino_GFX *g, uint16_t *fb, int x, int y, int size) {
  vvG = g; vvFb = fb;
  uint8_t *m = vvObj(vvMask);
  if (!m) return 0xFFFF;
  vvK = (int32_t)(((int64_t)size << 16) / vvM);
  vvOx = x; vvOy = y;
  vvCx0 = x; vvCy0 = y; vvCx1 = x + size; vvCy1 = y + size;
  vvRegionW = vvM; vvRegionH = vvM;
  vvDraw(vvMask, 0, 0, 0);
  return (m[2] == 1 || m[2] == 2) ? vvR16(m + 4) : 0xFFFF;
}

// Ritar mjukknapp nr i ur mjukknappsmasken skm i rutan (x, y, w, h)
static void vvRenderKey(Arduino_GFX *g, uint16_t *fb, uint16_t skm, int i, int x, int y, int w, int h) {
  vvG = g; vvFb = fb;
  uint8_t *s = vvObj(skm);
  vvK = (int32_t)(((int64_t)w << 16) / vvKW);
  vvOx = x; vvOy = y;
  vvCx0 = x; vvCy0 = y; vvCx1 = x + w; vvCy1 = y + h;
  vvRegionW = vvKW; vvRegionH = vvKH;
  uint16_t bg = s && s[2] == 4 ? vvPal[s[3]] : vvPal[8];
  vvFill(x, y, x + w, y + h, bg);
  if (!s || s[2] != 4 || i >= s[4]) return;
  vvDraw(vvR16(s + 6 + 2 * i), 0, 0, 0);
}

// Hela vyn: masken i rutan (mx, my, size); mjukknapparna i två kolumner i rutan (kx, ky, kw, kh) om kw > 0
static void vtviewRender(Arduino_GFX *g, uint16_t *fb, int mx, int my, int size, int kx, int ky, int kw, int kh) {
  if (!vvOk) return;
  uint16_t skm = vvRenderMask(g, fb, mx, my, size);
  if (kw <= 0) return;
  const int gap = 6, cols = 2;
  int rows = (vvKeys + cols - 1) / cols;
  float s1 = (kw - (cols + 1) * gap) / (float)(cols * vvKW);
  float s2 = (kh - (rows + 1) * gap) / (float)(rows * vvKH);
  float ks = s1 < s2 ? s1 : s2;
  int w = (int)(vvKW * ks), h = (int)(vvKH * ks);
  int y0 = ky + (kh - (rows * h + (rows - 1) * gap)) / 2;
  int x0 = kx + (kw - (cols * w + (cols - 1) * gap)) / 2;
  for (int i = 0; i < vvKeys; i++) {
    int c = i / rows, r = i % rows;                   // första kolumnen uppifrån och ner, sedan nästa
    vvRenderKey(g, fb, skm, i, x0 + c * (w + gap), y0 + r * (h + gap), w, h);
  }
}

// ---------------------------------------------------------------- Körs från loop

static void vtviewBegin() { vvPalInit(); }

static void vtviewLoop(uint32_t now) {
  // Vilket redskap? -> läs in dess pool (en gång per redskap och ny pool)
  if (rdInfo.addr >= 0 && rdInfo.nameKnown && (rdInfo.name != vvTriedName || vtPoolGen != vvTriedGen)) {
    vvTriedName = rdInfo.name; vvTriedGen = vtPoolGen;
    vvLoad(rdInfo.name);
  }
  vvAddr = (vvOk && rdInfo.addr >= 0 && rdInfo.name == vvName) ? rdInfo.addr : -1;
  // Kommandon från redskapet till terminalen. Medan en ny pool väntar på att sparas och läsas in
  // ligger de kvar i kön, så att skuren med startvärden direkt efter uppladdningen inte går förlorad.
  if (vtpReady) return;
  while (vtq && vtqTail != vtqHead) {
    VtCmd &c = vtq[vtqTail];
    if (vvOk && c.sa == vvAddr) { vvApply(c.d, c.len); vvCmdCount++; vvLastCmdMs = now; }
    vtqTail = (vtqTail + 1) % VTQ_N;
  }
}
