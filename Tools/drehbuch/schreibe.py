#!/usr/bin/env python3
"""Baut aus den Drehbuch-Modulen Content/SourceData/Story/drehbuch.json und das
lesbare Skript Docs/Drehbuch.md, und prueft dabei die Daten:
  - jede Kennung ist eindeutig, jede Voraussetzung existiert, es gibt keine Zyklen
  - jeder Ortsname steht in orte.json (Wahrzeichen und Plaetze) oder ist ein Laden
  - jede Figur ist bekannt, Stufen haben ihre Pflichtfelder
Aufruf:  python3 Tools/drehbuch/schreibe.py
"""
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).parent))
from dsl import FIGUREN  # noqa: E402
from akt0_prolog_akt1 import AKT1, PROLOG  # noqa: E402
from akt2_akt3 import AKT2, AKT3  # noqa: E402
from akt4_finale import AKT4, FINALE  # noqa: E402

WURZEL = Path(__file__).resolve().parents[2]
ORTE = WURZEL / "Content" / "SourceData" / "Orte" / "orte.json"
AUS_JSON = WURZEL / "Content" / "SourceData" / "Story" / "drehbuch.json"
AUS_MD = WURZEL / "Docs" / "Drehbuch.md"
LAEDEN = {"Lackiererei", "Paintball-Laden"}
SPEZIAL_ORTE = {"auto", "spieler"}
AKTE = {0: "Prolog", 1: "Akt I – Gelb: Die Pünktlichen", 2: "Akt II – Blau: Die Schnellen",
        3: "Akt III – Grün: Die Stillen", 4: "Akt IV – Weiß: Die Sieger", 5: "Finale und Abspann"}


def orte() -> set:
    o = json.loads(ORTE.read_text())
    return {m["n"] for m in o["marken"]} | {p["n"] for p in o["plaetze"]} | LAEDEN


def alle_stufen(stufen):
    """Alle Stufen samt der verschachtelten (wenn)."""
    for s in stufen:
        yield s
        if s["art"] == "wenn":
            yield from alle_stufen(s["dann"])
            yield from alle_stufen(s["sonst"])


def pruefe(missionen) -> list:
    fehler = []
    ids = [m["id"] for m in missionen]
    if len(ids) != len(set(ids)):
        fehler.append("doppelte Kennungen")
    bekannt = set(ids)
    namen = orte()
    for m in missionen:
        if m["start"] not in namen | SPEZIAL_ORTE:
            fehler.append(f"{m['id']}: Startort unbekannt: {m['start']}")
        for v in m["voraus"]:
            if v not in bekannt:
                fehler.append(f"{m['id']}: Voraussetzung fehlt: {v}")
        for s in alle_stufen(m["stufen"]):
            ort = s.get("ort")
            if ort and ort not in namen:
                fehler.append(f"{m['id']}: Ort unbekannt: {ort}")
            for sch in s.get("schnitte", []):
                o = sch.get("ort")
                if o and o not in namen | SPEZIAL_ORTE:
                    fehler.append(f"{m['id']}: Kameraort unbekannt: {o}")
                for f in sch.get("figuren", []):
                    if f not in FIGUREN:
                        fehler.append(f"{m['id']}: Figur unbekannt: {f}")
                for z in sch["zeilen"]:
                    if z["wer"] not in FIGUREN:
                        fehler.append(f"{m['id']}: Sprecher unbekannt: {z['wer']}")
                    if len(z["text"]) > 260:
                        fehler.append(f"{m['id']}: Untertitel zu lang ({len(z['text'])})")
            if s["art"] == "funk" and s["wer"] not in FIGUREN:
                fehler.append(f"{m['id']}: Funker unbekannt: {s['wer']}")
            if s["art"] == "wahl" and len(s["optionen"]) < 2:
                fehler.append(f"{m['id']}: Wahl mit weniger als zwei Optionen")
    # Zyklen: jede Mission muss aus den Wurzeln erreichbar sein
    fertig, offen = set(), {m["id"]: set(m["voraus"]) for m in missionen}
    while offen:
        bereit = [i for i, v in offen.items() if v <= fertig]
        if not bereit:
            fehler.append(f"Zyklus oder unerreichbar: {sorted(offen)}")
            break
        for i in bereit:
            fertig.add(i)
            del offen[i]
    return fehler


def skript(missionen) -> str:
    """Das Drehbuch als lesbares Markdown."""
    z = ["# FARBKRIEG – Drehbuch", "",
         "Ein Film in vier Akten, gedreht in Landsberg am Lech. Dieses Skript entsteht "
         "aus `Tools/drehbuch/` (nicht von Hand ändern) und wird vom Spiel als "
         "`Content/SourceData/Story/drehbuch.json` gelesen.", "",
         "## Figuren", ""]
    for k, (name, _fa, gestalt) in FIGUREN.items():
        if k in ("lale_off", "chor"):
            continue
        z.append(f"- **{name}**" + ("" if gestalt else " (nur Stimme)"))
    z += ["", "Alle Namen und Handlungen sind frei erfunden. Nur Landsberg ist echt.", ""]
    akt = None
    for m in missionen:
        if m["akt"] != akt:
            akt = m["akt"]
            z += [f"## {AKTE[akt]}", ""]
        start = "beginnt von selbst" if m["start"] == "auto" else f"Start: {m['start']}"
        z += [f"### {m['name']}", f"*{start} · Lohn {m['lohn']} € · Ruf +{m['ruf']}*", ""]
        z += _stufen_text(m["stufen"], "")
    return "\n".join(z) + "\n"


def _stufen_text(stufen, einzug) -> list:
    z = []
    for s in stufen:
        a = s["art"]
        if a == "szene":
            for sch in s["schnitte"]:
                ort = sch.get("ort") or "am Spielort"
                z.append(f"{einzug}> **SZENE** – {ort}, {sch['art']}")
                for zl in sch["zeilen"]:
                    wer = FIGUREN[zl["wer"]][0].upper()
                    z.append(f"{einzug}> **{wer}:** {zl['text']}  ")
                z.append(f"{einzug}>")
            z.append("")
        elif a == "titel":
            z += [f"{einzug}**[TITEL]** {s['text']} – {s['unter']}", ""]
        elif a == "funk":
            z += [f"{einzug}**[FUNK] {FIGUREN[s['wer']][0]}:** {s['text']}", ""]
        elif a == "wahl":
            z += [f"{einzug}**[WAHL]** {s['frage']}"]
            z += [f"{einzug}- {o['text']}" for o in s["optionen"]] + [""]
        elif a == "wenn":
            z += [f"{einzug}**[WENN {s['flag']} = {s['ist']}]**", ""]
            z += _stufen_text(s["dann"], einzug)
            if s["sonst"]:
                z += [f"{einzug}**[SONST]**", ""] + _stufen_text(s["sonst"], einzug)
        elif a in ("setze", "gib"):
            continue
        else:
            z += [f"{einzug}- *{a}* – {s.get('text', '')}"]
    if z and z[-1] != "":
        z.append("")
    return z


def uebernimm_texte(missionen) -> int:
    """Legt die ueberarbeiteten Texte (texte.json, siehe ueberarbeite.py) ueber die Module.
    Zaehlt, wie viele Missionen neue Texte bekommen haben."""
    pfad = Path(__file__).parent / "texte.json"
    if not pfad.exists():
        return 0
    from ueberarbeite import felder  # noqa: E402
    neu = json.loads(pfad.read_text(encoding="utf-8"))
    n = 0
    for m in missionen:
        if m["id"] not in neu:
            continue
        fl = list(felder(m["stufen"]))
        if len(fl) != len(neu[m["id"]]):
            print(f"{m['id']}: texte.json passt nicht zur Struktur, Mission bleibt alt",
                  file=sys.stderr)
            continue
        for i, (obj, schluessel, _a, _w, _p) in enumerate(fl):
            obj[schluessel] = neu[m["id"]][str(i)]
        n += 1
    return n


def vergebe_kennungen(missionen) -> None:
    """Jede gesprochene Zeile (Dialog und Funk) bekommt die Kennung "<Mission>.<Nr>"; unter dieser
    Kennung liegt ihre Sprachdatei (Godot/daten/sprache/<Kennung>.ogg, siehe erzeuge_sprache.py)."""
    from ueberarbeite import felder  # noqa: E402
    for m in missionen:
        for i, (obj, _k, art, _w, _p) in enumerate(felder(m["stufen"])):
            if art in ("ZEILE", "FUNK"):
                obj["id"] = f"{m['id']}.{i}"


def main() -> int:
    missionen = PROLOG + AKT1 + AKT2 + AKT3 + AKT4 + FINALE
    neu = uebernimm_texte(missionen)
    vergebe_kennungen(missionen)
    fehler = pruefe(missionen)
    if fehler:
        print("\n".join(fehler), file=sys.stderr)
        return 1
    figuren = {k: {"name": n, "farbe": f, "gestalt": g} for k, (n, f, g) in FIGUREN.items()}
    AUS_JSON.write_text(json.dumps({"schema": 2, "titel": "FARBKRIEG", "figuren": figuren,
                                    "missionen": missionen}, ensure_ascii=False, indent=1))
    AUS_MD.write_text(skript(missionen))
    zeilen = sum(len(sch["zeilen"]) for m in missionen for s in alle_stufen(m["stufen"])
                 for sch in s.get("schnitte", []))
    szenen = sum(1 for m in missionen for s in alle_stufen(m["stufen"]) if s["art"] == "szene")
    print(f"{len(missionen)} Missionen, {szenen} Szenen, {zeilen} Zeilen Dialog, "
          f"{neu} Missionen mit ueberarbeiteten Texten")
    return 0


if __name__ == "__main__":
    sys.exit(main())
