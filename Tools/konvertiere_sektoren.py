#!/usr/bin/env python3
"""Wandelt die Stadtsektoren (Content/SourceData/Sectors/*.json) in das kompakte
Binaerformat, das das Godot-Projekt unter Godot/ direkt einliest.

Aufruf:  python3 Tools/konvertiere_sektoren.py

Koordinaten: Die Quelle liegt in Zentimetern, x vorwaerts, y rechts, z oben.
Godot bekommt Meter und (x, z, y), also y nach oben. Die Achsen werden dabei
getauscht (Determinante -1); das gleicht den Wechsel von Unreals linkshaendigem
zum rechtshaendigen Godot-System aus. Die Wicklung muss dabei gedreht werden:
nach dem Achsentausch laeuft sie gegen den Uhrzeigersinn, Godot erwartet die
Vorderseite im Uhrzeigersinn. Ohne Drehung traegt der Boden von oben nicht
(Kollisionsform) und liegt mit der Rueckseite nach oben. Gemessen am Spawn:
Normale vorher +1, nachher -1 (rechtshaendig gerechnet, Godot-Vorderseite).

Je Sektor entstehen zwei Dateien in Godot/daten/sektoren/:
  <id>.msh  je Abschnitt hintereinander: float32 Positionen (3 je Ecke),
            dann int32 Indizes
  <id>.col  float32, 9 je Dreieck (Kollision: Boden, Strassen, Waende ...)
sowie Godot/daten/sektoren.json als Verzeichnis dazu.
"""
import json
import re
import shutil
import sys
from pathlib import Path

import numpy as np

WURZEL = Path(__file__).resolve().parent.parent
QUELLE = WURZEL / "Content" / "SourceData"
ZIEL = WURZEL / "Godot" / "daten"

# Diese Abschnittsarten tragen: man steht darauf oder stoesst dagegen.
KOLLISION = {"Ground", "Road", "Sidewalk", "Gehweg", "Plaza", "Wall", "Sockel", "Roof", "Rail", "Stone"}


def art(name: str) -> str:
    return re.sub(r"_.*", "", name)


def nach_godot(p: np.ndarray) -> np.ndarray:
    """(n*3) Zentimeter in Unreal-Achsen -> (n,3) Meter in Godot-Achsen."""
    v = p.reshape(-1, 3).astype(np.float64) / 100.0
    return v[:, [0, 2, 1]].astype(np.float32)


def main() -> int:
    manifest_pfad = QUELLE / "Sectors" / "manifest.json"
    if not manifest_pfad.exists() or manifest_pfad.stat().st_size < 1000:
        print("manifest.json fehlt oder ist nur ein LFS-Platzhalter: git lfs pull ausfuehren", file=sys.stderr)
        return 1
    manifest = json.loads(manifest_pfad.read_text())
    (ZIEL / "sektoren").mkdir(parents=True, exist_ok=True)

    verzeichnis = []
    summe_msh = summe_col = dreiecke = 0
    for eintrag in manifest["sectors"]:
        daten = json.loads((QUELLE / "Sectors" / eintrag["file"]).read_text())
        abschnitte = []
        kollision = []
        with open(ZIEL / "sektoren" / f"{eintrag['id']}.msh", "wb") as msh:
            for a in daten["sections"]:
                pos = nach_godot(np.asarray(a["positions"], dtype=np.float64))
                idx = np.asarray(a["indices"], dtype=np.int32)
                if len(idx) % 3 or idx.max(initial=-1) >= len(pos):
                    raise ValueError(f"{eintrag['id']}/{a['name']}: Indizes passen nicht zu den Ecken")
                idx = np.ascontiguousarray(idx.reshape(-1, 3)[:, [0, 2, 1]]).reshape(-1)
                msh.write(pos.tobytes())
                msh.write(idx.tobytes())
                abschnitte.append({"n": a["name"], "a": art(a["name"]),
                                   "c": [round(float(x), 4) for x in a["color"][:3]],
                                   "v": len(pos), "i": len(idx)})
                dreiecke += len(idx) // 3
                if art(a["name"]) in KOLLISION:
                    kollision.append(pos[idx])
        flaechen = np.concatenate(kollision) if kollision else np.zeros((0, 3), np.float32)
        (ZIEL / "sektoren" / f"{eintrag['id']}.col").write_bytes(flaechen.tobytes())
        summe_msh += (ZIEL / "sektoren" / f"{eintrag['id']}.msh").stat().st_size
        summe_col += flaechen.nbytes
        b = eintrag["boundsCm"]
        verzeichnis.append({"id": eintrag["id"], "index": eintrag["index"],
                            "grenzen": [b[0] / 100, b[1] / 100, b[2] / 100, b[3] / 100],
                            "kollision_ecken": int(len(flaechen)), "abschnitte": abschnitte})

    spawn = manifest["spawn"]
    (ZIEL / "sektoren.json").write_text(json.dumps({
        "sektorgroesse": manifest["cellSizeCm"] / 100,
        "spawn": {"name": spawn["name"], "x": spawn["x"] / 100, "y": spawn["z"] / 100,
                  "z": spawn["y"] / 100, "blick": spawn.get("blick", 0)},
        "sektoren": verzeichnis}, separators=(",", ":")))

    # Kleine Datensaetze unveraendert mitnehmen; die Skripte lesen sie aus res://daten.
    for datei in ("Orte/orte.json", "Fahrzeug/wagen.json", "Verkehr/verkehr.json", "Verkehr/netz.json",
                  "Story/drehbuch.json", "Story/missionen.json"):
        shutil.copy(QUELLE / datei, ZIEL / Path(datei).name)

    # Karte: die kleine Vorlage (3 m je Pixel) reicht fuer Mini- und Vollkarte.
    karte = QUELLE / "Karte" / "karte-klein.png"
    if karte.stat().st_size > 1000:
        shutil.copy(karte, ZIEL / "karte.png")
        shutil.copy(QUELLE / "Karte" / "karte.json", ZIEL / "karte.json")
    else:
        print("Karte fehlt (git lfs pull --include='Content/SourceData/Karte/**')")

    # Modelle (Panzer, Hubschrauber, Insasse): Godot importiert FBX selbst. Liegt nur
    # der LFS-Platzhalter da (git lfs pull fehlt), wird die Datei uebersprungen.
    (ZIEL / "modelle").mkdir(parents=True, exist_ok=True)
    fehlend = []
    for fbx in sorted((WURZEL / "Tools" / "Modelle").glob("*.fbx")):
        if fbx.stat().st_size < 1000:
            fehlend.append(fbx.name)
            continue
        shutil.copy(fbx, ZIEL / "modelle" / fbx.name)
    if fehlend:
        print("Modelle fehlen (git lfs pull --include='Tools/Modelle/**'): " + ", ".join(fehlend))

    print(f"{len(verzeichnis)} Sektoren, {dreiecke} Dreiecke, "
          f"Netze {summe_msh / 1e6:.1f} MB, Kollision {summe_col / 1e6:.1f} MB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
