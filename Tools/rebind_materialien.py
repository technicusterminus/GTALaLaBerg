"""Bindet die aktuelle PBR-Materialbibliothek an alle Stadt-Static-Meshes.

Die Stadtmeshes verweisen nach dem Neuaufbau eines Material-Pakets noch auf
das alte Objekt. Dieses Werkzeug ersetzt nur Slot 0, fasst Geometrie,
Nanite-Einstellungen und Kollision nicht an und speichert nur Assets, deren
Material sich wirklich aendert.

Aufruf:
  UnrealEditor-Cmd.exe GTALaLaBerg.uproject -run=pythonscript \
    -script=Tools/rebind_materialien.py -unattended -nop4
"""

import unreal

ROOT = "/Game/City/Sectors"
MATERIALS = {
    "Wall": "/Game/Art/Materials/M_Putz.M_Putz",
    "Roof": "/Game/Art/Materials/M_Ziegel.M_Ziegel",
    "Road": "/Game/Art/Materials/M_Asphalt.M_Asphalt",
    "Rail": "/Game/Art/Materials/M_Asphalt.M_Asphalt",
    "Plaza": "/Game/Art/Materials/M_Pflaster.M_Pflaster",
    "Water": "/Game/Art/Materials/M_Wasser.M_Wasser",
    "Tree": "/Game/Art/Materials/M_Laub.M_Laub",
    "Trunk": "/Game/Art/Materials/M_Laub.M_Laub",
    "Stone": "/Game/Art/Materials/M_Stein.M_Stein",
    "Figure": "/Game/Art/Materials/M_Stein.M_Stein",
    "Sockel": "/Game/Art/Materials/M_Stein.M_Stein",
    "Gesims": "/Game/Art/Materials/M_Stein.M_Stein",
    "Laden": "/Game/Art/Materials/M_Stein.M_Stein",
    "Kamin": "/Game/Art/Materials/M_Stein.M_Stein",
    "Tuer": "/Game/Art/Materials/M_Stein.M_Stein",
    "Auto": "/Game/Art/Materials/M_Lack.M_Lack",
    "Glas": "/Game/Art/Materials/M_Glas.M_Glas",
    "Reifen": "/Game/Art/Materials/M_Asphalt.M_Asphalt",
    "Stoff": "/Game/Art/Materials/M_Stoff.M_Stoff",
    "Haut": "/Game/Art/Materials/M_Stoff.M_Stoff",
}
DEFAULT = "/Game/Art/Materials/M_Boden.M_Boden"

loaded = {key: unreal.load_object(None, path) for key, path in MATERIALS.items()}
ground = unreal.load_object(None, DEFAULT)
if ground is None or any(value is None for value in loaded.values()):
    raise RuntimeError("LALABERG_REBIND fehlendes Material")


def mesh_class(asset_path):
    leaf = asset_path.rsplit("/", 1)[-1]
    sector = asset_path.split("/")[-2]
    prefix = "SM_%s_" % sector
    if not leaf.startswith(prefix):
        return None
    rest = leaf[len(prefix):]
    return rest.rsplit("_", 1)[0]


checked = changed = skipped = 0
for asset_path in unreal.EditorAssetLibrary.list_assets(ROOT, recursive=True, include_folder=False):
    name = mesh_class(asset_path)
    if name is None:
        continue
    checked += 1
    target = next((mat for prefix, mat in loaded.items() if name.startswith(prefix)), ground)
    mesh = unreal.load_asset(asset_path)
    if mesh is None:
        unreal.log_warning("LALABERG_REBIND fehlt " + asset_path)
        continue
    old = mesh.get_material(0)
    if old == target:
        skipped += 1
        continue
    mesh.set_material(0, target)
    unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=True)
    changed += 1

unreal.log("LALABERG_REBIND checked=%d changed=%d unchanged=%d" % (checked, changed, skipped))
