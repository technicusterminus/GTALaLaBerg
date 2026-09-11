# Loescht ein Textur-Asset und importiert es neu aus einem neutralen Pfad
# ausserhalb des Benutzerordners. Unreal speichert den beim Import ange-
# gebenen Pfad dauerhaft im Paket - auch als rohe Zeichenkette, die sich
# nachtraeglich nicht zuverlaessig entfernen laesst (ein reiner
# AssetImportData-Patch liess einen "Factory_<Pfad>"-Rohstring stehen).
# Vorher die PNG nach C:/GTALaLaBerg/Tools/Texturen/<Name>.png kopieren.
#
#   set LALABERG_TEXTUR=T_Name_D
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="<abs Pfad>/Tools/importiere_textur_neutral.py"
import os
import unreal

NAME = os.environ.get("LALABERG_TEXTUR")
if not NAME:
    unreal.log_error("LALABERG_TEXTUR_NEU Umgebungsvariable LALABERG_TEXTUR fehlt")
else:
    ORDNER_T = "/Game/Art/Textures"
    NEUTRAL = "C:/GTALaLaBerg/Tools/Texturen"
    pfad = "%s/%s" % (ORDNER_T, NAME)
    quelle = os.path.join(NEUTRAL, NAME + ".png")
    if not os.path.isfile(quelle):
        unreal.log_error("LALABERG_TEXTUR_NEU fehlt: " + quelle)
    else:
        werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()
        if unreal.EditorAssetLibrary.does_asset_exist(pfad):
            unreal.EditorAssetLibrary.delete_asset(pfad)
        auftrag = unreal.AssetImportTask()
        auftrag.filename = quelle
        auftrag.destination_path = ORDNER_T
        auftrag.destination_name = NAME
        auftrag.automated = True
        auftrag.replace_existing = True
        auftrag.save = True
        werkzeuge.import_asset_tasks([auftrag])
        unreal.log("LALABERG_TEXTUR_NEU %s ok=%s" % (NAME, unreal.EditorAssetLibrary.does_asset_exist(pfad)))
