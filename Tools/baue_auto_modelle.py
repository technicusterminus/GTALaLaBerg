# Liest die farbigen Fahrzeugblaetter (vom Bildmodell auf OpenRouter erzeugt, siehe
# Docs/Konzept/autos/<modell>_farbe.png) und schreibt je Modell
#   Tools/Modelle/auto_profil_<modell>.json   (Dachlinie, halbe Breite, Raeder, Fensterband)
#   Godot/texturen/auto_<modell>_{seite,oben,front,heck}.png
# Godot (scripts/wagenmesh.gd) baut daraus Karosserie und Raeder.
#
# Ein Blatt hat vier Ansichten auf weissem Grund: Seite oben links, Draufsicht darunter,
# Heck oben rechts, Front unten rechts. Sie werden als zusammenhaengende Flaechen gefunden.
# Zeigt die Front in Seiten- und Draufsicht nach rechts, werden beide gespiegelt (im Spiel
# liegt vorn bei u = 0).
#
#   python Tools/baue_auto_modelle.py            (alle Modelle aus MODELLE)
#   python Tools/baue_auto_modelle.py sport      (nur eines)
import json
import os
import sys

import numpy as np
from PIL import Image
from scipy import ndimage

HIER = os.path.dirname(os.path.abspath(__file__))
BLAETTER = os.path.join(HIER, "..", "Docs", "Konzept", "autos")
TEXTUREN = os.path.join(HIER, "..", "Godot", "texturen")
SCHNITTE = 72
## Laenge im Spiel (m) und ob die Front im Blatt rechts liegt.
MODELLE = {
    "limousine": (4.75, False),
    "kombi": (4.75, True),
    "bus": (5.9, True),
    "sport": (4.45, False),
    "renn": (4.6, False),
    "super": (4.6, False),
}


def median(a: np.ndarray, fenster: int) -> np.ndarray:
    r = fenster // 2
    pad = np.pad(a, r, mode="edge")
    return np.array([np.median(pad[i:i + fenster]) for i in range(len(a))])


def ansichten(bild: np.ndarray) -> dict:
    """Die vier groessten Flaechen auf weissem Grund, nach Lage benannt."""
    maske = (255 - bild.min(axis=2)) > 18
    maske = ndimage.binary_opening(maske, iterations=2)
    lab, _ = ndimage.label(ndimage.binary_closing(maske, iterations=3))
    teile = []
    for i, s in enumerate(ndimage.find_objects(lab)):
        teile.append((int((lab[s] == i + 1).sum()), s, i + 1))
    teile = sorted(teile, key=lambda t: -t[0])[:4]
    breite = bild.shape[1]
    links = sorted([t for t in teile if (t[1][1].start + t[1][1].stop) / 2 < breite * 0.65],
                   key=lambda t: t[1][0].start)
    rechts = sorted([t for t in teile if (t[1][1].start + t[1][1].stop) / 2 >= breite * 0.65],
                    key=lambda t: t[1][0].start)
    if len(links) != 2 or len(rechts) != 2:
        raise SystemExit(f"Blatt hat nicht vier Ansichten: links {len(links)}, rechts {len(rechts)}")
    ergebnis = {}
    for name, (_, s, nr) in zip(["seite", "oben", "heck", "front"], links + rechts):
        # Weisse Flecken im Umriss (Aufkleber, Glanzlichter) gehoeren zum Wagen.
        ergebnis[name] = (s, ndimage.binary_fill_holes(lab[s] == nr))
    return ergebnis


def mit_hoehe(rgb: np.ndarray) -> Image.Image:
    """RGBA: im Alpha eine Hoehenkarte fuer Relief und Parallaxe im Shader. Lack und Hintergrund
    oben (1), Fugen, Grill, Scheiben, Lufteinlaesse tief (dunkel = tief), weich gezeichnet."""
    # Nur was dunkler ist als seine Umgebung (Fugen, Grilloeffnungen, Einlaesse, Scheiben-
    # raender) liegt tief; weiche Schattierung im Bild (Woelbung) gibt keine Dellen.
    lum = rgb.astype(np.float32).mean(axis=2) / 255.0
    umgebung = ndimage.gaussian_filter(lum, 7.0)
    tiefe = np.clip((umgebung - lum) * 4.0, 0.0, 1.0)
    # Grosse sehr dunkle Flaechen (Grill, Einlaesse) zusaetzlich etwas tiefer.
    tiefe = np.maximum(tiefe, np.clip((0.22 - ndimage.gaussian_filter(lum, 2.0)) * 3.0, 0, 0.7))
    h = 1.0 - ndimage.gaussian_filter(tiefe, 0.8)
    a = (h * 255).astype(np.uint8)
    return Image.fromarray(np.dstack([rgb.astype(np.uint8), a]), "RGBA")


def laeufe(spalten: np.ndarray, luecke: int) -> list:
    """Zusammenhaengende Gruppen von Spaltennummern (Luecken bis 'luecke' werden ueberbrueckt)."""
    if len(spalten) == 0:
        return []
    gruppen, start = [], spalten[0]
    for a, b in zip(spalten[:-1], spalten[1:]):
        if b - a > luecke:
            gruppen.append((start, a))
            start = b
    gruppen.append((start, spalten[-1]))
    return gruppen


def baue(modell: str, laenge: float, spiegeln: bool) -> dict:
    farbbild = Image.open(os.path.join(BLAETTER, f"{modell}_farbe.png")).convert("RGB")
    bild = np.asarray(farbbild).astype(np.int32)
    v = ansichten(bild)
    s_sl, s_maske = v["seite"]
    o_sl, o_maske = v["oben"]
    s_rgb = bild[s_sl]
    o_rgb = bild[o_sl]
    if spiegeln:
        s_maske, s_rgb = s_maske[:, ::-1], s_rgb[:, ::-1]
        o_maske, o_rgb = o_maske[:, ::-1], o_rgb[:, ::-1]
    h, w = s_maske.shape
    masst = laenge / w                         # Meter je Pixel
    grau = s_rgb.mean(axis=2)
    satt = s_rgb.max(axis=2) - s_rgb.min(axis=2)

    # Dachlinie: Oberkante des geschlossenen Aufbaus je Spalte (von unten nach oben ohne
    # Luecke; ein Heckfluegel ueber Luft zaehlt nicht), Hoehe ueber dem Boden (Unterkante).
    dach = np.zeros(w)
    unten = np.zeros(w)
    ganz_oben = np.zeros(w)
    for i in range(w):
        col = np.where(s_maske[:, i])[0]
        if not len(col):
            continue
        unten[i] = (h - 1 - col.max()) * masst
        ganz_oben[i] = (h - 1 - col.min()) * masst
        oben = col.max()
        for z in col[::-1]:
            if oben - z > 4:
                break
            oben = z
        dach[i] = (h - 1 - oben) * masst
    dach = median(dach, 11)
    # Hinter dem hoechsten Punkt faellt das Dach nur noch: Fluegelstuetzen und Endplatten
    # ragen sonst als Zacken heraus.
    for i in range(int(np.argmax(dach)) + 1, w):
        dach[i] = min(dach[i], dach[i - 1] + 0.002)
    # Heckfluegel: Spalten, in denen ueber dem Aufbau noch etwas Getrenntes steht.
    ueber = np.where(ganz_oben - dach > 0.12)[0]
    ueber = ueber[ueber > w * 0.6]
    fluegel = None
    if len(ueber) > w * 0.04:
        a, b = int(ueber.min()), int(ueber.max())
        hoch = float(np.median(ganz_oben[a:b + 1]))
        fluegel = {"u": [round(a / w, 4), round(b / w, 4)], "hoehe": round(hoch, 3),
                   "unter": round(float(np.median(dach[a:b + 1])), 3)}

    # Raeder: Spalten mit viel sehr Dunklem (Reifen) im unteren Drittel.
    unteres = slice(int(h * 0.62), h)
    dunkel = (grau[unteres] < 70) & s_maske[unteres]
    anteil = dunkel.sum(axis=0) / (h - int(h * 0.62))
    gruppen = laeufe(np.where(anteil > 0.35)[0], 6)
    gruppen = sorted(gruppen, key=lambda g: g[0] - g[1])[:2]   # die zwei breitesten
    gruppen = sorted(gruppen)
    if len(gruppen) < 2:
        raise SystemExit(f"{modell}: keine zwei Raeder gefunden")
    raeder = []
    for a, b in gruppen:
        mitte = (a + b) // 2
        # Oberkante des Reifens in der Mittelspalte: oberstes dunkles Pixel im unteren Teil.
        streifen = (grau[int(h * 0.45):, mitte - 2:mitte + 3] < 70).any(axis=1)
        zeilen = np.where(streifen)[0]
        durchmesser = (h - (zeilen.min() + int(h * 0.45))) * masst if len(zeilen) else 0.66
        r = float(np.clip(durchmesser / 2.0, 0.28, 0.42))
        raeder.append({"u": round(mitte / w, 4), "r": round(r, 3)})

    # Schwelle: Unterkante zwischen den Raedern.
    zwischen = range(gruppen[0][1] + 6, gruppen[1][0] - 6)
    schwelle = float(np.median([unten[i] for i in zwischen])) if len(zwischen) else 0.2

    # Fensterband: dunkles, ungesaettigtes Glas oberhalb der Raeder, laengster senkrechter Lauf.
    glas = (grau > 25) & (grau < 115) & (satt < 30) & s_maske
    glas[int(h * 0.62):] = False
    spalten_f, boden_f = [], []
    for i in range(w):
        lauf = best = ende = 0
        for z in range(h):
            lauf = lauf + 1 if glas[z, i] else 0
            if lauf > best:
                best, ende = lauf, z
        if best >= 10:
            spalten_f.append(i)
            boden_f.append((h - 1 - ende) * masst)
    gurt = float(np.median(boden_f)) if boden_f else float(dach.max() * 0.68)

    # Kabine, Windschutz- und Heckscheibe aus der Dachlinie: wo das Dach ueber dem Gurt liegt;
    # die Scheiben sind die steigenden und fallenden Stuecke bis zum flachen Dach.
    kabine = np.where(dach > gurt + 0.1)[0]
    flach = np.where(dach > dach.max() - 0.06)[0]
    k0, k1 = int(kabine.min()), int(kabine.max())
    f0, f1 = int(flach.min()), int(flach.max())
    windschutz = [k0 / w, max(f0, k0 + 2) / w]
    heckscheibe = [max(min(f1, k1 - 2), k1 - int(0.16 * w)) / w, k1 / w]
    if spalten_f:
        seitenfenster = [max(min(spalten_f) / w, windschutz[0]), min(max(spalten_f) / w, heckscheibe[1])]
    else:
        seitenfenster = [windschutz[1], heckscheibe[0]]

    # Draufsicht: halbe Breite je Spalte, Aussenspiegel per Median entfernt.
    oh, ow = o_maske.shape
    halb = np.zeros(ow)
    for i in range(ow):
        col = np.where(o_maske[:, i])[0]
        halb[i] = (col.max() - col.min()) / 2 if len(col) else 0.0
    halb = median(halb, 41) * laenge / ow
    # Glaetten: Treppen von Pixel zu Pixel werden sonst im gespiegelten Lack zu Wellen.
    halb = ndimage.gaussian_filter1d(halb, ow / 90.0, mode="nearest")
    dach_glatt = ndimage.gaussian_filter1d(dach, w / 140.0, mode="nearest")

    u = np.linspace(0.0, 1.0, SCHNITTE)

    def probe(reihe, uu):
        return float(np.interp(uu * (len(reihe) - 1), np.arange(len(reihe)), reihe))

    seite_hoehe = h * masst
    profil = {
        "laenge": laenge, "schwelle": round(schwelle, 3), "gurt": round(gurt, 3),
        "dach": [round(probe(dach_glatt, x), 3) for x in u],
        "halbe_breite": [round(probe(halb, x), 3) for x in u],
        "raeder": raeder,
        "seitenfenster": [round(x, 4) for x in seitenfenster],
        "windschutz": [round(x, 4) for x in windschutz],
        "heckscheibe": [round(x, 4) for x in heckscheibe],
        "seite_hoehe": round(seite_hoehe, 3),
        "oben_breite": round(oh * laenge / ow, 3),
    }
    if fluegel:
        profil["fluegel"] = fluegel
    # Texturen: die Ansichten zugeschnitten (Seite und Draufsicht ggf. gespiegelt).
    os.makedirs(TEXTUREN, exist_ok=True)
    mit_hoehe(s_rgb).save(os.path.join(TEXTUREN, f"auto_{modell}_seite.png"))
    mit_hoehe(o_rgb).save(os.path.join(TEXTUREN, f"auto_{modell}_oben.png"))
    for name in ("front", "heck"):
        sl, _ = v[name]
        teil = bild[sl]
        mit_hoehe(teil).save(os.path.join(TEXTUREN, f"auto_{modell}_{name}.png"))
        # Masstab: dieselbe Hoehe wie die Seitenansicht (Boden bis Dach).
        profil[f"{name}_breite"] = round(teil.shape[1] * seite_hoehe / teil.shape[0], 3)
    with open(os.path.join(HIER, "Modelle", f"auto_profil_{modell}.json"), "w") as f:
        json.dump(profil, f, separators=(",", ":"))
    kurz = {k: val for k, val in profil.items() if k not in ("dach", "halbe_breite")}
    print("LALABERG_AUTOMODELL", modell, kurz,
          "dach %.2f breite %.2f" % (max(profil["dach"]), 2 * max(profil["halbe_breite"])))
    return profil


def main() -> None:
    namen = sys.argv[1:] or list(MODELLE)
    for n in namen:
        laenge, spiegeln = MODELLE[n]
        baue(n, laenge, spiegeln)


if __name__ == "__main__":
    main()
