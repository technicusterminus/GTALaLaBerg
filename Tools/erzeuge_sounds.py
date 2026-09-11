# Erzeugt alle Spiel-Sounds rein rechnerisch (kein Download, keine fremde
# Quelle) als 16-Bit-Mono-WAV: Schuss, Einschlag, Schritt, Motor. Reines
# Python (wave/struct/math), kein numpy noetig.
#
#   python Tools/erzeuge_sounds.py
import math
import random
import struct
import wave
import os

SR = 44100
ZIEL = os.path.join(os.path.dirname(__file__), "Sounds")
os.makedirs(ZIEL, exist_ok=True)


def schreibe(name, samples):
    pfad = os.path.join(ZIEL, name + ".wav")
    with wave.open(pfad, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        daten = b"".join(struct.pack("<h", max(-32767, min(32767, int(s * 32767)))) for s in samples)
        w.writeframes(daten)
    print("LALABERG_SOUND " + pfad)


def huelle_attack_decay(i, n, attack, decay_start):
    # Schnelles Einschwingen, danach exponentielles Abklingen - der Grundbaustein
    # fast jedes kurzen Effektsounds (Schuss, Klecks, Schritt).
    t = i / SR
    if t < attack:
        return t / attack
    return math.exp(-(t - attack) * decay_start)


def rauschen(n, seed):
    r = random.Random(seed)
    return [r.uniform(-1, 1) for _ in range(n)]


# --- Schuss: kurzer Knall aus gefiltertem Rauschen + tiefem Tonanteil ---------
def schuss(dauer, tonfrequenz, rauschanteil, decay):
    n = int(SR * dauer)
    roh = rauschen(n, 1)
    tief = 0.0
    gefiltert = []
    for i in range(n):
        # Einfaches Tiefpass-Rauschen (gleitender Mittelwert) statt weissem
        # Rauschen - klingt weniger wie Static, mehr wie ein Druckstoss.
        tief = tief * 0.72 + roh[i] * 0.28
        gefiltert.append(tief)
    aus = []
    for i in range(n):
        h = huelle_attack_decay(i, n, 0.002, decay)
        ton = math.sin(2 * math.pi * tonfrequenz * i / SR) * math.exp(-i / SR * decay * 1.3)
        aus.append(h * (gefiltert[i] * rauschanteil + ton * (1 - rauschanteil)) * 0.9)
    return aus


# --- Klecks: weicher, nasser "Thock" - tiefer Ton mit schnellem Abklingen ----
def klecks():
    n = int(SR * 0.18)
    roh = rauschen(n, 2)
    tief = 0.0
    aus = []
    for i in range(n):
        tief = tief * 0.85 + roh[i] * 0.15
        h = huelle_attack_decay(i, n, 0.001, 26)
        ton = math.sin(2 * math.pi * 180 * i / SR) * math.exp(-i / SR * 22)
        aus.append(h * (tief * 0.55 + ton * 0.65))
    return aus


# --- Schritt: kurzer, dumpfer Tap ---------------------------------------------
def schritt():
    n = int(SR * 0.09)
    roh = rauschen(n, 3)
    tief = 0.0
    aus = []
    for i in range(n):
        tief = tief * 0.6 + roh[i] * 0.4
        h = huelle_attack_decay(i, n, 0.001, 55)
        aus.append(h * tief * 0.5)
    return aus


# --- Motor: schleifenfaehiges tiefes Brummen ----------------------------------
def motor(dauer=1.0, grundton=55.0):
    n = int(SR * dauer)
    aus = []
    for i in range(n):
        t = i / SR
        # Ein paar Obertoene plus leichtes Rauschen - ein reiner Sinus klingt
        # zu sauber fuer einen Verbrennungsmotor.
        w = (math.sin(2 * math.pi * grundton * t) * 0.5
             + math.sin(2 * math.pi * grundton * 2 * t) * 0.25
             + math.sin(2 * math.pi * grundton * 3.01 * t) * 0.12
             + (random.Random(int(t * SR) % 997).uniform(-1, 1)) * 0.08)
        # Nahtlose Schleife: die Amplitude an Anfang/Ende gleich, ueber eine
        # Cosinus-Rampe an beiden Enden ausgeblendet und wieder addiert
        # (Crossfade der Schleifengrenze) - sonst knackt es beim Wiederholen.
        aus.append(w * 0.5)
    uebergang = int(SR * 0.05)
    for i in range(uebergang):
        f = i / uebergang
        mix = aus[i] * f + aus[n - uebergang + i] * (1 - f)
        aus[i] = mix
        aus[n - uebergang + i] = mix
    return aus


schreibe("SFX_Schuss_Pistole", schuss(0.10, 620, 0.55, 34))
schreibe("SFX_Schuss_Maschine", schuss(0.07, 720, 0.6, 42))
schreibe("SFX_Schuss_Schrot", schuss(0.16, 380, 0.7, 20))
schreibe("SFX_Schuss_Rakete", schuss(0.32, 140, 0.5, 9))
schreibe("SFX_Klecks", klecks())
schreibe("SFX_Schritt", schritt())
schreibe("SFX_Motor", motor())
