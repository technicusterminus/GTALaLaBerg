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


def farbig(r, g, b):
    """Ein Modulator je Kanal: 0.5 im Bild heisst 1.0 im Material."""
    return tuple(max(0, min(255, int(c * 0.5 * 255 + 0.5))) for c in (r, g, b))


# Staerken: frueher lagen alle Modulatoren bei +-7..10 % - im Spiel war davon
# nichts zu sehen, die Stadt wirkte wie aus Pappe (Rueckmeldung 2026-09-21).
# Jetzt deutlich sichtbar, aber so, dass die amtliche Scheitelfarbe die
# Grundfarbe bleibt.

def putz():
    fein = oktaven([128, 64], 11)
    grob = oktaven([8, 16], 23)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (fein[y][x] - 0.5) * 0.34 + (grob[y][x] - 0.5) * 0.30
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


def ziegel():
    """Biberschwanzdeckung wie auf den Altstadtdaechern: versetzte Reihen,
    unten gerundete Ziegel, jeder Ziegel etwas anders gebrannt, dunkle Fugen.
    Insgesamt waermer und dunkler als die Scheitelfarbe - aus Lachsrosa wird
    Ziegelrot."""
    fleck = oktaven([12, 48], 37)
    korn = oktaven([128, 256], 41)
    reihen, spalten = 18, 12
    reihe_h = GROESSE / float(reihen)
    ziegel_b = GROESSE / float(spalten)
    import random
    zufall = random.Random(5)
    ton = [[zufall.uniform(-1.0, 1.0) for _ in range(spalten)] for _ in range(reihen)]
    aus = []
    for y in range(GROESSE):
        reihe = int(y / reihe_h) % reihen
        versatz = (reihe % 2) * ziegel_b * 0.5
        dy = (y % reihe_h) / reihe_h
        zeile = []
        for x in range(GROESSE):
            xs = (x + versatz) % GROESSE
            spalte = int(xs / ziegel_b) % spalten
            dx = (xs % ziegel_b) / ziegel_b
            t = ton[reihe][spalte]
            v = 0.86 + t * 0.16 + (fleck[y][x] - 0.5) * 0.30 + (korn[y][x] - 0.5) * 0.12
            # Gerundetes Ziegelende: am unteren Rand jeder Reihe wird der
            # Ziegel zu den Seiten hin schmaler, dahinter die Fuge.
            rund = 0.5 - ((dx - 0.5) ** 2) * 2.0
            if dy > 0.78 + rund * 0.35 or dx < 0.035 or dx > 0.965:
                v *= 0.55
            elif dy < 0.12:
                v *= 0.80                      # Schatten der Reihe darueber
            warm = 1.0 + t * 0.05
            zeile.append(farbig(v * 1.06 * warm, v * 0.92, v * 0.82))
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
    trocken = oktaven([3, 7], 101)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (bueschel[y][x] - 0.5) * 0.55 + (lage[y][x] - 0.5) * 0.45
            # Trockene, gelbliche Stellen und satte, dunkle Mulden.
            gelb = max(0.0, trocken[y][x] - 0.55) * 1.6
            zeile.append(farbig(v * (0.92 + gelb * 0.35), v * (1.0 + gelb * 0.10), v * (0.80 - gelb * 0.20)))
        aus.append(zeile)
    return aus


def wasser():
    wellen = oktaven([24, 96], 131)
    zug = oktaven([4, 8], 137)
    aus = []
    for y in range(GROESSE):
        zeile = []
        for x in range(GROESSE):
            v = 1.0 + (wellen[y][x] - 0.5) * 0.40 + (zug[y][x] - 0.5) * 0.30
            zeile.append(farbig(v * 0.95, v * 1.0, v * 1.05))
        aus.append(zeile)
    return aus


def fassade():
    """Eine Kachel ist ein Fensterachsenfeld: 2,60 m breit, 3,20 m hoch. Das
    Fenster sitzt 0,95 m ueber dem Geschossboden, ist 1,15 m breit und 1,45 m
    hoch - Landsberger Altstadtmass, hochrechteckig, mit Sprosse und Bank.
    Der Wert 1.0 heisst Putz, dunkler heisst Fenster. Dazu sichtbarer
    Kalkputz, Schmutzfahnen unter den Fensterbaenken und der Schatten des
    Geschossgesimses."""
    korn = oktaven([96, 192], 17)
    grob = oktaven([6, 14], 29)
    fahne = oktaven([48, 24], 43)
    px_x = GROESSE / 2.60          # Bildpunkte je Meter, waagerecht
    px_y = GROESSE / 3.20
    f_x0, f_x1 = (2.60 - 1.15) / 2, (2.60 + 1.15) / 2
    f_y0, f_y1 = 0.95, 0.95 + 1.45
    rahmen = 0.07                  # Stock und Fluegelrahmen
    aus = []
    for y in range(GROESSE):
        # Zeile 0 ist oben, die Geschosshoehe zaehlt von unten.
        my = (GROESSE - 1 - y) / px_y
        zeile = []
        for x in range(GROESSE):
            mx = x / px_x
            v = 1.0 + (korn[y][x] - 0.5) * 0.26 + (grob[y][x] - 0.5) * 0.28
            # Schmutzfahne: unter der Fensterbank nach unten auslaufend.
            if f_x0 - 0.05 <= mx <= f_x1 + 0.05 and my < f_y0 - 0.09:
                tief = (f_y0 - 0.09 - my) / (f_y0 - 0.09)
                v *= 1.0 - (1.0 - tief) * 0.22 * (0.6 + fahne[y][x] * 0.8)
            # Unter dem Gesims des naechsten Geschosses liegt Schatten.
            if my > 3.02:
                v *= 0.78
            elif my > 2.85:
                v *= 0.90
            innen = f_x0 <= mx <= f_x1 and f_y0 <= my <= f_y1
            if innen:
                am_rand = (mx - f_x0 < rahmen or f_x1 - mx < rahmen or
                           my - f_y0 < rahmen or f_y1 - my < rahmen)
                sprosse = abs(mx - (f_x0 + f_x1) / 2) < 0.035
                kaempfer = abs(my - (f_y0 + f_y1 * 1.02) / 2) < 0.030
                if am_rand or sprosse or kaempfer:
                    v = 1.45                      # weisser Fensterstock
                else:
                    # Glas: oben Himmelsspiegelung, unten Raumtiefe
                    tiefe = (my - f_y0) / (f_y1 - f_y0)
                    v = 0.12 + 0.30 * tiefe
            elif f_x0 - 0.10 <= mx <= f_x1 + 0.10 and f_y0 - 0.09 <= my < f_y0:
                v = 1.40                          # Fensterbank
            elif f_x0 - 0.06 <= mx <= f_x1 + 0.06 and f_y1 < my <= f_y1 + 0.08:
                v = 1.25                          # Sturz
            zeile.append(grau(v * 0.5))
        aus.append(zeile)
    return aus


if __name__ == "__main__":
    for name, bau in (("T_Putz_D", putz), ("T_Ziegel_D", ziegel), ("T_Asphalt_D", asphalt),
                      ("T_Wiese_D", wiese), ("T_Wasser_D", wasser), ("T_Fassade_D", fassade)):
        pfad = schreibe(name, bau())
        print("%s %d Byte" % (pfad, os.path.getsize(pfad)))
