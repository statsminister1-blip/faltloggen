# Utskrivna delar

Licens: **CERN-OHL-S v2** (se [../LICENSES/CERN-OHL-S-2.0.txt](../LICENSES/CERN-OHL-S-2.0.txt)). Alla skal är för
**Waveshare ESP32-S3-Touch-LCD-7**. Skripten är Python med `trimesh` och skapar STL-filerna
(`SKAL_OUT=mapp python3 bygg_skal_snabbfaste.py`).

## Skal med snabbfäste (standard)

En enda del. Glaset fyller hela framsidan; väggen går runt glasets kant. Fästet på baksidan har samma hålbild
som mobilhållaren i Fendt-hytten (Richter/HR "4QuickFix").

| Del | Fil | Antal |
|---|---|---|
| Skalet | `skal7s_skal.stl` | 1 |
| Kabelklämma ISOBUS | `skal7s_klamma_isobus.stl` | 1 |
| Kabelklämma USB | `skal7s_klamma_usb.stl` | 1 |

Skript: `bygg_skal_snabbfaste.py`. Ritning sett framifrån: `skal7s_ritning.png`.

- **Status:** skärmen passar i skalet (provat). **Hålbilden för snabbfästet i den här versionen är inte provad
  mot ett riktigt fäste än** – skriv ut en provbit eller mät ditt fäste först.
- **Montering:** kablarna in genom hålen i nederkanten, klämmorna fast (2 × M3 vardera), sist läggs skärmen i
  framifrån och skruvas fast bakifrån med 4 × M3 × 8 i skärmens egna gängor.
- **Utskrift:** PETG eller ASA, baksidan nedåt, inga stöd. Klämmorna platt.
- USB-hålet är avlångt så att USB-C-kontakten går att trä in utifrån.

## Robust skal (alternativ)

Framskal med 5 mm vägg, baklock, TPU-stötkuddar, kabelklämmor och 38 mm RAM-kula. Skript: `bygg_skal_robust.py`.

| Del | Fil |
|---|---|
| Framskal | `skal7_framskal.stl` |
| Stötkuddar (TPU, samma projekt som framskalet) | `skal7_framskal_TPU.stl` |
| Baklock | `skal7_baklock.stl` |
| Kabelklämmor | `skal7_klamma_isobus.stl`, `skal7_klamma_usb.stl` |
| Blindplugg för USB-hålet (vid 12 V-matning) | `skal7_blindplugg_usb.stl` |
| RAM-kulfäste 38 mm | `RAM_kulfaste_38mm_v2.stl` |

Montering: skärmen skjuts in bakifrån i framskalet; baklocket skruvas med 4 × M3 i framskalets väggar och
4 × M3 i skärmens insatser; RAM-kulfästet med 4 × M4 och mutter. Ett DC/DC-kort (t.ex. Electrokit EKM006)
kan skjutas ner i spåren längs ena väggen. *Obs:* kontrollera på ritningen vilken sida ISOBUS- och USB-hålen
hamnar på innan du skriver ut – koordinaterna i skriptet är sett bakifrån.
