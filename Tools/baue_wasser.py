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

material = unreal.EditorAssetLibrary.load_asset(PFAD)
if material:
    # An Ort und Stelle neu bauen, damit die Stadt-Meshes ihren Verweis behalten.
    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
else:
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_Wasser", ORDNER, unreal.Material, unreal.MaterialFactoryNew())

material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
material.set_editor_property("two_sided", False)
# Durchscheinende Flaechen werden sonst nicht beleuchtet wie eine Oberflaeche.
material.set_editor_property("translucency_lighting_mode",
                             unreal.TranslucencyLightingMode.TLM_SURFACE_PER_PIXEL_LIGHTING)
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


# Lange Duenung, gross und langsam; kurzes Kraeuseln, klein und schnell und
# quer dazu.
duenung = welle("T_Welle_N", 900.0, 0.010, 0.004, -600)
kraeuseln = welle("T_Kraeusel_N", 260.0, -0.020, 0.013, -100)
wellen = knoten(material, unreal.MaterialExpressionAdd, -1250, -350)
verbinde(duenung, "RGB", wellen, "A")
verbinde(kraeuseln, "RGB", wellen, "B")
# Zwei addierte Normalen zeigen im Mittel zu steil - halbieren genuegt.
flacher = knoten(material, unreal.MaterialExpressionMultiply, -1050, -350)
verbinde(wellen, "", flacher, "A")
flacher.set_editor_property("const_b", 0.5)
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

tief = knoten(material, unreal.MaterialExpressionConstant3Vector, -1250, 380)
tief.set_editor_property("constant", unreal.LinearColor(0.010, 0.052, 0.060, 1.0))

# Tiefenfaerbung nur noch fuer die Deckkraft am Ufer: die Wasserflaechen
# liegen flach ueber dem Gelaende, der Tiefenunterschied ist fast ueberall
# gering - als Farbgeber blieb davon ein gleichmaessiges Hellgruen, das
# nach Schwimmbad aussah statt nach Fluss.
tiefe = knoten(material, unreal.MaterialExpressionDepthFade, -1250, 620)
tiefe.set_editor_property("fade_distance_default", 260.0)

# Farbe: nur das dunkle Gletschergruen. Der zweite Versuch hellte sie zum
# streifenden Blick hin auf - genau dort, wo Wasser in Wirklichkeit dunkel
# wird und stattdessen spiegelt. Das Ergebnis sah aus wie ein Schwimmbad.
# Hell wird die Flaeche jetzt nur ueber Spiegelung und Glanz.
# Schaum am Ufer: wo das Wasser auf Geometrie trifft, steht eine weisse
# Kante - das Merkmal, an dem man eine Wasserflaeche von einer gefaerbten
# Glasscheibe unterscheidet.
schaumtiefe = knoten(material, unreal.MaterialExpressionDepthFade, -1250, 760)
schaumtiefe.set_editor_property("fade_distance_default", 130.0)
schaum = knoten(material, unreal.MaterialExpressionOneMinus, -1050, 760)
verbinde(schaumtiefe, "", schaum, "")
weiss = knoten(material, unreal.MaterialExpressionConstant3Vector, -1250, 880)
weiss.set_editor_property("constant", unreal.LinearColor(0.72, 0.78, 0.78, 1.0))
grundfarbe = knoten(material, unreal.MaterialExpressionLinearInterpolate, -880, 440)
verbinde(tief, "", grundfarbe, "A")
verbinde(weiss, "", grundfarbe, "B")
verbinde(schaum, "", grundfarbe, "Alpha")
an_kanal(grundfarbe, "", unreal.MaterialProperty.MP_BASE_COLOR)

# Glanz: immer vorhanden, flach betrachtet voll. Werte ueber 1 bringen
# nichts, deshalb 0,55 plus Fresnel.
spiegel = knoten(material, unreal.MaterialExpressionMultiply, -1050, 200)
verbinde(fresnel, "", spiegel, "A")
spiegel.set_editor_property("const_b", 0.45)
spiegel_plus = knoten(material, unreal.MaterialExpressionAdd, -880, 200)
verbinde(spiegel, "", spiegel_plus, "A")
spiegel_plus.set_editor_property("const_b", 0.55)
an_kanal(spiegel_plus, "", unreal.MaterialProperty.MP_SPECULAR)

# Rauheit: spiegelglatt, nur der Schaum am Ufer ist stumpf.
rau = knoten(material, unreal.MaterialExpressionLinearInterpolate, -880, 300)
rau.set_editor_property("const_a", 0.035)    # glatt genug fuer Spiegelbilder
rau.set_editor_property("const_b", 0.55)
verbinde(schaum, "", rau, "Alpha")
an_kanal(rau, "", unreal.MaterialProperty.MP_ROUGHNESS)

# Deckkraft: am Ufer durchsichtig, weiter draussen dichter, und flach
# betrachtet fast undurchsichtig (dort spiegelt es).
deck_tiefe = knoten(material, unreal.MaterialExpressionLinearInterpolate, -950, 700)
deck_tiefe.set_editor_property("const_a", 0.22)
deck_tiefe.set_editor_property("const_b", 0.72)
verbinde(tiefe, "", deck_tiefe, "Alpha")
deckkraft = knoten(material, unreal.MaterialExpressionLinearInterpolate, -700, 700)
verbinde(deck_tiefe, "", deckkraft, "A")
deckkraft.set_editor_property("const_b", 1.0)
verbinde(fresnel, "", deckkraft, "Alpha")
an_kanal(deckkraft, "", unreal.MaterialProperty.MP_OPACITY)

# Brechung: Wasser hat 1,33; die Wellen bewegen sie mit.
brechung = knoten(material, unreal.MaterialExpressionLinearInterpolate, -700, 860)
brechung.set_editor_property("const_a", 1.0)
brechung.set_editor_property("const_b", 1.33)
verbinde(fresnel, "", brechung, "Alpha")
an_kanal(brechung, "", unreal.MaterialProperty.MP_REFRACTION)

unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_asset(PFAD)
unreal.log("LALABERG_MATERIALIEN " + PFAD)
