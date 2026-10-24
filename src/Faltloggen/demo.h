// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  demo.h – demoläge med simulerade värden
//
//  Slås på i inställningarna (DEMO). Skapar påhittade ISOBUS-meddelanden från en
//  "traktor" (0xF0), en Task Controller (0xF7) och ett påhittat redskap, en
//  "Demo-spruta" (0x82), och matar in dem i programmet som om de kom från bussen.
//  Så kan alla sidor visas utan att något är inkopplat.
//   - Inget av det simulerade sparas.
//   - Demoläget stängs av direkt om riktig ISOBUS-trafik hörs, och vid omstart.
//  Redskapets beskrivning (DDOP) nedan är påhittad (tillverkarkod 0, enhetsklass 6).
//  Positionen är försöksfälten vid Ultuna (SLU) utanför Uppsala.
// ============================================================================
#pragma once

static volatile bool demoOn = false;

static const uint8_t DEMO_DDOP[] = {
  0x61, 0x44, 0x56, 0x43, 0x00, 0x00, 0x0B, 0x44, 0x65, 0x6D, 0x6F, 0x2D, 0x73, 0x70, 0x72, 0x75, 0x74, 0x61, 0x03, 0x31,
  0x2E, 0x30, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0xA0, 0x09, 0x44, 0x45, 0x4D, 0x4F, 0x2D, 0x30, 0x30, 0x30, 0x31,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x65, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x45, 0x54, 0x01, 0x00, 0x01,
  0x0B, 0x44, 0x65, 0x6D, 0x6F, 0x2D, 0x73, 0x70, 0x72, 0x75, 0x74, 0x61, 0x00, 0x00, 0xFF, 0xFF, 0x01, 0x00, 0x0A, 0x00,
  0x44, 0x45, 0x54, 0x02, 0x00, 0x02, 0x03, 0x42, 0x6F, 0x6D, 0x01, 0x00, 0x01, 0x00, 0x04, 0x00, 0x0B, 0x00, 0x0C, 0x00,
  0x0D, 0x00, 0x0E, 0x00, 0x44, 0x50, 0x44, 0x0A, 0x00, 0x50, 0x00, 0x01, 0x09, 0x0B, 0x54, 0x6F, 0x74, 0x61, 0x6C, 0x20,
  0x76, 0x6F, 0x6C, 0x79, 0x6D, 0x14, 0x00, 0x44, 0x50, 0x44, 0x0B, 0x00, 0x8D, 0x00, 0x01, 0x09, 0x0A, 0x41, 0x72, 0x62,
  0x65, 0x74, 0x73, 0x6C, 0xE4, 0x67, 0x65, 0xFF, 0xFF, 0x44, 0x50, 0x44, 0x0C, 0x00, 0x43, 0x00, 0x01, 0x09, 0x0B, 0x41,
  0x72, 0x62, 0x65, 0x74, 0x73, 0x62, 0x72, 0x65, 0x64, 0x64, 0x15, 0x00, 0x44, 0x50, 0x44, 0x0D, 0x00, 0x02, 0x00, 0x01,
  0x09, 0x04, 0x47, 0x69, 0x76, 0x61, 0x16, 0x00, 0x44, 0x50, 0x44, 0x0E, 0x00, 0x01, 0x00, 0x01, 0x09, 0x08, 0x42, 0xF6,
  0x72, 0x76, 0xE4, 0x72, 0x64, 0x65, 0x16, 0x00, 0x44, 0x56, 0x50, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x83,
  0x3A, 0x00, 0x01, 0x6C, 0x44, 0x56, 0x50, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x12, 0x83, 0x3A, 0x02, 0x01, 0x6D,
  0x44, 0x56, 0x50, 0x16, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0A, 0xD7, 0x23, 0x3C, 0x01, 0x04, 0x6C, 0x2F, 0x68, 0x61,
};

static uint32_t demoStartMs = 0, demoLastMs = 0, demoTick = 0;
static double demoDistMm = 0, demoLiter = 0, demoLat = 59.8160, demoLon = 17.6560;

static void handleFrame(const twai_message_t &m, bool demo);   // i Faltloggen.ino

static void demoSend(uint32_t prio, uint32_t pgn, uint8_t da, uint8_t sa, const uint8_t *d, uint8_t dlc = 8) {
  twai_message_t m = {};
  m.extd = 1;
  uint32_t p = ((pgn >> 8) & 0xFF) < 240 ? (pgn | da) : pgn;
  m.identifier = (prio << 26) | (p << 8) | sa;
  m.data_length_code = dlc;
  memcpy(m.data, d, dlc);
  handleFrame(m, true);
}

static void demoPd(uint8_t sa, uint8_t da, uint16_t el, uint16_t ddi, int32_t v) {
  uint8_t d[8] = {(uint8_t)(0x03 | ((el & 0x0F) << 4)), (uint8_t)(el >> 4), (uint8_t)ddi, (uint8_t)(ddi >> 8),
                  (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)};
  demoSend(3, 0xCB00, da, sa, d);
}

static void demoClaim(uint8_t sa, uint64_t name) {
  uint8_t d[8];
  for (int i = 0; i < 8; i++) d[i] = (uint8_t)(name >> (8 * i));
  demoSend(6, 0xEE00, 0xFF, sa, d);
}

static uint64_t demoName(uint32_t ident, uint32_t tillv, uint32_t funk, uint32_t klass) {
  return (uint64_t)ident | ((uint64_t)tillv << 21) | ((uint64_t)funk << 40) | ((uint64_t)klass << 49) |
         (2ULL << 60) | (1ULL << 63);
}

static void demoStart() {
  demoOn = true;
  demoStartMs = demoLastMs = millis();
  demoTick = 0;
  demoDistMm = 0; demoLiter = 0;
}

// Körs från loop var 100:e ms när demoläget är på
static void demoLoop(uint32_t now) {
  if (!demoOn || now - demoLastMs < 100) return;
  float dt = (now - demoLastMs) / 1000.0f;
  demoLastMs = now;
  float t = (now - demoStartMs) / 1000.0f;
  uint32_t k = demoTick++;
  const uint8_t TR = 0xF0, TC = 0xF7, RS = 0x82;

  // Vändteg: 12 s av var 90:e
  bool vandteg = fmodf(t, 90.0f) > 78.0f;
  float kmh = vandteg ? 4.0f + 1.5f * sinf(t) : 7.5f + 0.6f * sinf(t / 5.0f);
  float lha = vandteg ? 0 : 200.0f + 6.0f * sinf(t / 7.0f);   // sprutans giva, l/ha

  // Adressanspråk vid start och var 10:e s, beskrivningen en gång
  if (k % 100 == 0) {
    demoClaim(TR, demoName(1616, 102, 134, 0));
    demoClaim(TC, demoName(20, 102, 130, 0));
    demoClaim(RS, demoName(1, 0, 0, 6));
  }
  if (k == 5 && !readyFlag && readyBuf && sizeof(DEMO_DDOP) <= DDOP_MAX) {
    memcpy(readyBuf, DEMO_DDOP, sizeof(DEMO_DDOP));
    readyLen = sizeof(DEMO_DDOP);
    readySa = RS;
    readyIsDemo = true;
    readyFlag = true;
  }

  // Traktorn: hastighet mark/hjul, sträcka, riktning
  demoDistMm += kmh / 3.6f * 1000.0f * dt;
  uint16_t mms = (uint16_t)(kmh / 3.6f * 1000.0f);
  uint32_t dist = (uint32_t)demoDistMm;
  uint8_t sp[8] = {(uint8_t)mms, (uint8_t)(mms >> 8), (uint8_t)dist, (uint8_t)(dist >> 8),
                   (uint8_t)(dist >> 16), (uint8_t)(dist >> 24), 0xFF, 0xFD};
  demoSend(3, 0xFE49, 0xFF, TR, sp);
  // v41: slirning ca 14 % i arbete (varierar), 3 % på vändtegen -> hjulfarten = markfarten / (1 - slirningen)
  float slir = vandteg ? 3.0f : 14.0f + 5.0f * sinf(t / 8.0f) + 2.0f * sinf(t * 1.7f);
  uint16_t mmsH = (uint16_t)(mms / (1.0f - slir / 100.0f));
  sp[0] = (uint8_t)mmsH; sp[1] = (uint8_t)(mmsH >> 8);
  demoSend(3, 0xFE48, 0xFF, TR, sp);

  // GPS: kör fram och tillbaka i en rak linje (öster/väster)
  double dirE = fmod(t, 180.0) < 90.0 ? 1.0 : -1.0;
  demoLon += dirE * (kmh / 3.6) * dt / (111320.0 * cos(demoLat * M_PI / 180.0));
  if (k % 2 == 0) {
    int32_t la = (int32_t)llround(demoLat * 1e7), lo = (int32_t)llround(demoLon * 1e7);
    uint8_t g[8] = {(uint8_t)la, (uint8_t)(la >> 8), (uint8_t)(la >> 16), (uint8_t)(la >> 24),
                    (uint8_t)lo, (uint8_t)(lo >> 8), (uint8_t)(lo >> 16), (uint8_t)(lo >> 24)};
    demoSend(2, 0x1F801, 0xFF, TR, g);
    uint16_t cog = (uint16_t)((dirE > 0 ? 90.0 : 270.0) * M_PI / 180.0 / 0.0001);
    uint16_t sog = (uint16_t)(kmh / 3.6f * 100.0f);
    uint8_t c[8] = {0, 0xFC, (uint8_t)cog, (uint8_t)(cog >> 8), (uint8_t)sog, (uint8_t)(sog >> 8), 0xFF, 0xFF};
    demoSend(2, 0x1F802, 0xFF, TR, c);
  }

  if (k % 5 == 0) {                         // 2 ggr/s
    // Motor, kraftuttag, lyft
    uint16_t rpm = (uint16_t)((1450.0f + 80.0f * sinf(t / 3.0f)) / 0.125f);
    uint8_t e[8] = {0xFF, 0xFF, 0xFF, (uint8_t)rpm, (uint8_t)(rpm >> 8), 0xFF, 0xFF, 0xFF};
    demoSend(3, 0xF004, 0xFF, TR, e);
    // Kraftuttag bak (0xFE43) går utom på vändtegen – som Fendt skickas det bara när det går
    if (!vandteg) {
      uint16_t pto = (uint16_t)((540.0f + 5.0f * sinf(t)) / 0.125f);
      uint8_t p[8] = {(uint8_t)pto, (uint8_t)(pto >> 8), 0xFF, 0xFF, 0x5F, 0xFF, 0xFF, 0xFF};
      demoSend(3, 0xFE43, 0xFF, TR, p);
    }
    // Lyft bak (0xFE45) i arbete, upp på vändtegen. Främre lyft (0xFE46) rör sig lite.
    // Dragkraft (b3..4, 10 N/steg, -320 kN): ca 25 kN i arbete, lite tryck när redskapet är lyft.
    float kN = vandteg ? -3.0f : 25.0f + 8.0f * sinf(t / 6.0f) + 4.0f * sinf(t * 1.3f);
    uint16_t dr = (uint16_t)((kN * 1000.0f + 320000.0f) / 10.0f);
    uint8_t h[8] = {(uint8_t)((vandteg ? 92.0f : 28.0f + 2.0f * sinf(t / 2.0f)) / 0.4f), (uint8_t)(vandteg ? 0x3F : 0x7F),
                    0xFF, (uint8_t)dr, (uint8_t)(dr >> 8), 0xFF, 0xFF, 0xFF};
    demoSend(3, 0xFE45, 0xFF, TR, h);
    h[0] = (uint8_t)((vandteg ? 60.0f : 20.0f) / 0.4f); h[1] = 0x3F; h[3] = h[4] = 0xFF;
    demoSend(3, 0xFE46, 0xFF, TR, h);
    // Lutning från styrsystemet (0x1F119, 1e-4 rad): fram/bak och åt sidan
    int16_t pi = (int16_t)(3.0f * sinf(t / 20.0f) / 57.29578f / 0.0001f), ro = (int16_t)(5.0f * sinf(t / 13.0f) / 57.29578f / 0.0001f);
    uint8_t at[8] = {0, 0xFF, 0x7F, (uint8_t)pi, (uint8_t)(pi >> 8), (uint8_t)ro, (uint8_t)(ro >> 8), 0xFF};
    demoSend(2, 0x1F119, 0xFF, 0x8C, at);
    // Belysning (0xFE40): positionsljus, halvljus och arbetsljus bak; arbetsljus fram och blinkers på vändtegen
    uint8_t lj[8] = {0x11, 0x03, 0x03, 0x7F, 0xCF, 0x0F, 0xFF, 0xFF};
    lj[4] = 0x5F;                                         // arbetsljus bak (låga + höga)
    if (vandteg) { lj[5] = 0x5F; if ((k / 5) % 2) lj[1] = 0x43; }   // arbetsljus fram, blinkers vänster (blinkar)
    demoSend(6, 0xFE40, 0xFF, TR, lj);
    // Ventiler 1-4 (0xFE11..): ventil 2 kör ut på vändtegen, ventil 3 in
    for (int vn = 1; vn <= 4; vn++) {
      uint8_t v[8] = {125, 125, 0x30, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
      if (vandteg && vn == 2) v[0] = 125 + 60;
      if (vandteg && vn == 3) v[1] = 125 + 35;
      demoSend(3, 0xFE10 + vn, 0xFF, TR, v);
    }
    // Bränsle (0,05 l/h) och timmar (0,05 h)
    uint16_t fuel = (uint16_t)((vandteg ? 9.0f : 14.5f + sinf(t / 4.0f)) / 0.05f);
    uint8_t fb[8] = {(uint8_t)fuel, (uint8_t)(fuel >> 8), 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    demoSend(6, 0xFEF2, 0xFF, TR, fb);
    uint32_t hrs = 76648 + (uint32_t)(t / 180.0f);
    uint8_t hb[8] = {(uint8_t)hrs, (uint8_t)(hrs >> 8), (uint8_t)(hrs >> 16), (uint8_t)(hrs >> 24), 0xFF, 0xFF, 0xFF, 0xFF};
    demoSend(6, 0xFEE7, 0xFF, TR, hb);

    // Redskapet -> TC
    demoLiter += lha * (kmh * 24.0f / 10.0f) / 3600.0f * 0.5f;   // l per halvsekund (24 m bom)
    demoPd(RS, TC, 1, 2, (int32_t)(lha * 100.0f));
    demoPd(RS, TC, 1, 1, 20000);
    demoPd(RS, TC, 0, 80, (int32_t)(demoLiter * 1000.0));
    demoPd(RS, TC, 1, 141, vandteg ? 0 : 1);
  }
  if (k % 10 == 0) {                        // 1 ggr/s
    uint8_t s[8] = {0x0E, 0xFF, 0xFF, 0xFF, 0x01, 0xFF, 0xFF, 0xFF};
    demoSend(3, 0xCB00, 0xFF, TC, s);      // TC-status
    demoPd(RS, TC, 1, 67, 24000);
    demoPd(TR, TC, 1, 141, 0);              // traktorn svarar också TC (som i verkligheten)
  }
}
