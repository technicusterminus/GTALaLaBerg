"""Akt II: Die Schnellen (Blau, Lechviertel) und Akt III: Die Stillen (Grün, Klinikum und Süd)."""
from dsl import *

AKT2 = [
    mission("s_a2_ufer", "Über den Lech", 2, "Mutterturm", ["s_a1_schorsch"], 700, 40, [
        titel("AKT II", "BLAU – Die Schnellen", 4.0),
        szene(schnitt([
            zeile("anneliese", "Da ist sie ja. Die Frau mit allen vier Farben. Anneliese Lechner. Mir gehört der Fluss, und was am Fluss liegt."),
            zeile("lale", "Konrad hat Sie erwähnt."),
            zeile("anneliese", "Konrad erwähnt jeden, den er für gefährlich hält. Das sind sehr wenige. Sie sind also gefährlich."),
            zeile("anneliese", "Ich fahre die schnellsten Boote am Lech und die schnellsten Wagen am Ufer. Wer zu den Blauen will, muss mich überholen."),
            zeile("lale", "Ich überhole nur, wenn jemand vor mir fährt."),
            zeile("anneliese", "Dann fahren Sie jetzt zur Johanniskirche. Mein Vorfahrer wartet dort. Er hat etwas, das Ihnen gehört."),
        ], ort="Mutterturm", art="kran", hoehe=22, abstand=30, winkel=60, figuren=["anneliese", "lale"])),
        hin("Johanniskirche", "Ans andere Ufer zur Johanniskirche – der Vorfahrer der Blauen wartet", 170, True),
        szene(schnitt([
            zeile("lale_off", "Der Vorfahrer wartete nicht. Er fuhr los, kaum dass ich bremste. Ein blauer Wagen, der aussah, als würde er auf dem Wasser fahren."),
            zeile("funk", "Er trägt die zweite Siegelhälfte. Bringen Sie ihn zum Stehen. Und zwar so, dass er noch weiß, wer ihn gestellt hat."),
        ], ort="Johanniskirche", art="statisch", hoehe=2.0, abstand=8, winkel=210)),
        stelle("Den Vorfahrer der Blauen stellen", 230),
        bring("Mutterturm", "Zurück zum Mutterturm", 180),
        szene(schnitt([
            zeile("anneliese", "Nicht schlecht. Eine halbe Sekunde langsamer als ich. Aber ich bin ja auch unfair gut."),
            zeile("anneliese", "Die Siegelhälfte bleibt bei mir. Noch. Wenn Sie wirklich wollen, dass sie Ihnen gehört, dann beweisen Sie es bei der Regatta."),
        ], ort="Mutterturm", art="statisch", hoehe=1.8, abstand=4.5, winkel=300, figuren=["anneliese", "lale"])),
    ]),
    mission("s_a2_regatta", "Regatta", 2, "Holzmarkt", ["s_a2_ufer"], 900, 50, [
        szene(schnitt([
            zeile("anneliese", "Die Regatta der Blauen ist keine auf dem Wasser. Sie führt über die Brücken und Gassen am Lech. Wer als Erster wieder auf dem Holzmarkt ist, gewinnt."),
            zeile("anneliese", "Infanterieplatz, Lady-Herkomer-Steg, zurück. Zwei Minuten dreißig. Ich fahre es in zwei zwanzig."),
            zeile("lale", "Dann fahre ich es in zwei zehn."),
            zeile("anneliese", "Schön. Zum ersten Mal in zehn Jahren macht jemand mir das Rennen interessant."),
        ], ort="Holzmarkt", art="umkreise", hoehe=2.0, abstand=6, figuren=["anneliese", "lale"])),
        hin("Infanterieplatz", "Erster Kontrollpunkt: Infanterieplatz", 130, True),
        funk("anneliese", "Zehn Meter vor Ihnen, Lale. Neun. Acht – ach, ich werde langsamer, damit es spannend bleibt!"),
        hin("Lady-Herkomer-Steg", "Zweiter Kontrollpunkt: Lady-Herkomer-Steg", 110, True),
        bring("Holzmarkt", "Zurück zum Holzmarkt – wer zuerst da ist, gewinnt", 110),
        szene(schnitt([
            zeile("anneliese", "Zwei Sekunden. Zwei Sekunden vor mir. Das gibt es nicht."),
            zeile("lale", "Ich hatte einen guten Lehrer. Er war schnell und eingebildet."),
            zeile("anneliese", "Ha! Hier – die zweite Siegelhälfte. Sie haben sie sich verdient. Aber heute Abend im Stadttheater gibt's was zu holen, das Sie noch mehr interessieren wird."),
        ], ort="Holzmarkt", art="statisch", hoehe=1.7, abstand=4.2, winkel=20, figuren=["anneliese", "lale"])),
        gib(geld=0, ruf=15),
    ]),
    mission("s_a2_theater", "Vorhang auf", 2, "Stadttheater", ["s_a2_regatta"], 1000, 55, [
        szene(schnitt([
            zeile("anneliese", "Im Stadttheater gibt man heute ›Der Fischer und seine Farbe‹. Das Stück hat nur einen Zweck: Es hält die Polizei beschäftigt."),
            zeile("anneliese", "Hinter der Bühne lagert ein Koffer. Gelb, Grün und Weiß haben ihn dort versteckt. Nicht aus Freundschaft. Aus Furcht."),
            zeile("lale", "Wovor?"),
            zeile("anneliese", "Vor dem, was darin liegt. Zeichnen Sie das Färbertor – das ist das Signal. Danach geht es los."),
        ], ort="Stadttheater", art="fahrt", hoehe=2.0, abstand=12, winkel=130, figuren=["anneliese", "lale"])),
        markiere("Färbertor", "Das Färbertor zeichnen – das Signal für die Blauen", 200),
        halte("Durchhalten, bis der Vorhang fällt – die Polizei ist unterwegs", 12, 2),
        hin("Stadttheater", "Zurück ins Stadttheater, hinter die Bühne", 120, True),
        szene(
            schnitt([
                zeile("lale_off", "Hinter der Bühne roch es nach Staub und alter Farbe. Der Koffer war kleiner, als ich dachte."),
                zeile("anneliese", "Machen Sie ihn auf. Ich habe ihn nie geöffnet. Ich wollte nicht wissen, wer von uns lügt."),
            ], ort="Stadttheater", art="statisch", hoehe=1.6, abstand=3, winkel=250, figuren=["anneliese", "lale"]),
            schnitt([
                zeile("lale", "Ein Brief. Mit Franz Echters Handschrift."),
                zeile("anneliese", "Lesen Sie vor."),
                zeile("lale", "›Ich habe den Ball nicht getauscht. Er lag schon so da, als ich ihn bekam. Wer das liest, soll wissen: Ich war es nicht.‹"),
                zeile("anneliese", "Er war es nicht. Das wusste ich. Alle haben es immer gewusst, nur keiner hat es gesagt."),
            ], ort="Stadttheater", art="umkreise", hoehe=1.8, abstand=3.5, figuren=["anneliese", "lale"]),
        ),
    ]),
    mission("s_a2_wache", "Zum dritten Mal", 2, "Polizeiinspektion", ["s_a2_theater"], 500, 30, [
        szene(schnitt([
            zeile("korbi", "Das ist das dritte Mal diese Woche. Ich zähle mit."),
            zeile("lale", "Ich habe nichts getan. Außer das Stadttheater zu verlassen."),
            zeile("korbi", "Mit sieben Streifenwagen im Rücken. Nehmen Sie Platz. Ich habe Kaffee. Er ist schlecht."),
        ], ort="Polizeiinspektion", art="statisch", hoehe=1.7, abstand=4.5, winkel=20, figuren=["korbi", "lale"])),
        hin("Polizeiinspektion", "Zur Polizeiinspektion – Wachtmeister Brandl will reden", 180, False),
        szene(schnitt([
            zeile("korbi", "Passen Sie auf, Frau Lale. Ich bin allein auf der Wache, aber ich habe Augen."),
            zeile("korbi", "Der Förster hat Schlüssel zu meinen Zellen. Eigene. Ich kann sie ihm nicht abnehmen, weil er sie von meinem Vorgesetzten hat."),
            zeile("lale", "Warum sagen Sie mir das?"),
            zeile("korbi", "Weil ich ein ehrlicher Polizist bin. Das ist ein einsamer Beruf."),
            zeile("korbi", "Wenn Sie in den Süden fahren, zu den Grünen: Rechnen Sie damit, dass die Polizei nicht auf Ihrer Seite ist. Auch wenn ich es bin."),
        ], ort="Polizeiinspektion", art="umkreise", hoehe=1.7, abstand=4, figuren=["korbi", "lale"])),
    ]),
    mission("s_a2_fluss", "Die Fährfrau", 2, "Mutterturm", ["s_a2_wache"], 1200, 70, [
        szene(schnitt([
            zeile("anneliese", "Jetzt zeigen Sie mir, dass Sie nicht nur schnell sind, sondern auch hartnäckig."),
            zeile("anneliese", "Zeichnen Sie die vier Wahrzeichen der Blauen. Drei Rennen. Und dann stellen Sie mich, Lale. Ich fahre schnell. Ich fahre sehr schnell."),
            zeile("lale", "Ich weiß."),
            zeile("anneliese", "Dann wissen Sie, dass es ein Vergnügen wird."),
        ], ort="Mutterturm", art="kran", hoehe=18, abstand=28, winkel=330, figuren=["anneliese", "lale"])),
        warte_revier(1, "Revier Lechviertel nehmen: vier Wahrzeichen zeichnen, drei Rennen, die Fährfrau stellen"),
        szene(
            schnitt([
                zeile("anneliese", "Au. Mein Lack. Mein schöner Lack. Das war's. Ich gebe auf."),
                zeile("anneliese", "Hier. Der Rotorschlüssel. Der Hubschrauber auf dem Klinikumsdach gehört jetzt Ihnen. Ich habe ihn nie benutzt, ich fahre lieber."),
                zeile("lale", "Sie lassen mich einfach gewinnen?"),
                zeile("anneliese", "Ich lasse niemanden gewinnen. Sie haben gewonnen. Das ist ein großer Unterschied."),
            ], ort="spieler", art="statisch", hoehe=1.6, abstand=4, winkel=40, figuren=["anneliese", "lale"]),
            schnitt([
                zeile("anneliese", "Und hören Sie, Lale. Franz hat es nicht getan. Ich war dabei, als er seine Hälfte vergraben hat. Er hat geweint."),
                zeile("anneliese", "Der Wirt hat getauscht. Und der Förster hat es gesehen und geschwiegen. Er ist der Nächste. Passen Sie auf sich auf. Er sieht alles, was sich im Wald bewegt."),
            ], ort="spieler", art="umkreise", hoehe=1.8, abstand=5, figuren=["anneliese", "lale"]),
        ),
        gib(geld=300, ruf=0),
    ]),
]

AKT3 = [
    mission("s_a3_klinik", "Nachtschicht", 3, "kbo-Lech-Mangfall-Klinik", ["s_a2_fluss"], 1100, 55, [
        titel("AKT III", "GRÜN – Die Stillen", 4.0),
        szene(schnitt([
            zeile("marta", "Sie sind weit gekommen. Weiter, als ich dachte. Setzen Sie sich. Nein, nicht dahin – dort liegen Akten."),
            zeile("lale", "Franz Echters Akten?"),
            zeile("marta", "Sie wissen es also. Pfarrer Alois, nehme ich an. Er konnte noch nie schweigen."),
            zeile("marta", "Franz war mein Bruder. Er war Kapitän der Grünen. Und er war ein anständiger Mensch."),
            zeile("marta", "Man hat ihn beschuldigt, den Ball getauscht zu haben. Er hat nie wieder gesprochen. Zwei Jahre später war er tot."),
        ], ort="kbo-Lech-Mangfall-Klinik", art="statisch", hoehe=1.6, abstand=3.6, winkel=290, figuren=["marta", "lale"])),
        hin("kbo-Lech-Mangfall-Klinik", "In die kbo-Lech-Mangfall-Klinik – Marta wartet im Archiv", 0, False),
        szene(schnitt([
            zeile("marta", "In der Nacht, als Franz starb, hatte ich Dienst. Er sagte nur: ›Der Wirt hat es gewusst.‹ Dann war er still."),
            zeile("lale", "Wissen Sie, wer der Schiedsrichter ist?"),
            zeile("marta", "Ich? Nein. Nicht, dass ich wüsste."),
            zeile("lale_off", "Sie hatte zu schnell geantwortet. Ich merkte es mir."),
        ], ort="kbo-Lech-Mangfall-Klinik", art="umkreise", hoehe=1.7, abstand=4, figuren=["marta", "lale"])),
        bring("Klinikum", "Einen Patienten ins Klinikum bringen – schnell und ohne Rempler", 150),
        funk("marta", "Danke. Er lebt. Das ist selten genug, wenn Sie fahren."),
    ]),
    mission("s_a3_friedhof", "Stille Schicht", 3, "Friedhofskirche zur Heiligen Dreifaltigkeit", ["s_a3_klinik"], 1300, 65, [
        szene(schnitt([
            zeile("lale_off", "Der Friedhof lag still wie ein ausgeschaltetes Radio. Franz Echters Grab hatte frische Blumen. Weiß."),
            zeile("ottmar", "Er hat Weiß nie gemocht."),
        ], ort="Friedhofskirche zur Heiligen Dreifaltigkeit", art="kran", hoehe=16, abstand=24, winkel=180)),
        hin("Friedhofskirche zur Heiligen Dreifaltigkeit", "Zur Friedhofskirche – ein Grab hat frische Blumen", 0, False),
        szene(schnitt([
            zeile("ottmar", "Ottmar Wald. Förster. Wer in den Wald geht, kommt an mir vorbei. Sie sind die Frau mit den vier Farben."),
            zeile("lale", "Sie waren dabei, damals."),
            zeile("ottmar", "Im Wald fällt ein Baum, und keiner sieht ihn. Ich habe vieles gesehen. Ich habe nichts gesagt."),
            zeile("ottmar", "Gehen Sie. Sofort. Die Polizei kommt gleich. Meine Polizei."),
        ], ort="Friedhofskirche zur Heiligen Dreifaltigkeit", art="statisch", hoehe=1.8, abstand=4.5, winkel=100, figuren=["ottmar", "lale"])),
        halte("Durchhalten – der Förster hat die Polizei gerufen", 12, 2),
        hin("Christuskirche", "Weg von hier – zur Christuskirche", 140, True),
    ]),
    mission("s_a3_wald", "Der Waldweg", 3, "Christuskirche", ["s_a3_friedhof"], 1400, 70, [
        szene(schnitt([
            zeile("funk", "Der Förster schickt einen Boten in den Wald. Er trägt den Schlüsselbund. Wenn Sie den Boten stellen, hat der Förster nichts mehr in der Hand."),
            zeile("lale", "Wo ist der Wald?"),
            zeile("funk", "Überall. Aber heute nur dort, wo ein Panzer als Hochsitz dient. Ja, ein Panzer. Ich mache keine Scherze."),
        ], ort="Christuskirche", art="umkreise", hoehe=2.4, abstand=9)),
        stelle("Den Boten des Försters stellen", 240),
        bring("Klinikum", "Den Schlüsselbund ins Klinikum zu Marta bringen", 200),
        szene(schnitt([
            zeile("marta", "Diese Schlüssel … ich kenne sie. Der große passt in die Tür zum Schmalzturm. Der kleine zu Franz' Spind."),
            zeile("lale", "Der Förster wollte ihn behalten."),
            zeile("marta", "Natürlich. Wer das Schloss hat, hat die Wahrheit."),
        ], ort="Klinikum", art="statisch", hoehe=1.7, abstand=4, winkel=330, figuren=["marta", "lale"])),
    ]),
    mission("s_a3_foerster", "Der Förster", 3, "Klinikum", ["s_a3_wald"], 1800, 90, [
        szene(schnitt([
            zeile("marta", "Der Förster hat zu viel gesehen. Er hat meinen Bruder untergehen lassen. Er muss reden."),
            zeile("lale", "Er hat die Polizei auf seiner Seite."),
            zeile("marta", "Dann sorgen Sie dafür, dass sie ihn nicht erreichen. Zeichnen Sie das Revier, fahren Sie drei Verfolgungen, und stellen Sie ihn."),
            zeile("marta", "Ich verlasse mich auf Sie, Lale."),
        ], ort="Klinikum", art="kran", hoehe=45, abstand=70, winkel=100, figuren=["marta", "lale"])),
        warte_revier(2, "Revier Klinikum und Süd nehmen: vier Wahrzeichen zeichnen, drei Verfolgungen, den Förster stellen"),
        szene(
            schnitt([
                zeile("ottmar", "Ich habe mein ganzes Leben lang nichts gesagt. Jetzt sage ich es."),
                zeile("ottmar", "Der Wirt hat den Ball getauscht. Mit Hilfe der Gelben, der Blauen und mir. Wir haben alle mitgemacht."),
                zeile("ottmar", "Franz hat es herausgefunden. Er wollte reden. Wir haben ihn zum Schweigen gebracht – nicht mit Gewalt. Mit Schuldzuweisung."),
                zeile("lale", "Das Einzige, was ein ehrlicher Mensch nicht überlebt."),
                zeile("ottmar", "Hier. Der Zündschlüssel. Der Panzer im Wald gehört Ihnen. Er ist alt, aber er fährt. Und der kleine, hier – der Spindschlüssel. Franz' Beweise liegen im Turm."),
            ], ort="spieler", art="statisch", hoehe=1.7, abstand=4.5, winkel=300, figuren=["ottmar", "lale"]),
            schnitt([
                zeile("ottmar", "Sagen Sie Marta, dass es mir leidtut. Und sagen Sie ihr: Sie soll das nicht für Rache tun. Rache macht einsam."),
            ], ort="spieler", art="umkreise", hoehe=1.9, abstand=5, figuren=["ottmar", "lale"]),
        ),
        gib(geld=500, ruf=0),
    ]),
]
