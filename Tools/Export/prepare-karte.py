# Stadtplan fuer Minikarte und Vollkarte im Spiel (siehe LaLaBergHUD::Karte):
# ein einmal vorgerendertes Bild statt tausender Linien und Gebaeudeumrisse je
# Bild. Dieselben Quelldaten und Farben wie der Stadtplan im Webprojekt
# (js/hud.js buildMap), 1 Pixel je Meter, Norden oben.
#
#   python prepare-karte.py
#
# Schreibt Content/SourceData/Karte/karte.png, karte-klein.png und karte.json (wo das Bild in
# Unreal-Zentimetern liegt). LALABERG_QUELLE wie bei prepare-stadt.cjs.
import json, os, sys
from PIL import Image, ImageDraw

HIER = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HIER, '..', '..'))
QUELLE = os.environ.get('LALABERG_QUELLE') or os.path.abspath(os.path.join(REPO, '..', 'GTA LaLaBerg'))

text = open(os.path.join(QUELLE, 'data', 'citydata.js'), encoding='utf-8').read()
city = json.loads(text[text.index('{'):text.rindex('}') + 1])

# Wie prepare-verkehr.cjs: Unreal-X = x - 605, Unreal-Y = z + 100, in cm.
# Unreal-X zeigt nach Osten, Unreal-Y nach Sueden - Bild-x und Bild-y also
# direkt, Norden oben.
ORIGIN = (605, -100)
PPM = 1.0      # Pixel je Meter im fertigen Bild
SS = 2         # Ueberabtastung gegen Treppenkanten
b = city['bounds']
x0, z0 = b['x0'], b['z0']
W, H = int((b['x1'] - x0) * PPM), int((b['z1'] - z0) * PPM)

COL = {
    'ground': '#242b22', 'park': '#32432b', 'wood': '#283823', 'water': '#1a3d5a',
    'road0': '#8b8578', 'road1': '#77716a', 'road2': '#635f58', 'road3': '#4f4c47',
    'ped': '#8a7d69', 'build': '#494640', 'buildOld': '#6a5946', 'wall': '#8e8264',
    'gewerbe': '#2a2a28', 'rest': '#2c3226',
}

bild = Image.new('RGB', (W * SS, H * SS), COL['ground'])
d = ImageDraw.Draw(bild)
f = PPM * SS
pkt = lambda p: [((p[i] - x0) * f, (p[i + 1] - z0) * f) for i in range(0, len(p) - 1, 2)]

def flaeche(p, farbe):
    q = pkt(p)
    if len(q) >= 3: d.polygon(q, fill=farbe)

def linie(p, breite, farbe):
    q = pkt(p)
    if len(q) < 2: return
    w = max(1, round(breite * f))
    d.line(q, fill=farbe, width=w, joint='curve')
    # Runde Enden wie lineCap 'round' im Webprojekt - sonst klaffen Luecken
    # an jeder Kreuzung, an der zwei Strassenzuege aufeinanderstossen.
    r = w / 2
    for (x, y) in (q[0], q[-1]): d.ellipse((x - r, y - r, x + r, y + r), fill=farbe)

for a in city['areas']:
    t = a['t']
    farbe = COL['wood'] if t == 1 else COL['park'] if t in (0, 2, 4, 7) else COL['gewerbe'] if t == 6 else COL['rest']
    flaeche(a['p'], farbe)
for w in city['water']: flaeche(w['p'], COL['water'])
for r in city['rivers']:
    if r.get('w', 0) >= 4: linie(r['p'], r['w'], COL['water'])
for pl in city['plazas']: flaeche(pl['p'], COL['ped'])
for cls in range(5, -1, -1):
    farbe = [COL['road0'], COL['road1'], COL['road2'], COL['road3'], COL['ped'], COL['ped']][cls]
    for r in city['roads']:
        if r['c'] == cls: linie(r['p'], max(1.6 if cls >= 4 else 3, r['w'] * 0.95), farbe)
for g in city['buildings']: flaeche(g['p'], COL['buildOld'] if g.get('a') else COL['build'])
for m in city['walls']:
    if m.get('t', 0) == 0: linie(m['p'], 3, COL['wall'])

bild = bild.resize((W, H), Image.LANCZOS)
ziel = os.path.join(REPO, 'Content', 'SourceData', 'Karte')
os.makedirs(ziel, exist_ok=True)
bild.save(os.path.join(ziel, 'karte.png'), optimize=True)
# Fuer die Vollkarte: ein Drittel so gross. Die Textur zur Laufzeit hat keine
# Mip-Stufen - auf den Bildschirm verkleinert wuerde das grosse Bild flimmern.
bild.resize((W // 3, H // 3), Image.LANCZOS).save(os.path.join(ziel, 'karte-klein.png'), optimize=True)
info = {'schema': 1, 'x0': round((x0 - ORIGIN[0]) * 100), 'y0': round((z0 - ORIGIN[1]) * 100),
        'cmProPixel': round(100 / PPM, 3), 'breite': W, 'hoehe': H}
json.dump(info, open(os.path.join(ziel, 'karte.json'), 'w'))
print(json.dumps({**info, 'bytes': os.path.getsize(os.path.join(ziel, 'karte.png'))}))
