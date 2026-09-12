# Importiert die vier lizenzierten Waffen-GLBs (Content/SourceData/Waffen,
# siehe LIZENZ.md - CC0, Quaternius) als Unreal-StaticMesh-Assets nach
# /Game/Art/Waffen. Gleiches Muster wie Tools/import_carconcept.py fuer das
# Fahrzeugmodell: glTF-Import ueber Interchange, kein manuelles Ausrichten
# hier - das uebernimmt LaLaBergWaffe.cpp per Transform am Component.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/importiere_waffen.py"
import os
import unreal

projekt = unreal.Paths.project_dir()
quelle = os.path.join(projekt, "Content", "SourceData", "Waffen")
ziel = "/Game/Art/Waffen"
werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()

DATEIEN = {
    "Pistol.glb": "SM_Pistole",
    "Smg.glb": "SM_Maschine",
    "Shotgun.glb": "SM_Schrotflinte",
    "RocketLauncher.glb": "SM_Raketenwerfer",
}

importiert = []
for datei, name in DATEIEN.items():
    pfad_quelle = os.path.join(quelle, datei)
    if not os.path.isfile(pfad_quelle):
        unreal.log_warning("LALABERG_WAFFENMODELL fehlt: " + pfad_quelle)
        continue
    aufgabe = unreal.AssetImportTask()
    aufgabe.filename = pfad_quelle
    aufgabe.destination_path = ziel
    aufgabe.destination_name = name
    aufgabe.automated = True
    aufgabe.replace_existing = True
    aufgabe.save = True
    aufgabe.replace_existing_settings = False
    werkzeuge.import_asset_tasks([aufgabe])
    pfade = aufgabe.get_editor_property("imported_object_paths")
    if not pfade:
        unreal.log_warning("LALABERG_WAFFENMODELL kein Asset aus " + datei)
        continue
    for p in pfade:
        objekt = unreal.load_asset(p)
        if objekt:
            unreal.EditorAssetLibrary.save_loaded_asset(objekt, only_if_is_dirty=False)
    importiert.append(name)
    # Abmessung protokollieren - zum Abgleichen der Groesse/Ausrichtung im
    # C++-Code, ohne den Editor dafuer oeffnen zu muessen.
    mesh_pfad = "%s/%s" % (ziel, name)
    mesh = unreal.load_asset(mesh_pfad)
    if isinstance(mesh, unreal.StaticMesh):
        box = mesh.get_editor_property("bounds").box_extent
        unreal.log("LALABERG_WAFFENMODELL_MASSE %s halb=(%.1f,%.1f,%.1f)" % (name, box.x, box.y, box.z))

unreal.log("LALABERG_WAFFENMODELLE " + " ".join(importiert))
