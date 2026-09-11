# Legt Texturen und Grundmaterialien der Stadt an.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_materialien.py"
#
# Die Geometrie hat keine UV-Koordinaten - sie entsteht zur Laufzeit aus
# amtlichen Umringen. Deshalb wird jede Textur weltbezogen projiziert
# (WorldAlignedTexture): drei Achsen, nach der Normalen ueberblendet. Die
# Textur ist nur ein Helligkeitsmodulator um 1.0; die Farbe kommt weiter aus
# der Scheitelfarbe, also aus den amtlichen Daten und der Altstadtpalette.

import os
import unreal

WURZEL = unreal.Paths.project_dir()
# Unreal speichert den beim Import angegebenen Pfad dauerhaft im Paket - auch
# als rohe Zeichenkette, die sich nachtraeglich nicht zuverlaessig entfernen
# laesst (siehe git-Historie: "Factory_<Pfad>" blieb trotz AssetImportData-
# Patch stehen). Im oeffentlichen Repo darf da kein Benutzername drinstehen.
# NEUTRALE_QUELLE zeigt auf eine Kopie der PNGs ausserhalb des Benutzer-
# ordners; ohne Angabe wird lokal aus dem Projekt importiert (Warnung).
QUELLE = os.environ.get("LALABERG_NEUTRALE_QUELLE") or os.path.join(WURZEL, "Tools", "Texturen")
if "LALABERG_NEUTRALE_QUELLE" not in os.environ:
    unreal.log_warning("LALABERG_TEXTUR_PFAD kein neutraler Pfad gesetzt - der Importpfad "
                        "landet dauerhaft im Paket. LALABERG_NEUTRALE_QUELLE setzen fuer den "
                        "oeffentlichen Stand.")
ORDNER_M = "/Game/Art/Materials"
ORDNER_T = "/Game/Art/Textures"

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()
WELT = unreal.load_object(None, "/Engine/Functions/Engine_MaterialFunctions01/"
                                "Texturing/WorldAlignedTexture.WorldAlignedTexture")
if WELT is None:
    unreal.log_warning("LALABERG_WELT Funktion nicht gefunden - Texturen bleiben aus")


def hole_textur(name):
    pfad = "%s/%s" % (ORDNER_T, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        return unreal.load_asset(pfad)
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
    return unreal.load_asset(pfad) if unreal.EditorAssetLibrary.does_asset_exist(pfad) else None


def knoten(material, klasse, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(material, klasse, x, y)


def verbinde(quelle, ausgang, ziel, eingang):
    return unreal.MaterialEditingLibrary.connect_material_expressions(quelle, ausgang, ziel, eingang)


def an_kanal(quelle, ausgang, eigenschaft):
    # Der Ausgangsname entscheidet. Trifft er nicht, bleibt der Kanal leer -
    # und die Stadt bekommt still das graue Vorgabematerial.
    return unreal.MaterialEditingLibrary.connect_material_property(quelle, ausgang, eigenschaft)


def baue(name, textur, kachel_cm, rauheit, spiegelung, kontrast=1.0, uv=False, glas=False):
    pfad = "%s/%s" % (ORDNER_M, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        unreal.EditorAssetLibrary.delete_asset(pfad)
    material = werkzeuge.create_asset(name, ORDNER_M, unreal.Material, unreal.MaterialFactoryNew())

    farbe = knoten(material, unreal.MaterialExpressionVertexColor, -1500, 0)
    quelle, ausgang = farbe, ""

    bild = hole_textur(textur) if textur else None
    if bild is not None and uv:
        # Fassaden haben eigene Texturkoordinaten: eine Kachel ist eine
        # Fensterachse mal ein Geschoss. Weltbezogene Projektion wuerde die
        # Fenster quer durch die Haeuser laufen lassen.
        probe = knoten(material, unreal.MaterialExpressionTextureSample, -1500, 300)
        probe.set_editor_property("texture", bild)
        mitte = knoten(material, unreal.MaterialExpressionSubtract, -1150, 300)
        mitte.set_editor_property("const_b", 0.5)
        verbinde(probe, "RGB", mitte, "A")
        stark = knoten(material, unreal.MaterialExpressionMultiply, -950, 300)
        stark.set_editor_property("const_b", 2.0 * kontrast)
        verbinde(mitte, "", stark, "A")
        eins = knoten(material, unreal.MaterialExpressionAdd, -750, 300)
        eins.set_editor_property("const_b", 1.0)
        verbinde(stark, "", eins, "A")
        misch = knoten(material, unreal.MaterialExpressionMultiply, -500, 60)
        if verbinde(farbe, "", misch, "A") and verbinde(eins, "", misch, "B"):
            quelle, ausgang = misch, ""
            unreal.log("LALABERG_UV %s auf Fensterachse" % name)
        if glas:
            # Glas ist glatt, Putz ist rau. Die Fenster sind in der Textur die
            # dunklen Stellen - daraus laesst sich die Rauheit ableiten.
            glatt = knoten(material, unreal.MaterialExpressionLinearInterpolate, -500, 700)
            glatt.set_editor_property("const_a", 0.14)
            glatt.set_editor_property("const_b", rauheit)
            if verbinde(probe, "R", glatt, "Alpha"):
                an_kanal(glatt, "", unreal.MaterialProperty.MP_ROUGHNESS)
                rauheit = None
    elif bild is not None and WELT is not None:
        objekt = knoten(material, unreal.MaterialExpressionTextureObjectParameter, -1900, 260)
        objekt.set_editor_property("parameter_name", "Detail")
        objekt.set_editor_property("texture", bild)

        groesse = knoten(material, unreal.MaterialExpressionConstant, -1900, 460)
        groesse.set_editor_property("r", float(kachel_cm))

        welt = knoten(material, unreal.MaterialExpressionMaterialFunctionCall, -1500, 300)
        welt.set_material_function(WELT)
        ok_t = verbinde(objekt, "", welt, "TextureObject")
        ok_g = verbinde(groesse, "", welt, "TextureSize")
        unreal.log("LALABERG_WELT %s textur=%s groesse=%s" % (name, ok_t, ok_g))

        # Modulation um 1.0: 1 + (Textur - 0.5) * 2k
        mitte = knoten(material, unreal.MaterialExpressionSubtract, -1150, 300)
        mitte.set_editor_property("const_b", 0.5)
        ok_m = verbinde(welt, "XYZ Texture", mitte, "A")
        stark = knoten(material, unreal.MaterialExpressionMultiply, -950, 300)
        stark.set_editor_property("const_b", 2.0 * kontrast)
        verbinde(mitte, "", stark, "A")
        eins = knoten(material, unreal.MaterialExpressionAdd, -750, 300)
        eins.set_editor_property("const_b", 1.0)
        verbinde(stark, "", eins, "A")

        misch = knoten(material, unreal.MaterialExpressionMultiply, -500, 60)
        if ok_m and verbinde(farbe, "", misch, "A") and verbinde(eins, "", misch, "B"):
            quelle, ausgang = misch, ""
        else:
            unreal.log_warning("LALABERG_MISCH %s nicht verbunden (welt=%s)" % (name, ok_m))

    if not an_kanal(quelle, ausgang, unreal.MaterialProperty.MP_BASE_COLOR):
        unreal.log_warning("LALABERG_FARBE %s nicht verbunden" % name)

    if rauheit is not None:
        rau = knoten(material, unreal.MaterialExpressionConstant, -500, 420)
        rau.set_editor_property("r", rauheit)
        an_kanal(rau, "", unreal.MaterialProperty.MP_ROUGHNESS)

    spek = knoten(material, unreal.MaterialExpressionConstant, -500, 560)
    spek.set_editor_property("r", spiegelung)
    an_kanal(spek, "", unreal.MaterialProperty.MP_SPECULAR)

    # Einseitig: zweiseitige Materialien klappen auf der Rueckseite die
    # Schattennormale um und legen ganze Flaechen ins Dunkel.
    material.set_editor_property("two_sided", False)
    # Ohne dieses Kennzeichen weigert sich die Engine, das Material auf
    # Nanite-Geometrie zu zeichnen, und nimmt still das graue Vorgabematerial.
    material.set_editor_property("used_with_nanite", True)
    material.set_editor_property("used_with_static_lighting", True)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(pfad)
    return pfad


gebaut = [
    baue("M_Putz", "T_Fassade_D", 0.0, 0.88, 0.32, 1.0, uv=True, glas=True),   # Fassaden
    baue("M_Ziegel",  "T_Ziegel_D",  190.0, 0.76, 0.35, 1.1),   # Daecher
    # Neue, detailreiche Asphaltoberflaeche: die fruehere prozedurale Textur
    # war bei normaler Kameradistanz fast einfarbig und liess jede Fahrbahn
    # wie eine graue Grundplatte aussehen.
    baue("M_Asphalt", "T_Asphalt_Real_D", 260.0, 0.72, 0.28, 1.15), # Fahrbahn und Gleis
    baue("M_Boden",   "T_Wiese_D",   950.0, 0.94, 0.20, 1.0),   # Wiese, Acker, Wald
    baue("M_Wasser",  "T_Wasser_D",  900.0, 0.26, 0.50, 0.6),   # Lech und Teiche
    baue("M_Laub",    "T_Wiese_D",   140.0, 0.92, 0.18, 1.2),   # Kronen und Staemme
    baue("M_Stein",   "T_Putz_D",    120.0, 0.62, 0.42, 0.8),   # Brunnen und Figuren
    # Autolack ist glatt und spiegelt: die Rauheit macht den Unterschied
    # zwischen Blech und Pappe.
    baue("M_Lack",    None,            0.0, 0.22, 0.60, 0.0),
    baue("M_Glas",    None,            0.0, 0.06, 0.70, 0.0),
    baue("M_Stoff",   "T_Putz_D",     45.0, 0.86, 0.24, 0.6),   # Kleidung und Haut
]
unreal.log("LALABERG_MATERIALIEN " + " ".join(gebaut))
