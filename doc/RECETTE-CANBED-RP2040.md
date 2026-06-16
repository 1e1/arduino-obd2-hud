# Cahier de recette — portage CANBed RP2040 (CAN + deep sleep)

Banc : **discodb2** (tap CAN listen-only) + CANBed RP2040 (Longan 1030018) + OLED SSD1309.
Objet : valider le portage RP2040 — communication CAN sur le SPI ré-routé, et le sommeil
piloté par l'interruption du MCP2515 (réveil sur activité bus).

Chaque ligne du firmware codée « à l'aveugle » (sans matériel) repose sur une **hypothèse**
identifiée `H1`..`H11`. Le code référence ces identifiants en commentaire. Tant qu'une hypothèse
n'est pas cochée *OK*, le comportement associé est non garanti.

## Référentiel des hypothèses

| ID | Hypothèse | Source / origine | Statut |
|---|---|---|---|
| H1 | Le MCP2515 onboard est câblé sur **SPI0 = GP2 (SCK) / GP3 (MOSI) / GP4 (MISO)** ; re-router le bus (`SPI.setSCK/setTX/setRX`) est nécessaire **et suffisant**. | Doc Zephyr `canbed_rp2040` + doc Longan 1030018 | ⬜ à vérifier |
| H2 | **CS** du MCP = **GP9**. | Doc Longan (exemple `SPI_CS_PIN = 9`) + Zephyr | ⬜ à vérifier |
| H3 | **INT** du MCP = **GP11**. | Doc Zephyr `canbed_rp2040` | ⬜ à vérifier |
| H4 | L'INT est **actif-bas** et génère un **front descendant** quand le MCP sort de `MCP_SLEEP` sur activité bus. | Datasheet MCP2515 (/INT) | ⬜ à vérifier |
| H5 | `pinMode(INT, INPUT)` **suffit** (l'/INT du MCP2515 est une sortie totem-pole / push-pull). Sinon → `INPUT_PULLUP`. | Datasheet MCP2515 + montage CANBed | ⬜ à vérifier |
| H6 | Quartz du MCP onboard = **16 MHz** → bit timing 500 kbps correct avec `MCP_16MHZ`. | Devicetree Zephyr `osc-freq = <16000000>` | ✅ confirmé doc, ⬜ comms réelles |
| H7 | `setSleepWakeup(1)` + `setMode(MCP_SLEEP)` arment le wake-on-bus et font asserter l'/INT. | Datasheet MCP2515 + lib mcp_can | ⬜ à vérifier |
| H8 | `shutdown()` (`__wfi` + IRQ GPIO sur l'INT) se réveille bien sur activité bus (réveil par interruption, pas polling). | Implémentation (PowerManager.cpp) | ⬜ à vérifier |
| H9 | `standby()` (`delay()` → SDK `sleep_ms` → `best_effort_wfe_or_timeout` = `__wfe`) est réellement bas-conso pendant les siestes 16/32 ms. | Core arduino-pico (delay.cpp) | ⬜ à mesurer |
| H10 | **Phase 2** — le vrai mode *dormant* (teardown XOSC/PLL) se réveille sur l'/INT puis `watchdog_reboot()` ; un éventuel hang est **récupérable par coupure d'alim / reset**, jamais auto-récupéré. | SDK RP2040 (`xosc_dormant`, `gpio_set_dormant_irq_enabled`) | 🟧 codé + compilé, gardé par flag `VH_RP2040_DORMANT`, **non validé matériel** |
| H11 | Le bus PQ powertrain tapé = **500 kbps** (`VH_CAN_BITRATE`). | Hypothèse projet (à confirmer harnais) | ⬜ à vérifier |

## Scénarios de recette

### R1 — Communication CAN (couvre H1, H2, H6, H11)
1. Flasher `vw-hud` sur le CANBed (FQBN `rp2040:rp2040:rpipico`).
2. Bus alimenté par discodb2 émettant des trames PQ connues (MOTOR_1 / KOMBI_1).
3. **Attendu** : le HUD décode RPM/vitesse cohérents.
4. **Critère** : au moins une trame `CAN_OK` lue et un signal affiché ≠ valeur par défaut.
5. **Si échec** : sonder GP2/3/4 à l'oscillo (H1) ; tester `MCP_8MHZ` (H6) ; vérifier le bitrate (H11).
   → Un quartz mal déclaré donne un **silence total** (aucune trame), pas une erreur.

### R2 — Assertion de l'INT au réveil bus (couvre H3, H4, H7)
1. MCP mis en `MCP_SLEEP` (`_switchOff`), bus inactif → mesurer GP11 = niveau **haut**.
2. Injecter une trame sur le bus.
3. **Attendu** : GP11 passe **bas** (front descendant) à l'activité.
4. **Critère** : front descendant observé à l'oscillo/logic analyzer sur GP11.
5. **Si pas de front** : vérifier le numéro de GP (H3), la polarité (H4), `setSleepWakeup` (H7).

### R3 — Stabilité de la lecture de l'INT (couvre H5)
1. Avec R2 OK, observer si le réveil `shutdown()` est fiable sur plusieurs cycles.
2. **Attendu** : aucun réveil intempestif (INT flottant) ni réveil manqué.
3. **Si instable** : activer `INPUT_PULLUP` (ligne commentée `[H5]` dans `vw-hud.ino`) et re-tester.
4. **Critère** : 10 cycles veille→réveil sans faux positif/négatif.

### R4 — Réveil par interruption `shutdown()` (couvre H8)
1. Contact coupé → le firmware entre dans `Energy.shutdown()` (`__wfi`).
2. Mesurer le courant : doit chuter sous l'actif (cœurs clock-gated) sans tomber au µA.
3. Injecter une activité bus.
4. **Attendu** : sortie de `shutdown()` < ~100 ms, reprise du `Workflow` (confirmation contact).
5. **Critère** : réveil systématique sur 10 essais ; le firmware ne « free-run » pas pendant la veille.

### R5 — Bas-conso des siestes `standby()` (couvre H9)
1. En conduite, mesurer le courant pendant les naps inter-frame (16/32 ms).
2. **Attendu** : creux de courant périodiques (cœur en `__wfe`).
3. **Critère** : courant moyen < actif plein régime ; cadence d'affichage inchangée (`realMillis` correct).

### R6 — Phase 2, vrai dormant (couvre H10) — **après R1–R5 OK uniquement**
0. **Pré-requis** : R4 (réveil `__wfi` sur INT) déjà validé — le dormant réutilise la même
   source de réveil (GP11), donc on ne teste ici que le teardown horloge + reboot.
1. Passer `VH_RP2040_DORMANT` de `0` à `1` dans `_wiring.h`, reflasher. Tester **d'abord sur
   alim USB de banc** (jamais directement en voiture sur 12V permanent).
2. Contact coupé → dormant ; mesurer le courant (cible **µA** / mA bas).
3. Injecter une activité bus.
4. **Attendu** : `xosc_dormant()` retourne, `rp2040.reboot()` → `setup()` rejoué, `millis()` à 0 (= nouveau trajet).
5. **Risque (H10)** : si le teardown horloge est faux, `xosc_dormant()` ne retourne jamais → **puce figée**,
   récupérable **uniquement** par coupure d'alim (retrait VSYS) ou reset matériel (pin RUN / bouton).
   → **Pré-requis montage** : confirmer si le CANBed est alimenté en **12V commuté** (clé) — auto-déblocage
   à la clé suivante — ou **12V permanent** — déblocage manuel obligatoire. Tester d'abord sur banc USB.
6. **Critère** : 20 cycles dormant→réveil→reboot sans hang.

## Journal de validation

| Date | Scénario | Résultat | Notes |
|---|---|---|---|
| | | | |
