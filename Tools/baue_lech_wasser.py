"""Erzeugt ein dynamisches Lech-Wassermaterial und bindet es an Wasser-Meshes."""
import unreal

TOOLS = unreal.AssetToolsHelpers.get_asset_tools()
MAT_PATH = "/Game/Art/Materials/M_Lech"
TEX = unreal.load_asset("/Game/Art/Textures/T_Wasser_D")
if TEX is None:
    raise RuntimeError("LALABERG_LECH fehlende Wassertextur")
if unreal.EditorAssetLibrary.does_asset_exist(MAT_PATH):
    unreal.EditorAssetLibrary.delete_asset(MAT_PATH)
mat = TOOLS.create_asset("M_Lech", "/Game/Art/Materials", unreal.Material, unreal.MaterialFactoryNew())
mat.set_editor_property("two_sided", True)
mat.set_editor_property("used_with_nanite", False)

def node(kind, x, y):
    return unreal.MaterialEditingLibrary.create_material_expression(mat, kind, x, y)
def channel(expr, out, prop):
    unreal.MaterialEditingLibrary.connect_material_property(expr, out, prop)
def link(a, out, b, pin):
    unreal.MaterialEditingLibrary.connect_material_expressions(a, out, b, pin)

# Zwei unterschiedlich schnelle Wellenlagen verhindern sichtbares Kacheln.
uv = node(unreal.MaterialExpressionTextureCoordinate, -1500, 0)
zeit = node(unreal.MaterialExpressionTime, -1450, 260)
s1x = node(unreal.MaterialExpressionConstant, -1320, -170); s1x.r = 0.018
s1y = node(unreal.MaterialExpressionConstant, -1320, -70); s1y.r = 0.032
v1 = node(unreal.MaterialExpressionAppendVector, -1200, -120)
s2x = node(unreal.MaterialExpressionConstant, -1320, 120); s2x.r = -0.026
s2y = node(unreal.MaterialExpressionConstant, -1320, 220); s2y.r = 0.012
v2 = node(unreal.MaterialExpressionAppendVector, -1200, 170)
m1 = node(unreal.MaterialExpressionMultiply, -1100, -120)
m2 = node(unreal.MaterialExpressionMultiply, -1100, 170)
p1 = node(unreal.MaterialExpressionAdd, -940, -120)
p2 = node(unreal.MaterialExpressionAdd, -940, 170)
link(s1x, "", v1, "A"); link(s1y, "", v1, "B")
link(s2x, "", v2, "A"); link(s2y, "", v2, "B")
link(zeit, "", m1, "A"); link(v1, "", m1, "B"); link(uv, "", p1, "A"); link(m1, "", p1, "B")
link(zeit, "", m2, "A"); link(v2, "", m2, "B"); link(uv, "", p2, "A"); link(m2, "", p2, "B")
s1 = node(unreal.MaterialExpressionTextureSample, -1050, -120); s1.texture = TEX
s2 = node(unreal.MaterialExpressionTextureSample, -1050, 170); s2.texture = TEX
link(p1, "", s1, "Coordinates"); link(p2, "", s2, "Coordinates")
mix = node(unreal.MaterialExpressionAdd, -800, 10)
link(s1, "RGB", mix, "A"); link(s2, "RGB", mix, "B")
half = node(unreal.MaterialExpressionMultiply, -620, 10)
half_strength = node(unreal.MaterialExpressionConstant, -800, 95); half_strength.r = 0.5
link(mix, "", half, "A")
link(half_strength, "", half, "B")
# Dunkles Gletscherwasser mit hellen, blickwinkelabhaengigen Reflexen.
deep = node(unreal.MaterialExpressionConstant3Vector, -620, 260); deep.constant = unreal.LinearColor(0.012, 0.075, 0.085, 1)
shallow = node(unreal.MaterialExpressionConstant3Vector, -620, 410); shallow.constant = unreal.LinearColor(0.045, 0.31, 0.28, 1)
lerp = node(unreal.MaterialExpressionLinearInterpolate, -390, 190)
link(deep, "", lerp, "A"); link(shallow, "", lerp, "B"); link(half, "R", lerp, "Alpha")
fresnel = node(unreal.MaterialExpressionFresnel, -380, -130)
foam = node(unreal.MaterialExpressionConstant3Vector, -160, -80); foam.constant = unreal.LinearColor(0.32, 0.62, 0.64, 1)
surface = node(unreal.MaterialExpressionLinearInterpolate, 60, 80)
link(lerp, "", surface, "A"); link(foam, "", surface, "B"); link(fresnel, "", surface, "Alpha")
channel(surface, "", unreal.MaterialProperty.MP_BASE_COLOR)
rough = node(unreal.MaterialExpressionConstant, 40, 290); rough.r = 0.12
spec = node(unreal.MaterialExpressionConstant, 40, 380); spec.r = 0.88
channel(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
channel(spec, "", unreal.MaterialProperty.MP_SPECULAR)
unreal.MaterialEditingLibrary.recompile_material(mat)
unreal.EditorAssetLibrary.save_asset(MAT_PATH)

changed = 0
for path in unreal.EditorAssetLibrary.list_assets("/Game/City/Sectors", recursive=True, include_folder=False):
    if "_Water_" not in path:
        continue
    mesh = unreal.load_asset(path)
    if mesh and mesh.get_material(0) != mat:
        mesh.set_material(0, mat)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=True)
        changed += 1
unreal.log("LALABERG_LECH material=%s water_meshes=%d" % (MAT_PATH, changed))
