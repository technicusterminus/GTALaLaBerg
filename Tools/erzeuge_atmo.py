#!/usr/bin/env python3
"""Atmosphaere draussen (Voegel, Wind, ferne Autos) mit Lyria ueber OpenRouter nach
Godot/daten/ton_atmo/draussen.mp3 (nicht im Repository). Hinweis: Lyria macht bei Menschenmengen
und Plaetzen gern Musik daraus; geprueft wurde mit einem Audio-Modell (gpt-audio), nur die
Strassen-Aufnahme war wirklich Atmosphaere. Das Brunnenplaetschern ist gerechnet
(Tools/Sounds/SFX_Brunnen.wav).

  python3 Tools/erzeuge_atmo.py
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from erzeuge_radio import erzeuge  # noqa: E402

ZIEL = Path(__file__).resolve().parent.parent / "Godot" / "daten" / "ton_atmo"
AUFTRAG = ("Realistic ambient field recording, no music, no melody, no instruments: a small German "
           "town street, light traffic of passing cars, distant engine sounds, tyres on asphalt, a "
           "moped, birds, wind. Natural soundscape only, continuous, seamless.")

if __name__ == "__main__":
    ZIEL.mkdir(parents=True, exist_ok=True)
    pfad = ZIEL / "draussen.mp3"
    if not pfad.exists():
        print("LALABERG_ATMO", erzeuge("google/lyria-3-clip-preview", AUFTRAG, str(pfad)))
