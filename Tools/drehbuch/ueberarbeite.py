#!/usr/bin/env python3
"""Laesst ein Sprachmodell auf OpenRouter alle Texte des Drehbuchs neu schreiben, ohne die
Struktur anzufassen (Missionen, Stufen, Verzweigungen, Kameras bleiben).

  python3 Tools/drehbuch/ueberarbeite.py [Modell]

Je Akt ein Aufruf: das Modell bekommt die Stilbibel (Docs/Konzept/drehbuch-kritik-und-
stilbibel.md), die Handlung und die Texte des Akts samt Sprecher und Zusammenhang und gibt
dieselbe Anzahl Texte zurueck. Das Ergebnis steht in Tools/drehbuch/texte.json; schreibe.py
legt es beim Bauen ueber die Texte der Module. Aufruf ohne Schluessel: der Proxy setzt ihn ein
(sonst OPENROUTER_API_KEY).
"""
import json
import os
import re
import sys
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from dsl import FIGUREN  # noqa: E402
from akt0_prolog_akt1 import AKT1, PROLOG  # noqa: E402
from akt2_akt3 import AKT2, AKT3  # noqa: E402
from akt4_finale import AKT4, FINALE  # noqa: E402

HIER = Path(__file__).parent
WURZEL = HIER.parents[1]
BIBEL = WURZEL / "Docs" / "Konzept" / "drehbuch-kritik-und-stilbibel.md"
AUS = HIER / "texte.json"
AKTE = {"prolog_akt1": PROLOG + AKT1, "akt2": AKT2, "akt3": AKT3, "akt4": AKT4,
        "finale": FINALE}


def felder(stufen, pfad=""):
    """Alle Textfelder in fester Reihenfolge: (Wörterbuch, Schluessel, Art, Sprecher, Bedingung)."""
    for s in stufen:
        art = s["art"]
        if art == "titel":
            yield s, "text", "TITEL", "", pfad
            yield s, "unter", "UNTERTITEL", "", pfad
        elif art == "funk":
            yield s, "text", "FUNK", s["wer"], pfad
        elif art == "szene":
            for sch in s["schnitte"]:
                for z in sch["zeilen"]:
                    yield z, "text", "ZEILE", z["wer"], pfad
        elif art == "wahl":
            yield s, "frage", "FRAGE", "", pfad
            for o in s["optionen"]:
                yield o, "text", "ANTWORT-OPTION %s" % json.dumps(o["setze"], ensure_ascii=False), \
                    "lale", pfad
        elif art == "wenn":
            bed = "wenn %s == %s" % (s["flag"], s["ist"])
            yield from felder(s["dann"], bed)
            yield from felder(s["sonst"], "sonst von " + bed)
        elif "text" in s:
            yield s, "text", "AUFGABE (kurze Anzeige, ein Satz)", "", pfad


def alle_felder_je_mission(missionen):
    for m in missionen:
        yield m, list(felder(m["stufen"]))


def frage(modell, auftrag):
    body = {"model": modell, "reasoning": {"effort": "low"}, "max_tokens": 16000,
            "messages": [{"role": "user", "content": auftrag}]}
    anfrage = urllib.request.Request(
        "https://openrouter.ai/api/v1/chat/completions", data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json", "Authorization": "Bearer "
                 + os.environ.get("OPENROUTER_API_KEY", "proxy-injected")})
    d = json.load(urllib.request.urlopen(anfrage, timeout=900))
    return d["choices"][0]["message"].get("content") or "", d.get("usage", {}).get("cost", 0)


def main():
    modell = sys.argv[1] if len(sys.argv) > 1 else "anthropic/claude-opus-5.5"
    bibel = BIBEL.read_text(encoding="utf-8")
    ergebnis = json.loads(AUS.read_text(encoding="utf-8")) if AUS.exists() else {}
    figuren = "\n".join("- %s (%s)" % (k, v[0]) for k, v in FIGUREN.items())
    kosten = 0.0
    for akt, missionen in AKTE.items():
        todo = [m for m in missionen if m["id"] not in ergebnis]
        if not todo:
            continue
        zeilen = []
        for m, fl in alle_felder_je_mission(todo):
            zeilen.append("== MISSION %s \"%s\" ==" % (m["id"], m["name"]))
            for i, (_obj, _k, art, wer, pfad) in enumerate(fl):
                kopf = "%s.%d | %s%s%s" % (m["id"], i, art, " | " + wer if wer else "",
                                           " | " + pfad if pfad else "")
                zeilen.append("%s | %s" % (kopf, json.dumps(_obj[_k], ensure_ascii=False)))
        auftrag = (
            "Du bist Drehbuchautor und Dialogregisseur. Unten stehen Kritik, Stilbibel und "
            "Handlungskern fuer das Spiel FARBKRIEG, danach die Texte eines Akts (jede Zeile: "
            "Kennung | Art | Sprecher | Bedingung | aktueller Text als JSON-String).\n\n"
            "Schreibe JEDEN Text neu, so dass er die Kritik behebt und den Ton der Stilbibel "
            "trifft. Regeln:\n- Struktur bleibt: gleiche Kennungen, gleiche Anzahl, gleicher "
            "Sprecher, gleiche Handlung und Ausgaenge der Szene; Verzweigungs-Texte (Bedingung) "
            "muessen zur jeweiligen Antwort-Option passen.\n- Ortsnamen, Eigennamen, Zahlen und "
            "Fristen in AUFGABE-Texten bleiben (Bahnhof, Schmalzturm ...), sie stehen so im Spiel.\n"
            "- Keine Kalendersprueche, kein 'nicht X, sondern Y', keine erklaerenden Saetze, die "
            "die Figur ueber sich selbst sagt. Stattdessen: konkrete Dinge, Absichten, "
            "Unterbrechungen, Unausgesprochenes, Landsberger und bayerische Faerbung je nach "
            "Figur (kurz, nicht albern). Jede Zeile soll etwas tun: etwas verlangen, verheimlichen, "
            "drohen, ablenken oder preisgeben.\n- Laenge: Dialogzeilen 1 bis 3 Saetze, AUFGABE "
            "hoechstens 70 Zeichen, FRAGE und OPTION knapp. TITEL und UNTERTITEL duerfen "
            "wirken.\n- Gib NUR ein JSON-Objekt zurueck (in einem ```json Block): "
            "{\"Kennung\": \"neuer Text\", ...} fuer alle Zeilen.\n\n"
            "FIGUREN:\n%s\n\nKRITIK UND STILBIBEL:\n%s\n\nTEXTE:\n%s\n" % (
                figuren, bibel, "\n".join(zeilen)))
        text, kosten_akt = frage(modell, auftrag)
        kosten += kosten_akt
        treffer = re.search(r"```json\s*(\{.*\})\s*```", text, re.S) or re.search(r"(\{.*\})", text, re.S)
        if not treffer:
            print("LALABERG_DREHBUCH %s: keine Antwort" % akt)
            continue
        neu = json.loads(treffer.group(1))
        for m in todo:
            fl = list(felder(m["stufen"]))
            fehlt = [i for i in range(len(fl)) if "%s.%d" % (m["id"], i) not in neu]
            if fehlt:
                print("LALABERG_DREHBUCH %s %s: %d Texte fehlen, Mission bleibt alt" % (
                    akt, m["id"], len(fehlt)))
                continue
            ergebnis[m["id"]] = {str(i): neu["%s.%d" % (m["id"], i)] for i in range(len(fl))}
        AUS.write_text(json.dumps(ergebnis, ensure_ascii=False, indent=1), encoding="utf-8")
        print("LALABERG_DREHBUCH %s fertig, %d Missionen, Kosten %.2f USD" % (
            akt, len(todo), kosten_akt), flush=True)
    print("LALABERG_DREHBUCH Summe %.2f USD" % kosten)


if __name__ == "__main__":
    main()
