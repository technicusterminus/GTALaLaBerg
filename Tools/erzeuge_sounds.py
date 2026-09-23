# Erzeugt alle Spiel-Klaenge rein rechnerisch (kein Download, keine fremde
# Quelle) als 16-Bit-Mono-WAV. Reines Python (wave/struct/math), kein numpy.
#
#   python Tools/erzeuge_sounds.py
#
# Seit 2026-09-23 nicht mehr "Sinus plus Rauschen", sondern nach dem, was
# den Klang in Wirklichkeit ausmacht:
#
#   Motor   - einzelne Zuendungen, durch die Resonanzen der Auspuffanlage
#             geschickt, dazu Ansauggeraeusch und mechanisches Klappern.
#   Schuss  - Paintball-Markierer: Luftstoss, dessen Klangfarbe mit dem
#             Druck abfaellt, der Ton des Laufs und die Klicks des Bolzens.
#   Schritt - Absatz und Abrollen, getrennt nach hartem Belag und Wiese.
#   Sirene  - Martinshorn nach DIN 14610 (a' und d''), Druckkammertoene.
#   Rotor   - Blattschlag plus Turbine, Panzer - Diesel plus Kettenklappern.
#
# Alles deterministisch: dieselben Startwerte, dieselbe Datei.
import math
import os
import random
import struct
import wave

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
    spitze = max((abs(s) for s in samples), default=0.0)
    print("LALABERG_SOUND %s %.2fs spitze=%.2f" % (name, len(samples) / SR, spitze))


# --------------------------------------------------------------- Werkzeug
def rauschen(n, seed):
    r = random.Random(seed)
    return [r.uniform(-1, 1) for _ in range(n)]


def rauschen_takt(laenge, n, seed):
    """Rauschen, das sich alle n Abtastwerte wiederholt. Fuer Schleifen
    unverzichtbar: sonst ist der Klang zwar eingeschwungen, das Rauschen am
    Ende aber ein anderes als am Anfang, und es knackt bei jeder
    Wiederholung."""
    grund = rauschen(n, seed)
    return [grund[i % n] for i in range(laenge)]


def bandpass(x, f0, guete, verstaerkung=1.0):
    """Zweipoliger Bandpass (RBJ-Kochbuch). Damit bekommt ein Rauschen oder
    ein Druckstoss eine Tonhoehe - genau das macht aus einem Knall den Klang
    eines bestimmten Rohres."""
    w0 = 2 * math.pi * f0 / SR
    alpha = math.sin(w0) / (2 * guete)
    b0, b1, b2 = alpha, 0.0, -alpha
    a0, a1, a2 = 1 + alpha, -2 * math.cos(w0), 1 - alpha
    b0, b1, b2, a1, a2 = b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0
    aus = [0.0] * len(x)
    x1 = x2 = y1 = y2 = 0.0
    for i, xi in enumerate(x):
        y = b0 * xi + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
        x2, x1 = x1, xi
        y2, y1 = y1, y
        aus[i] = y * verstaerkung
    return aus


def tiefpass(x, f):
    a = math.exp(-2 * math.pi * f / SR)
    aus = [0.0] * len(x)
    y = 0.0
    for i, xi in enumerate(x):
        y = y * a + xi * (1 - a)
        aus[i] = y
    return aus


def hochpass(x, f):
    tief = tiefpass(x, f)
    return [xi - ti for xi, ti in zip(x, tief)]


def mische(*spuren):
    n = max(len(s) for s in spuren)
    aus = [0.0] * n
    for s in spuren:
        for i, v in enumerate(s):
            aus[i] += v
    return aus


def weich_begrenzen(x, kraft=1.0):
    """Sanfte Saettigung statt harten Abschneidens - ein Motor klingt
    gepresst, nicht zerhackt."""
    return [math.tanh(v * kraft) for v in x]


def normiere(x, spitze=0.88):
    hoch = max((abs(v) for v in x), default=0.0)
    if hoch < 1e-9:
        return x
    f = spitze / hoch
    return [v * f for v in x]


def nachhall(x, staerke=0.3, daempfung=2600.0):
    """Ein paar Rueckwuerfe von den Haeusern ringsum: vier Verzoegerungen mit
    Tiefpass. Kein Hallraum, nur das, was einen Schuss im Freien von einem
    Schuss im Kopfhoerer unterscheidet."""
    aus = list(x)
    for verzug_s, pegel in ((0.031, 0.55), (0.057, 0.40), (0.089, 0.28), (0.137, 0.18)):
        verzug = int(SR * verzug_s)
        kopie = tiefpass(x, daempfung)
        aus += [0.0] * max(0, verzug + len(x) - len(aus))
        for i, v in enumerate(kopie):
            aus[i + verzug] += v * pegel * staerke
    return aus


def huellkurve(n, anstieg_s, abfall):
    """Schneller Anstieg, danach exponentieller Abfall - der Grundbaustein
    jedes kurzen Ereignisses."""
    anstieg = max(1, int(SR * anstieg_s))
    return [(i / anstieg) if i < anstieg else math.exp(-(i - anstieg) / SR * abfall)
            for i in range(n)]


def schleifenfest(aus, uebergang_s=0.05):
    """Nahtlose Schleife: die letzten Millisekunden mit den ersten
    verschmelzen, sonst knackt es bei jeder Wiederholung."""
    n = len(aus)
    uebergang = int(SR * uebergang_s)
    for i in range(uebergang):
        f = i / uebergang
        mix = aus[i] * f + aus[n - uebergang + i] * (1 - f)
        aus[i] = mix
        aus[n - uebergang + i] = mix
    return aus


def eingeschwungen(bauen, n):
    """Filter brauchen Anlauf. Wer eine Schleife will, rechnet drei Runden
    und nimmt die mittlere - dann ist der Filterzustand am Anfang derselbe
    wie am Ende, und die Schleife schliesst sich wirklich."""
    lang = bauen(3 * n)
    return lang[n:2 * n]


# ------------------------------------------------------------------ Motor
# Ein Viertakter zuendet je Umdrehung halb so oft, wie er Zylinder hat: ein
# Vierzylinder bei 900 U/min also 30-mal je Sekunde. Jede Zuendung ist ein
# Druckstoss, der die Auspuffanlage anregt - deren Resonanzen (rund 80, 165
# und 330 Hz) machen den Klang, nicht der Grundton selbst.
def motor(drehzahl=900.0, zylinder=4, dauer=1.0):
    zuendrate = drehzahl / 60.0 * zylinder / 2.0
    zuendungen = max(1, int(round(dauer * zuendrate)))
    n = int(round(SR * zuendungen / zuendrate))

    def baue(laenge):
        anregung = [0.0] * laenge
        # Feste Streuung je Zuendung, in jeder Runde dieselbe - sonst
        # wiederholt sich die Schleife nicht wirklich.
        r = random.Random(4)
        streuung = [r.uniform(-0.015, 0.015) for _ in range(zuendungen)]
        runden = laenge // n + 1
        for k in range(zuendungen * runden):
            # Leichte Ungleichfoerderung: kein Motor zuendet im Lineal-Takt.
            t = (k + streuung[k % zuendungen]) / zuendrate
            i0 = int(t * SR)
            if i0 >= laenge:
                break
            # Je Zylinder etwas anders - das macht den typischen Lauf.
            staerke = 1.0 + (0.14 if k % zylinder in (0, 3) else -0.09)   # Zylinderfolge
            stoss = int(SR * 0.005)
            for i in range(stoss):
                if i0 + i >= laenge:
                    break
                anregung[i0 + i] += staerke * math.exp(-i / SR * 700.0) * (3.0 if i == 0 else 1.0)
        auspuff = mische(bandpass(anregung, 78.0, 7.0, 1.0),
                         bandpass(anregung, 165.0, 5.0, 0.55),
                         bandpass(anregung, 330.0, 4.0, 0.22))
        # Ansaugen: breitbandiges Rauschen, im Takt der Zuendungen geoeffnet.
        roh = rauschen_takt(laenge, n, 9)
        ansaug = bandpass(tiefpass(roh, 2600.0), 520.0, 1.2, 1.0)
        klappern = bandpass(roh, 1900.0, 3.0, 1.0)
        takt = [0.5 + 0.5 * math.sin(2 * math.pi * zuendrate * i / SR) for i in range(laenge)]
        gemischt = [auspuff[i] * 0.9 + ansaug[i] * 0.22 * takt[i] + klappern[i] * 0.06
                    for i in range(laenge)]
        return weich_begrenzen(gemischt, 1.6)

    return schleifenfest(normiere(eingeschwungen(baue, n), 0.72))


# ------------------------------------------------------------------ Schuss
# Wichtig und lange uebersehen: die Waffen im Spiel sind Paintball-Waffen
# (siehe LaLaBergWaffe, der Laden heisst Paintball-Laden). Ein Markierer
# knallt nicht wie ein Gewehr - er stoesst Druckluft aus. Der erste Versuch
# baute Muendungsknall, Koerperschlag und Haeuserhall nach und klang
# entsprechend nach Rauschen mit Blecheimer.
#
# Was man bei einem Markierer tatsaechlich hoert:
#   1. den Luftstoss - Rauschen, dessen Klangfarbe binnen 25 ms von hell
#      nach dunkel faellt, weil der Druck abfaellt,
#   2. den Ton des Laufs - eine kurze, stark gedaempfte Resonanz,
#   3. die Mechanik - Bolzen vor und zurueck, ein bis zwei metallische
#      Klicks wenige Millisekunden spaeter,
#   4. kaum Nachhall - im Freien ist nach 150 ms nichts mehr da.
def faltung(x, antwort):
    """Kurze Faltung - nur fuer wenige Millisekunden Anregung gedacht, sonst
    rechnet reines Python zu lange."""
    n = len(x) + len(antwort)
    aus = [0.0] * n
    for i, xi in enumerate(x):
        if abs(xi) < 1e-5:
            continue
        for j, hj in enumerate(antwort):
            aus[i + j] += xi * hj
    return aus


def raumantwort(dauer, daempfung, seed):
    """Dichte, zufaellige Rueckwuerfe statt vier einzelner Echos: vier Taps
    klangen wie ein Blecheimer (Kammfilter), hier ist es einfach Raum."""
    n = int(SR * dauer)
    roh = rauschen(n, seed)
    weich = tiefpass(roh, daempfung)
    return [weich[i] * math.exp(-i / SR * (6.0 / dauer)) * (i / (SR * 0.004) if i < SR * 0.004 else 1.0)
            for i in range(n)]


def markierer(luftdauer, lauf_hz, helligkeit, mechanik, hall=0.10, nachbolzen=None):
    n = int(SR * luftdauer)
    roh = rauschen(n, 1)
    # 1. Luftstoss: ein Tiefpass, dessen Grenze mitlaeuft - von hell nach
    #    dunkel, weil der Druck im Lauf abfaellt.
    luft = [0.0] * n
    y = 0.0
    for i in range(n):
        t = i / SR
        grenze = (5200.0 * helligkeit) * math.exp(-t * 55.0) + 300.0
        a = math.exp(-2 * math.pi * grenze / SR)
        y = y * a + roh[i] * (1 - a)
        huelle = min(1.0, t / 0.0004) * math.exp(-t * 46.0)
        luft[i] = y * huelle
    # 2. Lauf: kurze, stark gedaempfte Resonanz auf demselben Stoss.
    lauf = bandpass([luft[i] * math.exp(-i / SR * 120.0) for i in range(n)], lauf_hz, 6.0, 1.6)
    # 3. Mechanik: metallische Klicks, ein paar Millisekunden versetzt.
    aus = [luft[i] * 1.0 + lauf[i] * 0.8 for i in range(n)]
    klicks = [(0.006, 1.0)] + ([(nachbolzen, 0.7)] if nachbolzen else [])
    for versatz, pegel in klicks:
        i0 = int(SR * versatz)
        kurz = int(SR * 0.012)
        anregung = [rauschen(kurz, 31)[i] * math.exp(-i / SR * 1400.0) for i in range(kurz)]
        metall = mische(bandpass(anregung, 2600.0, 18.0, 1.0), bandpass(anregung, 4300.0, 22.0, 0.6))
        for i, v in enumerate(metall):
            if i0 + i < n:
                aus[i0 + i] += v * mechanik * pegel
    # 4. Der Raum, sparsam: nur die ersten Millisekunden werden gefaltet.
    if hall > 0.0:
        anregung = aus[:int(SR * 0.006)]
        schwanz = faltung(anregung, raumantwort(0.16, 2400.0, 3))
        aus = mische(aus, [v * hall for v in schwanz])
    return normiere(aus, 0.9)


def werfer():
    """Paintball-Werfer: keine Druckluft aus duennem Lauf, sondern ein
    dumpfer Ausstoss aus weitem Rohr - tiefer, laenger, ohne scharfen
    Klick."""
    n = int(SR * 0.35)
    roh = rauschen(n, 7)
    aus = [0.0] * n
    y = 0.0
    for i in range(n):
        t = i / SR
        grenze = 1400.0 * math.exp(-t * 30.0) + 160.0
        a = math.exp(-2 * math.pi * grenze / SR)
        y = y * a + roh[i] * (1 - a)
        aus[i] = y * min(1.0, t / 0.0015) * math.exp(-t * 16.0)
    rohrton = bandpass(aus, 190.0, 4.0, 1.4)
    gemischt = [aus[i] * 0.8 + rohrton[i] for i in range(n)]
    schwanz = faltung(gemischt[:int(SR * 0.008)], raumantwort(0.22, 1800.0, 5))
    return normiere(mische(gemischt, [v * 0.16 for v in schwanz]), 0.92)


# ----------------------------------------------------------------- Klecks
def klecks():
    """Farbkugel platzt: nasser Schlag, danach ein paar Spritzer."""
    n = int(SR * 0.26)
    roh = rauschen(n, 2)
    nass = tiefpass(roh, 900.0)
    huelle = huellkurve(n, 0.0008, 26.0)
    ton = [math.sin(2 * math.pi * 165.0 * i / SR) * math.exp(-i / SR * 24.0) for i in range(n)]
    aus = [nass[i] * huelle[i] * 0.9 + ton[i] * 0.5 for i in range(n)]
    r = random.Random(12)
    for _ in range(5):                                  # Spritzer
        i0 = int(SR * r.uniform(0.03, 0.16))
        for i in range(int(SR * 0.012)):
            if i0 + i >= n:
                break
            aus[i0 + i] += roh[(i0 + i) % n] * math.exp(-i / SR * 600.0) * 0.22
    return normiere(aus, 0.85)


# ---------------------------------------------------------------- Schritt
# Ein Schritt ist zweiteilig: der Absatz setzt auf (kurzer Schlag mit der
# Resonanz des Belags), dann rollt der Fuss ab (kurzes Schaben). Auf hartem
# Belag klickt es hell, auf der Wiese knistert es dumpf.
def schritt(hart=True):
    n = int(SR * (0.13 if hart else 0.19))
    roh = rauschen(n, 3 if hart else 13)
    if hart:
        schlag = bandpass(roh, 1500.0, 2.4, 1.0)
        tief = [math.sin(2 * math.pi * 135.0 * i / SR) * math.exp(-i / SR * 70.0) for i in range(n)]
        huelle = huellkurve(n, 0.0006, 95.0)
        abrollen = bandpass(roh, 3400.0, 1.2, 1.0)
        aus = [schlag[i] * huelle[i] + tief[i] * 0.35 * huelle[i]
               + abrollen[i] * 0.18 * math.exp(-max(0, i - int(SR * 0.02)) / SR * 45.0)
               for i in range(n)]
    else:
        # Wiese: kein Klick, sondern viele kleine Halme - gefiltertes
        # Rauschen mit unregelmaessigen Spitzen.
        weich = bandpass(tiefpass(roh, 4200.0), 700.0, 0.9, 1.0)
        huelle = huellkurve(n, 0.004, 32.0)
        aus = [weich[i] * huelle[i] for i in range(n)]
        r = random.Random(21)
        for _ in range(9):
            i0 = int(SR * r.uniform(0.0, 0.10))
            for i in range(int(SR * 0.004)):
                if i0 + i >= n:
                    break
                aus[i0 + i] += roh[(i0 + i) % n] * math.exp(-i / SR * 1200.0) * 0.30
    return normiere(aus, 0.7)


# --------------------------------------------------------------- Sirene
# DIN 14610 nennt fuer das Einsatzhorn a' und d'' - eine reine Quart, die
# man aus jeder deutschen Stadt kennt. Ein reiner Sinus klaenge nach
# Pruefton: ein Druckkammerlautsprecher bringt die Obertoene kraeftig mit.
def sirene(tief=435.0, hoch=580.0, tonlaenge=0.7):
    n_ton = int(SR * tonlaenge)
    aus = []
    for ton in (tief, hoch):
        for i in range(n_ton):
            t = i / SR
            w = 0.0
            for k, staerke in enumerate((1.0, 0.55, 0.32, 0.18, 0.10, 0.06), start=1):
                w += math.sin(2 * math.pi * ton * k * t) * staerke
            kante = 0.012
            h = min(1.0, t / kante, (tonlaenge - t) / kante)
            aus.append(w * max(0.0, h))
    # Auf dieselbe Aussteuerung wie die uebrigen Klaenge: vorher war das Horn
    # mit 0,24 Spitze deutlich leiser als alles andere.
    return normiere(aus, 0.85)


# ----------------------------------------------------------------- Rotor
# Vier Blaetter bei rund sechs Umdrehungen je Sekunde ergeben 24 Schlaege -
# das tiefe Wummern. Darueber die Turbine als heller, schwebender Ton.
def rotor(dauer=1.0, schlaege=24.0):
    n = int(SR * dauer)

    def baue(laenge):
        roh = rauschen_takt(laenge, n, 7)
        tief = tiefpass(roh, 420.0)
        aus = []
        for i in range(laenge):
            t = i / SR
            phase = (t * schlaege) % 1.0
            schlag = math.exp(-phase * 16.0)
            turbine = (math.sin(2 * math.pi * 840.0 * t) * 0.10
                       + math.sin(2 * math.pi * 1290.0 * t) * 0.06)
            aus.append(tief[i] * schlag * 1.8 + turbine * (0.5 + 0.5 * schlag))
        return aus

    return schleifenfest(normiere(eingeschwungen(baue, n), 0.8))


# ---------------------------------------------------------------- Panzer
# Ein Zwoelfzylinder-Diesel im Standgas, dazu das Klappern der Kette: jedes
# Glied schlaegt beim Abrollen auf das naechste.
def panzer(dauer=1.2, drehzahl=800.0, zylinder=12):
    zuendrate = drehzahl / 60.0 * zylinder / 2.0
    zuendungen = max(1, int(round(dauer * zuendrate)))
    n = int(round(SR * zuendungen / zuendrate))

    def baue(laenge):
        anregung = [0.0] * laenge
        # Dieselben Abweichungen in jeder Runde, sonst ist die Schleife nicht
        # periodisch (siehe rauschen_takt).
        r = random.Random(11)
        streuung = [r.uniform(-0.02, 0.02) for _ in range(zuendungen)]
        for k in range(int(laenge / SR * zuendrate) + 1):
            i0 = int((k + streuung[k % zuendungen]) / zuendrate * SR)
            if i0 >= laenge or i0 < 0:
                continue
            for i in range(int(SR * 0.006)):
                if i0 + i >= laenge:
                    break
                anregung[i0 + i] += math.exp(-i / SR * 500.0) * (3.0 if i == 0 else 1.0)
        diesel = mische(bandpass(anregung, 42.0, 6.0, 1.0),
                        bandpass(anregung, 96.0, 4.0, 0.5),
                        bandpass(anregung, 210.0, 3.0, 0.2))
        roh = rauschen_takt(laenge, n, 17)
        # Kettenglieder: trockenes Klacken, schneller als die Zuendung.
        klack = bandpass(roh, 2400.0, 5.0, 1.0)
        aus = []
        for i in range(laenge):
            t = i / SR
            kettenphase = (t * 13.0) % 1.0
            aus.append(diesel[i] * 0.95 + klack[i] * math.exp(-kettenphase * 30.0) * 0.5)
        return weich_begrenzen(aus, 1.4)

    return schleifenfest(normiere(eingeschwungen(baue, n), 0.82))


if __name__ == "__main__":
    # Pistole: kurzer, heller Stoss. MP: schneller Bolzen, zweiter Klick
    # beim Zurueckfahren. Schrotflinte: weiteres Rohr, tiefer und voller.
    schreibe("SFX_Schuss_Pistole", markierer(0.20, 900.0, 1.00, 0.45))
    schreibe("SFX_Schuss_Maschine", markierer(0.15, 1150.0, 1.15, 0.60, nachbolzen=0.028))
    schreibe("SFX_Schuss_Schrot", markierer(0.30, 520.0, 0.80, 0.35, hall=0.16))
    schreibe("SFX_Schuss_Rakete", werfer())
    schreibe("SFX_Klecks", klecks())
    schreibe("SFX_Schritt", schritt(hart=True))
    schreibe("SFX_Schritt_Gras", schritt(hart=False))
    schreibe("SFX_Motor", motor())
    schreibe("SFX_Sirene", sirene())
    schreibe("SFX_Rotor", rotor())
    schreibe("SFX_Panzer", panzer())
