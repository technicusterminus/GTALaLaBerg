# Laedt die Radiotitel (Tools/radio_titel.json) von Pixabay herunter.
#
#   python Tools/hole_radio.py [Zielordner]
#
# Voreinstellung ist ein Ordner ausserhalb des Repos: die MP3-Quellen
# gehoeren nicht hinein, nur das umgewandelte Ergebnis (siehe
# Tools/wandle_radio.py). Wer eine Datei schon hat, laedt sie nicht neu.
import json
import os
import sys
import urllib.request

HIER = os.path.dirname(os.path.abspath(__file__))
LISTE = os.path.join(HIER, "radio_titel.json")
ZIEL = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.environ.get("TEMP", "."), "lalaberg_radio")

KOPF = {"User-Agent": "Mozilla/5.0", "Referer": "https://pixabay.com/"}


def dateiname(sender, nummer, titel):
    sauber = "".join(c if c.isalnum() or c in " -_" else "_" for c in titel).strip().replace(" ", "_")
    return "%s_%02d_%s.mp3" % (sender, nummer, sauber[:48])


if __name__ == "__main__":
    daten = json.load(open(LISTE, encoding="utf-8"))
    gesamt = 0
    for sender, titel in daten["sender"].items():
        ordner = os.path.join(ZIEL, sender)
        os.makedirs(ordner, exist_ok=True)
        for i, t in enumerate(titel, start=1):
            pfad = os.path.join(ordner, dateiname(sender, i, t["t"]))
            if os.path.isfile(pfad) and os.path.getsize(pfad) > 10000:
                print("schon da: " + os.path.basename(pfad))
                gesamt += os.path.getsize(pfad)
                continue
            try:
                auftrag = urllib.request.Request(t["u"], headers=KOPF)
                with urllib.request.urlopen(auftrag, timeout=120) as antwort, open(pfad, "wb") as datei:
                    datei.write(antwort.read())
                gesamt += os.path.getsize(pfad)
                print("LALABERG_RADIO geladen %s (%.1f MB)" % (os.path.basename(pfad), os.path.getsize(pfad) / 1e6))
            except Exception as fehler:
                print("LALABERG_RADIO FEHLER %s: %s" % (t["t"], fehler))
    print("LALABERG_RADIO gesamt %.1f MB in %s" % (gesamt / 1e6, ZIEL))
