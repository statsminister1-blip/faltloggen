# Delar från andra (third-party)

Fältloggen innehåller eller använder följande. Alla licenser går att kombinera med GPLv3.

| Del | Var | Upphov | Licens |
|---|---|---|---|
| QR Code generator library (C) | `src/Faltloggen/qrcodegen.c/.h` | Project Nayuki | MIT (texten står överst i filerna) |
| GFX Library for Arduino v1.5.0 | byggs med (inte i arkivet), två små ändringar, se UTVECKLING.md | moononournation, bygger på Adafruit GFX | BSD – [LICENSES/Arduino_GFX-BSD.txt](LICENSES/Arduino_GFX-BSD.txt) |
| U8g2 (typsnittsformatet och några av dess typsnitt i `vtview.h`) | byggs med | olikraus | BSD 2-clause; typsnitten egna licenser – [LICENSES/U8g2-BSD-2-Clause.txt](LICENSES/U8g2-BSD-2-Clause.txt) |
| DejaVu Sans, omgjort till bildpunktstypsnitt | `src/Faltloggen/s4font.h` | DejaVu-projektet / Bitstream | Bitstream Vera – [LICENSES/Bitstream-Vera.txt](LICENSES/Bitstream-Vera.txt) |
| ESP32 Arduino-kärnan 3.0.7 och ESP-IDF | byggs med | Espressif | LGPL 2.1 eller senare / Apache 2.0 |
| Kartbild Sentinel-2 cloudless 2016 | hämtas av skärmen, finns inte i arkivet | EOX IT Services GmbH, https://s2maps.eu (innehåller bearbetade Copernicus Sentinel-data 2016) | CC BY 4.0 |

**Kartbilden:** senare årgångar av Sentinel-2 cloudless (2018 och framåt) är CC BY-NC-SA 4.0 och får inte
användas kommersiellt utan licens från EOX. Fältloggen använder därför 2016-årgången, som får användas fritt
med angiven källa. Källan står på kartan, i kartinställningarna och i fältrapporten.

**Loggan** (`bilder/logo.png`) och traktormarkören på kartan (`tools/gen_traktor_ovan.py`) är projektets egna.
Traktorsymbolen på traktorsidan är ritad i samma stil som symboler på traktorterminaler, men är egen kod.
