# Das Autoradio: drei Sender, rein gerechnet wie alle Klaenge im Projekt
# (kein Download, keine fremde Aufnahme, keine Lizenzfrage).
#
#   python Tools/erzeuge_musik.py
#
#   SFX_Radio_Lech      Synthesizer, treibend - der Sender fuer die Fahrt
#   SFX_Radio_Blasmusik Blaskapelle mit Tuba und Trompete - Oberbayern
#   SFX_Radio_Klassik   Streicher ueber einer Kadenz - ruhig
#
# Aufgebaut wie ein kleines Studio: Stimmen (Saegezahn, Rechteck, Blech,
# Streicher), Schlagwerk (Bass-, Schnarr- und Beckentrommel) und ein
# Sequenzer, der Noten in einen Mischpuffer legt. Jedes Stueck ist eine
# ganze Zahl von Takten lang und schliesst sich nahtlos.
import math
import os
import random
import struct
import wave

SR = 44100
ZIEL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Sounds")
os.makedirs(ZIEL, exist_ok=True)

# Halbtonabstand zum Kammerton a' = 440 Hz.
NOTEN = {"c": -9, "cis": -8, "d": -7, "dis": -6, "e": -5, "f": -4, "fis": -3,
         "g": -2, "gis": -1, "a": 0, "b": 1, "h": 2}


def hz(name, oktave=0):
    return 440.0 * (2.0 ** ((NOTEN[name] + 12 * oktave) / 12.0))


def schreibe(name, samples):
    pfad = os.path.join(ZIEL, name + ".wav")
    with wave.open(pfad, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", max(-32767, min(32767, int(s * 32767)))) for s in samples))
    print("LALABERG_MUSIK %s %.1fs" % (name, len(samples) / SR))


def mische(ziel, ab, stimme, pegel=1.0):
    i0 = int(ab * SR)
    n = len(ziel)
    for i, v in enumerate(stimme):
        if i0 + i >= n:
            # Was hinten ueberhaengt, faellt vorn wieder hinein: so bleibt
            # die Schleife auch bei langen Toenen nahtlos.
            ziel[(i0 + i) % n] += v * pegel
        else:
            ziel[i0 + i] += v * pegel


def huelle(n, anstieg, halten, abfall):
    """Anstieg, Halten, Abfall - in Sekunden, der Rest klingt aus."""
    a, h = int(SR * anstieg), int(SR * halten)
    aus = []
    for i in range(n):
        if i < a:
            aus.append(i / max(1, a))
        elif i < a + h:
            aus.append(1.0)
        else:
            aus.append(math.exp(-(i - a - h) / SR / max(0.01, abfall)))
    return aus


def stimme(frequenz, dauer, art, anstieg=0.01, halten=0.2, abfall=0.25, vibrato=0.0):
    """Eine Note. Die Klangfarbe steckt in der Obertonreihe: Saegezahn fuer
    Synthesizer, ungerade Obertoene fuer Blech, wenige weiche fuer Streicher."""
    n = int(SR * dauer)
    if art == "saege":
        teile = [(k, 1.0 / k) for k in range(1, 12)]
    elif art == "blech":
        teile = [(1, 1.0), (2, 0.7), (3, 0.55), (4, 0.32), (5, 0.24), (6, 0.14), (7, 0.09)]
    elif art == "tuba":
        teile = [(1, 1.0), (2, 0.42), (3, 0.16), (4, 0.06)]
    elif art == "streicher":
        teile = [(1, 1.0), (2, 0.35), (3, 0.22), (4, 0.12), (5, 0.07), (6, 0.04)]
    else:                                   # "rechteck"
        teile = [(k, 1.0 / k) for k in range(1, 14, 2)]
    h = huelle(n, anstieg, halten, abfall)
    aus = [0.0] * n
    for i in range(n):
        t = i / SR
        f = frequenz * (1.0 + vibrato * math.sin(2 * math.pi * 5.2 * t))
        w = 0.0
        for k, staerke in teile:
            w += math.sin(2 * math.pi * f * k * t) * staerke
        aus[i] = w * h[i] * 0.12
    return aus


def rauschen(n, seed):
    r = random.Random(seed)
    return [r.uniform(-1, 1) for _ in range(n)]


def bass(dauer=0.32):
    """Basstrommel: Tonhoehe faellt von 130 auf 45 Hz."""
    n = int(SR * dauer)
    aus = []
    phase = 0.0
    for i in range(n):
        t = i / SR
        f = 45.0 + 85.0 * math.exp(-t * 32.0)
        phase += 2 * math.pi * f / SR
        aus.append(math.sin(phase) * math.exp(-t * 9.0) * 0.9)
    return aus


def schnarr(dauer=0.22, seed=5):
    n = int(SR * dauer)
    roh = rauschen(n, seed)
    tief = 0.0
    aus = []
    for i in range(n):
        t = i / SR
        tief = tief * 0.55 + roh[i] * 0.45
        ton = math.sin(2 * math.pi * 185.0 * t) * 0.5
        aus.append((tief * 0.8 + ton) * math.exp(-t * 26.0) * 0.55)
    return aus


def becken(dauer=0.10, seed=9):
    n = int(SR * dauer)
    roh = rauschen(n, seed)
    hoch = 0.0
    aus = []
    for i in range(n):
        hoch = roh[i] - (hoch * 0.2)
        aus.append(hoch * math.exp(-i / SR * 70.0) * 0.28)
    return aus


def normiere(x, spitze=0.86):
    hoch = max((abs(v) for v in x), default=0.0)
    return x if hoch < 1e-9 else [v * (spitze / hoch) for v in x]


def weich(x, kraft=1.1):
    return [math.tanh(v * kraft) for v in x]


# --------------------------------------------------------------- Lech FM
# Vier Akkorde, treibender Bass, Arpeggio darueber, gerader Schlag.
def lech_fm(takte=16, bpm=124):
    schlag = 60.0 / bpm
    dauer = takte * 4 * schlag
    n = int(SR * dauer)
    spur = [0.0] * n
    akkorde = [("a", -1, ["a", "c", "e"]), ("f", -1, ["f", "a", "c"]),
               ("c", 0, ["c", "e", "g"]), ("g", -1, ["g", "h", "d"])]
    for takt in range(takte):
        grund, okt, toene = akkorde[takt % 4]
        t0 = takt * 4 * schlag
        # Bass auf jeder Zaehlzeit, Achtelversatz dazwischen.
        for i in range(8):
            mische(spur, t0 + i * schlag / 2,
                   stimme(hz(grund, okt - 1), schlag * 0.45, "saege", 0.005, 0.10, 0.12), 0.9)
        # Arpeggio in Sechzehnteln.
        for i in range(16):
            ton = toene[i % len(toene)]
            oktave = okt + 1 + (1 if i % 8 >= 4 else 0)
            mische(spur, t0 + i * schlag / 4,
                   stimme(hz(ton, oktave), schlag * 0.22, "rechteck", 0.002, 0.04, 0.08), 0.5)
        # Flaeche.
        for ton in toene:
            mische(spur, t0, stimme(hz(ton, okt), 4 * schlag, "streicher", 0.25, 2.2, 1.2, 0.004), 0.35)
        # Schlagwerk.
        for i in range(4):
            mische(spur, t0 + i * schlag, bass(), 0.9)
            mische(spur, t0 + i * schlag + schlag / 2, becken(), 0.6)
            if i % 2 == 1:
                mische(spur, t0 + i * schlag, schnarr(), 0.7)
    return normiere(weich(spur))


# ------------------------------------------------------------ Blasmusik
# Oberbayerisch: Tuba auf eins und drei, Nachschlag auf zwei und vier,
# Trompete singt die Melodie.
def blasmusik(takte=16, bpm=116):
    schlag = 60.0 / bpm
    n = int(SR * takte * 4 * schlag)
    spur = [0.0] * n
    stufen = [("f", ["f", "a", "c"]), ("f", ["f", "a", "c"]),
              ("c", ["c", "e", "g"]), ("f", ["f", "a", "c"]),
              ("b", ["b", "d", "f"]), ("f", ["f", "a", "c"]),
              ("c", ["c", "e", "g"]), ("f", ["f", "a", "c"])]
    melodie = ["f", "a", "c", "a", "f", "c", "a", "f",
               "g", "h", "d", "h", "g", "d", "h", "g"]
    for takt in range(takte):
        grund, toene = stufen[takt % len(stufen)]
        t0 = takt * 4 * schlag
        for i in (0, 2):                                    # Tuba
            mische(spur, t0 + i * schlag, stimme(hz(grund, -2), schlag * 0.8, "tuba", 0.02, 0.30, 0.20), 1.0)
        for i in (1, 3):                                    # Nachschlag
            for ton in toene:
                mische(spur, t0 + i * schlag, stimme(hz(ton, 0), schlag * 0.35, "blech", 0.01, 0.10, 0.10), 0.35)
        for i in range(2):                                  # Trompete
            ton = melodie[(takt * 2 + i) % len(melodie)]
            mische(spur, t0 + i * 2 * schlag,
                   stimme(hz(ton, 1), schlag * 1.7, "blech", 0.03, 0.9, 0.35, 0.006), 0.6)
        for i in range(4):
            mische(spur, t0 + i * schlag, bass(0.26), 0.55)
            if i % 2 == 1:
                mische(spur, t0 + i * schlag, schnarr(0.18, seed=11), 0.5)
    return normiere(weich(spur, 1.05))


# -------------------------------------------------------------- Klassik
# Eine ruhige Kadenz, Streicher, kein Schlagwerk - der Sender fuers
# Spazierenfahren.
def klassik(takte=12, bpm=72):
    schlag = 60.0 / bpm
    n = int(SR * takte * 4 * schlag)
    spur = [0.0] * n
    folge = [("d", ["d", "f", "a"]), ("a", ["a", "c", "e"]),
             ("b", ["b", "d", "f"]), ("f", ["f", "a", "c"]),
             ("g", ["g", "b", "d"]), ("d", ["d", "f", "a"]),
             ("g", ["g", "b", "d"]), ("a", ["a", "c", "e"])]
    for takt in range(takte):
        grund, toene = folge[takt % len(folge)]
        t0 = takt * 4 * schlag
        mische(spur, t0, stimme(hz(grund, -2), 4 * schlag, "streicher", 0.4, 2.4, 1.6, 0.003), 0.8)
        for ton in toene:
            mische(spur, t0, stimme(hz(ton, 0), 4 * schlag, "streicher", 0.6, 2.0, 1.8, 0.005), 0.45)
        # Melodie in ruhigen Vierteln, eine Oktave hoeher.
        for i in range(4):
            ton = toene[(takt + i) % len(toene)]
            mische(spur, t0 + i * schlag, stimme(hz(ton, 1), schlag * 1.4, "streicher", 0.12, 0.5, 0.5, 0.008), 0.38)
    return normiere(weich(spur, 0.95), 0.78)


if __name__ == "__main__":
    schreibe("SFX_Radio_Lech", lech_fm())
    schreibe("SFX_Radio_Blasmusik", blasmusik())
    schreibe("SFX_Radio_Klassik", klassik())
