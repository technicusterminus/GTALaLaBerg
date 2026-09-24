# Baut die sitzende Figur, die in den Autos der Stadt am Steuer sitzt, und
# legt sie als FBX in Tools/Modelle ab.
#
#   "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b -P Tools/baue_insasse.py
#
# Warum ein eigenes, grobes Modell statt der Passantenfigur: in der Stadt
# fahren siebzig Autos, und jedes braucht ein bis zwei Insassen. Ein
# Skeletal Mesh je Sitzplatz waere hundertvierzig animierte Figuren, von
# denen man durch die Scheibe Schulter und Kopf sieht. Diese hier hat keine
# Knochen, sitzt still und wird ueber den Auto-Pool instanziert - ein
# Zeichenaufruf fuer alle Insassen der Stadt (siehe ALaLaBergAutoPool).
#
# Gebaut wird in Metern, Blickrichtung +X, +Z oben, Nullpunkt in der Mitte
# der Sitzflaeche - so laesst sich die Figur im Auto an der Sitzhoehe
# ausrichten, ohne die Beine zu suchen. Zwei Materialslots: 0 Kleidung
# (wird je Pool eingefaerbt), 1 Haut.
import math
import os

import bmesh
import bpy
import mathutils

ZIEL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Modelle")
os.makedirs(ZIEL, exist_ok=True)


def kasten(bm, laenge, breite, hoehe, ort, neigung=0.0):
    """Ein Quader, wahlweise um die Querachse geneigt (Grad, positiv = Lehne zurueck)."""
    teil = bmesh.new()
    bmesh.ops.create_cube(teil, size=1.0)
    bmesh.ops.scale(teil, vec=(laenge, breite, hoehe), verts=teil.verts)
    if neigung:
        bmesh.ops.transform(teil, matrix=mathutils.Matrix.Rotation(math.radians(neigung), 4, 'Y'),
                            verts=teil.verts)
    bmesh.ops.translate(teil, vec=ort, verts=teil.verts)
    # In den Sammel-bmesh uebernehmen
    me = bpy.data.meshes.new("temp")
    teil.to_mesh(me)
    teil.free()
    bm.from_mesh(me)
    bpy.data.meshes.remove(me)


def kugel(bm, radius, ort, unterteilung=2):
    teil = bmesh.new()
    bmesh.ops.create_icosphere(teil, subdivisions=unterteilung, radius=radius)
    bmesh.ops.translate(teil, vec=ort, verts=teil.verts)
    me = bpy.data.meshes.new("temp")
    teil.to_mesh(me)
    teil.free()
    bm.from_mesh(me)
    bpy.data.meshes.remove(me)


def baue():
    bpy.ops.wm.read_factory_settings(use_empty=True)

    # Kleidung: Oberschenkel, Unterschenkel, Rumpf, Arme.
    kleidung = bmesh.new()
    kasten(kleidung, 0.42, 0.36, 0.16, (0.19, 0.0, 0.08))                 # Oberschenkel
    kasten(kleidung, 0.16, 0.34, 0.40, (0.38, 0.0, -0.14))                # Unterschenkel
    kasten(kleidung, 0.30, 0.44, 0.60, (-0.02, 0.0, 0.44), neigung=9.0)   # Rumpf, leicht zurueckgelehnt
    kasten(kleidung, 0.34, 0.11, 0.12, (0.20, 0.21, 0.52), neigung=18.0)  # linker Arm ans Lenkrad
    kasten(kleidung, 0.34, 0.11, 0.12, (0.20, -0.21, 0.52), neigung=18.0)  # rechter Arm
    kasten(kleidung, 0.14, 0.14, 0.10, (0.03, 0.0, 0.73))                 # Hals/Kragen

    haut = bmesh.new()
    kugel(haut, 0.105, (0.01, 0.0, 0.85))                                  # Kopf

    me = bpy.data.meshes.new("SM_Insasse")
    ob = bpy.data.objects.new("SM_Insasse", me)
    bpy.context.collection.objects.link(ob)

    # Erst Kleidung, dann Haut in dasselbe Mesh - die Reihenfolge bestimmt,
    # welche Flaechen spaeter zu Materialslot 0 bzw. 1 gehoeren.
    sammel = bmesh.new()
    sammel.from_mesh(_als_mesh(kleidung, "kleidung"))
    kleidung_flaechen = len(sammel.faces)
    sammel.from_mesh(_als_mesh(haut, "haut"))
    sammel.to_mesh(me)
    sammel.free()

    ob.data.materials.append(bpy.data.materials.new("Kleidung"))
    ob.data.materials.append(bpy.data.materials.new("Haut"))
    for i, flaeche in enumerate(me.polygons):
        flaeche.material_index = 0 if i < kleidung_flaechen else 1

    ziel = os.path.join(ZIEL, "SM_Insasse.fbx")
    bpy.ops.object.select_all(action='DESELECT')
    ob.select_set(True)
    bpy.context.view_layer.objects.active = ob
    bpy.ops.export_scene.fbx(filepath=ziel, use_selection=True, apply_unit_scale=True,
                             axis_forward='X', axis_up='Z', mesh_smooth_type='FACE')
    print("LALABERG_INSASSE geschrieben: %s (%d Flaechen)" % (ziel, len(me.polygons)))


def _als_mesh(bm, name):
    me = bpy.data.meshes.new(name)
    bm.to_mesh(me)
    bm.free()
    return me


if __name__ == "__main__":
    baue()
