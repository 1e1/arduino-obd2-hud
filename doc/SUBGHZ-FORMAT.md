# Flipper Zero `.sub` (SubGHz RAW) — format

Ce que le futur module **RF-replay** (optionnel) doit savoir pour parser un
fichier exporté du Flipper Zero. Exemple synthétique : [`subghz-sample.sub`](./subghz-sample.sub)
(timings 100 % inventés, n'ouvre rien — sert de fixture de test).

> Ce module est un **add-on optionnel** du HUD : rejouer un signal de
> télécommande de portail **à code fixe** préalablement enregistré au Flipper.
> Il ne sert à rien sur un portail à **code tournant** (voir plus bas).

## En-tête (clés `Clé: valeur`, une par ligne)

| Clé | Exemple | Rôle pour le firmware |
|---|---|---|
| `Filetype` | `Flipper SubGhz RAW File` | garde-fou : rejeter si != cette valeur |
| `Version` | `1` | version de format (1 à ce jour) |
| `Frequency` | `433920000` | **Hz** → programme le CC1101 (433.92 / 868 / 915…) |
| `Preset` | `FuriHalSubGhzPresetOok650Async` | modulation. `Ook270`/`Ook650` = **OOK** (ASK tout-ou-rien) ; le `650`/`270` = largeur du filtre RX (kHz), n'impacte que la réception, pas le rejeu. `2FSKDev238`/`2FSKDev476` = 2-FSK (rare sur portail). |
| `Protocol` | `RAW` | `RAW` = timings bruts (ci-dessous). Sinon (`Princeton`, `CAME`, `NICE FLO`, `KeeLoq`…) le payload est dans d'autres clés (`Bit`, `Key`, `TE`) — **non géré par le rejeu brut**. |

## Corps (`Protocol: RAW`)

- Une ou plusieurs lignes `RAW_Data: v1 v2 v3 …`.
- Chaque `v` est une **durée en microsecondes**, le signe = état de la porteuse :
  - **positif** = porteuse **ON** (émission),
  - **négatif** = porteuse **OFF** (silence).
- Les signes **alternent strictement** (ON, OFF, ON, OFF…). Une ligne peut finir
  sur un ON et la suivante reprendre sur un OFF : **concaténer toutes les lignes
  `RAW_Data`** avant de parser — ne jamais raisonner ligne par ligne.
- La séquence entière = la durée de l'appui bouton : bruit initial, puis la
  **trame répétée ~8–10×**, chaque répétition séparée par un **long OFF**
  (silence inter-trame).

## Observé sur une capture réelle de portail (433.92 MHz, OOK)

```
[bruit] [préambule: ~8–12 × (≈250 ON / ≈250 OFF)] [données: mix ≈250 µs / ≈500 µs]
[silence ≈11–13 ms] [préambule] [données] [silence] …   ← répété 8–10×
```

- **Base de temps T ≈ 250 µs** ; symboles à 1T (~250 µs) et 2T (~500 µs) →
  encodage de type **Manchester** (ou PWM selon le portail).
- Le **silence inter-trame** (gap long, > ~8 ms) est le délimiteur fiable pour
  découper les trames.
- Le **début du fichier contient du bruit** (p. ex. une impulsion de ~150 ms,
  des durées irrégulières) capté avant l'appui réel : à ignorer (chercher le
  premier préambule régulier).

## Code fixe vs code tournant — le go/no-go du projet

Un `.sub` RAW rejoué **n'ouvre un portail à code tournant qu'une fois** (le
compteur de la capture est déjà « consommé » côté récepteur), voire jamais.

- **Code fixe** (Princeton PT2262, CAME, NICE FLO, Holtek, BFT…) → rejeu OK.
- **Code tournant** (KeeLoq, Somfy/Telis, Nice FloR-S, CAME Atomo, Hörmann
  BiSecur…) → rejeu inutile.

**Test de validation (le seul fiable) :** rejouer le `.sub` 2× de suite depuis
le Flipper. S'ouvre 2× ⇒ code fixe ⇒ projet viable. S'ouvre une fois puis plus ⇒
code tournant ⇒ abandonner (ou projet « apprentissage de clé », hors scope).

> Note : `Protocol: RAW` dans l'en-tête ne tranche **pas** fixe/tournant — c'est
> souvent juste le mode « Read RAW » du Flipper. Seul le test de rejeu tranche.
> La **longueur de trame** est un indice (≈24 bits → fixe ; ≈66 bits → KeeLoq),
> à confirmer quand le parseur décodera réellement les trames.

## Implications firmware (`RfRemoteCc1101`)

1. **Parser** : lire l'en-tête (freq + preset→modulation), concaténer les
   `RAW_Data`, stocker un tableau de durées signées. `int16_t` (±32767 µs) suffit
   pour les symboles ; les gaps > 32 ms sont rares → soit les clamper, soit
   coder en `int32_t`.
2. **Émettre** : régler le CC1101 sur `Frequency`, mode **OOK async**, puis
   bit-banger `GDO0` — `HIGH` pendant un `v>0`, `LOW` pendant un `v<0`, chaque
   durée temporisée au µs. **Aucun accès SPI pendant l'émission** → le bus reste
   libre pour le MCP2515 et l'OLED.
3. **Rejouer la séquence telle quelle**, répétitions incluses : ne PAS re-répéter
   nous-mêmes, le fichier contient déjà les répétitions de l'appui bouton.
4. Émission **bloquante** ~200 ms–1 s : le HUD se fige le temps du rejeu (on est
   à l'arrêt devant le portail) — c'est l'état **ÉMISSION** du fonctionnement
   ci-dessous. Option avancée : cœur 1 du RP2040 dédié au timing pour garder
   CAN + écran vivants.

## Fonctionnement — B1 (menu) / B2 (exécution) / minuteur

Deux boutons pilotent une petite machine d'états sur la **bande de la jauge
carburant** (bas de l'OLED, `y 54–63`, 10 px). Contrôleur :
[`../vw-hud/RfRemote.h`](../vw-hud/RfRemote.h) ; anti-rebond des boutons :
[`../vw-hud/_button.h`](../vw-hud/_button.h) ; pins/constantes :
[`../vw-hud/_wiring.h`](../vw-hud/_wiring.h) (`VH_BTN_MENU`, `VH_BTN_EXEC`,
`VH_RF_LABEL_MS`).

- **B1 (menu, `VH_BTN_MENU`)** : si la **jauge** est affichée → affiche le `.sub`
  **sélectionné** ; si un **`.sub`** est affiché → passe au **suivant** (qui
  devient la sélection).
- **B2 (exéc, `VH_BTN_EXEC`)** : affiche le `.sub` sélectionné **et le rejoue
  instantanément** (qu'il soit déjà visible ou seulement en mémoire). **Pas
  d'appui long** — le rejeu est immédiat.
- **Aucun appui pendant `VH_RF_LABEL_MS`** (défaut 4096 ms = un tour de fenêtre
  glissante) → la bande **revient à la jauge**.

| État | Bande | B1 | B2 | timeout |
|---|---|---|---|---|
| **JAUGE** | jauge carburant | → FICHIER (montre la sélection) | → ÉMISSION (joue) | — |
| **FICHIER** | nom du `.sub` sélectionné | → FICHIER (suivant = sélection) | → ÉMISSION (joue) | → JAUGE |
| **ÉMISSION** | nom du `.sub` (écran figé) | — | — | → FICHIER (fin du rejeu) |

```
        [*] → JAUGE
JAUGE   --B1-->  FICHIER            FICHIER --B1--> FICHIER (suivant)
JAUGE   --B2-->  ÉMISSION           FICHIER --B2--> ÉMISSION
FICHIER --timeout--> JAUGE          ÉMISSION --fin--> FICHIER
```

- `RfRemote::view()` expose **GAUGE / FILE** : le renderer dessine soit la jauge
  normale (`Hudisplay::_drawTank`), soit le nom du `.sub` dans la bande. ÉMISSION
  est l'état transitoire bloquant pendant `_transmit()` (CC1101), non observable
  entre deux frames puisque l'écran est modal.
- **Placement** : le nom est rendu en `profont12_tr` (**déjà** dans le firmware,
  0 octet de flash en plus), qui tient dans les 10 px de la bande. Rendu
  recommandé **inversé** (bande allumée, texte creusé) pour rester lisible
  par-dessus le remplissage de la jauge.
- **Garde-fou (sécurité)** : B1/B2 ne sont **armés qu'à l'arrêt** (vitesse 0 +
  contact + frein à main, via `Workflow`). En roulant, la bande reste la jauge et
  les boutons sont ignorés. Le rejeu instantané de B2 est protégé par ce
  garde-fou, **pas** par un appui long.
- **État d'implémentation** : le contrôleur (boutons + machine d'états + minuteur)
  est implémenté et compilé sous `-DVH_FEATURE_RF=1`, y compris le **rendu** du
  nom du `.sub` dans la bande (`Hudisplay::_drawRfLabel`, inversé). L'émission
  réelle (phase 3) et le chargement (phase 4) sont dans les sections ci-dessous.

## Garde-fous de parsing
- Rejeter si `Filetype` / `Protocol` inattendus.
- `Preset` 2-FSK → avertir (le montage vise l'OOK des portails).
- Borner le nombre de transitions (RAM) : tronquer/refuser au-delà d'un plafond
  (cf. tier « comfortable » RP2040 dans [DESIGN.md](./DESIGN.md) ; exclu du 328P).

## Phase 3 — émission réelle (CC1101)

- `vw-hud/RfCc1101.h` : driver CC1101 minimal (TX seulement). SPI config → `STX`
  (async) → bit-bang `GDO0` avec les timings µs → `SIDLE`. `setFrequencyHz()`
  depuis le champ `Frequency` du `.sub`.
- `vw-hud/RfRemoteCc1101.hpp` : backend `RfRemote` ; `_transmitStart(i)` lit les
  timings via `RfLibrary::loadTimings(i,…)` et émet. Émission modélisée en 3 temps
  (`_transmitStart`/`_transmitBusy`/`_transmitEnd`) → supporte bloquant ET non
  bloquant (état ÉMISSION).
- **Activer** : `-DVH_FEATURE_RF=1 -DVH_RF_CC1101=1` (sinon backend « stream »
  par défaut). Compilé en CI (`rp2040-rf-cc1101`).
- ⚠️ **À valider au banc** : les **valeurs de registres** CC1101 (OOK, async,
  `GDO0` en entrée data, PATABLE/puissance, XTAL 26 MHz) sont dérivées
  datasheet mais **non testées** sur le module. La *structure* est le contrat ;
  régler les octets au banc avec un récepteur.
- **Continuité écran (PIO)** : `vw-hud/RfOokPio.h` — moteur **PIO + DMA** (RP2040)
  qui joue la forme d'onde `GDO0` en matériel → émission **non bloquante**, l'OLED
  reste rafraîchi (plus de figeage ~1 s). Activer `-DVH_RF_PIO=1` (en plus de
  `VH_RF_CC1101`). Programme PIO 3 instructions, 1 cycle = 1 µs, overhead ~3 µs/
  échantillon compensé — **timing à vérifier au scope/récepteur**. CI :
  `rp2040-rf-pio`. Sans ce flag, émission bloquante (écran figé, OK à l'arrêt).

## Phase 4 — chargement des `.sub`

- `vw-hud/RfLibrary.h` : bibliothèque des captures. `loadFromFs()` scanne
  LittleFS (`/`, `*.sub`) et remplit nom + fréquence ; `loadTimings(i,…)` relit
  les timings **à la demande** (scratch unique, RAM bornée). `names()` se branche
  direct sur `RfRemote::setNames()`. Source alternative sans FS : `add(name,
  text, len)` (ex. échantillon figé en flash).

### Méthode de chargement retenue — upload LittleFS de l'IDE Arduino

Pas de protocole maison : on utilise l'**uploader LittleFS** du core arduino-pico.

1. Sélectionner une **Flash avec partition FS** (IDE : *Tools ▸ Flash Size* →
   une taille « … (FS: xxx KB) », ou en CLI un FQBN `…:flash=2097152_1048576`).
2. Déposer les `.sub` (exportés du Flipper) dans **`vw-hud/data/`**.
3. Lancer **« Pico LittleFS Data Upload »** (plugin IDE) — il écrit `data/` sur
   la partition LittleFS. (Équivalents : PlatformIO `uploadfs`, ou `picotool`.)
4. Au boot, `RfLibrary::loadFromFs()` lit les `.sub` et peuple les portails.

Pour remplacer/ajouter un portail : modifier `data/`, ré-uploader. Simple et
fiable — c'est la méthode officielle (et unique) de chargement du module.
