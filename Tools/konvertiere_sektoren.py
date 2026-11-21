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
import os
import re
import shutil
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from hoehen import Relief  # noqa: E402

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


# Diese Abschnitte folgen dem Gelaende Punkt fuer Punkt; alles andere (Haeuser, Baeume ...) wird
# je zusammenhaengendem Teil um denselben Betrag am Standort verschoben.
VERTEXWEISE = {"Ground", "Road", "Sidewalk", "Gehweg", "Plaza", "Rail", "Stone", "Water"}
RELIEF_ALPHA = float(os.environ.get("LALABERG_RELIEF", "0.5"))   # 1.0 = Gelaende unveraendert


def baue_relief(manifest) -> "Relief | None":
    """Liest einmal alle Bodenpunkte der Stadt und baut daraus das Reliefmodell (hoehen.py)."""
    if RELIEF_ALPHA >= 0.999:
        return None
    teile = []
    for eintrag in manifest["sectors"]:
        daten = json.loads((QUELLE / "Sectors" / eintrag["file"]).read_text())
        for a in daten["sections"]:
            if art(a["name"]) in VERTEXWEISE:
                teile.append(nach_godot(np.asarray(a["positions"], dtype=np.float64))[::2])
    relief = Relief(np.concatenate(teile), alpha=RELIEF_ALPHA)
    print("Relief: Gelaende %.0f bis %.0f m, auf %.0f %% gestaucht um %.0f m" % (
        relief.hoehe_vorher[0], relief.hoehe_vorher[1], RELIEF_ALPHA * 100, relief.referenz))
    return relief


def relief_anwenden(relief, name: str, pos: np.ndarray, idx: np.ndarray) -> None:
    """Verschiebt pos (n, 3; Godot-Achsen, Meter) in der Hoehe um die Reliefverschiebung."""
    if relief is None:
        return
    if art(name) in VERTEXWEISE:
        pos[:, 1] += relief.delta(pos[:, 0], pos[:, 2]).astype(np.float32)
        return
    for g in komponenten(pos, idx.reshape(-1, 3)):
        p = pos[g]
        pos[g, 1] += float(relief.delta((p[:, 0].min() + p[:, 0].max()) / 2,
                                        (p[:, 2].min() + p[:, 2].max()) / 2))


def verschiebe_flach(relief, zahlen: list) -> list:
    """Flache Liste x, y, z, x, y, z ... in Zentimetern (Unreal-Achsen): z um die Verschiebung."""
    if relief is None:
        return zahlen
    a = np.asarray(zahlen, dtype=np.float64).reshape(-1, 3)
    a[:, 2] += relief.delta(a[:, 0] / 100.0, a[:, 1] / 100.0) * 100.0
    return np.rint(a).astype(int).reshape(-1).tolist()


def komponenten(pos: np.ndarray, idx: np.ndarray) -> list:
    """Zusammenhaengende Teile eines Netzes: Liste von Eckenindex-Arrays."""
    eltern = np.arange(len(pos))

    def wurzel(a: int) -> int:
        while eltern[a] != a:
            eltern[a] = eltern[eltern[a]]
            a = eltern[a]
        return a

    for a, b, c in idx.reshape(-1, 3):
        ra, rb = wurzel(a), wurzel(b)
        eltern[ra] = rb
        eltern[wurzel(c)] = rb
    gruppen: dict = {}
    for v in range(len(pos)):
        gruppen.setdefault(wurzel(v), []).append(v)
    return [np.asarray(g) for g in gruppen.values()]


def baeume_aus(abschnitte: list) -> np.ndarray:
    """Jeder Baum der Quelldaten (ein Kronen-Netz plus ein Stamm-Netz) wird zu einer Zeile
    [x, y_fuss, z, krone_mitte_ueber_fuss, krone_radius, krone_halbe_hoehe, r, g, b]
    in Godot-Achsen. Godot setzt daraus Instanzen (Kugelkrone, Stamm) statt der Kegelnetze."""
    staemme = []
    kronen = []
    for name, pos, idx, farbe in abschnitte:
        if art(name) not in ("Tree", "Trunk"):
            continue
        for g in komponenten(pos, idx):
            p = pos[g]
            lo, hi = p.min(axis=0), p.max(axis=0)
            eintrag = ((lo[0] + hi[0]) / 2, (lo[2] + hi[2]) / 2, lo[1], hi[1],
                       max(hi[0] - lo[0], hi[2] - lo[2]) / 2, farbe)
            (kronen if art(name) == "Tree" else staemme).append(eintrag)
    zeilen = []
    for kx, kz, ky0, ky1, radius, farbe in kronen:
        boden = ky0 - 2.2
        beste = 16.0
        for sx, sz, sy0, _sy1, _r, _f in staemme:
            d = (sx - kx) ** 2 + (sz - kz) ** 2
            if d < beste:
                beste, boden = d, sy0
        zeilen.append([kx, boden, kz, (ky0 + ky1) / 2 - boden, max(radius, 0.6),
                       max((ky1 - ky0) / 2, 0.6), *farbe[:3]])
    return np.asarray(zeilen, dtype=np.float32).reshape(-1, 9)


def dach_details(pos: np.ndarray, idx: np.ndarray) -> list:
    """Dachueberstand und Gauben. pos wird veraendert: die tiefsten Ecken jedes Dachteils
    (die Traufe) ruecken 45 cm nach aussen und 6 cm nach unten, so steht das Dach ueber die
    Wand. Zurueck kommen Gauben als Zeilen [x, y, z, richtung_x, richtung_z]: auf geneigten
    Flaechen (33 bis 52 Grad) hoeher als 1 m ueber der Traufe, mindestens 5 m Abstand,
    hoechstens zwei je Dachteil, nur bei Daechern mit mehr als 40 m2 geneigter Flaeche."""
    gauben = []
    for g in komponenten(pos, idx):
        p = pos[g]
        unten, oben = p[:, 1].min(), p[:, 1].max()
        if oben - unten < 1.5:
            continue
        # Dreiecke dieses Teils: ueber die Ecken zuordnen.
        menge = set(g.tolist())
        tris = [t for t in idx if int(t[0]) in menge]
        tri_pos = pos[np.asarray(tris)]
        n = np.cross(tri_pos[:, 1] - tri_pos[:, 0], tri_pos[:, 2] - tri_pos[:, 0])
        flaeche = np.linalg.norm(n, axis=1) / 2
        n = n / np.maximum(np.linalg.norm(n, axis=1)[:, None], 1e-9)
        n[n[:, 1] < 0] *= -1                       # nach oben zeigen lassen
        neigung = np.degrees(np.arccos(np.clip(n[:, 1], 0, 1)))
        geneigt = (neigung > 33) & (neigung < 52)
        if flaeche[geneigt].sum() < 40.0:
            continue
        mitte = p[:, [0, 2]].mean(axis=0)
        # Gauben vor dem Verschieben der Traufe waehlen.
        gewaehlt = []
        for k in np.argsort(-flaeche):
            if not geneigt[k] or flaeche[k] < 1.5 or len(gewaehlt) >= 2:
                continue
            c = tri_pos[k].mean(axis=0)
            if c[1] < unten + 1.0 or c[1] > oben - 0.8:
                continue
            if any((c[0] - o[0]) ** 2 + (c[2] - o[2]) ** 2 < 25.0 for o in gewaehlt):
                continue
            gewaehlt.append(c)
            richtung = n[k][[0, 2]]
            laenge = float(np.linalg.norm(richtung))
            if laenge < 1e-6:
                gewaehlt.pop()
                continue
            gauben.append([c[0], c[1], c[2], richtung[0] / laenge, richtung[1] / laenge])
        # Traufe: die tiefsten Ecken nach aussen.
        for v in g:
            if pos[v, 1] < unten + 0.2:
                aussen = pos[v, [0, 2]] - mitte
                lg = float(np.linalg.norm(aussen))
                if lg > 1e-6:
                    pos[v, 0] += aussen[0] / lg * 0.45
                    pos[v, 2] += aussen[1] / lg * 0.45
                pos[v, 1] -= 0.06
    return gauben


def main() -> int:
    manifest_pfad = QUELLE / "Sectors" / "manifest.json"
    if not manifest_pfad.exists() or manifest_pfad.stat().st_size < 1000:
        print("manifest.json fehlt oder ist nur ein LFS-Platzhalter: git lfs pull ausfuehren", file=sys.stderr)
        return 1
    manifest = json.loads(manifest_pfad.read_text())
    (ZIEL / "sektoren").mkdir(parents=True, exist_ok=True)
    relief = baue_relief(manifest)

    verzeichnis = []
    summe_msh = summe_col = dreiecke = 0
    for eintrag in manifest["sectors"]:
        daten = json.loads((QUELLE / "Sectors" / eintrag["file"]).read_text())
        abschnitte = []
        kollision = []
        baumquellen = []
        gauben_zeilen = []
        with open(ZIEL / "sektoren" / f"{eintrag['id']}.msh", "wb") as msh:
            for a in daten["sections"]:
                pos = nach_godot(np.asarray(a["positions"], dtype=np.float64))
                idx = np.asarray(a["indices"], dtype=np.int32)
                if len(idx) % 3 or idx.max(initial=-1) >= len(pos):
                    raise ValueError(f"{eintrag['id']}/{a['name']}: Indizes passen nicht zu den Ecken")
                relief_anwenden(relief, a["name"], pos, idx)
                if art(a["name"]) == "Roof":
                    gauben_zeilen.extend(dach_details(pos, idx.reshape(-1, 3)))
                if art(a["name"]) in ("Tree", "Trunk"):
                    # Baeume werden als Instanzen gestellt (siehe baeume_aus), nicht als Netz.
                    baumquellen.append((a["name"], pos, idx.reshape(-1, 3), a["color"][:3]))
                    continue
                idx = np.ascontiguousarray(idx.reshape(-1, 3)[:, [0, 2, 1]]).reshape(-1)
                msh.write(pos.tobytes())
                msh.write(idx.tobytes())
                abschnitte.append({"n": a["name"], "a": art(a["name"]),
                                   "c": [round(float(x), 4) for x in a["color"][:3]],
                                   "v": len(pos), "i": len(idx)})
                dreiecke += len(idx) // 3
                if art(a["name"]) in KOLLISION:
                    kollision.append(pos[idx])
        gauben = np.asarray(gauben_zeilen, dtype=np.float32).reshape(-1, 5)
        (ZIEL / "sektoren" / f"{eintrag['id']}.gauben").write_bytes(gauben.tobytes())
        baeume = baeume_aus(baumquellen)
        (ZIEL / "sektoren" / f"{eintrag['id']}.baeume").write_bytes(baeume.tobytes())
        flaechen = np.concatenate(kollision) if kollision else np.zeros((0, 3), np.float32)
        (ZIEL / "sektoren" / f"{eintrag['id']}.col").write_bytes(flaechen.tobytes())
        summe_msh += (ZIEL / "sektoren" / f"{eintrag['id']}.msh").stat().st_size
        summe_col += flaechen.nbytes
        b = eintrag["boundsCm"]
        verzeichnis.append({"id": eintrag["id"], "index": eintrag["index"],
                            "grenzen": [b[0] / 100, b[1] / 100, b[2] / 100, b[3] / 100],
                            "kollision_ecken": int(len(flaechen)), "baeume": int(len(baeume)), "gauben": int(len(gauben)),
                            "abschnitte": abschnitte})

    spawn = manifest["spawn"]
    (ZIEL / "sektoren.json").write_text(json.dumps({
        "sektorgroesse": manifest["cellSizeCm"] / 100,
        "spawn": {"name": spawn["name"], "x": spawn["x"] / 100,
                  "y": spawn["z"] / 100 + (float(relief.delta(spawn["x"] / 100, spawn["y"] / 100))
                                           if relief is not None else 0.0),
                  "z": spawn["y"] / 100, "blick": spawn.get("blick", 0)},
        "sektoren": verzeichnis}, separators=(",", ":")))

    # Kleine Datensaetze unveraendert mitnehmen; die Skripte lesen sie aus res://daten.
    for datei in ("Orte/orte.json", "Fahrzeug/wagen.json", "Story/drehbuch.json",
                  "Story/missionen.json"):
        shutil.copy(QUELLE / datei, ZIEL / Path(datei).name)
    # Routen, Ampeln, geparkte Autos und das Strassennetz folgen dem gestauchten Relief.
    verkehr = json.loads((QUELLE / "Verkehr" / "verkehr.json").read_text())
    for gruppe in ("autos", "passanten"):
        for eintrag in verkehr[gruppe]:
            eintrag["p"] = verschiebe_flach(relief, eintrag["p"])
    for gruppe in ("ampeln", "geparkt"):
        for eintrag in verkehr[gruppe]:
            z = verschiebe_flach(relief, [eintrag["x"], eintrag["y"], eintrag["z"]])
            eintrag["z"] = z[2]
    (ZIEL / "verkehr.json").write_text(json.dumps(verkehr, separators=(",", ":")))
    netz = json.loads((QUELLE / "Verkehr" / "netz.json").read_text())
    netz["p"] = verschiebe_flach(relief, netz["p"])
    (ZIEL / "netz.json").write_text(json.dumps(netz, separators=(",", ":")))

    # Karte: die kleine Vorlage (3 m je Pixel) reicht fuer Mini- und Vollkarte.
    karte = QUELLE / "Karte" / "karte-klein.png"
    if karte.stat().st_size > 1000:
        shutil.copy(karte, ZIEL / "karte.png")
        shutil.copy(QUELLE / "Karte" / "karte.json", ZIEL / "karte.json")
    else:
        print("Karte fehlt (git lfs pull --include='Content/SourceData/Karte/**')")

    # Klaenge (Schuesse, Motor, Sirene ...): gerechnet bzw. aus Tools/Sounds.
    (ZIEL / "ton").mkdir(parents=True, exist_ok=True)
    for wav in sorted((WURZEL / "Tools" / "Sounds").glob("*.wav")):
        shutil.copy(wav, ZIEL / "ton" / wav.name)

    # Modelle (Panzer, Hubschrauber, Insasse): Godot importiert FBX selbst. Liegt nur
    # der LFS-Platzhalter da (git lfs pull fehlt), wird die Datei uebersprungen.
    (ZIEL / "modelle").mkdir(parents=True, exist_ok=True)
    fehlend = []
    for fbx in sorted((WURZEL / "Tools" / "Modelle").glob("*.fbx")):
        if fbx.stat().st_size < 1000:
            fehlend.append(fbx.name)
            continue
        shutil.copy(fbx, ZIEL / "modelle" / fbx.name)
    # Umrisse des neuen Autos (aus der Konstruktionszeichnung, Tools/baue_auto_profil.py).
    profil = WURZEL / "Tools" / "Modelle" / "auto_profil.json"
    if profil.exists():
        shutil.copy(profil, ZIEL / "auto_profil.json")

    # Menschenfiguren (Quaternius, CC0) mit allen Animationen im Rig.
    for rel in ("People/Casual/Casual.fbx", "People/Worker/Worker.fbx", "Waffen/Pistol.glb",
                "Waffen/Smg.glb", "Waffen/Shotgun.glb", "Waffen/RocketLauncher.glb"):
        quelle = QUELLE / rel
        if quelle.stat().st_size > 1000:
            shutil.copy(quelle, ZIEL / "modelle" / quelle.name)
        else:
            fehlend.append(quelle.name)
    if fehlend:
        print("Modelle fehlen (git lfs pull --include='Tools/Modelle/**'): " + ", ".join(fehlend))

    print(f"{len(verzeichnis)} Sektoren, {dreiecke} Dreiecke, "
          f"Netze {summe_msh / 1e6:.1f} MB, Kollision {summe_col / 1e6:.1f} MB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
