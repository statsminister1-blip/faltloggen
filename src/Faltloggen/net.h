// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  net.h – WiFi, frivilliga uppdateringar och skärmens eget WiFi (fältrapporten)
//
//  Inställningar:
//   - WiFi-nät väljs på skärmen (Inställningar -> WiFi) och sparas.
//   - Fler nät och skärmens namn kan läggas i /CONFIG.TXT på SD-kortet (eller skrivas
//     via USB-kabeln, 115200 baud, en rad "nyckel=värde" i taget):
//        wifi1_ssid=...
//        wifi1_pass=...
//        wifi2_ssid=...
//        wifi2_pass=...
//        namn=Traktor 1           (skärmens namn, visas på skärmen och som namn på eget WiFi)
//        valuta=kr                (valutan i bränsleanalysen och fältrapporten, t.ex. kr, EUR, £)
//        uppdatering=AV           (AV = leta aldrig efter uppdateringar, PA = fråga när en ny finns)
//        uppdatering_arkiv=agare/arkiv   (bara om du har en egen kopia av programmet)
//
//  Uppdateringar (frivilliga):
//   När skärmen har internet läser den firmware/version.txt i det publika arkivet på GitHub
//   (inget konto eller nyckel behövs). Finns en nyare version visas en fråga med ansvarstext.
//   Programmet hämtas och installeras BARA om föraren trycker JA. Föraren kan också stänga av
//   uppdateringarna helt – då kontaktar skärmen aldrig GitHub. Startar den nya versionen inte
//   ordentligt går skärmen själv tillbaka till förra versionen.
//
//  Skärmen laddar inte upp något: inga loggar, ingen position, inga rapporter.
// ============================================================================
#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <esp_mac.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>
#include "logger.h"
#include "version.h"
#include "lang.h"
#include "dubbel.h"

#define NET_SLOTS 3
#define SCAN_MAX  12

// Det publika arkivet som uppdateringarna hämtas från (kan ändras med uppdatering_arkiv=)
#define ARKIV_STANDARD "statsminister1-blip/faltloggen"
#define ARKIV_GREN     "main"
#define FW_FIL         "firmware/Faltloggen.bin"
#define VER_FIL        "firmware/version.txt"

struct NetStatus {
  bool     connected;
  char     ssid[33];
  char     ip[16];
  int      rssi;
  char     msg[64];          // senaste händelse (visas i inställningar)
  // nätsökning
  bool     scanning;
  int      nScan;
  char     scanSsid[SCAN_MAX][33];
  int8_t   scanRssi[SCAN_MAX];
  bool     scanSecure[SCAN_MAX];
  // uppdatering
  bool     otaActive;
  int      otaPct;
  int      latestVersion;    // -1 = okänt
  int      otaOffer;         // > 0 = frågan "Ny mjukvara tillgänglig" visas för denna version
  char     otaNote[48];      // senaste läget (inställningar)
};

static NetStatus net;
static portMUX_TYPE netMux = portMUX_INITIALIZER_UNLOCKED;

static char   cfgSsid[NET_SLOTS][33];
static char   cfgPass[NET_SLOTS][65];
static char   cfgArkiv[96] = ARKIV_STANDARD;
static char   devId[16] = "fl";
static char   cfgName[33] = "";      // skärmens namn (namn= i CONFIG.TXT eller via kabeln), tomt = inget namn
static volatile bool cfgUppd = true; // leta efter uppdateringar (fråga alltid innan installation)

static WiFiMulti *wifiMulti = nullptr;
static volatile bool reqScan = false;
static volatile bool reqNewNet = false;
static char   newSsid[33], newPass[65];
static volatile bool reqForget = false;
static volatile bool reqRebuild = false;
static void (*netBeforeRestart)() = nullptr;
static volatile bool netContactOk = false; // satt när GitHub svarat (för att godkänna ny version)
static volatile int  reqOtaVer = 0;       // föraren svarade JA: installera denna version
// Skärmens eget WiFi för fältrapporten
static volatile bool reqApStart = false, reqApStop = false, apActive = false;
static char apNamn[33] = "Faltloggen";
static char apFil[64] = "";                // senaste rapporten (i /RAPPORT/)
static volatile int apHamtade = 0, apStationer = 0;

static char bootOrsak[96] = "-";           // varför skärmen startade senast (sätts i setup, visas i enhetsinfon)
// Enhetsinfo: SD-kortets storlek mäts i nät-tasken (kan ta en stund på ett stort kort)
static volatile bool reqSdInfo = false;
static volatile bool sdInfoKand = false;
static volatile uint32_t sdInfoTotMB = 0, sdInfoFriMB = 0;
static volatile float enhTempC = NAN, enhTempMin = NAN, enhTempMax = NAN;
static void (*netExtraStep)() = nullptr;   // karta.h: hämtar kartrutor när WiFi finns

static void netMsg(const char *fmt, ...) {
  char b[64];
  va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof(b), fmt, ap); va_end(ap);
  portENTER_CRITICAL(&netMux);
  strncpy(net.msg, b, sizeof(net.msg) - 1);
  portEXIT_CRITICAL(&netMux);
  Serial.printf("[net] %s\n", b);
}

// Adressen dit feedback skickas (GitHub Issues i arkivet). Visas som QR-kod i inställningarna.
static void netFeedbackUrl(char *b, int n) { snprintf(b, n, "https://github.com/%s/issues", cfgArkiv); }

// ---------------------------------------------------------------- Inställningar
static void netLoadPrefs() {
  Preferences p;
  p.begin("natet", true);
  for (int i = 0; i < NET_SLOTS; i++) {
    char k[8];
    snprintf(k, sizeof(k), "w%ds", i); p.getString(k, cfgSsid[i], sizeof(cfgSsid[i]));
    snprintf(k, sizeof(k), "w%dp", i); p.getString(k, cfgPass[i], sizeof(cfgPass[i]));
  }
  if (p.isKey("arkiv")) p.getString("arkiv", cfgArkiv, sizeof(cfgArkiv));
  if (p.isKey("namn")) p.getString("namn", cfgName, sizeof(cfgName));
  if (p.isKey("valuta")) p.getString("valuta", valuta, sizeof(valuta));
  cfgUppd = p.getBool("uppd", true);
  p.end();
}
static void netSaveSlot(int i) {
  Preferences p;
  p.begin("natet", false);
  char k[8];
  snprintf(k, sizeof(k), "w%ds", i); p.putString(k, cfgSsid[i]);
  snprintf(k, sizeof(k), "w%dp", i); p.putString(k, cfgPass[i]);
  p.end();
}

// Uppdateringar PÅ (fråga när en ny version finns) / AV (kontakta aldrig GitHub). Sparas.
static void netSetUppd(bool pa) {
  cfgUppd = pa;
  Preferences p; p.begin("natet", false); p.putBool("uppd", pa); p.end();
  if (!pa) { net.otaOffer = 0; reqOtaVer = 0; }
}

static void trim(char *s) {
  int n = strlen(s);
  while (n > 0 && (s[n - 1] == '\r' || s[n - 1] == '\n' || s[n - 1] == ' ')) s[--n] = 0;
}

// Sätt en inställning (samma nycklar i /CONFIG.TXT och via USB-serieporten)
static bool netApplyKeyValue(const char *k, const char *v) {
  bool hit = false;
  Preferences p;
  p.begin("natet", false);
  for (int s = 1; s < NET_SLOTS; s++) {
    char ks[16], kp[16];
    snprintf(ks, sizeof(ks), "wifi%d_ssid", s);
    snprintf(kp, sizeof(kp), "wifi%d_pass", s);
    if (!strcmp(k, ks)) { strncpy(cfgSsid[s], v, 32); cfgSsid[s][32] = 0; hit = true; }
    if (!strcmp(k, kp)) { strncpy(cfgPass[s], v, 64); cfgPass[s][64] = 0; hit = true; }
  }
  if (!strcmp(k, "uppdatering_arkiv")) {
    strncpy(cfgArkiv, v[0] ? v : ARKIV_STANDARD, sizeof(cfgArkiv) - 1); cfgArkiv[sizeof(cfgArkiv) - 1] = 0;
    p.putString("arkiv", cfgArkiv); hit = true;
  }
  if (!strcmp(k, "uppdatering")) {
    bool pa = !(v[0] == 'A' || v[0] == 'a' || v[0] == '0' || v[0] == 'O' || v[0] == 'o');   // AV/av/0/OFF/off = av
    cfgUppd = pa; p.putBool("uppd", pa); hit = true;
  }
  if (!strcmp(k, "namn")) {
    strncpy(cfgName, v, sizeof(cfgName) - 1); cfgName[sizeof(cfgName) - 1] = 0;
    p.putString("namn", cfgName); hit = true;
  }
  if (!strcmp(k, "valuta")) {
    strncpy(valuta, v[0] ? v : "kr", sizeof(valuta) - 1); valuta[sizeof(valuta) - 1] = 0;
    p.putString("valuta", valuta); hit = true;
  }
  p.end();
  return hit;
}

// Läs /CONFIG.TXT från SD-kortet
static void netReadSdConfig() {
  if (!sdOk) return;
  sdLock();
  File f = SD.open("/CONFIG.TXT");
  if (!f) { sdUnlock(); return; }
  char line[200];
  while (f.available()) {
    int n = f.readBytesUntil('\n', line, sizeof(line) - 1);
    line[n] = 0; trim(line);
    char *eq = strchr(line, '=');
    if (!eq || line[0] == '#') continue;
    *eq = 0;
    netApplyKeyValue(line, eq + 1);
  }
  f.close();
  sdUnlock();
  for (int s = 1; s < NET_SLOTS; s++) netSaveSlot(s);
}

// Inställningar via USB-kabeln (UART-porten, 115200 baud): rader "nyckel=värde".
// "visa" skriver ut nuvarande inställningar (utan lösenord).
static void netSerialPoll() {
  static char line[200];
  static int n = 0;
  while (Serial.available()) {
    int c = Serial.read();
    if (c == '\r') continue;
    if (c != '\n') { if (n < (int)sizeof(line) - 1) line[n++] = (char)c; continue; }
    line[n] = 0; n = 0;
    if (!strcmp(line, "visa") || !strcmp(line, "show")) {
      Serial.printf("enhet=%s\nnamn=%s\nversion=%d\n", devId, cfgName, FW_VERSION);
      for (int i = 0; i < NET_SLOTS; i++) Serial.printf("wifi%d_ssid=%s\n", i, cfgSsid[i]);
      Serial.printf("uppdatering=%s\nuppdatering_arkiv=%s\nvaluta=%s\n", cfgUppd ? "PA" : "AV", cfgArkiv, valuta);
      Serial.printf("wifi=%s\n", WiFi.status() == WL_CONNECTED ? "ansluten" : "inte ansluten");
      continue;
    }
    if (!strcmp(line, "sd")) {
      if (!sdOk) { Serial.println("sd=saknas"); continue; }
      sdLock();
      uint64_t tot = SD.totalBytes(), used = SD.usedBytes();
      sdUnlock();
      Serial.printf("sd=OK kort %llu MB, filsystem %llu MB, ledigt %llu MB\n", SD.cardSize() >> 20, tot >> 20, (tot - used) >> 20);
      continue;
    }
    if (!strcmp(line, "sd_rensa_JA")) {
      Serial.println("Raderar SD-kortet ...");
      uint32_t files = 0; uint64_t freeB = 0;
      if (loggerWipeCard(files, freeB)) Serial.printf("SD-kortet raderat: %lu filer, ledigt %llu MB\n", (unsigned long)files, freeB >> 20);
      else Serial.println("FEL: inget SD-kort");
      continue;
    }
    if (!strcmp(line, "sd_formatera_JA")) {
      Serial.println("Formaterar SD-kortet (kan ta en halv minut) ...");
      uint64_t cardMB = 0, freeMB = 0;
      if (loggerFormatCard(cardMB, freeMB)) Serial.printf("SD-kortet formaterat: kort %llu MB, ledigt %llu MB\n", cardMB, freeMB);
      else Serial.println("FEL: formateringen misslyckades");
      continue;
    }
    char *eq = strchr(line, '=');
    if (!eq) { Serial.println("FEL: skriv nyckel=varde, visa, sd, sd_rensa_JA eller sd_formatera_JA"); continue; }
    *eq = 0;
    if (netApplyKeyValue(line, eq + 1)) {
      for (int s = 1; s < NET_SLOTS; s++) netSaveSlot(s);
      reqRebuild = true;
      Serial.printf("OK %s\n", line);
    } else Serial.printf("FEL: okand nyckel %s\n", line);
  }
}

static void netRebuildMulti() {
  if (wifiMulti) delete wifiMulti;
  wifiMulti = new WiFiMulti();
  for (int i = 0; i < NET_SLOTS; i++)
    if (cfgSsid[i][0]) wifiMulti->addAP(cfgSsid[i], cfgPass[i][0] ? cfgPass[i] : nullptr);
}

// ---------------------------------------------------------------- Hämta från det publika arkivet
// raw.githubusercontent.com, kontrollerat med certifikaten i ESP32-kärnans inbyggda rotcertifikatsbunt,
// så att programmet bara kan komma från GitHub.
extern const uint8_t x509_crt_bundle_start[] asm("_binary_x509_crt_bundle_start");
extern const uint8_t x509_crt_bundle_end[] asm("_binary_x509_crt_bundle_end");
static void ghSecure(WiFiClientSecure &c) {
  c.setCACertBundle(x509_crt_bundle_start, x509_crt_bundle_end - x509_crt_bundle_start);
  c.setTimeout(20000);
}
static void ghUrl(char *b, int n, const char *fil) {
  snprintf(b, n, "https://raw.githubusercontent.com/%s/%s/%s", cfgArkiv, ARKIV_GREN, fil);
}

// ---------------------------------------------------------------- Uppdatering (OTA)
static void otaNote(const char *fmt, ...) {
  char b[48];
  va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof(b), fmt, ap); va_end(ap);
  portENTER_CRITICAL(&netMux);
  strncpy(net.otaNote, b, sizeof(net.otaNote) - 1);
  portEXIT_CRITICAL(&netMux);
}

// Läs firmware/version.txt. Returnerar senaste versionen, eller -1 om GitHub inte svarade.
static int otaCheck() {
  char url[200]; ghUrl(url, sizeof(url), VER_FIL);
  WiFiClientSecure c; ghSecure(c);
  HTTPClient http;
  http.setTimeout(20000);
  if (!http.begin(c, url)) { netMsg("Versionskoll: ingen kontakt"); return -1; }
  http.setUserAgent("Faltloggen/" "ESP32");
  int code = http.GET();
  String body = code == 200 ? http.getString() : String();
  http.end();
  if (code != 200) { netMsg("Versionskoll misslyckades (%d)", code); return -1; }
  int latest = body.toInt();
  if (latest <= 0) { netMsg("Versionsfilen är trasig"); return -1; }
  net.latestVersion = latest;
  netContactOk = true;
  return latest;
}

// Hämta och installera programmet (bara efter JA på skärmen)
static void otaInstall(int latest) {
  netMsg("Hämtar version %d ...", latest);
  otaNote("v%d - installerar", latest);
  char url[200]; ghUrl(url, sizeof(url), FW_FIL);
  WiFiClientSecure c; ghSecure(c);
  HTTPClient http;
  http.setTimeout(20000);
  if (!http.begin(c, url)) { netMsg("Hämtning misslyckades (ingen kontakt)"); otaNote("v%d - hämtning misslyckades", latest); return; }
  http.setUserAgent("Faltloggen/" "ESP32");
  int hcode = http.GET();
  long clen = http.getSize();
  if (hcode != 200 || clen < 100000) {
    netMsg("Hämtning misslyckades (%d)", hcode); otaNote("v%d - hämtning misslyckades (%d)", latest, hcode);
    http.end(); return;
  }
  WiFiClient *st = http.getStreamPtr();
  net.otaActive = true;
  net.otaPct = 0;
  const esp_partition_t *plats = dubbelUppdPlats();     // vanligtvis "nästa" plats (se dubbel.h)
  esp_ota_handle_t oh = 0;
  if (!plats || esp_ota_begin(plats, clen, &oh) != ESP_OK) {
    netMsg("Uppdatering kunde inte starta"); otaNote("v%d - kunde inte starta", latest);
    net.otaActive = false; http.end(); return;
  }
  static uint8_t buf[2048];
  long got = 0;
  uint32_t t0 = millis();
  while (got < clen && millis() - t0 < 300000) {
    int a = st->available();
    if (a <= 0) { if (!http.connected()) break; delay(5); continue; }
    int n = st->read(buf, min(a, (int)sizeof(buf)));
    if (n <= 0) continue;
    if (esp_ota_write(oh, buf, n) != ESP_OK) break;
    got += n;
    net.otaPct = (int)(got * 100 / clen);
  }
  http.end();
  bool helt = got == clen && esp_ota_end(oh) == ESP_OK;   // esp_ota_end kontrollerar att programmet är helt
  if (helt && esp_ota_set_boot_partition(plats) == ESP_OK) {
    netMsg("Version %d installerad, startar om", latest);
    if (netBeforeRestart) netBeforeRestart();
    delay(500);
    ESP.restart();
  } else {
    if (!helt) esp_ota_abort(oh);
    netMsg("Uppdatering avbruten (%ld av %ld)", got, clen);
    otaNote("v%d - avbruten", latest);
    net.otaActive = false;
  }
}

// ---------------------------------------------------------------- Nät-tasken
static void doScan() {
  net.scanning = true;
  int n = WiFi.scanNetworks(false, false);
  int k = 0;
  for (int i = 0; i < n && k < SCAN_MAX; i++) {
    String s = WiFi.SSID(i);
    if (s.length() == 0) continue;
    bool dup = false;
    for (int j = 0; j < k; j++) if (s == net.scanSsid[j]) { dup = true; if (WiFi.RSSI(i) > net.scanRssi[j]) net.scanRssi[j] = WiFi.RSSI(i); }
    if (dup) continue;
    strncpy(net.scanSsid[k], s.c_str(), 32); net.scanSsid[k][32] = 0;
    net.scanRssi[k] = WiFi.RSSI(i);
    net.scanSecure[k] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
    k++;
  }
  // sortera på signalstyrka
  for (int a = 0; a < k; a++) for (int b = a + 1; b < k; b++) if (net.scanRssi[b] > net.scanRssi[a]) {
    char ts[33]; strcpy(ts, net.scanSsid[a]); strcpy(net.scanSsid[a], net.scanSsid[b]); strcpy(net.scanSsid[b], ts);
    int8_t tr = net.scanRssi[a]; net.scanRssi[a] = net.scanRssi[b]; net.scanRssi[b] = tr;
    bool tb = net.scanSecure[a]; net.scanSecure[a] = net.scanSecure[b]; net.scanSecure[b] = tb;
  }
  WiFi.scanDelete();
  net.nScan = k;
  net.scanning = false;
}

// ---------------------------------------------------------------- Fältrapporten: eget WiFi + webbsida
// Telefonen ansluter till skärmens nät (inget lösenord) och öppnar http://192.168.4.1. Alla namn pekar hit (DNS),
// och telefonernas internetkoll får "OK" så att de behåller anslutningen utan att öppna en inloggningssida.
static WebServer *apWeb = nullptr;
static DNSServer *apDns = nullptr;

static void apSkickaFil(const char *namn) {
  char p[96]; snprintf(p, sizeof(p), "/RAPPORT/%s", namn);
  if (strstr(namn, "..") || strchr(namn, '/')) { apWeb->send(404, "text/plain", "-"); return; }
  sdLock(); File f = SD.open(p); size_t n = f ? f.size() : 0; sdUnlock();
  if (!f) { apWeb->send(404, "text/plain", "-"); return; }
  char cd[120]; snprintf(cd, sizeof(cd), "attachment; filename=\"%s\"", namn);
  apWeb->sendHeader("Content-Disposition", cd);
  apWeb->setContentLength(n);
  apWeb->send(200, "application/pdf", "");
  WiFiClient c = apWeb->client();
  static uint8_t buf[4096];
  size_t done = 0;
  while (done < n && c.connected()) {
    sdLock(); size_t k = f.read(buf, sizeof(buf)); sdUnlock();
    if (!k) break;
    size_t sent = 0;
    while (sent < k && c.connected()) { size_t w = c.write(buf + sent, k - sent); if (!w) { delay(2); continue; } sent += w; }
    done += k;
  }
  sdLock(); f.close(); sdUnlock();
  if (done == n) apHamtade++;
}
static void apSida() {
  String h = F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
               "<style>body{font-family:sans-serif;background:#eef0f1;margin:0;padding:16px;color:#111}"
               "h1{font-size:22px;background:#c4dcf0;padding:8px 12px;margin:0 0 12px}"
               "a.k{display:block;background:#289628;color:#fff;text-align:center;padding:18px;font-size:20px;font-weight:bold;"
               "text-decoration:none;border-radius:8px;margin:12px 0}li{margin:8px 0}a{color:#0040c0}</style>");
  h += "<title>"; h += L("Fältrapport", "Field report"); h += "</title></head><body><h1>";
  h += L("Fältrapport", "Field report"); h += "</h1>";
  if (apFil[0]) {
    h += F("<a class='k' href='/r/"); h += apFil; h += F("' download>");
    h += L("LADDA NER RAPPORTEN (PDF)", "DOWNLOAD THE REPORT (PDF)");
    h += F("</a><p>"); h += apFil; h += F("</p>");
  }
  h += "<p>"; h += L("Tidigare rapporter på skärmen:", "Earlier reports on the display:"); h += "</p><ul>";
  sdLock();
  File d = SD.open("/RAPPORT");
  int n = 0;
  if (d) {
    for (File e = d.openNextFile(); e && n < 40; e = d.openNextFile()) {
      if (!e.isDirectory()) { String nm = e.name(); if (nm.endsWith(".pdf")) { h += F("<li><a href='/r/"); h += nm; h += F("' download>"); h += nm; h += F("</a></li>"); n++; } }
      e.close();
    }
    d.close();
  }
  sdUnlock();
  h += "</ul><p>"; h += L("Stäng sedan med STÄNG på skärmen.", "Then close with CLOSE on the display."); h += "</p></body></html>";
  apWeb->send(200, "text/html; charset=utf-8", h);
}
static void apStarta() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apNamn, nullptr, WiFi.status() == WL_CONNECTED ? WiFi.channel() : 6, 0, 4);
  if (!apDns) apDns = new DNSServer();
  apDns->start(53, "*", IPAddress(192, 168, 4, 1));
  if (!apWeb) {
    apWeb = new WebServer(80);
    apWeb->on("/", apSida);
    apWeb->on("/generate_204", []() { apWeb->send(204, "text/plain", ""); });          // Android: internet finns
    apWeb->on("/gen_204", []() { apWeb->send(204, "text/plain", ""); });
    apWeb->on("/hotspot-detect.html", []() { apWeb->send(200, "text/html", "<HTML><HEAD><TITLE>Success</TITLE></HEAD><BODY>Success</BODY></HTML>"); });
    apWeb->on("/ncsi.txt", []() { apWeb->send(200, "text/plain", "Microsoft NCSI"); });
    apWeb->onNotFound([]() {
      String u = apWeb->uri();
      if (u.startsWith("/r/")) { apSkickaFil(u.c_str() + 3); return; }
      apWeb->sendHeader("Location", "http://192.168.4.1/");
      apWeb->send(302, "text/plain", "");
    });
  }
  apWeb->begin();
  apActive = true; apHamtade = 0;
  netMsg("Eget WiFi %s på", apNamn);
}
static void apStoppa() {
  if (apWeb) apWeb->stop();
  if (apDns) apDns->stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  apActive = false; apStationer = 0;
  netMsg("Eget WiFi av");
}

static void netTask(void *) {
  uint32_t lastOta = 0;
  uint32_t connectedSince = 0;
  bool askedThisSession = false;            // versionen kollad (och ev. frågan ställd) sedan internet kom
  uint32_t lastSlow = 0;
  bool ntpStartad = false;
  for (;;) {
    // Eget WiFi för fältrapporten – webbsidan sköts ofta, resten en gång per sekund
    if (reqApStart) { reqApStart = false; if (!apActive) apStarta(); }
    if (reqApStop) { reqApStop = false; if (apActive) apStoppa(); }
    if (apActive) {
      apDns->processNextRequest();
      apWeb->handleClient();
      apStationer = WiFi.softAPgetStationNum();
      if (millis() - lastSlow < 1000) { vTaskDelay(pdMS_TO_TICKS(5)); continue; }
    }
    lastSlow = millis();
    if (reqScan) { reqScan = false; doScan(); }
    if (reqSdInfo) {                         // enhetsinfon vill veta SD-kortets storlek
      reqSdInfo = false;
      if (sdOk) {
        sdLock(); uint64_t tot = SD.totalBytes(), used = SD.usedBytes(); sdUnlock();
        sdInfoTotMB = tot >> 20; sdInfoFriMB = (tot - used) >> 20; sdInfoKand = true;
      }
    }
    if (reqNewNet) {
      reqNewNet = false;
      strncpy(cfgSsid[0], newSsid, 32); strncpy(cfgPass[0], newPass, 64);
      netSaveSlot(0);
      netRebuildMulti();
      WiFi.disconnect();
      netMsg("Ansluter till %s ...", cfgSsid[0]);
    }
    if (reqForget) {
      reqForget = false;
      for (int i = 0; i < NET_SLOTS; i++) { cfgSsid[i][0] = 0; cfgPass[i][0] = 0; netSaveSlot(i); }
      netRebuildMulti();
      WiFi.disconnect();
      netMsg("Alla sparade nät borttagna");
    }

    if (reqRebuild) { reqRebuild = false; netRebuildMulti(); WiFi.disconnect(); }
    bool conn = WiFi.status() == WL_CONNECTED;
    if (!conn) {
      net.connected = false;
      connectedSince = 0;
      askedThisSession = false;               // internet borta -> fråga igen när det kommer tillbaka
      net.otaOffer = 0;
      reqOtaVer = 0;
      bool any = false;
      for (int i = 0; i < NET_SLOTS; i++) if (cfgSsid[i][0]) any = true;
      if (any && !net.scanning && !apActive) wifiMulti->run(8000);   // inte medan eget WiFi är på (byter kanal)
    } else {
      if (!connectedSince) connectedSince = millis();
      // Läs WiFi-uppgifterna FÖRST – de anropen väntar på WiFi-drivrutinen och får aldrig
      // göras inne i en låst sektion (avbrotten är avstängda där -> vakthunden startar om).
      String ssid = WiFi.SSID();
      String ip = WiFi.localIP().toString();
      int8_t rssi = WiFi.RSSI();
      portENTER_CRITICAL(&netMux);
      net.connected = true;
      strncpy(net.ssid, ssid.c_str(), 32);
      strncpy(net.ip, ip.c_str(), 15);
      net.rssi = rssi;
      portEXIT_CRITICAL(&netMux);
      if (!ntpStartad) { ntpStartad = true; configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.google.com"); }   // klockan
      uint32_t now = millis();
      // Uppdateringar (bara om föraren inte stängt av dem): en koll när internet kommit. Misslyckas den provas
      // igen efter en minut. NEJ = ingen fråga förrän efter omstart eller nästa gång internet kommer tillbaka.
      if (cfgUppd && !apActive && now - connectedSince > 5000) {
        if (!askedThisSession && (!lastOta || now - lastOta > 60000UL)) {
          lastOta = now;
          int latest = otaCheck();
          if (latest > 0) {
            askedThisSession = true;
            if (latest > FW_VERSION) { net.otaOffer = latest; otaNote("v%d", latest); }
            else otaNote("-");
          }
        }
        if (reqOtaVer) {
          int v = reqOtaVer;
          reqOtaVer = 0;
          if (v > FW_VERSION) otaInstall(v);       // startar om om det lyckas
        }
      }
      // Kartrutor (satellitbild) – en åt gången
      if (netExtraStep && !net.otaActive && !apActive && now - connectedSince > 10000) {
        netExtraStep();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(apActive ? 5 : 1000));
  }
}

static void netBegin() {
  memset(&net, 0, sizeof(net));
  net.latestVersion = -1;
  netLoadPrefs();
  netReadSdConfig();
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);          // fungerar även innan WiFi har startat
  snprintf(devId, sizeof(devId), "fl-%02x%02x%02x", mac[3], mac[4], mac[5]);
  netRebuildMulti();
  xTaskCreatePinnedToCore(netTask, "net", 16384, nullptr, 2, nullptr, 0);
}

// Anrop från skärmen
static void netRequestScan() { reqScan = true; net.scanning = true; }
static void netConnectTo(const char *ssid, const char *pass) {
  strncpy(newSsid, ssid, 32); newSsid[32] = 0;
  strncpy(newPass, pass, 64); newPass[64] = 0;
  reqNewNet = true;
}
static void netForgetAll() { reqForget = true; }
static bool netHasSaved() { for (int i = 0; i < NET_SLOTS; i++) if (cfgSsid[i][0]) return true; return false; }
// Svar på frågan "Ny mjukvara tillgänglig". Rutan stängs direkt; installationen görs av nät-tasken.
static void netOtaJa() {
  int v = net.otaOffer;
  net.otaOffer = 0;
  if (v > 0) reqOtaVer = v;
}
static void netOtaNej() {
  int v = net.otaOffer;
  net.otaOffer = 0;
  if (v > 0) otaNote("v%d", v);
}
