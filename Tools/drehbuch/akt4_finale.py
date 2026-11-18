"""Akt IV: Die Sieger (Weiß, Altstadt), das Finale mit drei Enden und der Epilog."""
from dsl import *

ALLE = ["marta", "xaver", "konrad", "anneliese", "ottmar", "korbi", "lale"]

AKT4 = [
    mission("s_a4_hauptplatz", "Der Wirt", 4, "Historisches Rathaus", ["s_a3_foerster"], 1500, 70, [
        titel("AKT IV", "WEISS – Die Sieger", 4.0),
        szene(schnitt([
            zeile("xaver", "Da kommt sie! Die Frau mit allen vier Farben! Setzen Sie sich, essen Sie, trinken Sie – der Wirt lädt ein!"),
            zeile("lale", "Xaver Gruber."),
            zeile("xaver", "Der Einzige und Letzte. Ich habe ein Gasthaus am schönsten Platz Bayerns, einen Keller voller Bier und einen Ruf wie Donnerhall."),
            zeile("xaver", "Und Sie haben in den letzten Tagen drei meiner Freunde besiegt. Das ist beeindruckend. Das ist auch unhöflich."),
            zeile("lale", "Ich hätte gern über den Ball von 1999 geredet."),
            zeile("xaver", "Der Ball. Immer der Ball. Ein hübscher Ball. Aus Gold, wie jeder weiß."),
        ], ort="Hauptplatz", art="kran", hoehe=28, abstand=40, winkel=20, figuren=["xaver", "lale"])),
        hin("Hauptplatz", "Auf den Hauptplatz – der Wirt wartet beim Marienbrunnen", 0, False),
        szene(schnitt([
            zeile("xaver", "Hören Sie, Lale. Ich mache Ihnen ein Angebot. Alle vier Farben auf einer Jacke – das ist keine Verletzung, das ist eine Krone."),
            zeile("xaver", "Werden Sie meine Nachfolgerin. Der Wirt von morgen. Ein Haus, ein Platz, eine Stadt, die einem zu Füßen liegt."),
            zeile("lale", "Und Franz Echter?"),
            zeile("xaver", "Wer? – Ach der. Ein Mann, der nicht verlieren konnte."),
            zeile("lale_off", "Ich sah ihn an. Er lächelte wie jemand, der sich seit fünfundzwanzig Jahren nicht mehr daran erinnern will, wie man aufhört zu lächeln."),
        ], ort="Hauptplatz", art="umkreise", hoehe=1.8, abstand=5, figuren=["xaver", "lale"])),
    ]),
    mission("s_a4_marktnacht", "Marktnacht", 4, "Holzmarkt", ["s_a4_hauptplatz"], 1900, 90, [
        szene(schnitt([
            zeile("funk", "Der Wirt hat seine Leute auf den Holzmarkt geschickt. Sie sollen verhindern, dass Sie zum Turm kommen. Man nennt das Marktnacht."),
            zeile("funk", "Es ist die gefährlichste Nacht des Jahres. Und die lauteste."),
        ], ort="Holzmarkt", art="kran", hoehe=22, abstand=30, winkel=270)),
        hin("Holzmarkt", "Zum Holzmarkt – hier sammeln sich die Leute des Wirts", 150, True),
        stelle("Den Anführer der Weißen auf dem Holzmarkt stellen", 240),
        halte("Durchhalten – der ganze Platz ist aufgebracht", 15, 3),
        bring("Historisches Rathaus", "Durch die Gassen zum Historischen Rathaus", 160),
        szene(schnitt([
            zeile("lale_off", "Als der Morgen kam, war der Holzmarkt leer. Nur Farbe auf dem Pflaster. Gelb, Blau, Grün – und ein bisschen Weiß."),
            zeile("funk", "Der Weg zum Schmalzturm ist frei. Gehen Sie. Bevor er sich besinnt."),
        ], ort="Historisches Rathaus", art="statisch", hoehe=2.0, abstand=9, winkel=60)),
    ]),
    mission("s_a4_turm", "Der Schmalzturm", 4, "Schmalzturm", ["s_a4_marktnacht"], 2000, 100, [
        szene(schnitt([
            zeile("lale_off", "Der Schmalzturm. Unten ein Tor, oben ein Dach, das aussieht wie ein Schmalzbrot. Und in der Mitte ein Geheimnis."),
        ], ort="Schmalzturm", art="kran", hoehe=60, abstand=45, winkel=150)),
        hin("Schmalzturm", "Zum Schmalzturm – die Schlüssel passen", 0, False),
        markiere("Schmalzturm", "Die Säule am Schmalzturm zeichnen – das Siegel öffnet sich nur unter Farbe"),
        szene(
            schnitt([
                zeile("lale_off", "Die vier Siegelhälften passten zusammen wie ein Puzzle. Das Tor ging auf mit einem Seufzer, als hätte es fünfundzwanzig Jahre die Luft angehalten."),
                zeile("lale_off", "Im Turm stand ein Sockel. Darauf eine Kugel. Nicht golden. Grün."),
            ], ort="Schmalzturm", art="statisch", hoehe=1.5, abstand=3, winkel=30, figuren=["lale"]),
            schnitt([
                zeile("lale", "Das Original. Sie ist grün geworden. Die Goldene Kugel war nie golden."),
                zeile("lale_off", "Unter der Kugel lag ein Brief. Franz Echters Handschrift. Datiert auf die Nacht nach dem Finale."),
                zeile("lale_off", "›Ich habe gesehen, wer den Ball getauscht hat. Xaver, Konrad, Anneliese, Ottmar. Wir alle haben es getan. Ich wollte es gestehen. Man hat mich daran gehindert.‹"),
                zeile("lale_off", "›Wenn jemand das liest: Erzähl es der Stadt. Aber erzähl es mit Mitleid.‹"),
            ], ort="Schmalzturm", art="umkreise", hoehe=1.6, abstand=3.2, figuren=["lale"]),
        ),
        szene(schnitt([
            zeile("funk", "Gut gemacht, Lale. Jetzt nehmen Sie die Kugel und den Brief. Und kommen zum Hauptplatz."),
            zeile("lale", "Sie klingen anders als sonst."),
            zeile("funk", "Ich weiß. Die Stimme hat jetzt einen Namen. Marta Echter. Ich war der Schiedsrichter. Ich war immer der Schiedsrichter."),
            zeile("lale", "Sie haben mich benutzt."),
            zeile("marta", "Ich habe Sie geführt. Das ist ein Unterschied – und manchmal auch nicht. Jetzt kommen Sie. Es geht um alles."),
        ], ort="Schmalzturm", art="statisch", hoehe=1.7, abstand=4, winkel=210, figuren=["lale"])),
    ]),
    mission("s_a4_wirt", "Der Wirt vom Hauptplatz", 4, "Bayertor", ["s_a4_turm"], 3000, 120, [
        szene(schnitt([
            zeile("marta", "Der Wirt hat von der Kugel erfahren. Er weiß, dass es zu Ende ist. Er wird um sein Gasthaus kämpfen, um seinen Namen, um alles."),
            zeile("marta", "Zeichnen Sie das Revier der Weißen, stellen Sie ihn. Und kommen Sie dann zu mir. Ich warte am Hauptplatz."),
            zeile("lale", "Marta."),
            zeile("marta", "Ja?"),
            zeile("lale", "Ich bin noch nicht fertig mit Ihnen."),
            zeile("marta", "Das hoffe ich. Ich wäre sonst enttäuscht."),
        ], ort="Bayertor", art="umkreise", hoehe=3.0, abstand=14, figuren=["marta", "lale"])),
        warte_revier(3, "Revier Altstadt nehmen: vier Wahrzeichen zeichnen, acht Lieferungen, den Wirt vom Hauptplatz stellen"),
        szene(schnitt([
            zeile("xaver", "Gut. Gut. Sie haben gewonnen. Ich habe immer gewonnen. Ich habe vergessen, wie man verliert."),
            zeile("xaver", "Ich wollte nur, dass die Stadt nicht zerbricht. Ein Sieger braucht ein Ende. Ein Ende braucht einen Ball. Einen goldenen."),
            zeile("lale", "Sie haben einen Mann dafür kaputtgemacht."),
            zeile("xaver", "Ja. Das habe ich. Und ich habe jeden Tag daran gedacht. Ich habe nur nie wieder darüber gesprochen."),
            zeile("lale_off", "Er saß auf dem Pflaster, in seiner weißen Jacke, und sah plötzlich sehr klein aus."),
        ], ort="spieler", art="statisch", hoehe=1.5, abstand=3.5, winkel=300, figuren=["xaver", "lale"])),
        gib(geld=500, ruf=0),
    ]),
]

FINALE = [
    mission("s_f_hauptplatz", "Samstagmorgen", 5, "Hauptplatz", ["s_a4_wirt"], 5000, 150, [
        titel("FINALE", "Samstag, kurz vor Sonnenaufgang", 4.0),
        szene(schnitt([
            zeile("lale_off", "Der Hauptplatz im Morgengrauen. Auf dem Marienbrunnen: vier Dosen. Gelb, Blau, Grün, Weiß. Davor ein Dutzend Leute, die so taten, als wären sie zufällig da."),
        ], ort="Hauptplatz", art="kran", hoehe=45, abstand=60, winkel=60, dauer=7)),
        hin("Hauptplatz", "Zum Hauptplatz – Samstag, kurz vor Sonnenaufgang", 0, False),
        szene(
            schnitt([
                zeile("konrad", "Pünktlich. Auf die Minute. Das ist ein Kompliment."),
                zeile("anneliese", "Und zwei Sekunden schneller als erwartet."),
                zeile("ottmar", "Ich habe geschwiegen. Jetzt stehe ich hier. Das muss reichen."),
                zeile("korbi", "Ich bin offiziell nicht hier. Ich habe nur zufällig Dienstschluss."),
            ], ort="Hauptplatz", art="umkreise", hoehe=2.2, abstand=9, figuren=ALLE),
            schnitt([
                zeile("marta", "Lale. Sie haben die Kugel. Sie haben den Brief. Jetzt gehört die Stadt Ihnen."),
                zeile("xaver", "Es ist nicht zu spät. Sie müssen es nicht lesen."),
                zeile("marta", "Doch. Das müssen Sie. Mein Bruder hat dafür bezahlt."),
                zeile("lale_off", "Zehn Gesichter. Zwanzig Augen. Und ich hatte einen Brief in der Hand, der fünfundzwanzig Jahre alt war."),
            ], ort="Hauptplatz", art="statisch", hoehe=1.7, abstand=7, winkel=190, figuren=ALLE),
        ),
        wahl("Was tust du mit der Wahrheit?",
             option("Den Brief vorlesen – die ganze Stadt soll es hören.", ende="wahrheit"),
             option("Die Farben mischen – ein Fest statt eines Prozesses.", ende="frieden"),
             option("Die Kugel behalten – die Stadt gehört jetzt dir.", ende="macht")),
        wenn("ende", "wahrheit", [
            szene(
                schnitt([
                    zeile("lale", "›Ich habe den Ball nicht getauscht. Wer das liest, soll wissen: Ich war es nicht.‹"),
                    zeile("lale", "›Aber erzähl es mit Mitleid.‹"),
                    zeile("chor", "…"),
                ], ort="Hauptplatz", art="kran", hoehe=14, abstand=20, winkel=100, figuren=ALLE),
                schnitt([
                    zeile("xaver", "Das war ich. Ich habe den Ball getauscht. Es tut mir leid."),
                    zeile("konrad", "Und ich habe den Fahrplan verschoben, damit niemand es merkt."),
                    zeile("anneliese", "Ich habe die Boote bereitgestellt. Es tut mir leid."),
                    zeile("ottmar", "Ich habe geschwiegen. Es tut mir mehr leid als alles andere."),
                    zeile("marta", "Danke. Das war alles, was ich wollte. Dass jemand es laut sagt."),
                    zeile("korbi", "Nach Paragraph 14 des Handbuchs muss ich Sie jetzt festnehmen. Alle. Das wird eine lange Schicht."),
                ], ort="Hauptplatz", art="umkreise", hoehe=2.0, abstand=8, figuren=ALLE),
            ),
        ], [
            wenn("ende", "frieden", [
                szene(
                    schnitt([
                        zeile("lale", "Hier. Gelb, Blau, Grün, Weiß. Alle vier Farben. Wir kippen sie zusammen."),
                        zeile("xaver", "Das wird braun."),
                        zeile("lale", "Probieren wir's."),
                        zeile("lale_off", "Ich nahm die vier Dosen, eine nach der anderen. Das Wasser im Marienbrunnen färbte sich zuerst gelb, dann grün, dann blau – und am Ende wurde es doch kein Braun. Es wurde bunt."),
                    ], ort="Hauptplatz", art="kran", hoehe=16, abstand=22, winkel=40, figuren=ALLE),
                    schnitt([
                        zeile("marta", "Das ist nicht, was ich wollte."),
                        zeile("lale", "Das weiß ich. Aber Franz wollte es mit Mitleid. Das hier ist Mitleid."),
                        zeile("marta", "…Vielleicht hatte er recht."),
                        zeile("anneliese", "Ich fahre Sie alle ans Ufer. Zum ersten Farbfest."),
                        zeile("konrad", "Pünktlich um zehn."),
                        zeile("korbi", "Ich schreibe nur die Falschparker auf. Alles andere ist heute nicht mein Beruf."),
                    ], ort="Hauptplatz", art="umkreise", hoehe=2.2, abstand=9, figuren=ALLE),
                ),
            ], [
                szene(
                    schnitt([
                        zeile("lale", "Ich behalte die Kugel."),
                        zeile("marta", "Was?"),
                        zeile("lale", "Wenn die Stadt eine Farbe haben will, dann bin ich sie. Vier Farben auf einer Jacke. Mehr als jeder von euch."),
                        zeile("xaver", "Das hätte ich sagen können."),
                        zeile("konrad", "Das hat er gesagt. Nur schlechter."),
                    ], ort="Hauptplatz", art="statisch", hoehe=1.8, abstand=6, winkel=240, figuren=ALLE),
                    schnitt([
                        zeile("marta", "Ich habe Sie gebaut, Lale. Ich habe Sie gebaut, damit Sie die Wahrheit sagen. Nicht, damit Sie die Krone aufsetzen."),
                        zeile("lale", "Man baut nie nur eine Sache."),
                        zeile("lale_off", "Sie sah mich an wie jemand, der eine Maschine gebaut hat und merkt, dass sie schneller wird als er selbst."),
                        zeile("korbi", "Ich … ich glaube, ich muss mir das Handbuch noch einmal ansehen."),
                    ], ort="Hauptplatz", art="umkreise", hoehe=2.0, abstand=7, figuren=ALLE),
                ),
            ]),
        ]),
    ]),
    mission("s_f_epilog", "Abspann", 5, "auto", ["s_f_hauptplatz"], 0, 0, [
        wenn("ende", "wahrheit", [
            szene(schnitt([
                zeile("lale_off", "Es dauerte drei Wochen, bis die Zeitungen aufhörten. Der Wirt bekam ein Jahr auf Bewährung. Die anderen kamen mit Sozialstunden davon."),
                zeile("lale_off", "Franz Echters Name steht jetzt auf einer Tafel im Schmalzturm. Marta hat sie selbst angebracht."),
                zeile("lale_off", "Sie sagt, sie hat keinen Hass mehr. Ich glaube ihr zur Hälfte. Das ist mehr, als ich erwartet hatte."),
            ], ort="Schmalzturm", art="kran", hoehe=50, abstand=55, winkel=300)),
        ], [
            wenn("ende", "frieden", [
                szene(schnitt([
                    zeile("lale_off", "Das erste Farbfest war ein Chaos. Der Lech wurde an einer Stelle blau, an einer anderen grün. Die Fische sahen sehr überrascht aus."),
                    zeile("lale_off", "Die Goldene Kugel liegt jetzt im Rathaus. Neben einem Zettel: ›Hier ist nie etwas gewesen.‹ Alle wissen, dass das gelogen ist. Alle sind sehr höflich."),
                ], ort="Hauptplatz", art="kran", hoehe=26, abstand=34, winkel=210)),
            ], [
                szene(schnitt([
                    zeile("lale_off", "Landsberg hatte eine neue Farbe. Ich trug sie jeden Tag. Sie stand mir nicht schlecht."),
                    zeile("lale_off", "Marta verließ die Stadt in der ersten Woche. Sie hat nichts mitgenommen außer ein Foto von ihrem Bruder."),
                    zeile("lale_off", "Manchmal, spät nachts, kommt über Funk eine Stimme: ›Gut gemacht, Lale.‹ Und ich weiß nicht mehr, ob ich sie gehört habe oder erfunden."),
                ], ort="Hauptplatz", art="kran", hoehe=26, abstand=34, winkel=210)),
            ]),
        ]),
        titel("FARBKRIEG", "Mitwirkende: Lale · Schwester Marta · Konrad Weidinger · Anneliese Lechner · Ottmar Wald · Xaver Gruber · Wachtmeister Brandl · Schorsch · Pfarrer Alois", 6.0),
        titel("Kein echter Ball wurde bei diesem Film verletzt.", "Alle Namen und Handlungen sind frei erfunden. Nur Landsberg ist echt.", 5.0),
        szene(schnitt([
            zeile("korbi", "Frau Lale. Hier. Ein Strafzettel. Wegen falsch geparktem Paintball-Wagen. Hauptplatz. Sonntag, elf Uhr."),
            zeile("lale", "Das war mitten im Farbfest."),
            zeile("korbi", "Eben. Falsch ist falsch, auch wenn alle lachen."),
        ], ort="Hauptplatz", art="statisch", hoehe=1.7, abstand=5, winkel=20, figuren=["korbi", "lale"])),
    ]),
]
