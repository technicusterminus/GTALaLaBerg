# FARBKRIEG – Drehbuch

Ein Film in vier Akten, gedreht in Landsberg am Lech. Dieses Skript entsteht aus `Tools/drehbuch/` (nicht von Hand ändern) und wird vom Spiel als `Content/SourceData/Story/drehbuch.json` gelesen.

## Figuren

- **Lale**
- **Schwester Marta**
- **Der Schiedsrichter** (nur Stimme)
- **Konrad Weidinger**
- **Anneliese Lechner**
- **Ottmar Wald**
- **Xaver Gruber**
- **Wachtmeister Brandl**
- **Schorsch**
- **Pfarrer Alois**

Alle Namen und Handlungen sind frei erfunden. Nur Landsberg ist echt.

## Prolog

### Kaltstart
*beginnt von selbst · Lohn 0 € · Ruf +0*

**[TITEL]** FARBKRIEG – Ein Film in vier Akten – gedreht in Landsberg am Lech

> **SZENE** – Klinikum, kran
> **LALE (OFF):** Landsberg am Lech. Ein Fluss, ein Turm, der aussieht wie ein Schmalzbrot – und vier Banden, die sich seit fünfundzwanzig Jahren mit Farbe bewerfen.  
> **LALE (OFF):** Das ist die Geschichte, wie ich aufgewacht bin. Mit Kopfweh. Und einer Pistole, die nicht meine war.  
>
> **SZENE** – spieler, umkreise
> **SCHWESTER MARTA:** Na, wieder da? Sie haben drei Stunden auf der Straße gelegen. Mit vier Farben auf der Jacke.  
> **LALE:** Welche vier?  
> **SCHWESTER MARTA:** Gelb, Blau, Grün, Weiß. Alle. Das schafft sonst keiner, ohne dass es wehtut.  
> **LALE:** Und die Pistole?  
> **SCHWESTER MARTA:** Steckte in Ihrer Hand. Paintball. Geliehen von jemandem, der sie zurückhaben will.  
> **LALE:** Von wem?  
> **SCHWESTER MARTA:** Das steht auf dem Zettel in Ihrer Tasche.  
> **LALE (OFF):** Der Waffenstillstand endet am Samstag. Komm zum Bahnhof. – S.  
> **SCHWESTER MARTA:** S. wie Schiedsrichter. Oder wie: Sie haben ein Problem.  
>
> **SZENE** – spieler, statisch
> **SCHWESTER MARTA:** Hören Sie zu: Landsberg ist aufgeteilt. Gelb am Bahnhof, Blau am Lech, Grün rund um die Klinik, Weiß in der Altstadt.  
> **SCHWESTER MARTA:** Seit dem Finale von 1999 herrscht Waffenstillstand. Frieden ist das nicht. Nur Pause.  
> **LALE:** Und was habe ich damit zu tun?  
> **SCHWESTER MARTA:** Sie sind die Erste seit Jahren, die alle vier Farben gleichzeitig trägt. Das ist entweder ein Wunder oder ein Todesurteil.  
> **SCHWESTER MARTA:** Der Wagen vor der Tür gehört Ihnen. Fragen Sie nicht, wem er vorher gehört hat.  
>

- *im_wagen* – Steig in den Wagen vor dem Klinikum – Taste F

### Der Schiedsrichter
*beginnt von selbst · Lohn 200 € · Ruf +15*

**[FUNK] Der Schiedsrichter:** Guten Morgen, Lale. Ja, ich kenne Ihren Namen. Ich kenne alle Namen.

**[FUNK] Der Schiedsrichter:** Fahren Sie zum Bahnhof. Die Gelben warten. Pünktlichkeit ist dort keine Tugend, sondern Religion.

- *hin* – Zum Bahnhof fahren – die Gelben warten
> **SZENE** – Bahnhof, fahrt
> **KONRAD WEIDINGER:** Sieben Uhr einundvierzig. Eine Minute zu spät. Das sind sechzig Sekunden, in denen ein Zug hätte einfahren können.  
> **LALE:** Ich wusste nicht, dass ich verabredet war.  
> **KONRAD WEIDINGER:** Jeder ist verabredet. Mit dem Fahrplan. Konrad Weidinger, Dispatcher der Gelben. Sie tragen unsere Farbe auf der Schulter.  
> **LALE:** Und drei andere auf der Jacke.  
> **KONRAD WEIDINGER:** Das ist der Teil, der mir Sorgen macht.  
>
> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** In Landsberg fährt nichts ohne uns. Taxis, Lieferungen, Krankenfahrten – alles läuft über unsere Säulen.  
> **KONRAD WEIDINGER:** Wer hier fährt, fährt für die Gelben. Oder gar nicht.  
>

**[WAHL]** Wie antwortest du dem Dispatcher?
- »Ich fahre für niemanden.«
- »Ich höre erst einmal zu.«

**[WENN ton = frech]**

> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Mut. Bei uns nennt man das Verspätung im Kopf.  
> **KONRAD WEIDINGER:** Aber gut. Frech fährt sich schneller. Drei Taxifahrten, Lale. Dann reden wir weiter.  
>

**[SONST]**

> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Zuhören. Der erste Schritt zur Pünktlichkeit. Sie gefallen mir.  
> **KONRAD WEIDINGER:** Drei Taxifahrten, Lale. Zeigen Sie mir, dass Sie ankommen. Dann reden wir weiter.  
>

**[FUNK] Der Schiedsrichter:** Bemerkenswert. Er hat Ihnen kein einziges Mal gedroht. Das macht er sonst bei jedem zweiten Satz.

## Akt I – Gelb: Die Pünktlichen

### Fahrplan
*Start: Bahnhof · Lohn 400 € · Ruf +25*

**[TITEL]** AKT I – GELB – Die Pünktlichen

> **SZENE** – Bahnhof, umkreise
> **KONRAD WEIDINGER:** Landsberg läuft nach Fahrplan. Meinem. Und heute ist ein Kurier der Gelben nicht angekommen.  
> **KONRAD WEIDINGER:** Er trug ein Päckchen. Klein, schwer, in gelbes Papier gewickelt. Wachssiegel obendrauf.  
> **LALE:** Was war drin?  
> **KONRAD WEIDINGER:** Das geht Sie nichts an. Fahren Sie zur Stadtverwaltung. Dort wurde er zuletzt gesehen.  
>

- *hin* – Zur Stadtverwaltung – dort wurde der Kurier zuletzt gesehen
> **SZENE** – Stadtverwaltung, statisch
> **LALE (OFF):** Der Kurier war weg. Nur sein Mützenband lag noch da, blau von oben bis unten.  
> **LALE (OFF):** Blau. Wer bei den Gelben ein blaues Band hinterlässt, will, dass man es findet.  
> **DER SCHIEDSRICHTER:** Das Päckchen sitzt in einem Wagen mit rosa Heckleuchte. Er fährt nach Westen. Schnell.  
>

- *stelle* – Den flüchtenden Wagen mit dem Päckchen stellen
- *bring* – Das Päckchen zurück zum Bahnhof bringen
> **SZENE** – Bahnhof, fahrt
> **KONRAD WEIDINGER:** Siegel unversehrt. Und doch – sehen Sie das? Die Prägung.  
> **LALE:** Ein Ball. Mit einem Sprung drin.  
> **KONRAD WEIDINGER:** Eine von vier Hälften des Siegels von 1999. Wenn alle vier Mannschaften ihre Hälfte zusammenlegen, öffnet sich der Schmalzturm.  
> **KONRAD WEIDINGER:** Und wer hinter diesem Päckchen her ist, will nicht, dass es zusammenkommt.  
>

### Wachtmeister Brandl
*Start: Polizeiinspektion · Lohn 450 € · Ruf +25*

> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** Halt. Stopp. Polizei. Bitte steigen Sie nicht aus. Oder doch. Ich muss kurz im Handbuch nachsehen.  
> **LALE:** Wachtmeister, wir stehen vor Ihrer Wache.  
> **WACHTMEISTER BRANDL:** Eben. Das macht es einfacher. Korbinian Brandl, Polizeiinspektion Landsberg. Sie haben einen Paintball-Wagen.  
> **LALE:** Ich habe auch ein Recht auf eine Meinung dazu.  
> **WACHTMEISTER BRANDL:** Haben Sie nicht. Stillhalten bitte. Ich nehme Ihre Personalien auf. Das dauert.  
>

- *halte* – Stillhalten – der Wachtmeister blättert im Handbuch
> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** So. Verwarnung. Und noch etwas, vertraulich: Wenn Sie in meinem Bezirk Säulen anmalen, schreibe ich Sie auf.  
> **LALE:** Verstanden.  
> **WACHTMEISTER BRANDL:** Nicht dass ich es verhindern könnte. Ich bin allein. Aber schreiben, das kann ich.  
>

- *markiere* – Die Säule der Polizeiinspektion zeichnen – direkt vor seiner Nase
> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** Das war jetzt Absicht.  
> **LALE:** Sie standen im Weg.  
> **WACHTMEISTER BRANDL:** Ich stehe immer im Weg. Das ist mein Beruf. – Ach, und Frau Lale: Im Archiv von Sankt Katharina liegen Fotos von 1999. Der Pfarrer weiß mehr, als er sagt.  
>

### Das Archiv
*Start: Sankt Katharina · Lohn 520 € · Ruf +30*

> **SZENE** – Sankt Katharina, kran
> **PFARRER ALOIS:** Der Herr sei mit dir. Und, wenn ich das sagen darf, die Gelben sind es nicht.  
> **LALE:** Wachtmeister Brandl meinte, Sie hätten Fotos von 1999.  
> **PFARRER ALOIS:** Ich habe Fotos von allem. Man nennt das Pfarrchronik. Kommen Sie mit in die Sakristei.  
>

- *hin* – In die Sakristei von Sankt Katharina
> **SZENE** – Sankt Katharina, statisch
> **PFARRER ALOIS:** Hier. Das Finale 1999. Vier Kapitäne, ein Schiedsrichter, die Goldene Kugel.  
> **LALE:** Ein Gesicht ist durchgestrichen.  
> **PFARRER ALOIS:** Franz Echter, Kapitän der Grünen. Nach dem Spiel hat man ihn des Betrugs beschuldigt. Er ist nie darüber hinweggekommen.  
> **LALE:** Echter. Wie die Schwester im Klinikum.  
> **PFARRER ALOIS:** Ja. Marta ist seine Schwester. Fragen Sie sie nicht danach. Noch nicht.  
>
> **SZENE** – Sankt Katharina, umkreise
> **LALE (OFF):** Ich sah mir das Foto lange an. Vier Männer in Farben. Einer in Weiß, sehr breit, sehr zufrieden. Der Ball in seiner Hand glänzte golden.  
> **LALE (OFF):** Zu golden. Auf einem Foto von 1999 glänzt nichts so frisch wie Lack von gestern.  
>

- *bring* – Das Foto zu Konrad Weidinger bringen
> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Der Ball war schon vor dem Anpfiff gestrichen worden? Das heißt, jemand hat das Finale entschieden, bevor es begann.  
> **LALE:** Wer war der Mann in Weiß?  
> **KONRAD WEIDINGER:** Xaver Gruber. Der Wirt vom Hauptplatz. Der Mann, der immer gewinnt.  
> **KONRAD WEIDINGER:** Das erklärt vieles. Und es macht mir Angst. Vor allem, dass ich es nie gesehen habe.  
>

### Der Dispatcher
*Start: Bahnhof · Lohn 800 € · Ruf +50*

> **SZENE** – Bahnhof, umkreise
> **KONRAD WEIDINGER:** Ich helfe Ihnen, Lale. Aber nicht aus Freundschaft. Aus Berechnung.  
> **KONRAD WEIDINGER:** Beweisen Sie, dass Sie nicht nur fahren, sondern ankommen: Zeichnen Sie die vier Wahrzeichen der Gelben, fahren Sie drei Taxifahrten – und stellen Sie mich.  
> **LALE:** Sie stellen? Ich dachte, Sie helfen mir.  
> **KONRAD WEIDINGER:** Eben. Ein Dispatcher, der sich nicht fangen lässt, ist kein Dispatcher. Ich fahre dann los. Pünktlich.  
>

- *warte_revier* – Revier Vorstadt-Nord nehmen: vier Wahrzeichen zeichnen, drei Taxifahrten, den Dispatcher stellen
> **SZENE** – spieler, statisch
> **KONRAD WEIDINGER:** Sieben Uhr einundvierzig. Schon wieder zu spät. Diesmal ich.  
> **KONRAD WEIDINGER:** Hier. Der Werkstattschlüssel. Die Lackiererei arbeitet ab sofort umsonst für Sie, und Ihr Wagen steckt mehr weg.  
> **LALE:** Warum tun Sie das?  
> **KONRAD WEIDINGER:** Weil Sie angekommen sind, Lale. Das war die Prüfung. Alles andere war Fahrplan.  
>
> **SZENE** – spieler, umkreise
> **KONRAD WEIDINGER:** Und noch etwas: Fragen Sie in der Lackiererei nach Schorsch. Er hat eine Farbe gemischt, die es nicht geben dürfte.  
> **KONRAD WEIDINGER:** Ein Gold, das in der Sonne grün wird. Das war 1999 auf dem Ball.  
>

### Gold, das grün wird
*Start: Lackiererei · Lohn 600 € · Ruf +40*

> **SZENE** – Lackiererei, statisch
> **SCHORSCH:** Servus! Konrad hat angerufen. Sie sind die mit den vier Farben. Mei, das hab ich noch nie gesehen.  
> **LALE:** Er sagte, Sie hätten ein Gold gemischt, das grün wird.  
> **SCHORSCH:** Ich hab's nicht gemischt, ich hab's gefunden. In einer Dose, hinten im Regal, Aufschrift: ›Ball, Final 99, Rückgabe unerwünscht‹.  
> **SCHORSCH:** Das Gold ist ein Lack mit Kupferanteil. In der Sonne wird er grün. Auf dem Foto sieht man's nicht. In echt schon.  
>

- *hin* – In die Lackiererei fahren – Schorsch wartet
> **SZENE** – Lackiererei, umkreise
> **SCHORSCH:** Der echte Pokal der Goldenen Kugel ist grün geworden, als man ihn in die Sonne stellte. Man hat ihn also ausgetauscht.  
> **LALE:** Gegen was?  
> **SCHORSCH:** Gegen einen gestrichenen. Gelb. Dann gold. Wer das gemacht hat, brauchte Zeit, Lack und eine Werkstatt wie meine.  
> **SCHORSCH:** Mir fällt nur einer ein, der hier damals so viel Farbe bestellt hat: Xaver Gruber. Der Wirt.  
> **LALE:** Wieder er.  
>

**[FUNK] Der Schiedsrichter:** Akt I ist vorbei. Einer von vier Köpfen liegt am Boden. Die Blauen am Lech wissen es bereits.

## Akt II – Blau: Die Schnellen

### Über den Lech
*Start: Mutterturm · Lohn 700 € · Ruf +40*

**[TITEL]** AKT II – BLAU – Die Schnellen

> **SZENE** – Mutterturm, kran
> **ANNELIESE LECHNER:** Da ist sie ja. Die Frau mit allen vier Farben. Anneliese Lechner. Mir gehört der Fluss, und was am Fluss liegt.  
> **LALE:** Konrad hat Sie erwähnt.  
> **ANNELIESE LECHNER:** Konrad erwähnt jeden, den er für gefährlich hält. Das sind sehr wenige. Sie sind also gefährlich.  
> **ANNELIESE LECHNER:** Ich fahre die schnellsten Boote am Lech und die schnellsten Wagen am Ufer. Wer zu den Blauen will, muss mich überholen.  
> **LALE:** Ich überhole nur, wenn jemand vor mir fährt.  
> **ANNELIESE LECHNER:** Dann fahren Sie jetzt zur Johanniskirche. Mein Vorfahrer wartet dort. Er hat etwas, das Ihnen gehört.  
>

- *hin* – Ans andere Ufer zur Johanniskirche – der Vorfahrer der Blauen wartet
> **SZENE** – Johanniskirche, statisch
> **LALE (OFF):** Der Vorfahrer wartete nicht. Er fuhr los, kaum dass ich bremste. Ein blauer Wagen, der aussah, als würde er auf dem Wasser fahren.  
> **DER SCHIEDSRICHTER:** Er trägt die zweite Siegelhälfte. Bringen Sie ihn zum Stehen. Und zwar so, dass er noch weiß, wer ihn gestellt hat.  
>

- *stelle* – Den Vorfahrer der Blauen stellen
- *bring* – Zurück zum Mutterturm
> **SZENE** – Mutterturm, statisch
> **ANNELIESE LECHNER:** Nicht schlecht. Eine halbe Sekunde langsamer als ich. Aber ich bin ja auch unfair gut.  
> **ANNELIESE LECHNER:** Die Siegelhälfte bleibt bei mir. Noch. Wenn Sie wirklich wollen, dass sie Ihnen gehört, dann beweisen Sie es bei der Regatta.  
>

### Regatta
*Start: Holzmarkt · Lohn 900 € · Ruf +50*

> **SZENE** – Holzmarkt, umkreise
> **ANNELIESE LECHNER:** Die Regatta der Blauen ist keine auf dem Wasser. Sie führt über die Brücken und Gassen am Lech. Wer als Erster wieder auf dem Holzmarkt ist, gewinnt.  
> **ANNELIESE LECHNER:** Infanterieplatz, Lady-Herkomer-Steg, zurück. Zwei Minuten dreißig. Ich fahre es in zwei zwanzig.  
> **LALE:** Dann fahre ich es in zwei zehn.  
> **ANNELIESE LECHNER:** Schön. Zum ersten Mal in zehn Jahren macht jemand mir das Rennen interessant.  
>

- *hin* – Erster Kontrollpunkt: Infanterieplatz
**[FUNK] Anneliese Lechner:** Zehn Meter vor Ihnen, Lale. Neun. Acht – ach, ich werde langsamer, damit es spannend bleibt!

- *hin* – Zweiter Kontrollpunkt: Lady-Herkomer-Steg
- *bring* – Zurück zum Holzmarkt – wer zuerst da ist, gewinnt
> **SZENE** – Holzmarkt, statisch
> **ANNELIESE LECHNER:** Zwei Sekunden. Zwei Sekunden vor mir. Das gibt es nicht.  
> **LALE:** Ich hatte einen guten Lehrer. Er war schnell und eingebildet.  
> **ANNELIESE LECHNER:** Ha! Hier – die zweite Siegelhälfte. Sie haben sie sich verdient. Aber heute Abend im Stadttheater gibt's was zu holen, das Sie noch mehr interessieren wird.  
>

### Vorhang auf
*Start: Stadttheater · Lohn 1000 € · Ruf +55*

> **SZENE** – Stadttheater, fahrt
> **ANNELIESE LECHNER:** Im Stadttheater gibt man heute ›Der Fischer und seine Farbe‹. Das Stück hat nur einen Zweck: Es hält die Polizei beschäftigt.  
> **ANNELIESE LECHNER:** Hinter der Bühne lagert ein Koffer. Gelb, Grün und Weiß haben ihn dort versteckt. Nicht aus Freundschaft. Aus Furcht.  
> **LALE:** Wovor?  
> **ANNELIESE LECHNER:** Vor dem, was darin liegt. Zeichnen Sie das Färbertor – das ist das Signal. Danach geht es los.  
>

- *markiere* – Das Färbertor zeichnen – das Signal für die Blauen
- *halte* – Durchhalten, bis der Vorhang fällt – die Polizei ist unterwegs
- *hin* – Zurück ins Stadttheater, hinter die Bühne
> **SZENE** – Stadttheater, statisch
> **LALE (OFF):** Hinter der Bühne roch es nach Staub und alter Farbe. Der Koffer war kleiner, als ich dachte.  
> **ANNELIESE LECHNER:** Machen Sie ihn auf. Ich habe ihn nie geöffnet. Ich wollte nicht wissen, wer von uns lügt.  
>
> **SZENE** – Stadttheater, umkreise
> **LALE:** Ein Brief. Mit Franz Echters Handschrift.  
> **ANNELIESE LECHNER:** Lesen Sie vor.  
> **LALE:** ›Ich habe den Ball nicht getauscht. Er lag schon so da, als ich ihn bekam. Wer das liest, soll wissen: Ich war es nicht.‹  
> **ANNELIESE LECHNER:** Er war es nicht. Das wusste ich. Alle haben es immer gewusst, nur keiner hat es gesagt.  
>

### Zum dritten Mal
*Start: Polizeiinspektion · Lohn 500 € · Ruf +30*

> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** Das ist das dritte Mal diese Woche. Ich zähle mit.  
> **LALE:** Ich habe nichts getan. Außer das Stadttheater zu verlassen.  
> **WACHTMEISTER BRANDL:** Mit sieben Streifenwagen im Rücken. Nehmen Sie Platz. Ich habe Kaffee. Er ist schlecht.  
>

- *hin* – Zur Polizeiinspektion – Wachtmeister Brandl will reden
> **SZENE** – Polizeiinspektion, umkreise
> **WACHTMEISTER BRANDL:** Passen Sie auf, Frau Lale. Ich bin allein auf der Wache, aber ich habe Augen.  
> **WACHTMEISTER BRANDL:** Der Förster hat Schlüssel zu meinen Zellen. Eigene. Ich kann sie ihm nicht abnehmen, weil er sie von meinem Vorgesetzten hat.  
> **LALE:** Warum sagen Sie mir das?  
> **WACHTMEISTER BRANDL:** Weil ich ein ehrlicher Polizist bin. Das ist ein einsamer Beruf.  
> **WACHTMEISTER BRANDL:** Wenn Sie in den Süden fahren, zu den Grünen: Rechnen Sie damit, dass die Polizei nicht auf Ihrer Seite ist. Auch wenn ich es bin.  
>

### Die Fährfrau
*Start: Mutterturm · Lohn 1200 € · Ruf +70*

> **SZENE** – Mutterturm, kran
> **ANNELIESE LECHNER:** Jetzt zeigen Sie mir, dass Sie nicht nur schnell sind, sondern auch hartnäckig.  
> **ANNELIESE LECHNER:** Zeichnen Sie die vier Wahrzeichen der Blauen. Drei Rennen. Und dann stellen Sie mich, Lale. Ich fahre schnell. Ich fahre sehr schnell.  
> **LALE:** Ich weiß.  
> **ANNELIESE LECHNER:** Dann wissen Sie, dass es ein Vergnügen wird.  
>

- *warte_revier* – Revier Lechviertel nehmen: vier Wahrzeichen zeichnen, drei Rennen, die Fährfrau stellen
> **SZENE** – spieler, statisch
> **ANNELIESE LECHNER:** Au. Mein Lack. Mein schöner Lack. Das war's. Ich gebe auf.  
> **ANNELIESE LECHNER:** Hier. Der Rotorschlüssel. Der Hubschrauber auf dem Klinikumsdach gehört jetzt Ihnen. Ich habe ihn nie benutzt, ich fahre lieber.  
> **LALE:** Sie lassen mich einfach gewinnen?  
> **ANNELIESE LECHNER:** Ich lasse niemanden gewinnen. Sie haben gewonnen. Das ist ein großer Unterschied.  
>
> **SZENE** – spieler, umkreise
> **ANNELIESE LECHNER:** Und hören Sie, Lale. Franz hat es nicht getan. Ich war dabei, als er seine Hälfte vergraben hat. Er hat geweint.  
> **ANNELIESE LECHNER:** Der Wirt hat getauscht. Und der Förster hat es gesehen und geschwiegen. Er ist der Nächste. Passen Sie auf sich auf. Er sieht alles, was sich im Wald bewegt.  
>

## Akt III – Grün: Die Stillen

### Nachtschicht
*Start: kbo-Lech-Mangfall-Klinik · Lohn 1100 € · Ruf +55*

**[TITEL]** AKT III – GRÜN – Die Stillen

> **SZENE** – kbo-Lech-Mangfall-Klinik, statisch
> **SCHWESTER MARTA:** Sie sind weit gekommen. Weiter, als ich dachte. Setzen Sie sich. Nein, nicht dahin – dort liegen Akten.  
> **LALE:** Franz Echters Akten?  
> **SCHWESTER MARTA:** Sie wissen es also. Pfarrer Alois, nehme ich an. Er konnte noch nie schweigen.  
> **SCHWESTER MARTA:** Franz war mein Bruder. Er war Kapitän der Grünen. Und er war ein anständiger Mensch.  
> **SCHWESTER MARTA:** Man hat ihn beschuldigt, den Ball getauscht zu haben. Er hat nie wieder gesprochen. Zwei Jahre später war er tot.  
>

- *hin* – In die kbo-Lech-Mangfall-Klinik – Marta wartet im Archiv
> **SZENE** – kbo-Lech-Mangfall-Klinik, umkreise
> **SCHWESTER MARTA:** In der Nacht, als Franz starb, hatte ich Dienst. Er sagte nur: ›Der Wirt hat es gewusst.‹ Dann war er still.  
> **LALE:** Wissen Sie, wer der Schiedsrichter ist?  
> **SCHWESTER MARTA:** Ich? Nein. Nicht, dass ich wüsste.  
> **LALE (OFF):** Sie hatte zu schnell geantwortet. Ich merkte es mir.  
>

- *bring* – Einen Patienten ins Klinikum bringen – schnell und ohne Rempler
**[FUNK] Schwester Marta:** Danke. Er lebt. Das ist selten genug, wenn Sie fahren.

### Stille Schicht
*Start: Friedhofskirche zur Heiligen Dreifaltigkeit · Lohn 1300 € · Ruf +65*

> **SZENE** – Friedhofskirche zur Heiligen Dreifaltigkeit, kran
> **LALE (OFF):** Der Friedhof lag still wie ein ausgeschaltetes Radio. Franz Echters Grab hatte frische Blumen. Weiß.  
> **OTTMAR WALD:** Er hat Weiß nie gemocht.  
>

- *hin* – Zur Friedhofskirche – ein Grab hat frische Blumen
> **SZENE** – Friedhofskirche zur Heiligen Dreifaltigkeit, statisch
> **OTTMAR WALD:** Ottmar Wald. Förster. Wer in den Wald geht, kommt an mir vorbei. Sie sind die Frau mit den vier Farben.  
> **LALE:** Sie waren dabei, damals.  
> **OTTMAR WALD:** Im Wald fällt ein Baum, und keiner sieht ihn. Ich habe vieles gesehen. Ich habe nichts gesagt.  
> **OTTMAR WALD:** Gehen Sie. Sofort. Die Polizei kommt gleich. Meine Polizei.  
>

- *halte* – Durchhalten – der Förster hat die Polizei gerufen
- *hin* – Weg von hier – zur Christuskirche

### Der Waldweg
*Start: Christuskirche · Lohn 1400 € · Ruf +70*

> **SZENE** – Christuskirche, umkreise
> **DER SCHIEDSRICHTER:** Der Förster schickt einen Boten in den Wald. Er trägt den Schlüsselbund. Wenn Sie den Boten stellen, hat der Förster nichts mehr in der Hand.  
> **LALE:** Wo ist der Wald?  
> **DER SCHIEDSRICHTER:** Überall. Aber heute nur dort, wo ein Panzer als Hochsitz dient. Ja, ein Panzer. Ich mache keine Scherze.  
>

- *stelle* – Den Boten des Försters stellen
- *bring* – Den Schlüsselbund ins Klinikum zu Marta bringen
> **SZENE** – Klinikum, statisch
> **SCHWESTER MARTA:** Diese Schlüssel … ich kenne sie. Der große passt in die Tür zum Schmalzturm. Der kleine zu Franz' Spind.  
> **LALE:** Der Förster wollte ihn behalten.  
> **SCHWESTER MARTA:** Natürlich. Wer das Schloss hat, hat die Wahrheit.  
>

### Der Förster
*Start: Klinikum · Lohn 1800 € · Ruf +90*

> **SZENE** – Klinikum, kran
> **SCHWESTER MARTA:** Der Förster hat zu viel gesehen. Er hat meinen Bruder untergehen lassen. Er muss reden.  
> **LALE:** Er hat die Polizei auf seiner Seite.  
> **SCHWESTER MARTA:** Dann sorgen Sie dafür, dass sie ihn nicht erreichen. Zeichnen Sie das Revier, fahren Sie drei Verfolgungen, und stellen Sie ihn.  
> **SCHWESTER MARTA:** Ich verlasse mich auf Sie, Lale.  
>

- *warte_revier* – Revier Klinikum und Süd nehmen: vier Wahrzeichen zeichnen, drei Verfolgungen, den Förster stellen
> **SZENE** – spieler, statisch
> **OTTMAR WALD:** Ich habe mein ganzes Leben lang nichts gesagt. Jetzt sage ich es.  
> **OTTMAR WALD:** Der Wirt hat den Ball getauscht. Mit Hilfe der Gelben, der Blauen und mir. Wir haben alle mitgemacht.  
> **OTTMAR WALD:** Franz hat es herausgefunden. Er wollte reden. Wir haben ihn zum Schweigen gebracht – nicht mit Gewalt. Mit Schuldzuweisung.  
> **LALE:** Das Einzige, was ein ehrlicher Mensch nicht überlebt.  
> **OTTMAR WALD:** Hier. Der Zündschlüssel. Der Panzer im Wald gehört Ihnen. Er ist alt, aber er fährt. Und der kleine, hier – der Spindschlüssel. Franz' Beweise liegen im Turm.  
>
> **SZENE** – spieler, umkreise
> **OTTMAR WALD:** Sagen Sie Marta, dass es mir leidtut. Und sagen Sie ihr: Sie soll das nicht für Rache tun. Rache macht einsam.  
>

## Akt IV – Weiß: Die Sieger

### Der Wirt
*Start: Historisches Rathaus · Lohn 1500 € · Ruf +70*

**[TITEL]** AKT IV – WEISS – Die Sieger

> **SZENE** – Hauptplatz, kran
> **XAVER GRUBER:** Da kommt sie! Die Frau mit allen vier Farben! Setzen Sie sich, essen Sie, trinken Sie – der Wirt lädt ein!  
> **LALE:** Xaver Gruber.  
> **XAVER GRUBER:** Der Einzige und Letzte. Ich habe ein Gasthaus am schönsten Platz Bayerns, einen Keller voller Bier und einen Ruf wie Donnerhall.  
> **XAVER GRUBER:** Und Sie haben in den letzten Tagen drei meiner Freunde besiegt. Das ist beeindruckend. Das ist auch unhöflich.  
> **LALE:** Ich hätte gern über den Ball von 1999 geredet.  
> **XAVER GRUBER:** Der Ball. Immer der Ball. Ein hübscher Ball. Aus Gold, wie jeder weiß.  
>

- *hin* – Auf den Hauptplatz – der Wirt wartet beim Marienbrunnen
> **SZENE** – Hauptplatz, umkreise
> **XAVER GRUBER:** Hören Sie, Lale. Ich mache Ihnen ein Angebot. Alle vier Farben auf einer Jacke – das ist keine Verletzung, das ist eine Krone.  
> **XAVER GRUBER:** Werden Sie meine Nachfolgerin. Der Wirt von morgen. Ein Haus, ein Platz, eine Stadt, die einem zu Füßen liegt.  
> **LALE:** Und Franz Echter?  
> **XAVER GRUBER:** Wer? – Ach der. Ein Mann, der nicht verlieren konnte.  
> **LALE (OFF):** Ich sah ihn an. Er lächelte wie jemand, der sich seit fünfundzwanzig Jahren nicht mehr daran erinnern will, wie man aufhört zu lächeln.  
>

### Marktnacht
*Start: Holzmarkt · Lohn 1900 € · Ruf +90*

> **SZENE** – Holzmarkt, kran
> **DER SCHIEDSRICHTER:** Der Wirt hat seine Leute auf den Holzmarkt geschickt. Sie sollen verhindern, dass Sie zum Turm kommen. Man nennt das Marktnacht.  
> **DER SCHIEDSRICHTER:** Es ist die gefährlichste Nacht des Jahres. Und die lauteste.  
>

- *hin* – Zum Holzmarkt – hier sammeln sich die Leute des Wirts
- *stelle* – Den Anführer der Weißen auf dem Holzmarkt stellen
- *halte* – Durchhalten – der ganze Platz ist aufgebracht
- *bring* – Durch die Gassen zum Historischen Rathaus
> **SZENE** – Historisches Rathaus, statisch
> **LALE (OFF):** Als der Morgen kam, war der Holzmarkt leer. Nur Farbe auf dem Pflaster. Gelb, Blau, Grün – und ein bisschen Weiß.  
> **DER SCHIEDSRICHTER:** Der Weg zum Schmalzturm ist frei. Gehen Sie. Bevor er sich besinnt.  
>

### Der Schmalzturm
*Start: Schmalzturm · Lohn 2000 € · Ruf +100*

> **SZENE** – Schmalzturm, kran
> **LALE (OFF):** Der Schmalzturm. Unten ein Tor, oben ein Dach, das aussieht wie ein Schmalzbrot. Und in der Mitte ein Geheimnis.  
>

- *hin* – Zum Schmalzturm – die Schlüssel passen
- *markiere* – Die Säule am Schmalzturm zeichnen – das Siegel öffnet sich nur unter Farbe
> **SZENE** – Schmalzturm, statisch
> **LALE (OFF):** Die vier Siegelhälften passten zusammen wie ein Puzzle. Das Tor ging auf mit einem Seufzer, als hätte es fünfundzwanzig Jahre die Luft angehalten.  
> **LALE (OFF):** Im Turm stand ein Sockel. Darauf eine Kugel. Nicht golden. Grün.  
>
> **SZENE** – Schmalzturm, umkreise
> **LALE:** Das Original. Sie ist grün geworden. Die Goldene Kugel war nie golden.  
> **LALE (OFF):** Unter der Kugel lag ein Brief. Franz Echters Handschrift. Datiert auf die Nacht nach dem Finale.  
> **LALE (OFF):** ›Ich habe gesehen, wer den Ball getauscht hat. Xaver, Konrad, Anneliese, Ottmar. Wir alle haben es getan. Ich wollte es gestehen. Man hat mich daran gehindert.‹  
> **LALE (OFF):** ›Wenn jemand das liest: Erzähl es der Stadt. Aber erzähl es mit Mitleid.‹  
>

> **SZENE** – Schmalzturm, statisch
> **DER SCHIEDSRICHTER:** Gut gemacht, Lale. Jetzt nehmen Sie die Kugel und den Brief. Und kommen zum Hauptplatz.  
> **LALE:** Sie klingen anders als sonst.  
> **DER SCHIEDSRICHTER:** Ich weiß. Die Stimme hat jetzt einen Namen. Marta Echter. Ich war der Schiedsrichter. Ich war immer der Schiedsrichter.  
> **LALE:** Sie haben mich benutzt.  
> **SCHWESTER MARTA:** Ich habe Sie geführt. Das ist ein Unterschied – und manchmal auch nicht. Jetzt kommen Sie. Es geht um alles.  
>

### Der Wirt vom Hauptplatz
*Start: Bayertor · Lohn 3000 € · Ruf +120*

> **SZENE** – Bayertor, umkreise
> **SCHWESTER MARTA:** Der Wirt hat von der Kugel erfahren. Er weiß, dass es zu Ende ist. Er wird um sein Gasthaus kämpfen, um seinen Namen, um alles.  
> **SCHWESTER MARTA:** Zeichnen Sie das Revier der Weißen, stellen Sie ihn. Und kommen Sie dann zu mir. Ich warte am Hauptplatz.  
> **LALE:** Marta.  
> **SCHWESTER MARTA:** Ja?  
> **LALE:** Ich bin noch nicht fertig mit Ihnen.  
> **SCHWESTER MARTA:** Das hoffe ich. Ich wäre sonst enttäuscht.  
>

- *warte_revier* – Revier Altstadt nehmen: vier Wahrzeichen zeichnen, acht Lieferungen, den Wirt vom Hauptplatz stellen
> **SZENE** – spieler, statisch
> **XAVER GRUBER:** Gut. Gut. Sie haben gewonnen. Ich habe immer gewonnen. Ich habe vergessen, wie man verliert.  
> **XAVER GRUBER:** Ich wollte nur, dass die Stadt nicht zerbricht. Ein Sieger braucht ein Ende. Ein Ende braucht einen Ball. Einen goldenen.  
> **LALE:** Sie haben einen Mann dafür kaputtgemacht.  
> **XAVER GRUBER:** Ja. Das habe ich. Und ich habe jeden Tag daran gedacht. Ich habe nur nie wieder darüber gesprochen.  
> **LALE (OFF):** Er saß auf dem Pflaster, in seiner weißen Jacke, und sah plötzlich sehr klein aus.  
>

## Finale und Abspann

### Samstagmorgen
*Start: Hauptplatz · Lohn 5000 € · Ruf +150*

**[TITEL]** FINALE – Samstag, kurz vor Sonnenaufgang

> **SZENE** – Hauptplatz, kran
> **LALE (OFF):** Der Hauptplatz im Morgengrauen. Auf dem Marienbrunnen: vier Dosen. Gelb, Blau, Grün, Weiß. Davor ein Dutzend Leute, die so taten, als wären sie zufällig da.  
>

- *hin* – Zum Hauptplatz – Samstag, kurz vor Sonnenaufgang
> **SZENE** – Hauptplatz, umkreise
> **KONRAD WEIDINGER:** Pünktlich. Auf die Minute. Das ist ein Kompliment.  
> **ANNELIESE LECHNER:** Und zwei Sekunden schneller als erwartet.  
> **OTTMAR WALD:** Ich habe geschwiegen. Jetzt stehe ich hier. Das muss reichen.  
> **WACHTMEISTER BRANDL:** Ich bin offiziell nicht hier. Ich habe nur zufällig Dienstschluss.  
>
> **SZENE** – Hauptplatz, statisch
> **SCHWESTER MARTA:** Lale. Sie haben die Kugel. Sie haben den Brief. Jetzt gehört die Stadt Ihnen.  
> **XAVER GRUBER:** Es ist nicht zu spät. Sie müssen es nicht lesen.  
> **SCHWESTER MARTA:** Doch. Das müssen Sie. Mein Bruder hat dafür bezahlt.  
> **LALE (OFF):** Zehn Gesichter. Zwanzig Augen. Und ich hatte einen Brief in der Hand, der fünfundzwanzig Jahre alt war.  
>

**[WAHL]** Was tust du mit der Wahrheit?
- Den Brief vorlesen – die ganze Stadt soll es hören.
- Die Farben mischen – ein Fest statt eines Prozesses.
- Die Kugel behalten – die Stadt gehört jetzt dir.

**[WENN ende = wahrheit]**

> **SZENE** – Hauptplatz, kran
> **LALE:** ›Ich habe den Ball nicht getauscht. Wer das liest, soll wissen: Ich war es nicht.‹  
> **LALE:** ›Aber erzähl es mit Mitleid.‹  
> **MENGE:** …  
>
> **SZENE** – Hauptplatz, umkreise
> **XAVER GRUBER:** Das war ich. Ich habe den Ball getauscht. Es tut mir leid.  
> **KONRAD WEIDINGER:** Und ich habe den Fahrplan verschoben, damit niemand es merkt.  
> **ANNELIESE LECHNER:** Ich habe die Boote bereitgestellt. Es tut mir leid.  
> **OTTMAR WALD:** Ich habe geschwiegen. Es tut mir mehr leid als alles andere.  
> **SCHWESTER MARTA:** Danke. Das war alles, was ich wollte. Dass jemand es laut sagt.  
> **WACHTMEISTER BRANDL:** Nach Paragraph 14 des Handbuchs muss ich Sie jetzt festnehmen. Alle. Das wird eine lange Schicht.  
>

**[SONST]**

**[WENN ende = frieden]**

> **SZENE** – Hauptplatz, kran
> **LALE:** Hier. Gelb, Blau, Grün, Weiß. Alle vier Farben. Wir kippen sie zusammen.  
> **XAVER GRUBER:** Das wird braun.  
> **LALE:** Probieren wir's.  
> **LALE (OFF):** Ich nahm die vier Dosen, eine nach der anderen. Das Wasser im Marienbrunnen färbte sich zuerst gelb, dann grün, dann blau – und am Ende wurde es doch kein Braun. Es wurde bunt.  
>
> **SZENE** – Hauptplatz, umkreise
> **SCHWESTER MARTA:** Das ist nicht, was ich wollte.  
> **LALE:** Das weiß ich. Aber Franz wollte es mit Mitleid. Das hier ist Mitleid.  
> **SCHWESTER MARTA:** …Vielleicht hatte er recht.  
> **ANNELIESE LECHNER:** Ich fahre Sie alle ans Ufer. Zum ersten Farbfest.  
> **KONRAD WEIDINGER:** Pünktlich um zehn.  
> **WACHTMEISTER BRANDL:** Ich schreibe nur die Falschparker auf. Alles andere ist heute nicht mein Beruf.  
>

**[SONST]**

> **SZENE** – Hauptplatz, statisch
> **LALE:** Ich behalte die Kugel.  
> **SCHWESTER MARTA:** Was?  
> **LALE:** Wenn die Stadt eine Farbe haben will, dann bin ich sie. Vier Farben auf einer Jacke. Mehr als jeder von euch.  
> **XAVER GRUBER:** Das hätte ich sagen können.  
> **KONRAD WEIDINGER:** Das hat er gesagt. Nur schlechter.  
>
> **SZENE** – Hauptplatz, umkreise
> **SCHWESTER MARTA:** Ich habe Sie gebaut, Lale. Ich habe Sie gebaut, damit Sie die Wahrheit sagen. Nicht, damit Sie die Krone aufsetzen.  
> **LALE:** Man baut nie nur eine Sache.  
> **LALE (OFF):** Sie sah mich an wie jemand, der eine Maschine gebaut hat und merkt, dass sie schneller wird als er selbst.  
> **WACHTMEISTER BRANDL:** Ich … ich glaube, ich muss mir das Handbuch noch einmal ansehen.  
>

### Abspann
*beginnt von selbst · Lohn 0 € · Ruf +0*

**[WENN ende = wahrheit]**

> **SZENE** – Schmalzturm, kran
> **LALE (OFF):** Es dauerte drei Wochen, bis die Zeitungen aufhörten. Der Wirt bekam ein Jahr auf Bewährung. Die anderen kamen mit Sozialstunden davon.  
> **LALE (OFF):** Franz Echters Name steht jetzt auf einer Tafel im Schmalzturm. Marta hat sie selbst angebracht.  
> **LALE (OFF):** Sie sagt, sie hat keinen Hass mehr. Ich glaube ihr zur Hälfte. Das ist mehr, als ich erwartet hatte.  
>

**[SONST]**

**[WENN ende = frieden]**

> **SZENE** – Hauptplatz, kran
> **LALE (OFF):** Das erste Farbfest war ein Chaos. Der Lech wurde an einer Stelle blau, an einer anderen grün. Die Fische sahen sehr überrascht aus.  
> **LALE (OFF):** Die Goldene Kugel liegt jetzt im Rathaus. Neben einem Zettel: ›Hier ist nie etwas gewesen.‹ Alle wissen, dass das gelogen ist. Alle sind sehr höflich.  
>

**[SONST]**

> **SZENE** – Hauptplatz, kran
> **LALE (OFF):** Landsberg hatte eine neue Farbe. Ich trug sie jeden Tag. Sie stand mir nicht schlecht.  
> **LALE (OFF):** Marta verließ die Stadt in der ersten Woche. Sie hat nichts mitgenommen außer ein Foto von ihrem Bruder.  
> **LALE (OFF):** Manchmal, spät nachts, kommt über Funk eine Stimme: ›Gut gemacht, Lale.‹ Und ich weiß nicht mehr, ob ich sie gehört habe oder erfunden.  
>

**[TITEL]** FARBKRIEG – Mitwirkende: Lale · Schwester Marta · Konrad Weidinger · Anneliese Lechner · Ottmar Wald · Xaver Gruber · Wachtmeister Brandl · Schorsch · Pfarrer Alois

**[TITEL]** Kein echter Ball wurde bei diesem Film verletzt. – Alle Namen und Handlungen sind frei erfunden. Nur Landsberg ist echt.

> **SZENE** – Hauptplatz, statisch
> **WACHTMEISTER BRANDL:** Frau Lale. Hier. Ein Strafzettel. Wegen falsch geparktem Paintball-Wagen. Hauptplatz. Sonntag, elf Uhr.  
> **LALE:** Das war mitten im Farbfest.  
> **WACHTMEISTER BRANDL:** Eben. Falsch ist falsch, auch wenn alle lachen.  
>

