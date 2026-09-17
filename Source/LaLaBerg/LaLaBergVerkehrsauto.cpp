#include "LaLaBergVerkehrsauto.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "LaLaBergWagenForm.h"
#include "LaLaBergAutoPool.h"
#include "LaLaBergKastenPool.h"
#include "LaLaBergWagenTypen.h"
#include "LaLaBergAmpel.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

TArray<ALaLaBergVerkehrsauto*> ALaLaBergVerkehrsauto::Alle;
TArray<FVector> ALaLaBergVerkehrsauto::KreuzungOrte;
TArray<int32> ALaLaBergVerkehrsauto::KreuzungKlassen;
TArray<float> ALaLaBergVerkehrsauto::KreuzungBreiten;

namespace {
 // Keine echte Verkehrssimulation - nur: bremsen, wenn ein anderer
 // Verkehrswagen naeher als 9 m voraus steht, sonst faehrt jeder stur durch
 // jeden hindurch. Vorfahrt an Kreuzungen gibt es weiterhin nicht. Liefert
 // zusaetzlich den bremsenden Wagen selbst zurueck (fuer das Ueberholen
 // unten - der braucht das konkrete Auto, nicht nur den Bremsfaktor).
 ALaLaBergVerkehrsauto* NaechstesAutoVoraus(const AActor* Selbst, const FVector& Ort, const FVector& Vorwaerts, float& AusBremse) {
  ALaLaBergVerkehrsauto* Naechstes = nullptr;
  float BesteDist = 900.0f;
  for (ALaLaBergVerkehrsauto* Andere : ALaLaBergVerkehrsauto::Alle) {
   if (!Andere || Andere == Selbst) continue;
   const FVector Diff = Andere->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > 900.0f || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) continue;   // nicht voraus
   if (Dist < BesteDist) { BesteDist = Dist; Naechstes = Andere; }
  }
  AusBremse = Naechstes ? FMath::Clamp((BesteDist - 260.0f) / 640.0f, 0.05f, 1.0f) : 1.0f;
  return Naechstes;
 }
 // Vor einer Kurve langsamer werden. Seit eine Fahrt ueber den Strassen-
 // graphen mehrere Strassen verkettet (siehe Tools/Export/prepare-verkehr.cjs),
 // biegt ein Auto an Kreuzungen wirklich ab - mit voller Reisegeschwindigkeit
 // durch einen 90-Grad-Bogen sieht das aus, als schnalzte es um die Ecke:
 // die Bewegung ist kinematisch, es gibt also keine Fliehkraft, die von
 // selbst bremsen wuerde. Summiert die Richtungsaenderung der naechsten rund
 // 25 m Route auf und bremst proportional dazu ab.
 float BremseInKurve(const FLaLaBergWegfolger& Weg) {
  if (!Weg.Gueltig()) return 1.0f;
  constexpr float VORAUSSCHAU = 2500.0f;   // cm
  const int32 Anzahl = Weg.Route.Num();
  float Strecke = 0.0f, Summe = 0.0f;
  FVector Letzte = FVector::ZeroVector;
  int32 i = Weg.Index;
  // Schrittzahl begrenzt: auf einem Rundkurs (siehe FLaLaBergWegfolger::bRund)
  // liefe die Vorausschau sonst endlos im Kreis, wenn der Ring kuerzer als
  // die Vorausschau ist.
  for (int32 Schritt = 0; Schritt < Anzahl && Strecke < VORAUSSCHAU; Schritt++) {
   int32 Naechster = i + Weg.Richtung;
   if (Weg.bRund) Naechster = (Naechster + Anzahl) % Anzahl;
   if (!Weg.Route.IsValidIndex(i) || !Weg.Route.IsValidIndex(Naechster)) break;
   const FVector Segment = Weg.Route[Naechster] - Weg.Route[i];
   i = Naechster;
   const float Laenge = Segment.Size2D();
   if (Laenge < 1.0f) continue;
   const FVector Richtung = (Segment / Laenge);
   if (!Letzte.IsNearlyZero())
    Summe += FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(
     FVector::DotProduct(Letzte, Richtung), -1.0f, 1.0f)));
   Letzte = Richtung;
   Strecke += Laenge;
  }
  // 90 Grad Richtungsaenderung (eine volle Abbiegung) bremsen auf 45 Prozent.
  return FMath::Clamp(1.0f - Summe / 164.0f, 0.45f, 1.0f);
 }
 // Ist die Gegenspur frei genug zum Ueberholen? Jedes andere Auto (ausser
 // dem zu ueberholenden selbst) innerhalb der Sicherheitsreichweite grob
 // voraus zaehlt als Hindernis - unabhaengig davon, ob es entgegenkommt
 // oder in dieselbe Richtung faehrt, beides wuerde beim Ausscheren gefaehrlich.
 bool WegFreiZumUeberholen(const AActor* Selbst, const AActor* Ausgenommen, const FVector& Ort, const FVector& Vorwaerts, float Reichweite) {
  for (ALaLaBergVerkehrsauto* Andere : ALaLaBergVerkehrsauto::Alle) {
   if (!Andere || Andere == Selbst || Andere == Ausgenommen) continue;
   const FVector Diff = Andere->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > Reichweite || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.3f) continue;   // deutlich hinter uns zaehlt nicht
   return false;
  }
  return true;
 }
 // Echtes Vorfahrtsrecht an den Kreuzungen der eigenen Route (siehe
 // Tools/Export/prepare-verkehr.cjs "kreuzungen", aus dem echten OSM-
 // Strassengraphen, nicht nur Ampel-Abstandsgruppierung wie unten): ein Auto
 // bremst bis zum Stillstand (Untergrenze 0, nicht wie BremseVorAndauto -
 // echtes Anhalten statt nur Kriechen), wenn ein anderes an derselben
 // Kreuzung von der wichtigeren Strasse (niedrigere Klasse) naht. Bei
 // gleicher Klasse zaehlt der Abstand zur Kreuzung als Naeherung fuer die
 // Ankunftsreihenfolge (wer naeher dran ist, war zuerst da) - nur bei
 // echtem Gleichstand (< 3 m Unterschied) entscheidet ersatzweise Rechts-
 // vor-Links. Weiterhin kein echtes Anhalten-und-Warten-bis-frei als
 // Zustand (kein Queue-System je Kreuzung) - nur ein kontinuierliches
 // Vorrang-Bremsen wie bei den anderen BremseVor*-Funktionen, aber jetzt
 // auf echter Kreuzungstopologie statt Distanzgruppierung.
 float BremseVorKreuzung(const AActor* Selbst, const FVector& Ort, const FVector& Vorwaerts, int32 EigeneKlasse, const TArray<int32>& MeineKreuzungen) {
  float Bremse = 1.0f;
  for (int32 KIdx : MeineKreuzungen) {
   if (!ALaLaBergVerkehrsauto::KreuzungOrte.IsValidIndex(KIdx)) continue;
   // Kreuzungen tragen keine Hoehe (siehe prepare-verkehr.cjs) - Abstand
   // deshalb nur in der Grundflaeche, sonst verfaelschten Hoehenunterschiede
   // (Bruecken, Gefaelle) die Entfernung zur Kreuzung selbst.
   const FVector& KreuzOrt = ALaLaBergVerkehrsauto::KreuzungOrte[KIdx];
   const float EigenerAbstand = FVector::Dist2D(Ort, KreuzOrt);
   // Anhalteabstand von der tatsaechlichen Kreuzungsbreite abhaengig statt
   // eines fuer jede Kreuzung gleichen Werts (siehe KreuzungBreiten): eine
   // breite oder mehrarmige Kreuzung braucht mehr Platz zum Anhalten, sonst
   // stand ein wartendes Auto sichtbar mitten in der querenden Fahrbahn.
   // Halbe Kreuzungsbreite (bis zum eigenen Fahrbahnrand) plus die halbe
   // Wagenlaenge (rund 210 cm, siehe Karosse-Querschnitte in
   // LaLaBergWagenForm) als Sicherheitsabstand zur Vorderstossstange.
   const float Puffer = ALaLaBergVerkehrsauto::KreuzungBreiten.IsValidIndex(KIdx)
    ? ALaLaBergVerkehrsauto::KreuzungBreiten[KIdx] * 50.0f + 210.0f : 300.0f;
   const float Relevanz = Puffer + 1100.0f;
   if (EigenerAbstand > Relevanz) continue;   // Kreuzung noch nicht relevant
   for (ALaLaBergVerkehrsauto* Andere : ALaLaBergVerkehrsauto::Alle) {
    if (!Andere || Andere == Selbst || !Andere->HoleKreuzungen().Contains(KIdx)) continue;
    const FVector AndererOrt = Andere->GetActorLocation();
    const float AndererAbstand = FVector::Dist2D(AndererOrt, KreuzOrt);
    if (AndererAbstand > Relevanz) continue;
    bool bMussWarten = Andere->HoleKlasse() < EigeneKlasse;
    if (!bMussWarten && Andere->HoleKlasse() == EigeneKlasse) {
     if (AndererAbstand < EigenerAbstand - 300.0f) bMussWarten = true;
     else if (FMath::Abs(AndererAbstand - EigenerAbstand) <= 300.0f)
      bMussWarten = FVector::DotProduct(Selbst->GetActorRightVector(), (AndererOrt - Ort).GetSafeNormal()) > 0.3f;
    }
    if (bMussWarten)
     Bremse = FMath::Min(Bremse, FMath::Clamp((EigenerAbstand - Puffer) / 1100.0f, 0.0f, 1.0f));
   }
  }
  return Bremse;
 }
 // Vor einer roten oder gelben Ampel bis zum Stillstand abbremsen. Keine
 // Zuordnung zu einer bestimmten Kreuzung - die naechste Ampel auf dem
 // Fahrweg zaehlt, unabhaengig davon, zu welcher Strasse sie eigentlich gehoert.
 float BremseVorAmpel(const FVector& Ort, const FVector& Vorwaerts) {
  float Bremse = 1.0f;
  for (ALaLaBergAmpel* Ampel : ALaLaBergAmpel::Alle) {
   if (!Ampel || !Ampel->HaeltAn()) continue;
   const FVector Diff = Ampel->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > 1400.0f || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.7f) continue;   // nicht voraus auf der Strecke
   Bremse = FMath::Min(Bremse, FMath::Clamp((Dist - 350.0f) / 900.0f, 0.0f, 1.0f));
  }
  return Bremse;
 }
 // Bremst vor dem Spieler genau wie vor einem anderen Verkehrswagen - egal
 // ob zu Fuss oder im fahrbaren Wagen, die KI sah ihn zuvor gar nicht.
 float BremseVorSpieler(const UWorld* Welt, const FVector& Ort, const FVector& Vorwaerts) {
  const APawn* SpielerPawn = UGameplayStatics::GetPlayerPawn(Welt, 0);
  if (!SpielerPawn) return 1.0f;
  const FVector Diff = SpielerPawn->GetActorLocation() - Ort;
  const float Dist = Diff.Size();
  if (Dist > 900.0f || Dist < 1.0f) return 1.0f;
  if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) return 1.0f;
  return FMath::Clamp((Dist - 260.0f) / 640.0f, 0.05f, 1.0f);
 }
 // Groesster zusaetzlicher seitlicher Versatz, den ein Wagen einem Hindernis
 // ausweicht - nach rechts, wie im echten Verkehr ueblich. Keine echte
 // Fahrspur-Breite: der Versatz ist ein fester Wert, kein Blick darauf, ob
 // rechts ueberhaupt noch Fahrbahn ist. Nur vor Autos/Spieler, nicht vor
 // einer roten Ampel - dort soll stehenbleiben, nicht vorbeischleichen, das
 // richtige Verhalten sein.
 constexpr float MAX_SEITVERSATZ = 220.0f;
 // Halbe Fahrzeugbreite (siehe Rumpf->InitBoxExtent unten) - Sicherheitsrand,
 // damit der Gesamtversatz (Spur + Ausweichen) das Auto nie ueber den
 // eigentlichen Fahrbahnrand hinausschiebt, auch nicht auf einer schmalen
 // Strasse mit wenig Platz zum Ausweichen.
 constexpr float HALBE_WAGENBREITE = 88.0f;
 // Ueberholen: ein deutlich langsameres oder stehendes Auto direkt voraus
 // (nicht der Spieler - der wird nicht ueberholt, nur gebremst) wird
 // umfahren statt endlos dahinter herzukriechen. Nur auf ausreichend
 // breiter Strasse (zwei Spuren plus etwas Reserve) und nur, wenn die
 // Gegenspur ueber die Sicherheitsreichweite frei ist - keine Ruecksicht
 // auf eine bestimmte Ueberholsichtweite entlang von Kurven, nur auf
 // andere Autos im geraden Vorausblick.
 constexpr float UEBERHOL_MINDESTBREITE = 550.0f;
 constexpr float UEBERHOL_SICHERHEIT = 1800.0f;
 constexpr float UEBERHOL_ABSTAND_FREI = 500.0f;
 // Sichtweiten-LOD (siehe Tick): jenseits davon der leichte Kasten statt
 // des CarConcept-Detailmodells. 70 Autos gleichzeitig im Detailmodell
 // druecken die Bildrate auf 2 fps (Glas/Chrom-Material, viele Dreiecke je
 // Wagen) - mit diesem Radius sind es realistisch nur eine Handvoll.
 constexpr float LOD_ABSTAND = 8000.0f;
 // City-Sample-Fahrzeuge (siehe LaLaBergWagenTypen) sind fuer die Engine
 // selbst gebaut, anders als das aus einem externen glTF-Sample importierte
 // CarConcept (-90 Grad, siehe unten) - bislang keine Drehkorrektur noetig,
 // per Testbild geprueft (siehe Begruendung bei ALaLaBergWagen::BaueKarosserie).
 constexpr float TYP_DREHKORREKTUR = 0.0f;
}

ALaLaBergVerkehrsauto::ALaLaBergVerkehrsauto() {
 PrimaryActorTick.bCanEverTick = true;

 // Eigener Wurzelpunkt auf Fahrbahnhoehe (dort setzt die Route den Wagen
 // ab); die Stossstange aus wagen.json beginnt bei Z=0 in genau diesem
 // Bezug. Rumpf und Netz sitzen darueber, um den Wagen als Kasten mittig
 // zu erfassen statt zur Haelfte im Boden zu stecken.
 auto* Wurzel = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
 SetRootComponent(Wurzel);

 Rumpf = CreateDefaultSubobject<UBoxComponent>(TEXT("Rumpf"));
 Rumpf->SetupAttachment(Wurzel);
 Rumpf->SetRelativeLocation(FVector(0, 0, 75));
 Rumpf->InitBoxExtent(FVector(210, 88, 75));
 Rumpf->SetCollisionProfileName(TEXT("BlockAllDynamic"));
 // Kinematisch: die Route bestimmt die Lage, keine eigene Physiksimulation -
 // trotzdem ein festes Hindernis fuer Spieler und Farbkugeln.
 Rumpf->SetSimulatePhysics(false);

 Karosseriepunkt = CreateDefaultSubobject<USceneComponent>(TEXT("Karosseriepunkt"));
 Karosseriepunkt->SetupAttachment(Rumpf);
 Karosseriepunkt->SetRelativeLocation(FVector(0, 0, -75));

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Karosserie"));
 Netz->SetupAttachment(Karosseriepunkt);
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALaLaBergVerkehrsauto::SetzeLack(const FLinearColor& Farbe) {
 Lack = Farbe;
 // Nur fuer geparkte Autos gerufen (siehe LadeVerkehr) - deren Kasten kommt
 // aus dem gemeinsamen Pool statt aus einem eigenen Netz (siehe
 // ALaLaBergKastenPool). BeginPlay hat das Netz schon grau gebaut, bevor
 // diese Faerbung eintrifft (SpawnActor ruft BeginPlay vor SetzeLack auf) -
 // die leere ClearAllMeshSections raeumt die ueberfluessige Kopie weg.
 if (ALaLaBergKastenPool::Instanz) {
  KastenGriff = ALaLaBergKastenPool::Instanz->FuegeHinzu(Lack, GetActorTransform());
  if (KastenGriff.Gueltig()) {
   bKastenGepoolt = true;
   if (bNetzGebaut) Netz->ClearAllMeshSections();
   return;
  }
 }
 if (bNetzGebaut) LaLaBergWagenForm::BaueNetz(Netz, Lack);
}

void ALaLaBergVerkehrsauto::SetzeSpurdaten(const TArray<float>& BreitenM, const TArray<int32>& Klassen) {
 BreiteJeWegpunkt.Reset(BreitenM.Num());
 for (float BreiteM : BreitenM) BreiteJeWegpunkt.Add(BreiteM * 100.0f);
 KlasseJeWegpunkt = Klassen;
}

void ALaLaBergVerkehrsauto::SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh) {
 Weg.Route = Punkte;
 Tempo = TempoKmh / 3.6f;
 if (Weg.Gueltig()) {
  SetActorLocation(Weg.Start());
  // Nur fahrende Autos brauchen sich gegenseitig zu bremsen (siehe
  // BremseVorAndauto) - die 1078 geparkten Autos ohne Route bleiben aus
  // Alle heraus, sonst waechst diese Schleife von 70x70 auf 70x1148
  // Abstandspruefungen pro Bild (19 statt 26 fps im Fahrtest gemessen).
  Alle.Add(this);
 }
}

void ALaLaBergVerkehrsauto::BeginPlay() {
 Super::BeginPlay();
 // Immer beide Formen anlegen: den leichten Kasten aus wagen.json (sichtbar
 // per Default) und, falls verfuegbar, eine reservierte, zunaechst versteckte
 // Instanz im CarConcept-Pool. Tick() blendet je nach Abstand zum Spieler
 // zwischen beiden um - siehe bDetailliert. Selbst als gebuendelte Pool-
 // Instanzen blieben alle 70 Autos gleichzeitig im Detailmodell zu teuer
 // (Glas/Chrom, viele Dreiecke - 2 fps im Test), nur wenige nahe Autos
 // gleichzeitig sind es nicht.
 LaLaBergWagenForm::BaueNetz(Netz, Lack);
 bNetzGebaut = true;
 if (!ALaLaBergAutoPool::Instanz || !ALaLaBergAutoPool::Instanz->Gueltig()) return;

 // Zufaellig entweder das bisherige CarConcept (-1) oder einer der
 // realistischen City-Sample-Typen (siehe LaLaBergWagenTypen) - fuer
 // Fahrzeugvielfalt statt eines einzigen Modells fuer jedes KI-Auto.
 FahrzeugTyp = FMath::RandRange(-1, LaLaBergWagenTypen::TYPEN_ANZAHL - 1);
 if (FahrzeugTyp >= 0 && ALaLaBergAutoPool::Instanz->TypGueltig(FahrzeugTyp)) {
  const FTransform Versteckt(FVector(0, 0, -500000.0f));
  TypPoolIndizes = ALaLaBergAutoPool::Instanz->FuegeTypHinzu(FahrzeugTyp, Versteckt);
  // An der wirklichen Stelle sichtbar von Anfang an - dieselbe Rolle wie der
  // Kasten aus wagen.json beim CarConcept-Pfad (siehe unten), nur mit dem
  // eigenen, bereits zusammengefassten Fern-Mesh des Typs statt der
  // prozeduralen Form. Fehlt eins (z.B. vehicle07_Car), bleibt der leichte
  // Kasten unnoetig - das Detailmodell zeigt sich dann immer (siehe Tick).
  TypFernIndex = ALaLaBergAutoPool::Instanz->FuegeTypFernHinzu(FahrzeugTyp, GetActorTransform());
  bTypFernBenutzt = TypFernIndex >= 0;
  bPoolGenutzt = true;
  Netz->ClearAllMeshSections();
  Netz->SetVisibility(false);
 } else {
  FahrzeugTyp = -1;
  PoolIndizes = ALaLaBergAutoPool::Instanz->FuegeHinzu(FTransform(FVector(0, 0, -500000.0f)));
  bPoolGenutzt = true;
 }
}

void ALaLaBergVerkehrsauto::EndPlay(const EEndPlayReason::Type Grund) {
 // Nicht entfernen, nur verstecken - siehe ALaLaBergAutoPool::Verstecke.
 if (bPoolGenutzt && ALaLaBergAutoPool::Instanz) {
  if (FahrzeugTyp == -1) {
   ALaLaBergAutoPool::Instanz->Verstecke(PoolIndizes);
  } else {
   ALaLaBergAutoPool::Instanz->VersteckeTyp(FahrzeugTyp, TypPoolIndizes);
   if (bTypFernBenutzt) ALaLaBergAutoPool::Instanz->VersteckeTypFern(FahrzeugTyp, TypFernIndex);
  }
 }
 if (bKastenGepoolt && ALaLaBergKastenPool::Instanz) ALaLaBergKastenPool::Instanz->Verstecke(KastenGriff);
 Alle.RemoveSingleSwap(this);
 Super::EndPlay(Grund);
}

void ALaLaBergVerkehrsauto::Tick(float Zeit) {
 Super::Tick(Zeit);
 // Ohne gueltige Route (die 1078 geparkten Autos - siehe LadeVerkehr) faellt
 // nur die Fahr-/Brems-/Ausweichlogik weg, nicht das Sichtweiten-LOD weiter
 // unten: ein stehendes Auto soll trotzdem aus der Naehe echt aussehen.
 if (Weg.Gueltig()) {
  // Nach einem Treffer kurz fast stehen bleiben, dann wieder auf Tempo -
  // sonst waere ein Treffer nur eine Farbe, kein Ereignis.
  const bool bGestoert = GetWorld()->GetTimeSeconds() < StoerungBis;
  FVector Ort = GetActorLocation();
  const FVector Vorwaerts = GetActorForwardVector();

  float BremseAndauto = 1.0f;
  ALaLaBergVerkehrsauto* Voraus = NaechstesAutoVoraus(this, Ort, Vorwaerts, BremseAndauto);
  const float BremseAmpel = BremseVorAmpel(Ort, Vorwaerts);
  const float BremseKreuzung = BremseVorKreuzung(this, Ort, Vorwaerts, HoleKlasse(), MeineKreuzungen);
  const float BremseKurve = BremseInKurve(Weg);

  // Ueberholen starten: ein spuerbar langsameres oder stehendes Auto direkt
  // voraus (BremseAndauto < 0.75 - eine erste Kalibrierung mit 0.55 loeste
  // in 256s Beobachtung organischen Verkehrs kein einziges Mal aus), genug
  // breite Strasse (siehe UEBERHOL_MINDESTBREITE), keine unmittelbar
  // anstehende Ampel oder Vorfahrt (sonst nur, um in der Gegenspur ebenfalls
  // anzuhalten) und eine ueber die Sicherheitsreichweite freie Gegenspur.
  if (!Ueberholt.IsValid() && Voraus
      && BreiteJetzt() >= UEBERHOL_MINDESTBREITE && BremseAndauto < 0.75f
      && BremseAmpel > 0.8f && BremseKreuzung > 0.8f && !bGestoert
      && WegFreiZumUeberholen(this, Voraus, Ort, Vorwaerts, UEBERHOL_SICHERHEIT))
   Ueberholt = Voraus;
  // Ueberholen beenden: das Auto liegt hinter uns (vorbei), ist
  // verschwunden, oder eine Ampel/Vorfahrt macht das Ausscheren in die
  // Gegenspur sinnlos - dann lieber wieder normal einordnen und mitbremsen.
  if (Ueberholt.IsValid()) {
   const float VorausDist = FVector::DotProduct(Ueberholt->GetActorLocation() - Ort, Vorwaerts);
   if (VorausDist < -UEBERHOL_ABSTAND_FREI || BremseAmpel < 0.8f || BremseKreuzung < 0.8f
       || BreiteJetzt() < UEBERHOL_MINDESTBREITE)
    Ueberholt = nullptr;
  }
  // Waehrend des Ueberholens nicht mehr fuer das ueberholte Auto selbst
  // bremsen - sonst kaeme der Wagen trotz Ausscheren kaum naeher heran.
  const bool bUeberholtGerade = Ueberholt.IsValid() && Ueberholt.Get() == Voraus;
  const float BremseObjekt = FMath::Min(bUeberholtGerade ? 1.0f : BremseAndauto, BremseVorSpieler(GetWorld(), Ort, Vorwaerts));
  const float Bremse = FMath::Min(FMath::Min(BremseObjekt, BremseAmpel), FMath::Min(BremseKreuzung, BremseKurve));
  const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * (bGestoert ? 0.08f : 1.0f) * Bremse);
  SetActorLocation(Ort);
  if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 5.0f));

  // Weicht einem Auto oder dem Spieler direkt voraus zusaetzlich seitlich
  // aus (nach rechts), statt nur davor stehenzubleiben - nicht vor einer
  // roten Ampel, da soll die Fahrt tatsaechlich enden. Der dauerhafte
  // Spur-Versatz selbst gilt immer (siehe unten) - echte Fahrspurtrennung
  // anhand der Fahrbahnbreite aus den Quelldaten (StrassenBreite, siehe
  // SetzeStrassenbreite) statt eines fuer jede Strasse gleichen Werts: bei
  // zwei Spuren liegt die Mitte der eigenen Spur ein Viertel der Breite
  // neben der Fahrbahnmitte. Der Gesamtversatz (Spur + Ausweichen) bleibt
  // innerhalb des Fahrbahnrands - auf einer schmalen Strasse bleibt dafuer
  // weniger Platz zum Ausweichen, statt ueber den Rand hinauszufahren.
  // Beim Ueberholen zaehlt stattdessen die Gegenspur (derselbe Abstand,
  // nur auf der anderen Seite) statt des Ausweich-Zuschlags.
  const float BreiteHier = BreiteJetzt();
  const float SpurVersatz = BreiteHier * 0.25f;
  const float Rand = FMath::Max(0.0f, BreiteHier * 0.5f - HALBE_WAGENBREITE);
  const float SeitZiel = Ueberholt.IsValid()
   ? FMath::Clamp(-SpurVersatz, -Rand, Rand)
   : FMath::Min(SpurVersatz + (BremseObjekt < 0.9f ? MAX_SEITVERSATZ : 0.0f), Rand);
  Seitversatz = FMath::FInterpTo(Seitversatz, SeitZiel, Zeit, 0.7f);
  if (FMath::Abs(Seitversatz) > 0.5f) SetActorLocation(GetActorLocation() + GetActorRightVector() * Seitversatz);
 }

 // Sichtweiten-LOD: das Detailmodell (Pool-Instanz) nur nah am Spieler, sonst
 // der leichte Kasten - siehe Begruendung in BeginPlay. Der Wechsel selbst
 // (Sichtbarkeit umschalten) passiert nur bei einem tatsaechlichen Uebergang,
 // nicht jedes Bild - waere sonst derselbe unnoetige Zustandswechsel
 // hunderte Male pro Sekunde.
 if (bPoolGenutzt && ALaLaBergAutoPool::Instanz) {
  const APawn* SpielerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
  const bool bInNaehe = SpielerPawn && FVector::DistSquared(SpielerPawn->GetActorLocation(), GetActorLocation()) < LOD_ABSTAND * LOD_ABSTAND;
  // Ohne eigenes Fern-Mesh (FahrzeugTyp >= 0, aber bTypFernBenutzt falsch -
  // siehe BeginPlay) gibt es keinen Fernzustand, in den umgeschaltet werden
  // koennte: das Detailmodell bleibt dann immer an.
  const bool bHatFern = FahrzeugTyp == -1 || bTypFernBenutzt;
  const bool bSollDetail = !bHatFern || bInNaehe;
  if (bSollDetail != bDetailliert) {
   bDetailliert = bSollDetail;
   if (FahrzeugTyp == -1) {
    Netz->SetVisibility(!bDetailliert);
    if (!bDetailliert) ALaLaBergAutoPool::Instanz->Verstecke(PoolIndizes);
    // Derselbe Umschalter fuer den gepoolten Kasten (geparkte Autos, siehe
    // SetzeLack) - der bewegt sich nie, GetActorTransform() bleibt also ueber
    // die ganze Standzeit richtig.
    if (bKastenGepoolt && ALaLaBergKastenPool::Instanz) {
     if (bDetailliert) ALaLaBergKastenPool::Instanz->Verstecke(KastenGriff);
     else ALaLaBergKastenPool::Instanz->Aktualisiere(KastenGriff, GetActorTransform());
    }
   } else if (bTypFernBenutzt) {
    if (!bDetailliert) ALaLaBergAutoPool::Instanz->VersteckeTyp(FahrzeugTyp, TypPoolIndizes);
    if (bDetailliert) ALaLaBergAutoPool::Instanz->VersteckeTypFern(FahrzeugTyp, TypFernIndex);
    else ALaLaBergAutoPool::Instanz->AktualisiereTypFern(FahrzeugTyp, TypFernIndex, GetActorTransform());
   }
  }
  if (FahrzeugTyp == -1) {
   if (bDetailliert) ALaLaBergAutoPool::Instanz->Aktualisiere(PoolIndizes, FTransform(GetActorRotation() + FRotator(0, -90, 0), GetActorLocation()));
  } else {
   if (bDetailliert)
    ALaLaBergAutoPool::Instanz->AktualisiereTyp(FahrzeugTyp, TypPoolIndizes, FTransform(GetActorRotation() + FRotator(0, TYP_DREHKORREKTUR, 0), GetActorLocation()));
   // Faehrt der Wagen (Weg.Gueltig()) und ist gerade weit weg, muss das
   // Fern-Mesh trotzdem folgen - anders als Netz oben ist es keine
   // angehaengte Komponente, die sich automatisch mitbewegt. Ein geparktes
   // Auto steht ohnehin fest (siehe BeginPlay), braucht das nicht.
   else if (bTypFernBenutzt && Weg.Gueltig()) ALaLaBergAutoPool::Instanz->AktualisiereTypFern(FahrzeugTyp, TypFernIndex, GetActorTransform());
  }
 }
}

// ILaLaBergFarbbar: nur kurz abbremsen. Der Klecks ist schon das Decal der
// Kugel - der Wagen behaelt seinen Lack, ein Treffer faerbt ihn nicht um.
void ALaLaBergVerkehrsauto::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 StoerungBis = GetWorld()->GetTimeSeconds() + 1.4f;
}
