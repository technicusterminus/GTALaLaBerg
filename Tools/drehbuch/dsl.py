"""Kleine Schreibhilfe fuer das Drehbuch (Content/SourceData/Story/drehbuch.json).

Jede Funktion liefert ein Woerterbuch im Format, das Godot/scripts/drehbuch.gd liest.
Stufenarten: hin, bring, markiere, stelle, halte, im_wagen, warte_revier (Spiel),
szene, titel, funk, wahl, wenn, setze, gib (Film und Zustand).
"""

# --- Figuren: Kennung -> (Anzeigename, Untertitelfarbe, Gestaltfarbe) ----------
FIGUREN = {
    "lale": ("Lale", "#ffffff", "#e76f51"),
    "lale_off": ("Lale (Off)", "#d9d9d9", None),
    "marta": ("Schwester Marta", "#8ecae6", "#8ecae6"),
    "funk": ("Der Schiedsrichter", "#c9c9c9", None),
    "konrad": ("Konrad Weidinger", "#f6c90e", "#f6c90e"),
    "anneliese": ("Anneliese Lechner", "#4d8bff", "#4d8bff"),
    "ottmar": ("Ottmar Wald", "#4fbf6d", "#4fbf6d"),
    "xaver": ("Xaver Gruber", "#f4f1e6", "#f4f1e6"),
    "korbi": ("Wachtmeister Brandl", "#9aa5b1", "#6c757d"),
    "schorsch": ("Schorsch", "#ff9f5a", "#ff9f5a"),
    "pfarrer": ("Pfarrer Alois", "#d6b3ff", "#b38ad6"),
    "chor": ("Menge", "#cfcfcf", None),
}


def zeile(wer, text):
    assert wer in FIGUREN, f"unbekannte Figur {wer}"
    return {"wer": wer, "text": text}


def schnitt(zeilen, ort=None, art="statisch", hoehe=None, abstand=None, winkel=None,
            dauer=None, figuren=None):
    """Ein Kameraschnitt. ort: Ortsname, "spieler" oder "auto" (Standard: dort, wo die
    Figur steht). art: statisch, umkreise, fahrt, kran."""
    s = {"art": art, "zeilen": zeilen}
    for k, v in (("ort", ort), ("hoehe", hoehe), ("abstand", abstand), ("winkel", winkel),
                 ("dauer", dauer), ("figuren", figuren)):
        if v is not None:
            s[k] = v
    return s


def szene(*schnitte):
    return {"art": "szene", "schnitte": list(schnitte)}


def titel(text, unter="", dauer=4.0):
    return {"art": "titel", "text": text, "unter": unter, "dauer": dauer}


def funk(wer, text):
    return {"art": "funk", "wer": wer, "text": text}


def hin(ort, text, zeit=0, im_wagen=False):
    return {"art": "hin", "ort": ort, "text": text, "zeit": zeit, "imWagen": im_wagen}


def bring(ort, text, zeit=0):
    return {"art": "bring", "ort": ort, "text": text, "zeit": zeit}


def markiere(ort, text, zeit=0):
    return {"art": "markiere", "ort": ort, "text": text, "zeit": zeit}


def stelle(text, zeit=0):
    return {"art": "stelle", "text": text, "zeit": zeit}


def halte(text, zeit, sterne=0):
    return {"art": "halte", "text": text, "zeit": zeit, "sterne": sterne}


def im_wagen(text):
    return {"art": "im_wagen", "text": text}


def warte_revier(nr, text):
    return {"art": "warte_revier", "nr": nr, "text": text}


def option(text, **setze):
    return {"text": text, "setze": setze}


def wahl(frage, *optionen):
    return {"art": "wahl", "frage": frage, "optionen": list(optionen)}


def wenn(flag, ist, dann, sonst=None):
    return {"art": "wenn", "flag": flag, "ist": ist, "dann": dann, "sonst": sonst or []}


def setze(flag, wert):
    return {"art": "setze", "flag": flag, "wert": wert}


def gib(geld=0, ruf=0, waffe=None):
    g = {"art": "gib", "geld": geld, "ruf": ruf}
    if waffe is not None:
        g["waffe"] = waffe
    return g


def mission(id, name, akt, start, voraus, lohn, ruf, stufen):
    """start: Ortsname oder "auto" (beginnt von selbst, sobald sie dran ist)."""
    return {"id": id, "name": name, "akt": akt, "start": start, "voraus": list(voraus),
            "lohn": lohn, "ruf": ruf, "stufen": stufen}
