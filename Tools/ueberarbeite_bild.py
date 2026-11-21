# Schickt ein Bild an ein Bildmodell auf OpenRouter und speichert das Ergebnis.
#
#   python Tools/ueberarbeite_bild.py <Modell> <Eingabe.png> <Ausgabe.png> "<Auftrag>"
#
# Der Schluessel kommt vom vorgeschalteten Proxy (Platzhalter) oder aus der Umgebung.
import base64
import json
import os
import sys
import urllib.request
modell, eingabe, ausgabe, auftrag = sys.argv[1:5]
daten = base64.b64encode(open(eingabe, "rb").read()).decode()
body = {"model": modell, "modalities": ["image", "text"],
        "messages": [{"role": "user", "content": [
            {"type": "text", "text": auftrag},
            {"type": "image_url", "image_url": {"url": "data:image/png;base64," + daten}}]}]}
req = urllib.request.Request("https://openrouter.ai/api/v1/chat/completions", data=json.dumps(body).encode(),
    headers={"Content-Type": "application/json", "Authorization": "Bearer " + os.environ.get("OPENROUTER_API_KEY", "proxy-injected")})
try:
    d = json.load(urllib.request.urlopen(req, timeout=300))
except Exception as e:
    print("ERR", e, e.read()[:400] if hasattr(e, "read") else ""); sys.exit(1)
m = d["choices"][0]["message"]
print("TEXT", (m.get("content") or "")[:300])
n = 0
for b in m.get("images", []) or []:
    url = b["image_url"]["url"]
    open(ausgabe, "wb").write(base64.b64decode(url.split(",", 1)[1])); n += 1
print("BILDER", n, "usage", d.get("usage"))
