# Diagnose-Werkzeug (nicht Teil der Spiellogik): listet alle importierten
# CarConcept-Teilnetze mit ihrer lokalen Bounding Box, um zu pruefen, ob sie
# bei einer identischen (Null-)Relativtransform zu einem vollstaendigen Wagen
# zusammensetzen - das ist bei diesen Fahrzeug-Kits ueblich, aber ungeprueft
# Behauptung statt Tatsache.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/pruefe_carconcept.py"
import unreal

ORDNER = "/Game/Art/Vehicles/CarConcept/StaticMeshes"
pfade = sorted(unreal.EditorAssetLibrary.list_assets(ORDNER, recursive=False, include_folder=False))
unreal.log("LALABERG_CARCONCEPT_ANZAHL %d" % len(pfade))
gesamt_min = [1e9, 1e9, 1e9]
gesamt_max = [-1e9, -1e9, -1e9]
for pfad in pfade:
    mesh = unreal.load_asset(pfad)
    if mesh is None:
        continue
    box = mesh.get_bounding_box()
    name = pfad.split("/")[-1]
    unreal.log("LALABERG_CARCONCEPT_TEIL %s min=%s max=%s" % (name, box.min, box.max))
    gesamt_min = [min(gesamt_min[i], [box.min.x, box.min.y, box.min.z][i]) for i in range(3)]
    gesamt_max = [max(gesamt_max[i], [box.max.x, box.max.y, box.max.z][i]) for i in range(3)]
unreal.log("LALABERG_CARCONCEPT_GESAMT min=%s max=%s" % (gesamt_min, gesamt_max))
