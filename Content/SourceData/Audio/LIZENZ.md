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

## Das Autoradio

Die drei Sender im Wagen spielen echte Musik - je rund eine Stunde, ebenfalls
von **Pixabay** unter derselben Inhaltslizenz. Die Titelliste steht in
`Tools/radio_titel.json`; `Tools/hole_radio.py` laedt sie, `Tools/wandle_radio.py`
wandelt sie (32 kHz, Mono, 16 Bit) und `Tools/importiere_radio.py` holt sie nach
`/Game/Audio/Radio/<Sender>/` und schreibt das Programm nach
`Content/SourceData/Audio/radio.json`. Weder die MP3-Quellen noch die WAVs
liegen im Repo - die Liste und die drei Skripte stellen alles wieder her.

### Lech FM (19 Titel)

| Titel auf Pixabay | Urheber |
|---|---|
| Uranus | fanchisanchez |
| 80s Retro Synthwave | Playsound |
| Driving Techno | RibhavAgrawal |
| Synthwave | AlexGrohl |
| Nostalgic Motivating Retro Synth Wave Pop | nickogloire |
| Synthwave Music Neon Drive Horizon | alex-morgan |
| Synthwave Retro 80s | MondaMusic |
| Synthwave Retro 80s II | MondaMusic |
| Synthwave | The_Mountain |
| Broken Beat | alex-morgan |
| Retro Wave | BerryDeep |
| Garage Music | alex-morgan |
| Lounge Retro Wave | Abydos_Music |
| Breakbeat Music | alex-morgan |
| Focus Music | alex-morgan |
| Chillstep | alex-morgan |
| Trip Hop | alex-morgan |
| Dark Wave Synth Instrumental | NickPanek |
| 80s New Wave Instrumental | NickPanek |

### Blasmusik Landsberg (18 Titel)

| Titel auf Pixabay | Urheber |
|---|---|
| Marching Music - President Garfields Inauguration (Sousa) | Nesrality |
| Saint Leo - Polka Rock | NickPanek |
| March of the Royal Trumpets (Sousa) | Nesrality |
| Sylvensteinsee Walzer | u_dfhby5lcqk |
| Majestic Marching Band Anthem | NickPanek |
| The Stars and Stripes Forever (Sousa) | Nesrality |
| The Liberty Bell (Sousa) | Nesrality |
| Brass Band Anthem | NickPanek |
| Echoes of Old Streets | PsyAI |
| The Pit has gone | MediaHobbyist |
| Moldavian Polka | DPStudioMusic |
| German Oktoberfest | Art_Roseman |
| German Oktoberfest Polka | Art_Roseman |
| On Parade (Sousa) | Nesrality |
| Oktoberfest | AliveTunes |
| The Wilfred Waltz Polka | NickPanek |
| Ramona - Bavarian Folk Dance | PeterPiotr |
| The Whippets Waltz | MediaHobbyist |

### Klinik Klassik (10 Titel)

| Titel auf Pixabay | Urheber |
|---|---|
| Debussy - Streichquartett g-moll op. 10 | GregorQuendel |
| Mondscheinsonate | Clavier-Music |
| Piano Classical Music | Tunetank |
| Classical Baroque | Dreamy_Rabbit |
| Song of the Crimson Standard | u_opg8azc0w3 |
| Piano Classical Music II | Tunetank |
| Orchestra 2 | MagentaSix |
| Honor and Sword | Good_B_Music |
| Baroque Pop | alanajordan |
| Baroque Music | Dreamy_Rabbit |
