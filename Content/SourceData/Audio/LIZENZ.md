# Herkunft der Klangdateien

Die meisten Klaenge im Spiel sind gerechnet (siehe `Tools/erzeuge_sounds.py`):
Motor, Schritte, Klecks, Martinshorn, Rotor, Panzerdiesel.

Die Schuesse kommen dagegen aus Aufnahmen von **Pixabay**
(<https://pixabay.com>). Die Pixabay-Inhaltslizenz erlaubt die Nutzung fuer
private und kommerzielle Zwecke ohne Namensnennung; untersagt ist der
Weiterverkauf der Dateien als solche. Genannt werden die Urheber hier
trotzdem.

| Datei im Spiel | Titel auf Pixabay | Urheber |
|---|---|---|
| `SFX_Schuss_Pistole` | Single Pistol Gunshot 3.3 | freesound_community |
| `SFX_Schuss_Maschine` | AKM Gunshot | Ahsbt |
| `SFX_Schuss_Schrot` | Shotgun Blast | Universfield |
| `SFX_Schuss_Rakete` | Rpg 7 Sound Effect | Sovetsky_Rastov72 |
| `SFX_Panzer_Schuss` | 053893_Tank Shots | freesound_community |

Umgewandelt werden sie mit `Tools/wandle_sounds.py` (Blender im Stapelbetrieb,
weil dessen Modul `aud` einen vollstaendigen Audiodekoder mitbringt): 44,1 kHz,
Mono, 16 Bit, vorn beschnitten, hinten ausgeblendet, auf 0,92 ausgesteuert.
Die MP3-Quelldateien selbst liegen nicht im Repo.
