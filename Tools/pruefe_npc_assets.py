# Diagnose-Werkzeug: listet alle Assets unter /Game/Art/People mit Klasse.
import unreal
for pfad in sorted(unreal.EditorAssetLibrary.list_assets("/Game/Art/People", recursive=True, include_folder=False)):
    daten = unreal.EditorAssetLibrary.find_asset_data(pfad)
    unreal.log("LALABERG_NPC_ASSET %s klasse=%s" % (pfad, daten.asset_class_path.asset_name if daten else "?"))
