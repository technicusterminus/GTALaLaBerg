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


# Nahtlose Schleife: die letzten Millisekunden mit den ersten verschmelzen,
# sonst knackt es bei jeder Wiederholung. Frueher stand das in motor(); jetzt
# brauchen es auch Rotor und Panzerkette.
def schleifenfest(aus, uebergang_s=0.05):
    n = len(aus)
    uebergang = int(SR * uebergang_s)
    for i in range(uebergang):
        f = i / uebergang
        mix = aus[i] * f + aus[n - uebergang + i] * (1 - f)
        aus[i] = mix
        aus[n - uebergang + i] = mix
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


# --- Martinshorn: zwei Toene im Quartabstand, wechselnd ----------------------
# DIN 14610 nennt fuer das Einsatzhorn a' und d'' - eine reine Quart, die man
# aus jeder deutschen Stadt kennt. Ein reiner Sinus klaenge nach Pruefton:
# ein Druckkammerlautsprecher bringt die Obertoene kraeftig mit, deshalb die
# absteigende Obertonreihe und das leichte Anblasen zu Beginn jedes Tons.
def sirene(tief=435.0, hoch=580.0, tonlaenge=0.7):
    n_ton = int(SR * tonlaenge)
    aus = []
    for ton in (tief, hoch):
        for i in range(n_ton):
            t = i / SR
            w = 0.0
            for k, staerke in enumerate((1.0, 0.55, 0.32, 0.18, 0.10, 0.06), start=1):
                w += math.sin(2 * math.pi * ton * k * t) * staerke
            # Anblasen und Abreissen: 12 ms, sonst knackt der Wechsel.
            kante = 0.012
            h = min(1.0, t / kante, (tonlaenge - t) / kante)
            aus.append(w * 0.16 * max(0.0, h))
    return aus


# --- Rotor: Hubschrauber, Blattschlag plus Turbine ---------------------------
# Vier Blaetter bei rund 6 Umdrehungen je Sekunde ergeben 24 Schlaege - das
# tiefe Wummern. Darueber die Turbine als heller, leicht schwebender Ton.
def rotor(dauer=1.0, schlaege=24.0):
    n = int(SR * dauer)
    roh = rauschen(n, 7)
    tief = 0.0
    aus = []
    for i in range(n):
        t = i / SR
        phase = (t * schlaege) % 1.0
        # Kurzer, harter Schlag je Blatt, dazwischen fast Stille.
        schlag = math.exp(-phase * 16.0)
        tief = tief * 0.80 + roh[i] * 0.20
        turbine = (math.sin(2 * math.pi * 840 * t) * 0.10
                   + math.sin(2 * math.pi * 1290 * t) * 0.06)
        aus.append((tief * schlag * 1.6 + turbine * (0.5 + 0.5 * schlag)) * 0.6)
    return schleifenfest(aus)


# --- Panzer: Diesel im Standgas, dazu das Klappern der Kette -----------------
def panzer(dauer=1.2, zuendungen=9.0):
    n = int(SR * dauer)
    roh = rauschen(n, 11)
    tief = 0.0
    aus = []
    for i in range(n):
        t = i / SR
        zuendung = math.exp(-((t * zuendungen) % 1.0) * 9.0)
        tief = tief * 0.88 + roh[i] * 0.12
        brummen = (math.sin(2 * math.pi * 38 * t) * 0.5 + math.sin(2 * math.pi * 76 * t) * 0.22)
        # Kettenglieder: ein trockenes Klacken, schneller als die Zuendung.
        klack = math.exp(-((t * 14.0) % 1.0) * 40.0) * roh[(i * 3) % n]
        aus.append((brummen * zuendung + tief * 0.5 + klack * 0.25) * 0.55)
    return schleifenfest(aus)


schreibe("SFX_Schuss_Pistole", schuss(0.10, 620, 0.55, 34))
schreibe("SFX_Schuss_Maschine", schuss(0.07, 720, 0.6, 42))
schreibe("SFX_Schuss_Schrot", schuss(0.16, 380, 0.7, 20))
schreibe("SFX_Schuss_Rakete", schuss(0.32, 140, 0.5, 9))
schreibe("SFX_Klecks", klecks())
schreibe("SFX_Schritt", schritt())
schreibe("SFX_Motor", motor())
schreibe("SFX_Sirene", sirene())
schreibe("SFX_Rotor", rotor())
schreibe("SFX_Panzer", panzer())
