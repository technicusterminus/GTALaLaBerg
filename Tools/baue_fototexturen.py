# Detailtexturen aus Fotomaterialien von ambientCG (CC0, https://ambientcg.com,
# keine Namensnennung noetig). Wie bei baue_texturen.py sind die Bilder
# Modulatoren um 1.0: die Farbe jedes Gebaeudes kommt weiter aus der
# Scheitelfarbe, das Foto bringt Struktur und anteilig seinen Farbton.
#
#   python Tools/baue_fototexturen.py <Ordner mit den ambientCG-Zips>
#
# Erwartet die 1K-JPG-Pakete (z.B. RoofingTiles011A_1K-JPG.zip); die Zips
# selbst gehoeren nicht ins Repo. Schreibt nach Tools/Texturen/:
#   T_Ziegel_D    <- RoofingTiles011A   (Ton, rotbraun)
#   T_Pflaster_D  <- PavingStones051    (Kleinpflaster in Boegen)
#   T_Wiese_D     <- Grass004
#   T_Fassade_D   <- Fensterraster aus baue_texturen.py, Putzkorn aus PaintedPlaster017
import io, os, sys, zipfile
import importlib.util
from PIL import Image, ImageChops, ImageStat

HIER = os.path.dirname(os.path.abspath(__file__))
ZIEL = os.path.join(HIER, "Texturen")
QUELLE = sys.argv[1] if len(sys.argv) > 1 else os.environ.get("LALABERG_AMBIENTCG", "")


def lade(asset, karte):
    with zipfile.ZipFile(os.path.join(QUELLE, asset + "_1K-JPG.zip")) as z:
        return Image.open(io.BytesIO(z.read("%s_1K-JPG_%s.jpg" % (asset, karte)))).convert("RGB")


def modulator(bild, farbanteil, helligkeit=1.0, staerke=1.0, ao=None):
    """Foto -> Modulator: Helligkeit geteilt durch ihren Mittelwert (1.0 =
    Scheitelfarbe unveraendert), der Farbton des Fotos nur zu farbanteil."""
    if ao is not None:
        # Umgebungsverdeckung einrechnen: Fugen und Ziegelunterkanten dunkel.
        a = ao.convert("L").resize(bild.size)
        bild = ImageChops.multiply(bild, Image.merge("RGB", (a, a, a)))
    mittel = ImageStat.Stat(bild).mean
    lum = sum(mittel) / 3.0
    px = bild.load()
    aus = Image.new("RGB", bild.size)
    ap = aus.load()
    for y in range(bild.size[1]):
        for x in range(bild.size[0]):
            r, g, b = px[x, y]
            l = (r + g + b) / 3.0 / lum
            werte = []
            for c in (r, g, b):
                farbig = c / lum
                v = l + (farbig - l) * farbanteil
                v = 1.0 + (v - 1.0) * staerke
                werte.append(max(0, min(255, int(v * helligkeit * 0.5 * 255 + 0.5))))
            ap[x, y] = tuple(werte)
    return aus


def speichere(name, bild):
    pfad = os.path.join(ZIEL, name + ".png")
    bild.save(pfad, optimize=True)
    st = ImageStat.Stat(bild)
    print("%s %s mittel=%s streuung=%s" % (name, bild.size, [round(v) for v in st.mean], [round(v) for v in st.stddev]))


def fassade():
    spec = importlib.util.spec_from_file_location("bt", os.path.join(HIER, "baue_texturen.py"))
    bt = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(bt)
    raster = bt.fassade()                     # Zeilen von (r, g, b), 512 x 512
    n = len(raster)
    putz = modulator(lade("PaintedPlaster017", "Color"), 0.0, staerke=2.2).resize((n, n), Image.LANCZOS).load()
    aus = Image.new("RGB", (n, n))
    ap = aus.load()
    for y in range(n):
        for x in range(n):
            v = raster[y][x][0]
            # Nur der Putz bekommt das Korn: Glas (dunkel) und Fensterstock
            # (hell) bleiben, wie das Raster sie zeichnet.
            if 70 <= v <= 150:
                m = putz[x, y][0] / 127.5
                v = max(0, min(255, int(v * m + 0.5)))
            ap[x, y] = (v, v, v)
    return aus


# Farbanteile und Helligkeit im Spiel abgestimmt (2026-09-21): mit halbem
# Fotofarbton wurden die Daecher orange und die Wiese giftgruen, weil die
# Scheitelfarbe den Ton schon mitbringt.
if __name__ == "__main__":
    if not QUELLE or not os.path.isdir(QUELLE):
        sys.exit("Ordner mit den ambientCG-Zips angeben (Argument oder LALABERG_AMBIENTCG)")
    speichere("T_Ziegel_D", modulator(lade("RoofingTiles011A", "Color"), 0.25, helligkeit=0.80, staerke=1.2,
                                      ao=lade("RoofingTiles011A", "AmbientOcclusion")))
    speichere("T_Pflaster_D", modulator(lade("PavingStones051", "Color"), 0.35, helligkeit=0.82, staerke=1.3,
                                        ao=lade("PavingStones051", "AmbientOcclusion")))
    speichere("T_Wiese_D", modulator(lade("Grass004", "Color"), 0.30, helligkeit=0.85, staerke=1.1,
                                     ao=lade("Grass004", "AmbientOcclusion")))
    speichere("T_Fassade_D", fassade())
