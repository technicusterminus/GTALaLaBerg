# Liest aus einer Konstruktionszeichnung (Seitenansicht oben, Draufsicht darunter, vom
# Bildmodell auf OpenRouter erzeugt, siehe Docs/Konzept/auto-konstruktionszeichnung.png)
# die Umrisse eines Kleinwagens und schreibt Tools/Modelle/auto_profil.json:
# Dachlinie und halbe Breite je Laengsschnitt, Fensterband, Gurtlinie, Radpositionen.
# Godot (Godot/scripts/wagenmesh.gd) baut daraus die glatte Karosserie.
#
#   python Tools/baue_auto_profil.py [Zeichnung.png]
import json
import os
import sys

import numpy as np
from PIL import Image

from baue_auto_modelle import speichere_ansicht   # Farbe und Masken (Lack, Glas, Lampen, Hoehe)

HIER = os.path.dirname(os.path.abspath(__file__))
QUELLE = sys.argv[1] if len(sys.argv) > 1 else os.path.join(
    HIER, "..", "Docs", "Konzept", "auto-konstruktionszeichnung.png")
ZIEL = os.path.join(HIER, "Modelle", "auto_profil.json")
LAENGE_M = 4.3      # so lang wird das Auto im Spiel (wie das alte Modell)
SCHNITTE = 72


def median(a: np.ndarray, fenster: int) -> np.ndarray:
    r = fenster // 2
    pad = np.pad(a, r, mode="edge")
    return np.array([np.median(pad[i:i + fenster]) for i in range(len(a))])


def main() -> None:
    bild = np.asarray(Image.open(QUELLE).convert("RGB")).astype(np.int32)
    hoehe, breite = bild.shape[:2]
    grau = bild.mean(axis=2)
    farbig = bild.min(axis=2) < 225           # nicht weiss
    # Seite oben, Draufsicht darunter: die leere Zeilenluecke trennt sie.
    zeilen = farbig[:, : int(breite * 0.65)].sum(axis=1)
    leer = np.where(zeilen[int(hoehe * 0.3): int(hoehe * 0.7)] < 3)[0] + int(hoehe * 0.3)
    trenn = int(leer.mean()) if len(leer) else hoehe // 2
    seite = farbig[:trenn, : int(breite * 0.65)]
    oben = farbig[trenn:, : int(breite * 0.65)]

    ys, xs = np.where(seite)
    sx0, sx1, sy0, sy1 = xs.min(), xs.max(), ys.min(), ys.max()
    masst = LAENGE_M / (sx1 - sx0)            # Meter je Pixel
    breite_px = sx1 - sx0 + 1

    # Dachlinie (Oberkante der Silhouette) je Spalte, Hoehe ueber dem Boden.
    dach = np.zeros(breite_px)
    for i in range(breite_px):
        col = np.where(seite[:, sx0 + i])[0]
        dach[i] = (sy1 - col.min()) * masst if len(col) else 0.0
    dach = median(dach, 5)

    # Raeder: sehr dunkle Pixel (Reifen) unten.
    dunkel = (grau[:trenn, sx0:sx1 + 1] < 55)
    reifen = dunkel[sy1 - int(0.3 / masst): sy1 + 1].sum(axis=0)
    spalten = np.where(reifen > 6)[0]
    gruppen, start = [], spalten[0]
    for a, b in zip(spalten[:-1], spalten[1:]):
        if b - a > 12:
            gruppen.append((start, a))
            start = b
    gruppen.append((start, spalten[-1]))
    gruppen = sorted(gruppen, key=lambda g: g[0] - g[1])[:2]
    gruppen = sorted(gruppen)
    raeder = []
    for a, b in gruppen:
        zr, _ = np.where(dunkel[:, a:b + 1])
        zr = zr[zr > sy1 - int(0.5 / masst) - sy0 - 5]
        radius = (b - a) / 2 * masst
        raeder.append({"x": float(((a + b) / 2) * masst), "r": float(min(radius, 0.34))})

    # Schwelle: Unterkante des Aufbaus zwischen den Raedern.
    zwischen = range(int(gruppen[0][1]) + 8, int(gruppen[1][0]) - 8)
    unten = []
    for i in zwischen:
        col = np.where(seite[:, sx0 + i])[0]
        unten.append((sy1 - col.max()) * masst)
    schwelle = float(np.median(unten))

    # Fensterband: dunkle, nicht ganz schwarze Flaechen oberhalb der Raeder.
    rad_oben = sy1 - 2 * raeder[0]["r"] / masst
    fenster = (grau[:trenn, sx0:sx1 + 1] > 38) & (grau[:trenn, sx0:sx1 + 1] < 105)
    fenster[int(rad_oben - sy0):] = False
    fenster &= (bild[:trenn, sx0:sx1 + 1].max(axis=2) - bild[:trenn, sx0:sx1 + 1].min(axis=2) < 40)
    # Pro Spalte zaehlt nur der laengste senkrechte Lauf; duenne Linien (Tuerfugen, Umrisse)
    # sind kuerzer als 14 Pixel und fallen heraus.
    spalten_f, boden_f = [], []
    for i in range(fenster.shape[1]):
        lauf = best = ende = 0
        for z in range(fenster.shape[0]):
            lauf = lauf + 1 if fenster[z, i] else 0
            if lauf > best:
                best, ende = lauf, z
        if best >= 14:
            spalten_f.append(i)
            boden_f.append((sy1 - ende) * masst)
    spalten_f = np.asarray(spalten_f)
    gurt = float(np.median(boden_f))
    fenster_von = min(float(spalten_f.min() * masst), 0.31 * LAENGE_M)
    fenster_bis = max(float(spalten_f.max() * masst), 0.86 * LAENGE_M)

    # Draufsicht: halbe Breite je Spalte, Aussenspiegel per Median entfernt.
    oy, ox = np.where(oben)
    ox0, ox1 = ox.min(), ox.max()
    halb = np.zeros(ox1 - ox0 + 1)
    for i in range(len(halb)):
        col = np.where(oben[:, ox0 + i])[0]
        halb[i] = (col.max() - col.min()) / 2 if len(col) else 0.0
    halb = median(halb, 41)
    # Windschutz- und Heckscheibe: dunkle Pixel auf der Mittellinie der Draufsicht.
    mitte_y = int((oy.min() + oy.max()) / 2)
    mittel = grau[trenn + mitte_y - 4: trenn + mitte_y + 5, ox0:ox1 + 1].mean(axis=0)
    scheibe = np.where(mittel < 100)[0]
    scheibe = scheibe[(scheibe > 0.08 * len(halb)) & (scheibe < 0.94 * len(halb))]
    gruppen_s, st = [], scheibe[0]
    for a, b in zip(scheibe[:-1], scheibe[1:]):
        if b - a > 12:
            gruppen_s.append((st, a))
            st = b
    gruppen_s.append((st, scheibe[-1]))
    gruppen_s = [g for g in gruppen_s if g[1] - g[0] > 0.04 * len(halb)]
    vorn = gruppen_s[0]
    hinten = gruppen_s[-1]

    u = np.linspace(0.0, 1.0, SCHNITTE)
    def probe(reihe, uu):
        return float(np.interp(uu * (len(reihe) - 1), np.arange(len(reihe)), reihe))
    profil = {
        "laenge": LAENGE_M, "schwelle": round(schwelle, 3), "gurt": round(gurt, 3),
        "dach": [round(probe(dach, x), 3) for x in u],
        "halbe_breite": [round(probe(halb, x) * LAENGE_M / len(halb), 3) for x in u],
        "raeder": [{"u": round(w["x"] / LAENGE_M, 4), "r": round(w["r"], 3)} for w in raeder],
        "seitenfenster": [round(fenster_von / LAENGE_M, 4), round(min(fenster_bis / LAENGE_M, 0.88), 4)],
        "windschutz": [round(vorn[0] / len(halb), 4), round(vorn[1] / len(halb), 4)],
        "heckscheibe": [round(hinten[0] / len(halb), 4), round(hinten[1] / len(halb), 4)],
    }
    # Detailtextur: die vom Bildmodell farbig ausgearbeitete Fassung der Zeichnung
    # (Docs/Konzept/auto-farbtextur.png, gleiches Layout), sonst die Zeichnung selbst.
    # Zugeschnitten auf die Umrisse jeder Ansicht; u laeuft ueber die Laenge (vorn = 0).
    farbe_pfad = os.path.join(HIER, "..", "Docs", "Konzept", "auto-farbtextur.png")
    bild_rgb = Image.open(farbe_pfad if os.path.exists(farbe_pfad) else QUELLE).convert("RGB")
    # Vom Bildmodell eingefaerbtes Blatt (Lack gruen, Glas magenta, Lampen gelb), gleiches Layout.
    seg_pfad = os.path.join(HIER, "..", "Docs", "Konzept", "auto-maske.png")
    seg_bild = Image.open(seg_pfad).convert("RGB").resize(bild_rgb.size, Image.NEAREST)

    def ansicht(kasten, name):
        speichere_ansicht(np.asarray(bild_rgb.crop(kasten)), np.asarray(seg_bild.crop(kasten)),
                          "auto_", name)
    texturen = os.path.join(HIER, "..", "Godot", "texturen")
    os.makedirs(texturen, exist_ok=True)
    ansicht((int(sx0), int(sy0), int(sx1) + 1, int(sy1) + 1), "seite")
    ansicht((int(ox0), int(trenn + oy.min()), int(ox1) + 1, int(trenn + oy.max()) + 1), "oben")
    profil["seite_hoehe"] = round(float((sy1 - sy0 + 1) * masst), 3)
    profil["oben_breite"] = round(float((oy.max() - oy.min() + 1) * LAENGE_M / (ox1 - ox0 + 1)), 3)
    # Front (unten rechts) und Heck (oben rechts) stehen rechts neben den beiden Ansichten.
    farbig_f = np.asarray(bild_rgb).min(axis=2) < 225
    rechts = farbig_f[:, int(breite * 0.65):]
    for name, zeilen_von, zeilen_bis in (("heck", 0, trenn), ("front", trenn, hoehe)):
        ys2, xs2 = np.where(rechts[zeilen_von:zeilen_bis])
        if len(ys2) < 100:
            continue
        x0, x1 = xs2.min() + int(breite * 0.65), xs2.max() + int(breite * 0.65)
        y0, y1 = ys2.min() + zeilen_von, ys2.max() + zeilen_von
        ansicht((int(x0), int(y0), int(x1) + 1, int(y1) + 1), name)
        # Masstab: dieselbe Hoehe wie die Seitenansicht (Boden bis Dach).
        hoehe_m = profil["seite_hoehe"]
        profil[f"{name}_breite"] = round(float((x1 - x0 + 1) * hoehe_m / (y1 - y0 + 1)), 3)
    with open(ZIEL, "w") as f:
        json.dump(profil, f, separators=(",", ":"))
    print("LALABERG_AUTOPROFIL", {k: v for k, v in profil.items() if k not in ("dach", "halbe_breite")})
    print("dach max %.2f  breite max %.2f" % (max(profil["dach"]), 2 * max(profil["halbe_breite"])))


if __name__ == "__main__":
    main()
