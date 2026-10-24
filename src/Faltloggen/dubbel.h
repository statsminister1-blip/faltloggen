// SPDX-License-Identifier: GPL-3.0-or-later
// ============================================================================
//  dubbel.h – två programvarianter på samma skärm (valfritt, för den som utvecklar)
//
//  Med den vanliga partitionstabellen (två programplatser) gör den här filen ingenting: uppdateringar
//  skrivs till "nästa" plats som vanligt och ingen växlingsknapp visas.
//
//  Med en partitionstabell med fyra programplatser (ota_0..ota_3) kan två varianter av programmet ligga
//  på samma skärm, en i ota_0/ota_1 och en i ota_2/ota_3. Då gäller:
//   - En uppdatering skrivs bara till den andra platsen i det egna paret – den andra varianten rörs aldrig.
//   - dubbelAnnan() ger den andra variantens senast körda (giltiga) plats; dubbelByt() startar om i den.
//     Knappen för det visas bara när den andra varianten verkligen finns installerad.
//   - Efter en växling godkänns programmet direkt (det har redan körts), så att skärmen inte går tillbaka.
// ============================================================================
#pragma once
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "esp_app_desc.h"
#include <Preferences.h>

static const esp_partition_t *dubbelPlats(int i) {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP, (esp_partition_subtype_t)(ESP_PARTITION_SUBTYPE_APP_OTA_0 + i), NULL);
}
static bool dubbelTabell() { return dubbelPlats(2) && dubbelPlats(3); }
// Platsnummer 0..3 för den plats programmet körs från, -1 = okänt
static int dubbelKor() {
  const esp_partition_t *r = esp_ota_get_running_partition();
  if (!r || r->type != ESP_PARTITION_TYPE_APP) return -1;
  int i = (int)r->subtype - (int)ESP_PARTITION_SUBTYPE_APP_OTA_0;
  return (i >= 0 && i <= 3) ? i : -1;
}
static bool dubbelGiltig(const esp_partition_t *p) {
  esp_app_desc_t d;
  return p && esp_ota_get_partition_description(p, &d) == ESP_OK && d.magic_word == ESP_APP_DESC_MAGIC_WORD;
}

// Platsen som en uppdatering ska skrivas till
static const esp_partition_t *dubbelUppdPlats() {
  int i = dubbelKor();
  if (!dubbelTabell() || i < 0) return esp_ota_get_next_update_partition(NULL);
  return dubbelPlats(i ^ 1);
}

// Den andra variantens plats att starta, eller nullptr (vanlig tabell / andra varianten saknas)
static const esp_partition_t *dubbelAnnan() {
  int i = dubbelKor();
  if (!dubbelTabell() || i < 0) return nullptr;
  int q = (i / 2) ^ 1;                                   // det andra paret
  Preferences pr; pr.begin("dubbel", true);
  int sist = pr.getInt(q ? "s1" : "s0", -1);             // senast körda plats i det paret
  pr.end();
  if (sist == 2 * q || sist == 2 * q + 1) {
    const esp_partition_t *p = dubbelPlats(sist);
    if (dubbelGiltig(p)) return p;
  }
  for (int k = 0; k < 2; k++) {
    const esp_partition_t *p = dubbelPlats(2 * q + k);
    if (dubbelGiltig(p)) return p;
  }
  return nullptr;
}

// Anropas tidigt i setup(): kommer ihåg vilken plats den här varianten körs från, och godkänner
// programmet direkt om skärmen just växlat variant. Returnerar true om programmet godkändes här.
static bool dubbelStart() {
  int i = dubbelKor();
  if (!dubbelTabell() || i < 0) return false;
  Preferences pr; pr.begin("dubbel", false);
  pr.putInt((i / 2) ? "s1" : "s0", i);
  bool byt = pr.getBool("byt", false);
  if (byt) pr.putBool("byt", false);
  pr.end();
  if (byt) esp_ota_mark_app_valid_cancel_rollback();
  return byt;
}

// Startar om i den andra varianten. Returnerar bara om det inte gick.
static bool dubbelByt(void (*foreOmstart)()) {
  const esp_partition_t *p = dubbelAnnan();
  if (!p) return false;
  Preferences pr; pr.begin("dubbel", false); pr.putBool("byt", true); pr.end();
  if (esp_ota_set_boot_partition(p) != ESP_OK) return false;
  if (foreOmstart) foreOmstart();
  delay(300);
  ESP.restart();
  return true;
}
