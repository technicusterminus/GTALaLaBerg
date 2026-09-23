# Rendert Vorschaubilder der in Blender gebauten Sonderfahrzeuge - damit man
# sie ansehen kann, ohne das Spiel zu starten.
#
#   "C:/Program Files/Blender Foundation/Blender 5.1/blender.exe" -b -P Tools/rendere_modelle.py
#
# Legt Tools/Modelle/vorschau_panzer.png und vorschau_heli.png an.
import math
import os

import bpy
import mathutils

ORDNER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Modelle")


# Turm und Rotor haben ihren Ursprung im Drehpunkt; im Spiel setzt sie das
# Fahrzeug auf ihre Hoehe. Fuer das Bild dieselbe Hoehe von Hand.
HOEHE = {"SM_Panzer_Turm": 1.90, "SM_Heli_Rotor": 3.15, "SM_Heli_Heckrotor": 2.90}
VERSATZ_X = {"SM_Heli_Rotor": 0.2, "SM_Heli_Heckrotor": -6.4}


def szene(dateien, name, abstand, hoehe, ziel_z):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    for datei in dateien:
        bpy.ops.import_scene.fbx(filepath=os.path.join(ORDNER, datei + ".fbx"))
        for ob in bpy.context.selected_objects:
            ob.location = (VERSATZ_X.get(datei, 0.0), 0.0, HOEHE.get(datei, 0.0))
    # Mattes Olivgruen wie im Spiel.
    stoff = bpy.data.materials.new("Vorschau")
    stoff.use_nodes = True
    bsdf = stoff.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (0.13, 0.15, 0.09, 1)
    bsdf.inputs["Roughness"].default_value = 0.85
    for ob in bpy.context.scene.objects:
        if ob.type == "MESH":
            ob.data.materials.clear()
            ob.data.materials.append(stoff)

    sonne = bpy.data.objects.new("Sonne", bpy.data.lights.new("Sonne", "SUN"))
    sonne.data.energy = 4.0
    sonne.rotation_euler = (math.radians(52), 0, math.radians(35))
    bpy.context.collection.objects.link(sonne)

    kamera = bpy.data.objects.new("Kamera", bpy.data.cameras.new("Kamera"))
    kamera.location = (abstand, -abstand * 0.75, hoehe)
    richtung = mathutils.Vector((0, 0, ziel_z)) - kamera.location
    kamera.rotation_euler = richtung.to_track_quat("-Z", "Y").to_euler()
    bpy.context.collection.objects.link(kamera)
    bpy.context.scene.camera = kamera

    welt = bpy.data.worlds.new("Welt")
    welt.use_nodes = True
    welt.node_tree.nodes["Background"].inputs[0].default_value = (0.5, 0.62, 0.78, 1)
    bpy.context.scene.world = welt

    bpy.context.scene.render.engine = "BLENDER_EEVEE"
    bpy.context.scene.render.resolution_x = 1280
    bpy.context.scene.render.resolution_y = 800
    bpy.context.scene.render.filepath = os.path.join(ORDNER, name)
    bpy.ops.render.render(write_still=True)
    print("LALABERG_VORSCHAU " + bpy.context.scene.render.filepath)


szene(["SM_Panzer_Wanne", "SM_Panzer_Turm"], "vorschau_panzer.png", 16.0, 6.5, 1.6)
szene(["SM_Heli_Rumpf", "SM_Heli_Rotor", "SM_Heli_Heckrotor"], "vorschau_heli.png", 17.0, 6.5, 2.0)
