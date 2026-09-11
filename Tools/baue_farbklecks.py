# Legt M_Farbklecks an: ein Abziehbild-Material (Decal) fuer Farbkugel-
# Treffer. Die Farbe kommt aus einem Vektor-Parameter je Treffer.
#
# Ein erster Versuch mit einem von Hand gerechneten radialen Verlauf (Distanz
# zur Mitte aus Grundrechenknoten, da der fertige RadialGradient-Knoten ueber
# Python nicht erreichbar ist) rendert als hartkantiges, gleichmaessig
# gefaerbtes Rechteck statt eines weichen Kreises - die Knotenverbindungen
# meldeten Erfolg, das Ergebnis im Spiel widersprach dem. Ohne Zugriff auf
# den Material-Editor liess sich die Ursache nicht weiter eingrenzen. Bis
# dahin bleibt der Klecks bewusst schlicht: ein flaechig gefaerbtes Feld,
# kein organischer Spritzer.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_farbklecks.py"

import unreal

ORDNER_M = "/Game/Art/Materials"
werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()

pfad = "%s/M_Farbklecks" % ORDNER_M
if unreal.EditorAssetLibrary.does_asset_exist(pfad):
    unreal.EditorAssetLibrary.delete_asset(pfad)
material = werkzeuge.create_asset("M_Farbklecks", ORDNER_M, unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
# decal_blend_mode laesst sich ueber Python nicht setzen ("protected" - weder
# lesen noch schreiben); der Standardwert bei MD_DEFERRED_DECAL ist bereits
# DBM_TRANSLUCENT.
material.set_editor_property("two_sided", True)

farbe = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
farbe.set_editor_property("parameter_name", "Farbe")
farbe.set_editor_property("default_value", unreal.LinearColor(0.8, 0.1, 0.5, 1.0))

deckung = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
deckung.set_editor_property("r", 0.9)

unreal.MaterialEditingLibrary.connect_material_property(farbe, "", unreal.MaterialProperty.MP_BASE_COLOR)
unreal.MaterialEditingLibrary.connect_material_property(deckung, "", unreal.MaterialProperty.MP_OPACITY)

unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(pfad)
unreal.log("LALABERG_FARBKLECKS " + pfad)
