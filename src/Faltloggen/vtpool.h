// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  vtpool.h – fångar redskapets egen bild (VT-objektpoolen)
//
//  När ett ISOBUS-redskap ansluter till terminalen (VT) laddar det upp en
//  "objektpool": alla masker, knappar, texter, bilder och fält som redskapet
//  visar. Skärmen lyssnar på överföringen (TP/ETP med PGN 0xE700 och första
//  byten 0x11) och sparar hela poolen på SD-kortet när redskapet skickar "slut
//  på poolen" (0x12): /VTPOOL/<namn>_<crc>.BIN (rå pool, utan 0x11-bytarna).
//  vtview.h använder den sedan för att visa redskapets bild (TERMINAL).
//
//  Laddar redskapet i stället en sparad version ur terminalens minne (0xD1)
//  skickas ingen pool. Radera då redskapets sparade pool i terminalen och
//  starta om, så skickas den på nytt.
//
//  Redskapets bild är tillverkarens – den stannar på ditt SD-kort.
//  Allt är bara lyssning – inget skickas på bussen.
// ============================================================================
#pragma once
#include <SD.h>
#include <Preferences.h>
#include "esp_heap_caps.h"
#include "esp_rom_crc.h"

#define VTP_MAX  (2 * 1024 * 1024)            // största pool som sparas
#define VTP_MIN  (256 * 1024)                 // minsta buffert som tas

// Bufferten tas bara medan en pool tas emot och lämnas tillbaka när den sparats,
// så att kartans rutor m.m. inte får mindre minne i vanliga fall.
static uint8_t *vtpBuf = nullptr;             // poolen utan 0x11-bytarna (PSRAM)
static uint32_t vtpCap = 0;                   // buffertens storlek
static uint8_t *vtpBits = nullptr;            // vilka TP-paket som kommit i pågående överföring
static uint8_t  vtAddr[256];                  // 1 = adressen är en terminal (VT)

// Pågående överföring till terminalen (en åt gången)
struct VtpSess {
  bool     on, etp, pool, firstSeen, small;
  uint8_t  sa, da;
  uint32_t size, nPkt, got, base, etpOff, ms;
};
static VtpSess vs;

// Poolen som byggs upp (kan komma i flera delar)
static volatile uint32_t vtpLen = 0;
static volatile int      vtpSa = -1, vtpDa = -1;
static volatile uint16_t vtpParts = 0;
static volatile bool     vtpBad = false;      // en del kom inte fram hel
static volatile bool     vtpReady = false;    // hel pool, väntar på att sparas (CAN-tasken rör den inte)
static volatile uint32_t vtpReadyMs = 0;
static volatile uint32_t vtpMemReq = 0;       // minne som redskapet begärde (0xC0)

// Det terminalen berättar (svar till redskapet)
struct VtInfo {
  int      vtVer;                             // VT-version (svar på 0xC0)
  int      maskX, maskY;                      // datamaskens storlek i bildpunkter (0xC7)
  int      graf;                              // 0 = svartvit, 1 = 16 färger, 2 = 256 färger
  int      skX, skY, skVirt, skFys;           // mjukknappar (0xC2)
  int      eopFel;                            // svar på slut på poolen: -1 okänt, 0 OK, annars fel
  uint8_t  eopRaw[8];
  char     laddad[8];                         // versionsnamn som laddades ur terminalens minne
  int      laddadOk;                          // -1 okänt, 1 laddad, 0 fanns inte
  uint32_t laddadMs;                          // när den laddades
};
static VtInfo vtInfo;

// Händelselogg (skrivs av CAN-tasken, läses av loop)
#define VTE_N 16
static char     vtEv[VTE_N][72];
static volatile uint32_t vtEvN = 0;

static void vtEvent(const char *fmt, ...) {
  char *b = vtEv[vtEvN % VTE_N];
  int o = snprintf(b, 72, "%lu s: ", (unsigned long)(millis() / 1000));
  va_list ap; va_start(ap, fmt); vsnprintf(b + o, 72 - o, fmt, ap); va_end(ap);
  vtEvN++;
}

// Kommandon från redskapet till terminalen (nytt värde, visa/dölj ...) -> vtview.h
// En skrivare (CAN-tasken) och en läsare (loop), utan lås.
#define VTQ_N 512                              // räcker för skuren med startvärden direkt efter en ny pool
#define VTQ_D 64
struct VtCmd { uint8_t sa, len; uint8_t d[VTQ_D]; };
static VtCmd *vtq = nullptr;                  // VTQ_N poster i PSRAM (sparar internminne)
static volatile uint16_t vtqHead = 0, vtqTail = 0;
static volatile uint32_t vtqDrop = 0;
static void vtqPush(uint8_t sa, const uint8_t *d, int n) {
  if (!vtq) return;
  uint16_t h = vtqHead, nh = (h + 1) % VTQ_N;
  if (nh == vtqTail) { vtqDrop++; return; }
  if (n > VTQ_D) n = VTQ_D;
  vtq[h].sa = sa; vtq[h].len = n; memcpy(vtq[h].d, d, n);
  __sync_synchronize();
  vtqHead = nh;
}
// Korta meddelanden via transportprotokollet (t.ex. ny text, ny position)
#define VTS_MAX 256
static uint8_t vtsBuf[VTS_MAX];
static volatile uint32_t vtPoolGen = 0;       // ökas när en ny pool sparats (vtview.h läser in den)

// Senaste sparade pool
static Preferences vtPrefs;
static char  vtpSdPath[48]  = "";
static uint32_t vtpSavedLen = 0, vtpSavedCrc = 0;
static int      vtpSavedSa = -1;
static uint32_t vtpSavedMs = 0;              // när poolen senast fångades
static char     vtpState[80] = "inväntar";

// ---------------------------------------------------------------- Uppstart
static void vtpoolBegin() {
  memset(vtAddr, 0, sizeof(vtAddr));
  memset(&vtInfo, 0, sizeof(vtInfo));
  vtInfo.vtVer = vtInfo.graf = vtInfo.eopFel = vtInfo.laddadOk = -1;
  vtpBits = (uint8_t *)heap_caps_malloc(VTP_MAX / 7 / 8 + 64, MALLOC_CAP_SPIRAM);
  vtq = (VtCmd *)heap_caps_malloc(sizeof(VtCmd) * VTQ_N, MALLOC_CAP_SPIRAM);
  if (!vtpBits) {
    snprintf(vtpState, sizeof(vtpState), "inget minne för poolen");
    Serial.println("vtpool: inget minne");
  }
  vtPrefs.begin("vtpool", false);
  vtpSavedCrc = vtPrefs.getUInt("crc", 0);
}

static inline bool vtIsVt(uint8_t a) { return vtAddr[a] != 0; }

// Se till att bufferten rymmer need byte (behåller innehållet). Från CAN-tasken.
static bool vtpEnsure(uint32_t need) {
  if (need > VTP_MAX) return false;
  if (vtpBuf && vtpCap >= need) return true;
  uint32_t cap = need < VTP_MIN ? VTP_MIN : need;
  uint8_t *b = (uint8_t *)heap_caps_realloc(vtpBuf, cap, MALLOC_CAP_SPIRAM);
  if (!b) return false;
  vtpBuf = b; vtpCap = cap;
  return true;
}

// ---------------------------------------------------------------- Från CAN-tasken
static void vtpoolFrame(uint32_t pgn, uint8_t sa, uint8_t da, const uint8_t *d, uint8_t dlc, uint32_t now) {
  if (dlc < 8 || sa >= 254) return;
  switch (pgn) {
    case 0xE600: {                    // terminal -> redskap
      vtAddr[sa] = 1;
      uint8_t f = d[0];
      if (f == 0xFE) return;          // terminalens status varje sekund
      if (f == 0xC0) {                // svar: minne
        vtInfo.vtVer = d[1];
        vtEvent("VT 0x%02X svarar minne: VT-version %d, %s", sa, d[1], d[2] == 0 ? "räcker" : "räcker inte");
      } else if (f == 0xC2) {         // svar: mjukknappar
        vtInfo.skX = d[4]; vtInfo.skY = d[5]; vtInfo.skVirt = d[6]; vtInfo.skFys = d[7];
        vtEvent("mjukknappar %dx%d px, %d virtuella, %d fysiska", d[4], d[5], d[6], d[7]);
      } else if (f == 0xC7) {         // svar: hårdvara
        vtInfo.graf = d[2];
        vtInfo.maskX = d[4] | (d[5] << 8); vtInfo.maskY = d[6] | (d[7] << 8);
        vtEvent("datamask %dx%d px, grafik %d", vtInfo.maskX, vtInfo.maskY, d[2]);
      } else if (f == 0x12) {         // svar: slut på poolen
        vtInfo.eopFel = d[1];
        memcpy(vtInfo.eopRaw, d, 8);
        vtEvent("VT: poolen %s (fel %02X, objekt %u, poolfel %02X)", d[1] == 0 ? "godkänd" : "FEL",
                d[1], (unsigned)(d[4] | (d[5] << 8)), d[6]);
      } else if (f == 0xD1) {         // svar: ladda version
        bool ok = (d[5] & 0x0F) == 0;
        vtInfo.laddadOk = ok ? 1 : 0;
        vtInfo.laddadMs = now ? now : 1;
        vtEvent("VT: sparad version %s (fel %02X)", ok ? "laddad ur minnet" : "fanns inte", d[5]);
      } else if (f == 0xD0) {
        vtEvent("VT: version sparad (fel %02X)", d[5]);
      } else if (f == 0xE0) {
        vtEvent("VT: versioner (TP)");
      }
      return;
    }

    case 0xE700: {                    // redskap -> terminal
      if (da < 254) vtAddr[da] = 1;
      uint8_t f = d[0];
      if (f == 0xC0) {                // begär minne = början på en ny pool
        uint32_t n = d[2] | (d[3] << 8) | ((uint32_t)d[4] << 16) | ((uint32_t)d[5] << 24);
        vtpMemReq = n;
        if (!vtpReady && (vtpSa == sa || vtpSa < 0 || vtpLen == 0)) { vtpLen = 0; vtpParts = 0; vtpBad = false; vtpSa = sa; }
        vtEvent("0x%02X begär minne för %lu byte", sa, (unsigned long)n);
      } else if (f == 0xD1) {
        char l[8]; for (int i = 0; i < 7; i++) l[i] = (d[1 + i] >= 32 && d[1 + i] < 127) ? d[1 + i] : '.';
        l[7] = 0;
        memcpy(vtInfo.laddad, l, 8);
        vtEvent("0x%02X laddar sparad version '%s'", sa, l);
      } else if (f == 0xD0) {
        char l[8]; for (int i = 0; i < 7; i++) l[i] = (d[1 + i] >= 32 && d[1 + i] < 127) ? d[1 + i] : '.';
        l[7] = 0;
        vtEvent("0x%02X sparar version '%s'", sa, l);
      } else if (f == 0xC2 || f == 0xC3 || f == 0xC7 || f == 0xDF) {
        vtEvent("0x%02X frågar terminalen (0x%02X)", sa, f);
      } else if (f >= 0xA0 && f <= 0xBF) {   // kommando (nytt värde, visa/dölj ...) -> speglingen
        vtqPush(sa, d, 8);
      } else if (f == 0x11) {
        vtEvent("0x%02X: kort pooldel i en ram (sparas inte)", sa);
        vtpBad = true;
      } else if (f == 0x12) {         // slut på poolen
        vtEvent("0x%02X: slut på poolen, %lu byte i %u delar%s", sa, (unsigned long)vtpLen, vtpParts,
                vtpBad ? " (ofullständig)" : "");
        if (!vtpReady && sa == vtpSa && vtpLen > 0 && !vtpBad) {
          vtpDa = da;
          vtpReadyMs = now;
          vtpReady = true;
        } else if (vtpBad) {
          vtpLen = 0; vtpParts = 0; vtpBad = false;
        }
      }
      return;
    }

    case 0xEC00: case 0xC800: {       // transportprotokoll: styrmeddelanden
      bool etp = (pgn == 0xC800);
      uint32_t tpPgn = d[5] | (d[6] << 8) | ((uint32_t)d[7] << 16);
      uint8_t c = d[0];
      if ((!etp && c == 0x10) || (etp && c == 0x14)) {            // RTS
        if (tpPgn != 0xE700) return;
        if (da < 254) vtAddr[da] = 1;
        if (!vtpBits) return;
        uint32_t size = etp ? (d[1] | (d[2] << 8) | ((uint32_t)d[3] << 16) | ((uint32_t)d[4] << 24))
                            : (uint32_t)(d[1] | (d[2] << 8));
        vs.on = true; vs.etp = etp; vs.pool = false; vs.firstSeen = false; vs.small = false;
        vs.sa = sa; vs.da = da; vs.size = size; vs.nPkt = (size + 6) / 7;
        vs.got = 0; vs.base = 0; vs.etpOff = 0; vs.ms = now;
        if (vs.nPkt > VTP_MAX / 7) vs.nPkt = VTP_MAX / 7;
        memset(vtpBits, 0, vs.nPkt / 8 + 1);
        return;
      }
      if (!vs.on) return;
      if (etp && c == 0x16 && sa == vs.sa && da == vs.da) {       // ETP: dataoffset
        vs.etpOff = d[2] | (d[3] << 8) | ((uint32_t)d[4] << 16);
      } else if ((c == 0x13 || c == 0x17) && sa == vs.da && da == vs.sa) {   // klart
        if (vs.small && vs.got == vs.nPkt) vtqPush(vs.sa, vtsBuf, vs.size);
        if (vs.pool) {
          if (vs.got == vs.nPkt) {
            vtpLen = vs.base + vs.size - 1;
            vtpParts++;
          } else {
            vtpBad = true;
            vtEvent("pooldel ofullständig: %lu av %lu paket", (unsigned long)vs.got, (unsigned long)vs.nPkt);
          }
        }
        vs.on = false;
      } else if (c == 0xFF && ((sa == vs.sa && da == vs.da) || (sa == vs.da && da == vs.sa))) {
        if (vs.pool) { vtpBad = true; vtEvent("pooldel avbruten (orsak %u)", d[1]); }
        vs.on = false;
      }
      return;
    }

    case 0xEB00: case 0xC700: {       // transportprotokoll: data
      if (!vs.on || sa != vs.sa || da != vs.da) return;
      if ((pgn == 0xC700) != vs.etp) return;
      if (d[0] == 0) return;
      uint32_t pk = (vs.etp ? vs.etpOff : 0) + d[0] - 1;          // paketnummer från 0
      if (pk >= vs.nPkt) return;
      vs.ms = now;
      if (!vs.firstSeen) {
        if (pk != 0) { vs.on = false; return; }                   // missade början
        vs.firstSeen = true;
        if (d[1] != 0x11) {                                       // annat än poolen
          if (d[1] >= 0xA0 && d[1] <= 0xBF && vs.size <= VTS_MAX) {
            vs.small = true;                                      // kommando -> speglingen
          } else {
            if (d[1] < 0xA0 || d[1] > 0xBF) vtEvent("0x%02X -> VT: meddelande 0x%02X, %lu byte", sa, d[1], (unsigned long)vs.size);
            vs.on = false;
            return;
          }
        }
        if (!vs.small) {
          if (vtpReady) { vs.on = false; return; }                // förra poolen inte sparad än
          if (sa != vtpSa) { vtpSa = sa; vtpLen = 0; vtpParts = 0; vtpBad = false; }
          uint32_t need = vtpLen + vs.size;
          if (vtpMemReq > need && vtpMemReq <= VTP_MAX) need = vtpMemReq;   // hela poolen på en gång
          if (!vtpEnsure(need)) {
            vtEvent("inget minne för poolen (%lu byte)", (unsigned long)(vtpLen + vs.size));
            vtpBad = true; vs.on = false; return;
          }
          vs.pool = true;
          vs.base = vtpLen;
          if (vtpParts == 0) vtEvent("0x%02X laddar upp poolen till 0x%02X", sa, da);
        }
      }
      if (vs.small) {
        if (!(vtpBits[pk >> 3] & (1 << (pk & 7)))) { vtpBits[pk >> 3] |= 1 << (pk & 7); vs.got++; }
        for (int i = 0; i < 7; i++) { uint32_t q = pk * 7 + i; if (q < vs.size) vtsBuf[q] = d[1 + i]; }
        return;
      }
      if (!vs.pool) return;
      if (!(vtpBits[pk >> 3] & (1 << (pk & 7)))) { vtpBits[pk >> 3] |= 1 << (pk & 7); vs.got++; }
      uint32_t pos = pk * 7;
      for (int i = 0; i < 7; i++) {
        uint32_t p = pos + i;
        if (p == 0 || p >= vs.size) continue;                     // byte 0 = 0x11
        vtpBuf[vs.base + p - 1] = d[1 + i];
      }
      return;
    }
  }
}

// ---------------------------------------------------------------- Spara och sammanfatta
static int32_t vvObjLen(const uint8_t *d, uint32_t p, uint32_t n);   // i vtview.h

// Hel pool (har ett Working Set-objekt) eller bara ett tillägg? När redskapet laddar sin bild ur
// terminalens minne skickar det ibland bara ett tillägg (t.ex. texterna på valt språk).
static bool vtpHasWs(const uint8_t *b, uint32_t len) {
  for (uint32_t p = 0; p < len;) {
    int32_t l = vvObjLen(b, p, len);
    if (l <= 0) return false;
    if (b[p + 2] == 0) return true;
    p += l;
  }
  return false;
}

static void vtpName(char *out, int n, int sa) {
  if (sa >= 0 && sa < 254 && busDev[sa].known)
    snprintf(out, n, "%08lX%08lX", (unsigned long)(busDev[sa].name >> 32), (unsigned long)(busDev[sa].name & 0xFFFFFFFF));
  else
    snprintf(out, n, "ADR%02X", sa & 0xFF);
}

static void vtpSave() {
  uint32_t len = vtpLen;
  int sa = vtpSa, da = vtpDa;
  uint32_t crc = esp_rom_crc32_le(0, vtpBuf, len);
  char namn[24];
  vtpName(namn, sizeof(namn), sa);
  vtpSavedLen = len; vtpSavedSa = sa; vtpSavedMs = millis();
  bool dup = (crc == vtpSavedCrc);
  snprintf(vtpSdPath, sizeof(vtpSdPath), "/VTPOOL/%s_%08lX.BIN", namn, (unsigned long)crc);
  bool ok = false;
  if (sdOk) {
    sdLock();
    if (!SD.exists("/VTPOOL")) SD.mkdir("/VTPOOL");
    File f = SD.open(vtpSdPath, FILE_WRITE);
    sdUnlock();
    if (f) {
      ok = true;
      for (uint32_t p = 0; p < len && ok; p += 4096) {
        uint32_t k = min((uint32_t)4096, len - p);
        sdLock();
        ok = f.write(vtpBuf + p, k) == k;
        sdUnlock();
      }
      sdLock(); f.close(); sdUnlock();
    }
  }
  Serial.printf("vtpool: pool från 0x%02X, %lu byte, CRC %08lX, SD %s%s\n", sa, (unsigned long)len,
                (unsigned long)crc, ok ? "OK" : "FEL", dup ? " (samma som förut)" : "");
  bool full = vtpHasWs(vtpBuf, len);
  vtEvent("%s", full ? "hel pool" : "tillägg till poolen (ingen Working Set)");
  if (ok) {                                     // senaste pool/tillägg för redskapet + terminalens mått (vtview.h)
    uint64_t nm = (sa >= 0 && sa < 254 && busDev[sa].known) ? busDev[sa].name : 0;
    if (nm) {
      char k[12];
      // nyckeln är NAME:s låga 32 bitar (tillverkare + identitet), som inte ändras om redskapet byter klass
      snprintf(k, sizeof(k), "%c%08lX", full ? 'n' : 'a', (unsigned long)(nm & 0xFFFFFFFF)); vtPrefs.putUInt(k, crc);
      if (full) { snprintf(k, sizeof(k), "a%08lX", (unsigned long)(nm & 0xFFFFFFFF)); vtPrefs.remove(k); }
      if (vtInfo.maskX > 0) {
        snprintf(k, sizeof(k), "m%08lX", (unsigned long)(nm & 0xFFFFFFFF));
        vtPrefs.putUInt(k, (uint32_t)(vtInfo.maskX & 0xFFFF) | ((uint32_t)(vtInfo.skX & 0xFF) << 16) | ((uint32_t)(vtInfo.skY & 0xFF) << 24));
      }
      if (vtInfo.skFys > 0) { snprintf(k, sizeof(k), "k%08lX", (unsigned long)(nm & 0xFFFFFFFF)); vtPrefs.putUInt(k, vtInfo.skFys); }
    }
    vtPoolGen++;
  }
  if (ok && !dup) {
    vtpSavedCrc = crc;
    vtPrefs.putUInt("crc", crc);
    snprintf(vtpState, sizeof(vtpState), "fångad %lu byte (CRC %08lX)", (unsigned long)len, (unsigned long)crc);
  } else if (ok) {
    snprintf(vtpState, sizeof(vtpState), "fångad %lu byte (CRC %08lX), samma som förut", (unsigned long)len, (unsigned long)crc);
  } else {
    snprintf(vtpState, sizeof(vtpState), "fångad %lu byte men kunde inte sparas på SD", (unsigned long)len);
  }
}

// ---------------------------------------------------------------- Körs från loop
static void vtpoolLoop(uint32_t now) {
  // Spara poolen när terminalen hunnit svara på "slut på poolen"
  if (vtpReady && now - vtpReadyMs > 3000) {
    vtpSave();
    vtpLen = 0; vtpParts = 0; vtpBad = false;
    uint8_t *b = vtpBuf;                        // lämna tillbaka minnet
    vtpBuf = nullptr; vtpCap = 0;
    free(b);
    vtpReady = false;
  }
  if (vs.on && now - vs.ms > 5000) {            // överföring som aldrig blev klar
    if (vs.pool) { vtpBad = true; vtEvent("pooldel tog slut efter %lu av %lu paket", (unsigned long)vs.got, (unsigned long)vs.nPkt); }
    vs.on = false;
  }
}
