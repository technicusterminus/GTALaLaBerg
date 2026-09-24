# Wandelt die heruntergeladenen Radiotitel (Tools/hole_radio.py) in die Form,
# die das Spiel braucht: 32 kHz, Mono, 16 Bit WAV, ausgesteuert.
#
#   "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b -P Tools/wandle_radio.py -- <Quellordner>
#
# 32 kHz statt 44,1: das ist Autoradio, kein Konzertsaal - und es halbiert
# fast den Platz, den die Titel im Projekt belegen. Blender liefert den
# Dekoder (Modul "aud"), weil das Projekt weder ffmpeg noch numpy hat.
#
# Das Ergebnis landet in Tools/Musik/<Sender>/ und wird von
# Tools/importiere_radio.py ins Spiel geholt. Weder die MP3-Quellen noch
# diese WAVs gehoeren ins Repo - es reichen die Liste (radio_titel.json)
# und diese Skripte, um alles wieder herzustellen.
import os
import struct
import sys
import wave

import aud

HIER = os.path.dirname(os.path.abspath(__file__))
ZIEL = os.path.join(HIER, "Musik")
ARGV = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
QUELLE = ARGV[0] if ARGV else os.path.join(os.environ.get("TEMP", "."), "lalaberg_radio")
TAKT = 32000


# Kein einziger Abtastwert geht durch Python: das Umrechnen auf 32 kHz, das
# Zusammenlegen auf Mono und die Lautstaerke macht alles das C-Modul. Der
# erste Versuch normierte in Python ueber 380 Millionen Werte und haette
# Stunden gebraucht.
def wandle(quelle, ziel):
    klang = aud.Sound(quelle).rechannel(1).volume(0.9)
    try:
        klang.resample(TAKT, True).write(ziel, TAKT, aud.CHANNELS_MONO, aud.FORMAT_S16,
                                         aud.CONTAINER_WAV, aud.CODEC_PCM)
        return TAKT
    except Exception:
        # Bei manchen Quelltakten (48 kHz) bricht ffmpeg beim Umrechnen ab.
        # Dann im Originaltakt schreiben - Unreal rechnet beim Kochen ohnehin
        # noch einmal um, es kostet nur etwas mehr Platz im Projekt.
        klang.write(ziel, 0, aud.CHANNELS_MONO, aud.FORMAT_S16, aud.CONTAINER_WAV, aud.CODEC_PCM)
        return 0


if __name__ == "__main__":
    if not os.path.isdir(QUELLE):
        raise SystemExit("Quellordner fehlt: " + QUELLE)
    gesamt = 0.0
    for sender in sorted(os.listdir(QUELLE)):
        ordner = os.path.join(QUELLE, sender)
        if not os.path.isdir(ordner):
            continue
        ausgabe_ordner = os.path.join(ZIEL, sender)
        os.makedirs(ausgabe_ordner, exist_ok=True)
        for datei in sorted(os.listdir(ordner)):
            if not datei.endswith(".mp3"):
                continue
            ziel = os.path.join(ausgabe_ordner, datei[:-4] + ".wav")
            # Groesse mitpruefen: ein abgebrochener Versuch hinterlaesst eine
            # winzige Datei, und die galt sonst als "schon gewandelt".
            if os.path.isfile(ziel) and os.path.getsize(ziel) > 200000:
                print("schon gewandelt: " + datei)
                continue
            try:
                wandle(os.path.join(ordner, datei), ziel)
            except Exception as fehler:
                print("LALABERG_RADIO FEHLER %s: %s" % (datei, fehler))
                continue
            sekunden = os.path.getsize(ziel) / (TAKT * 2.0)   # Naeherung, siehe wandle()
            gesamt += sekunden
            print("LALABERG_RADIO gewandelt %s %.1fs" % (datei, sekunden))
    print("LALABERG_RADIO gesamt %.1f Minuten" % (gesamt / 60.0))
