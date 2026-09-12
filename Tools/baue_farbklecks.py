# Legt M_Farbklecks an: ein Abziehbild-Material (Decal) fuer Farbkugel-
# Treffer. Die Farbe kommt aus einem Vektor-Parameter je Treffer.
#
# Zwei fruehere Anlaeufe (ein von Hand gerechneter radialer Verlauf, dann eine
# Textur-Maske ueber einen Multiply-Knoten) rendersen beide als hartkantiges,
# voll deckendes Rechteck, obwohl jede Knotenverbindung Erfolg meldete. Die
# eigentliche Ursache, an zwei Stellen zugleich: (1) "decal_blend_mode" ist in
# UE 5.8 eine "DeprecatedProperty ... No longer used" (Material.h) - weder
# lesbar noch wirksam ueber Python, und selbst wenn gesetzt haette es nichts
# geaendert. Massgeblich ist die normale "blend_mode" (EBlendMode), die ohne
# ausdrueckliche Zuweisung implizit auf Opaque steht - das ignoriert
# MP_OPACITY vollstaendig. (2) Ein Multiply-Knoten zwischen TextureSample und
# Opacity meldete zwar "Verbindung erfolgreich", blieb aber wirkungslos; die
# direkte Verbindung TextureSample.R -> Opacity (ohne Multiply) funktioniert
# reproduzierbar. Die Spritzer-Maske selbst (T_Farbklecks_A.png, siehe
# erzeuge_klecks_textur.py) ist rein rechnerisch mit PIL erzeugt, kein
# Download.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_farbklecks.py"

import os
import unreal

ORDNER_M = "/Game/Art/Materials"
ORDNER_T = "/Game/Art/Textures"
WURZEL = unreal.Paths.project_dir()
QUELLE = os.environ.get("LALABERG_NEUTRALE_QUELLE") or os.path.join(WURZEL, "Tools", "Texturen")
if "LALABERG_NEUTRALE_QUELLE" not in os.environ:
    unreal.log_warning("LALABERG_NEUTRALE_QUELLE nicht gesetzt - der Importpfad landet dauerhaft "
                        "im Paket. Fuer den oeffentlichen Stand setzen.")

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()


def hole_maske():
    name = "T_Farbklecks_A"
    pfad = "%s/%s" % (ORDNER_T, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        unreal.EditorAssetLibrary.delete_asset(pfad)
    datei = os.path.join(QUELLE, name + ".png")
    if not os.path.isfile(datei):
        unreal.log_warning("LALABERG_KLECKSTEXTUR fehlt: " + datei)
        return None
    auftrag = unreal.AssetImportTask()
    auftrag.filename = datei
    auftrag.destination_path = ORDNER_T
    auftrag.destination_name = name
    auftrag.automated = True
    auftrag.replace_existing = True
    auftrag.save = True
    werkzeuge.import_asset_tasks([auftrag])
    bild = unreal.load_asset(pfad) if unreal.EditorAssetLibrary.does_asset_exist(pfad) else None
    if bild is not None:
        bild.set_editor_property("srgb", False)
        bild.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    return bild


pfad = "%s/M_Farbklecks" % ORDNER_M
if unreal.EditorAssetLibrary.does_asset_exist(pfad):
    unreal.EditorAssetLibrary.delete_asset(pfad)
material = werkzeuge.create_asset("M_Farbklecks", ORDNER_M, unreal.Material, unreal.MaterialFactoryNew())
material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
# Der eigentliche Fehler beider bisherigen Anlaeufe: "decal_blend_mode" ist
# in UE 5.8 als Property "DeprecatedProperty ... No longer used" markiert
# (Material.h) - deshalb liess es sich weder lesen noch schreiben, UND es
# haette ohnehin nichts bewirkt. Massgeblich ist die normale "blend_mode"
# (EBlendMode) - stand implizit auf Opaque, das ignoriert MP_OPACITY
# vollstaendig und zeichnet ein hartkantiges, voll deckendes Rechteck, ganz
# gleich was im Materialgraphen an Opacity haengt. Das erklaert, warum beide
# Anlaeufe (radialer Verlauf UND Textur-Maske) exakt dasselbe falsche
# Ergebnis zeigten, obwohl die Knotenverbindungen jedes Mal Erfolg meldeten.
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("two_sided", True)

farbe = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionVectorParameter, -400, 0)
farbe.set_editor_property("parameter_name", "Farbe")
farbe.set_editor_property("default_value", unreal.LinearColor(0.8, 0.1, 0.5, 1.0))
unreal.MaterialEditingLibrary.connect_material_property(farbe, "", unreal.MaterialProperty.MP_BASE_COLOR)

maske = hole_maske()
if maske is not None:
    # Ohne einen expliziten TextureCoordinate-Knoten liefert TextureSample bei
    # einem Decal-Material keine brauchbare Projektions-UV (dieselbe Falle wie
    # bei M_Waffenmetall). Direkt R -> Opacity, OHNE einen Multiply-Knoten
    # dazwischen: mit Multiply meldete die Verbindung Erfolg, blieb im Spiel
    # aber wirkungslos (hartkantiges Rechteck) - Ursache unklar, aber die
    # direkte Verbindung ist reproduzierbar richtig (im Test bestaetigt).
    uv = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -650, 250)
    uv.set_editor_property("coordinate_index", 0)
    probe = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionTextureSample, -400, 250)
    probe.set_editor_property("texture", maske)
    ok_uv = unreal.MaterialEditingLibrary.connect_material_expressions(uv, "", probe, "UVs")
    ok_op = unreal.MaterialEditingLibrary.connect_material_property(probe, "R", unreal.MaterialProperty.MP_OPACITY)
    unreal.log("LALABERG_FARBKLECKS_VERBINDUNG uv->probe=%s probe->opacity=%s texture=%s srgb=%s" % (
        ok_uv, ok_op, maske.get_path_name(), maske.get_editor_property("srgb")))
else:
    deckung = unreal.MaterialEditingLibrary.create_material_expression(material, unreal.MaterialExpressionConstant, -400, 200)
    deckung.set_editor_property("r", 0.9)
    unreal.MaterialEditingLibrary.connect_material_property(deckung, "", unreal.MaterialProperty.MP_OPACITY)

unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(pfad)
unreal.log("LALABERG_FARBKLECKS " + pfad + " maske=" + str(maske is not None))
