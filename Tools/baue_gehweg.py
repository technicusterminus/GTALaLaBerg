# Legt M_Gehweg an: Betonplatten fuer die Gehwege ausserhalb der Altstadt
# (Textur T_Gehweg aus baue_fototexturen.py, ambientCG Concrete033, CC0).
# In der Altstadt bleibt es beim Pflaster (M_Pflaster).
#
#   UnrealEditor-Cmd.exe <uproject> -run=pythonscript -script="Tools/baue_gehweg.py"
#
# Eigenes Skript aus demselben Grund wie baue_pflaster.py: baue_materialien.py
# baut nur die Materialien seiner eigenen Liste neu.
import os
import unreal

quelle = open(os.path.join(unreal.Paths.project_dir(), "Tools", "baue_materialien.py"), encoding="utf-8").read()
exec(quelle.split("\ngebaut = [")[0])

# Kachel 1,2 m: das ist die Groesse einer Gehwegplatte, und in dieser Groesse
# sieht man die Fugen beim Darueberlaufen.
pfad = baue("M_Gehweg", "T_Gehweg_D", 120.0, 0.92, 0.20, 1.0, tiefenkarte="T_Gehweg_N")
unreal.log("LALABERG_MATERIALIEN " + pfad)
