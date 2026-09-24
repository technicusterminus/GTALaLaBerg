# Holt die gewandelten Radiotitel (Tools/Musik/<Sender>/*.wav) ins Spiel und
# schreibt die Senderliste, die das Autoradio zur Laufzeit liest.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/importiere_radio.py"
#
# Ergebnis:
#   /Game/Audio/Radio/<Sender>/<Datei>          die Titel als USoundWave
#   Content/SourceData/Audio/radio.json          Sender, Titel, Urheber, Pfad
#
# Titel und Urheber stehen in Tools/radio_titel.json und werden ueber die
# Nummer im Dateinamen zugeordnet - so steht im Spiel der richtige Name im
# Display, nicht der Dateiname.
import json
import os

import unreal

WURZEL = unreal.Paths.project_dir()
QUELLE = os.path.join(WURZEL, "Tools", "Musik")
LISTE = os.path.join(WURZEL, "Tools", "radio_titel.json")
ORDNER = "/Game/Audio/Radio"
AUSGABE = os.path.join(WURZEL, "Content", "SourceData", "Audio", "radio.json")

werkzeuge = unreal.AssetToolsHelpers.get_asset_tools()
daten = json.load(open(LISTE, encoding="utf-8"))["sender"]
ergebnis = {}

for sender in sorted(os.listdir(QUELLE)):
    ordner = os.path.join(QUELLE, sender)
    if not os.path.isdir(ordner):
        continue
    titel_liste = daten.get(sender, [])
    eintraege = []
    for datei in sorted(os.listdir(ordner)):
        if not datei.endswith(".wav"):
            continue
        name = datei[:-4]
        pfad = "%s/%s/%s" % (ORDNER, sender, name)
        if not unreal.EditorAssetLibrary.does_asset_exist(pfad):
            auftrag = unreal.AssetImportTask()
            auftrag.filename = os.path.join(ordner, datei)
            auftrag.destination_path = "%s/%s" % (ORDNER, sender)
            auftrag.destination_name = name
            auftrag.automated = True
            auftrag.replace_existing = True
            auftrag.save = True
            werkzeuge.import_asset_tasks([auftrag])
        if not unreal.EditorAssetLibrary.does_asset_exist(pfad):
            unreal.log_warning("LALABERG_RADIO nicht importiert: " + datei)
            continue
        klang = unreal.load_asset(pfad)
        klang.set_editor_property("looping", False)
        # Ein Titel ist Minuten lang: nicht beim Laden in den Speicher ziehen,
        # sondern erst kurz vor dem Abspielen - "streaming" heisst in 5.8
        # loading_behavior. Faellt die Eigenschaft weg, bleibt die Voreinstellung.
        try:
            klang.set_editor_property("loading_behavior",
                                      unreal.SoundWaveLoadingBehavior.LOAD_ON_DEMAND)
        except Exception:
            pass
        unreal.EditorAssetLibrary.save_asset(pfad)
        # Nummer aus dem Dateinamen: Sender_NN_Titel
        teile = name.split("_")
        nummer = int(teile[1]) if len(teile) > 1 and teile[1].isdigit() else 0
        angabe = titel_liste[nummer - 1] if 0 < nummer <= len(titel_liste) else {"t": name, "k": ""}
        eintraege.append({"titel": angabe["t"], "kuenstler": angabe["k"], "pfad": "%s.%s" % (pfad, name)})
    ergebnis[sender] = eintraege
    unreal.log("LALABERG_RADIO %s titel=%d" % (sender, len(eintraege)))

os.makedirs(os.path.dirname(AUSGABE), exist_ok=True)
with open(AUSGABE, "w", encoding="utf-8") as datei:
    json.dump({"schema": "lalaberg.radio.2", "sender": ergebnis}, datei, ensure_ascii=False, indent=1)
unreal.log("LALABERG_RADIO liste geschrieben: " + AUSGABE)
