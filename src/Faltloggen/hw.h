// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  hw.h – hårdvara på Waveshare ESP32-S3-Touch-LCD-7
//  Stift, IO-expandern CH422G och pekskärmen GT911
// ============================================================================
#pragma once
#include <Arduino.h>
#include <Wire.h>

// ---------------------------------------------------------------- Stift
#define I2C_SDA      8
#define I2C_SCL      9
#define TOUCH_INT    4
#define CAN_TX_PIN   20
#define CAN_RX_PIN   19
#define SD_MOSI      11
#define SD_SCK       12
#define SD_MISO      13
#define SD_DUMMY_CS  6     // SD-kortets riktiga CS sitter på CH422G (EXIO4)

// ---------------------------------------------------------------- CH422G
#define CH422G_ADDR_SET  0x24
#define CH422G_ADDR_IO   0x38
#define EXIO_TP_RST   1
#define EXIO_BL       2    // bakgrundsbelysning (bara av/på)
#define EXIO_LCD_RST  3
#define EXIO_SD_CS    4    // låg = SD-kortet valt
#define EXIO_CAN_SEL  5    // hög = CAN, låg = USB

static uint8_t exioState = 0;
static SemaphoreHandle_t i2cMutex = nullptr;

static void exioWrite() {
  Wire.beginTransmission(CH422G_ADDR_IO);
  Wire.write(exioState);
  Wire.endTransmission();
}
static void exioSet(uint8_t bit, bool on) {
  if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY);
  if (on) exioState |= (1 << bit); else exioState &= ~(1 << bit);
  exioWrite();
  if (i2cMutex) xSemaphoreGive(i2cMutex);
}
static void exioInit() {
  Wire.beginTransmission(CH422G_ADDR_SET);
  Wire.write(0x01);                 // IO0-7 som utgångar
  Wire.endTransmission();
  exioState = (1 << EXIO_TP_RST) | (1 << EXIO_BL) | (1 << EXIO_LCD_RST) | (1 << EXIO_CAN_SEL);
  // EXIO_SD_CS = 0 -> SD-kortet valt hela tiden (enda enheten på SPI-bussen)
  exioWrite();
}

// ---------------------------------------------------------------- GT911
static uint8_t gtAddr = 0x5D;
static bool    touchOk = false;
static char    touchInfo[48] = "ej startad";
static volatile uint32_t touchPresses = 0;
static volatile int touchLastX = -1, touchLastY = -1;

static bool gtWriteReg(uint16_t reg, const uint8_t *d, int n) {
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF);
  for (int i = 0; i < n; i++) Wire.write(d[i]);
  return Wire.endTransmission() == 0;
}
static bool gtReadReg(uint16_t reg, uint8_t *d, int n) {
  Wire.beginTransmission(gtAddr);
  Wire.write(reg >> 8); Wire.write(reg & 0xFF);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)gtAddr, n) != n) return false;
  for (int i = 0; i < n; i++) d[i] = Wire.read();
  return true;
}
static void touchInit() {
  // Reset med INT låg -> adress 0x5D
  pinMode(TOUCH_INT, OUTPUT);
  digitalWrite(TOUCH_INT, LOW);
  exioSet(EXIO_TP_RST, false);
  delay(20);
  exioSet(EXIO_TP_RST, true);
  delay(10);
  pinMode(TOUCH_INT, INPUT);
  delay(60);
  uint8_t id[4] = {0};
  gtAddr = 0x5D;
  touchOk = gtReadReg(0x8140, id, 4);
  if (!touchOk) { gtAddr = 0x14; touchOk = gtReadReg(0x8140, id, 4); }
  snprintf(touchInfo, sizeof(touchInfo), "%s adr 0x%02X id %c%c%c%c", touchOk ? "OK" : "FEL", gtAddr,
           id[0] ? id[0] : '-', id[1] ? id[1] : '-', id[2] ? id[2] : '-', id[3] ? id[3] : '-');
  Serial.printf("Pekskärm: %s\n", touchInfo);
}
// 1 = skärmen trycks (x/y i pixlar), 0 = pekskärmen rapporterar att fingret har släppt,
// -1 = ingen ny rapport (v43: släppet syns direkt i stället för efter 120 ms)
static int touchRead(int &x, int &y) {
  int pressed = -1;
  if (i2cMutex) xSemaphoreTake(i2cMutex, portMAX_DELAY);
  uint8_t st;
  if (gtReadReg(0x814E, &st, 1) && (st & 0x80)) {
    uint8_t n = st & 0x0F;
    pressed = 0;
    if (n > 0) {
      uint8_t p[7];
      pressed = -1;                         // läsfel = ingen ny rapport
      if (gtReadReg(0x8150, p, 7)) {
        // 0x8150 = X låg, 0x8151 = X hög, 0x8152 = Y låg, 0x8153 = Y hög
        x = p[0] | (p[1] << 8);
        y = p[2] | (p[3] << 8);
        if (x > 799) x = 799;
        if (y > 479) y = 479;
        pressed = 1;
      }
    }
    uint8_t z = 0;
    gtWriteReg(0x814E, &z, 1);
  }
  if (i2cMutex) xSemaphoreGive(i2cMutex);
  return pressed;
}
