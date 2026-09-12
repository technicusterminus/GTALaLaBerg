# Diagnose: prueft, ob die Walk/Idle-AnimSequences tatsaechlich Keyframes
# enthalten (nicht nur als leere Huelle importiert wurden) und listet ihre
# Laenge.
import unreal
NAMEN = ["Idle", "Idle_Neutral", "Walk", "Run"]
for name in NAMEN:
    pfad = "/Game/Art/People/Animations/Anim_HumansCharacterArmature_%s" % name
    anim = unreal.load_asset(pfad)
    if anim is None:
        unreal.log_warning("LALABERG_NPC_ANIM_PRUEF fehlt: " + pfad)
        continue
    unreal.log("LALABERG_NPC_ANIM_PRUEF %s laenge=%.3f bilder=%d" % (
        name, anim.get_editor_property("sequence_length"),
        anim.get_editor_property("number_of_sampled_keys")))
