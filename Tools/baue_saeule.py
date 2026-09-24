# Legt M_Saeule an: das Material der Markierungssaeulen im Spiel (Auftrag,
# Laden, Revier). Durchsichtig und selbstleuchtend - eine Saeule ist ein
# Zeichen, kein Bauwerk: man soll hindurchsehen und sie trotzdem ueber die
# ganze Stadt erkennen.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_saeule.py"
#
# Parameter zur Laufzeit:
#   Color      - Farbe der Saeule (Vektor)
#   Deckkraft  - wie dicht sie ist (0 bis 1, Vorgabe 0,38)
import os
import unreal

quelle = open(os.path.join(unreal.Paths.project_dir(), "Tools", "baue_materialien.py"), encoding="utf-8").read()
# Nur die Werkzeuge uebernehmen, nicht die Liste, die alles neu baut.
exec(quelle.split("\ngebaut = [")[0])

ORDNER = "/Game/Art/Materials"
PFAD = "%s/M_Saeule" % ORDNER

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()
if unreal.EditorAssetLibrary.does_asset_exist(PFAD):
    material = unreal.EditorAssetLibrary.load_asset(PFAD)
    # An Ort und Stelle neu bauen, damit bestehende Verweise bleiben.
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
else:
    material = werkzeuge.create_asset("M_Saeule", ORDNER, unreal.Material, unreal.MaterialFactoryNew())

# Unlit: die Saeule soll in jeder Tageszeit gleich aussehen und auch im
# Schatten der Altstadt leuchten.
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property("two_sided", True)
material.set_editor_property("used_with_nanite", True)

farbe = knoten(material, unreal.MaterialExpressionVectorParameter, -700, -100)
farbe.set_editor_property("parameter_name", "Color")
farbe.set_editor_property("default_value", unreal.LinearColor(0.25, 0.6, 1.0, 1.0))
an_kanal(farbe, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

deckkraft = knoten(material, unreal.MaterialExpressionScalarParameter, -700, 140)
deckkraft.set_editor_property("parameter_name", "Deckkraft")
deckkraft.set_editor_property("default_value", 0.38)
an_kanal(deckkraft, "", unreal.MaterialProperty.MP_OPACITY)

unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(PFAD)
unreal.log("LALABERG_MATERIALIEN " + PFAD)
