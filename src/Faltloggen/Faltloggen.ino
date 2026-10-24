// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  Fältloggen – extra ISOBUS-skärm för traktorhytten
//  Waveshare ESP32-S3-Touch-LCD-7 (800 x 480, pekskärm, CAN)
//
//  Copyright (C) 2026 Fältloggen-projektet
//  Detta program är fri programvara: du kan sprida och/eller ändra det under villkoren i
//  GNU General Public License, version 3 eller senare, som publicerad av Free Software Foundation.
//  Programmet sprids i hopp om att det ska vara användbart, men UTAN NÅGON GARANTI, inte ens
//  underförstådd garanti om SÄLJBARHET eller LÄMPLIGHET FÖR ETT VISST ÄNDAMÅL. Se LICENSE.
//
//  Fältloggen är inte gjord, granskad, godkänd eller stödd av Fendt, AGCO eller någon annan
//  tillverkare. Varumärken tillhör sina ägare och nämns bara för att beskriva vad skärmen fungerar med.
//
//  Lyssnar (ENDAST lyssnar!) på traktorns ISOBUS och visar:
//   - traktorsida: hastighet, motorvarv, kraftuttag, lyft, ventiler, bränsle, slirning, dragkraft,
//     lutning och belysning (det traktorn skickar)
//   - karta: körd yta på satellitbild, areal, bränsle per hektar, start/stopp via ventil, kraftuttag,
//     lyft, redskapets arbetsläge eller knapp; fältrapport som PDF till telefonen
//   - bränsle- och kostnadsanalys de senaste 30 minuterna
//   - ISOBUS-redskapets värden (namn och enheter ur redskapets beskrivning, DDOP)
//   - TERMINAL: redskapets egen bild som på traktorns terminal (spegling, bara visning)
//  Dessutom: svenska och engelska, dag/natt, manual, enhetsinfo, demoläge, frivilliga uppdateringar
//  (alltid med fråga och ansvarstext), valfri inspelning av bussen på SD-kortet.
//
//  Filer:  hw.h (hårdvara), logger.h (SD), net.h (WiFi, uppdateringar), ui.h (skärm), lang.h (språk),
//          redskap.h (redskapet, DDOP), traktor.h (traktorns data), demo.h (demoläge), karta.h (karta),
//          vtpool.h (fångar redskapets bild), vtview.h (visar den), analys.h (bränsleanalys),
//          falt.h (fältrapport), manual_data.h (manualen), logo_data.h (loggan), version.h
// ============================================================================

#include <U8g2lib.h>              // endast för typsnitten
#include <Arduino_GFX_Library.h>
#include <Wire.h>
#include <Preferences.h>
#include "driver/twai.h"
#include "soc/usb_serial_jtag_reg.h"
#include "soc/soc.h"
#include "esp_ota_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "driver/temperature_sensor.h"   // chippets temperatur (enhetsinfo)
#include "lang.h"
#include "hw.h"
#include "logger.h"
#include "net.h"
#include "ui.h"
#include "redskap.h"
#include "traktor.h"
#include "demo.h"
#include "karta.h"
#include "vtpool.h"
#include "vtview.h"
#include "analys.h"
#include "falt.h"                 // fältrapport som PDF
#include "dubbel.h"               // två programvarianter på samma skärm (bara med fyra programplatser)

// ============================================================================
//  KONFIGURATION
// ============================================================================
// Standardvärde för manuell arbetsbredd (m)
#define WIDTH_DEFAULT_M   6.0f
// Viloläge efter så här lång tystnad på bussen
#define IDLE_AFTER_MS     10000UL
// Arbetsbredd från redskapet (DDI 67 = verklig arbetsbredd, 70 = maximal), mm
#define DDI_WIDTH_A       67
#define DDI_WIDTH_B       70
// ============================================================================

// ---------------------------------------------------------------- Skärm
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
  5, 3, 46, 7,
  1, 2, 42, 41, 40,
  39, 0, 45, 48, 47, 21,
  14, 38, 18, 17, 10,
  0, 8, 4, 8,
  0, 8, 4, 8,
  1, 13000000);   // 13 MHz (~32 bilder/s): lämnar mer PSRAM-bandbredd åt WiFi, annars glider bilden
Arduino_RGB_Display *lcd = new Arduino_RGB_Display(800, 480, rgbpanel, 0, true);
Arduino_Canvas *canvas = nullptr;   // bildbuffert i PSRAM (flimmerfritt)
Arduino_GFX *g = nullptr;

// ---------------------------------------------------------------- Ny version: godkänn först när den visat att den fungerar
// (annars går ESP:n automatiskt tillbaka till förra versionen vid nästa omstart)
bool verifyRollbackLater() { return true; }
static bool otaMarkedValid = false;

// ---------------------------------------------------------------- Bussdata (skrivs av CAN-tasken)
static portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
static volatile uint32_t frames = 0;
static volatile uint32_t lastFrameMs = 0;
static volatile int      tcAddr = -1;

static volatile int32_t  speedMmS = -1;
static volatile uint32_t speedMs = 0;
static volatile bool     speedIsGround = false;

static volatile float    busWidthM = NAN;

static DiagRow diag[DIAG_ROWS];
static int nDiag = 0;

static uint32_t pgnOf(uint32_t id) {
  uint8_t dp = (id >> 24) & 0x03, pf = (id >> 16) & 0xFF, ps = (id >> 8) & 0xFF;
  if (pf < 240) return ((uint32_t)dp << 16) | ((uint32_t)pf << 8);
  return ((uint32_t)dp << 16) | ((uint32_t)pf << 8) | ps;
}

static void diagUpdate(uint8_t sa, uint8_t da, uint16_t el, uint16_t ddi, uint8_t cmd, int32_t v, uint32_t now) {
  portENTER_CRITICAL(&mux);
  int i;
  for (i = 0; i < nDiag; i++)
    if (diag[i].sa == sa && diag[i].element == el && diag[i].ddi == ddi && diag[i].cmd == cmd) break;
  if (i == nDiag) {
    if (nDiag < DIAG_ROWS) nDiag++;
    else {
      int old = 0;
      for (int k = 1; k < DIAG_ROWS; k++) if (diag[k].lastMs < diag[old].lastMs) old = k;
      i = old;
    }
    diag[i].sa = sa; diag[i].element = el; diag[i].ddi = ddi; diag[i].cmd = cmd; diag[i].count = 0;
  }
  diag[i].da = da; diag[i].value = v; diag[i].count++; diag[i].lastMs = now;
  portEXIT_CRITICAL(&mux);
}

static volatile uint32_t realFrameMs = 0;   // senaste RIKTIGA meddelande (inte demo)

// demo = true för simulerade meddelanden från demo.h: de loggas inte
static void handleFrame(const twai_message_t &m, bool demo) {
  uint32_t now = millis();
  frames++;
  lastFrameMs = now;
  uint32_t id = m.identifier;
  uint8_t sa = id & 0xFF, da = (id >> 8) & 0xFF;
  uint32_t pgn = m.extd ? pgnOf(id) : 0;
  if (!demo) {
    realFrameMs = now;
    loggerFrame(id, m.extd, m.data, m.data_length_code);
  }
  if (!m.extd) return;
  if (!demo) vtpoolFrame(pgn, sa, da, m.data, m.data_length_code, now);   // redskapets bild (VT-objektpool)
  redskapFrame(pgn, sa, da, m.data, m.data_length_code, now, tcAddr);   // namn, redskapets värden, DDOP
  traktorFrame(pgn, sa, m.data, m.data_length_code, now, rdImplAddr);    // traktorns egna sändningar

  // Hastighet: markhastighet (GBSD) i första hand, annars hjulhastighet (WBSD)
  if ((pgn == 0xFE49 || pgn == 0xFE48) && m.data_length_code >= 2) {
    uint16_t raw = m.data[0] | (m.data[1] << 8);
    if (raw < 0xFB00) {
      bool ground = (pgn == 0xFE49);
      if (ground || !speedIsGround || now - speedMs > 1500) {
        speedMmS = raw; speedMs = now; speedIsGround = ground;
      }
    }
    return;
  }

  if (pgn != 0xCB00 || m.data_length_code < 8) return;
  uint8_t cmd = m.data[0] & 0x0F;
  if (cmd == 0x0E) { if (da == 0xFF) tcAddr = sa; return; }   // TC-status
  uint16_t el = (m.data[0] >> 4) | ((uint16_t)m.data[1] << 4);
  uint16_t ddi = m.data[2] | (m.data[3] << 8);
  int32_t v = (int32_t)((uint32_t)m.data[4] | ((uint32_t)m.data[5] << 8) | ((uint32_t)m.data[6] << 16) | ((uint32_t)m.data[7] << 24));
  diagUpdate(sa, da, el, ddi, cmd, v, now);

  if (cmd != 0x03) return;
  if (tcAddr >= 0 && sa == tcAddr) return;   // värden FRÅN TC är börvärden
  // Arbetsbredd från det valda redskapet (redskap.h). Andra enheter (t.ex. traktorn) räknas inte.
  if (sa != rdImplAddr) return;
  if ((ddi == DDI_WIDTH_A || ddi == DDI_WIDTH_B) && v >= 1000 && v <= 60000) {
    if (ddi == DDI_WIDTH_A || isnan(busWidthM)) busWidthM = v / 1000.0f;
  }
}

static void canTask(void *) {
  twai_message_t m;
  for (;;) {
    if (twai_receive(&m, pdMS_TO_TICKS(100)) == ESP_OK) handleFrame(m, false);
    else {
      twai_status_info_t st;
      if (twai_get_status_info(&st) == ESP_OK) {
        if (st.state == TWAI_STATE_BUS_OFF) twai_initiate_recovery();
        else if (st.state == TWAI_STATE_STOPPED) twai_start();
      }
    }
  }
}

static bool canStart() {
  // GPIO19/20 används av USB som standard – koppla bort USB-paddarna
  CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE);
  twai_general_config_t gc = TWAI_GENERAL_CONFIG_DEFAULT((gpio_num_t)CAN_TX_PIN, (gpio_num_t)CAN_RX_PIN, TWAI_MODE_LISTEN_ONLY);
  gc.rx_queue_len = 512;
  twai_timing_config_t tc = TWAI_TIMING_CONFIG_250KBITS();
  twai_filter_config_t fc = TWAI_FILTER_CONFIG_ACCEPT_ALL();
  if (twai_driver_install(&gc, &tc, &fc) != ESP_OK) return false;
  return twai_start() == ESP_OK;
}

// ---------------------------------------------------------------- Tillstånd
static UiState ui;
static Preferences prefs;
static bool dirty = true;           // rita om direkt

static double fuelMinSum = 0;          // bränsle (l/h) summerat under minuten
static uint32_t fuelMinN = 0, minSamples = 0;
static uint32_t minuteStartMs = 0;

// Fyller i redskapssidans data
static float genImplWidth = NAN;     // redskapets arbetsbredd (m) från ISOBUS, för kartan
static int   genImplWork = -1;       // redskapets arbetsläge (DDI 141): 1/0, -1 okänt

static void fillGeneric(uint32_t now) {
  RedskapInfo r = rdInfo;
  bool hasD = redskapHasDdop();
  ui.genHasImpl = r.addr >= 0;
  ui.genDdop = hasD;
  if (r.addr < 0) {
    snprintf(ui.genTitel, sizeof(ui.genTitel), "%s", L("Inget redskap", "No implement"));
    snprintf(ui.genSub, sizeof(ui.genSub), "%s", L("inget ISOBUS-redskap pratar med traktorns TC", "no ISOBUS implement is talking to the tractor's TC"));
  } else {
    if (hasD && ddop.namn[0]) snprintf(ui.genTitel, sizeof(ui.genTitel), "%s", ddop.namn);
    else snprintf(ui.genTitel, sizeof(ui.genTitel), "%s", L("Okänt redskap", "Unknown implement"));
    if (r.nameKnown) {
      const char *kn = klassNamn(nmKlass(r.name));
      char kb[24];
      if (!kn) { snprintf(kb, sizeof(kb), L("klass %d", "class %d"), nmKlass(r.name)); kn = kb; }
      int o = snprintf(ui.genSub, sizeof(ui.genSub), L("%s · tillverkare %d · adress %d", "%s · manufacturer %d · address %d"), kn, nmTillv(r.name), r.addr);
      if (hasD && ddop.ver[0] && o < (int)sizeof(ui.genSub))
        snprintf(ui.genSub + o, sizeof(ui.genSub) - o, L(" · program %s", " · software %s"), ddop.ver);
    } else {
      snprintf(ui.genSub, sizeof(ui.genSub), L("ISOBUS-namnet inte hört än · adress %d", "ISOBUS name not heard yet · address %d"), r.addr);
    }
  }
  // Redskapets värden, sorterade på element och DDI
  ImplVal v[IMPL_VALS];
  int n;
  portENTER_CRITICAL(&rdMux);
  n = nImplVals;
  memcpy(v, implVals, sizeof(ImplVal) * n);
  portEXIT_CRITICAL(&rdMux);
  for (int i = 1; i < n; i++) {
    ImplVal t = v[i]; int j = i - 1;
    while (j >= 0 && (v[j].el > t.el || (v[j].el == t.el && v[j].ddi > t.ddi))) { v[j + 1] = v[j]; j--; }
    v[j + 1] = t;
  }
  if (n > GEN_ROWS) n = GEN_ROWS;
  float width = NAN; int work = -1;
  genImplWidth = NAN;
  for (int i = 0; i < n; i++) {
    GenRow &g = ui.gen[i];
    redskapFormat(v[i].el, v[i].ddi, v[i].v, g.namn, sizeof(g.namn), g.val, sizeof(g.val), g.enh, sizeof(g.enh));
    g.farsk = now - v[i].ms < 10000;
    if (!g.farsk) continue;
    if (v[i].ddi == 67 && v[i].v >= 100 && v[i].v <= 60000) width = v[i].v / 1000.0f;
    if (v[i].ddi == 70 && isnan(width) && v[i].v >= 100 && v[i].v <= 60000) width = v[i].v / 1000.0f;
    if (v[i].ddi == 141 && (v[i].v == 0 || v[i].v == 1)) work = (work == 1 || v[i].v == 1) ? 1 : 0;
  }
  ui.nGen = n;
  genImplWidth = width;
  genImplWork = work;
  uiFmt(ui.genWidth, sizeof(ui.genWidth), width, 1);
  snprintf(ui.genWork, sizeof(ui.genWork), "%s", work == 1 ? L("PÅ", "ON") : work == 0 ? L("AV", "OFF") : "--");
}

static void saveSettings() {
  prefs.putInt("bri", ui.brightness);
  prefs.putFloat("wman", ui.manualWidth);
  prefs.putBool("wmode", ui.widthManualMode);
}

// körs precis innan omstart efter uppdatering
static void beforeRestart() {
  loggerFlushAll();
}

static void copyNetStatus() {
  portENTER_CRITICAL(&netMux);
  NetView &v = ui.net;
  v.connected = net.connected;
  memcpy(v.ssid, net.ssid, sizeof(v.ssid));
  memcpy(v.ip, net.ip, sizeof(v.ip));
  v.rssi = net.rssi;
  memcpy(v.msg, net.msg, sizeof(v.msg));
  v.scanning = net.scanning;
  v.nScan = net.nScan;
  for (int i = 0; i < net.nScan && i < UI_SCAN_MAX; i++) {
    memcpy(v.scanSsid[i], net.scanSsid[i], 33);
    v.scanRssi[i] = net.scanRssi[i];
    v.scanSecure[i] = net.scanSecure[i];
  }
  v.otaActive = net.otaActive;
  v.otaPct = net.otaPct;
  v.latestVersion = net.latestVersion;
  v.otaOffer = net.otaOffer;
  memcpy(v.otaNote, net.otaNote, sizeof(v.otaNote));
  portEXIT_CRITICAL(&netMux);
  strncpy(v.devId, devId, sizeof(v.devId));
  strncpy(v.namn, cfgName, sizeof(v.namn) - 1); v.namn[sizeof(v.namn) - 1] = 0;
  v.hasSaved = netHasSaved();
  v.uppd = cfgUppd;
  netFeedbackUrl(v.fbUrl, sizeof(v.fbUrl));
  v.sdOk = sdOk;
}

static void demoStop() {
  demoOn = false;
  ui.demo = false;
  for (int i = 0; i < CHART_POINTS; i++) ui.trFuelMin[i] = NAN;                // bränslediagrammet hade demodata
}

static void updateState(float dt) {
  uint32_t now = millis();
  // Riktig ISOBUS-trafik -> demoläget av direkt
  if (demoOn && realFrameMs && now - realFrameMs < 2000) demoStop();
  ui.demo = demoOn;
  ui.nowMs = now;
  ui.frames = frames;
  bool hadBus = ui.busOk;
  ui.busOk = (now - lastFrameMs) < 2000 && frames > 0;
  ui.tcSeen = tcAddr >= 0;
  ui.tcAddr = tcAddr;

  (void)hadBus;

  // Vilket redskap? -> redskapssidan, annars traktorsidan
  bool silent = frames == 0 || (now - lastFrameMs) > IDLE_AFTER_MS;
  static uint32_t busSince = 0;
  if (silent) busSince = 0; else if (!busSince) busSince = now;
  int autoPage;
  if (rdInfo.addr >= 0) {
    autoPage = PG_GENERIC;
  } else if (!silent && now - busSince < 8000) {
    autoPage = PG_TRAKTOR;                                          // ge redskapet tid att höras
  } else {
    autoPage = PG_TRAKTOR;                                          // inget redskap: traktorns data
  }
  ui.autoPage = autoPage;
  ui.vtAvail = vvOk && rdInfo.addr >= 0 && rdInfo.nameKnown && rdInfo.name == vvName;
  ui.vtLive = ui.vtAvail && vvLastCmdMs && now - vvLastCmdMs < 3000;
  fillGeneric(now);
  traktorFill(ui, now);
  kartaFill(ui, now);

  // Viloläge av/på, och byte mellan redskaps- och traktorsidan.
  // En sida som öppnats från inställningarna (genManual) ligger kvar tills man trycker
  // TILLBAKA – även utan ISOBUS, så att man kan se hur den ser ut.
  // Startval: när ett ISOBUS-redskap hittas efter start frågar skärmen TRAKTOR eller ISOBUS.
  // Finns inget redskap (8 s efter att bussen vaknat) blir det traktorvyn direkt.
  static uint32_t valOpenedMs = 0;
  if (ui.startVal == 0 && !silent && !demoOn) {
    if (rdInfo.addr >= 0) { ui.startVal = 1; ui.page = PG_VAL; valOpenedMs = now; }
    else if (now - busSince >= 8000) ui.startVal = 2;
  }
  if (ui.page == PG_VAL) {
    ui.valSecs = 20 - (int)((now - valOpenedMs) / 1000);
    if (silent) { ui.page = PG_IDLE; ui.startVal = 0; ui.valHem = false; }
    else if (ui.valSecs <= 0 && !ui.valHem) {             // v37: öppnat med huset = väljer inget själv
      ui.page = autoPage == PG_TRAKTOR ? PG_GENERIC : autoPage;
      ui.genManual = ui.page != autoPage; ui.startVal = 2;
    }
  }
  if ((ui.page == PG_VT || ui.page == PG_ANALYS) && silent) { ui.page = PG_IDLE; ui.genManual = false; }   // tändningen av -> vila
  bool busPage = ui.page == PG_GENERIC || ui.page == PG_TRAKTOR;
  if (busPage && silent && !ui.genManual) ui.page = PG_IDLE;
  else if (ui.page == PG_IDLE && !silent) ui.page = autoPage;
  else if (busPage && !ui.genManual && ui.page != autoPage) ui.page = autoPage;

  ui.kmh = (speedMmS >= 0 && now - speedMs < 2000) ? speedMmS * 0.0036f : NAN;

  float bw = busWidthM;
  ui.widthFromBus = !ui.widthManualMode && !isnan(bw);
  ui.widthM = ui.widthFromBus ? bw : ui.manualWidth;

  portENTER_CRITICAL(&mux);
  memcpy(ui.rows, diag, sizeof(diag));
  ui.nRows = nDiag;
  portEXIT_CRITICAL(&mux);

  // Bränsleanalysen: ett mätvärde per sekund (inte i demoläget), räknas om varje sekund när sidan visas
  static uint32_t anLastSec = 0;
  if (now - anLastSec >= 1000) {
    anLastSec = now;
    float pto = fmaxf(isnan(ui.trv.ptoF) ? 0 : ui.trv.ptoF, isnan(ui.trv.ptoR) ? 0 : ui.trv.ptoR);
    if (!demoOn) analysSample(ui.trv.kmh, ui.trv.fuel, ui.trv.rpm, pto, ui.karta.widthM, ui.karta.logging);
    faltSample(now, ui.trv.kmh, ui.trv.fuel, ui.karta.logging);               // v37: fältloggen (tid på fältet)
    if (trd.gpsTidMs && now - trd.gpsTidMs < 3000) faltGpsTid(trd.gpsDag, trd.gpsSek10k);   // klockan från GPS
    if (ui.page == PG_ANALYS) analysCompute(ui.an, ui.anDiesel, ui.anTimKr, ui.anLage);
  }
  // Bränslediagrammet: ett minutmedel per minut
  if (!isnan(ui.trv.fuel)) { fuelMinSum += ui.trv.fuel; fuelMinN++; }
  minSamples++;
  if (now - minuteStartMs >= 60000) {
    minuteStartMs += 60000;
    memmove(&ui.trFuelMin[0], &ui.trFuelMin[1], sizeof(float) * (CHART_POINTS - 1));
    ui.trFuelMin[CHART_POINTS - 1] = fuelMinN > minSamples / 2 ? (float)(fuelMinSum / fuelMinN / 60.0) : NAN;   // l/h -> l/min
    fuelMinSum = 0; fuelMinN = 0; minSamples = 0;
  }

  copyNetStatus();

  // Ny version godkänns när den har kontakt med GitHub eller efter 10 minuters drift
  if (!otaMarkedValid && (netContactOk || now > 10UL * 60UL * 1000UL)) {
    esp_ota_mark_app_valid_cancel_rollback();
    otaMarkedValid = true;
  }
}

// ---------------------------------------------------------------- Knapptryck
static uint32_t confirmOpenedMs = 0;

static void enhFill(bool direkt);                  // v44: enhetsinfo (längre ner)
static void onButton(int id) {
  uint32_t now = millis();
  // Tangentbordets bokstäver
  if (id >= B_KEY0) {
    const char *k = uiKeyStr(ui.kbMode, id);
    if (strlen(ui.kbText) + strlen(k) < 64) strcat(ui.kbText, k);
    if (ui.kbMode == 1) ui.kbMode = 0;          // stor bokstav gäller ett tecken
    return;
  }
  // Nätlistan
  if (id >= B_NET0 && id < B_NET0 + UI_SCAN_MAX) {
    int i = id - B_NET0;
    if (i >= ui.net.nScan) return;
    strncpy(ui.kbSsid, ui.net.scanSsid[i], 32);
    if (ui.net.scanSecure[i]) {
      ui.kbFor = 0; ui.kbText[0] = 0; ui.kbMode = 0; ui.page = PG_KEYBOARD;
    } else {
      netConnectTo(ui.kbSsid, "");
      ui.page = PG_SETTINGS;
    }
    return;
  }
  // prisrutan på analyssidan: bara dess egna knappar gäller
  if (ui.anPris && ui.page == PG_ANALYS && !(id >= B_AN_D_M && id <= B_AN_PRIS_OK)) return;
  if (id >= B_NUM0 && id <= B_NUM9) {                     // v37: siffra i sifferfältet (högst 4)
    size_t n = strlen(ui.numText);
    if (ui.numFor == 3) {                                  // v45: skiftets areal, högst fem siffror och två decimaler
      const char *dot = strchr(ui.numText, '.');
      size_t dec = dot ? strlen(dot + 1) : 0, sif = n - (dot ? 1 : 0);
      if (n == 1 && ui.numText[0] == '0') n = 0;
      if ((!dot || dec < 2) && sif < 5 && n < 6) { ui.numText[n] = '0' + (id - B_NUM0); ui.numText[n + 1] = 0; }
      return;
    }
    if (n == 1 && ui.numText[0] == '0') n = 0;
    if (n < 4) { ui.numText[n] = '0' + (id - B_NUM0); ui.numText[n + 1] = 0; }
    return;
  }
  if (id == B_NUM_DOT) {                                   // v45: decimaltecken (bara skiftets areal)
    size_t n = strlen(ui.numText);
    if (ui.numFor == 3 && !strchr(ui.numText, '.') && n < 5) { if (!n) { ui.numText[0] = '0'; n = 1; } ui.numText[n] = '.'; ui.numText[n + 1] = 0; }
    return;
  }
  if (id >= B_MAN_KAP0 && id < B_MAN_KAP0 + MAN_N) { ui.manKap = id - B_MAN_KAP0; ui.manSida = 0; return; }   // v42
  switch (id) {
    case B_SETTINGS:                                      // kom ihåg varifrån (traktor, redskap, terminal)
      ui.setBack = (ui.page == PG_GENERIC || ui.page == PG_TRAKTOR || ui.page == PG_VT) ? ui.page : -1;
      ui.page = PG_SETTINGS; break;
    case B_CLOSE:                                         // bocken: tillbaka dit man kom ifrån (vila sköts som förut)
      if (ui.setBack >= 0) { ui.page = ui.setBack; ui.genManual = ui.setBack != ui.autoPage; }
      else { ui.page = ui.autoPage; ui.genManual = false; }
      ui.setBack = -1; saveSettings(); break;
    case B_REDSKAP:  ui.page = PG_GENERIC; ui.genManual = true; ui.genPage = 0; break;
    case B_TRAKTOR:  ui.page = PG_TRAKTOR; ui.genManual = true; break;
    case B_GEN_BACK: ui.page = ui.autoPage; ui.genManual = false; break;
    case B_GEN_PAGE: ui.genPage++; break;
    case B_TR_RAW:   ui.trRawView = !ui.trRawView; break;
    case B_TR_INFO0: case B_TR_INFO1:
      ui.trInfo = id - B_TR_INFO0; break;
    case B_NATT:     ui.natt = !ui.natt; prefs.putBool("natt", ui.natt); break;
    case B_HEM:      ui.page = PG_VAL; ui.valHem = true; break;                       // v37: huset = startvalet
    case B_K_GARDIN: ui.kartaGardin = !ui.kartaGardin; break;                        // v37: kartans rutor in/ut
    case B_KI_BG_SAT: kartaBakgrund(0); break;                                       // v37: kartans bakgrund
    case B_KI_BG_ENF: kartaBakgrund(1); break;
    // Uppdateringsfrågan: JA = installera, INTE NU = fråga igen efter omstart, STÄNG AV = fråga aldrig mer
    case B_OTA_JA:   ui.net.otaOffer = 0; netOtaJa(); break;
    case B_OTA_NEJ:  ui.net.otaOffer = 0; netOtaNej(); break;
    case B_OTA_AV:   ui.net.otaOffer = 0; netOtaNej(); netSetUppd(false); break;
    // MER: språk, uppdateringar, feedback, om
    case B_MER_OPEN: ui.page = PG_MER; break;
    case B_MER_BACK: ui.page = PG_SETTINGS; break;
    case B_SPRAK_SV: case B_SPRAK_EN:
      uiLang = ui.lang = id == B_SPRAK_EN ? 1 : 0; prefs.putUChar("sprak", ui.lang); break;
    case B_UPPD_PA:  netSetUppd(true); break;
    case B_UPPD_AV:  netSetUppd(false); break;
    case B_FB_OPEN:  ui.page = PG_FEEDBACK; break;
    case B_FB_BACK:  ui.page = PG_MER; break;
    case B_DUB_OPEN: ui.dubbelFraga = true; break;
    case B_DUB_NEJ:  ui.dubbelFraga = false; break;
    case B_DUB_JA:   ui.dubbelFraga = false; dubbelByt(beforeRestart); break;
    case B_OM_OPEN:  ui.page = PG_MANUAL; ui.manKap = MAN_N - 1; ui.manSida = 0; ui.manBack = PG_MER; break;
    // Karta
    case B_KARTA:    ui.page = PG_KARTA; break;
    case B_K_BACK:   ui.page = PG_TRAKTOR; ui.genManual = ui.autoPage != PG_TRAKTOR; break;
    case B_K_ZIN:    ui.kartaMpp = fmaxf(K_MPP_MIN, ui.kartaMpp / 2); break;
    case B_K_ZUT:    ui.kartaMpp = fminf(K_MPP_MAX, ui.kartaMpp * 2); break;
    case B_K_Z100:   ui.kartaMpp = K_MPP_100M; break;
    case B_K_FOLJ:   kartaFolj(); break;
    case B_K_FARG_B: kartaFarg(0); break;
    case B_K_FARG_D: kartaFarg(1); break;
    case B_K_FARG_S: kartaFarg(2); break;
    case B_K_START:  kartaKnapp(now); break;
    case B_K_INST:   ui.page = PG_KARTINST; break;
    case B_K_NOLLA:  ui.kartaConfirm = true; confirmOpenedMs = now; break;
    case B_K_NOLLA_NO: ui.kartaConfirm = false; break;
    case B_K_NOLLA_OK: ui.kartaConfirm = false; kartaNolla(); faltNolla(); break;
    case B_KI_KLAR:  ui.page = PG_KARTA; kartaSaveCfg(); break;
    case B_KI_TRIG_M: kcfg.trig = (kcfg.trig + KT_N - 1) % KT_N; kPrevState = -2; break;
    case B_KI_TRIG_P: kcfg.trig = (kcfg.trig + 1) % KT_N; kPrevState = -2; break;
    case B_KI_S_M: case B_KI_S_P: if (kcfg.trig != KT_MANUELL && kcfg.trig != KT_ISOBUS) kcfg.startEv ^= 1; break;
    case B_KI_P_M: case B_KI_P_P: if (kcfg.trig != KT_MANUELL && kcfg.trig != KT_ISOBUS) kcfg.stopEv ^= 1; break;
    case B_VAL_TRAKTOR: ui.page = PG_TRAKTOR; ui.genManual = true; ui.startVal = 2; ui.valHem = false; break;
    case B_VAL_VT:   ui.page = PG_VT; ui.genManual = true; ui.startVal = 2; ui.valHem = false; break;
    case B_VT_OPEN:  ui.page = PG_VT; ui.genManual = true; break;
    case B_VT_BACK:  ui.page = ui.autoPage; ui.genManual = false; break;
    case B_TR_ANALYS: analysCompute(ui.an, ui.anDiesel, ui.anTimKr, ui.anLage); ui.page = PG_ANALYS; ui.anPris = false; break;
    case B_AN_BACK:  ui.page = PG_TRAKTOR; ui.genManual = ui.autoPage != PG_TRAKTOR; ui.anPris = false; break;
    case B_AN_L0: case B_AN_L1: case B_AN_L2:
      ui.anLage = id - B_AN_L0; prefs.putInt("anlage", ui.anLage);
      analysCompute(ui.an, ui.anDiesel, ui.anTimKr, ui.anLage); break;
    case B_AN_PRIS:  ui.anPris = true; break;
    case B_AN_D_M:   if (ui.anDiesel > 1.0f) ui.anDiesel -= 0.5f; break;
    case B_AN_D_P:   if (ui.anDiesel < 60.0f) ui.anDiesel += 0.5f; break;
    case B_AN_T_M:   if (ui.anTimKr > 0.0f) ui.anTimKr -= 50.0f; break;
    case B_AN_T_P:   if (ui.anTimKr < 5000.0f) ui.anTimKr += 50.0f; break;
    case B_AN_PRIS_OK:
      ui.anPris = false;
      prefs.putFloat("diesel", ui.anDiesel); prefs.putFloat("timkr", ui.anTimKr);
      analysCompute(ui.an, ui.anDiesel, ui.anTimKr, ui.anLage); break;
    case B_VAL_ISOBUS:
      ui.page = PG_GENERIC;
      ui.genManual = ui.page != ui.autoPage; ui.startVal = 2; ui.valHem = false; break;
    case B_KI_WMODE: kcfg.autoW = !kcfg.autoW; break;
    case B_KI_W_M:   if (kcfg.widthM > 1.0f) kcfg.widthM -= 0.5f; break;
    case B_KI_W_P:   if (kcfg.widthM < 60.0f) kcfg.widthM += 0.5f; break;
    case B_DEMO:
      if (demoOn) demoStop();
      else if (!realFrameMs || now - realFrameMs > 5000) { demoStart(); ui.demo = true; }
      break;
    case B_BRI_MINUS: if (ui.brightness > 20) ui.brightness -= 10; break;
    case B_BRI_PLUS:  if (ui.brightness < 100) ui.brightness += 10; break;
    case B_W_MINUS:   if (ui.manualWidth > 1.0f) ui.manualWidth -= 0.5f; break;
    // v37: arbetsbredden i cm med sifferfältet (inställningarna och kartan)
    case B_W_NUM: case B_KI_W_NUM:
      ui.numFor = id == B_W_NUM ? 1 : 2; ui.numBack = ui.page;
      snprintf(ui.numText, sizeof(ui.numText), "%d", (int)lroundf((id == B_W_NUM ? ui.manualWidth : kcfg.widthM) * 100));
      ui.page = PG_SIFFROR; break;
    case B_NUM_DEL: { size_t n = strlen(ui.numText); if (n) ui.numText[n - 1] = 0; break; }
    case B_NUM_CANCEL: ui.page = ui.numBack; break;
    case B_K_SKIFT:                                          // v45 (10.19): skiftets areal
      ui.numFor = 3; ui.numBack = ui.page;
      ui.numText[0] = 0;
      if (kSkiftHa > 0) {                                  // t.ex. 35.65 eller 12.5 (utan onödiga nollor)
        snprintf(ui.numText, sizeof(ui.numText), "%.2f", kSkiftHa);
        size_t n = strlen(ui.numText);
        while (n > 0 && ui.numText[n - 1] == '0') ui.numText[--n] = 0;
        if (n > 0 && ui.numText[n - 1] == '.') ui.numText[--n] = 0;
      }
      ui.page = PG_SIFFROR; break;
    case B_NUM_OK: {
      int cm = atoi(ui.numText);
      if (ui.numFor == 3) { float ha = (float)atof(ui.numText); kartaSkift(ha >= 0.01f && ha <= 999.99f ? roundf(ha * 100) / 100 : 0); ui.page = ui.numBack; break; }
      if (cm >= 100 && cm <= 6000) {
        if (ui.numFor == 1) { ui.manualWidth = cm / 100.0f; saveSettings(); }
        else { kcfg.widthM = cm / 100.0f; kartaSaveCfg(); }
      }
      ui.page = ui.numBack; break;
    }
    // v37: loggning PÅ/AV (10.5)
    case B_LOGG_PA: loggAv = ui.loggAv = false; prefs.putBool("loggav", false); break;
    case B_LOGG_AV: loggAv = ui.loggAv = true; prefs.putBool("loggav", true); break;
    case B_W_PLUS:    if (ui.manualWidth < 40.0f) ui.manualWidth += 0.5f; break;
    case B_W_MODE:    ui.widthManualMode = !ui.widthManualMode; break;
    case B_WIFI:      ui.page = PG_WIFI; netRequestScan(); break;
    case B_WIFI_SCAN: netRequestScan(); break;
    case B_WIFI_BACK: ui.page = PG_SETTINGS; break;
    case B_WIFI_FORGET: netForgetAll(); break;
    case B_DIAG:      ui.page = PG_DIAG; break;
    // v42: manualen
    case B_MAN_OPEN:  ui.page = PG_MANUAL; ui.manKap = -1; ui.manSida = 0; ui.manBack = PG_SETTINGS; break;
    case B_MAN_BACK:  ui.page = ui.manBack == PG_MER ? PG_MER : PG_SETTINGS; break;
    case B_MAN_INNEH: ui.manKap = -1; ui.manSida = 0; break;
    case B_MAN_NASTA:
      if (uiManSidaVisad + 1 < uiManSidor) ui.manSida = uiManSidaVisad + 1;
      else if (ui.manKap + 1 < MAN_N) { ui.manKap++; ui.manSida = 0; }
      break;
    case B_MAN_FORRA:
      if (uiManSidaVisad > 0) ui.manSida = uiManSidaVisad - 1;
      else if (ui.manKap > 0) { ui.manKap--; ui.manSida = 99; }      // sista sidan i förra kapitlet
      break;
    case B_DIAG_BACK: ui.page = PG_SETTINGS; break;
    // v44: enhetsinfo
    case B_ENH_OPEN:  ui.dubbelFraga = false; ui.page = PG_ENHET; reqSdInfo = true; enhFill(true); break;
    case B_ENH_BACK:  ui.dubbelFraga = false; ui.page = PG_SETTINGS; break;
    case B_KB_SHIFT:  ui.kbMode = (ui.kbMode == 1) ? 0 : 1; break;
    case B_KB_MODE:   ui.kbMode = (ui.kbMode == 2) ? 0 : 2; break;
    case B_KB_BACK: {
      int n = strlen(ui.kbText);
      if (n > 0) {
        n--;
        while (n > 0 && (ui.kbText[n] & 0xC0) == 0x80) n--;   // ta bort hela å/ä/ö
        ui.kbText[n] = 0;
      }
      break;
    }
    case B_KB_SPACE:  if (strlen(ui.kbText) < 63) strcat(ui.kbText, " "); break;
    case B_KB_CANCEL: case B_KB_OK:
      if (ui.kbFor == 2) {                            // v37: fältets namn -> skapa rapporten (HOPPA ÖVER = utan namn)
        memset(&ui.exp, 0, sizeof(ui.exp));
        if (id == B_KB_OK) strncpy(ui.exp.namn, ui.kbText, sizeof(ui.exp.namn) - 1);
        ui.exp.status = 1; ui.kbFor = 0; ui.page = PG_EXPORT;
      } else if (id == B_KB_CANCEL) ui.page = PG_WIFI;
      else { netConnectTo(ui.kbSsid, ui.kbText); ui.page = PG_SETTINGS; }
      break;
    // v37: fältrapporten
    case B_K_EXPORT: ui.kbFor = 2; ui.kbText[0] = 0; ui.kbMode = 1; ui.page = PG_KEYBOARD; break;
    case B_EXP_STANG: reqApStop = true; ui.exp.status = 0; ui.page = PG_KARTA; break;
  }
}

// Pek-hantering: knappen gäller när man SLÄPPER, och bara om man tryckte
// och släppte på samma knapp. Undantag (v43): tangentbordets och sifferfältets tangenter
// gäller redan när man trycker NED, så att bokstaven syns i samma bild som markeringen.
//
// v43: pekskärmen läses i en egen task (touchTask) ca 100 ggr/s, även medan skärmen ritas.
// Tryck och släpp läggs i en kö som huvudloopen tömmer – inga tryck missas längre, hur
// snabbt man än skriver. Släppet syns så fort pekskärmen rapporterar det (förut 120 ms).
#define TOUCH_POLL_MS   10      // pektaskens intervall
#define TOUCH_HALL_MS   120     // ingen rapport så här länge = fingret har släppt (reserv)
#define TOUCH_SLAPP_MS  25      // en släpp-rapport räknas först så här länge efter senaste tryck
struct TEv { uint8_t typ; int16_t x, y; uint32_t ms; };     // typ 1 = ned, 2 = upp
static TEv tEv[32];
static volatile uint8_t tEvW = 0, tEvR = 0;
static volatile int16_t tCurX = 0, tCurY = 0;               // senaste läget (för att dra i kartan)
static portMUX_TYPE tMux = portMUX_INITIALIZER_UNLOCKED;
static void tPush(uint8_t typ, int x, int y, uint32_t ms) {
  portENTER_CRITICAL(&tMux);
  uint8_t n = (tEvW + 1) & 31;
  if (n != tEvR) { tEv[tEvW].typ = typ; tEv[tEvW].x = x; tEv[tEvW].y = y; tEv[tEvW].ms = ms; tEvW = n; }
  portEXIT_CRITICAL(&tMux);
}
static bool tPop(TEv &e) {
  bool ok = false;
  portENTER_CRITICAL(&tMux);
  if (tEvR != tEvW) { e = tEv[tEvR]; tEvR = (tEvR + 1) & 31; ok = true; }
  portEXIT_CRITICAL(&tMux);
  return ok;
}
static void touchTask(void *) {
  bool was = false;
  int lx = 0, ly = 0;
  uint32_t lastSeen = 0, lastRetry = 0;
  for (;;) {
    uint32_t now = millis();
    if (!touchOk && now - lastRetry > 5000) {    // pekskärmen svarade inte vid start: försök igen
      lastRetry = now;
      touchInit();                               // (tar själv I2C-låset vid reset)
    }
    int x = 0, y = 0;
    int r = touchRead(x, y);
    if (r == 1) { lx = x; ly = y; lastSeen = now; tCurX = x; tCurY = y; touchLastX = x; touchLastY = y; }
    bool pressed;
    if (r == 1) pressed = true;
    else if (!was) pressed = false;
    else if (r == 0) pressed = now - lastSeen < TOUCH_SLAPP_MS;
    else pressed = now - lastSeen < TOUCH_HALL_MS;
    if (pressed && !was) { touchPresses++; tPush(1, lx, ly, now); }
    if (!pressed && was) tPush(2, lx, ly, now);
    was = pressed;
    vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
  }
}

// Tangenter som gäller redan vid nedtryck (tangentbordet och sifferfältet). OK/AVBRYT gäller vid släpp.
static bool tryckDirekt(int id) {
  if (ui.net.otaOffer) return false;
  if (ui.page == PG_KEYBOARD)
    return id >= B_KEY0 || (id >= B_KB_SHIFT && id <= B_KB_SPACE);
  if (ui.page == PG_SIFFROR)
    return (id >= B_NUM0 && id <= B_NUM9) || id == B_NUM_DEL || id == B_NUM_DOT;
  return false;
}

static bool wasPressed = false;
static int downId = B_NONE;
static bool downKlar = false;            // knappen har redan gällt vid nedtrycket
static uint32_t tryckMs = 0;             // när trycket kom (för mätningen tryck -> bild)

static void handleTouch() {
  uint32_t now = millis();
  // Dra med fingret på helskärmskartan = flytta kartan
  static int dragX = 0, dragY = 0;
  static bool dragging = false;
  TEv e;
  while (tPop(e)) {
    if (e.typ == 1) {
      downId = uiHit(e.x, e.y); ui.pressId = downId; dirty = true; downKlar = false;
      dragging = (ui.page == PG_KARTA && downId == B_NONE && !ui.kartaConfirm && !ui.net.otaOffer && !ui.net.otaActive);
      dragX = e.x; dragY = e.y;
      if (!tryckMs) tryckMs = e.ms ? e.ms : 1;
      if (downId != B_NONE && tryckDirekt(downId)) { onButton(downId); downKlar = true; }
      wasPressed = true;
    } else {
      if (dragging && (abs(e.x - dragX) + abs(e.y - dragY) >= 4))
        kartaPan(-(e.x - dragX) * ui.kartaMpp, (e.y - dragY) * ui.kartaMpp);
      int upId = uiHit(e.x, e.y);
      ui.pressId = B_NONE;
      if (!downKlar && upId != B_NONE && upId == downId) onButton(upId);
      downKlar = false;
      dragging = false;
      wasPressed = false;
      dirty = true;
    }
  }
  if (wasPressed && dragging) {
    int cx = tCurX, cy = tCurY;
    if (abs(cx - dragX) + abs(cy - dragY) >= 4) {
      kartaPan(-(cx - dragX) * ui.kartaMpp, (cy - dragY) * ui.kartaMpp);
      dragX = cx; dragY = cy;
      dirty = true;
    }
  }
  if (ui.kartaConfirm && now - confirmOpenedMs > 15000) { ui.kartaConfirm = false; dirty = true; }
}

// ---------------------------------------------------------------- Ljusstyrka (programvara)
static uint16_t lutR[32], lutG[64], lutB[32];
static int lutFor = -1;
static void applyBrightness(uint16_t *fb, int n, int pct) {
  if (pct >= 100) return;
  if (lutFor != pct) {
    for (int i = 0; i < 32; i++) { lutR[i] = (uint16_t)((i * pct / 100) << 11); lutB[i] = (uint16_t)(i * pct / 100); }
    for (int i = 0; i < 64; i++) lutG[i] = (uint16_t)((i * pct / 100) << 5);
    lutFor = pct;
  }
  for (int i = 0; i < n; i++) {
    uint16_t c = fb[i];
    fb[i] = lutR[c >> 11] | lutG[(c >> 5) & 63] | lutB[c & 31];
  }
}

// mätning av ritningen (visas i enhetsinfon)
static volatile uint32_t mRitN = 0, mRitSum = 0, mRitMax = 0, mKopSum = 0, mTryckN = 0, mTryckSum = 0, mTryckMax = 0;
static volatile int mRitMaxSida = -1;
static void render() {
  uint32_t t0 = millis();
  ui.tick++;
  uiTema(ui.natt);
  uiDraw(g, ui);
  uint32_t t1 = millis();
  if (g == canvas) {
    applyBrightness(canvas->getFramebuffer(), 800 * 480, ui.brightness);
    canvas->flush();
  }
  uint32_t t2 = millis(), d = t2 - t0;
  mRitN++; mRitSum += d; mKopSum += t2 - t1;
  if (d > mRitMax) { mRitMax = d; mRitMaxSida = ui.page; }
  if (tryckMs) {                                   // första bilden efter ett tryck
    uint32_t l = t2 - tryckMs;
    tryckMs = 0;
    if (l < 60000) { mTryckN++; mTryckSum += l; if (l > mTryckMax) mTryckMax = l; }
  }
}

// ---------------------------------------------------------------- Enhetsinfo (v44)
static temperature_sensor_handle_t tempSens = nullptr;
static char bootOrsakEn[96] = "-";
static void tempInit() {
  temperature_sensor_config_t c = TEMPERATURE_SENSOR_CONFIG_DEFAULT(-10, 80);   // bäst noggrannhet -10..80 °C
  if (temperature_sensor_install(&c, &tempSens) != ESP_OK) { tempSens = nullptr; return; }
  if (temperature_sensor_enable(tempSens) != ESP_OK) tempSens = nullptr;
}
static void tempRead() {                            // var 2:a sekund från loop()
  float f;
  if (!tempSens || temperature_sensor_get_celsius(tempSens, &f) != ESP_OK || isnan(f)) return;
  enhTempC = f;
  if (isnan(enhTempMin) || f < enhTempMin) enhTempMin = f;
  if (isnan(enhTempMax) || f > enhTempMax) enhTempMax = f;
}
static void enhFill(bool direkt) {                  // fyller ui.enh, ungefär en gång per sekund när sidan visas
  static uint32_t lastMs = 0, lastFrames = 0;
  uint32_t now = millis();
  if (!direkt && now - lastMs < 1000) return;
  EnhView &e = ui.enh;
  if (lastMs && now > lastMs) e.ramarS = (uint32_t)((uint64_t)(frames - lastFrames) * 1000 / (now - lastMs));
  lastMs = now; lastFrames = frames;
  e.ramarTot = frames;
  e.tempC = enhTempC; e.tempMin = enhTempMin; e.tempMax = enhTempMax;
  e.psFreeKB = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024;
  e.psTotKB = heap_caps_get_total_size(MALLOC_CAP_SPIRAM) / 1024;
  e.psBigKB = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024;
  e.inFreeKB = heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024;
  e.inTotKB = heap_caps_get_total_size(MALLOC_CAP_INTERNAL) / 1024;
  e.inMinKB = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL) / 1024;
  e.inBigKB = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL) / 1024;
  e.sdKand = sdInfoKand; e.sdTotMB = sdInfoTotMB; e.sdFriMB = sdInfoFriMB;
  e.upSek = now / 1000;
  e.bootNo = bootNo;
  strncpy(e.omstart, L(bootOrsak, bootOrsakEn), sizeof(e.omstart) - 1);
  const esp_partition_t *pr = esp_ota_get_running_partition();
  strncpy(e.part, pr ? pr->label : "-", sizeof(e.part) - 1);
  twai_status_info_t st;
  if (twai_get_status_info(&st) == ESP_OK) {
    e.canState = st.state == TWAI_STATE_STOPPED ? 0 : st.state == TWAI_STATE_RUNNING ? 1 :
                 st.state == TWAI_STATE_BUS_OFF ? 2 : 3;
    e.canMiss = st.rx_missed_count; e.canOverrun = st.rx_overrun_count;
    e.canBusErr = st.bus_error_count; e.canRxErr = st.rx_error_counter;
  } else e.canState = -1;
  e.ritN = mRitN; e.ritSnitt = mRitN ? mRitSum / mRitN : 0; e.ritMax = mRitMax;
}

// Redskapets egen bild (vtview.h) – ritas direkt i bildbufferten när den finns
static void vtRenderHook(Arduino_GFX *gg, int x, int y, int size, int kx, int ky, int kw, int kh) {
  vtviewRender(gg, gg == canvas ? canvas->getFramebuffer() : nullptr, x, y, size, kx, ky, kw, kh);
}

// ---------------------------------------------------------------- Setup / loop
void setup() {
  Serial.begin(115200);
  {                                                     // varför startade skärmen (visas i enhetsinfon)
    static const char *const R[] = {"okänd", "ström på", "extern", "programmet (restart)", "KRASCH (panic)", "VAKTHUND (avbrott)",
                                    "VAKTHUND (task)", "VAKTHUND (annan)", "djupsömn", "spänningsfall", "SDIO", "USB", "JTAG"};
    static const char *const E[] = {"unknown", "power on", "external", "software (restart)", "CRASH (panic)", "WATCHDOG (interrupt)",
                                    "WATCHDOG (task)", "WATCHDOG (other)", "deep sleep", "brownout", "SDIO", "USB", "JTAG"};
    int rr = (int)esp_reset_reason();
    bool ok = rr >= 0 && rr < (int)(sizeof(R) / sizeof(R[0]));
    bool rapp = (rappSteg & 0xFFFF0000u) == RAPP_MAGI && (rr == ESP_RST_PANIC || rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT);
    snprintf(bootOrsak, sizeof(bootOrsak), "%s%s", ok ? R[rr] : "?", rapp ? " - under fältrapporten" : "");
    snprintf(bootOrsakEn, sizeof(bootOrsakEn), "%s%s", ok ? E[rr] : "?", rapp ? " - during the field report" : "");
    rappSteg = 0;
  }
  if (dubbelStart()) otaMarkedValid = true;           // skärmen har just växlat variant (dubbel.h)
  Wire.begin(I2C_SDA, I2C_SCL, 400000);
  exioInit();
  i2cMutex = xSemaphoreCreateMutex();
  delay(20);
  exioSet(EXIO_LCD_RST, false); delay(10); exioSet(EXIO_LCD_RST, true); delay(100);

  lcd->begin();
  canvas = new Arduino_Canvas(800, 480, lcd, 0, 0);
  if (canvas->begin(GFX_SKIP_OUTPUT_BEGIN)) g = canvas;
  else g = lcd;                              // reserv: rita direkt (kan flimra)
  g->setUTF8Print(true);

  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1); tzset();     // v37: svensk tid i fältrapporten
  prefs.begin("flode", false);
  ui.brightness = prefs.getInt("bri", 100);
  ui.natt = prefs.getBool("natt", false);
  ui.loggAv = loggAv = prefs.getBool("loggav", true);         // loggning PÅ/AV (förval AV)
  uiLang = ui.lang = prefs.getUChar("sprak", 0) ? 1 : 0;       // språk
  ui.manualWidth = prefs.getFloat("wman", WIDTH_DEFAULT_M);
  ui.widthManualMode = prefs.getBool("wmode", false);
  ui.anDiesel = prefs.getFloat("diesel", 23.0f);      // förval: diesel 23 kr/l (ändras i bränsleanalysens prisruta)
  ui.anTimKr = prefs.getFloat("timkr", 800.0f);       // förval: traktor + förare 800 kr/h
  ui.anLage = prefs.getInt("anlage", 1);

  uiDrawSplash(g);
  if (g == canvas) { applyBrightness(canvas->getFramebuffer(), 800 * 480, ui.brightness); canvas->flush(); }
  uint32_t splashStart = millis();

  touchInit();
  tempInit();                                   // v44: chippets temperatur
  ui.enh.tempC = ui.enh.tempMin = ui.enh.tempMax = NAN; ui.enh.canState = -1;
  xTaskCreatePinnedToCore(touchTask, "pek", 3072, nullptr, 4, nullptr, 1);   // v43: pekskärmen i egen task

  uint32_t boot = prefs.getUInt("boot", 0) + 1;
  prefs.putUInt("boot", boot);
  for (int i = 0; i < CHART_POINTS; i++) ui.trFuelMin[i] = NAN;
  ui.page = PG_IDLE;
  ui.setBack = -1;
  ui.manKap = -1; ui.manSida = 0; ui.manBack = PG_SETTINGS;     // manualen börjar på innehållet
  ui.kmh = NAN;
  ui.widthM = ui.manualWidth;
  ui.tcAddr = -1;
  ui.version = FW_VERSION;
  ui.autoPage = PG_TRAKTOR;
  ui.dubbel = dubbelAnnan() != nullptr;
  ui.kartaMpp = K_MPP_100M;

  loggerBegin(boot);
  redskapBegin();
  vtpoolBegin();
  vtviewBegin();
  analysBegin();
  uiVtRender = vtRenderHook;
  traktorBegin();
  kartaBegin();
  uiKartaRender = kartaRender;
  netExtraStep = kartaNetStep;
  netBeforeRestart = beforeRestart;
  netBegin();

  bool canOk = canStart();
  Serial.printf("Version %d  CAN: %s  SD: %s\n", FW_VERSION, canOk ? "OK" : "FEL", sdOk ? "OK" : "FEL");
  xTaskCreatePinnedToCore(canTask, "can", 4096, nullptr, 10, nullptr, 0);

  while (millis() - splashStart < 2500) delay(20);   // visa loggan en stund
  minuteStartMs = millis();
}

void loop() {
  static uint32_t lastUpd = 0, lastDraw = 0, lastResync = 0;
  uint32_t now = millis();

  handleTouch();
  netSerialPoll();                 // inställningar via USB-kabeln
  redskapLoop(now, tcAddr);        // välj redskap, tolka redskapsbeskrivning
  vtpoolLoop(now);                 // spara redskapets bild (VT-objektpool) på SD-kortet
  vtviewLoop(now);                 // följ redskapets kommandon till terminalen (speglingen)
  demoLoop(now);                   // simulerade meddelanden (bara i demoläget)
  {
    // Autoläget: ISOBUS-redskapet arbetar (DDI 141)
    int iso = rdInfo.addr >= 0 ? genImplWork : -1;
    kartaLoop(now, genImplWidth, iso);              // karta och areal
  }

  if (now - lastUpd >= 250) {
    float dt = lastUpd ? (now - lastUpd) / 1000.0f : 0.25f;
    if (dt > 2) dt = 2;
    lastUpd = now;
    updateState(dt);
  }
  {                                                // v44: temperaturen var 2:a sekund, enhetsinfon när den visas
    static uint32_t lastTemp = 0;
    if (now - lastTemp >= 2000) { lastTemp = now; tempRead(); }
    if (ui.page == PG_ENHET) enhFill(false);
  }
  bool vtNew = ui.page == PG_VT && vvDirty && now - lastDraw >= 200;   // speglingen: upp till 5 ggr/s
  if (dirty || vtNew || now - lastDraw >= 500) {   // 2 ggr/s räcker (värdena ändras ~1 ggr/s); pekning ritar direkt
    lastDraw = now;
    dirty = false;
    vvDirty = false;
    render();
    // v37: fältrapporten skapas i en egen task (falt.h) efter att "Skapar rapporten ..." har ritats
    static bool expRitad = false;
    if (ui.page == PG_EXPORT && ui.exp.status == 1 && rappLage != 1) {
      if (!expRitad) expRitad = true;
      else {
        expRitad = false;
        rappStarta(ui.exp, ui.net.namn, ui.anDiesel, ui.anTimKr, ui.anLage, ui.karta.widthM);
      }
    }
  }
  if (rappLage >= 2) {                                         // rapporten är klar (eller misslyckades)
    bool ok = rappLage == 2;
    rappLage = 0;
    if (ui.page == PG_EXPORT && ui.exp.status == 1) {
      int st = ok ? 2 : 3;
      ui.exp = rappE;
      if (ok) {
        strncpy(apNamn, ui.net.namn[0] ? ui.net.namn : "Faltloggen", sizeof(apNamn) - 1);
        strncpy(apFil, ui.exp.fil, sizeof(apFil) - 1);
        strncpy(ui.exp.ap, apNamn, sizeof(ui.exp.ap) - 1);
        reqApStart = true;
      }
      ui.exp.status = st;
      dirty = true;
    }
  }
  if (ui.page == PG_EXPORT) { ui.exp.stationer = apStationer; ui.exp.hamtade = apHamtade; }
  else if (apActive && !reqApStop) reqApStop = true;          // sidan lämnad (t.ex. vila): stäng eget WiFi
  // Synka om skärmbilden varje sekund. Vid skrivning till flashminnet (inställningar,
  // uppdatering) och mycket WiFi-trafik kan bilden annars glida och hamna förskjuten.
  // Omstarten sker vid nästa bildbyte och syns inte när bilden redan ligger rätt.
  if (now - lastResync >= 1000) {
    lastResync = now;
    esp_lcd_rgb_panel_restart(rgbpanel->panelHandle());
  }
  delay(5);
}
