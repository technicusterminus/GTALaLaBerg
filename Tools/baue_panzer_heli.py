# Baut Panzer und Hubschrauber in Blender und legt sie als FBX in
# Tools/Modelle ab. Reine Kastengeometrie mit abgeschraegten Platten - kein
# gekauftes Modell, keine fremde Lizenz, und klein genug, dass es neben der
# Stadt nicht ins Gewicht faellt.
#
#   "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b -P Tools/baue_panzer_heli.py
#
# Gebaut wird in Metern entlang +X (Fahrtrichtung), +Z oben; der Export
# stellt das fuer Unreal richtig (axis_forward='X', axis_up='Z').
import math
import os

import bmesh
import bpy
import mathutils

ZIEL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Modelle")
os.makedirs(ZIEL, exist_ok=True)


def leere_szene():
    bpy.ops.wm.read_factory_settings(use_empty=True)


def neu(name):
    me = bpy.data.meshes.new(name)
    ob = bpy.data.objects.new(name, me)
    bpy.context.collection.objects.link(ob)
    return ob, me


def kasten(name, laenge, breite, hoehe, ort=(0, 0, 0)):
    ob, me = neu(name)
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    bmesh.ops.scale(bm, vec=(laenge, breite, hoehe), verts=bm.verts)
    bmesh.ops.translate(bm, vec=ort, verts=bm.verts)
    bm.to_mesh(me)
    bm.free()
    return ob


def zylinder(name, radius, laenge, ort=(0, 0, 0), achse="X", segmente=16):
    ob, me = neu(name)
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, cap_tris=False, segments=segmente,
                          radius1=radius, radius2=radius, depth=laenge)
    if achse == "X":
        bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0),
                         matrix=mathutils.Matrix.Rotation(math.radians(90), 3, "Y"))
    elif achse == "Y":
        bmesh.ops.rotate(bm, verts=bm.verts, cent=(0, 0, 0),
                         matrix=mathutils.Matrix.Rotation(math.radians(90), 3, "X"))
    bmesh.ops.translate(bm, vec=ort, verts=bm.verts)
    bm.to_mesh(me)
    bm.free()
    return ob


SEITEN = [(0, 1, 2, 3), (7, 6, 5, 4), (0, 3, 7, 4), (1, 5, 6, 2), (0, 4, 5, 1), (3, 2, 6, 7)]


def keil(name, punkte, ort=(0, 0, 0)):
    """Freie Form aus acht Punkten (links vorn/hinten unten/oben, dann
    rechts) - fuer geneigte Panzerplatten und den Heckausleger."""
    ob, me = neu(name)
    me.from_pydata(punkte, [], SEITEN)
    me.validate()
    me.update()
    ob.location = ort
    return ob


def vereine(name, objekte):
    bpy.ops.object.select_all(action="DESELECT")
    for ob in objekte:
        ob.select_set(True)
    bpy.context.view_layer.objects.active = objekte[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bpy.ops.object.join()
    ob = bpy.context.view_layer.objects.active
    ob.name = name
    ob.data.name = name
    return ob


def exportiere(name, ob):
    bpy.ops.object.select_all(action="DESELECT")
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    pfad = os.path.join(ZIEL, name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=pfad, use_selection=True, global_scale=1.0,
                             axis_forward="X", axis_up="Z", object_types={"MESH"},
                             mesh_smooth_type="FACE", use_mesh_modifiers=True)
    print("LALABERG_MODELL " + pfad)


def profil(name, unten, z_unten, oben, z_oben):
    """Koerper aus zwei Umrissen (Draufsicht) in zwei Hoehen: unten und oben
    duerfen verschieden sein, dadurch entstehen geneigte Waende wie an einem
    Turm. Beide Listen muessen gleich viele Punkte haben."""
    n = len(unten)
    punkte = [(x, y, z_unten) for x, y in unten] + [(x, y, z_oben) for x, y in oben]
    flaechen = []
    for i in range(n):
        j = (i + 1) % n
        flaechen.append((i, j, n + j, n + i))
    flaechen.append(tuple(range(n - 1, -1, -1)))          # Boden
    flaechen.append(tuple(range(n, 2 * n)))               # Deckel
    ob, me = neu(name)
    me.from_pydata(punkte, [], flaechen)
    me.validate()
    me.update()
    return ob


def kettenbahn(vorne, hinten):
    """Der Lauf der Kette um Leitrad (vorn) und Treibrad (hinten): die beiden
    aeusseren Tangenten und die Boegen um die Raeder. Jedes Rad als
    (x, z, radius). Liefert Geraden- und Bogenstuecke fuer kette()."""
    x0, z0, r0 = vorne
    x1, z1, r1 = hinten
    d = math.hypot(x1 - x0, z1 - z0)
    phi = math.atan2(z1 - z0, x1 - x0)
    alpha = math.acos(max(-1.0, min(1.0, (r0 - r1) / d)))
    a, b = phi + alpha, phi - alpha
    punkt = lambda cx, cz, r, w: (cx + math.cos(w) * r, cz + math.sin(w) * r)
    zwei_pi = 2 * math.pi
    return [
        ("gerade", punkt(x0, z0, r0, a), punkt(x1, z1, r1, a)),
        ("bogen", (x1, z1, r1), (a, b if b > a else b + zwei_pi)),
        ("gerade", punkt(x1, z1, r1, b), punkt(x0, z0, r0, b)),
        ("bogen", (x0, z0, r0), (b, a if a > b else a + zwei_pi)),
    ]


def kette(teile, y, breite, glied=0.17, dicke=0.055):
    """Legt einzelne Kettenglieder auf eine geschlossene Bahn aus Geraden und
    Boegen - erst damit sieht man, dass da eine Kette laeuft und nicht ein
    Brett klebt."""
    from math import atan2, cos, sin, pi
    # Bahn aus Stuetzpunkten: unten vorwaerts, um das Leitrad, oben zurueck,
    # um das Treibrad. Reihenfolge im Uhrzeigersinn (von +Y aus gesehen).
    stationen = []
    for art, a, b in teile:
        if art == "gerade":
            (x0, z0), (x1, z1) = a, b
            laenge = math.hypot(x1 - x0, z1 - z0)
            anzahl = max(1, int(round(laenge / glied)))
            for i in range(anzahl):
                t = (i + 0.5) / anzahl
                stationen.append((x0 + (x1 - x0) * t, z0 + (z1 - z0) * t, atan2(z1 - z0, x1 - x0)))
        else:
            (cx, cz, r), (w0, w1) = a, b
            spanne = (w1 - w0) % (2 * pi)
            anzahl = max(1, int(round(spanne * r / glied)))
            for i in range(anzahl):
                w = w0 + spanne * (i + 0.5) / anzahl
                stationen.append((cx + cos(w) * r, cz + sin(w) * r, w + pi / 2))
    glieder = []
    for x, z, winkel in stationen:
        g = kasten("Glied", glied * 0.94, breite, dicke, (0, 0, 0))
        g.rotation_euler = (0, -winkel, 0)
        g.location = (x, y, z)
        glieder.append(g)
    return glieder


def kanten_brechen(ob, breite=0.012, segmente=2):
    """Eine leichte Fase auf alle Kanten: im Spiel faengt sie das Licht und
    nimmt den Teilen das Pappkarton-Aussehen."""
    bpy.context.view_layer.objects.active = ob
    mod = ob.modifiers.new("Fase", "BEVEL")
    mod.width = breite
    mod.segments = segmente
    mod.limit_method = "ANGLE"
    mod.angle_limit = math.radians(35)
    mod.harden_normals = False
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return ob


# ----------------------------------------------------------------- Panzer
# Vorbild Leopard 2A6 nach den offenen Datenblaettern: Wanne 7,7 m, ueber die
# Schuerzen 3,7 m, Turmdach auf 2,48 m, Rohr L/55 mit 5,3 m Ueberstand,
# sieben Doppellaufraeder je Seite, Treibrad hinten, Leitrad vorn, vier
# Stuetzrollen, Kettenbreite 63,5 cm.
#
# Die Kette besteht aus einzelnen Gliedern, die um Leit- und Treibrad
# herumlaufen, die Laufraeder sind Doppelraeder mit Nabe, und alle Kanten
# bekommen zum Schluss eine Fase - ohne das war es ein Brett auf zwei Balken.
def panzer():
    leere_szene()

    # --------------------------------------------------------- Wannenkorper
    # Umriss in der Draufsicht, unten schmaler als oben: die Wanne steht
    # nicht senkrecht, sondern zieht sich nach unten ein.
    wanne = [profil("Wannenkorper",
                    [(3.30, -1.35), (3.30, 1.35), (-3.45, 1.35), (-3.45, -1.35)], 0.28,
                    [(3.45, -1.46), (3.45, 1.46), (-3.50, 1.46), (-3.50, -1.46)], 1.22)]
    # Bugplatte: beim Leopard liegt sie sehr flach (rund 82 Grad zur
    # Senkrechten) und reicht vom Kettenboden bis ans Dach.
    wanne.append(profil("Bugplatte",
                        [(3.45, -1.46), (3.45, 1.46), (2.95, 1.46), (2.95, -1.46)], 0.30,
                        [(1.35, -1.46), (1.35, 1.46), (0.95, 1.46), (0.95, -1.46)], 1.24))
    # Wannendach, Motordeck und die Jalousien der Kuehlanlage.
    wanne.append(kasten("Dach", 6.95, 2.92, 0.08, (-0.02, 0, 1.26)))
    wanne.append(kasten("Motordeck", 2.60, 2.80, 0.16, (-2.15, 0, 1.34)))
    for i in range(5):
        wanne.append(kasten("Jalousie", 0.34, 2.40, 0.10, (-1.15 - i * 0.48, 0, 1.45)))
    for seite in (-1, 1):
        wanne.append(zylinder("Luefterdeckel", 0.46, 0.12, (-3.05, seite * 0.78, 1.46),
                              achse="Z", segmente=24))
    # Fahrerluk (der Fahrer sitzt rechts vorn) mit drei Winkelspiegeln.
    wanne.append(zylinder("Fahrerluk", 0.36, 0.09, (1.55, 0.72, 1.33), achse="Z", segmente=20))
    for i in range(3):
        wanne.append(kasten("Winkelspiegel", 0.14, 0.24, 0.10, (1.92, 0.72 + (i - 1) * 0.34, 1.33)))
    # Heckplatte mit Auspuffgittern und Abschleppkupplungen.
    wanne.append(profil("Heckplatte",
                        [(-3.50, -1.46), (-3.50, 1.46), (-3.62, 1.46), (-3.62, -1.46)], 0.30,
                        [(-3.50, -1.46), (-3.50, 1.46), (-3.70, 1.46), (-3.70, -1.46)], 1.24))
    for seite in (-1, 1):
        wanne.append(kasten("Auspuffgitter", 0.12, 0.62, 0.42, (-3.66, seite * 0.85, 0.92)))
        wanne.append(zylinder("Kupplung", 0.09, 0.22, (-3.70, seite * 0.45, 0.55), achse="X", segmente=12))
        # Werkzeugkaesten auf dem Kotfluegel, Scheinwerfer vorn.
        wanne.append(kasten("Werkzeugkasten", 1.30, 0.34, 0.34, (-0.60, seite * 1.62, 1.44)))
        wanne.append(kasten("Staubox", 0.80, 0.30, 0.28, (1.20, seite * 1.62, 1.41)))
        wanne.append(zylinder("Scheinwerfer", 0.16, 0.14, (3.42, seite * 1.15, 1.34), achse="X", segmente=16))
    rumpf = vereine("Wanne", wanne)
    kanten_brechen(rumpf, 0.018)

    # ------------------------------------------------------------ Laufwerk
    laufwerk = []
    LEITRAD = (3.05, 0.46, 0.33)        # vorn
    TREIBRAD = (-3.22, 0.86, 0.34)      # hinten, hoeher gesetzt
    for seite in (-1, 1):
        y = seite * 1.47
        laufwerk += kette(kettenbahn(LEITRAD, TREIBRAD), y, 0.635)
        # Sieben Doppellaufraeder: je zwei Scheiben mit Nabe dazwischen.
        for i in range(7):
            x = -2.55 + i * 0.86
            for versatz in (-0.16, 0.16):
                laufwerk.append(zylinder("Laufradscheibe", 0.37, 0.22, (x, y + versatz, 0.40),
                                         achse="Y", segmente=24))
            laufwerk.append(zylinder("Laufradnabe", 0.15, 0.36, (x, y, 0.40), achse="Y", segmente=16))
            # Schwingarm zur Drehstabfederung.
            laufwerk.append(kasten("Schwingarm", 0.55, 0.14, 0.16, (x + 0.2, y - seite * 0.34, 0.62)))
        # Treibrad mit Zahnkranz, Leitrad glatt.
        laufwerk.append(zylinder("Treibrad", TREIBRAD[2], 0.52, (TREIBRAD[0], y, TREIBRAD[1]),
                                 achse="Y", segmente=22))
        for i in range(11):
            w = i / 11.0 * 2 * math.pi
            # Am Ursprung bauen, dann drehen und erst danach setzen: ein
            # Kasten, dessen Netz schon versetzt liegt, schwingt beim Drehen
            # des Objekts auf weitem Bogen davon - im ersten Bild flogen die
            # Zaehne und Sichtbloecke frei um den Panzer herum.
            zahn = kasten("Zahn", 0.12, 0.5, 0.12)
            zahn.rotation_euler = (0, -w, 0)
            zahn.location = (TREIBRAD[0] + math.cos(w) * 0.36, y, TREIBRAD[1] + math.sin(w) * 0.36)
            laufwerk.append(zahn)
        laufwerk.append(zylinder("Leitrad", LEITRAD[2], 0.5, (LEITRAD[0], y, LEITRAD[1]),
                                 achse="Y", segmente=22))
        for i in range(4):
            laufwerk.append(zylinder("Stuetzrolle", 0.12, 0.30, (-1.85 + i * 1.25, y - seite * 0.18, 1.05),
                                     achse="Y", segmente=14))
        # Schuerzen: vorn die schweren Panzerplatten, dahinter die duennen.
        laufwerk.append(kasten("Panzerschuerze", 2.70, 0.16, 0.86, (1.95, seite * 1.84, 0.80)))
        for i in range(4):
            laufwerk.append(kasten("Schuerze", 0.90, 0.07, 0.70, (0.15 - i * 0.95, seite * 1.82, 0.72)))
        laufwerk.append(kasten("Kotfluegel", 7.00, 0.42, 0.08, (-0.1, seite * 1.66, 1.26)))
    kette_ganz = vereine("Laufwerk", laufwerk)
    wanne_ganz = vereine("SM_Panzer_Wanne", [rumpf, kette_ganz])

    # ---------------------------------------------------------------- Turm
    # Ursprung = Drehkranz. Der Umriss ist die Pfeilform des Leopard 2:
    # breit hinten, vorn in zwei stark geneigte Keilflaechen zulaufend.
    UNTEN = [(2.30, -0.24), (2.30, 0.24), (1.15, 1.30), (-1.50, 1.42),
             (-1.50, -1.42), (1.15, -1.30)]
    OBEN = [(1.55, -0.20), (1.55, 0.20), (0.95, 1.12), (-1.50, 1.24),
            (-1.50, -1.24), (0.95, -1.12)]
    turm = [profil("Turmkorper", UNTEN, -0.44, OBEN, 0.46)]
    # Heckkiste: niedriger als der Korper, dahinter der offene Staukorb aus
    # Rohren statt eines geschlossenen Kastens.
    turm.append(profil("Heckkiste",
                       [(-1.50, -1.42), (-1.50, 1.42), (-2.70, 1.36), (-2.70, -1.36)], -0.40,
                       [(-1.50, -1.24), (-1.50, 1.24), (-2.70, 1.22), (-2.70, -1.22)], 0.42))
    for i in range(2):
        turm.append(kasten("Korbrahmen", 0.06, 2.50, 0.06, (-3.25, 0, -0.30 + i * 0.66)))
    for seite in (-1, 1):
        turm.append(kasten("Korbseite", 0.60, 0.06, 0.06, (-2.95, seite * 1.22, -0.30)))
        turm.append(kasten("Korbseite", 0.60, 0.06, 0.06, (-2.95, seite * 1.22, 0.36)))
        for i in range(4):
            turm.append(kasten("Korbstrebe", 0.05, 0.05, 0.70, (-3.25, seite * (1.22 - i * 0.4), 0.02)))
    # Kommandantenkuppel mit Rundumsichtblocks und Luke.
    turm.append(zylinder("Kuppel", 0.44, 0.20, (-0.30, -0.66, 0.56), achse="Z", segmente=24))
    for i in range(8):
        w = i / 8.0 * 2 * math.pi
        block = kasten("Sichtblock", 0.16, 0.10, 0.12)          # siehe Zahn
        block.rotation_euler = (0, 0, w)
        block.location = (-0.30 + math.cos(w) * 0.43, -0.66 + math.sin(w) * 0.43, 0.58)
        turm.append(block)
    turm.append(zylinder("Kuppelluke", 0.40, 0.08, (-0.30, -0.66, 0.70), achse="Z", segmente=24))
    # Rundblickperiskop PERI vor der Kuppel, Richtschuetzenoptik EMES rechts.
    turm.append(zylinder("Periskopfuss", 0.22, 0.26, (0.28, -0.66, 0.59), achse="Z", segmente=18))
    turm.append(kasten("Periskopkopf", 0.34, 0.40, 0.26, (0.28, -0.66, 0.78)))
    turm.append(kasten("EMES", 0.60, 0.46, 0.30, (0.62, 0.74, 0.58)))
    turm.append(kasten("EMESKlappe", 0.10, 0.42, 0.26, (0.92, 0.74, 0.58)))
    # Ladeschuetzenluke mit MG-Lafette.
    turm.append(zylinder("Ladeluke", 0.40, 0.08, (-0.55, 0.62, 0.50), achse="Z", segmente=22))
    turm.append(zylinder("MGLafette", 0.09, 0.26, (-0.15, 0.62, 0.58), achse="Z", segmente=10))
    turm.append(kasten("MGKasten", 0.30, 0.20, 0.18, (-0.15, 0.62, 0.72)))
    turm.append(zylinder("MG", 0.045, 0.75, (0.35, 0.62, 0.78), achse="X", segmente=10))
    # Nebelmittelwurfanlage: zwei Baenke zu vier Bechern, schraeg nach aussen.
    for seite in (-1, 1):
        for i in range(4):
            becher = zylinder("Nebelbecher", 0.055, 0.32, (0, 0, 0), achse="X", segmente=12)
            becher.rotation_euler = (0, math.radians(-25), math.radians(seite * 42))
            becher.location = (0.62 - i * 0.17, seite * 1.16, 0.40)
            turm.append(becher)
        turm.append(kasten("Nebelbank", 0.72, 0.12, 0.10, (0.36, seite * 1.14, 0.30)))
        # Antennen hinten auf dem Turmdach.
        turm.append(zylinder("Antennenfuss", 0.08, 0.14, (-1.35, seite * 1.05, 0.52), achse="Z", segmente=10))
        turm.append(zylinder("Antenne", 0.018, 1.80, (-1.35, seite * 1.05, 1.45), achse="Z", segmente=6))
    # Windmesser auf dem Heck, wie beim Original hinter dem Staukorb.
    turm.append(zylinder("Windmastfuss", 0.05, 0.50, (-2.55, 0, 0.70), achse="Z", segmente=8))
    turm.append(kasten("Windmesser", 0.22, 0.22, 0.10, (-2.55, 0, 1.00)))
    # Blende, Waermeschutzhuelle, Rohr, Rauchabsauger und Muendung.
    turm.append(profil("Blende",
                       [(2.30, -0.46), (2.30, 0.46), (1.70, 0.62), (1.70, -0.62)], -0.34,
                       [(2.30, -0.40), (2.30, 0.40), (1.70, 0.54), (1.70, -0.54)], 0.30))
    turm.append(zylinder("Huelle", 0.155, 2.60, (3.55, 0, -0.02), achse="X", segmente=20))
    turm.append(zylinder("Rauchabsauger", 0.21, 0.70, (4.40, 0, -0.02), achse="X", segmente=20))
    turm.append(zylinder("Rohr", 0.105, 3.40, (6.10, 0, -0.02), achse="X", segmente=20))
    turm.append(zylinder("Muendung", 0.135, 0.30, (7.65, 0, -0.02), achse="X", segmente=20))
    turm_ganz = vereine("SM_Panzer_Turm", turm)
    kanten_brechen(turm_ganz, 0.012)

    exportiere("SM_Panzer_Wanne", wanne_ganz)
    exportiere("SM_Panzer_Turm", turm_ganz)


# ------------------------------------------------------------ Hubschrauber
# Vorbild ist der Rettungshubschrauber, wie er auf jedem Klinikumsdach steht
# (Zelle rund 10 m lang, Rotor 10 m, Hoehe 3,5 m): runde Kabine mit weit
# heruntergezogener Kanzel, Triebwerksdeck darueber, schlanker Heckausleger
# und der ummantelte Heckrotor im Ringkanal - die Bauform, an der man einen
# solchen Hubschrauber auf hundert Meter erkennt.
#
# Der erste Versuch war ein Kasten mit Ausleger und vier Brettern; hier
# entstehen Kabine und Ausleger aus Umrissen mit geneigten Waenden, der
# Ringkanal aus einer echten Kreisscheibe mit Loch.
def scheibe_mit_loch(name, r_aussen, r_innen, dicke, ort=(0, 0, 0), segmente=28):
    """Kreisring als Koerper - fuer den Ringkanal des Heckrotors. Achse Y."""
    ob, me = neu(name)
    punkte, flaechen = [], []
    for seite, y in ((0, -dicke / 2), (1, dicke / 2)):
        for i in range(segmente):
            w = i / segmente * 2 * math.pi
            punkte.append((math.cos(w) * r_aussen, y, math.sin(w) * r_aussen))
            punkte.append((math.cos(w) * r_innen, y, math.sin(w) * r_innen))
    for i in range(segmente):
        j = (i + 1) % segmente
        a0, a1 = 2 * i, 2 * i + 1
        b0, b1 = 2 * j, 2 * j + 1
        c0, c1 = a0 + 2 * segmente, a1 + 2 * segmente
        d0, d1 = b0 + 2 * segmente, b1 + 2 * segmente
        flaechen.append((a0, b0, b1, a1))          # vorn
        flaechen.append((c1, d1, d0, c0))          # hinten
        flaechen.append((a0, c0, d0, b0))          # aussen
        flaechen.append((b1, d1, c1, a1))          # innen
    me.from_pydata([(p[0] + ort[0], p[1] + ort[1], p[2] + ort[2]) for p in punkte], [], flaechen)
    me.validate()
    me.update()
    return ob


def helikopter():
    leere_szene()

    # ------------------------------------------------------------- Zelle
    # Kabine: unten schmal, in Fensterhoehe am breitesten, zum Dach wieder
    # eingezogen - deshalb drei Umrisse uebereinander.
    KABINE_UNTEN = [(2.05, -0.62), (2.45, 0.0), (2.05, 0.62), (0.2, 0.78),
                    (-1.5, 0.66), (-1.9, 0.0), (-1.5, -0.66), (0.2, -0.78)]
    KABINE_MITTE = [(2.35, -0.72), (2.80, 0.0), (2.35, 0.72), (0.2, 0.92),
                    (-1.6, 0.80), (-2.05, 0.0), (-1.6, -0.80), (0.2, -0.92)]
    KABINE_OBEN = [(1.45, -0.55), (1.75, 0.0), (1.45, 0.55), (0.2, 0.74),
                   (-1.5, 0.66), (-1.85, 0.0), (-1.5, -0.66), (0.2, -0.74)]
    zelle = [profil("KabineUnten", KABINE_UNTEN, 0.62, KABINE_MITTE, 1.52),
             profil("KabineOben", KABINE_MITTE, 1.52, KABINE_OBEN, 2.18)]
    # Kanzel: die weit heruntergezogene Frontscheibe als eigene, flache
    # Platte ueber der Kabinenspitze.
    zelle.append(profil("Kanzel",
                        [(2.45, -0.62), (2.72, 0.0), (2.45, 0.62), (1.6, 0.66),
                         (1.3, 0.0), (1.6, -0.66)], 0.75,
                        [(1.9, -0.52), (2.10, 0.0), (1.9, 0.52), (1.35, 0.56),
                         (1.15, 0.0), (1.35, -0.56)], 1.75))
    # Triebwerksdeck mit zwei Auspuffstutzen, dahinter der Rotormast.
    zelle.append(profil("Triebwerksdeck",
                        [(0.95, -0.72), (0.95, 0.72), (-1.45, 0.66), (-1.45, -0.66)], 2.12,
                        [(0.75, -0.56), (0.75, 0.56), (-1.35, 0.52), (-1.35, -0.52)], 2.46))
    for seite in (-1, 1):
        zelle.append(zylinder("Auspuff", 0.14, 0.5, (-1.35, seite * 0.42, 2.34), achse="X", segmente=14))
    zelle.append(zylinder("Mast", 0.17, 0.5, (0.35, 0, 2.42), achse="Z", segmente=16))

    # Heckausleger: langer, sich verjuengender Koerper bis zum Ringkanal.
    zelle.append(profil("Heckausleger",
                        [(-1.45, -0.42), (-1.45, 0.42), (-5.15, 0.24), (-5.15, -0.24)], 1.62,
                        [(-1.45, -0.40), (-1.45, 0.40), (-5.15, 0.22), (-5.15, -0.22)], 2.12))
    # Ringkanal (Fenestron) mit Blaettern darin - das auffaelligste Merkmal.
    zelle.append(scheibe_mit_loch("Ringkanal", 0.78, 0.60, 0.34, (-5.55, 0.0, 1.88)))
    zelle.append(zylinder("Kanalnabe", 0.14, 0.36, (-5.55, 0, 1.88), achse="Y", segmente=14))
    # Seitenleitwerk ueber dem Kanal, Hoehenflosse mit Endscheiben davor.
    zelle.append(profil("Seitenflosse",
                        [(-5.0, -0.09), (-5.0, 0.09), (-5.95, 0.07), (-5.95, -0.07)], 2.40,
                        [(-5.25, -0.07), (-5.25, 0.07), (-5.95, 0.06), (-5.95, -0.06)], 3.15))
    zelle.append(kasten("Hoehenflosse", 0.62, 2.20, 0.09, (-4.35, 0, 1.95)))
    for seite in (-1, 1):
        zelle.append(kasten("Endscheibe", 0.55, 0.07, 0.62, (-4.35, seite * 1.06, 2.22)))
    # Kufen: zwei Laengsrohre auf zwei Querbuegeln.
    for seite in (-1, 1):
        zelle.append(zylinder("Kufe", 0.075, 3.30, (0.15, seite * 1.05, 0.26), achse="X", segmente=12))
        zelle.append(zylinder("Kufenspitze", 0.075, 0.5, (1.85, seite * 1.05, 0.38), achse="X", segmente=12))
    for x in (1.05, -0.85):
        zelle.append(zylinder("Querbuegel", 0.085, 2.10, (x, 0, 0.55), achse="Y", segmente=12))
        for seite in (-1, 1):
            strebe = zylinder("Strebe", 0.075, 0.42, (0, 0, 0), achse="Z", segmente=10)
            strebe.rotation_euler = (math.radians(seite * 22), 0, 0)
            strebe.location = (x, seite * 0.92, 0.38)
            zelle.append(strebe)
    # Landescheinwerfer unter der Nase, Antenne auf dem Ausleger.
    zelle.append(zylinder("Landelicht", 0.13, 0.12, (2.25, 0, 0.72), achse="Z", segmente=14))
    zelle.append(kasten("Antenne", 0.5, 0.05, 0.14, (-3.2, 0, 2.18)))
    rumpf_ganz = vereine("SM_Heli_Rumpf", zelle)
    kanten_brechen(rumpf_ganz, 0.015)

    # ------------------------------------------------------------- Rotor
    # Nabe mit Taumelscheibe und vier leicht verjuengten Blaettern.
    rotor = [zylinder("Nabe", 0.26, 0.30, (0, 0, 0), achse="Z", segmente=20),
             zylinder("Taumelscheibe", 0.34, 0.08, (0, 0, -0.16), achse="Z", segmente=20)]
    for i in range(4):
        blatt = profil("Blatt",
                       [(0.30, -0.16), (5.05, -0.11), (5.05, 0.11), (0.30, 0.16)], -0.035,
                       [(0.30, -0.16), (5.05, -0.11), (5.05, 0.11), (0.30, 0.16)], 0.035)
        # Das Netz liegt schon aussen am Blattansatz, gedreht wird um den
        # Ursprung - so wandert der Halter mit dem Blatt mit. Ein .location
        # wuerde nicht mitgedreht und liesse alle vier an derselben Stelle.
        halter = kasten("Blatthalter", 0.34, 0.14, 0.12, (0.28, 0, 0))
        blatt.rotation_euler = (0, 0, math.radians(90 * i))
        halter.rotation_euler = (0, 0, math.radians(90 * i))
        rotor.append(blatt)
        rotor.append(halter)
    rotor_ganz = vereine("SM_Heli_Rotor", rotor)
    kanten_brechen(rotor_ganz, 0.008)

    # Heckrotor im Kanal: acht kurze Blaetter, Achse quer.
    heck = [zylinder("Hecknabe", 0.12, 0.26, (0, 0, 0), achse="Y", segmente=14)]
    for i in range(8):
        blatt = kasten("Heckblatt", 0.50, 0.05, 0.11, (0.30, 0, 0))
        blatt.rotation_euler = (math.radians(45 * i), 0, 0)
        heck.append(blatt)
    heckrotor = vereine("SM_Heli_Heckrotor", heck)

    exportiere("SM_Heli_Rumpf", rumpf_ganz)
    exportiere("SM_Heli_Rotor", rotor_ganz)
    exportiere("SM_Heli_Heckrotor", heckrotor)


panzer()
helikopter()
print("LALABERG_MODELLE fertig")
