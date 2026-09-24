# Zwei kachelbare Wellen-Tiefenkarten fuer das Wasser (siehe
# Tools/baue_wasser.py und M_Wasser).
#
#   python Tools/baue_wellen.py
#
# Gerechnet als Summe vieler Sinuswellen mit ganzzahligen Wellenvektoren -
# nur so schliesst sich die Kachel nahtlos. Aus dem Hoehenfeld wird die
# Normale gebildet, wie sie Unreal erwartet (DirectX: Y nach unten).
#
#   T_Welle_N     lange Duenung, wenige, grosse Wellen
#   T_Kraeusel_N  kurzes Kraeuseln, viele, kleine Wellen
#
# Zwei verschiedene Karten, im Material mit verschiedenem Tempo und in
# verschiedene Richtungen geschoben - erst das laesst die Oberflaeche leben
# statt zu gleiten.
import math
import os
import random

from PIL import Image

HIER = os.path.dirname(os.path.abspath(__file__))
ZIEL = os.path.join(HIER, "Texturen")
os.makedirs(ZIEL, exist_ok=True)
N = 512


def hoehenfeld(wellen, seed, schaerfe=1.0):
    """Summe von Sinuswellen mit ganzzahligen Wellenzahlen (kachelbar)."""
    r = random.Random(seed)
    teile = []
    for _ in range(wellen):
        # Ganzzahlige Wellenvektoren: sonst bricht die Kachel an der Naht.
        kx = r.randint(-6, 6)
        ky = r.randint(-6, 6)
        if kx == 0 and ky == 0:
            kx = 1
        laenge = math.hypot(kx, ky)
        # Kurze Wellen flacher als lange - wie auf echtem Wasser.
        amplitude = 1.0 / (laenge ** schaerfe)
        teile.append((kx, ky, amplitude, r.uniform(0, 2 * math.pi)))
    feld = [[0.0] * N for _ in range(N)]
    for y in range(N):
        v = y / N * 2 * math.pi
        for kx, ky, a, phase in teile:
            # Zeilenweise vorberechnen spart die Haelfte der Sinusaufrufe.
            grund = ky * v + phase
            for x in range(N):
                feld[y][x] += a * math.sin(kx * (x / N * 2 * math.pi) + grund)
    return feld


def normalkarte(feld, staerke):
    bild = Image.new("RGB", (N, N))
    px = bild.load()
    for y in range(N):
        for x in range(N):
            dx = feld[y][(x + 1) % N] - feld[y][(x - 1) % N]
            dy = feld[(y + 1) % N][x] - feld[(y - 1) % N][x]
            nx, ny, nz = -dx * staerke, -dy * staerke, 1.0
            laenge = math.sqrt(nx * nx + ny * ny + nz * nz)
            nx, ny, nz = nx / laenge, ny / laenge, nz / laenge
            # DirectX-Normalen: gruen nach unten.
            px[x, y] = (int((nx * 0.5 + 0.5) * 255), int((-ny * 0.5 + 0.5) * 255), int((nz * 0.5 + 0.5) * 255))
    return bild


def speichere(name, bild):
    pfad = os.path.join(ZIEL, name + ".png")
    bild.save(pfad, optimize=True)
    print("LALABERG_WELLE %s %s" % (name, bild.size))


if __name__ == "__main__":
    speichere("T_Welle_N", normalkarte(hoehenfeld(14, 3, schaerfe=1.6), 26.0))
    speichere("T_Kraeusel_N", normalkarte(hoehenfeld(40, 7, schaerfe=0.9), 12.0))
