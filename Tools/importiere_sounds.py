# Importiert alle in Tools/Sounds erzeugten WAVs (siehe erzeuge_sounds.py) als
# USoundWave nach /Game/Audio. Gleiches Muster wie bei Texturen: der
# Importpfad landet dauerhaft im Paket, deshalb ueber LALABERG_NEUTRALE_QUELLE
# eine Kopie ausserhalb des Benutzerordners angeben, sonst Warnung.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/importiere_sounds.py"
import os
import unreal

WURZEL = unreal.Paths.project_dir()
QUELLE = os.environ.get("LALABERG_NEUTRALE_QUELLE") or os.path.join(WURZEL, "Tools", "Sounds")
if "LALABERG_NEUTRALE_QUELLE" not in os.environ:
    unreal.log_warning("LALABERG_NEUTRALE_QUELLE nicht gesetzt - der Importpfad landet dauerhaft "
                        "im Paket. Fuer den oeffentlichen Stand setzen.")

ORDNER = "/Game/Audio"
werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()

NAMEN = ["SFX_Schuss_Pistole", "SFX_Schuss_Maschine", "SFX_Schuss_Schrot", "SFX_Schuss_Rakete",
         "SFX_Klecks", "SFX_Schritt", "SFX_Motor",
         # Dauerklaenge: Martinshorn, Rotor und Panzerdiesel laufen in der
         # Schleife, solange die Quelle da ist.
         "SFX_Sirene", "SFX_Rotor", "SFX_Panzer"]
SCHLEIFE = ("Motor", "Sirene", "Rotor", "Panzer")

importiert = []
for name in NAMEN:
    pfad = "%s/%s" % (ORDNER, name)
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        unreal.EditorAssetLibrary.delete_asset(pfad)
    datei = os.path.join(QUELLE, name + ".wav")
    if not os.path.isfile(datei):
        unreal.log_warning("LALABERG_SOUND fehlt: " + datei)
        continue
    auftrag = unreal.AssetImportTask()
    auftrag.filename = datei
    auftrag.destination_path = ORDNER
    auftrag.destination_name = name
    auftrag.automated = True
    auftrag.replace_existing = True
    auftrag.save = True
    werkzeuge.import_asset_tasks([auftrag])
    if unreal.EditorAssetLibrary.does_asset_exist(pfad):
        importiert.append(name)
        sound = unreal.load_asset(pfad)
        if any(k in name for k in SCHLEIFE):
            sound.set_editor_property("looping", True)

unreal.log("LALABERG_SOUNDS " + " ".join(importiert))
