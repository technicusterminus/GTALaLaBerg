# Diagnose: prueft, ob alle vier Farmer-Teile wirklich dasselbe Skeleton-
# Asset referenzieren (Voraussetzung fuer SetLeaderPoseComponent).
import unreal
TEILE = ["Body", "Feet", "Head", "Legs"]
for teil in TEILE:
    pfad = "/Game/Art/People/Farmer/SK_Farmer_%s" % teil
    mesh = unreal.load_asset(pfad)
    if mesh is None:
        unreal.log_warning("LALABERG_NPC_SKELETT fehlt: " + pfad)
        continue
    skelett = mesh.get_editor_property("skeleton")
    unreal.log("LALABERG_NPC_SKELETT %s -> %s" % (teil, skelett.get_path_name() if skelett else "None"))
