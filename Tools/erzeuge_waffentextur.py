# Erzeugt T_Waffenmetall_D.png rein rechnerisch (kein Download, keine fremde
# Quelle): feine horizontale Schleifriefen wie bei gebuerstetem Stahl/Polymer,
# plus ein paar zufaellige Kratzer. Dient als Detailtextur ueber der
# Scheitelfarbe der Waffe (siehe baue_waffenmetall.py) - vorher lag auf dem
# Modell nur eine flache Vertexfarbe ohne jede Oberflaechenstruktur.
import random
from PIL import Image

random.seed(20260911)
W, H = 512, 512
bild = Image.new("L", (W, H))
px = bild.load()

for y in range(H):
    # Feine Riefen in Laufrichtung: ein Grundton je Zeile, leicht verrauscht.
    riefe = 128 + int(14 * ((y * 37) % 23 - 11) / 11)
    for x in range(W):
        rauschen = random.randint(-10, 10)
        px[x, y] = max(0, min(255, riefe + rauschen))

# Ein paar laengere, hellere Kratzer quer zur Riefenrichtung.
for _ in range(28):
    x0 = random.randint(0, W - 1)
    y0 = random.randint(0, H - 1)
    laenge = random.randint(20, 90)
    winkel = random.uniform(-0.35, 0.35)
    hell = random.randint(30, 60)
    for i in range(laenge):
        x = int(x0 + i)
        y = int(y0 + i * winkel)
        if 0 <= x < W and 0 <= y < H:
            px[x, y] = min(255, px[x, y] + hell)
            if y + 1 < H:
                px[x, y + 1] = min(255, px[x, y + 1] + hell // 2)

ziel = __file__.replace("erzeuge_waffentextur.py", "Texturen/T_Waffenmetall_D.png")
bild.save(ziel)
print("LALABERG_WAFFENTEXTUR " + ziel)
