// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  logger.h – inspelning av ISOBUS-trafiken på SD-kortet (valfri)
//
//  Loggning PÅ (inställningarna -> MER): all trafik spelas in i /CANnnnnn.LOG
//  (candump-format: "(sek.usek) can0 ID#DATA"), en fil per start. Bra vid felsökning
//  eller om du vill se vad din traktor skickar. Förval AV. Inget laddas upp någonstans.
//  Är kortet nästan fullt raderas den äldsta loggen (minst 2 GB hålls ledigt).
// ============================================================================
#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include "freertos/ringbuf.h"
#include "hw.h"

#define LOG_ALL_MAX_BYTES   (1000UL * 1024UL * 1024UL)   // ny fil efter ca 1 GB

static SemaphoreHandle_t sdMutex = nullptr;
static RingbufHandle_t ringAll = nullptr;

static volatile bool     sdOk = false;
static volatile uint32_t logDropped = 0;
// Loggning PÅ/AV. AV (förval) = ingen bussinspelning. Kartan, spåren, kartbilderna, redskapsbeskrivningar
// och redskapens bilder (VT-pooler) sparas ändå.
static volatile bool loggAv = true;
static uint32_t bootNo = 0;                 // startnummer (sätts från Preferences)
static char     allName[24] = "-";
static File     allFile;
static volatile uint32_t allBytesTotal = 0;

static void sdLock()   { xSemaphoreTake(sdMutex, portMAX_DELAY); }
static void sdUnlock() { xSemaphoreGive(sdMutex); }

static bool sdStart() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_DUMMY_CS);
  // 80 klockpulser med CS högt så att kortet går in i SPI-läge
  exioSet(EXIO_SD_CS, true);
  SPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 10; i++) SPI.transfer(0xFF);
  SPI.endTransaction();
  exioSet(EXIO_SD_CS, false);
  return SD.begin(SD_DUMMY_CS, SPI, 20000000);
}

static void openAllFile() {
  for (int i = 1; i < 100000; i++) {
    snprintf(allName, sizeof(allName), "/CAN%05d.LOG", i);
    if (!SD.exists(allName)) break;
  }
  allFile = SD.open(allName, FILE_WRITE);
  if (allFile) allFile.printf("# ISOBUS, all trafik, start %lu\n", (unsigned long)bootNo);
}

// Håll minst SD_MIN_FREE ledigt: radera äldsta fullständiga loggen (/CANnnnnn.LOG)
// (inte den som skrivs just nu). Körs med sdLock taget.
#define SD_MIN_FREE (2ULL * 1024 * 1024 * 1024)
static void sdHousekeeping() {
  for (int round = 0; round < 20; round++) {
    uint64_t freeB = SD.totalBytes() - SD.usedBytes();
    if (freeB >= SD_MIN_FREE) return;
    char oldest[40] = "";
    File root = SD.open("/");
    for (File f = root.openNextFile(); f; f = root.openNextFile()) {
      const char *n = f.name();
      if (!f.isDirectory() && !strncmp(n, "CAN", 3)) {
        char full[40]; snprintf(full, sizeof(full), "/%s", n);
        if (strcmp(full, allName) && (!oldest[0] || strcmp(full, oldest) < 0)) strncpy(oldest, full, sizeof(oldest) - 1);
      }
      f.close();
    }
    root.close();
    if (!oldest[0]) return;
    SD.remove(oldest);
    Serial.printf("[sd] lite plats kvar, raderade %s\n", oldest);
  }
}

// SD-tasken: tömmer ringbuffertarna till kortet
static void sdTask(void *) {
  uint32_t lastFlush = millis(), lastHk = 0;
  for (;;) {
    if (sdOk && millis() - lastHk > 10UL * 60UL * 1000UL) {   // var 10:e minut (och vid start)
      lastHk = millis();
      sdLock(); sdHousekeeping(); sdUnlock();
    }
    bool did = false;
    size_t len;
    char *d = (char *)xRingbufferReceiveUpTo(ringAll, &len, pdMS_TO_TICKS(20), 8192);
    if (d) {
      if (sdOk) {
        sdLock();
        if (!allFile) openAllFile();
        if (allFile) {
          size_t w = allFile.write((const uint8_t *)d, len);
          allBytesTotal += w;
          if (w != len) sdOk = false;
          if (allFile.size() > LOG_ALL_MAX_BYTES) { allFile.close(); openAllFile(); }
        }
        sdUnlock();
      }
      vRingbufferReturnItem(ringAll, d);
      did = true;
    }
    uint32_t now = millis();
    if (sdOk && now - lastFlush > 2000) {
      sdLock();
      if (allFile) allFile.flush();
      sdUnlock();
      lastFlush = now;
    }
    if (!did) vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// Stäng allt snyggt (före omstart vid uppdatering)
static void loggerFlushAll() {
  if (!sdOk) return;
  sdLock();
  if (allFile) allFile.flush();
  sdUnlock();
}

static void loggerBegin(uint32_t boot) {
  bootNo = boot;
  sdMutex = xSemaphoreCreateMutex();
  ringAll = xRingbufferCreate(96 * 1024, RINGBUF_TYPE_BYTEBUF);
  sdOk = sdStart();
  xTaskCreatePinnedToCore(sdTask, "sd", 6144, nullptr, 1, nullptr, 1);   // prioritet 1 = samma som skärmen, så att ritningen inte väntar på SD-kortet
}

// ---- Radera hela SD-kortet (kommandot "sd_rensa_JA" via USB). Allt raderas, även
//      sådant som inte skärmen skrivit. Inställningarna ligger i skärmens eget minne.
static uint32_t wipeCount = 0;
static void wipeDir(const char *path) {
  File dir = SD.open(path);
  if (!dir || !dir.isDirectory()) return;
  for (;;) {
    File f = dir.openNextFile();
    if (!f) break;
    char p[256];
    snprintf(p, sizeof(p), "%s%s%s", path, strcmp(path, "/") ? "/" : "", f.name());
    bool isDir = f.isDirectory();
    f.close();
    if (isDir) { wipeDir(p); SD.rmdir(p); }
    else { SD.remove(p); wipeCount++; }
    if ((wipeCount & 15) == 0) vTaskDelay(1);
  }
  dir.close();
}
static bool loggerWipeCard(uint32_t &files, uint64_t &freeBytes) {
  if (!sdOk) return false;
  sdLock();
  if (allFile) allFile.close();
  wipeCount = 0;
  // läs om katalogen tills den är tom (openNextFile kan hoppa över poster när man raderar)
  for (int pass = 0; pass < 5; pass++) {
    uint32_t before = wipeCount;
    wipeDir("/");
    if (wipeCount == before && pass > 0) break;
  }
  files = wipeCount;
  freeBytes = SD.totalBytes() - SD.usedBytes();
  sdUnlock();
  return true;
}

// ---- Formatera om hela kortet till ett FAT32 (kommandot "sd_formatera_JA").
//      Behövs när kortet har en liten partition (t.ex. från en Raspberry Pi) och resten är oanvänd.
static bool loggerFormatCard(uint64_t &cardMB, uint64_t &freeMB) {
  sdLock();
  if (allFile) allFile.close();
  static uint8_t sec[512];
  memset(sec, 0, sizeof(sec));
  bool ok = sdOk && SD.writeRAW(sec, 0);          // nolla partitionstabellen -> kortet ses som tomt
  SD.end();
  exioSet(EXIO_SD_CS, false);
  ok = SD.begin(SD_DUMMY_CS, SPI, 20000000, "/sd", 5, true);   // true = formatera om tomt
  sdOk = ok;
  cardMB = SD.cardSize() >> 20;
  freeMB = ok ? (SD.totalBytes() - SD.usedBytes()) >> 20 : 0;
  sdUnlock();
  return ok;
}

// Formatera en CAN-ram som candump-rad och lägg den i ringbufferten
static void loggerFrame(uint32_t id, bool extd, const uint8_t *data, uint8_t dlc) {
  if (loggAv) return;
  char line[64];
  int64_t us = esp_timer_get_time();
  int n = snprintf(line, sizeof(line), extd ? "(%lld.%06lld) can0 %08lX#" : "(%lld.%06lld) can0 %03lX#",
                   us / 1000000, us % 1000000, (unsigned long)id);
  static const char hex[] = "0123456789ABCDEF";
  for (int i = 0; i < dlc && i < 8; i++) {
    line[n++] = hex[data[i] >> 4];
    line[n++] = hex[data[i] & 0x0F];
  }
  line[n++] = '\n';
  if (xRingbufferSend(ringAll, line, n, 0) != pdTRUE) logDropped++;
}
