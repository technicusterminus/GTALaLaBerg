# Legt M_Waffenmetall an: das bisherige Waffenmodell nutzte M_Stoff (Rauheit
# 0.86, kein Metallic) - dieselbe Textur wie Kleidung, dazu vollkommen flache
# Vertexfarben ohne jede Oberflaechenstruktur. Hier: Metallic/Rauheit fuer
# Metall/Hartplastik, dazu T_Waffenmetall_D (siehe erzeuge_waffentextur.py -
# rein rechnerisch erzeugt, keine externe Quelle) als Riefen-/Kratzerdetail
# ueber der Scheitelfarbe. Keine WorldAlignedTexture wie bei der Stadt: die
# Waffe hat eigene, objektbezogene UVs (siehe LaLaBergWaffe.cpp) - eine
# weltbezogene Projektion wuerde beim Umsehen ueber das Ansichtsmodell
# "schwimmen".
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_waffenmetall.py"

import os
import unreal

WURZEL = unreal.Paths.project_dir()
QUELLE = os.environ.get("LALABERG_NEUTRALE_QUELLE") or os.path.join(WURZEL, "Tools", "Texturen")
if "LALABERG_NEUTRALE_QUELLE" not in os.environ:
    unreal.log_warning("LALABERG_NEUTRALE_QUELLE nicht gesetzt - der Importpfad landet dauerhaft "
                        "im Paket. Fuer den oeffentlichen Stand setzen.")

ORDNER_M = "/Game/Art/Materials"
ORDNER_T = "/Game/Art/Textures"
NAME = "M_Waffenmetall"
PFAD = "%s/%s" % (ORDNER_M, NAME)

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()


def hole_textur(name):
    pfad = "%s/%s" % (ORDNER_T, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        unreal.EditorAssetLibrary.delete_asset(pfad)
    datei = os.path.join(QUELLE, name + ".png")
    if not os.path.isfile(datei):
        unreal.log_warning("LALABERG_TEXTUR fehlt: " + datei)
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
        # Graustufen-Detail, keine Farbinformation - sonst faerbt die Textur
        # selbst mit statt nur die Rauheit/Helligkeit zu modulieren.
        bild.set_editor_property("srgb", False)
        bild.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_GRAYSCALE)
    return bild


if unreal.EditorAssetLibrary.does_asset_exist(PFAD):
    unreal.EditorAssetLibrary.delete_asset(PFAD)
material = werkzeuge.create_asset(NAME, ORDNER_M, unreal.Material, unreal.MaterialFactoryNew())


def knoten(klasse, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, klasse, x, y)


def verbinde(quelle, ausgang, ziel, eingang):
    return unreal.MaterialEditingLibrary.connect_material_expressions(quelle, ausgang, ziel, eingang)


def an_kanal(quelle, ausgang, eigenschaft):
    return unreal.MaterialEditingLibrary.connect_material_property(quelle, ausgang, eigenschaft)


farbe = knoten(unreal.MaterialExpressionVertexColor, -900, -100)

bild = hole_textur("T_Waffenmetall_D")
basis_quelle, basis_ausgang = farbe, ""
rau_konstante = 0.32
if bild is not None:
    uv = knoten(unreal.MaterialExpressionTextureCoordinate, -900, 200)
    uv.set_editor_property("u_tiling", 3.0)
    uv.set_editor_property("v_tiling", 3.0)
    probe = knoten(unreal.MaterialExpressionTextureSample, -650, 200)
    probe.set_editor_property("texture", bild)
    verbinde(uv, "", probe, "UVs")

    # Um 1.0 modulieren (0.85..1.15 statt 0..1), damit die Riefen die
    # Vertexfarbe abdunkeln/aufhellen statt sie zu ueberdecken.
    mitte = knoten(unreal.MaterialExpressionSubtract, -450, 200)
    mitte.set_editor_property("const_b", 0.5)
    verbinde(probe, "R", mitte, "A")
    stark = knoten(unreal.MaterialExpressionMultiply, -300, 200)
    stark.set_editor_property("const_b", 0.30)
    verbinde(mitte, "", stark, "A")
    eins = knoten(unreal.MaterialExpressionAdd, -150, 200)
    eins.set_editor_property("const_b", 1.0)
    verbinde(stark, "", eins, "A")

    misch = knoten(unreal.MaterialExpressionMultiply, 50, -50)
    if verbinde(farbe, "", misch, "A") and verbinde(eins, "", misch, "B"):
        basis_quelle, basis_ausgang = misch, ""

    # Dieselben Riefen auch leicht in die Rauheit - Kratzer sind matter als
    # die polierte Flaeche daneben.
    rau_misch = knoten(unreal.MaterialExpressionLinearInterpolate, -300, 400)
    rau_misch.set_editor_property("const_a", 0.24)
    rau_misch.set_editor_property("const_b", 0.42)
    verbinde(probe, "R", rau_misch, "Alpha")
    an_kanal(rau_misch, "", unreal.MaterialProperty.MP_ROUGHNESS)
    rau_konstante = None

an_kanal(basis_quelle, basis_ausgang, unreal.MaterialProperty.MP_BASE_COLOR)

if rau_konstante is not None:
    rau = knoten(unreal.MaterialExpressionConstant, -400, 400)
    rau.set_editor_property("r", rau_konstante)
    an_kanal(rau, "", unreal.MaterialProperty.MP_ROUGHNESS)

metallic = knoten(unreal.MaterialExpressionConstant, -400, 520)
metallic.set_editor_property("r", 0.85)
an_kanal(metallic, "", unreal.MaterialProperty.MP_METALLIC)

spek = knoten(unreal.MaterialExpressionConstant, -400, 640)
spek.set_editor_property("r", 0.55)
an_kanal(spek, "", unreal.MaterialProperty.MP_SPECULAR)

material.set_editor_property("two_sided", False)
material.set_editor_property("used_with_nanite", True)
material.set_editor_property("used_with_static_lighting", True)
unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(PFAD)
unreal.log("LALABERG_WAFFENMETALL " + PFAD + " textur=" + str(bild is not None))
