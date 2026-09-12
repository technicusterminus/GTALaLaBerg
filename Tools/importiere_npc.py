# Importiert eine lizenzierte NPC-Figur (CC0, Quaternius - siehe
# Content/SourceData/People/LIZENZ.md) als Skeletal Mesh MIT echter
# Animation. Zwei fruehere Anlaeufe sind hier dokumentiert, weil der naive
# Weg nicht funktioniert:
#
# 1. Die einzelnen Charakter-FBX aus "Individual Characters" (Casual.fbx
#    usw.) enthalten entgegen der "How To Use.txt" des Pakets KEINE
#    Animation - nur Mesh, eigenes Skelett, Material (im Test bestaetigt:
#    0 AnimSequences nach dem Import).
# 2. Animations.fbx (aus "Separate Skeletal Meshes and Animations", laut
#    Paket-Anleitung "empfohlen fuer Engines wie Unreal") gegen das
#    Skelett aus (1) importiert schlaegt fehl: "Knochenindex fuer Track
#    kann nicht abgerufen werden: Body" - Animations.fbx bringt ein
#    eigenes Skelett ("CharacterArmature") mit, dessen Knochennamen nicht
#    zum Skelett aus den Individual-Character-FBX passen.
#
# Der Weg, der funktioniert: Animations.fbx zuerst OHNE Zielskelett
# importieren (baut sein eigenes CharacterArmature-Skelett + alle
# Animationen), danach die MODULAREN Koerperteile (Body/Feet/Head/Legs)
# aus demselben "Separate Skeletal Meshes and Animations"-Ordner GEGEN
# dieses Skelett importieren - beide stammen aus derselben Quelle und
# teilen sich dieselbe Knochenstruktur.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/importiere_npc.py"
import os
import unreal

PROJEKT = unreal.Paths.project_dir()
QUELLE = os.path.join(PROJEKT, "Content", "SourceData", "People")
ZIEL = "/Game/Art/People"

# Modulare Teile je Figur - alle vier zusammen ergeben eine vollstaendige
# Figur (Body allein ist nur der Rumpf, nicht der ganze Charakter).
FIGUREN = {
    "Farmer": ["Farmer_Body", "Farmer_Feet", "Farmer_Head", "Farmer_Legs"],
}

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()


def importiere(datei, ziel_ordner, ziel_name, skelett=None, nur_animation=False):
    if not os.path.isfile(datei):
        unreal.log_warning("LALABERG_NPC fehlt: " + datei)
        return []
    optionen = unreal.FbxImportUI()
    optionen.import_mesh = not nur_animation
    optionen.import_as_skeletal = True
    optionen.import_animations = True
    optionen.import_materials = True
    optionen.import_textures = True
    optionen.create_physics_asset = False
    # Ohne diese Zeile bleibt mesh_type_to_import auf seinem Default (Static
    # Mesh) stehen, obwohl import_as_skeletal=True gesetzt ist - die
    # Interchange-Pipeline meldet dann "nichts zu importieren" statt eines
    # klaren Fehlers (im Test bestaetigt).
    optionen.mesh_type_to_import = (
        unreal.FBXImportType.FBXIT_ANIMATION if nur_animation else unreal.FBXImportType.FBXIT_SKELETAL_MESH
    )
    if skelett is not None:
        optionen.skeleton = skelett

    skel_data = optionen.skeletal_mesh_import_data
    skel_data.normal_import_method = unreal.FBXNormalImportMethod.FBXNIM_COMPUTE_NORMALS

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
    return ergebnisse


# Animations.fbx enthaelt NUR Skelett+Animationskurven, keine Mesh-Payload
# (im Test bestaetigt: "MeshPayload ... [0 0 0]") - ein Animations-Import
# braucht aber immer ein Zielskelett, das kann diese Datei folglich nicht
# selbst mitbringen. Umgekehrte Reihenfolge: zuerst der erste Koerperteil
# (bringt Mesh + eigenes Skelett mit), danach die restlichen Teile UND
# Animations.fbx gegen dieses eine Skelett.
skelett = None
for figur, teile in FIGUREN.items():
    ordner = "%s/%s" % (ZIEL, figur)
    angelegt = []
    for teil in teile:
        datei = os.path.join(QUELLE, teil + ".fbx")
        ergebnisse = importiere(datei, ordner, "SK_" + teil, skelett=skelett)
        angelegt += ergebnisse
        if skelett is None and ergebnisse:
            skelett_pfad = "%s/SK_%s_Skeleton" % (ordner, teil)
            skelett = unreal.load_asset(skelett_pfad)
            if skelett is None:
                unreal.log_warning("LALABERG_NPC kein Skelett unter " + skelett_pfad)
    unreal.log("LALABERG_NPC_FIGUR %s teile=%d assets=%s" % (figur, len(angelegt), " ".join(angelegt)))

if skelett is None:
    unreal.log_warning("LALABERG_NPC_ANIM kein Skelett gefunden - Abbruch")
else:
    anim_datei = os.path.join(QUELLE, "Animations.fbx")
    anim_ordner = "%s/Animations" % ZIEL
    anim_ergebnisse = importiere(anim_datei, anim_ordner, "Anim_Humans", skelett=skelett, nur_animation=True)
    unreal.log("LALABERG_NPC_ANIM assets=%d %s" % (len(anim_ergebnisse), " ".join(anim_ergebnisse)))
