# Baut die Fahrzeugtexturen aus: jede Ansicht (Seite, Oben, Front, Heck) einzeln vom Bildmodell
# auf OpenRouter in hoher Aufloesung neu rendern lassen - gleicher Umriss, gleiche Lage, aber
# viel feinere Einzelheiten (Scheinwerfer-Innenleben, Grillgitter, Reifenflanke, Fugen).
# Die Ansicht wird dazu auf ein unterstuetztes Seitenverhaeltnis mit weissem Rand gebracht und
# das Ergebnis wieder auf genau diesen Ausschnitt zugeschnitten, damit es zur Geometrie passt.
# Ergebnisse liegen in Docs/Konzept/autos/hd/ (einmal bezahlt, danach nur noch angewandt).
#
#   python Tools/verfeinere_autotexturen.py [modell ...] [--neu]
#
# Voraussetzung: Tools/baue_auto_modelle.py bzw. baue_auto_profil.py ist gelaufen.
import base64
import io
import json
import os
import sys
import urllib.request

import numpy as np
from PIL import Image

HIER = os.path.dirname(os.path.abspath(__file__))
TEXTUREN = os.path.join(HIER, "..", "Godot", "texturen")
HD = os.path.join(HIER, "..", "Docs", "Konzept", "autos", "hd")
MODELLE = ["klein", "limousine", "kombi", "bus", "sport", "renn", "super"]
ANSICHTEN = {"seite": "side", "oben": "top-down", "front": "front", "heck": "rear"}
VERHAELTNISSE = {"1:1": 1.0, "4:3": 4 / 3, "3:2": 1.5, "16:9": 16 / 9, "21:9": 21 / 9,
                 "3:4": 3 / 4, "2:3": 2 / 3, "4:5": 0.8, "5:4": 1.25}
FAKTOR = 3          # so viel feiner wird die Textur (Pixel je Meter)
MODELL_BILD = "google/gemini-nano-banana-2.1"   # kann 4K

AUFTRAG = (
    "This is an orthographic {ansicht} view of a car on a pure white background, part of a "
    "texture sheet. Re-render it as an ultra-detailed photorealistic texture at maximum "
    "resolution. KEEP EXACTLY: the outline, position, size, framing and proportions (it must "
    "overlay the input pixel-perfectly), the orthographic projection, the neutral light grey "
    "body paint (do not change the paint color or add colored reflections), and the white "
    "background. ADD fine realistic detail: crisp panel gaps and shut lines, detailed headlight "
    "internals (projector lenses, LED daytime strips, chrome reflectors), tail light internals, "
    "fine grille mesh, tyre sidewall lettering and tread, brake discs behind the rims, rubber "
    "window seals, door handles, badges removed, glass dark with a hint of interior. No "
    "shadows on the ground, no text labels, no perspective change.")


def name(modell: str, ansicht: str) -> str:
    return f"auto_{ansicht}" if modell == "klein" else f"auto_{modell}_{ansicht}"


def anfrage(bild: Image.Image, auftrag: str, verhaeltnis: str) -> Image.Image:
    puffer = io.BytesIO()
    bild.save(puffer, "PNG")
    daten = base64.b64encode(puffer.getvalue()).decode()
    body = {"model": MODELL_BILD, "modalities": ["image", "text"],
            "image_config": {"aspect_ratio": verhaeltnis, "image_size": "4K"},
            "messages": [{"role": "user", "content": [
                {"type": "text", "text": auftrag},
                {"type": "image_url", "image_url": {"url": "data:image/png;base64," + daten}}]}]}
    req = urllib.request.Request(
        "https://openrouter.ai/api/v1/chat/completions", data=json.dumps(body).encode(),
        headers={"Content-Type": "application/json",
                 "Authorization": "Bearer " + os.environ.get("OPENROUTER_API_KEY", "proxy-injected")})
    antwort = json.load(urllib.request.urlopen(req, timeout=600))
    nachricht = antwort["choices"][0]["message"]
    bilder = nachricht.get("images") or []
    if not bilder:
        raise RuntimeError("kein Bild: " + str(nachricht.get("content"))[:200])
    url = bilder[0]["image_url"]["url"]
    kosten = antwort.get("usage", {}).get("cost")
    print("   Kosten", kosten)
    return Image.open(io.BytesIO(base64.b64decode(url.split(",", 1)[1]))).convert("RGB")


def verfeinere(modell: str, ansicht: str, neu: bool) -> None:
    pfad = os.path.join(TEXTUREN, name(modell, ansicht) + ".png")
    if not os.path.exists(pfad):
        return
    os.makedirs(HD, exist_ok=True)
    ziel_hd = os.path.join(HD, f"{modell}_{ansicht}.png")
    quelle = Image.open(pfad).convert("RGB")
    w, h = quelle.size
    if neu or not os.path.exists(ziel_hd):
        # Auf das naechste unterstuetzte Seitenverhaeltnis auffuellen (Mitte, weiss, 6 % Rand).
        verh = min(VERHAELTNISSE, key=lambda k: abs(np.log(VERHAELTNISSE[k] / (w / h))))
        r = VERHAELTNISSE[verh]
        pw, ph = (int(w * 1.06), int(w * 1.06 / r)) if w / h > r else (int(h * 1.06 * r), int(h * 1.06))
        pw, ph = max(pw, w + 2), max(ph, h + 2)
        leinwand = Image.new("RGB", (pw, ph), "white")
        ox, oy = (pw - w) // 2, (ph - h) // 2
        leinwand.paste(quelle, (ox, oy))
        print(f"{modell} {ansicht}: {w}x{h} -> {verh}")
        ergebnis = anfrage(leinwand, AUFTRAG.format(ansicht=ANSICHTEN[ansicht]), verh)
        # Zurueck auf den Ausschnitt: das Ergebnis deckt die ganze Leinwand.
        sx, sy = ergebnis.width / pw, ergebnis.height / ph
        ausschnitt = ergebnis.crop((round(ox * sx), round(oy * sy), round((ox + w) * sx),
                                    round((oy + h) * sy)))
        ausschnitt.resize((w * FAKTOR, h * FAKTOR), Image.LANCZOS).save(ziel_hd)
    hd = Image.open(ziel_hd).convert("RGB")
    hd.save(pfad)
    # Masken mitziehen: Lack, Glas, Lampen weich hochskaliert; die Hoehe (Relief) neu aus der
    # feinen Textur, damit Fugen und Gitter scharf werden.
    mpfad = os.path.join(TEXTUREN, name(modell, ansicht) + "_maske.png")
    if os.path.exists(mpfad):
        maske = Image.open(mpfad).convert("RGBA")
        if maske.size != hd.size:
            maske = maske.resize(hd.size, Image.BICUBIC)
        kanaele = np.asarray(maske).astype(np.float32) / 255.0
        kanaele[..., 3] = hoehe(np.asarray(hd), kanaele[..., 1], float(FAKTOR))
        Image.fromarray((np.clip(kanaele, 0, 1) * 255).astype(np.uint8), "RGBA").save(mpfad)


def hoehe(rgb: np.ndarray, glas: np.ndarray, massstab: float) -> np.ndarray:
    """Wie baue_auto_modelle.masken: nur was dunkler ist als seine Umgebung liegt tief;
    die Filterweiten wachsen mit der Aufloesung."""
    from scipy import ndimage
    lum = rgb.astype(np.float32).mean(axis=2) / 255.0
    umgebung = ndimage.gaussian_filter(lum, 7.0 * massstab)
    tiefe = np.clip((umgebung - lum) * 4.0, 0.0, 1.0)
    tiefe = np.maximum(tiefe, np.clip((0.22 - ndimage.gaussian_filter(lum, 2.0 * massstab)) * 3.0,
                                      0, 0.7))
    tiefe[glas > 0.5] = 0.0
    return 1.0 - ndimage.gaussian_filter(tiefe, 0.8 * massstab)


def main() -> None:
    neu = "--neu" in sys.argv
    namen = [a for a in sys.argv[1:] if not a.startswith("--")] or MODELLE
    for m in namen:
        for a in ANSICHTEN:
            verfeinere(m, a, neu)


if __name__ == "__main__":
    main()
