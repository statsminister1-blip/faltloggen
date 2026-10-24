# För utvecklare

## Bygga programmet

- **Arduino**, ESP32-kärnan **3.0.7** (Espressif).
- Kort: *ESP32S3 Dev Module*, **PSRAM: OPI**, **Flash Size: 16 MB**, **Partition Scheme: 16M Flash (3MB APP/9.9MB FATFS)**.
- Bibliotek: **GFX Library for Arduino 1.5.0** (mappen ska heta `Arduino_GFX`) och **U8g2** (bara typsnitten används).
- **GFX-biblioteket ändras på två ställen** (annars "glider" bilden):
  1. `src/databus/Arduino_ESP32RGBPanel.cpp`: lägg till raden `.bounce_buffer_size_px = 800 * 20,` direkt efter
     `.num_fbs = 1,` i `panel_config`.
  2. `src/databus/Arduino_ESP32RGBPanel.h`: lägg till `esp_lcd_panel_handle_t panelHandle() { return _panel_handle; }`
     efter `getFrameBuffer` i klassens public-del.

```
arduino-cli compile --fqbn esp32:esp32:esp32s3:PSRAM=opi,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB \
  --build-path build --export-binaries src/Faltloggen
tools/merge.sh build firmware/Faltloggen_vN_usb.bin
```

## Släppa en ny version

1. Ändra koden i `src/Faltloggen/` och höj `FW_VERSION` i `version.h`.
2. Ändra manualen i `manual/MANUAL.md` **och** `manual/MANUAL_EN.md`, kör `python3 tools/manual/md2c.py`.
3. Bygg. Kopiera `build/Faltloggen.ino.bin` till `firmware/Faltloggen.bin` och gör USB-filen med `tools/merge.sh`
   (ta bort den gamla USB-filen).
4. Skriv samma nummer i `firmware/version.txt` **sist** – det är den som får skärmarna att fråga om uppdatering.
5. Skärmarna hämtar `firmware/version.txt` och `firmware/Faltloggen.bin` från grenen `main` på
   `raw.githubusercontent.com` (certifikatet kontrolleras). En egen kopia (fork) pekas ut med
   `uppdatering_arkiv=konto/arkiv` i CONFIG.TXT.

## Regler i koden

- Skärmen **lyssnar bara** (`TWAI_MODE_LISTEN_ONLY` i `Faltloggen.ino`). Ändra inte det.
- Alla texter som visas skrivs `L("svenska", "English")` (`lang.h`). Tal formateras med `uiFmt()`/`langDec()`
  så att decimaltecknet följer språket.
- Skärmen laddar inte upp något. Uppdateringar installeras aldrig utan att föraren trycker JA.
- Nya knappar: id i `enum` överst i `ui.h`, ritas med `s4Key`/`s4KeyT`, hanteras i `onButton()` i `Faltloggen.ino`.

## Filer

| Fil | Innehåll |
|---|---|
| `Faltloggen.ino` | huvudprogram, CAN-task, pekskärm, knappar |
| `hw.h` | I/O-expandern CH422G, pekskärmen GT911, stift |
| `logger.h` | SD-kortet, valfri inspelning av bussen |
| `net.h` | WiFi, frivilliga uppdateringar, eget WiFi för fältrapporten |
| `ui.h` | alla sidor (S4-stil, dag/natt) |
| `lang.h` | språket |
| `redskap.h` | ISOBUS-redskapet, redskapets beskrivning (DDOP) |
| `traktor.h` | traktorns egna meddelanden (hastighet, lyft, kraftuttag, ventiler, bränsle, slirning, belysning, GPS) |
| `karta.h` | karta, areal, kartbilder (Sentinel-2 cloudless 2016, EOX) |
| `analys.h` | bränsle- och kostnadsanalys |
| `falt.h` | fältrapport som PDF |
| `vtpool.h`, `vtview.h` | fångar och visar redskapets VT-bild (TERMINAL) |
| `demo.h` | demoläge med påhittade värden |
| `manual_data.h` | manualen (skapad av `tools/manual/md2c.py`) |
| `s4font.h` | DejaVu Sans som U8g2-typsnitt (`tools/s4font/`) |
| `logo_data.h`, `traktor_ovan.h` | loggan och kartans traktormarkör (`tools/genlogo.py`, `tools/gen_traktor_ovan.py`) |

## Hårdvaran

Waveshare ESP32-S3-Touch-LCD-7: 800 × 480 RGB-panel (pixelklocka 13 MHz), GT911 på I2C 0x5D (SDA 8, SCL 9),
CH422G-expander (EXIO1 pekreset, EXIO2 bakgrundsljus, EXIO3 LCD-reset, EXIO4 SD-kortets CS, EXIO5 CAN/USB-val,
hög = CAN), CAN TX 20 / RX 19 (USB-paddarna stängs av i koden), SD via SPI (MOSI 11, SCK 12, MISO 13).

## Vad traktorn skickar (Fendt Vario S4, kontrollerat mot loggar)

| PGN | Innehåll |
|---|---|
| FE49 / FE48 | hastighet mark / hjul (mm/s), sträcka, riktning; skillnaden = slirning |
| F004 | motorvarv |
| FE43 / FE44 | kraftuttag bak / fram (bara när det går) |
| FE45 / FE46 | lyft bak / fram; FE45 b3–4 = dragkraft i lyften |
| FE11–FE17 | hydraulventil 1–7 (7 = front) |
| FEF2 | bränsle l/h |
| FEE7 | timräknare |
| FE40 | belysning |
| 1F801 / 1F802 / 1F119 | GPS-position, kurs och lutning från styrsystemet (NMEA 2000 på ISOBUS) |

Skickas inte: vridmoment, kylvätsketemperatur, bränslenivå, AdBlue, fyrhjulsdrift, diffspärr.

## Prova sidorna på datorn

Se [tools/vt/pc/README.md](tools/vt/pc/README.md): skärmens riktiga ritkod kompileras på datorn och ritar
valfri sida till en bild, på svenska och engelska.
