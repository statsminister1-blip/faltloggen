// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  traktor.h – det traktorn själv skickar ut på ISOBUS
//
//  Kontrollerat mot loggar från Fendt 720/828 Vario S4:
//    0xFE49 / 0xFE48  hastighet mark / hjul (mm/s), sträcka (mm, b2..5), riktning (b7&3: 1 = framåt)
//    0xF004           motorvarvtal (b3..4, 0,125 v/min per steg)
//    0xFE43 / 0xFE44  kraftuttag BAK / FRAM (b0..1, 0,125 v/min). Skickas bara när det går.
//    0xFE45 / 0xFE46  lyft BAK / FRAM (b0, 0,4 % per steg; b1 bit 6-7 = i arbete).
//    0xFE11..0xFE17   hydraulventil 1-7, uppskattat flöde (b0 ut, b1 in, 1 % per steg, -125 %)
//    0xFEF2           bränsleförbrukning (b0..1, 0,05 l/h)
//    0xFEE7           timräknare (b0..3, 0,05 h)
//    0x1F801 / 0x1F802  GPS-position (1e-7 grader) / kurs (1e-4 rad) – från styrsystemet (adress 0x8C)
//  Från v36 (bussinventeringen 2026-10-21, alla 97 loggar):
//    0xFE45 b3..4     dragkraft i bakre lyften: 10 N per steg, -320 000 N (sett -36..+45 kN)
//    0x1F119          attityd från styrsystemet (NMEA 2000 127257): b3..4 lutning fram/bak, b5..6 sidolutning,
//                     int16 × 0,0001 rad (sett -4..+5° resp. -9,4..+9,7°). Tecknen är INTE kontrollerade.
//    0xFE40           belysning (J1939/ISO 11783-7 "Lighting data"), 2 bitar per lampa, 01 = tänd.
//                     Bitarna är tolkade efter standarden och stämmer med loggarna (arbetsljus, positionsljus
//                     och bromsljus slår om som de ska), men är INTE provade lampa för lampa än.
//  Från v41:
//    slirning = (hjulfart 0xFE48 - markfart 0xFE49) / hjulfart, utjämnad över ca 3 s, bara vid hjulfart >= 1,5 km/h.
//    Loggarna (828 i arbete): 10-25 % i arbete, ca 2-3 % vid transport (markfarten från radarn/GPS:en).
//    Dragkraften i lyften följde inte slirningen alls (redskap i dragkroken syns inte i lyftens kraftgivare).
//  Övrigt som hörs (t.ex. 0xF022, 0xFE40/41, 0x1F119) visas också som rådata.
//  Allt är bara lyssning.
// ============================================================================
#pragma once

struct TrVal { float v; uint32_t ms; };

// Tolkning av 0xFE40 (byte, bitpar 0..3 räknat från minst signifikanta) -> lampa
static uint16_t trLjus(const uint8_t *d) {
  auto on = [&](int b, int par) { return ((d[b] >> (par * 2)) & 3) == 1; };
  uint16_t m = 0;
  // v37 (belysningsprovet, lampa för lampa): b0 par 0 ("running light") är 01 när INGA strålkastare är tända,
  // så positionsljuset tas bara från b3 par 3 (markeringsljus), som är 01 när strålkastarna är på.
  // Helljus syntes som b0 = 0x3C (par 2 = 11) + b3 par 3 = 01 – inte b0 par 3 som standarden säger.
  if (on(3, 3)) m |= LJ_POS;                   // tractor marker light
  if (on(0, 2)) m |= LJ_HALV;                  // low-beam head light
  else if (on(3, 3)) m |= LJ_HEL;              // strålkastare på men inte halvljus = helljus (bara ett prov)
  if (on(5, 2) || on(5, 3)) m |= LJ_ARB_FRAM;  // front low / high mounted work lights
  if (on(4, 2) || on(4, 3)) m |= LJ_ARB_BAK;   // rear low / high mounted work lights
  if (on(5, 0) || on(5, 1)) m |= LJ_ARB_SIDA;  // side low / high mounted work lights
  if (on(2, 1) || on(2, 2) || on(2, 3)) m |= LJ_BROMS;   // center / right / left stop light
  if (on(1, 3)) m |= LJ_BLINK_V;               // left turn signal
  if (on(1, 2)) m |= LJ_BLINK_H;               // right turn signal
  if (on(1, 1)) m |= LJ_ROTOR;                 // rotating beacon
  if (on(2, 0)) m |= LJ_BACK;                  // back-up light
  return m;
}

struct TraktorData {
  TrVal kmhMark, kmhHjul, strackaM, motorRpm, ptoBak, ptoFram, lyftBak, lyftFram, bransle, timmar;
  TrVal dragKN, lutFram, lutSida;         // dragkraft bak (kN), lutning fram/bak och sidolutning (grader)
  TrVal slir;                             // v41: slirning (%), utjämnad
  uint16_t ljus; uint32_t ljusMs;         // belysning (LJ_*), när den hördes senast
  uint16_t gpsDag; uint32_t gpsSek10k, gpsTidMs;   // v37: GPS-tiden (0x1F805): dagar sedan 1970, 0,1 ms sedan midnatt UTC
  int   arbBak, arbFram;                  // lyft i arbete: 1/0, -1 okänt
  int   riktning;  uint32_t riktMs;       // 1 framåt, 0 bakåt, -1 okänt
  double lat, lon; uint32_t posMs;
  float kurs;      uint32_t kursMs;       // grader
  float ventUt[16], ventIn[16]; uint32_t ventMs[16];
};
static TraktorData trd;

// Övriga sändningar (för att se vad traktorn skickar)
#define TR_RAW 24
struct TrRaw { uint32_t pgn; uint8_t sa; uint8_t dlc; uint8_t d[8]; uint32_t ms; uint32_t n; };
static TrRaw trRaw[TR_RAW];
static int nTrRaw = 0;
static portMUX_TYPE trMux = portMUX_INITIALIZER_UNLOCKED;

static inline void trSet(TrVal &t, float v, uint32_t now) { t.v = v; t.ms = now; }
static inline uint16_t tr16(const uint8_t *d) { return d[0] | (d[1] << 8); }
static inline uint32_t tr32(const uint8_t *d) { return d[0] | (d[1] << 8) | ((uint32_t)d[2] << 16) | ((uint32_t)d[3] << 24); }

// v41: slirning ur hjul- och markfart. Anropas när någon av farterna kommit; båda måste vara färska.
#define TR_SLIR_MIN_KMH 1.5f
#define TR_SLIR_TAU_S   3.0f
static void trSlir(uint32_t now) {
  const TrVal &h = trd.kmhHjul, &m = trd.kmhMark;
  if (!h.ms || !m.ms || now - h.ms > 1000 || now - m.ms > 1000) return;
  if (h.v < TR_SLIR_MIN_KMH) return;
  float s = (h.v - m.v) / h.v * 100.0f;
  if (s < -30.0f) s = -30.0f; else if (s > 100.0f) s = 100.0f;   // svängar och låg fart ger enstaka vilda värden
  if (!trd.slir.ms || now - trd.slir.ms > 3000) { trSet(trd.slir, s, now); return; }
  float a = (now - trd.slir.ms) / 1000.0f / TR_SLIR_TAU_S;
  if (a > 1) a = 1;
  trSet(trd.slir, trd.slir.v + (s - trd.slir.v) * a, now);
}

static void traktorBegin() {
  memset(&trd, 0, sizeof(trd));
  trd.riktning = -1;
  trd.arbBak = trd.arbFram = -1;
}

// Anropas från CAN-tasken för alla meddelanden med 29-bitars id
static void traktorFrame(uint32_t pgn, uint8_t sa, const uint8_t *d, uint8_t dlc, uint32_t now, int implAddr) {
  bool known = true;
  switch (pgn) {
    case 0xFE49: case 0xFE48:
      if (dlc < 8) return;
      if (tr16(d) < 0xFB00) { trSet(pgn == 0xFE49 ? trd.kmhMark : trd.kmhHjul, tr16(d) * 0.0036f, now); trSlir(now); }
      if (pgn == 0xFE49 || !trd.strackaM.ms || now - trd.strackaM.ms > 2000) {
        uint32_t mm = tr32(d + 2);
        if (mm < 0xFB000000u) trSet(trd.strackaM, mm / 1000.0f, now);
      }
      if ((d[7] & 3) < 2) { trd.riktning = d[7] & 3; trd.riktMs = now; }
      break;
    case 0xF004:
      if (dlc >= 5 && tr16(d + 3) < 0xFB00) trSet(trd.motorRpm, tr16(d + 3) * 0.125f, now);
      break;
    case 0xFE44: case 0xFE43:
      if (dlc >= 2 && tr16(d) < 0xFB00) trSet(pgn == 0xFE43 ? trd.ptoBak : trd.ptoFram, tr16(d) * 0.125f, now);
      break;
    case 0xFE46: case 0xFE45:
      if (pgn == 0xFE45 && dlc >= 5 && tr16(d + 3) < 0xFB00)
        trSet(trd.dragKN, (tr16(d + 3) * 10.0f - 320000.0f) / 1000.0f, now);
      if (dlc >= 2 && d[0] <= 250) {
        int arb = (d[1] >> 6) & 3;
        if (pgn == 0xFE45) { trSet(trd.lyftBak, d[0] * 0.4f, now); if (arb < 2) trd.arbBak = arb; }
        else               { trSet(trd.lyftFram, d[0] * 0.4f, now); if (arb < 2) trd.arbFram = arb; }
      }
      break;
    case 0xFEF2:
      if (dlc >= 2 && tr16(d) < 0xFB00) trSet(trd.bransle, tr16(d) * 0.05f, now);
      break;
    case 0xFEE7:
      if (dlc >= 4 && tr32(d) < 0xFB000000u) trSet(trd.timmar, tr32(d) * 0.05f, now);
      break;
    case 0x1F801:
      if (dlc >= 8) {
        int32_t la = (int32_t)tr32(d), lo = (int32_t)tr32(d + 4);
        if (la != 0x7FFFFFFF && lo != 0x7FFFFFFF) { trd.lat = la * 1e-7; trd.lon = lo * 1e-7; trd.posMs = now; }
      }
      break;
    case 0x1F805:                                 // v37: GNSS-position (fast packet) – bara datum och tid används
      {
        static uint8_t sekv = 0xFF; static uint16_t dag; static uint32_t lag;
        if (dlc >= 8 && sa == 0x8C) {
          if ((d[0] & 0x1F) == 0 && d[1] >= 7) { sekv = d[0] >> 5; dag = d[3] | (d[4] << 8); lag = d[5] | (d[6] << 8) | ((uint32_t)d[7] << 16); }
          else if ((d[0] & 0x1F) == 1 && (d[0] >> 5) == sekv) {
            trd.gpsDag = dag; trd.gpsSek10k = lag | ((uint32_t)d[1] << 24); trd.gpsTidMs = now; sekv = 0xFF;
          }
        }
      }
      known = false;
      break;
    case 0x1F802:
      if (dlc >= 6 && tr16(d + 2) != 0xFFFF) { trd.kurs = tr16(d + 2) * 0.0001f * 57.29578f; trd.kursMs = now; }
      break;
    case 0x1F119:                                 // attityd (visas också som rådata)
      if (dlc >= 7) {
        int16_t p = (int16_t)tr16(d + 3), r = (int16_t)tr16(d + 5);
        if (p != 0x7FFF) trSet(trd.lutFram, p * 0.0001f * 57.29578f, now);
        if (r != 0x7FFF) trSet(trd.lutSida, r * 0.0001f * 57.29578f, now);
      }
      known = false;
      break;
    case 0xFE40:                                  // belysning (visas också som rådata)
      if (dlc >= 6 && sa == 0xF0) { trd.ljus = trLjus(d); trd.ljusMs = now; }
      known = false;
      break;
    default:
      if (pgn >= 0xFE10 && pgn <= 0xFE1F) {
        int n = pgn - 0xFE10;
        if (dlc >= 2 && d[0] <= 250 && d[1] <= 250) {
          trd.ventUt[n] = d[0] - 125.0f; trd.ventIn[n] = d[1] - 125.0f; trd.ventMs[n] = now;
        }
      } else known = false;
  }
  if (known) return;
  // Övriga sändningar (PDU2) från andra än redskapet: spara senaste innehållet
  if ((pgn & 0xFF00) < 0xF000 || sa == implAddr) return;
  portENTER_CRITICAL(&trMux);
  int i;
  for (i = 0; i < nTrRaw; i++) if (trRaw[i].pgn == pgn && trRaw[i].sa == sa) break;
  if (i == nTrRaw) {
    if (nTrRaw < TR_RAW) nTrRaw++;
    else {
      int old = 0;
      for (int k = 1; k < TR_RAW; k++) if (trRaw[k].ms < trRaw[old].ms) old = k;
      i = old;
    }
    trRaw[i].pgn = pgn; trRaw[i].sa = sa; trRaw[i].n = 0;
  }
  trRaw[i].dlc = dlc > 8 ? 8 : dlc;
  memcpy(trRaw[i].d, d, trRaw[i].dlc);
  trRaw[i].ms = now; trRaw[i].n++;
  portEXIT_CRITICAL(&trMux);
}

// ---------------------------------------------------------------- Till skärmen
static float trGet(const TrVal &t, uint32_t now, uint32_t maxAge = 5000) {
  return (t.ms && now - t.ms < maxAge) ? t.v : NAN;
}

static void traktorFill(UiState &u, uint32_t now) {
  TrView &v = u.trv;
  v.kmh = trGet(trd.kmhMark, now);
  if (isnan(v.kmh)) v.kmh = trGet(trd.kmhHjul, now);
  v.rpm = trGet(trd.motorRpm, now);
  // Kraftuttagen skickas bara när de går: hörts förut men inte nu = av (0)
  v.ptoR = trGet(trd.ptoBak, now, 3000);
  if (isnan(v.ptoR) && trd.ptoBak.ms) v.ptoR = 0;
  v.ptoF = trGet(trd.ptoFram, now, 3000);
  if (isnan(v.ptoF) && trd.ptoFram.ms) v.ptoF = 0;
  v.liftR = trGet(trd.lyftBak, now);
  v.liftF = trGet(trd.lyftFram, now);
  v.workR = isnan(v.liftR) ? -1 : trd.arbBak;
  v.dragKN = trGet(trd.dragKN, now);
  v.slir = trGet(trd.slir, now, 1500);
  v.lutFram = trGet(trd.lutFram, now);
  v.lutSida = trGet(trd.lutSida, now);
  v.ljusOk = trd.ljusMs && now - trd.ljusMs < 5000;
  v.ljus = v.ljusOk ? trd.ljus : 0;
  v.workF = isnan(v.liftF) ? -1 : trd.arbFram;
  v.fuel = trGet(trd.bransle, now);
  v.hours = trGet(trd.timmar, now, 30000);
  v.km = trGet(trd.strackaM, now);
  if (!isnan(v.km)) v.km /= 1000.0f;
  v.dir = (trd.riktMs && now - trd.riktMs < 5000) ? trd.riktning : -1;
  v.gps = trd.posMs != 0;
  v.gpsFresh = trd.posMs && now - trd.posMs < 5000;
  v.lat = trd.lat; v.lon = trd.lon;
  v.cog = (trd.kursMs && now - trd.kursMs < 5000) ? trd.kurs : NAN;
  // Ventiler som hörts senaste 10 s (+ = ut, - = in)
  v.nValve = 0;
  for (int n = 0; n < 16 && v.nValve < TR_VALVES; n++) {
    if (!trd.ventMs[n] || now - trd.ventMs[n] > 10000) continue;
    float ut = trd.ventUt[n], in = trd.ventIn[n];
    v.valveNo[v.nValve] = n;
    v.valveFlow[v.nValve] = (ut > 0.5f && ut >= in) ? ut : (in > 0.5f ? -in : 0);
    v.nValve++;
  }
  // Övriga meddelanden, sorterade på PGN
  TrRaw r2[TR_RAW]; int n2;
  portENTER_CRITICAL(&trMux);
  n2 = nTrRaw; memcpy(r2, trRaw, sizeof(TrRaw) * n2);
  portEXIT_CRITICAL(&trMux);
  for (int i = 1; i < n2; i++) { TrRaw t = r2[i]; int j = i - 1; while (j >= 0 && r2[j].pgn > t.pgn) { r2[j + 1] = r2[j]; j--; } r2[j + 1] = t; }
  u.nTrRaw = 0;
  for (int i = 0; i < n2 && u.nTrRaw < TR_UI_ROWS; i++) {
    if (now - r2[i].ms > 10000) continue;
    char hex[28] = ""; int o = 0;
    for (int k = 0; k < r2[i].dlc; k++) o += snprintf(hex + o, sizeof(hex) - o, "%02X ", r2[i].d[k]);
    snprintf(u.trRaw[u.nTrRaw++], 64, L("%05lX  från %3u   %s", "%05lX  from %3u   %s"), (unsigned long)r2[i].pgn, r2[i].sa, hex);
  }
}
