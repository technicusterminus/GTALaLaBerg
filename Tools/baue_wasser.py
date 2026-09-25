# Baut M_Wasser neu: Lech und Teiche als durchscheinende Wasserflaeche mit
# zwei laufenden Wellenkarten, Fresnel, Brechung und Tiefenfaerbung.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_wasser.py"
#
# Warum eigenes Skript: baue_materialien.py baut nur flache, undurchsichtige
# Oberflaechen (eine Textur, eine Rauheit). Wasser braucht mehr:
#
#   - Zwei Normalkarten (Tools/baue_wellen.py), verschieden gross skaliert
#     und mit verschiedenem Tempo in verschiedene Richtungen geschoben. Eine
#     allein sieht aus wie ein gleitendes Bild, zwei ergeben Bewegung.
#   - Fresnel: flach betrachtet spiegelt Wasser fast alles, senkrecht von
#     oben sieht man hinein. Daran haengen Farbe und Deckkraft.
#   - Brechung (Index 1,33 wie echtes Wasser), von denselben Wellen bewegt.
#   - Tiefenfaerbung ueber DepthFade: am Ufer klar, in der Mitte gruentuerkis
#     (der Lech fuehrt Gletscherwasser).
#
# Die Wasserflaechen der Stadt sind bewusst nicht Nanite (siehe
# LaLaBergImportCommandlet) - nur deshalb kann das Material durchscheinend
# sein.
import os
import unreal

quelle = open(os.path.join(unreal.Paths.project_dir(), "Tools", "baue_materialien.py"), encoding="utf-8").read()
exec(quelle.split("\ngebaut = [")[0])

ORDNER = "/Game/Art/Materials"
PFAD = "%s/M_Wasser" % ORDNER

# Das Material wird jedes Mal neu angelegt statt ausgeraeumt:
# delete_all_material_expressions laesst eigene Ausgabeknoten
# (SingleLayerWaterMaterialOutput) stehen, und ab dem zweiten Lauf lagen zwei
# davon im Material - Unreal uebersetzt es dann gar nicht mehr ("can contain
# only one Single Layer Water Material node") und zeigt das Schachbrett des
# Ersatzmaterials. An die Knotenliste kommt man ueber Python nicht heran
# (weder "expressions" noch "expression_collection" sind freigegeben),
# deshalb der grobe Weg. Der Pfad bleibt derselbe - die Stadt findet ihr
# Wassermaterial beim naechsten Laden wieder.
if unreal.EditorAssetLibrary.does_asset_exist(PFAD):
    unreal.EditorAssetLibrary.delete_asset(PFAD)
material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
    "M_Wasser", ORDNER, unreal.Material, unreal.MaterialFactoryNew())

# Single Layer Water statt durchscheinender Flaeche. Auf einer
# durchscheinenden Flaeche spiegelt in Unreal weder Screen Space noch eine
# Spiegelebene - nur Lumens Frontschicht, und die gab dem Lech den Himmel,
# aber nie das Ufer. Dieses Beleuchtungsmodell ist undurchsichtig, dafuer
# greifen beide Spiegelungen darauf.
material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_SINGLE_LAYER_WATER)
material.set_editor_property("two_sided", False)
material.set_editor_property("screen_space_reflections", True)
material.set_editor_property("used_with_static_lighting", True)


def welle(textur_name, groesse_cm, tempo_x, tempo_y, y):
    """Eine laufende Wellenkarte: Weltkoordinaten / Kachelgroesse, vom
    Panner geschoben, als Normalkarte abgetastet."""
    welt = knoten(material, unreal.MaterialExpressionWorldPosition, -2200, y)
    masse = knoten(material, unreal.MaterialExpressionConstant, -2200, y + 120)
    masse.set_editor_property("r", groesse_cm)
    teilen = knoten(material, unreal.MaterialExpressionDivide, -1950, y)
    verbinde(welt, "XY", teilen, "A")
    verbinde(masse, "", teilen, "B")
    panner = knoten(material, unreal.MaterialExpressionPanner, -1750, y)
    panner.set_editor_property("speed_x", tempo_x)
    panner.set_editor_property("speed_y", tempo_y)
    verbinde(teilen, "", panner, "Coordinate")
    probe = knoten(material, unreal.MaterialExpressionTextureSample, -1500, y)
    probe.set_editor_property("texture", hole_textur(textur_name))
    probe.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    verbinde(panner, "", probe, "UVs")
    return probe


# Drei Lagen statt zwei: lange Duenung (9 m), kurzes Kraeuseln (2,6 m) und
# darueber ein feines Zittern (0,9 m). Zwei Lagen ergaben aus der Naehe ein
# erkennbar wiederkehrendes Muster - dieselbe Kachel, nur zweimal. Die dritte
# Lage laeuft schraeg zu beiden und in anderem Tempo; damit faellt der Takt
# der Kachel nicht mehr auf.
duenung = welle("T_Welle_N", 900.0, 0.010, 0.004, -600)
kraeuseln = welle("T_Kraeusel_N", 260.0, -0.020, 0.013, -100)
zittern = welle("T_Kraeusel_N", 90.0, 0.031, -0.026, 380)
wellen = knoten(material, unreal.MaterialExpressionAdd, -1250, -350)
verbinde(duenung, "RGB", wellen, "A")
verbinde(kraeuseln, "RGB", wellen, "B")
feiner = knoten(material, unreal.MaterialExpressionMultiply, -1350, -250)
verbinde(zittern, "RGB", feiner, "A")
feiner.set_editor_property("const_b", 0.45)   # die feinste Lage nur angedeutet
wellen_alle = knoten(material, unreal.MaterialExpressionAdd, -1150, -350)
verbinde(wellen, "", wellen_alle, "A")
verbinde(feiner, "", wellen_alle, "B")
# Drei addierte Normalen zeigen im Mittel viel zu steil.
flacher = knoten(material, unreal.MaterialExpressionMultiply, -1050, -350)
verbinde(wellen_alle, "", flacher, "A")
flacher.set_editor_property("const_b", 0.36)
# In der Ferne die Wellen flachziehen: sonst liegt am Horizont ein
# sichtbarer Teppich aus derselben Kachel. Ueber 40 m wird die Normale
# zur glatten Flaeche hin ueberblendet.
tiefe_pixel = knoten(material, unreal.MaterialExpressionPixelDepth, -1050, -180)
weit = knoten(material, unreal.MaterialExpressionSmoothStep, -880, -180)
weit.set_editor_property("const_min", 4000.0)
weit.set_editor_property("const_max", 30000.0)
verbinde(tiefe_pixel, "", weit, "Value")
glatt = knoten(material, unreal.MaterialExpressionConstant3Vector, -880, -60)
glatt.set_editor_property("constant", unreal.LinearColor(0.5, 0.5, 1.0, 1.0))
normale = knoten(material, unreal.MaterialExpressionLinearInterpolate, -700, -300)
verbinde(flacher, "", normale, "A")
verbinde(glatt, "", normale, "B")
verbinde(weit, "", normale, "Alpha")
an_kanal(normale, "", unreal.MaterialProperty.MP_NORMAL)

# Fresnel: flach betrachtet spiegelt es, von oben sieht man hinein.
fresnel = knoten(material, unreal.MaterialExpressionFresnel, -1250, 200)
fresnel.set_editor_property("exponent", 4.0)
fresnel.set_editor_property("base_reflect_fraction", 0.04)
verbinde(normale, "", fresnel, "Normal")

# Die Farbe der Oberflaeche: dunkles Gletschergruen. Die Tiefenfaerbung kam
# frueher aus DepthFade - das darf ein undurchsichtiges Material nicht mehr
# lesen ("Only transparent or postprocess materials can read from scene
# depth"). Mit ihr ist auch der Uferschaum entfallen: er haengt an derselben
# Szenentiefe und braucht eine eigene Loesung.
# Den Blauanteil holt eine Maske heraus: ein Ausgang namens "B" existiert
# an der Ueberblendung nicht, und ohne Maske blieb der Eingang leer
# ("Node OneMinus: Missing 1-x input") - das Material uebersetzte dann gar
# nicht mehr und die Stadt zeigte das Schachbrett des Ersatzmaterials.
blau = knoten(material, unreal.MaterialExpressionComponentMask, -1200, 300)
blau.set_editor_property("r", False)
blau.set_editor_property("g", False)
blau.set_editor_property("b", True)
blau.set_editor_property("a", False)
verbinde(normale, "", blau, "")
flanke = knoten(material, unreal.MaterialExpressionOneMinus, -1050, 300)
verbinde(blau, "", flanke, "")
# Zwei Farben statt einer: steil von oben sieht man in den Fluss hinein
# (dunkles Gletschergruen), flach ueber die Flaeche fast nur noch das, was
# sich spiegelt - dort wird die Oberflaeche selbst dunkel und tritt zurueck.
# Mit einer einzigen Farbe lag ueber dem ganzen Fluss dasselbe Tuerkis, egal
# von wo man schaute, und das sah nach eingefaerbtem Glas aus.
tief = knoten(material, unreal.MaterialExpressionConstant3Vector, -1250, 380)
tief.set_editor_property("constant", unreal.LinearColor(0.008, 0.034, 0.038, 1.0))
streifend = knoten(material, unreal.MaterialExpressionConstant3Vector, -1250, 460)
streifend.set_editor_property("constant", unreal.LinearColor(0.002, 0.006, 0.008, 1.0))
grundfarbe = knoten(material, unreal.MaterialExpressionLinearInterpolate, -1000, 400)
verbinde(tief, "", grundfarbe, "A")
verbinde(streifend, "", grundfarbe, "B")
verbinde(fresnel, "", grundfarbe, "Alpha")

# Schaumkronen auf den steilsten Wellen. Uferschaum im Wortsinn - die weisse
# Kante, wo das Wasser auf Land trifft - braucht die Szenentiefe und ist im
# undurchsichtigen Material nicht mehr zu haben. Der Lech ist ohnehin ein
# Gebirgsfluss: er schaeumt auf den Kaemmen, nicht nur am Ufer. Gemessen
# wird die Steilheit an derselben Flanke wie die Rauheit (siehe unten): wo
# die Welle am steilsten steht, steht der Schaum.
kamm = knoten(material, unreal.MaterialExpressionSmoothStep, -1000, 540)
kamm.set_editor_property("const_min", 0.42)
kamm.set_editor_property("const_max", 0.78)
verbinde(flanke, "", kamm, "Value")
gischt = knoten(material, unreal.MaterialExpressionConstant3Vector, -1000, 620)
gischt.set_editor_property("constant", unreal.LinearColor(0.62, 0.68, 0.70, 1.0))
mit_schaum = knoten(material, unreal.MaterialExpressionLinearInterpolate, -820, 440)
verbinde(grundfarbe, "", mit_schaum, "A")
verbinde(gischt, "", mit_schaum, "B")
verbinde(kamm, "", mit_schaum, "Alpha")
an_kanal(mit_schaum, "", unreal.MaterialProperty.MP_BASE_COLOR)

# Glanz: immer vorhanden, flach betrachtet voll. Werte ueber 1 bringen
# nichts, deshalb 0,55 plus Fresnel.
spiegel = knoten(material, unreal.MaterialExpressionMultiply, -1050, 200)
verbinde(fresnel, "", spiegel, "A")
spiegel.set_editor_property("const_b", 0.45)
spiegel_plus = knoten(material, unreal.MaterialExpressionAdd, -880, 200)
verbinde(spiegel, "", spiegel_plus, "A")
spiegel_plus.set_editor_property("const_b", 0.55)
an_kanal(spiegel_plus, "", unreal.MaterialProperty.MP_SPECULAR)

# Rauheit: spiegelglatt, wo die Flaeche ruhig liegt, eine Spur stumpfer auf
# den Wellenflanken. Diese Spur ist das Glitzern: die Sonne bricht sich dort
# in tausend kleinen Punkten statt in einem grossen Fleck. Ueber den
# Blauanteil der Normalen - der ist 1, wo die Flaeche waagerecht liegt, und
# kleiner, je steiler die Welle steht.
rau = knoten(material, unreal.MaterialExpressionLinearInterpolate, -880, 300)
rau.set_editor_property("const_a", 0.020)
rau.set_editor_property("const_b", 0.120)
verbinde(flanke, "", rau, "Alpha")
an_kanal(rau, "", unreal.MaterialProperty.MP_ROUGHNESS)

# Was unter der Oberflaeche geschieht, regelt das Wassermodell: Streuung gibt
# dem Wasser Tiefe, Absorption schluckt zuerst das Rot.
wasserwerte = knoten(material, unreal.MaterialExpressionSingleLayerWaterMaterialOutput, -400, 700)
streuung = knoten(material, unreal.MaterialExpressionConstant3Vector, -700, 700)
streuung.set_editor_property("constant", unreal.LinearColor(0.00080, 0.00190, 0.00230, 1.0))
absorption = knoten(material, unreal.MaterialExpressionConstant3Vector, -700, 820)
absorption.set_editor_property("constant", unreal.LinearColor(0.3000, 0.1000, 0.0700, 1.0))
phase = knoten(material, unreal.MaterialExpressionConstant, -700, 940)
phase.set_editor_property("r", 0.0)
# Nichts scheint von hinten durch: unter den Wasserflaechen der Stadt liegt
# kein Flussbett, sondern nichts - ohne diese Null schien dort der Himmel
# durch und tauchte den Lech in ein leuchtendes Schwimmbadtuerkis.
hinter = knoten(material, unreal.MaterialExpressionConstant, -700, 1010)
hinter.set_editor_property("r", 0.0)


def ans_wasser(quelle, *namen):
    """Eingang des Wasserknotens verbinden. Trifft der Name nicht, bleibt der
    Eingang still leer und Unreals Vorgabewerte gelten - deshalb alle
    Schreibweisen durchprobieren und sonst warnen."""
    for name in namen:
        if verbinde(quelle, "", wasserwerte, name):
            return name
    unreal.log_warning("LALABERG_WASSER Eingang nicht gefunden: %s" % (namen,))
    return None


ans_wasser(streuung, "ScatteringCoefficients", "Scattering Coefficients")
ans_wasser(absorption, "AbsorptionCoefficients", "Absorption Coefficients")
ans_wasser(phase, "PhaseG", "Phase G")
ans_wasser(hinter, "ColorScaleBehindWater", "Color Scale Behind Water")

# Brechung: Wasser hat 1,33; die Wellen bewegen sie mit.
brechung = knoten(material, unreal.MaterialExpressionLinearInterpolate, -700, 860)
brechung.set_editor_property("const_a", 1.0)
brechung.set_editor_property("const_b", 1.33)
verbinde(fresnel, "", brechung, "Alpha")
an_kanal(brechung, "", unreal.MaterialProperty.MP_REFRACTION)

unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(PFAD)
unreal.log("LALABERG_MATERIALIEN " + PFAD)
