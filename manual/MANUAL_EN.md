# Fältloggen – manual (English)

The same text is on the display: Settings → MANUAL (with ENGLISH chosen). Swedish version: `MANUAL.md`.
When building, `tools/manual/md2c.py` turns both files into `src/Faltloggen/manual_data.h` – edit here, not there.
Both languages must have the same number of chapters in the same order.
Lines in *italics* are notes for developers and are not shown on the display.

---

## 1. What Fältloggen is

Fältloggen ("the field log") is an extra display for the tractor cab. It connects to the cab's ISOBUS
socket and shows what the tractor and the implement are doing: speed, engine, hitch, PTO, hydraulics,
fuel, wheel slip and where you have driven.

The display **only listens**. It never sends anything on the bus and cannot control the tractor or the
implement. Everything you press only affects Fältloggen itself.

Fältloggen is free and open source. It is made by a private person in spare time, entirely without
involvement from Fendt, AGCO or any other manufacturer. Everything is used at your own risk – read
chapter 15.

It has been tested on Fendt Vario S4 (720 and 828). Other tractors may send other things; whatever is not
heard is shown as "--".

---

## 2. The buttons on the left

The tractor page and the implement page have the same column of buttons on the far left, from the top:

1. **The page's own buttons** (two places) – on the tractor page **Info⁺ Three-point hitch** and
   **Info⁺ Hydraulic valves**, on the implement page **TERMINAL**.
2. **The moon / sun** – switches between day and night colours. Saved.
3. **The house** – the start choice: TRACTOR, ISOBUS or TERMINAL.
4. **The spanner** – settings.

The chosen button has a red frame.

Two things open by tapping the box itself:
- **The map box** → full-screen map.
- **The fuel chart** → fuel analysis.

---

## 3. The start choice

When the display finds an ISOBUS implement it asks what you want to see:

- **TRACTOR** – the tractor page.
- **ISOBUS** – the implement page with the implement's own values.
- **TERMINAL** – the implement's own screen, as on the tractor terminal (when it has been captured).

If you do not answer, the display picks the implement after 20 seconds. If no implement is found it goes
straight to the tractor page.

If you open the start choice with **the house** it never picks by itself. Without an implement the
ISOBUS button is grey with the text "no implement found".

---

## 4. The tractor page

Four boxes:

**Top left – Info⁺** (chosen with the buttons on the left):
- **Three-point hitch:** position in percent front and rear, PTO speeds, WORK when the hitch is in work position.
- **Hydraulic valves:** every valve the tractor sends, with the flow out (+) or in (−). The names and colours
  (Yellow, Blue, Red, Green, Brown, Front) follow Fendt's order – if they do not match your tractor, go by the number.

**Top right – Tractor:**
- The blue strip: green dot = the bus is heard, WiFi, **hour meter**, **slope** front/rear and sideways (degrees).
- The tractor symbol: the hitch arms move with the hitch (green when working), the gears turn green when
  the PTOs run. The symbol tilts with the slope and shows lights that are on (headlights, work lights,
  indicators, brake lights, reversing light). The beacon on the cab roof turns orange and "rotates" when on.
- The rows: **speed** and **direction** (FWD green / REV), **engine speed** and **fuel l/h**,
  **wheel slip** and **draft force**.

**Bottom left – Map:** where you are and what has been covered. The area in the white box, green ring when
the map is logging, l/ha at the bottom left. Tap for full screen.

**Bottom right – Fuel:** l/h and l/min now, and a bar chart with one average per minute for the last
30 minutes. Tap for the fuel analysis.

---

## 5. Wheel slip and draft force

**Wheel slip (%)** is calculated by Fältloggen: how much faster the wheels turn than the tractor really
moves (wheel speed compared with ground speed). It is smoothed over a few seconds and only shown above 1.5 km/h.
- The field turns **yellow from 20 %** and **red from 30 %**.
- During transport it is normally a few percent.

**Draft force (kN)** comes from the force sensors in the rear hitch lower links. It is therefore **only
seen when the implement hangs in the hitch** – an implement on the drawbar or hitch hook is not seen here.
Use the wheel slip instead.
- The arrow grows with the force. The field turns **yellow from 35 kN** and **red from 50 kN**.

Both are estimates from the tractor's own sensors. Trust the tractor's instruments if they differ.

---

## 6. The implement page and TERMINAL

**The implement page** shows the values an ISOBUS implement sends to the tractor's Task Controller, with
the implement's own names and units as the implement itself describes them. At the top are speed,
working width and work state. If the list is long the button on the left turns the page.

**TERMINAL** shows the implement's own screen, the same as on the tractor terminal, updated all the time.
It is **only a picture** – the buttons cannot be pressed here. Use the tractor terminal to control the
implement.

The picture is captured the first time the implement uploads it to the terminal and is saved on the SD
card. If the terminal already has it stored it is not sent – delete the implement's stored pool in the
terminal and restart.

---

## 7. The full-screen map

**Buttons on the right:** back · **+** / **100 m** / **−** (zoom) · **START / STOP** (manual logging) ·
the spanner (map settings) · **RESET** · **FOLLOW** (shown when the map has been moved).

- **Drag with a finger** to move the map. **FOLLOW** centres on the tractor again.
- North is always up. The tractor is the small marker; the implement is drawn 3 m behind the antenna.
- **The Area box:** area worked in hectares, fuel in litres and l/ha since the last RESET, and the
  button **EXPORT REPORT** (chapter 9).
- **Field area:** press **FIELD AREA** and type the area in hectares with up to two decimals. The box
  then shows how many hectares are left, how long it will take and when it will be done. The time is
  calculated from the last approx. 20 minutes of logging and is shown after about 3 minutes of work.
  The clock time needs the display to know the time (internet or GPS). RESET removes the field area.
- **The tab with the arrow** folds the boxes in and out so the map is easier to see.
- **RESET** removes tracks, area, fuel and colours (the display asks first). Do it when you start a new field.

Overlapping passes are only counted once in the area. The map needs a GPS position from the tractor's
guidance system on ISOBUS.

---

## 8. Track colour

The **Track colour** box under Area chooses what the track is coloured by:

- **FUEL** – litres per hectare.
- **DRAFT** – draft force in the hitch (kN).
- **SLIP** – wheel slip (%).

The colours are compared with **the run's own average** since the last RESET:
- **blue** = like the average
- **green** = 30 % lower
- **red** = 30 % higher

The numbers under the scale show what fully green, the average and fully red are right now. The colours
are saved with the track and remain after a restart. Below 1.5 km/h no colour is painted.

---

## 9. Field report to the phone

1. Full-screen map → **EXPORT REPORT**.
2. Type the field name, or press **SKIP** (the name is then the date + the display's name).
3. Wait a few seconds while the report is created.
4. Scan **QR code 1** with the phone – it joins the display's own WiFi (no password).
5. Scan **QR code 2** (or open http://192.168.4.1) and press **DOWNLOAD**.
6. Press **CLOSE** when you are done.

The report is a PDF with three maps (fuel, wheel slip, draft force) and area, fuel, l/ha, l/h, time in the
field and working, standing still, distance, average speed, ha/h and cost per hectare.

**The field** = the largest connected area you have driven since RESET. Odd logging far away does not
count. Turns up to 30 seconds count as work.

Works out in the field without internet. The report is also saved on the SD card (folder RAPPORT). It is
not sent anywhere else.

---

## 10. Map settings

The spanner on the full-screen map:

- **Trigger** – what starts and stops logging:
  - **ISOBUS (auto)** (default): logs when the ISOBUS implement says it is working.
  - **Valve**, rear/front **PTO**, rear/front **hitch**, or **manual** with START/STOP.
- **Starts / Stops** – which event starts and stops (e.g. hitch DOWN / UP).
- **Working width** – **AUTO** takes the width from the implement. **MANUAL**: set with − / +, or **tap the
  value** and type it exactly in centimetres. The manual width is also used when AUTO is chosen but the
  implement sends no width.
- **Background** – **SATELLITE** (satellite image) or **PLAIN** (light yellow, like a guidance screen).

The satellite images are downloaded automatically when the display has WiFi and GPS, 6 km around the place,
and saved on the SD card. Source: Sentinel-2 cloudless 2016 by EOX IT Services GmbH (CC BY 4.0).

---

## 11. The fuel analysis

Tap the fuel chart. The analysis covers **the last 30 minutes** of work.

- **Top:** average speed, l/h, l/ha, cost per hectare and capacity ha/h.
- **The bars:** cost per hectare for every speed (1 km/h steps), with l/ha below.
  **Green** = best value, **yellow** = lowest fuel use.
- **Recommendation** at the bottom, also about engine speed.
- **Mode** on the left: **ECONOMY** (time counts half), **BALANCE** (at its cost), **HURRY**
  (time counts double, e.g. rain on the way).
- **PRICES:** diesel and tractor + driver. Saved. The currency is set with `valuta=` in CONFIG.TXT.

Working = when the map is logging. Turns up to 30 s count as work. It is an estimate – terrain, load and
turns all matter.

---

## 12. Settings

The spanner:

- **Brightness** – dims the colours.
- **Day / night** – light or dark colours.
- **Language** – SVENSKA or ENGLISH.
- **Working width** – AUTO or MANUAL; tap the value to type centimetres.
- **WiFi → CHOOSE** – choose a network and type the password on the display's keyboard.
- **ISOBUS:** **TRACTOR** (tractor page), **IMPLEMENT** (implement page), **DIAGNOSTICS** (what the bus sends).
- **DEMO** – shows made-up values to try the display. Nothing is saved and the map does not log.
  Turned off with DEMO OFF, by a restart or when real traffic arrives.
- **The info line** – the display's name, version and SD card.
- **MANUAL** – this manual. Tap a chapter; **<** and **>** turn pages, **CONTENTS** goes back.
- **DEVICE** – the chip temperature, memory, SD card, uptime, why the display last restarted, WiFi and
  ISOBUS traffic.
- **MORE** – updates, logging, feedback and liability & licences (chapters 13 and 15).
- **The tick** at the top right goes back to the page you came from.

The display's name and more WiFi networks can be put in the file **CONFIG.TXT** on the SD card (see README).

---

## 13. More: updates, logging and feedback

**Updates** – **ASK** (default) or **OFF**.
- When the display starts or gets internet it checks whether a new version is in the public repository on GitHub.
- If there is, a box with a liability text is shown. **Nothing is installed unless you press YES, INSTALL.**
  **NOT NOW** asks again next time. **TURN OFF** never asks again.
- **OFF** = the display never looks for new versions and does not contact GitHub.
- If a new version does not start properly the display goes back to the previous one by itself.

**Logging** – **ON** records all ISOBUS traffic on the SD card (files CANnnnnn.LOG). Useful for fault
finding or to see what your tractor sends. **OFF** (default) = no recording. Nothing is uploaded.

**Feedback** – **SEND FEEDBACK** shows a QR code to the project's issue page on GitHub. There you can report
bugs, ask and suggest improvements. It is answered **when time allows** – there is no promise of an answer
or support. Everything written there is public.

---

## 14. Idle mode and troubleshooting

**Idle mode:** when the bus is quiet the display shows the logo and "Waiting for ISOBUS". The display's
name and version are at the bottom right.

**If something does not work:**
- **"No ISOBUS traffic":** check the plug in the socket and that CAN H and CAN L are not swapped.
- **No values from the tractor:** wait a few seconds after starting. Settings → DIAGNOSTICS shows what is heard.
- **"No GPS position" on the map:** the guidance system sends no position on ISOBUS.
- **"Outside the 4 km map":** the map covers 4 × 4 km around where you started – press RESET at the new place.
- **Wheel slip shows "--":** below 1.5 km/h, or the speeds are missing.
- **Draft force near zero while working:** the implement is on the drawbar, not in the hitch – look at the wheel slip.
- **Field report:** turn the phone's WiFi off and on if the page does not appear, and scan QR code 1 again.
- **Anything else:** Settings → MORE → SEND FEEDBACK.

---

## 15. About Fältloggen: liability and licences

**No connection to manufacturers.** Fältloggen was built entirely without involvement from Fendt, AGCO or
any other manufacturer of tractors, implements or electronics (nor Waveshare, who make the display board).
It is not made, reviewed, approved or supported by them. Fendt, AGCO, Vario, ISOBUS and other names are
trademarks of their owners and are only mentioned to describe what the display works with.

**No warranty – your own risk.** Fältloggen is provided as is, without warranty of any kind. Building,
installing, updating and using it is entirely at your own risk. The person who made Fältloggen is not
responsible for errors, damage to tractor, implement, person or property, lost data or decisions made
from what is shown.
- The display only listens to ISOBUS and sends nothing – but you connect it to the tractor at your own risk.
- It does not replace the tractor's terminal or instruments. All values (area, fuel, cost, slip, draft)
  are estimates.
- Do not let the display take your attention away from driving.

**Open source.** The software is free software under the **GNU GPL version 3 or later**. You may use,
change, share and sell it, but whoever shares or sells it – changed or not – must give the recipient the
source code and their changes under the same licence. The housings and drawings are under **CERN-OHL-S v2**,
the manual under **CC BY-SA 4.0**. The full texts are in the repository on GitHub.

**Included, with their own licences:**
- QR Code generator library – Project Nayuki (MIT)
- GFX Library for Arduino – moononournation, based on Adafruit GFX (BSD)
- U8g2 – olikraus (BSD 2-clause); the DejaVu Sans font, converted to a bitmap font (Bitstream Vera licence)
- ESP32 Arduino core – Espressif (LGPL 2.1) and ESP-IDF (Apache 2.0)
- Map image: Sentinel-2 cloudless – https://s2maps.eu by EOX IT Services GmbH (contains modified
  Copernicus Sentinel data 2016), CC BY 4.0

**Privacy.** Fältloggen sends no logs, positions or reports anywhere. With WiFi it only contacts GitHub
(version check, if updates are on) and EOX (map images – EOX then sees which area the images cover).
You send feedback yourself from your phone.

*Note for developers: when a function changes, change the manual in the same version, in both languages.*
