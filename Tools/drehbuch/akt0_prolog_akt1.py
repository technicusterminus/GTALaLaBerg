"""Prolog und Akt I: Die Pünktlichen (Gelb, Vorstadt-Nord)."""
from dsl import *

PROLOG = [
    mission("s_p1", "Kaltstart", 0, "auto", [], 0, 0, [
        titel("FARBKRIEG", "Ein Film in vier Akten – gedreht in Landsberg am Lech", 5.0),
        szene(
            schnitt([
                zeile("lale_off", "Landsberg am Lech. Ein Fluss, ein Turm, der aussieht wie ein Schmalzbrot – und vier Banden, die sich seit fünfundzwanzig Jahren mit Farbe bewerfen."),
                zeile("lale_off", "Das ist die Geschichte, wie ich aufgewacht bin. Mit Kopfweh. Und einer Pistole, die nicht meine war."),
            ], ort="Klinikum", art="kran", hoehe=70, abstand=90, winkel=200, dauer=9),
            schnitt([
                zeile("marta", "Na, wieder da? Sie haben drei Stunden auf der Straße gelegen. Mit vier Farben auf der Jacke."),
                zeile("lale", "Welche vier?"),
                zeile("marta", "Gelb, Blau, Grün, Weiß. Alle. Das schafft sonst keiner, ohne dass es wehtut."),
                zeile("lale", "Und die Pistole?"),
                zeile("marta", "Steckte in Ihrer Hand. Paintball. Geliehen von jemandem, der sie zurückhaben will."),
                zeile("lale", "Von wem?"),
                zeile("marta", "Das steht auf dem Zettel in Ihrer Tasche."),
                zeile("lale_off", "Der Waffenstillstand endet am Samstag. Komm zum Bahnhof. – S."),
                zeile("marta", "S. wie Schiedsrichter. Oder wie: Sie haben ein Problem."),
            ], ort="spieler", art="umkreise", hoehe=1.8, abstand=4.5, figuren=["marta", "lale"]),
            schnitt([
                zeile("marta", "Hören Sie zu: Landsberg ist aufgeteilt. Gelb am Bahnhof, Blau am Lech, Grün rund um die Klinik, Weiß in der Altstadt."),
                zeile("marta", "Seit dem Finale von 1999 herrscht Waffenstillstand. Frieden ist das nicht. Nur Pause."),
                zeile("lale", "Und was habe ich damit zu tun?"),
                zeile("marta", "Sie sind die Erste seit Jahren, die alle vier Farben gleichzeitig trägt. Das ist entweder ein Wunder oder ein Todesurteil."),
                zeile("marta", "Der Wagen vor der Tür gehört Ihnen. Fragen Sie nicht, wem er vorher gehört hat."),
            ], ort="spieler", art="statisch", hoehe=1.6, abstand=3.2, winkel=40, figuren=["marta", "lale"]),
        ),
        im_wagen("Steig in den Wagen vor dem Klinikum – Taste F"),
    ]),
    mission("s_p2", "Der Schiedsrichter", 0, "auto", ["s_p1"], 200, 15, [
        funk("funk", "Guten Morgen, Lale. Ja, ich kenne Ihren Namen. Ich kenne alle Namen."),
        funk("funk", "Fahren Sie zum Bahnhof. Die Gelben warten. Pünktlichkeit ist dort keine Tugend, sondern Religion."),
        hin("Bahnhof", "Zum Bahnhof fahren – die Gelben warten", 360, True),
        szene(
            schnitt([
                zeile("konrad", "Sieben Uhr einundvierzig. Eine Minute zu spät. Das sind sechzig Sekunden, in denen ein Zug hätte einfahren können."),
                zeile("lale", "Ich wusste nicht, dass ich verabredet war."),
                zeile("konrad", "Jeder ist verabredet. Mit dem Fahrplan. Konrad Weidinger, Dispatcher der Gelben. Sie tragen unsere Farbe auf der Schulter."),
                zeile("lale", "Und drei andere auf der Jacke."),
                zeile("konrad", "Das ist der Teil, der mir Sorgen macht."),
            ], ort="Bahnhof", art="fahrt", hoehe=2.0, abstand=14, winkel=300, figuren=["konrad", "lale"]),
            schnitt([
                zeile("konrad", "In Landsberg fährt nichts ohne uns. Taxis, Lieferungen, Krankenfahrten – alles läuft über unsere Säulen."),
                zeile("konrad", "Wer hier fährt, fährt für die Gelben. Oder gar nicht."),
            ], ort="Bahnhof", art="statisch", hoehe=1.7, abstand=3.5, winkel=20, figuren=["konrad", "lale"]),
        ),
        wahl("Wie antwortest du dem Dispatcher?",
             option("»Ich fahre für niemanden.«", ton="frech"),
             option("»Ich höre erst einmal zu.«", ton="klug")),
        wenn("ton", "frech", [
            szene(schnitt([
                zeile("konrad", "Mut. Bei uns nennt man das Verspätung im Kopf."),
                zeile("konrad", "Aber gut. Frech fährt sich schneller. Drei Taxifahrten, Lale. Dann reden wir weiter."),
            ], ort="Bahnhof", art="statisch", hoehe=1.7, abstand=3.5, winkel=340, figuren=["konrad", "lale"])),
        ], [
            szene(schnitt([
                zeile("konrad", "Zuhören. Der erste Schritt zur Pünktlichkeit. Sie gefallen mir."),
                zeile("konrad", "Drei Taxifahrten, Lale. Zeigen Sie mir, dass Sie ankommen. Dann reden wir weiter."),
            ], ort="Bahnhof", art="statisch", hoehe=1.7, abstand=3.5, winkel=340, figuren=["konrad", "lale"])),
        ]),
        funk("funk", "Bemerkenswert. Er hat Ihnen kein einziges Mal gedroht. Das macht er sonst bei jedem zweiten Satz."),
    ]),
]

AKT1 = [
    mission("s_a1_fahrplan", "Fahrplan", 1, "Bahnhof", ["s_p2"], 400, 25, [
        titel("AKT I", "GELB – Die Pünktlichen", 4.0),
        szene(schnitt([
            zeile("konrad", "Landsberg läuft nach Fahrplan. Meinem. Und heute ist ein Kurier der Gelben nicht angekommen."),
            zeile("konrad", "Er trug ein Päckchen. Klein, schwer, in gelbes Papier gewickelt. Wachssiegel obendrauf."),
            zeile("lale", "Was war drin?"),
            zeile("konrad", "Das geht Sie nichts an. Fahren Sie zur Stadtverwaltung. Dort wurde er zuletzt gesehen."),
        ], ort="Bahnhof", art="umkreise", hoehe=2.2, abstand=5, figuren=["konrad", "lale"])),
        hin("Stadtverwaltung", "Zur Stadtverwaltung – dort wurde der Kurier zuletzt gesehen", 170, True),
        szene(schnitt([
            zeile("lale_off", "Der Kurier war weg. Nur sein Mützenband lag noch da, blau von oben bis unten."),
            zeile("lale_off", "Blau. Wer bei den Gelben ein blaues Band hinterlässt, will, dass man es findet."),
            zeile("funk", "Das Päckchen sitzt in einem Wagen mit rosa Heckleuchte. Er fährt nach Westen. Schnell."),
        ], ort="Stadtverwaltung", art="statisch", hoehe=2.0, abstand=7, winkel=130)),
        stelle("Den flüchtenden Wagen mit dem Päckchen stellen", 220),
        bring("Bahnhof", "Das Päckchen zurück zum Bahnhof bringen", 200),
        szene(schnitt([
            zeile("konrad", "Siegel unversehrt. Und doch – sehen Sie das? Die Prägung."),
            zeile("lale", "Ein Ball. Mit einem Sprung drin."),
            zeile("konrad", "Eine von vier Hälften des Siegels von 1999. Wenn alle vier Mannschaften ihre Hälfte zusammenlegen, öffnet sich der Schmalzturm."),
            zeile("konrad", "Und wer hinter diesem Päckchen her ist, will nicht, dass es zusammenkommt."),
        ], ort="Bahnhof", art="fahrt", hoehe=1.9, abstand=9, winkel=40, figuren=["konrad", "lale"])),
        gib(geld=0, ruf=10),
    ]),
    mission("s_a1_wache", "Wachtmeister Brandl", 1, "Polizeiinspektion", ["s_a1_fahrplan"], 450, 25, [
        szene(schnitt([
            zeile("korbi", "Halt. Stopp. Polizei. Bitte steigen Sie nicht aus. Oder doch. Ich muss kurz im Handbuch nachsehen."),
            zeile("lale", "Wachtmeister, wir stehen vor Ihrer Wache."),
            zeile("korbi", "Eben. Das macht es einfacher. Korbinian Brandl, Polizeiinspektion Landsberg. Sie haben einen Paintball-Wagen."),
            zeile("lale", "Ich habe auch ein Recht auf eine Meinung dazu."),
            zeile("korbi", "Haben Sie nicht. Stillhalten bitte. Ich nehme Ihre Personalien auf. Das dauert."),
        ], ort="Polizeiinspektion", art="statisch", hoehe=1.7, abstand=5, winkel=200, figuren=["korbi", "lale"])),
        halte("Stillhalten – der Wachtmeister blättert im Handbuch", 8, 1),
        szene(schnitt([
            zeile("korbi", "So. Verwarnung. Und noch etwas, vertraulich: Wenn Sie in meinem Bezirk Säulen anmalen, schreibe ich Sie auf."),
            zeile("lale", "Verstanden."),
            zeile("korbi", "Nicht dass ich es verhindern könnte. Ich bin allein. Aber schreiben, das kann ich."),
        ], ort="Polizeiinspektion", art="statisch", hoehe=1.7, abstand=4.5, winkel=150, figuren=["korbi", "lale"])),
        markiere("Polizeiinspektion", "Die Säule der Polizeiinspektion zeichnen – direkt vor seiner Nase"),
        szene(schnitt([
            zeile("korbi", "Das war jetzt Absicht."),
            zeile("lale", "Sie standen im Weg."),
            zeile("korbi", "Ich stehe immer im Weg. Das ist mein Beruf. – Ach, und Frau Lale: Im Archiv von Sankt Katharina liegen Fotos von 1999. Der Pfarrer weiß mehr, als er sagt."),
        ], ort="Polizeiinspektion", art="statisch", hoehe=1.7, abstand=4.5, winkel=60, figuren=["korbi", "lale"])),
    ]),
    mission("s_a1_archiv", "Das Archiv", 1, "Sankt Katharina", ["s_a1_wache"], 520, 30, [
        szene(schnitt([
            zeile("pfarrer", "Der Herr sei mit dir. Und, wenn ich das sagen darf, die Gelben sind es nicht."),
            zeile("lale", "Wachtmeister Brandl meinte, Sie hätten Fotos von 1999."),
            zeile("pfarrer", "Ich habe Fotos von allem. Man nennt das Pfarrchronik. Kommen Sie mit in die Sakristei."),
        ], ort="Sankt Katharina", art="kran", hoehe=14, abstand=22, winkel=250, figuren=["pfarrer", "lale"])),
        hin("Sankt Katharina", "In die Sakristei von Sankt Katharina", 0, False),
        szene(
            schnitt([
                zeile("pfarrer", "Hier. Das Finale 1999. Vier Kapitäne, ein Schiedsrichter, die Goldene Kugel."),
                zeile("lale", "Ein Gesicht ist durchgestrichen."),
                zeile("pfarrer", "Franz Echter, Kapitän der Grünen. Nach dem Spiel hat man ihn des Betrugs beschuldigt. Er ist nie darüber hinweggekommen."),
                zeile("lale", "Echter. Wie die Schwester im Klinikum."),
                zeile("pfarrer", "Ja. Marta ist seine Schwester. Fragen Sie sie nicht danach. Noch nicht."),
            ], ort="Sankt Katharina", art="statisch", hoehe=1.5, abstand=2.8, winkel=330, figuren=["pfarrer", "lale"]),
            schnitt([
                zeile("lale_off", "Ich sah mir das Foto lange an. Vier Männer in Farben. Einer in Weiß, sehr breit, sehr zufrieden. Der Ball in seiner Hand glänzte golden."),
                zeile("lale_off", "Zu golden. Auf einem Foto von 1999 glänzt nichts so frisch wie Lack von gestern."),
            ], ort="Sankt Katharina", art="umkreise", hoehe=2.2, abstand=6),
        ),
        bring("Bahnhof", "Das Foto zu Konrad Weidinger bringen", 260),
        szene(schnitt([
            zeile("konrad", "Der Ball war schon vor dem Anpfiff gestrichen worden? Das heißt, jemand hat das Finale entschieden, bevor es begann."),
            zeile("lale", "Wer war der Mann in Weiß?"),
            zeile("konrad", "Xaver Gruber. Der Wirt vom Hauptplatz. Der Mann, der immer gewinnt."),
            zeile("konrad", "Das erklärt vieles. Und es macht mir Angst. Vor allem, dass ich es nie gesehen habe."),
        ], ort="Bahnhof", art="statisch", hoehe=1.8, abstand=4, winkel=100, figuren=["konrad", "lale"])),
    ]),
    mission("s_a1_dispatcher", "Der Dispatcher", 1, "Bahnhof", ["s_a1_archiv"], 800, 50, [
        szene(schnitt([
            zeile("konrad", "Ich helfe Ihnen, Lale. Aber nicht aus Freundschaft. Aus Berechnung."),
            zeile("konrad", "Beweisen Sie, dass Sie nicht nur fahren, sondern ankommen: Zeichnen Sie die vier Wahrzeichen der Gelben, fahren Sie drei Taxifahrten – und stellen Sie mich."),
            zeile("lale", "Sie stellen? Ich dachte, Sie helfen mir."),
            zeile("konrad", "Eben. Ein Dispatcher, der sich nicht fangen lässt, ist kein Dispatcher. Ich fahre dann los. Pünktlich."),
        ], ort="Bahnhof", art="umkreise", hoehe=2.0, abstand=6, figuren=["konrad", "lale"])),
        warte_revier(0, "Revier Vorstadt-Nord nehmen: vier Wahrzeichen zeichnen, drei Taxifahrten, den Dispatcher stellen"),
        szene(
            schnitt([
                zeile("konrad", "Sieben Uhr einundvierzig. Schon wieder zu spät. Diesmal ich."),
                zeile("konrad", "Hier. Der Werkstattschlüssel. Die Lackiererei arbeitet ab sofort umsonst für Sie, und Ihr Wagen steckt mehr weg."),
                zeile("lale", "Warum tun Sie das?"),
                zeile("konrad", "Weil Sie angekommen sind, Lale. Das war die Prüfung. Alles andere war Fahrplan."),
            ], ort="spieler", art="statisch", hoehe=1.6, abstand=4, winkel=20, figuren=["konrad", "lale"]),
            schnitt([
                zeile("konrad", "Und noch etwas: Fragen Sie in der Lackiererei nach Schorsch. Er hat eine Farbe gemischt, die es nicht geben dürfte."),
                zeile("konrad", "Ein Gold, das in der Sonne grün wird. Das war 1999 auf dem Ball."),
            ], ort="spieler", art="umkreise", hoehe=1.8, abstand=5, figuren=["konrad", "lale"]),
        ),
        gib(geld=300, ruf=0),
    ]),
    mission("s_a1_schorsch", "Gold, das grün wird", 1, "Lackiererei", ["s_a1_dispatcher"], 600, 40, [
        szene(schnitt([
            zeile("schorsch", "Servus! Konrad hat angerufen. Sie sind die mit den vier Farben. Mei, das hab ich noch nie gesehen."),
            zeile("lale", "Er sagte, Sie hätten ein Gold gemischt, das grün wird."),
            zeile("schorsch", "Ich hab's nicht gemischt, ich hab's gefunden. In einer Dose, hinten im Regal, Aufschrift: ›Ball, Final 99, Rückgabe unerwünscht‹."),
            zeile("schorsch", "Das Gold ist ein Lack mit Kupferanteil. In der Sonne wird er grün. Auf dem Foto sieht man's nicht. In echt schon."),
        ], ort="Lackiererei", art="statisch", hoehe=1.8, abstand=5, winkel=160, figuren=["schorsch", "lale"])),
        hin("Lackiererei", "In die Lackiererei fahren – Schorsch wartet", 0, True),
        szene(schnitt([
            zeile("schorsch", "Der echte Pokal der Goldenen Kugel ist grün geworden, als man ihn in die Sonne stellte. Man hat ihn also ausgetauscht."),
            zeile("lale", "Gegen was?"),
            zeile("schorsch", "Gegen einen gestrichenen. Gelb. Dann gold. Wer das gemacht hat, brauchte Zeit, Lack und eine Werkstatt wie meine."),
            zeile("schorsch", "Mir fällt nur einer ein, der hier damals so viel Farbe bestellt hat: Xaver Gruber. Der Wirt."),
            zeile("lale", "Wieder er."),
        ], ort="Lackiererei", art="umkreise", hoehe=1.9, abstand=5.5, figuren=["schorsch", "lale"])),
        funk("funk", "Akt I ist vorbei. Einer von vier Köpfen liegt am Boden. Die Blauen am Lech wissen es bereits."),
    ]),
]
