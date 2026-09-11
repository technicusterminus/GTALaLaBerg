"""Importiert das lizenzierte CarConcept-GLB als Unreal-Fahrzeugasset."""
import os
import unreal

project = unreal.Paths.project_dir()
source = os.path.join(project, "Content", "SourceData", "Vehicles", "CarConcept.glb")
destination = "/Game/Art/Vehicles"
if not os.path.isfile(source):
    raise RuntimeError("LALABERG_CAR fehlt: " + source)

task = unreal.AssetImportTask()
task.filename = source
task.destination_path = destination
task.destination_name = "SM_CarConcept"
task.automated = True
task.replace_existing = True
task.save = True
task.replace_existing_settings = False
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
assets = task.get_editor_property("imported_object_paths")
if not assets:
    raise RuntimeError("LALABERG_CAR Import ohne Asset")
for path in assets:
    asset = unreal.load_asset(path)
    if asset:
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)
unreal.log("LALABERG_CAR_IMPORT assets=" + " ".join(assets))
