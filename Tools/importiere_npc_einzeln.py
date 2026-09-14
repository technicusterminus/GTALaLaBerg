# Importiert zusaetzliche NPC-Figuren (CC0, Quaternius, "Ultimate Modular
# Men Pack" - siehe Content/SourceData/People/LIZENZ.md) als eigenstaendige
# Skeletal Meshes MIT eingebauter Animation. Anders als Farmer (siehe
# importiere_npc.py, vier modulare Teile + externe Animations.fbx) bringt
# jede dieser "Individual Character"-FBX ihr eigenes, vollstaendiges Mesh
# UND alle 24 Animationen in einer einzigen Datei mit - kein modularer
# Zusammenbau, keine Retargeting-Klippe (im Test bestaetigt: 24 AnimSequences
# nach dem Import einer einzelnen FBX-Datei, anders als beim fruehesten
# Test mit einer anderen Downloadquelle, siehe importiere_npc.py-Kommentar).
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<absoluter Pfad>/Tools/importiere_npc_einzeln.py"
import os
import unreal

PROJEKT = unreal.Paths.project_dir()
QUELLE = os.path.join(PROJEKT, "Content", "SourceData", "People")
ZIEL = "/Game/Art/People"

# Jede Figur: eigener Unterordner in SourceData/People, eigene FBX-Datei.
FIGUREN = {
    "Casual": "Casual.fbx",
    "Worker": "Worker.fbx",
}

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()

for figur, datei_name in FIGUREN.items():
    datei = os.path.join(QUELLE, figur, datei_name)
    if not os.path.isfile(datei):
        unreal.log_warning("LALABERG_NPC_EINZELN fehlt: " + datei)
        continue

    optionen = unreal.FbxImportUI()
    optionen.import_mesh = True
    optionen.import_as_skeletal = True
    optionen.import_animations = True
    optionen.import_materials = True
    optionen.import_textures = True
    optionen.create_physics_asset = False
    optionen.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH
    skel_data = optionen.skeletal_mesh_import_data
    skel_data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS

    ziel_ordner = "%s/%s" % (ZIEL, figur)
    ziel_name = "SK_" + figur
    ziel_pfad = "%s/%s" % (ziel_ordner, ziel_name)
    if unreal.EditorAssetLibrary.does_asset_exist(ziel_pfad):
        unreal.EditorAssetLibrary.delete_asset(ziel_pfad)

    auftrag = unreal.AssetImportTask()
    auftrag.filename = datei
    auftrag.destination_path = ziel_ordner
    auftrag.destination_name = ziel_name
    auftrag.automated = True
    auftrag.replace_existing = True
    auftrag.save = True
    auftrag.options = optionen
    werkzeuge.import_asset_tasks([auftrag])
    ergebnisse = list(auftrag.get_editor_property("imported_object_paths"))
    for pfad in ergebnisse:
        objekt = unreal.load_asset(pfad)
        if objekt:
            unreal.EditorAssetLibrary.save_loaded_asset(objekt, only_if_is_dirty=False)

    anims = [p for p in ergebnisse if unreal.load_asset(p) and "AnimSequence" in unreal.load_asset(p).get_class().get_name()]
    unreal.log("LALABERG_NPC_EINZELN_FIGUR %s assets=%d animationen=%d" % (figur, len(ergebnisse), len(anims)))
