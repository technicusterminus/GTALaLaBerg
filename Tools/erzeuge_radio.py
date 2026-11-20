# Erzeugt die Musik der drei Radiosender mit Googles Lyria 3 ueber OpenRouter.
#
#   python Tools/erzeuge_radio.py [Titel je Sender] [Modell]
#
# Der API-Schluessel kommt aus der Umgebung (OPENROUTER_API_KEY); fehlt er, wird ein
# Platzhalter geschickt, den ein vorgeschalteter Proxy ersetzt. Ein voller Song
# (google/lyria-3-pro-preview) kostet etwa 0,08 USD, ein 30-Sekunden-Clip
# (google/lyria-3-clip-preview) 0,04 USD. Die MP3-Dateien landen in Godot/daten/radio und
# gehoeren nicht ins Repository (siehe .gitignore); wer eine Datei schon hat, erzeugt
# sie nicht neu. Ohne diese Dateien spielt das Radio die gerechneten Schleifen aus
# Tools/Sounds.
import base64
import json
import os
import sys
import urllib.request

HIER = os.path.dirname(os.path.abspath(__file__))
ZIEL = os.path.join(HIER, "..", "Godot", "daten", "radio")
URL = "https://openrouter.ai/api/v1/chat/completions"

# Pro Sender mehrere Stimmungen; jeder Auftrag ergibt einen Titel.
AUFTRAEGE = {
    "Blasmusik": [
        "Instrumental Bavarian brass band polka, tuba oom-pah bass, clarinet and trumpets, "
        "cheerful village festival, no vocals",
        "Instrumental American-style brass band parade march, snare drum, trombones, "
        "piccolo flute, triumphant, no vocals",
        "Instrumental Bavarian brass waltz, warm flugelhorns and tuba, gentle and nostalgic "
        "like a beer garden evening, no vocals",
    ],
    "Klassik": [
        "Instrumental romantic piano solo, slow moonlit nocturne, expressive, no vocals",
        "Instrumental baroque chamber orchestra, harpsichord and strings, lively allegro, "
        "no vocals",
        "Instrumental string quartet, impressionist, dreamy harmonies, no vocals",
    ],
    "Lech": [
        "Instrumental 80s synthwave, driving arpeggios, neon night drive, analog bass, "
        "retro drum machine, no vocals",
        "Instrumental melodic techno, pulsing four-on-the-floor, hypnotic synth lead, "
        "late night city, no vocals",
        "Instrumental dreamy retrowave, soft pads, gated reverb snare, sunset over a river, "
        "no vocals",
    ],
}


def erzeuge(modell, auftrag, pfad):
    schluessel = os.environ.get("OPENROUTER_API_KEY", "proxy-injected")
    body = {"model": modell, "modalities": ["text", "audio"], "stream": True,
            "messages": [{"role": "user", "content": auftrag}]}
    anfrage = urllib.request.Request(
        URL, data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json", "Authorization": "Bearer " + schluessel})
    daten = b""
    with urllib.request.urlopen(anfrage, timeout=600) as antwort:
        for zeile in antwort:
            z = zeile.decode().strip()
            if not z.startswith("data:") or z.endswith("[DONE]"):
                continue
            for wahl in json.loads(z[5:]).get("choices", []):
                audio = wahl.get("delta", {}).get("audio")
                if isinstance(audio, dict) and audio.get("data"):
                    daten += base64.b64decode(audio["data"])
    if len(daten) < 10000:
        raise RuntimeError("keine Audiodaten erhalten")
    with open(pfad, "wb") as f:
        f.write(daten)
    return len(daten)


def main():
    je_sender = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    modell = sys.argv[2] if len(sys.argv) > 2 else "google/lyria-3-pro-preview"
    os.makedirs(ZIEL, exist_ok=True)
    for sender, liste in AUFTRAEGE.items():
        for i, auftrag in enumerate(liste[:je_sender], 1):
            pfad = os.path.join(ZIEL, "%s_%02d.mp3" % (sender, i))
            if os.path.exists(pfad):
                print("LALABERG_RADIO %s vorhanden" % os.path.basename(pfad))
                continue
            # Das Modell liefert gelegentlich eine leere Antwort: bis zu dreimal versuchen.
            for versuch in range(1, 4):
                try:
                    n = erzeuge(modell, auftrag, pfad)
                    print("LALABERG_RADIO %s %.1f MB" % (os.path.basename(pfad), n / 1e6),
                          flush=True)
                    break
                except Exception as e:  # ein Fehlschlag soll die uebrigen Titel nicht stoppen
                    print("LALABERG_RADIO %s Versuch %d FEHLER %s"
                          % (os.path.basename(pfad), versuch, e), flush=True)


if __name__ == "__main__":
    main()
