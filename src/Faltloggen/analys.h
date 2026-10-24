// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  analys.h – bränsle- och kostnadsanalys: vilken hastighet lönar sig bäst?
//
//  Ett mätvärde per sekund sparas för de senaste 30 minuterna: fart (km/h),
//  bränsle (l/h), motorvarv, kraftuttagets varv, arbetsbredd och om traktorn
//  arbetar (kartans loggning, eller fart > 1 km/h om kartan inte loggat alls).
//
//  Arbetstiden delas upp i fartsteg om 1 km/h. För varje steg räknas
//      ha/h  = km/h × arbetsbredd m / 10
//      l/ha  = l/h / ha/h
//      kr/ha = (l/h × dieselpris + timkostnad × tidsvikt) / ha/h
//  Tidsvikten beror på läget som föraren väljer:
//      EKONOMI 0,5  – tiden spelar mindre roll, kör snålt
//      BALANS  1,0  – traktor och förare kostar det de kostar
//      BRÅTTOM 2,0  – t.ex. regn på väg, att bli klar är värt dubbelt
//  Det steg med lägst kr/ha (minst 1 minuts data) blir rekommendationen.
//
//  Motorvarv: med kraftuttaget i gång kan motorvarvet bara ändras ungefär ±10 %
//  utan att arbetet blir sämre, så då jämförs bara varv inom ±10 % av det vanliga.
//  Utan kraftuttag jämförs lägre och högre varv fritt vid samma fart.
//
//  v37: vändningar (uppehåll i arbetet högst 30 s) räknas som arbete. Liter och tid tar med vändningarna,
//  hektaren räknas bara när redskapet arbetar: l/ha = liter / ha, kr/ha = (liter × diesel + timmar × timkostnad
//  × tidsvikt) / ha. I fartstegen läggs vändningarnas liter och tid per hektar lika på alla steg.
//
//  Det är en uppskattning: terräng, last och redskapets inställning
//  påverkar också. Ju mer variation i farten, desto säkrare blir jämförelsen.
// ============================================================================
#pragma once
#include "esp_heap_caps.h"
#include "lang.h"

#define AN_N     1800          // 30 min à 1 s
#define AN_BINS  30            // fartsteg 0..29 km/h (1 km/h breda)
#define AN_MINSEK 60           // minst så här mycket data i ett steg för att jämföras
#define AN_VAND  30            // v37: uppehåll i arbetet upp till så här många sekunder = vändning (räknas som arbete)

static const float AN_TIDSVIKT[3] = {0.5f, 1.0f, 2.0f};
// Lägenas namn: anLage() i lang.h

struct AnSample { int16_t kmh10, fuel10, rpm, pto; uint16_t width10; uint8_t work, ok; };
static AnSample *anBuf = nullptr;
static int anHead = 0, anCount = 0;

static void analysBegin() {
  anBuf = (AnSample *)heap_caps_malloc(sizeof(AnSample) * AN_N, MALLOC_CAP_SPIRAM);
}

// Körs en gång per sekund. pto = högsta kraftuttagsvarvet (fram/bak), NAN = inget hört
static void analysSample(float kmh, float fuel, float rpm, float pto, float widthM, bool logging) {
  if (!anBuf) return;
  AnSample &s = anBuf[anHead];
  s.ok = !isnan(kmh) && !isnan(fuel);
  s.kmh10 = isnan(kmh) ? 0 : (int16_t)lroundf(kmh * 10);
  s.fuel10 = isnan(fuel) ? 0 : (int16_t)lroundf(fuel * 10);
  s.rpm = isnan(rpm) ? -1 : (int16_t)lroundf(rpm);
  s.pto = (isnan(pto) || pto < 50) ? 0 : (int16_t)lroundf(pto);
  s.width10 = (isnan(widthM) || widthM <= 0) ? 0 : (uint16_t)lroundf(widthM * 10);
  s.work = logging;
  anHead = (anHead + 1) % AN_N;
  if (anCount < AN_N) anCount++;
}

static void analysClear() { anHead = 0; anCount = 0; }

// Räknar fram analysen till AnView (ui.h). diesel = kr/l, timKr = traktor + förare kr/h, lage = 0..2
static void analysCompute(AnView &a, float diesel, float timKr, int lage) {
  memset(&a, 0, sizeof(a));
  a.best = a.bestL = -1;
  if (lage < 0 || lage > 2) lage = 1;
  float tv = AN_TIDSVIKT[lage];
  a.diesel = diesel; a.timKr = timKr; a.lage = lage;
  if (!anBuf) return;
  bool anyLog = false;
  for (int i = 0; i < anCount; i++) if (anBuf[i].work) { anyLog = true; break; }
  a.workFromMap = anyLog;
  auto isWork = [&](const AnSample &s) { return anyLog ? (bool)s.work : s.kmh10 > 10; };
  // v37: uppehåll i arbetet som är högst AN_VAND sekunder (vändning på vändtegen) räknas som arbete.
  // Bufferten är en ring – at(i) ger sekunderna i tidsordning. Ett uppehåll som pågår nu (inget arbete efter) räknas
  // inte förrän det är avgjort.
  auto at = [&](int i) -> const AnSample & { return anBuf[(anHead - anCount + i + 2 * AN_N) % AN_N]; };
  static uint8_t vand[AN_N];
  memset(vand, 0, sizeof(vand));
  {
    int last = -1;
    for (int i = 0; i < anCount; i++) {
      const AnSample &s = at(i);
      if (!s.ok || !isWork(s)) continue;
      int gap = i - last - 1;
      if (last >= 0 && gap > 0 && gap <= AN_VAND) for (int j = last + 1; j < i; j++) vand[j] = 1;
      last = i;
    }
  }
  double binF[AN_BINS] = {0}, binK[AN_BINS] = {0}, binW[AN_BINS] = {0}, binHa[AN_BINS] = {0};
  int binN[AN_BINS] = {0};
  double sumF = 0, sumK = 0, sumW = 0, sumHa = 0, idleF = 0, allF = 0, sumPto = 0, vandF = 0;
  int nWork = 0, nIdle = 0, nOk = 0, nPto = 0, nVand = 0;
  for (int i = 0; i < anCount; i++) {
    const AnSample &s = at(i);
    if (!s.ok) continue;
    nOk++;
    float kmh = s.kmh10 / 10.0f, f = s.fuel10 / 10.0f, w = s.width10 / 10.0f;
    allF += f;
    bool kor = isWork(s) && w > 0 && kmh >= 1.0f;          // redskapet arbetar och täcker mark
    if (!kor && (vand[i] || isWork(s))) { nVand++; vandF += f; continue; }   // vändning eller stopp i arbetet
    if (kmh < 0.5f) { nIdle++; idleF += f; continue; }
    if (!kor) continue;                                     // transport med redskapet uppe
    double ha = kmh * w / 10.0 / 3600.0;                    // hektar den här sekunden
    nWork++; sumF += f; sumK += kmh; sumW += w; sumHa += ha;
    if (s.pto > 0) { nPto++; sumPto += s.pto; }
    int b = (int)kmh; if (b >= AN_BINS) b = AN_BINS - 1;
    binN[b]++; binF[b] += f; binK[b] += kmh; binW[b] += w; binHa[b] += ha;
  }
  a.sekOk = nOk; a.sekWork = nWork + nVand; a.sekIdle = nIdle; a.sekVand = nVand;
  a.idleL = idleF / 3600.0f;
  a.vandL = vandF / 3600.0f;
  a.totL = allF / 3600.0f;
  a.ptoOn = nWork > 0 && nPto > nWork / 2;           // kraftuttaget gick större delen av arbetstiden
  a.ptoRpm = nPto ? sumPto / nPto : NAN;
  // Liter och tid inklusive vändningar, hektar bara när redskapet arbetar
  double L = (sumF + vandF) / 3600.0, T = (nWork + nVand) / 3600.0;
  double oL = sumHa > 0 ? vandF / 3600.0 / sumHa : 0;        // vändningarnas liter per hektar
  double oT = sumHa > 0 ? nVand / 3600.0 / sumHa : 0;        // vändningarnas timmar per hektar
  if (nWork && sumHa > 0) {
    a.avgKmh = sumK / nWork; a.avgW = sumW / nWork;
    a.avgLh = L / T;
    a.haPerH = sumHa / T;
    a.avgLha = L / sumHa;
    a.avgKrHa = (L * diesel + T * timKr * tv) / sumHa;
  } else { a.avgKmh = a.avgLh = a.avgW = a.haPerH = a.avgLha = a.avgKrHa = NAN; }
  // fartsteg
  int lo = -1, hi = -1;
  for (int b = 0; b < AN_BINS; b++) if (binN[b]) { if (lo < 0) lo = b; hi = b; }
  float bestKr = 1e9f, bestL = 1e9f;
  if (lo >= 0) {
    for (int b = lo; b <= hi && a.nBin < AN_VBINS; b++) {
      int i = a.nBin++;
      a.binLo[i] = b; a.binSek[i] = binN[b];
      a.binLh[i] = a.binLha[i] = a.binKrHa[i] = a.binHaH[i] = NAN;
      if (!binN[b]) continue;
      float k = binK[b] / binN[b], f = binF[b] / binN[b], w = binW[b] / binN[b];
      float haH = k * w / 10.0f;
      a.binLh[i] = f; a.binHaH[i] = haH;
      if (binHa[b] > 0) {                                  // v37: vändningarnas liter och tid per ha läggs lika på alla steg
        double lha = binF[b] / 3600.0 / binHa[b] + oL, hha = binN[b] / 3600.0 / binHa[b] + oT;
        a.binLha[i] = lha;
        a.binKrHa[i] = lha * diesel + hha * timKr * tv;
      }
      if (binN[b] >= AN_MINSEK) {
        if (!isnan(a.binKrHa[i]) && a.binKrHa[i] < bestKr) { bestKr = a.binKrHa[i]; a.best = i; }
        if (!isnan(a.binLha[i]) && a.binLha[i] < bestL) { bestL = a.binLha[i]; a.bestL = i; }
      }
    }
  }
  for (int i = 0; i < a.nBin; i++) if (a.binSek[i] >= AN_MINSEK) a.nGood++;
  // motorvarv i bästa fartsteget: lägre vs högre halva (med kraftuttag bara inom ±10 % av medianen)
  a.rpmLo = a.rpmHi = a.rpmGain = NAN;
  if (a.best >= 0) {
    int b = a.binLo[a.best];
    static int rp[AN_N]; int nr = 0;
    for (int i = 0; i < anCount; i++) {
      const AnSample &s = anBuf[i];
      if (s.ok && s.rpm > 0 && s.kmh10 / 10 == b && isWork(s) && s.width10 > 0) rp[nr++] = s.rpm;
    }
    if (nr >= 2 * AN_MINSEK) {
      for (int i = 1; i < nr; i++) { int v = rp[i], j = i - 1; while (j >= 0 && rp[j] > v) { rp[j + 1] = rp[j]; j--; } rp[j + 1] = v; }
      int med = rp[nr / 2];
      int lim = a.ptoOn ? med / 10 : 100000;              // med kraftuttag: bara ±10 %
      double fl = 0, fh = 0, rl = 0, rh = 0; int nl = 0, nh = 0;
      for (int i = 0; i < anCount; i++) {
        const AnSample &s = anBuf[i];
        if (!s.ok || s.rpm <= 0 || s.kmh10 / 10 != b || !isWork(s) || abs(s.rpm - med) > lim) continue;
        if (s.rpm < med) { fl += s.fuel10 / 10.0; rl += s.rpm; nl++; } else { fh += s.fuel10 / 10.0; rh += s.rpm; nh++; }
      }
      if (nl >= AN_MINSEK / 2 && nh >= AN_MINSEK / 2 && rh / nh - rl / nl >= 80) {
        a.rpmLo = rl / nl; a.rpmHi = rh / nh;
        a.rpmGain = 1.0f - (float)((fl / nl) / (fh / nh));
      }
    }
  }
  // ---------- text
  auto fmt = [](char *b, int n, float v, int d) { snprintf(b, n, "%.*f", d, v); langDec(b); };
  const char *V = valuta;
  char x1[16], x2[16], x3[16], x4[16];
  int r = 0;
  #define REK(...) do { if (r < AN_REK) snprintf(a.rek[r++], sizeof(a.rek[0]), __VA_ARGS__); } while (0)
  if (nWork < 3 * AN_MINSEK) {
    REK(L("För lite arbete än för att jämföra (%d av minst 3 min).", "Not enough work yet to compare (%d of at least 3 min)."), nWork / 60);
    REK("%s", L("Analysen fylls på medan du kör, de senaste 30 minuterna räknas.", "The analysis fills up as you drive; the last 30 minutes count."));
  } else if (a.nGood < 2) {
    fmt(x1, sizeof(x1), a.avgKrHa, 0); fmt(x2, sizeof(x2), a.avgLha, 1); fmt(x3, sizeof(x3), a.avgKmh, 1);
    REK(L("Snitt %s %s/ha (%s l/ha) vid %s km/h. Hastigheten har varit för jämn att jämföra.", "Average %s %s/ha (%s l/ha) at %s km/h. The speed has been too even to compare."), x1, V, x2, x3);
    REK("%s", L("Prova några minuter 1-2 km/h fortare eller långsammare, så syns skillnaden.", "Try a few minutes 1-2 km/h faster or slower to see the difference."));
  } else if (a.best >= 0) {
    int b = a.binLo[a.best];
    float kr = a.binKrHa[a.best];
    float brKr = a.binLha[a.best] * diesel, tidKr = kr - brKr;
    fmt(x1, sizeof(x1), kr, 0); fmt(x2, sizeof(x2), brKr, 0); fmt(x3, sizeof(x3), tidKr, 0); fmt(x4, sizeof(x4), a.avgKrHa, 0);
    REK(L("Lönsammast vid %d-%d km/h: %s %s/ha (bränsle %s + tid %s), snitt %s.", "Most profitable at %d-%d km/h: %s %s/ha (fuel %s + time %s), average %s."), b, b + 1, x1, V, x2, x3, x4);
    float spar = a.avgKrHa - kr;
    if (spar >= 1 && a.haPerH > 0) {
      fmt(x1, sizeof(x1), spar, 0);
      REK(L("Att hålla den hastigheten sparar ca %s %s per hektar jämfört med ditt snitt.", "Keeping that speed saves about %s %s per hectare compared with your average."), x1, V);
    } else REK("%s", L("Du ligger redan nära det lönsammaste.", "You are already close to the most profitable."));
    if (a.bestL >= 0 && a.bestL != a.best) {
      int bl = a.binLo[a.bestL];
      fmt(x1, sizeof(x1), a.binLha[a.bestL], 1); fmt(x2, sizeof(x2), a.binLha[a.best], 1);
      REK(L("Snålast är %d-%d km/h (%s l/ha mot %s), men den extra tiden kostar mer än bränslet sparar.", "Most fuel-efficient is %d-%d km/h (%s l/ha vs %s), but the extra time costs more than the fuel saved."), bl, bl + 1, x1, x2);
    }
  }
  if (!isnan(a.rpmGain) && a.rpmGain >= 0.03f) {
    fmt(x3, sizeof(x3), a.rpmGain * 100, 0);
    if (a.ptoOn)
      REK(L("Kraftuttag ca %d v/min: inom ±10 %% gav %d v/min %s %% lägre l/h än %d v/min.", "PTO approx. %d rpm: within ±10 %% %d rpm gave %s %% lower l/h than %d rpm."), (int)lroundf(a.ptoRpm / 10) * 10,
          (int)lroundf(a.rpmLo / 50) * 50, x3, (int)lroundf(a.rpmHi / 50) * 50);
    else
      REK(L("Vid samma hastighet gav ca %d v/min %s %% lägre l/h än %d v/min.", "At the same speed approx. %d rpm gave %s %% lower l/h than %d rpm."), (int)lroundf(a.rpmLo / 50) * 50, x3,
          (int)lroundf(a.rpmHi / 50) * 50);
  } else if (a.ptoOn && r < AN_REK && nWork >= 3 * AN_MINSEK) {
    REK(L("Kraftuttag ca %d v/min: motorvarvet kan bara ändras ca ±10 %%, välj hastigheten efter kostnaden ovan.", "PTO approx. %d rpm: engine speed can only change about ±10 %%, choose the speed by the cost above."),
        (int)lroundf(a.ptoRpm / 10) * 10);
  }
  if (nIdle >= 120 && a.totL > 0) {
    fmt(x1, sizeof(x1), a.idleL, 1); fmt(x2, sizeof(x2), a.idleL * diesel, 0);
    REK(L("Stillastående %d min: %s liter (%s %s, %d %% av bränslet).", "Standing still %d min: %s litres (%s %s, %d %% of the fuel)."), nIdle / 60, x1, x2, V, (int)lroundf(a.idleL / a.totL * 100));
  }
  #undef REK
  a.nRek = r;
  a.ok = nOk > 0;
}
