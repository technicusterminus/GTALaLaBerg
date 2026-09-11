# Legt nur M_Pflaster an - rau und matt, fuer Plaetze und Altstadtpflaster.
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_pflaster.py"
#
# Pflaster lag bis dahin auf M_Asphalt (Rauheit 0,66). Gegen die Sonne
# gesehen spiegelte der Hauptplatz davon fast weiss. baue_materialien.py
# selbst laeuft hier nicht noch einmal: es loescht jedes Material vor dem
# Neuanlegen, und an den bestehenden haengen alle Stadt-Meshes.

import os
import unreal

quelle = open(os.path.join(unreal.Paths.project_dir(), "Tools", "baue_materialien.py"), encoding="utf-8").read()
# Nur die Werkzeuge uebernehmen, nicht die Liste, die alles neu baut
exec(quelle.split("\ngebaut = [")[0])

pfad = baue("M_Pflaster", "T_Asphalt_D", 260.0, 0.92, 0.22, 1.3)
unreal.log("LALABERG_MATERIALIEN " + pfad)
