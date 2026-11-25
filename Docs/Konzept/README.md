# Konzeptbilder

Screenshots aus dem Godot-Spiel (`*-vorlage.png`), von einem Bildmodell über OpenRouter
überarbeitet (`stadt.png`, `autos.png`, `figuren.png`; Modell `google/gemini-3-pro-image`,
rund 0,14 USD je Bild). Das sind **2D-Zielbilder**, keine Spielassets: Sie zeigen, wohin
Fassaden, Dächer, Autos und Figuren stilistisch sollen. Erzeugt mit
`Tools/ueberarbeite_bild.py <Modell> <Eingabe.png> <Ausgabe.png> "<Auftrag>"`
(Schlüssel über `OPENROUTER_API_KEY` bzw. den Proxy).

Aus der Stadt-Vorlage wurden die Hausfassaden im Shader (`Godot/shader/flach.gdshader`,
Stil 3) übernommen: eingelassene Fenster mit Laibung, Rahmen, Bank und Fensterläden.
