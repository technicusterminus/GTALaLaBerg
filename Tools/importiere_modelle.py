# Importiert die in Blender gebauten Sonderfahrzeuge (Tools/baue_panzer_heli.py)
# als Static Meshes nach /Game/Art/Vehicles/Sonder.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/importiere_modelle.py"
#
# Die FBX liegen in Metern vor; Unreal rechnet beim Import auf Zentimeter um
# (1 m = 100 Einheiten). Nanite bleibt aus: die Teile haben ein paar hundert
# Dreiecke, dafuer lohnt es nicht.
import os

import unreal

WURZEL = unreal.Paths.project_dir()
QUELLE = os.environ.get("LALABERG_NEUTRALE_QUELLE") or os.path.join(WURZEL, "Tools", "Modelle")
ORDNER = "/Game/Art/Vehicles/Sonder"

NAMEN = ["SM_Panzer_Wanne", "SM_Panzer_Turm", "SM_Heli_Rumpf", "SM_Heli_Rotor", "SM_Heli_Heckrotor",
         # Die sitzende Figur in den Autos der Stadt (Tools/baue_insasse.py).
         "SM_Insasse"]
# Ein erneuter Import loescht und baut jedes Modell neu. Wer nur eines
# geaendert hat, nennt es in LALABERG_MODELL_NUR (Namen mit Komma getrennt)
# und wartet nicht auf die anderen.
NUR = [n.strip() for n in os.environ.get("LALABERG_MODELL_NUR", "").split(",") if n.strip()]
if NUR:
    NAMEN = [n for n in NAMEN if n in NUR]

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()
importiert = []
for name in NAMEN:
    datei = os.path.join(QUELLE, name + ".fbx")
    if not os.path.isfile(datei):
        unreal.log_warning("LALABERG_MODELL fehlt: " + datei)
        continue
    pfad = "%s/%s" % (ORDNER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        unreal.EditorAssetLibrary.delete_asset(pfad)
    auftrag = unreal.AssetImportTask()
    auftrag.filename = datei
    auftrag.destination_path = ORDNER
    auftrag.destination_name = name
    auftrag.automated = True
    auftrag.replace_existing = True
    auftrag.save = True
    einstellungen = unreal.FbxImportUI()
    einstellungen.import_mesh = True
    einstellungen.import_as_skeletal = False
    einstellungen.import_materials = False
    einstellungen.import_textures = False
    einstellungen.static_mesh_import_data.set_editor_property("combine_meshes", True)
    einstellungen.static_mesh_import_data.set_editor_property("generate_lightmap_u_vs", True)
    auftrag.options = einstellungen
    werkzeuge.import_asset_tasks([auftrag])
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        importiert.append(name)

unreal.log("LALABERG_MODELLE " + " ".join(importiert))
