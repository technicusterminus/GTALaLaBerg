#!/usr/bin/env python3
"""Erzeugt die Spielmusik (Menue, Fahrt, Hubschrauber, Verfolgung) mit Googles Lyria 3 ueber
OpenRouter nach Godot/daten/musik/<Name>.mp3 (nicht im Repository; vorhandene Dateien bleiben).

  python3 Tools/erzeuge_musik.py [Modell]

Ein voller Song (google/lyria-3-pro-preview) kostet etwa 0,08 USD. Das Radio im Auto
(Tools/erzeuge_radio.py) ist davon getrennt.
"""
import os
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from erzeuge_radio import erzeuge  # noqa: E402

ZIEL = Path(__file__).resolve().parent.parent / "Godot" / "daten" / "musik"
STUECKE = {
    "menue": "Instrumental warm and inviting game menu theme: Bavarian zither, accordion and "
             "soft acoustic guitar over gentle strings, relaxed and nostalgic, seamlessly loopable, "
             "no vocals",
    "fahrt": "Instrumental upbeat open-world driving music: light brass, groovy bass, modern "
             "drums, a subtle Bavarian folk melody, sunny and free, loopable, no vocals",
    "heli": "Instrumental cinematic aerial flight theme over a river town: soaring strings, "
            "warm synth pads, steady pulse, majestic and calm with a sense of height, loopable, "
            "no vocals",
    "verfolgung": "Instrumental tense car chase music: fast driving percussion, urgent brass stabs, "
                  "pulsing bass, rising tension, loopable, no vocals",
}


def main() -> None:
    modell = sys.argv[1] if len(sys.argv) > 1 else "google/lyria-3-pro-preview"
    ZIEL.mkdir(parents=True, exist_ok=True)
    for name, auftrag in STUECKE.items():
        pfad = ZIEL / f"{name}.mp3"
        if pfad.exists():
            print("LALABERG_MUSIK %s vorhanden" % name)
            continue
        for versuch in range(1, 4):
            try:
                n = erzeuge(modell, auftrag, str(pfad))
                print("LALABERG_MUSIK %s %.1f MB" % (name, n / 1e6), flush=True)
                break
            except Exception as e:  # Lyria liefert gelegentlich leer
                print("LALABERG_MUSIK %s Versuch %d FEHLER %s" % (name, versuch, e), flush=True)


if __name__ == "__main__":
    main()
