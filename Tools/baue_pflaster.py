# Legt nur M_Pflaster an - rau und matt, fuer Plaetze und Altstadtpflaster.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_pflaster.py"
#
# Pflaster lag bis dahin auf M_Asphalt (Rauheit 0,66). Gegen die Sonne
# gesehen spiegelte der Hauptplatz davon fast weiss. Seit 2026-09-21 baut
# baue_materialien.py an Ort und Stelle neu (die Stadt-Meshes behalten ihre
# Verweise); dieses Skript bleibt, weil M_Pflaster nicht in dessen Liste
# steht. Die Detailtextur ist Kleinpflaster in Segmentboegen (T_Pflaster_D,
# baue_texturen.py) - vorher lag hier das fast einfarbige T_Asphalt_D.

import os
import unreal

quelle = open(os.path.join(unreal.Paths.project_dir(), "Tools", "baue_materialien.py"), encoding="utf-8").read()
# Nur die Werkzeuge uebernehmen, nicht die Liste, die alles neu baut
exec(quelle.split("\ngebaut = [")[0])

pfad = baue("M_Pflaster", "T_Pflaster_D", 300.0, 0.90, 0.24, 1.0, tiefenkarte="T_Pflaster_N")
unreal.log("LALABERG_MATERIALIEN " + pfad)
