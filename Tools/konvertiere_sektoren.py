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
            dann int32 Indizes, bei Fassaden danach float32 UVs (2 je Ecke)
  <id>.col  float32, 9 je Dreieck (Kollision: Boden, Strassen, Waende ...)
sowie Godot/daten/sektoren.json als Verzeichnis dazu.
"""
import json
import re
import shutil
import sys
from collections import Counter
from pathlib import Path

import numpy as np

WURZEL = Path(__file__).resolve().parent.parent
QUELLE = WURZEL / "Content" / "SourceData"
ZIEL = WURZEL / "Godot" / "daten"

# Diese Abschnittsarten tragen: man steht darauf oder stoesst dagegen.
KOLLISION = {"Ground", "Road", "Sidewalk", "Gehweg", "Plaza", "Wall", "Sockel", "Roof", "Rail", "Stone"}
STADTTEXTUREN = ("Fassade", "Ziegel", "Pflaster", "Asphalt_Real", "Gehweg", "Wiese")
FREIHALTEN = {"Road", "Rail", "Plaza", "Sidewalk", "Gehweg", "Roof", "Wall", "Water"}
# Laserscanpunkte, die das Pflaster am Europaplatz bzw. in der
# Pater-Rupert-Mayer-Straße durchstoßen: x, z, ursprüngliche Höhe, freie Höhe.
PFLASTERKANTEN = ((-505, -190, 7.14, 6.92), (-505, -180, 7.18, 6.99),
                  (-505, -200, 7.11, 6.92), (-1245, -670, 20.92, 20.81),
                  (-1225, -680, 20.68, 20.58))
MARKIERTE_STRASSEN = {"Münchener Straße", "Weilheimer Straße", "Siemensstraße",
                      "Bürgermeister-Doktor-Hartmann-Straße"}
JVA_DACHSEKTOREN = {"S_N2_N1", "S_N2_N2"}
PLATANENSCHULE_SEKTOREN = {"S_N3_N1", "S_N3_N2"}


def art(name: str) -> str:
    return re.sub(r"_.*", "", name)


def nach_godot(p: np.ndarray) -> np.ndarray:
    """(n*3) Zentimeter in Unreal-Achsen -> (n,3) Meter in Godot-Achsen."""
    v = p.reshape(-1, 3).astype(np.float64) / 100.0
    return v[:, [0, 2, 1]].astype(np.float32)


def freie_pflasterkanten(pos):
    for x, z, vorher, nachher in PFLASTERKANTEN:
        trifft = (pos[:, 0] == x) & (pos[:, 2] == z) & (np.abs(pos[:, 1] - vorher) < 0.005)
        pos[trifft, 1] = nachher


def jva_dachflaechen(dreiecke):
    """Das flache LoD2-Netz des historischen Hauptbaus."""
    x, y, z = dreiecke.transpose(2, 0, 1)
    return np.all((x > -865) & (x < -665) & (z > -594) & (z < -454) &
                  (np.abs(y - 24.87) < 0.003), axis=1)


def platanenschule_dachprofil(pos):
    """LoD2-Grundrisse behalten, die Dachstaffelung an Fotos anpassen."""
    x, y, z = pos.T
    rund = (x > -1170) & (x < -1124) & (z > -521) & (z < -477)
    anbau = (x >= -1214) & (x <= -1167.3) & (z >= -500.2) & (z <= -480)
    halle = (x >= -1156.1) & (x <= -1129.9) & (z >= -563.4) & (z <= -532.3)
    y[rund & (np.abs(y - 30.86) < 0.005)] = 28.02
    dachkante = anbau & (y > 29.99) & (y < 30.80)
    y[dachkante] -= 5.76
    # Das amtliche Luftbild zeigt ein weitgehend ebenes Gründach.
    hoehere_dachteile = halle & (y > 28.01)
    y[hoehere_dachteile] = 28.0 + (y[hoehere_dachteile] - 28.0) * 0.035


def jva_dachkanten(manifest):
    haeufigkeit = Counter()
    for eintrag in manifest["sectors"]:
        if eintrag["id"] not in JVA_DACHSEKTOREN:
            continue
        daten = json.loads((QUELLE / "Sectors" / eintrag["file"]).read_bytes())
        for abschnitt in daten["sections"]:
            if art(abschnitt["name"]) != "Roof":
                continue
            pos = nach_godot(np.asarray(abschnitt["positions"], dtype=np.float64))
            dreiecke = pos[np.asarray(abschnitt["indices"])].reshape(-1, 3, 3)
            for dreieck in dreiecke[jva_dachflaechen(dreiecke)]:
                kanten = dreieck[:, [0, 2]]
                for i in range(3):
                    haeufigkeit[tuple(sorted((tuple(kanten[i]), tuple(kanten[(i + 1) % 3]))))] += 1
    return np.asarray([k for k, anzahl in haeufigkeit.items() if anzahl == 1], dtype=np.float64)


def jva_walmdach(pos, idx, kanten):
    """Hebt das Dach entlang der äußeren Traufen an, inklusive Sektorgrenzen."""
    if not kanten.size:
        return pos, idx
    dreiecke = idx.reshape(-1, 3)
    auswahl = jva_dachflaechen(pos[dreiecke])
    if not np.any(auswahl):
        return pos, idx

    teile = 16
    nummer, gewichte, raster = {}, [], []
    for i in range(teile + 1):
        for j in range(teile + 1 - i):
            nummer[i, j] = len(gewichte)
            gewichte.append((1 - (i + j) / teile, i / teile, j / teile))
    for i in range(teile):
        for j in range(teile - i):
            raster.extend([nummer[i, j], nummer[i + 1, j], nummer[i, j + 1]])
            if i + j < teile - 1:
                raster.extend([nummer[i + 1, j], nummer[i + 1, j + 1], nummer[i, j + 1]])

    quelle = pos[dreiecke[auswahl]]
    unterteilt = (np.asarray(gewichte) @ quelle).reshape(-1, 3)
    xz = unterteilt[:, [0, 2]]
    abstand = np.full(len(xz), np.inf)
    for beginn, ende in kanten:
        v = ende - beginn
        laenge = v @ v
        if laenge < 1e-8:
            continue
        t = np.clip(np.sum((xz - beginn) * v, axis=1) / laenge, 0, 1)
        abstand = np.minimum(abstand, np.linalg.norm(xz - (beginn + t[:, None] * v), axis=1))
    unterteilt[:, 1] += np.minimum(abstand * 0.61, 5.16)
    versatz = len(pos) + np.arange(len(quelle)) * len(gewichte)
    neue_idx = versatz[:, None] + np.asarray(raster)
    return (np.concatenate((pos, unterteilt.astype(np.float32))),
            np.concatenate((dreiecke[~auswahl].ravel(), neue_idx.ravel())).astype(np.int32))


def verbundene_ecken(anzahl, dreiecke):
    eltern = list(range(anzahl))

    def wurzel(i):
        while eltern[i] != i:
            eltern[i] = eltern[eltern[i]]
            i = eltern[i]
        return i

    for a, b, c in dreiecke.tolist():
        anker = wurzel(a)
        eltern[wurzel(b)] = anker
        eltern[wurzel(c)] = anker
    gruppen = {}
    for i in range(anzahl):
        gruppen.setdefault(wurzel(i), []).append(i)
    return gruppen.values()


def laden_uvs(pos, idx):
    """Misst jede vertikale Auslage in ihren eigenen Fassadenkoordinaten."""
    uv = np.zeros((len(pos), 2), dtype=np.float32)
    for ecken in verbundene_ecken(len(pos), idx.reshape(-1, 3)):
        wand = pos[ecken]
        boden, decke = wand[:, 1].min(), wand[:, 1].max()
        if decke - boden < 0.05:
            continue
        xz = wand[:, [0, 2]]
        differenzen = xz[:, None, :] - xz[None, :, :]
        distanzen = np.sum(differenzen ** 2, axis=2)
        a, b = np.unravel_index(distanzen.argmax(), distanzen.shape)
        laenge = np.sqrt(distanzen[a, b])
        if laenge < 0.18:
            continue
        achsen = max(1, round(float(laenge) / 3.25))
        uv[ecken, 0] = (xz - xz[b]) @ (differenzen[a, b] / laenge ** 2) * achsen
        uv[ecken, 1] = (wand[:, 1] - boden) / (decke - boden)
    return uv


def extrahiere_kronen(pos, idx, nadelbaum):
    """Ersetzt vollständige Kronen, belässt am Sektorrand zerschnittene Netze."""
    dreiecke = idx.reshape(-1, 3)
    ersetzbar = np.zeros(len(pos), dtype=bool)
    kronen = []
    for ecken in verbundene_ecken(len(pos), dreiecke):
        if len(ecken) != (31 if nadelbaum else 41):
            continue
        punkte = pos[ecken]
        minimum, maximum = punkte.min(axis=0), punkte.max(axis=0)
        masse = maximum - minimum
        if min(masse) < 0.6 or max(masse) > 30:
            continue
        mitte = (minimum + maximum) * 0.5
        kronen.append([*np.round(mitte, 3).astype(float), *np.round(masse, 3).astype(float),
                       int(nadelbaum)])
        ersetzbar[ecken] = True
    if not kronen:
        return pos, idx, kronen
    rest = dreiecke[~ersetzbar[dreiecke[:, 0]]].reshape(-1)
    ecken, neu = np.unique(rest, return_inverse=True)
    return pos[ecken], neu.astype(np.int32), kronen


def wiesenpunkte(wiesen, hindernisse, eintrag):
    """Verteilt Horste auf den tatsächlichen Geländedreiecken."""
    if not wiesen:
        return []
    grenzen = np.asarray(eintrag["boundsCm"], dtype=np.float64) / 100
    ursprung = grenzen[:2]
    zellweite = 2.0
    breite, tiefe = np.ceil((grenzen[2:] - ursprung) / zellweite).astype(int)
    belegt = np.zeros((breite, tiefe), dtype=bool)
    for dreiecke in hindernisse:
        xz = dreiecke[:, :, [0, 2]]
        minimum = np.floor((xz.min(axis=1) - ursprung - 1.3) / zellweite).astype(int)
        maximum = np.floor((xz.max(axis=1) - ursprung + 1.3) / zellweite).astype(int)
        for (x0, z0), (x1, z1) in zip(minimum, maximum):
            if x1 >= 0 and z1 >= 0 and x0 < breite and z0 < tiefe:
                belegt[max(0, x0):x1 + 1, max(0, z0):z1 + 1] = True

    dreiecke = np.concatenate(wiesen)
    v1 = dreiecke[:, 1] - dreiecke[:, 0]
    v2 = dreiecke[:, 2] - dreiecke[:, 0]
    normalen = np.cross(v1, v2)
    flaeche = np.abs(normalen[:, 1])
    flaeche[flaeche < np.linalg.norm(normalen, axis=1) * 0.88] = 0
    summe = flaeche.sum(dtype=np.float64)
    if summe == 0:
        return []
    sx, sz = eintrag["index"]
    rng = np.random.default_rng((sx + 32768) * 65536 + sz + 32768)
    anzahl = min(520, int(summe / 145))
    wahl = rng.choice(len(dreiecke), anzahl, p=flaeche.astype(np.float64) / summe)
    uv = rng.random((anzahl, 2))
    uv[uv.sum(axis=1) > 1] = 1 - uv[uv.sum(axis=1) > 1]
    punkte = dreiecke[wahl, 0] + uv[:, :1] * v1[wahl] + uv[:, 1:] * v2[wahl]
    raster = np.floor((punkte[:, [0, 2]] - ursprung) / zellweite).astype(int)
    uebernommen = []
    for punkt, (x, z) in zip(punkte, raster):
        if 0 < x < breite - 1 and 0 < z < tiefe - 1 and not belegt[x, z]:
            uebernommen.append([round(float(wert), 3) for wert in punkt])
            belegt[x, z] = True
            if len(uebernommen) >= 208:
                break
    return uebernommen


def markierungsachsen(sektorgroesse, verkehr):
    """Leitlinien entlang breiter Straßenabschnitte; Kreuzungen bleiben frei."""
    orte = json.loads((QUELLE / "Orte/orte.json").read_text(encoding="utf-8"))
    knoten = np.array([[k["x"], k["y"]] for k in verkehr["kreuzungen"]], dtype=float) / 100
    gruppen, belegt = {}, set()
    for strasse in orte["strassen"]:
        if strasse["n"] not in MARKIERTE_STRASSEN:
            continue
        for linie in strasse["teile"]:
            punkte = np.asarray(linie, dtype=float).reshape(-1, 2) / 100
            for anfang, ende in zip(punkte, punkte[1:]):
                laenge = np.linalg.norm(ende - anfang)
                if laenge < 9:
                    continue
                richtung = (ende - anfang) / laenge
                for abstand in np.arange(4.0, laenge - 3.0, 8.0):
                    mitte = anfang + richtung * abstand
                    if len(knoten) and np.any(np.sum((knoten - mitte) ** 2, axis=1) < 11 ** 2):
                        continue
                    zelle = tuple(np.round(mitte / 3).astype(int))
                    if zelle not in belegt:
                        belegt.add(zelle)
                        sektor = tuple(np.floor(mitte / sektorgroesse).astype(int))
                        gruppen.setdefault(sektor, []).append((mitte, richtung))
    return gruppen


def hoehenabfrage(flaechen):
    if not flaechen:
        return lambda _punkt: None
    dreiecke = np.concatenate(flaechen).astype(np.float64)
    grundriss = dreiecke[:, :, [0, 2]]
    minimum, maximum = grundriss.min(axis=1), grundriss.max(axis=1)
    v = grundriss[:, 1] - grundriss[:, 0]
    w = grundriss[:, 2] - grundriss[:, 0]
    nenner = v[:, 0] * w[:, 1] - v[:, 1] * w[:, 0]

    def hoehe(punkt):
        maske = np.all((minimum - 1e-6 <= punkt) & (maximum + 1e-6 >= punkt), axis=1)
        maske &= np.abs(nenner) > 1e-8
        i = np.flatnonzero(maske)
        delta = punkt - grundriss[i, 0]
        u = (delta[:, 0] * w[i, 1] - delta[:, 1] * w[i, 0]) / nenner[i]
        q = (v[i, 0] * delta[:, 1] - v[i, 1] * delta[:, 0]) / nenner[i]
        innen = (u >= -1e-6) & (q >= -1e-6) & (u + q <= 1.000001)
        if not np.any(innen):
            return None
        y = dreiecke[i[innen], :, 1]
        return np.max(y[:, 0] + u[innen] * (y[:, 1] - y[:, 0]) + q[innen] * (y[:, 2] - y[:, 0]))

    return hoehe


def zeichne_markierungen(achsen, hoehe):
    pos, idx = [], []
    for mitte, richtung in achsen:
        quer = np.array([-richtung[1], richtung[0]])
        if hoehe(mitte + quer * 1.85) is None or hoehe(mitte - quer * 1.85) is None:
            continue
        ecken = [mitte + richtung * a + quer * b
                 for a, b in [(-1.1, -0.065), (1.1, -0.065), (1.1, 0.065), (-1.1, 0.065)]]
        y = [hoehe(punkt) for punkt in ecken]
        if any(h is None for h in y) or max(y) - min(y) > 0.55:
            continue
        start = len(pos)
        pos.extend((x, h + 0.013, z) for (x, z), h in zip(ecken, y))
        idx.extend(start + i for i in (0, 1, 2, 0, 2, 3))
    return np.asarray(pos, dtype=np.float32), np.asarray(idx, dtype=np.int32)


def positioniere_ampeln(ampeln, fahrbahn, untergrund, gehweg):
    """Löst den Ampelmast vom OSM-Knoten und stellt ihn neben die Fahrbahn."""
    for ampel in ampeln:
        mitte = np.array([ampel["x"], ampel["y"]], dtype=float) / 100
        y = ampel["z"] / 100
        winkel = np.deg2rad(ampel["gier"])
        quer = np.array([np.cos(winkel), np.sin(winkel)])
        kandidaten = []
        for seite in (1, -1):
            for abstand in np.arange(2.75, 13.0, 0.40):
                punkt = mitte + quer * seite * abstand
                fahrhoehe = fahrbahn(punkt)
                if fahrhoehe is not None and abs(fahrhoehe - y) < 1.7:
                    continue
                h = untergrund(punkt)
                if h is None or abs(h - y) > 1.7:
                    continue
                buergersteig = gehweg(punkt)
                score = abstand - (2.5 if buergersteig is not None and abs(buergersteig - h) < 0.10 else 0)
                kandidaten.append((score, seite * abstand, h - y))
        if kandidaten:
            _, x, dy = min(kandidaten)
            ampel["mast"] = [round(x, 3), round(dy, 3)]


def main() -> int:
    manifest_pfad = QUELLE / "Sectors" / "manifest.json"
    if not manifest_pfad.exists() or manifest_pfad.stat().st_size < 1000:
        print("manifest.json fehlt oder ist nur ein LFS-Platzhalter: git lfs pull ausfuehren", file=sys.stderr)
        return 1
    manifest = json.loads(manifest_pfad.read_text())
    (ZIEL / "sektoren").mkdir(parents=True, exist_ok=True)
    dachkanten = jva_dachkanten(manifest)
    groesse = manifest["cellSizeCm"] / 100
    verkehr = json.loads((QUELLE / "Verkehr/verkehr.json").read_text(encoding="utf-8"))
    achsen = markierungsachsen(groesse, verkehr)
    ampeln = {}
    for ampel in verkehr["ampeln"]:
        sektor = (int(ampel["x"] // (100 * groesse)), int(ampel["y"] // (100 * groesse)))
        ampeln.setdefault(sektor, []).append(ampel)

    verzeichnis = []
    summe_msh = summe_col = dreiecke = 0
    for eintrag in manifest["sectors"]:
        daten = json.loads((QUELLE / "Sectors" / eintrag["file"]).read_text())
        abschnitte = []
        kollision = []
        wiesen = []
        hindernisse = []
        kronen = []
        strassen = []
        untergrund, gehwege = [], []
        k = tuple(eintrag["index"])
        with open(ZIEL / "sektoren" / f"{eintrag['id']}.msh", "wb") as msh:
            for a in daten["sections"]:
                abschnittsart = art(a["name"])
                pos = nach_godot(np.asarray(a["positions"], dtype=np.float64))
                idx = np.asarray(a["indices"], dtype=np.int32)
                if len(idx) % 3 or idx.max(initial=-1) >= len(pos):
                    raise ValueError(f"{eintrag['id']}/{a['name']}: Indizes passen nicht zu den Ecken")
                idx = np.ascontiguousarray(idx.reshape(-1, 3)[:, [0, 2, 1]]).reshape(-1)
                if abschnittsart == "Ground":
                    freie_pflasterkanten(pos)
                elif abschnittsart == "Roof" and eintrag["id"] in JVA_DACHSEKTOREN:
                    pos, idx = jva_walmdach(pos, idx, dachkanten)
                elif abschnittsart == "Tree":
                    pos, idx, neue_kronen = extrahiere_kronen(pos, idx, a["color"][1] <= a["color"][0])
                    kronen.extend(neue_kronen)
                    if len(idx) == 0:
                        continue
                if eintrag["id"] in PLATANENSCHULE_SEKTOREN and abschnittsart in {
                    "Roof", "Wall", "Gesims", "Kamin", "Sockel", "Tuer"
                }:
                    platanenschule_dachprofil(pos)
                msh.write(pos.tobytes())
                msh.write(idx.tobytes())
                meta = {"n": a["name"], "a": abschnittsart,
                        "c": [round(float(x), 4) for x in a["color"][:3]],
                        "v": len(pos), "i": len(idx)}
                uvs = None
                if abschnittsart == "Wall" and "uvs" in a:
                    uvs = np.asarray(a["uvs"], dtype=np.float32)
                    if uvs.size != len(pos) * 2:
                        raise ValueError(f"{eintrag['id']}/{a['name']}: UVs passen nicht zu den Ecken")
                elif abschnittsart == "Laden":
                    uvs = laden_uvs(pos, idx)
                if uvs is not None:
                    msh.write(uvs.tobytes())
                    meta["uv"] = True
                abschnitte.append(meta)
                dreiecke += len(idx) // 3
                if abschnittsart in KOLLISION:
                    kollision.append(pos[idx])
                if abschnittsart == "Ground" and a["color"][1] > a["color"][2]:
                    wiesen.append(pos[idx].reshape(-1, 3, 3))
                elif abschnittsart in FREIHALTEN:
                    hindernisse.append(pos[idx].reshape(-1, 3, 3))
                if abschnittsart == "Road":
                    strassen.append(hindernisse[-1])
                if k in ampeln and abschnittsart in {"Ground", "Road", "Plaza", "Sidewalk", "Gehweg"}:
                    flaeche = kollision[-1].reshape(-1, 3, 3)
                    untergrund.append(flaeche)
                    if abschnittsart in {"Sidewalk", "Gehweg"}:
                        gehwege.append(flaeche)
            fahrbahn = hoehenabfrage(strassen) if k in achsen or k in ampeln else None
            if k in ampeln:
                positioniere_ampeln(ampeln[k], fahrbahn, hoehenabfrage(untergrund), hoehenabfrage(gehwege))
            pos, idx = zeichne_markierungen(achsen.get(k, []), fahrbahn)
            if idx.size:
                msh.write(pos.tobytes())
                msh.write(idx.tobytes())
                abschnitte.append({"n": "Leitlinien", "a": "Markierung", "c": [1, 1, 1],
                                    "v": len(pos), "i": len(idx)})
                dreiecke += len(idx) // 3
        flaechen = np.concatenate(kollision) if kollision else np.zeros((0, 3), np.float32)
        (ZIEL / "sektoren" / f"{eintrag['id']}.col").write_bytes(flaechen.tobytes())
        summe_msh += (ZIEL / "sektoren" / f"{eintrag['id']}.msh").stat().st_size
        summe_col += flaechen.nbytes
        b = eintrag["boundsCm"]
        verzeichnis.append({"id": eintrag["id"], "index": eintrag["index"],
                            "grenzen": [b[0] / 100, b[1] / 100, b[2] / 100, b[3] / 100],
                            "kollision_ecken": int(len(flaechen)), "abschnitte": abschnitte,
                            "wiese": wiesenpunkte(wiesen, hindernisse, eintrag), "kronen": kronen})

    spawn = manifest["spawn"]
    (ZIEL / "sektoren.json").write_text(json.dumps({
        "sektorgroesse": manifest["cellSizeCm"] / 100,
        "spawn": {"name": spawn["name"], "x": spawn["x"] / 100, "y": spawn["z"] / 100,
                  "z": spawn["y"] / 100, "blick": spawn.get("blick", 0)},
        "sektoren": verzeichnis}, separators=(",", ":")))

    # Kleine Datensaetze unveraendert mitnehmen; die Skripte lesen sie aus res://daten.
    for datei in ("Orte/orte.json", "Fahrzeug/wagen.json", "Verkehr/netz.json",
                  "Story/drehbuch.json", "Story/missionen.json"):
        shutil.copy(QUELLE / datei, ZIEL / Path(datei).name)
    (ZIEL / "verkehr.json").write_text(json.dumps(verkehr, separators=(",", ":")), encoding="utf-8")

    # Karte: die kleine Vorlage (3 m je Pixel) reicht fuer Mini- und Vollkarte.
    karte = QUELLE / "Karte" / "karte-klein.png"
    if karte.stat().st_size > 1000:
        shutil.copy(karte, ZIEL / "karte.png")
        shutil.copy(QUELLE / "Karte" / "karte.json", ZIEL / "karte.json")
    else:
        print("Karte fehlt (git lfs pull --include='Content/SourceData/Karte/**')")

    texturen = ZIEL / "texturen"
    texturen.mkdir(parents=True, exist_ok=True)
    for name in STADTTEXTUREN:
        datei = f"T_{name}_D.png"
        shutil.copy(WURZEL / "Tools" / "Texturen" / datei, texturen / datei)

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
