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

**[TITEL]** FARBKRIEG – Vier Farben, eine Stadt am Lech und ein Finale, das 1999 keiner abgepfiffen hat

> **SZENE** – Klinikum, kran
> **LALE (OFF):** Landsberg am Lech. Den Fluss riecht man bis ins Krankenzimmer, und die Glocken schlagen, wann sie wollen.  
> **LALE (OFF):** Ich wachte auf mit Kopfweh und einer Jacke wie eine Malerpalette. In der Hand hielt ich eine Pistole, und die hielt ich fester als die Decke.  
>
> **SZENE** – spieler, umkreise
> **SCHWESTER MARTA:** Liegen bleiben. Drei Stunden waren Sie draußen auf der Straße, da steht man nicht gleich auf, gell.  
> **LALE:** Draußen? Wo draußen?  
> **SCHWESTER MARTA:** Katharinenstraße. Nicht an die Jacke fassen, die Farbe ist noch a bissl frisch.  
> **LALE:** Und die da? Die hab ich nicht losgelassen, oder?  
> **SCHWESTER MARTA:** Zwei Pfleger haben es versucht. Paintball, keine echte. Die Kappe am Lauf ist alt, die hat einer lang gepflegt.  
> **LALE:** Gepflegt? Von wem?  
> **SCHWESTER MARTA:** Schauen Sie in Ihre linke Jackentasche. Nicht dahin, da liegen Akten.  
> **LALE (OFF):** Der Waffenstillstand endet am Samstag. Komm zum Bahnhof. – S.  
> **SCHWESTER MARTA:** S. steht bei uns auf jedem dritten Rezept. Lesen Sie es nicht zweimal, davon wird es nicht klarer.  
>
> **SZENE** – spieler, statisch
> **SCHWESTER MARTA:** Ihre Jacke kennt hier jedes Kind. Fragen Sie am Bahnhof, was Gelb heißt. Die erklären es Ihnen gern und lang.  
> **SCHWESTER MARTA:** Seit 99 hat hier keiner mehr geworfen. Am Samstag ist Schluss damit, und Sie liegen mittendrin.  
> **LALE:** Mittendrin? Ich weiß nicht mal, wie ich heiße. – Doch. Lale. Das weiß ich.  
> **SCHWESTER MARTA:** Lale. Gut. Ich an Ihrer Stelle würde die Jacke anlassen. Ausziehen können Sie sie immer noch.  
> **SCHWESTER MARTA:** Der Wagen vorm Haupteingang ist offen, der Schlüssel steckt. Die Pistole nehmen S' mit, die will hier keiner.  
>

- *im_wagen* – Steig in den Wagen vor dem Klinikum – Taste F

### Der Schiedsrichter
*beginnt von selbst · Lohn 200 € · Ruf +15*

**[FUNK] Der Schiedsrichter:** Guten Morgen, Lale. Um sechs lag Ihr Blutdruck noch bei neunzig zu sechzig. Fahren Sie sachte an.

**[FUNK] Der Schiedsrichter:** Anpfiff ist am Bahnhof. Die Gelben zählen dort jede Minute. Laufen Sie nicht ins Abseits.

- *hin* – Zum Bahnhof fahren – die Gelben warten
> **SZENE** – Bahnhof, fahrt
> **KONRAD WEIDINGER:** Sieben Uhr einundvierzig, Frau Lale. Um diese Uhrzeit ist hier schon einmal jemand zu spät gekommen. Ich trage es Ihnen nicht nach, ich notiere es nur.  
> **LALE:** Zu spät? Ich wusste nicht, dass ich verabredet war.  
> **KONRAD WEIDINGER:** Man hat Sie mir um sechs Uhr zwölf angekündigt, mit einem Anruf ohne Nummer. Konrad Weidinger. Das Gelb an Ihrer Schulter steht Ihnen, und das sage ich ungern.  
> **LALE:** Ungern? Am Ärmel ist Blau, vorne Grün und hinten Weiß.  
> **KONRAD WEIDINGER:** Kruzifix. Drehen Sie sich nicht um, ich habe es gesehen.  
>
> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Jedes Taxi am Bahnhof, jede Krankenfahrt und jeder Lieferwagen zum Hauptplatz meldet sich vorher bei mir an, an Säule drei.  
> **KONRAD WEIDINGER:** Fahren Sie hier ohne Anmeldung, steht Ihr Wagen heute Abend in der Lackiererei, Frau Lale, und zwar ohne Räder.  
>

**[WAHL]** Was sagst du Konrad?
- »Melden Sie mich doch an. Als Verspätung.«
- »Säule drei. Wo genau?«

**[WENN ton = frech]**

> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Als Verspätung. Diese Kategorie vergebe ich seit fünfundzwanzig Jahren nicht mehr, Frau Lale.  
> **KONRAD WEIDINGER:** Drei Taxifahrten nach meiner Uhr. Kommen Sie an, reden wir weiter. Kommen Sie nicht an, hole ich mir den Wagen.  
>

**[SONST]**

> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Hinter dem Fahrkartenautomaten, neben dem Abfalleimer, den keiner leert. Sie merken sich Dinge. Das ist gefährlich und brauchbar.  
> **KONRAD WEIDINGER:** Drei Taxifahrten, Frau Lale, pünktlich nach meiner Uhr, die ich eben gestellt habe. Danach reden wir darüber, was Sie wirklich wollen.  
>

**[FUNK] Der Schiedsrichter:** Er hat Ihrem Wagen gedroht, Ihnen selbst nicht. Notieren Sie das. Gelbe Karte für ihn, keine für Sie.

## Akt I – Gelb: Die Pünktlichen

### Fahrplan
*Start: Bahnhof · Lohn 400 € · Ruf +25*

**[TITEL]** AKT I – GELB – Sieben Uhr einundvierzig

> **SZENE** – Bahnhof, umkreise
> **KONRAD WEIDINGER:** Um acht Uhr drei hätte mein Kurier an Gleis zwei stehen sollen, Frau Lale. Jetzt ist es acht Uhr neunzehn, und am Gleis steht eine Taube.  
> **KONRAD WEIDINGER:** Er hatte ein Päckchen dabei, so groß wie eine Faust und in gelbes Papier gewickelt. Ich habe es selbst versiegelt.  
> **LALE:** Selbst versiegelt? Was war drin?  
> **KONRAD WEIDINGER:** Das Päckchen geht Sie nichts an, das Siegel schon. Zuletzt hat man ihn an der Stadtverwaltung gesehen, im Hof beim Fahrradständer.  
>

- *hin* – Zur Stadtverwaltung – dort wurde der Kurier zuletzt gesehen
> **SZENE** – Stadtverwaltung, statisch
> **LALE (OFF):** Am Fahrradständer war kein Kurier, nur sein Mützenband im Kies. Es war blau gefärbt bis in die Naht und roch nach Lech.  
> **LALE (OFF):** Ein Gelber mit blauem Band. Entweder hatte er es verloren, oder jemand wollte, dass ich es einstecke. Ich steckte es ein.  
> **DER SCHIEDSRICHTER:** Ein Wagen mit rosa Heckleuchte fährt nach Westen. Der Fahrer hat Gips am linken Arm und schaltet schlecht. Laufen Sie ihn auf.  
>

- *stelle* – Den flüchtenden Wagen mit dem Päckchen stellen
- *bring* – Das Päckchen zurück zum Bahnhof bringen
> **SZENE** – Bahnhof, fahrt
> **KONRAD WEIDINGER:** Das Siegel ist ganz. Sie haben es nicht aufgemacht, Frau Lale, das rechne ich Ihnen an. Schauen Sie sich die Prägung an, nicht mich.  
> **LALE:** Ein Ball. Mit einem Sprung quer durch.  
> **KONRAD WEIDINGER:** Der Sprung ist eine Kante, und an diese Kante passt etwas, das nicht bei mir liegt. Mehr schreibe ich Ihnen nicht in den Fahrplan.  
> **KONRAD WEIDINGER:** Wer meinem Kurier ein blaues Band hinterlässt, will, dass ich zum Lech schaue. Da schaue ich heute nicht hin.  
>

### Wachtmeister Brandl
*Start: Polizeiinspektion · Lohn 450 € · Ruf +25*

> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** Halt! Gemäß, äh, Paragraf … Moment. Steigen S' nicht aus. Oder doch, ich find's grad nicht.  
> **LALE:** Wachtmeister, ich stehe direkt vor Ihrer Wache.  
> **WACHTMEISTER BRANDL:** Eben, dann ist der Weg kurz. Brandl, Polizeiinspektion Landsberg. An Ihrem Kotflügel ist Farbe, Frau Lale, und zwar Gelb.  
> **LALE:** Frau Lale? Ist Gelb bei Ihnen verboten?  
> **WACHTMEISTER BRANDL:** Bei mir ist Gelb notiert. Ihre Personalien bitte. Das Handbuch liegt im Handschuhfach, glaub ich, das dauert.  
>

- *halte* – Stillhalten – der Wachtmeister blättert im Handbuch
> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** So. Mündliche Verwarnung, schriftlich festgehalten. Wenn Sie mir die Säule da vorn anmalen, schreib ich Sie auf.  
> **LALE:** Aufschreiben? Mehr nicht?  
> **WACHTMEISTER BRANDL:** Ich hab die Schicht allein. Aber den Block hab ich immer dabei. Den find ich, das Handbuch nie.  
>

- *markiere* – Die Säule der Polizeiinspektion zeichnen – direkt vor seiner Nase
> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** Frau Lale. Die Farbe ist auf meinem Schuh.  
> **LALE:** Sie standen zu nah dran.  
> **WACHTMEISTER BRANDL:** Minute und Ort stehen jetzt drin. – Und was nicht im Block steht: Der Pfarrer von Sankt Katharina hat Fotos vom Finale 99. Ich bin da auch drauf, ganz am Rand.  
>

### Das Archiv
*Start: Sankt Katharina · Lohn 520 € · Ruf +30*

> **SZENE** – Sankt Katharina, kran
> **PFARRER ALOIS:** Grüß dich Gott, komm rein, Gott duzt auch. Klopf dir die Schuhe ab, die Gelben haben mir schon den halben Bahnhof in den Mittelgang getragen.  
> **LALE:** Brandl sagt, Sie haben Fotos vom Finale 1999.  
> **PFARRER ALOIS:** Der Korbinian. Als Bub hat er mir die Kollekte verzählt. Freilich hab ich Fotos, nur liest die Chronik halt keiner. Komm mit in die Sakristei, links unten.  
>

- *hin* – In die Sakristei von Sankt Katharina
> **SZENE** – Sankt Katharina, statisch
> **PFARRER ALOIS:** Da. Finale 99 am Hauptplatz. Vier Kapitäne, ein Schiedsrichter und die Kugel. Schau, wie die glänzt.  
> **LALE:** Glänzt? Einer ist durchgestrichen. Mit Kugelschreiber.  
> **PFARRER ALOIS:** Das war net ich. Das ist der Franz Echter von den Grünen. Nach dem Spiel hieß es, er hätt beschissen. Verzeih, Herr. Betrogen.  
> **LALE:** Echter? Auf dem Namensschild im Klinikum stand Echter.  
> **PFARRER ALOIS:** So? Auf Namensschildern steht viel. Lukas, glaub ich: Was du suchst, liegt dir manchmal vor der Nase. Mehr steht net in meiner Chronik.  
>
> **SZENE** – Sankt Katharina, umkreise
> **LALE (OFF):** Der Mann in Weiß stand in der Mitte, breit wie ein Wirtshaustisch. Er hielt die Kugel wie ein Bierfass beim Anstich.  
> **LALE (OFF):** Alles auf dem Foto war vergilbt, nur die Kugel nicht. Die glänzte wie Lack von gestern.  
>

- *bring* – Das Foto zu Konrad Weidinger bringen
> **SZENE** – Bahnhof, statisch
> **KONRAD WEIDINGER:** Ja, für ein Foto aus dem Juli 99 hat das Gold zu viel Glanz. Das sehe ich jetzt, Frau Lale. Schreiben Sie mir das bitte nicht als Vorwurf ins Buch.  
> **LALE:** Und der in Weiß? Wer ist das?  
> **KONRAD WEIDINGER:** Das ist Xaver Gruber, ihm gehört das Gasthaus am Hauptplatz. Um vierzehn Uhr acht hat er den Ball hochgehalten, und ich stand drei Meter daneben.  
> **KONRAD WEIDINGER:** Drei Meter. Legen Sie das Foto weg, bitte. Mein Zug um neun Uhr zehn wartet nicht.  
>

### Der Dispatcher
*Start: Bahnhof · Lohn 800 € · Ruf +50*

> **SZENE** – Bahnhof, umkreise
> **KONRAD WEIDINGER:** Ich mache Ihnen einen Vorschlag, Frau Lale. Sehen Sie ihn sich gut an, bevor Sie danke sagen.  
> **KONRAD WEIDINGER:** Sie zeichnen vier Wahrzeichen der Gelben und fahren drei Taxifahrten. Danach holen Sie mich ein, wenn ich losfahre. Schaffen Sie das, gehört Vorstadt-Nord Ihnen.  
> **LALE:** Einholen? Ich dachte, Sie helfen mir.  
> **KONRAD WEIDINGER:** Was ich Ihnen schenke, nimmt Ihnen morgen jemand wieder weg. Ich fahre um zehn Uhr null null los, keine Minute früher.  
>

- *warte_revier* – Vorstadt-Nord: 4 Wahrzeichen, 3 Taxifahrten, den Dispatcher stellen
> **SZENE** – spieler, statisch
> **KONRAD WEIDINGER:** Meine Uhr zeigt sieben Uhr einundvierzig, und Sie haben mich eben abgedrängt. Sie geht falsch, Lale. Sie geht seit Jahren falsch.  
> **KONRAD WEIDINGER:** Nehmen Sie. Das ist der Werkstattschlüssel der Lackiererei, dort reparieren sie Ihren Wagen ab jetzt umsonst. Seit 99 hängt er an meinem Bund, ich will ihn nicht mehr spüren.  
> **LALE:** Seit 99? Warum ausgerechnet Sie?  
> **KONRAD WEIDINGER:** Weil am Bahnhof nachts als Einziger jemand wach ist. Fragen Sie mich das nicht noch einmal vor Samstag, Lale.  
>
> **SZENE** – spieler, umkreise
> **KONRAD WEIDINGER:** Fragen Sie in der Lackiererei nach Schorsch. Sagen Sie ihm, ich schicke Sie wegen der Dose, und schauen Sie, wie er dann schaut.  
> **KONRAD WEIDINGER:** Sein Vater hat 99 die Werkstatt geführt und nachts nicht abgesperrt. Kruzifix. Das habe ich jetzt nicht gesagt.  
>

### Gold, das grün wird
*Start: Lackiererei · Lohn 600 € · Ruf +40*

> **SZENE** – Lackiererei, statisch
> **SCHORSCH:** Servus! Der Konrad hat angerufen, ganz außer Atem, das war er noch nie. Mei, die Jacke! Vier Farben und keine verlaufen.  
> **LALE:** Konrad schickt mich wegen einer Dose.  
> **SCHORSCH:** A Dose? Mei, Dosen hab ich hundert. Grundierung, Klarlack, Spachtel. Suchen S' Ihnen eine aus.  
> **SCHORSCH:** Ah, die. ›Ball, Final 99, Rückgabe unerwünscht‹. Die steht hinterm Klarlack. Gefunden hab ich die, vor Jahren. Irgendwann halt.  
>

- *hin* – In die Lackiererei fahren – Schorsch wartet
> **SZENE** – Lackiererei, umkreise
> **SCHORSCH:** Das Gold auf dem Foto ist Kupferpigment unterm Klarlack. Im Keller glänzt's, in der Sonn wird's grün wie a alte Dachrinne. Der Ball war also frisch lackiert, wie das Foto gemacht worden ist.  
> **LALE:** Frisch lackiert? Und wo ist der echte Ball?  
> **SCHORSCH:** Den hat wer verschwinden lassen und über Nacht an zweiten lackiert. Dafür braucht's a Spritzkabine, a ruhige Hand und kein Licht im Hof.  
> **SCHORSCH:** Und Kupferpigment hat 99 in Landsberg nur einer kistenweis bestellt, für seine Wirtshausschilder. Der Gruber Xaver.  
> **LALE:** Gruber? Und wer hat ihm nachts aufgesperrt?  
>

**[FUNK] Der Schiedsrichter:** Gelb ist abgepfiffen. Am Lech hat man es schon gehört. Essen Sie was, Lale, Sie haben seit gestern nichts im Magen.

## Akt II – Blau: Die Schnellen

### Über den Lech
*Start: Mutterturm · Lohn 700 € · Ruf +40*

**[TITEL]** AKT II – BLAU – Wer zuerst am Wehr ist

> **SZENE** – Mutterturm, kran
> **ANNELIESE LECHNER:** Geh, da schau her, die mit der bunten Jacke! Ich wett mit dir um deine Pistole, dass du nicht weißt, wo das Gelb aufhört und das Blau anfängt. Anneliese.  
> **LALE:** Der Konrad hat gesagt, ich soll mich vor Ihnen in Acht nehmen. Sieben Uhr einundvierzig, hat er dazugesagt. Keine Ahnung, warum.  
> **ANNELIESE LECHNER:** Mei, der Konrad. Der sagt die Uhrzeit, wenn er was nicht sagen mag. Hat er dich wenigstens pünktlich hergeschickt?  
> **ANNELIESE LECHNER:** Hörst des? Unterm Steg, des Kehrwasser. Da hat's heuer schon zwei Kanus umgedreht. Du schaust aus, als wärst du auch schon mal irgendwo reingefallen.  
> **LALE:** Reingefallen? Ich bin im Klinikum aufgewacht. Mehr weiß ich nicht. Was wollen Sie von mir?  
> **ANNELIESE LECHNER:** Mein Vorfahrer steht an der Johanniskirche, der Benni. Der hat was in der Tasche, das dich interessiert. Wenn du ihn kriegst, reden wir. Wenn nicht, war's a schöner Nachmittag.  
>

- *hin* – Ans andere Ufer zur Johanniskirche – der Vorfahrer wartet
> **SZENE** – Johanniskirche, statisch
> **LALE (OFF):** Die Glocke von Sankt Johann schlug gerade vier, als der Benni Gas gab. Gewartet hatte er offenbar nur aufs Läuten.  
> **DER SCHIEDSRICHTER:** Er hat die zweite Siegelhälfte, Lale. Drücken Sie ihn an die Mauer, aber nicht zu fest. Der Junge hatte letzten Winter einen Schlüsselbeinbruch. Rechts.  
>

- *stelle* – Den Vorfahrer der Blauen stellen
- *bring* – Zurück zum Mutterturm
> **SZENE** – Mutterturm, statisch
> **ANNELIESE LECHNER:** Der Benni hat gefunkt, du hast ihm den Spiegel abgefahren. Mei, der weint jetzt. Wetten, dass er sich morgen trotzdem ins Boot setzt?  
> **ANNELIESE LECHNER:** Die Hälfte hab jetzt ich, gell. Er hat sie mir zugeworfen, bevor du da warst. Willst sie haben? Dann fahr gegen mich, nicht gegen meine Buben.  
>

### Regatta
*Start: Holzmarkt · Lohn 900 € · Ruf +50*

> **SZENE** – Holzmarkt, umkreise
> **ANNELIESE LECHNER:** Wir fahren hoid am Wasser entlang statt drauf, des Wasser ist heut zu hoch. Start Holzmarkt, Ziel Holzmarkt. Die Brücken zählen, die Gehsteige auch, wenn's sein muss.  
> **ANNELIESE LECHNER:** Infanterieplatz, Lady-Herkomer-Steg, zurück. Zwei Minuten dreißig für Leute, die bremsen. Ich bin bei zwei zwanzig, seit fünfundzwanzig Jahr.  
> **LALE:** Zwanzig? Ich nehm zwei zehn. Und wenn ich gewinne, sagen Sie mir, wer mir die Jacke angemalt hat.  
> **ANNELIESE LECHNER:** Hab ich gesagt, dass ich's weiß? Geh. Wenn du gewinnst, kriegst die Hälfte. Wenn ich gewinn, kriegst a Leberkässemmel und fährst heim.  
>

- *hin* – Erster Kontrollpunkt: Infanterieplatz
**[FUNK] Anneliese Lechner:** Zehn Meter. Neun. Acht – mei, jetzt lass ich dich halt a bissl ran, sonst schläfst mir noch ein! Sieben!

- *hin* – Zweiter Kontrollpunkt: Lady-Herkomer-Steg
- *bring* – Zurück zum Holzmarkt – wer zuerst da ist, gewinnt
> **SZENE** – Holzmarkt, statisch
> **ANNELIESE LECHNER:** Zwei Sekunden. Vor mir. Wart, des Display spinnt. Nein. Kruzifix, des Display spinnt fei nicht.  
> **LALE:** Sie haben am Steg vom Gas gehen müssen. Wegen der Frau mit dem Kinderwagen. Ich nicht. Ich hab sie gar nicht gesehen.  
> **ANNELIESE LECHNER:** Des sagst mir so ins Gesicht? Ha! Da, nimm's, die Hälfte, bevor ich's mir anders überleg. Heut Abend spielen s' im Stadttheater, da steht hinten was rum, wo ich mich selber nie hingetraut hab.  
>

### Vorhang auf
*Start: Stadttheater · Lohn 1000 € · Ruf +55*

> **SZENE** – Stadttheater, fahrt
> **ANNELIESE LECHNER:** ›Der Fischer und seine Farbe‹, Premiere. Der halbe Stadtrat sitzt drin, und die Polizei steht vorm Eingang und friert. Gut für uns.  
> **ANNELIESE LECHNER:** Hinter der Bühne, beim Schnürboden, steht ein Koffer. Gelb, Grün und Weiß haben ihn da reingestellt, neunundneunzig. Ich war bloß zum Tragen da. Hab ich gemeint.  
> **LALE:** Gemeint?  
> **ANNELIESE LECHNER:** Frag net so. Mal das Färbertor an, dann wissen meine Leut, dass sie die Streifen weglocken sollen. Und mach schnell, die Pause dauert zwanzig Minuten.  
>

- *markiere* – Das Färbertor zeichnen – das Signal für die Blauen
- *halte* – Durchhalten, bis der Vorhang fällt – die Polizei ist unterwegs
- *hin* – Zurück ins Stadttheater, hinter die Bühne
> **SZENE** – Stadttheater, statisch
> **LALE (OFF):** Hinter der Bühne roch es nach Staub und alter Farbe. Von vorn kam Applaus, an der falschen Stelle. Der Koffer war kleiner, als ich dachte, und das Schloss schon aufgebrochen.  
> **ANNELIESE LECHNER:** Du machst ihn auf. Ich halt die Lampe. Und wenn was drin ist, was mit mir zu tun hat, liest du's leise.  
>
> **SZENE** – Stadttheater, umkreise
> **LALE:** Ein Brief. Unterschrieben mit Franz. Das F sieht aus wie auf meinem Zettel. Fast.  
> **ANNELIESE LECHNER:** Fast? Zeig her. Nein, lies vor. Laut ist schon recht.  
> **LALE:** ›Den Ball hab ich nicht ausgesucht, aber ich hab gespürt, dass er zu schwer war, und hab trotzdem geschossen. Die Dosen hat die Anneliese übern Lech gebracht. Ich hab sie gesehen. Sie mich auch.‹  
> **ANNELIESE LECHNER:** Er hat mich gesehen. Die ganze Zeit hab ich gemeint, er hat mich nicht gesehen. Gib den Brief her, Lale. Bitte. Den kriegt sonst keiner.  
>

### Zum dritten Mal
*Start: Polizeiinspektion · Lohn 500 € · Ruf +30*

> **SZENE** – Polizeiinspektion, statisch
> **WACHTMEISTER BRANDL:** Drittes Mal diese Woche, Frau Lale. Oder viertes. Ich hab's aufgeschrieben, irgendwo. Moment.  
> **LALE:** Ich bin aus einem Theater gelaufen. Das ist nicht verboten, oder?  
> **WACHTMEISTER BRANDL:** Mit sieben Streifenwagen hinter Ihnen und Farbe am Färbertor. Setzen S' Ihnen hin. Der Kaffee ist von heut früh, der schmeckt nach Montag.  
>

- *hin* – Zur Polizeiinspektion – Wachtmeister Brandl will reden
> **SZENE** – Polizeiinspektion, umkreise
> **WACHTMEISTER BRANDL:** Gemäß, äh, Dienstvorschrift Abschnitt … wo hab ich jetzt das Handbuch. Egal. Was haben S' da im Mantel? Ein Brief? Nein, sagen S' nichts, sonst muss ich ihn beschlagnahmen.  
> **WACHTMEISTER BRANDL:** Der Förster, der Wald Ottmar, der hat Schlüssel zu meinen Zellen. Eigene. Gekriegt hat er die von meinem Chef, und der Chef unterschreibt meinen Urlaub.  
> **LALE:** Schlüssel? Und das sagen Sie ausgerechnet mir. Einer Verdächtigen.  
> **WACHTMEISTER BRANDL:** Sie haben keinen Block dabei. Ich schon. Und ich hab schon einmal zugeschaut, wie was passiert ist, damals am Platz. Fragen S' nicht weiter, sonst hör ich zum Reden nimmer auf.  
> **WACHTMEISTER BRANDL:** Fahren S' in den Süden, zu den Grünen, dann sind meine Kollegen gegen Sie. Ich parke dann zufällig vor der Kreuzung. Mit Warnblinker.  
>

### Die Fährfrau
*Start: Mutterturm · Lohn 1200 € · Ruf +70*

> **SZENE** – Mutterturm, kran
> **ANNELIESE LECHNER:** Den Brief hast mir nicht gegeben. Gut, dann holen wir's uns auf die alte Art. Mein Viertel gegen dein Papier.  
> **ANNELIESE LECHNER:** Vier Wahrzeichen, drei Rennen, und am End fährst gegen mich, auf der Uferstraß bis zum Wehr. Wetten, dass du am Wehr bremst?  
> **LALE:** Wehr? Um was wetten Sie diesmal?  
> **ANNELIESE LECHNER:** Wenn du bremst, verbrennst den Brief. Wenn nicht, sag ich dir, was ich in der Nacht am Lech gesehen hab.  
>

- *warte_revier* – Lechviertel: vier Wahrzeichen, drei Rennen, Fährfrau stellen
> **SZENE** – spieler, statisch
> **ANNELIESE LECHNER:** Mein Kotflügel! Den hat der Schorsch erst lackiert! Geh, hör auf, hör auf, ich steh ja schon.  
> **ANNELIESE LECHNER:** Da, der Rotorschlüssel. Für den Hubschrauber am Klinikumsdach. Den hat mir der Ottmar gegeben, damit ich nie wieder übern Fluss rudern muss. Ich bin nie damit geflogen.  
> **LALE:** Sie sind am Wehr langsamer geworden. Ich hab's gesehen. Haben Sie mich gewinnen lassen?  
> **ANNELIESE LECHNER:** Schau mich an. Mir zittern die Händ. Glaubst, so lass ich wen gewinnen? Das erste Mal seit neunundneunzig, dass mir eine davongefahren ist.  
>
> **SZENE** – spieler, umkreise
> **ANNELIESE LECHNER:** Die Wette. Ich bin ihm nachgegangen in der Nacht, am Lech, weil ich ihn gesucht hab, nicht wegen dem Siegel. Er hat seine Hälfte unterm Steg vergraben und geweint. Ich bin stehen geblieben und hab nichts gesagt.  
> **ANNELIESE LECHNER:** Getauscht hat der Wirt, die Dosen hab ich gefahren, und der Ottmar ist im Wald gestanden und hat zugeschaut. Fahr zu ihm. Aber nimm den Brief mit, der Ottmar redet nur, wenn er was in der Hand sieht.  
>

## Akt III – Grün: Die Stillen

### Nachtschicht
*Start: kbo-Lech-Mangfall-Klinik · Lohn 1100 € · Ruf +55*

**[TITEL]** AKT III – GRÜN – Die Stillen

> **SZENE** – kbo-Lech-Mangfall-Klinik, statisch
> **SCHWESTER MARTA:** Hocken S' sich hin. Nein, net dahin, da liegen Akten. Auf den Hocker, der wackelt a bissl, das hält Sie wach.  
> **LALE:** Akten? Von wem?  
> **SCHWESTER MARTA:** Sie haben den Namen schon im Mund, ich seh's Ihnen an. War der Alois, gell? Der hat in der Sakristei immer schon lauter geredet als auf der Kanzel.  
> **SCHWESTER MARTA:** Echter, Franz. Mein Bruder. Kapitän der Grünen. Geben S' mir die Mappe da rüber, die mit dem Kaffeerand.  
> **SCHWESTER MARTA:** Nach dem Finale haben's ihm den Ball umgehängt. Danach hat er kein Wort mehr gesagt, zu niemandem, auch zu mir nicht. Zwei Jahr später lag er hier, Zimmer vierzehn.  
>

- *hin* – In die kbo-Lech-Mangfall-Klinik – Marta wartet im Archiv
> **SZENE** – kbo-Lech-Mangfall-Klinik, umkreise
> **SCHWESTER MARTA:** Ich hab Nachtdienst gehabt, wie heut. Er hat gesagt: ›Der Wirt hat's gewusst.‹ Dann hab ich ihm das Kissen aufgeschüttelt. Mehr war nicht zu tun.  
> **LALE:** Und der Schiedsrichter? Der Mann am Funk, der weiß, wann ich blute, bevor ich's merk.  
> **SCHWESTER MARTA:** Ich? Nein. Nicht, dass ich wüsste. Legen S' die Mappe zurück, die gehört nach oben links.  
> **LALE (OFF):** Sie hatte zu schnell geantwortet. Und ich hatte gar nicht gefragt, ob sie es ist. Der Flur roch nach Desinfektionsmittel. Ich kannte den Geruch von irgendwo.  
>

- *bring* – Einen Patienten ins Klinikum bringen – schnell und ohne Rempler
**[FUNK] Schwester Marta:** Puls achtundachtzig, er schimpft schon wieder. Gut gemacht. Und jetzt waschen S' sich die Hände, Sie haben Blut am Ärmel. Nicht Ihres.

### Stille Schicht
*Start: Friedhofskirche zur Heiligen Dreifaltigkeit · Lohn 1300 € · Ruf +65*

> **SZENE** – Friedhofskirche zur Heiligen Dreifaltigkeit, kran
> **LALE (OFF):** Hinter der Friedhofskirche hörte man den Lech, sonst nichts. Auf Franz Echters Grab lagen frische Blumen, noch nass vom Gießen. Weiß.  
> **OTTMAR WALD:** Weiß hat er nie mögen.  
>

- *hin* – Zur Friedhofskirche – ein Grab hat frische Blumen
> **SZENE** – Friedhofskirche zur Heiligen Dreifaltigkeit, statisch
> **OTTMAR WALD:** Die Pistol'. Wo haben S' die her?  
> **LALE:** Her? Geliehen. Sie waren dabei, damals. Neunundneunzig.  
> **OTTMAR WALD:** Gießkann' gehört zurück an'n Brunnen. Bringen S' die mit, wenn S' schon dastehen.  
> **OTTMAR WALD:** Polizei kommt gleich. Meine. Hab angerufen, wie S' am Tor waren.  
>

- *halte* – Durchhalten – der Förster hat die Polizei gerufen
- *hin* – Weg von hier – zur Christuskirche

### Der Waldweg
*Start: Christuskirche · Lohn 1400 € · Ruf +70*

> **SZENE** – Christuskirche, umkreise
> **DER SCHIEDSRICHTER:** Anpfiff zweite Halbzeit, Lale. Der Förster gibt einen Schlüsselbund weiter, an einen jungen Burschen, Knie lädiert, rechts. Der läuft nicht weit.  
> **LALE:** Woher wissen Sie das mit dem Knie?  
> **DER SCHIEDSRICHTER:** Nehmen Sie den Forstweg hinterm Klinikum, bis zum Panzer. Ja, Panzer. Der Förster sitzt da drauf wie auf einem Hochsitz. Kein Abseits, fahren Sie.  
>

- *stelle* – Den Boten des Försters stellen
- *bring* – Den Schlüsselbund ins Klinikum zu Marta bringen
> **SZENE** – Klinikum, statisch
> **SCHWESTER MARTA:** Geben S' her. Der große ist vom Schmalzturm. Der kleine … den hat der Franz an einer Schnur getragen, da ist noch der Knoten dran.  
> **LALE:** Dran? Der Förster hat geschrien, als ich ihn dem Jungen abgenommen hab.  
> **SCHWESTER MARTA:** Den kleinen behalt ich. Nein, lassen S' die Hand weg. Den großen kriegen Sie, wenn der Ottmar geredet hat.  
>

### Der Förster
*Start: Klinikum · Lohn 1800 € · Ruf +90*

> **SZENE** – Klinikum, kran
> **SCHWESTER MARTA:** Fünfundzwanzig Jahr legt der Blumen hin und schaut weg. Heut schaut er nicht weg. Sie holen ihn mir.  
> **LALE:** Holen? Der hat die Polizei in der Tasche. Und Sie haben meinen Schlüssel.  
> **SCHWESTER MARTA:** Dann nehmen S' ihm erst das Revier. Klinikum, Süd, alles, wo er Leute hat. Wenn er allein ist, redet er.  
> **SCHWESTER MARTA:** Und ziehen S' die Jacke wieder an. Die Farbe soll man fei sehen.  
>

- *warte_revier* – Klinikum/Süd: vier Wahrzeichen, drei Verfolgungen, Förster stellen
> **SZENE** – spieler, statisch
> **OTTMAR WALD:** Hast mich. Gut.  
> **OTTMAR WALD:** Der Wirt hat getauscht. Gelb hat den Zug verschoben, Blau hat die Dosen gefahren. Ich hab Schmiere gestanden. Am Waldrand.  
> **OTTMAR WALD:** Franz ist draufgekommen. Wir haben's ihm angehängt, alle vier, beim Stammtisch, bis's die ganze Stadt geglaubt hat. Hat keiner Hand anlegen müssen.  
> **LALE:** Und die weißen Blumen? Damit's Ihnen besser geht?  
> **OTTMAR WALD:** Zündschlüssel. Panzer. Springt schlecht an, Choke ziehen. Und die Pistol' da behalt. Gehört eh nimmer mir zum Zurückgeben.  
>
> **SZENE** – spieler, umkreise
> **OTTMAR WALD:** Der Marta sagst nix von mir. Die weiß, wo ich bin. Jeden Sonntag, mit Weiß.  
>

## Akt IV – Weiß: Die Sieger

### Der Wirt
*Start: Historisches Rathaus · Lohn 1500 € · Ruf +70*

**[TITEL]** AKT IV – WEISS – Wer zuletzt zahlt

> **SZENE** – Hauptplatz, kran
> **XAVER GRUBER:** Ja da schau her, die mit de vier Farben! Hock di her, mei Liebe. Nein, net da, da sitzt der Konrad, auch wenn er nimmer kommt.  
> **LALE:** Sie sind der Gruber. Der vom Stammtisch.  
> **XAVER GRUBER:** Der Xaver. Für dich der Xaver. Mogst a Weißbier? Geht aufs Haus. Alles geht aufs Haus, bis einer anfängt zu rechnen.  
> **XAVER GRUBER:** Der Konrad stellt seine Uhr nimmer, d'Anneliese fahrt allein auf'm Lech, und der Ottmar geht ans Telefon. Des warst du, gell? Drei Freund in einer Woche. Respekt. Und a bissl frech.  
> **LALE:** Ich will über den Ball reden. Neunundneunzig.  
> **XAVER GRUBER:** Den Ball. Freilich. Den erzähl ich jeden Freitag, da musst halt um achte kommen, dann gibt's ihn mit Schweinsbraten. Und so wie ich ihn erzähl, glänzt er.  
>

- *hin* – Auf den Hauptplatz – der Wirt wartet beim Marienbrunnen
> **SZENE** – Hauptplatz, umkreise
> **XAVER GRUBER:** Jetzt pass auf. Vier Farben auf einer Jackn, des hat in Landsberg noch keiner tragen dürfen. Des is a Kron, mei Liebe. Und a Kron braucht a Haus.  
> **XAVER GRUBER:** Ich hab koane Kinder, nur an Keller und an Stammtisch. Unterschreib mir, dass du's übernimmst, und die ganze Altstadt grüßt dich beim Bäcker zuerst.  
> **LALE:** Und wer grüßt dann den Franz Echter?  
> **XAVER GRUBER:** Den Franz. Mei. Der Franz hat Weiß nie mögen. Trink aus, bevor's warm wird.  
> **LALE (OFF):** Er wischte den Brunnenrand ab. Dreimal an derselben Stelle. Da war nichts.  
>

### Marktnacht
*Start: Holzmarkt · Lohn 1900 € · Ruf +90*

> **SZENE** – Holzmarkt, kran
> **DER SCHIEDSRICHTER:** Anpfiff zur zweiten Halbzeit, Lale. Gruber hat seine Weißen auf den Holzmarkt geschickt, sie stehen vor dem Weg zum Turm.  
> **DER SCHIEDSRICHTER:** Ihr Knie ist seit Dienstag geschwollen, schonen Sie das linke Bein. Und wer am Boden liegt, den lassen Sie liegen. Das ist Abseits, kein Sieg.  
>

- *hin* – Zum Holzmarkt – hier sammeln sich die Leute des Wirts
- *stelle* – Den Anführer der Weißen auf dem Holzmarkt stellen
- *halte* – Durchhalten – der ganze Platz ist aufgebracht
- *bring* – Durch die Gassen zum Historischen Rathaus
> **SZENE** – Historisches Rathaus, statisch
> **LALE (OFF):** Gegen fünf läutete Sankt Mariä Himmelfahrt, und der Holzmarkt war leer. Auf dem Pflaster Gelb, Blau, Grün. Das bisschen Weiß hatte ich an den Schuhen.  
> **DER SCHIEDSRICHTER:** Nachspielzeit läuft. Der Schmalzturm ist frei, bis Gruber wieder nüchtern ist. Gehen Sie jetzt, Sie haben seit zwanzig Stunden nichts getrunken.  
>

### Der Schmalzturm
*Start: Schmalzturm · Lohn 2000 € · Ruf +100*

> **SZENE** – Schmalzturm, kran
> **LALE (OFF):** Durchs Tor zog es nach Lech und kaltem Mörtel. Ich hatte vier Schlüssel in der Hand und keine Ahnung, welcher zuerst.  
>

- *hin* – Zum Schmalzturm – die Schlüssel passen
- *markiere* – Die Säule am Schmalzturm zeichnen – das Siegel öffnet sich nur unter Farbe
> **SZENE** – Schmalzturm, statisch
> **LALE (OFF):** Gelb, Blau, Grün, Weiß. Die Farbe lief in die Rillen der Säule, dann klickte es. Meine Jacke hatte dieselben vier Flecken an denselben Stellen. Ich wollte das nicht sehen.  
> **LALE (OFF):** Oben auf dem Sockel lag eine Kugel. Sie war fleckig, grün wie ein altes Kirchendach.  
>
> **SZENE** – Schmalzturm, umkreise
> **LALE:** Grün. Schorsch hat's gesagt, das Gold wird in der Sonne grün. Das ist die echte. Die im Gasthaus ist nachlackiert.  
> **LALE (OFF):** Darunter ein Umschlag, die Ecken weich vom Feuchten. Franz Echters Schrift, krakelig, als hätte er im Stehen geschrieben. Datum: die Nacht nach dem Finale.  
> **LALE (OFF):** ›Xaver hat die Kugel getauscht. Konrad hat ihm den Zug verschoben, Anneliese das Boot geliehen, Ottmar hat Schmiere gestanden. Und ich hab gewusst, dass wir sonst verlieren, und nichts gesagt. Morgen geh ich zur Polizei.‹  
> **LALE (OFF):** ›Falls ich nicht hinkomm: Sagt's der Marta zuerst. Und sagt's dem Xaver ins Gesicht, nicht hinterm Rücken.‹  
>

> **SZENE** – Schmalzturm, statisch
> **DER SCHIEDSRICHTER:** Abpfiff in diesem Abschnitt, Lale. Nehmen Sie Kugel und Brief. Langsam, der Boden ist nass, und Sie haben kein Gefühl mehr im linken Fuß.  
> **LALE:** Woher wissen Sie das? Mein Fuß. Mein Knie. Seit dem ersten Tag wissen Sie das.  
> **DER SCHIEDSRICHTER:** Weil ich Ihnen den Verband gewickelt hab. Rechts herum, Sie haben sich beschwert. Die Frauenstimme in der Nacht, an die Sie sich erinnern, das war auch ich.  
> **LALE:** Sie haben mich auf die Straße gelegt. Mit seiner Pistole. Ich hab gedacht, ich hätte das alles selber gewollt.  
> **SCHWESTER MARTA:** Legen Sie die Kugel nicht auf den Boden, da ist Taubendreck. Kommen Sie zum Hauptplatz, Lale. Die Pistole will ich danach zurück. Die gehört nicht Ihnen.  
>

### Der Wirt vom Hauptplatz
*Start: Bayertor · Lohn 3000 € · Ruf +120*

> **SZENE** – Bayertor, umkreise
> **SCHWESTER MARTA:** Der Xaver hat's erfahren, der Turm war offen und halb Landsberg hat zugeschaut. Er hat die Weißen wieder eingesammelt. Er sperrt nicht zu, er schenkt aus.  
> **SCHWESTER MARTA:** Nehmen Sie ihm die Altstadt, vorher redet er nicht. Und trinken Sie nichts, was er hinstellt. Ich warte am Brunnen.  
> **LALE:** Marta.  
> **SCHWESTER MARTA:** Was denn? Ihre Jacke ist offen.  
> **LALE:** Die Pistole kriegen Sie, wenn ich weiß, was in der Nacht noch war. Alles.  
> **SCHWESTER MARTA:** Dann behalten Sie sie halt noch a bissl. Gesichert, gell.  
>

- *warte_revier* – Altstadt: vier Wahrzeichen, acht Lieferungen, Wirt am Hauptplatz
> **SZENE** – spieler, statisch
> **XAVER GRUBER:** Jetzt hockst du auf meinem Platz und ich am Boden. Moment, des Pflaster is dreckig, ich wisch's dir noch –  
> **XAVER GRUBER:** Mir hätten die Altstadt hergeben müssen, Madl. Mei Vater, mei Haus, alles an die Grünen. Ich hab Angst ghabt. Und dann hab ich halt ausgschenkt, bis keiner mehr gfragt hat.  
> **LALE:** Den Franz haben Sie gefragt. Und dann?  
> **XAVER GRUBER:** Der wollt zum Brandl seinem Chef. Ich hab ihm a Bier hingstellt und gsagt, er soll's überschlafen. Die Dose mit dem echten Gold hab ich beschriftet, weil ich's net wegschmeißen hab können. Mehr sag ich dir net, solang die Leut zuschaun.  
> **LALE (OFF):** Er wischte mit dem Ärmel über das Pflaster neben sich. Die weiße Jacke war danach grau. Er hörte nicht auf.  
>

## Finale und Abspann

### Samstagmorgen
*Start: Hauptplatz · Lohn 5000 € · Ruf +150*

**[TITEL]** FINALE – Samstag, kurz vor Sonnenaufgang. Nachspielzeit.

> **SZENE** – Hauptplatz, kran
> **LALE (OFF):** Vier Dosen auf dem Rand vom Marienbrunnen: Gelb, Blau, Grün, Weiß. Der Bäcker hatte noch nicht aufgesperrt und stand trotzdem schon da, Mehl an der Schürze.  
>

- *hin* – Zum Hauptplatz – Samstag, kurz vor Sonnenaufgang
> **SZENE** – Hauptplatz, umkreise
> **KONRAD WEIDINGER:** Fünf Uhr achtundfünfzig, Lale. Ich habe die Uhr heute früh nicht gestellt, zum ersten Mal seit fünfundzwanzig Jahren, und sie geht trotzdem.  
> **ANNELIESE LECHNER:** Mei, wetten, dass der Wirt als Erster redet? Geh, um nix. Um die Ehre, die hab ich eh nimmer.  
> **OTTMAR WALD:** Bin da. (Pause) Fragst halt.  
> **WACHTMEISTER BRANDL:** Gemäß Dienstplan bin ich, äh, seit sechs Minuten nicht im Dienst. Den Block hab ich trotzdem dabei.  
>
> **SZENE** – Hauptplatz, statisch
> **SCHWESTER MARTA:** Den Brief geben Sie nicht aus der Hand, gell. Und richten S' den Kragen, der steht.  
> **XAVER GRUBER:** Mei Liebe, trink erst an Kaffee, geht aufs Haus. Einen Brief liest man doch net nüchtern vor.  
> **SCHWESTER MARTA:** Lassen S' den Brunnen, Xaver, der ist sauber. Franz hat lang genug gewartet.  
> **LALE (OFF):** Der Brief war weich vom vielen Falten. In der Jackentasche drückte die Pistole, die ich immer noch niemandem zurückgegeben hatte.  
>

**[WAHL]** Was tust du mit dem Brief?
- Vorlesen. Laut.
- Die Farben in den Brunnen kippen.
- Die Kugel behalten.

**[WENN ende = wahrheit]**

> **SZENE** – Hauptplatz, kran
> **LALE:** ›Den Ball hab ich nicht getauscht, aber gesehen hab ich's. Wir alle haben es getan: der Wirt, der Bahnhof, der Fluss, der Wald. Und ich.‹  
> **LALE:** ›Wer das liest: Erzähl es so, dass man uns hinterher noch grüßen kann.‹  
> **MENGE:** TAXLER: Gwusst hamma's doch alle, seit neunundneunzig. – BÄCKER: Halt die Goschn, Sepp. – TAXLER: Ja eh. Fünfundzwanzig Jahr lang.  
>
> **SZENE** – Hauptplatz, umkreise
> **XAVER GRUBER:** Getauscht hab ich. Verloren hätten wir, und dann wär die Altstadt weg gewesen, und mei Wirtschaft gleich mit.  
> **KONRAD WEIDINGER:** Sieben Minuten habe ich den Fahrplan verschoben, Kruzifix, sieben Minuten. Um sieben Uhr einundvierzig stand Franz allein auf Gleis zwei.  
> **ANNELIESE LECHNER:** Die Dosen hab ich übern Lech gfahren, nachts. Und ihm beim Graben zugschaut und nix gsagt, weil … geh, weil halt.  
> **OTTMAR WALD:** Schlüssel hab ich gnommen. (Pause) Fürs Maulhalten.  
> **SCHWESTER MARTA:** Sich selber hat er auch mitgezählt. Typisch. – Xaver, hören S' auf zu wischen, Sie tropfen auf den Brief.  
> **WACHTMEISTER BRANDL:** Paragraf … ist wurscht. Ich schreib's auf, alles, mit Namen. Dich auch, Marta. Wer den Schiedsrichter spielt, kommt ins Protokoll.  
>

**[SONST]**

**[WENN ende = frieden]**

> **SZENE** – Hauptplatz, kran
> **LALE:** Gelb, Blau, Grün, Weiß. Halt mal, Konrad. Wir kippen's alle zusammen rein.  
> **XAVER GRUBER:** Des wird braun, mei Liebe. Mit der Farb hab ich Lehrgeld zahlt.  
> **LALE:** Braun? Dann halt braun.  
> **LALE (OFF):** Das Wasser zog die Farben in Schlieren, eine nach der anderen, und keine wollte sich mit der nächsten vertragen. Ein Kind hielt die Hand rein und zog sie grün wieder raus.  
>
> **SZENE** – Hauptplatz, umkreise
> **SCHWESTER MARTA:** Das stand in keinem Plan von mir.  
> **LALE:** Der Brief bleibt bei mir. Den les ich vor, sobald einer von denen vergisst, warum wir heut hier stehen.  
> **SCHWESTER MARTA:** … Die Hand von dem Kind waschen S' nachher ab, gell. Die Farbe geht schlecht runter.  
> **ANNELIESE LECHNER:** Ich fahr euch rüber ans Ufer. Wetten, dass ich den Konrad nass mach? Um nix.  
> **KONRAD WEIDINGER:** Zehn Uhr. Ungefähr, Anneliese. Ungefähr zehn.  
> **WACHTMEISTER BRANDL:** Ich schreib heut nur Falschparker auf. Und den Brunnen, äh … der ist jetzt auch irgendwie falsch.  
>

**[SONST]**

> **SZENE** – Hauptplatz, statisch
> **LALE:** Die Kugel bleibt bei mir.  
> **SCHWESTER MARTA:** Wie bitte?  
> **LALE:** Ihr habt mir die Farben draufgschmiert. Alle vier. Dann sind's jetzt halt meine.  
> **XAVER GRUBER:** Mei Liebe. Genau so hab ich mir das vorgestellt. Nur mit Schürze.  
> **KONRAD WEIDINGER:** Das hat er neunundneunzig auch gesagt, Lale, fast wörtlich. Um acht Uhr zehn.  
>
> **SZENE** – Hauptplatz, umkreise
> **SCHWESTER MARTA:** Ich hab Sie auf die Straße gelegt. Ich hab Ihnen die Farbe aufgetragen und alle zehn Minuten den Puls gefühlt. Für die Wahrheit, Lale.  
> **LALE:** Die Pistole von Ihrem Bruder? Die behalt ich auch.  
> **LALE (OFF):** Marta griff nach meinem Kragen, um ihn zu richten. Dann ließ sie die Hand sinken.  
> **WACHTMEISTER BRANDL:** Ich … das Handbuch. Da steht irgendwo, was man macht, wenn alle zuschauen und keiner was tut. Ich find's gleich.  
>

### Abspann
*beginnt von selbst · Lohn 0 € · Ruf +0*

**[WENN ende = wahrheit]**

> **SZENE** – Schmalzturm, kran
> **LALE (OFF):** Drei Wochen lang stand Landsberg in der Zeitung. Der Wirt bekam ein Jahr auf Bewährung, die anderen Sozialstunden. Konrad leistet seine am Bahnhof ab, darum hat er gebeten.  
> **LALE (OFF):** Im Schmalzturm hängt jetzt eine Tafel mit Franz' Namen. Marta hat sie selbst angeschraubt und mir die vierte Schraube in die Hand gedrückt.  
> **LALE (OFF):** Die Pistole hab ich ihr zurückgegeben. Sie hat sie eingesteckt, ohne nachzuschauen, ob sie geladen war.  
>

**[SONST]**

**[WENN ende = frieden]**

> **SZENE** – Hauptplatz, kran
> **LALE (OFF):** Das erste Farbfest ging bis Mitternacht. Unterm Wehr war der Lech blau, am Mutterturm grün, und Anneliese hat jede Wette verloren. Absichtlich, sagt sie.  
> **LALE (OFF):** Die Kugel liegt im Rathaus unter Glas, grün, wie sie ist. Daneben ein Zettel in Xavers Schrift: ›Rückgabe erwünscht.‹  
>

**[SONST]**

> **SZENE** – Hauptplatz, kran
> **LALE (OFF):** Die Jacke trag ich jeden Tag. Beim Gruber am Stammtisch hab ich jetzt einen Platz, und keiner setzt sich drauf.  
> **LALE (OFF):** Marta war in der ersten Woche weg. Mitgenommen hat sie ein Foto von Franz. Den Zettel mit dem ›S.‹ hat sie mir aufs Kopfkissen gelegt.  
> **LALE (OFF):** Manchmal, kurz nach Nachtdienstende, rauscht das Funkgerät. ›Ihr Puls ist zu hoch, Lale.‹ Dann schalt ich ab. Meistens.  
>

**[TITEL]** FARBKRIEG – Mitwirkende: Lale · Schwester Marta · Konrad Weidinger · Anneliese Lechner · Ottmar Wald · Xaver Gruber · Wachtmeister Brandl · Schorsch · Pfarrer Alois

**[TITEL]** Bei diesem Spiel wurde kein echter Ball verletzt. Getauscht schon. – Alle Namen und Handlungen sind frei erfunden. Nur Landsberg ist echt.

> **SZENE** – Hauptplatz, statisch
> **WACHTMEISTER BRANDL:** Frau Lale. Strafzettel. Paintball-Wagen, Hauptplatz, am Marienbrunnen, Samstag, elf Uhr. Und, äh … ist Lale jetzt der Vor- oder der Nachname?  
> **LALE:** Vorname. Den Nachnamen hab ich noch nicht wieder.  
> **WACHTMEISTER BRANDL:** Dann lass ich das Feld offen. Ich hab Zeit, Lale. Ich schreib's dazu, wenn's Ihnen einfällt.  
>

