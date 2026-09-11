# Erzeugt T_Waffenmetall_D.png rein rechnerisch (kein Download, keine fremde
# Quelle): weich verwaschenes Rauschen wie bei gebuerstetem Stahl/Polymer,
# plus ein paar Kratzer. Dient als Detailtextur ueber der Scheitelfarbe der
# Waffe (siehe baue_waffenmetall.py) - vorher lag auf dem Modell nur eine
# flache Vertexfarbe ohne jede Oberflaechenstruktur.
#
# Wichtig: reines Pixelrauschen (jede Zeile/Spalte unabhaengig zufaellig)
# wirkt beim FPS-Ansichtsmodell aus naechster Naehe wie grobe, brettartige
# Streifen, weil dort nur ein winziger UV-Ausschnitt stark vergroessert
# sichtbar ist - benachbarte Texel duerfen sich dann nicht sprunghaft
# unterscheiden. Deshalb wird das Rauschen mit einem Weichzeichner deutlich
# verwaschen, bevor die Kratzer aufgesetzt werden.
import random
from PIL import Image, ImageFilter

random.seed(20260911)
W, H = 512, 512
grund = Image.new("L", (W, H))
px = grund.load()
for y in range(H):
    for x in range(W):
        px[x, y] = random.randint(90, 166)
# Stark weichzeichnen: aus hartem Pixelrauschen wird sanfte, grossflaechige
# Wolkigkeit - genau das gewuenschte "gebuerstet", keine harten Kanten.
bild = grund.filter(ImageFilter.GaussianBlur(radius=6))
px = bild.load()

# Ein paar duenne, kurze Kratzer - einzeln kaum breiter als ein Texel, damit
# sie auch bei starker Vergroesserung nicht zu Balken werden.
for _ in range(40):
    x0 = random.randint(0, W - 1)
    y0 = random.randint(0, H - 1)
    laenge = random.randint(8, 30)
    winkel = random.uniform(-0.3, 0.3)
    hell = random.randint(18, 34)
    for i in range(laenge):
        x = int(x0 + i)
        y = int(y0 + i * winkel)
        if 0 <= x < W and 0 <= y < H:
            px[x, y] = min(255, px[x, y] + hell)

bild = bild.filter(ImageFilter.GaussianBlur(radius=0.6))

ziel = __file__.replace("erzeuge_waffentextur.py", "Texturen/T_Waffenmetall_D.png")
bild.save(ziel)
print("LALABERG_WAFFENTEXTUR " + ziel)
