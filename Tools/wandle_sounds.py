# Wandelt heruntergeladene MP3-Klaenge in die Form, die das Spiel braucht:
# 44,1 kHz, Mono, 16 Bit WAV, vorn beschnitten und ausgesteuert.
#
#   "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b -P Tools/wandle_sounds.py -- <Quellordner>
#
# Warum Blender: das Projekt hat weder ffmpeg noch numpy, Blender bringt aber
# mit dem Modul "aud" einen vollstaendigen Audiodekoder mit. Die Zuordnung
# Datei -> Spielklang steht in ZUORDNUNG; die Herkunft der Dateien in
# Content/SourceData/Audio/LIZENZ.md.
import os
import struct
import sys
import wave

import aud

ZIEL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Sounds")
ARGV = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
QUELLE = ARGV[0] if ARGV else ""

# Quelldatei -> Name im Spiel. Die Waffen sind Paintball-Markierer, klingen
# aber auf Wunsch wie echte Waffen; der Panzerschuss gehoert zur Kanone des
# Leopard (siehe LaLaBergSonderfahrzeug).
ZUORDNUNG = {
    "pistole.mp3": ("SFX_Schuss_Pistole", 1.20),
    "akm.mp3": ("SFX_Schuss_Maschine", 0.70),
    "schrot.mp3": ("SFX_Schuss_Schrot", 1.60),
    "rpg.mp3": ("SFX_Schuss_Rakete", 2.40),
    "panzer.mp3": ("SFX_Panzer_Schuss", 3.00),
}


def lies_wav(pfad):
    with wave.open(pfad) as w:
        n, breite, kanaele = w.getnframes(), w.getsampwidth(), w.getnchannels()
        roh = w.readframes(n)
        rate = w.getframerate()
    werte = struct.unpack("<%dh" % (len(roh) // 2), roh)
    if kanaele > 1:
        werte = [sum(werte[i:i + kanaele]) / kanaele for i in range(0, len(werte), kanaele)]
    return [v / 32768.0 for v in werte], rate


def schreib_wav(pfad, werte, rate=44100):
    with wave.open(pfad, "w") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(rate)
        w.writeframes(b"".join(struct.pack("<h", max(-32767, min(32767, int(v * 32767)))) for v in werte))


def zuschneiden(werte, rate, hoechstdauer):
    """Vorlauf weg (viele Aufnahmen beginnen mit Stille), hinten auf die
    gewuenschte Laenge kuerzen und die letzten 30 ms ausblenden."""
    spitze = max((abs(v) for v in werte), default=0.0)
    if spitze < 1e-6:
        return werte
    schwelle = spitze * 0.02
    start = next((i for i, v in enumerate(werte) if abs(v) > schwelle), 0)
    start = max(0, start - int(rate * 0.005))
    ende = min(len(werte), start + int(rate * hoechstdauer))
    aus = list(werte[start:ende])
    blende = int(rate * 0.03)
    for i in range(min(blende, len(aus))):
        aus[len(aus) - 1 - i] *= i / blende
    # Aussteuern wie die gerechneten Klaenge.
    hoch = max(abs(v) for v in aus)
    return [v * (0.92 / hoch) for v in aus]


if __name__ == "__main__":
    if not QUELLE or not os.path.isdir(QUELLE):
        raise SystemExit("Quellordner angeben: blender -b -P Tools/wandle_sounds.py -- <Ordner>")
    os.makedirs(ZIEL, exist_ok=True)
    zwischen = os.path.join(QUELLE, "_roh.wav")
    for datei, (name, dauer) in ZUORDNUNG.items():
        pfad = os.path.join(QUELLE, datei)
        if not os.path.isfile(pfad):
            print("LALABERG_SOUND fehlt: " + pfad)
            continue
        # Erst den Schreiber umrechnen lassen; klappt das nicht (bei 48-kHz-
        # Quellen brach ffmpeg mit "filling the audio frame failed" ab), im
        # Originaltakt schreiben und selbst umrechnen.
        try:
            aud.Sound(pfad).write(zwischen, 44100, aud.CHANNELS_MONO, aud.FORMAT_S16,
                                  aud.CONTAINER_WAV, aud.CODEC_PCM)
        except Exception:
            aud.Sound(pfad).write(zwischen, 0, aud.CHANNELS_MONO, aud.FORMAT_S16,
                                  aud.CONTAINER_WAV, aud.CODEC_PCM)
        werte, rate = lies_wav(zwischen)
        if rate != 44100:
            faktor = rate / 44100.0
            neu = int(len(werte) / faktor)
            werte = [werte[min(len(werte) - 1, int(i * faktor))] for i in range(neu)]
            rate = 44100
        werte = zuschneiden(werte, rate, dauer)
        ausgabe = os.path.join(ZIEL, name + ".wav")
        schreib_wav(ausgabe, werte, rate)
        print("LALABERG_SOUND %s %.2fs aus %s" % (name, len(werte) / rate, datei))
    if os.path.exists(zwischen):
        os.remove(zwischen)
