#!/usr/bin/env python3
"""Vertont alle Dialog- und Funkzeilen des Drehbuchs mit dem Audio-Modell auf OpenRouter.

  python3 Tools/erzeuge_sprache.py [Modell]

Liest Content/SourceData/Story/drehbuch.json (nach Tools/drehbuch/schreibe.py: jede Zeile hat
eine Kennung), spricht jede Zeile in der Stimme ihrer Figur und schreibt Ogg-Dateien nach
Godot/daten/sprache/<Kennung>.ogg. Vorhandene Dateien werden nicht neu erzeugt. Braucht
ffmpeg. Der Schluessel kommt vom Proxy (Platzhalter) oder aus OPENROUTER_API_KEY. Kosten:
rund 0,0005 USD je Zeile mit gpt-audio-mini.
"""
import base64
import json
import os
import subprocess
import sys
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

WURZEL = Path(__file__).resolve().parent.parent
QUELLE = WURZEL / "Content" / "SourceData" / "Story" / "drehbuch.json"
ZIEL = WURZEL / "Godot" / "daten" / "sprache"

# Figur -> (Stimme, Sprechweise)
STIMMEN = {
    "lale": ("coral", "junge Frau, wach, ein bisschen frech, Landsberger Einschlag"),
    "lale_off": ("sage", "dieselbe Frau als Erzaehlerin im Rueckblick, ruhig, trocken, leise ironisch"),
    "marta": ("shimmer", "aeltere Krankenschwester, resolut, warm, bayerisch gefaerbt"),
    "funk": ("onyx", "der Schiedsrichter im Funk: tief, ruhig, ueberlegen, mit trockenem Humor"),
    "konrad": ("ash", "pedantischer Dispatcher, knapp, spitz, leicht bayerisch"),
    "anneliese": ("nova", "Kapitaenin, selbstbewusst, schnell, mit Schmunzeln"),
    "ottmar": ("echo", "Biologielehrer im Ruhestand, leise, genau, nachdenklich"),
    "xaver": ("fable", "Wirt, behaebig, laut, herzlich, stark bayerisch"),
    "korbi": ("verse", "Wachtmeister, dienstlich, muede, gutmuetig, bayerisch"),
    "schorsch": ("ballad", "Kleinganove und Mechaniker, schnoddrig, bayerisch"),
    "pfarrer": ("alloy", "alter Pfarrer, mild, langsam, mit Pausen"),
    "chor": ("alloy", "Menge, durcheinander, laut"),
}
BEKANNTE = {"alloy", "ash", "ballad", "coral", "echo", "fable", "nova", "onyx", "sage", "shimmer", "verse"}


def sprich(modell: str, wer: str, text: str) -> bytes:
    stimme, art = STIMMEN.get(wer, ("alloy", "natuerlich"))
    stimme = stimme if stimme in BEKANNTE else "alloy"
    auftrag = ("Du bist Synchronsprecher. Sprich den folgenden deutschen Text genau so, ohne etwas "
               "hinzuzufuegen oder wegzulassen, in der Rolle: %s. Nur den Text sprechen, kein "
               "Kommentar.\n\nTEXT: %s" % (art, text))
    body = {"model": modell, "modalities": ["text", "audio"], "stream": True,
            "audio": {"voice": stimme, "format": "pcm16"},
            "messages": [{"role": "user", "content": auftrag}]}
    anfrage = urllib.request.Request(
        "https://openrouter.ai/api/v1/chat/completions", data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json", "Authorization": "Bearer "
                 + os.environ.get("OPENROUTER_API_KEY", "proxy-injected")})
    daten = b""
    with urllib.request.urlopen(anfrage, timeout=300) as r:
        for zeile in r:
            z = zeile.decode().strip()
            if not z.startswith("data:") or z.endswith("[DONE]"):
                continue
            for c in json.loads(z[5:]).get("choices", []):
                a = c.get("delta", {}).get("audio")
                if isinstance(a, dict) and a.get("data"):
                    daten += base64.b64decode(a["data"])
    if len(daten) < 4000:
        raise RuntimeError("keine Audiodaten")
    return daten


def ogg(pcm: bytes, pfad: Path) -> None:
    """PCM16 mit 24 kHz mono -> Ogg Vorbis."""
    subprocess.run(["ffmpeg", "-y", "-loglevel", "error", "-f", "s16le", "-ar", "24000", "-ac", "1",
                    "-i", "-", "-c:a", "libvorbis", "-q:a", "4", str(pfad)], input=pcm, check=True)


def zeilen(daten: dict):
    def lauf(stufen):
        for s in stufen:
            if s["art"] == "szene":
                for sch in s["schnitte"]:
                    for z in sch["zeilen"]:
                        yield z
            elif s["art"] == "funk":
                yield s
            elif s["art"] == "wenn":
                yield from lauf(s["dann"])
                yield from lauf(s["sonst"])
    for m in daten["missionen"]:
        yield from lauf(m["stufen"])


def eine(args):
    modell, z = args
    pfad = ZIEL / f"{z['id']}.ogg"
    if pfad.exists():
        return "vorhanden"
    for versuch in range(3):
        try:
            ogg(sprich(modell, z["wer"], z["text"]), pfad)
            return "ok"
        except Exception as e:  # ein Fehlschlag stoppt die uebrigen Zeilen nicht
            fehler = str(e)
    return "FEHLER " + fehler


def main() -> None:
    modell = sys.argv[1] if len(sys.argv) > 1 else "openai/gpt-audio-mini"
    ZIEL.mkdir(parents=True, exist_ok=True)
    daten = json.loads(QUELLE.read_text(encoding="utf-8"))
    liste = [z for z in zeilen(daten) if z.get("id")]
    with ThreadPoolExecutor(6) as pool:
        ergebnisse = list(pool.map(eine, [(modell, z) for z in liste]))
    print("LALABERG_SPRACHE zeilen=%d ok=%d vorhanden=%d fehler=%d" % (
        len(liste), ergebnisse.count("ok"), ergebnisse.count("vorhanden"),
        sum(1 for r in ergebnisse if r.startswith("FEHLER"))))


if __name__ == "__main__":
    main()
