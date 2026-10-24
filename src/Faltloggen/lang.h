// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  lang.h – språk: svenska och engelska
//
//  Alla texter som visas skrivs som L("svenska", "English"). Språket väljs i
//  inställningarna och sparas (NVS "flode"/"sprak"). Decimaltecknet följer
//  språket: komma på svenska, punkt på engelska.
// ============================================================================
#pragma once
#include <stdint.h>

static volatile uint8_t uiLang = 0;            // 0 = svenska, 1 = English
#define L(sv, en) (uiLang ? (en) : (sv))

// Byter '.' mot ',' i ett tal som formaterats med printf (bara på svenska)
static inline void langDec(char *s) {
  if (uiLang) return;
  for (; *s; s++) if (*s == '.') *s = ',';
}

// Valuta för kostnaderna (bränsleanalysen och fältrapporten). Ändras med valuta= i CONFIG.TXT.
static char valuta[8] = "kr";

// Bränsleanalysens lägen (0 EKONOMI, 1 BALANS, 2 BRÅTTOM)
static const char *anLage(int i) {
  static const char *const SV[3] = {"EKONOMI", "BALANS", "BRÅTTOM"}, *const EN[3] = {"ECONOMY", "BALANCE", "HURRY"};
  if (i < 0 || i > 2) i = 1;
  return L(SV[i], EN[i]);
}
