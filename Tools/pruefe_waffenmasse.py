# Diagnose-Werkzeug (nicht Teil der Spiellogik): gibt fuer jedes importierte
# Waffen-Mesh die lokale Bounding Box aus, um Skalierung/Versatz in
# ModellInfo (LaLaBergWaffe.cpp) nachvollziehbar auf echten Massen statt
# Augenmass aufzubauen.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/pruefe_waffenmasse.py"
import unreal

PFADE = [
    "/Game/Art/Waffen/Pistol/StaticMeshes/SM_Pistole.SM_Pistole",
    "/Game/Art/Waffen/Smg/StaticMeshes/SM_Maschine.SM_Maschine",
    "/Game/Art/Waffen/Shotgun/StaticMeshes/SM_Schrotflinte.SM_Schrotflinte",
    "/Game/Art/Waffen/RocketLauncher/StaticMeshes/SM_Raketenwerfer.SM_Raketenwerfer",
]

for pfad in PFADE:
    mesh = unreal.load_asset(pfad)
    if mesh is None:
        unreal.log_warning("LALABERG_MASS fehlt: " + pfad)
        continue
    box = mesh.get_bounding_box()
    groesse = box.max - box.min
    mitte = mesh.get_bounds().origin
    unreal.log("LALABERG_MASS %s groesse=%s mitte=%s min=%s max=%s" % (
        pfad, groesse, mitte, box.min, box.max))
