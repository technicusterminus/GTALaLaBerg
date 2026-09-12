# Erzeugt T_Farbklecks_A.png rein rechnerisch (kein Download): eine weiche,
# unregelmaessige Spritzer-Maske als Graustufenbild (weiss=deckend, schwarz=
# durchsichtig). Ersetzt den gescheiterten Versuch, den weichen Rand im
# Material-Editor selbst zu verrechnen (siehe baue_farbklecks.py) - eine
# fertige Textur im Opacity-Kanal braucht keinen radialen Verlaufsknoten.
import math
import random
from PIL import Image, ImageFilter

random.seed(4711)
W, H = 256, 256
CX, CY = W / 2, H / 2

# Unregelmaessiger Rand statt eines perfekten Kreises: Radius je Winkel aus
# ein paar ueberlagerten Sinuswellen mit zufaelliger Phase - das wirkt eher
# wie ein Farbspritzer als wie eine Muenze.
wellen = [(random.uniform(2, 5), random.uniform(0.08, 0.22), random.uniform(0, 2 * math.pi)) for _ in range(4)]


def radius_bei(winkel):
    r = 0.72
    for freq, staerke, phase in wellen:
        r += staerke * math.sin(freq * winkel + phase)
    return max(0.35, min(0.95, r)) * (W / 2)


bild = Image.new("L", (W, H), 0)
px = bild.load()
for y in range(H):
    for x in range(W):
        dx, dy = x - CX, y - CY
        dist = math.hypot(dx, dy)
        winkel = math.atan2(dy, dx)
        rand = radius_bei(winkel)
        # Weicher Uebergang ueber die letzten 30% des Radius statt einer
        # harten Kante - das ist der eigentliche Kern des Effekts.
        weich = rand * 0.3
        if dist >= rand:
            deckung = 0.0
        elif dist <= rand - weich:
            deckung = 1.0
        else:
            deckung = 1.0 - (dist - (rand - weich)) / weich
        px[x, y] = int(255 * deckung)

# Ein paar kleine Spritzertropfen um den Hauptfleck - macht aus der reinen
# Silhouette einen glaubwuerdigeren Treffer.
for _ in range(6):
    a = random.uniform(0, 2 * math.pi)
    d = random.uniform(W * 0.32, W * 0.46)
    tx, ty = CX + math.cos(a) * d, CY + math.sin(a) * d
    r = random.uniform(4, 11)
    for y in range(max(0, int(ty - r - 2)), min(H, int(ty + r + 2))):
        for x in range(max(0, int(tx - r - 2)), min(W, int(tx + r + 2))):
            dist = math.hypot(x - tx, y - ty)
            if dist < r:
                deckung = 1.0 - max(0.0, (dist - r * 0.5) / (r * 0.5))
                px[x, y] = max(px[x, y], int(255 * min(1.0, deckung)))

bild = bild.filter(ImageFilter.GaussianBlur(radius=1.2))

ziel = __file__.replace("erzeuge_klecks_textur.py", "Texturen/T_Farbklecks_A.png")
bild.save(ziel)
print("LALABERG_KLECKSTEXTUR " + ziel)
