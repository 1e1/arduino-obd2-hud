# 3D models

3D-printable enclosure for the main assembly (OLED SSD1309 2.42" + CANBed RP2040).
See the component specs in [`../README.md`](../README.md).

## Convention

- Source: parametric **OpenSCAD** `.scad` (versioned, human-readable).
- Exports: `.stl` / `.3mf` rendered from the `.scad`.
- All real-world dimensions are exposed as variables at the top of each `.scad`,
  fed from the `⟨measure⟩` values in [`../README.md`](../README.md).

## Render

```sh
openscad -o enclosure.stl enclosure.scad      # CLI
# or open the .scad in the OpenSCAD GUI and F6 to render, then export STL
```

> No model committed yet — measure the parts first (checklist in
> [`../README.md`](../README.md)), then the parametric enclosure can be generated.
