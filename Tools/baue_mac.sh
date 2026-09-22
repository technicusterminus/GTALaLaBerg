#!/usr/bin/env bash
# Mac-Paket bauen. Muss auf einem Mac laufen: Apples Werkzeuge (Xcode,
# Metal-Shadercompiler, Signatur) gibt es nur dort, eine Mac-Fassung laesst
# sich auf Windows nicht erzeugen.
#
#   Tools/baue_mac.sh [Zielordner]
#
# Voraussetzungen auf dem Mac:
#   - Apple Silicon (M1 oder neuer). Die Stadt ist Nanite-Geometrie, das
#     braucht Metal 3; auf Intel-Macs laeuft sie nicht.
#   - Xcode mit Kommandozeilenwerkzeugen (xcode-select --install)
#   - Unreal Engine 5.8 fuer macOS (Epic Games Launcher)
#   - git und git-lfs (brew install git-lfs; git lfs install)
#   - City Sample Vehicles ueber den Fab-Reiter im Editor nach
#     Content/CitySampleVehicles holen - sie liegen aus Lizenzgruenden nicht
#     im Repo. Ohne sie faehrt der Verkehr als Kastenform.
#
# UE_WURZEL zeigt auf die Engine, falls sie nicht im Standardpfad liegt.
set -euo pipefail

UE_WURZEL="${UE_WURZEL:-/Users/Shared/Epic Games/UE_5.8}"
PROJEKT="$(cd "$(dirname "$0")/.." && pwd)/GTALaLaBerg.uproject"
ZIEL="${1:-$HOME/GTALaLaBerg-Paket}"
UAT="$UE_WURZEL/Engine/Build/BatchFiles/RunUAT.sh"

[ -x "$UAT" ] || { echo "RunUAT.sh nicht gefunden: $UAT (UE_WURZEL setzen)"; exit 1; }
[ -f "$PROJEKT" ] || { echo "Projekt nicht gefunden: $PROJEKT"; exit 1; }

# Development statt Shipping: Log und Konsole bleiben erhalten, die
# Pruefschalter (-LaLaBergSmoke, -LaLaBergFahrtest ...) melden ihr Ergebnis
# wie unter Windows nach Saved/Logs/LaLaBerg-Test.txt im Paket.
"$UAT" BuildCookRun \
  -project="$PROJEKT" \
  -noP4 \
  -platform=Mac \
  -clientconfig=Development \
  -build -cook -stage -pak -iostore \
  -archive -archivedirectory="$ZIEL" \
  -prereqs -unattended -utf8output

echo "Fertig. Paket: $ZIEL"
echo "Probelauf:    \"$ZIEL/Mac/GTALaLaBerg.app/Contents/MacOS/GTALaLaBerg\" -LaLaBergSmoke -windowed -ResX=1600 -ResY=900"
