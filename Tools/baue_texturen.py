# Erzeugt kachelbare Detailtexturen ohne Fremdmaterial: reines Wertrauschen
# auf einem periodischen Gitter, dazu je Klasse ein Muster. Die Bilder sind
# Helligkeitsmodulatoren um 1.0 - im Material werden sie mit der amtlichen
# Scheitelfarbe multipliziert. Dadurch bleibt die Farbgebung der Stadt echt,
# und die Flaechen verlieren ihr Plastikaussehen.
#
#   python Tools/baue_texturen.py            (schreibt nach Tools/Texturen/)
#
# Nur Standardbibliothek: das Skript laeuft auch ohne numpy oder Pillow.

import math, os, random, struct, zlib

GROESSE = 512
ZIEL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Texturen")


def gitter(zellen, saat):
    r = random.Random(saat)
    return [[r.random() for _ in range(zellen)] for _ in range(zellen)]


def weich(t):
    return t * t * (3 - 2 * t)


def rauschen(zellen, saat):
    """Wertrauschen, das sich nahtlos wiederholt: das Gitter wird zyklisch
    gelesen, deshalb passt der rechte Rand auf den linken."""
    g = gitter(zellen, saat)
    feld = [[0.0] * GROESSE for _ in range(GROESSE)]
    schritt = GROESSE / zellen
    for y in range(GROESSE):
        fy = y / schritt
        y0 = int(fy) % zellen
        y1 = (y0 + 1) % zellen
        ty = weich(fy - int(fy))
        for x in range(GROESSE):
            fx = x / schritt
            x0 = int(fx) % zellen
            x1 = (x0 + 1) % zellen
            tx = weich(fx - int(fx))
            a = g[y0][x0] * (1 - tx) + g[y0][x1] * tx
            b = g[y1][x0] * (1 - tx) + g[y1][x1] * tx
            feld[y][x] = a * (1 - ty) + b * ty
    return feld


def oktaven(stufen, saat):
    feld = [[0.0] * GROESSE for _ in range(GROESSE)]
    gewicht = 0.0
    for i, zellen in enumerate(stufen):
        g = 1.0 / (i + 1)
        r = rauschen(zellen, saat + i * 977)
        for y in range(GROESSE):
            zr, zf = r[y], feld[y]
            for x in range(GROESSE):
                zf[x] += zr[x] * g
        gewicht += g
    for y in range(GROESSE):
        for x in range(GROESSE):
            feld[y][x] /= gewicht
    return feld


def schreibe(name, pixel):
    """Minimaler PNG-Schreiber: Farbtyp 2 (RGB), 8 Bit, keine Filterung."""
    roh = bytearray()
    for zeile in pixel:
        roh.append(0)
        for (r, g, b) in zeile:
            roh += bytes((r, g, b))

    def block(art, daten):
        return (struct.pack(">I", len(daten)) + art + daten +
                struct.pack(">I", zlib.crc32(art + daten) & 0xFFFFFFFF))

    kopf = struct.pack(">IIBBBBB", GROESSE, GROESSE, 8, 2, 0, 0, 0)
    datei = (b"\x89PNG\r\n\x1a\n" + block(b"IHDR", kopf) +
             block(b"IDAT", zlib.compress(bytes(roh), 9)) + block(b"IEND", b""))
    os.makedirs(ZIEL, exist_ok=True)
    pfad = os.path.join(ZIEL, name + ".png")
    with open(pfad, "wb") as f:
        f.write(datei)
    return pfad


def grau(wert):
    v = max(0, min(255, int(wert * 255 + 0.5)))
    return (v, v, v)


def putz():
    fein = oktaven([128, 64], 11)
    grob = oktaven([8, 16], 23)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (fein[y][x] - 0.5) * 0.14 + (grob[y][x] - 0.5) * 0.13
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


def ziegel():
    """Waagerechte Ziegelreihen mit versetzter Teilung, dazu Verwitterung."""
    fleck = oktaven([12, 48], 37)
    reihe_h = GROESSE / 16.0
    ziegel_b = GROESSE / 10.0
    aus = []
    for y in range(GROESSE):
        reihe = int(y / reihe_h)
        versatz = (reihe % 2) * ziegel_b * 0.5
        dy = (y % reihe_h) / reihe_h
        zeile = []
        for x in range(GROESSE):
            dx = ((x + versatz) % ziegel_b) / ziegel_b
            v = 1.0 + (fleck[y][x] - 0.5) * 0.22
            if dy < 0.10:
                v *= 0.62                      # Fuge zwischen den Reihen
            elif dy > 0.86:
                v *= 1.10                      # Lichtkante der Ziegelnase
            if dx < 0.06:
                v *= 0.78                      # senkrechte Fuge
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


def asphalt():
    korn = oktaven([256, 128], 53)
    risse = oktaven([6, 24], 71)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (korn[y][x] - 0.5) * 0.26 + (risse[y][x] - 0.5) * 0.10
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


def wiese():
    bueschel = oktaven([64, 160], 89)
    lage = oktaven([5, 11], 97)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (bueschel[y][x] - 0.5) * 0.30 + (lage[y][x] - 0.5) * 0.24
            g = max(0, min(255, int(v * 128 + 0.5)))
            zeile.append((int(g * 0.92), g, int(g * 0.82)))   # leicht ins Gruene
        aus.append(zeile)
    return aus


def wasser():
    wellen = oktaven([24, 96], 131)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (wellen[y][x] - 0.5) * 0.16
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


def fassade():
    """Eine Kachel ist ein Fensterachsenfeld: 2,60 m breit, 3,20 m hoch. Das
    Fenster sitzt 0,95 m ueber dem Geschossboden, ist 1,15 m breit und 1,45 m
    hoch - Landsberger Altstadtmass, hochrechteckig, mit Sprosse und Bank.
    Der Wert 1.0 heisst Putz, dunkler heisst Fenster."""
    korn = oktaven([96, 192], 17)
    grob = oktaven([6, 14], 29)
    px_x = GROESSE / 2.60          # Bildpunkte je Meter, waagerecht
    px_y = GROESSE / 3.20
    f_x0, f_x1 = (2.60 - 1.15) / 2, (2.60 + 1.15) / 2
    f_y0, f_y1 = 0.95, 0.95 + 1.45
    rahmen = 0.06                  # Stock und Fluegelrahmen
    aus = []
    for y in range(GROESSE):
        # Zeile 0 ist oben, die Geschosshoehe zaehlt von unten.
        my = (GROESSE - 1 - y) / px_y
        zeile = []
        for x in range(GROESSE):
            mx = x / px_x
            v = 1.0 + (korn[y][x] - 0.5) * 0.10 + (grob[y][x] - 0.5) * 0.11
            innen = f_x0 <= mx <= f_x1 and f_y0 <= my <= f_y1
            if innen:
                am_rand = (mx - f_x0 < rahmen or f_x1 - mx < rahmen or
                           my - f_y0 < rahmen or f_y1 - my < rahmen)
                sprosse = abs(mx - (f_x0 + f_x1) / 2) < 0.035
                kaempfer = abs(my - (f_y0 + f_y1 * 1.02) / 2) < 0.030
                if am_rand or sprosse or kaempfer:
                    v = 1.06                      # heller Fensterstock
                else:
                    # Glas: oben Himmelsspiegelung, unten Raumtiefe
                    tiefe = (my - f_y0) / (f_y1 - f_y0)
                    v = 0.20 + 0.26 * tiefe
            elif f_x0 - 0.09 <= mx <= f_x1 + 0.09 and f_y0 - 0.09 <= my < f_y0:
                v = 1.16                          # Fensterbank
            elif f_x0 - 0.05 <= mx <= f_x1 + 0.05 and f_y1 < my <= f_y1 + 0.07:
                v = 1.10                          # Sturz
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


if __name__ == "__main__":
    for name, bau in (("T_Putz_D", putz), ("T_Ziegel_D", ziegel), ("T_Asphalt_D", asphalt),
                      ("T_Wiese_D", wiese), ("T_Wasser_D", wasser), ("T_Fassade_D", fassade)):
        pfad = schreibe(name, bau())
        print("%s %d Byte" % (pfad, os.path.getsize(pfad)))
