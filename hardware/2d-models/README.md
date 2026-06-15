# 2D models — architect-style plans

Orthographic 2D drawings (vue de face / dessus / côté) of the main-assembly
components, plus principle "consigne" sketches, for reasoning about the enclosure
before the 3D model exists. Component specs: [`../README.md`](../README.md).

### Dimensioned component plans

| File | Component | Confirmed | To measure |
|---|---|---|---|
| [`oled-ssd1309.svg`](./oled-ssd1309.svg) | OLED 2.42" SSD1309 | **fully dimensioned** — outline 72.00 × 43.00 × 5.90 · 4× Ø2.80 (pitch 67.80 × 38.80) · V.A 57.01 × 29.5 · A.A 55.01 × 27.49 | (V.A vertical offset ≈5.11 to confirm) |
| [`canbed-rp2040.svg`](./canbed-rp2040.svg) | CANBed RP2040 | outline 56 × 41 mm · PCB 1.0 mm · edge layout (CAN right, µUSB bottom, GPIO left, headers top) | connector heights, mounting-hole Ø/positions, connector offsets along edges |

### Principle sketches (consignes — schematic, not to scale)

| File | Shows |
|---|---|
| [`connection-principle.svg`](./connection-principle.svg) | CAN wiring: car OBD-II → **thin 4-wire cable (no DB9)** → CANBed 4-pin terminal. 120 Ω OFF · +12V from pin 16 · OLED on SPI header (not GP9) · signals to verify with discodb2 |
| [`mounting-principle.svg`](./mounting-principle.svg) | Side elevation: **cowl-lip hook + TPU foot** (no adhesive), panel **tilt + hood + matte bezel** for anti-reflection, low CG, 2-part removable, cable to OBD |

## Conventions

- **Units mm, scale 1:1 on A4 landscape** — printed on A4 the geometry is true size.
- **First-angle orthographic layout**: the top view sits below the front view,
  the side view to its right, all projection-aligned.
- **Colour code**
  - solid black outline = **confirmed** dimension (datasheet).
  - blue = active display area (OLED) · grey = connectors · dark = ICs.
  - **red dashed + `⟨?⟩` = TO MEASURE** with calipers before any 3D model.
- Internal component placement on the CANBed is **indicative** (traced from the
  product photo) — only the 56 × 41 mm outline is dimensioned by the source.

## Editing

Plain SVG — edit in any text editor or Inkscape. Dimension values live as `<text>`
nodes next to their dimension lines; replace each `⟨?⟩` with the measured value
(and adjust the matching geometry) as the parts are measured. These same numbers
then feed the parametric OpenSCAD enclosure in [`../3d-models/`](../3d-models/).

## Sources

- OLED: [hicenda 2.42" OLED module](https://www.hicenda.com/product/242-inch-oled-module-05.html) (clone of the diymore module)
- CANBed: [Longan docs 1030018](https://docs.longan-labs.cc/1030018/) · Seeed datasheet (SKU 102991596)
