#include "LaLaBergVerkehrsauto.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "LaLaBergWagenForm.h"
#include "LaLaBergAutoPool.h"
#include "LaLaBergKastenPool.h"
#include "LaLaBergAmpel.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"

TArray<ALaLaBergVerkehrsauto*> ALaLaBergVerkehrsauto::Alle;

namespace {
 // Keine echte Verkehrssimulation - nur: bremsen, wenn ein anderer
 // Verkehrswagen naeher als 9 m voraus steht, sonst faehrt jeder stur durch
 // jeden hindurch. Vorfahrt an Kreuzungen gibt es weiterhin nicht.
 float BremseVorAndauto(const AActor* Selbst, const FVector& Ort, const FVector& Vorwaerts) {
  float Bremse = 1.0f;
  for (ALaLaBergVerkehrsauto* Andere : ALaLaBergVerkehrsauto::Alle) {
   if (!Andere || Andere == Selbst) continue;
   const FVector Diff = Andere->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > 900.0f || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) continue;   // nicht voraus
   Bremse = FMath::Min(Bremse, FMath::Clamp((Dist - 260.0f) / 640.0f, 0.05f, 1.0f));
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
 // Groesster seitlicher Versatz, den ein Wagen einem Hindernis ausweicht -
 // nach rechts, wie im echten Verkehr ueblich. Keine Fahrspur-Erkennung: der
 // Versatz ist ein fester Wert, kein Blick darauf, ob rechts ueberhaupt noch
 // Fahrbahn ist. Nur vor Autos/Spieler, nicht vor einer roten Ampel - dort
 // soll stehenbleiben, nicht vorbeischleichen, das richtige Verhalten sein.
 constexpr float MAX_SEITVERSATZ = 220.0f;
 // Sichtweiten-LOD (siehe Tick): jenseits davon der leichte Kasten statt
 // des CarConcept-Detailmodells. 70 Autos gleichzeitig im Detailmodell
 // druecken die Bildrate auf 2 fps (Glas/Chrom-Material, viele Dreiecke je
 // Wagen) - mit diesem Radius sind es realistisch nur eine Handvoll.
 constexpr float LOD_ABSTAND = 8000.0f;
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
 if (ALaLaBergAutoPool::Instanz && ALaLaBergAutoPool::Instanz->Gueltig()) {
  PoolIndizes = ALaLaBergAutoPool::Instanz->FuegeHinzu(FTransform(FVector(0, 0, -500000.0f)));
  bPoolGenutzt = true;
 }
}

void ALaLaBergVerkehrsauto::EndPlay(const EEndPlayReason::Type Grund) {
 // Nicht entfernen, nur verstecken - siehe ALaLaBergAutoPool::Verstecke.
 if (bPoolGenutzt && ALaLaBergAutoPool::Instanz) ALaLaBergAutoPool::Instanz->Verstecke(PoolIndizes);
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
  const float BremseObjekt = FMath::Min(BremseVorAndauto(this, Ort, Vorwaerts), BremseVorSpieler(GetWorld(), Ort, Vorwaerts));
  const float Bremse = FMath::Min(BremseObjekt, BremseVorAmpel(Ort, Vorwaerts));
  const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * (bGestoert ? 0.08f : 1.0f) * Bremse);
  SetActorLocation(Ort);
  if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 5.0f));

  // Weicht einem Auto oder dem Spieler direkt voraus seitlich aus (nach
  // rechts), statt nur davor stehenzubleiben - nicht vor einer roten Ampel,
  // da soll die Fahrt tatsaechlich enden. Keine Fahrspur-Erkennung: ein
  // fester Versatz, sanft ein- und wieder ausgeblendet.
  const float SeitZiel = BremseObjekt < 0.9f ? MAX_SEITVERSATZ : 0.0f;
  Seitversatz = FMath::FInterpTo(Seitversatz, SeitZiel, Zeit, 0.7f);
  if (FMath::Abs(Seitversatz) > 0.5f) SetActorLocation(GetActorLocation() + GetActorRightVector() * Seitversatz);
 }

 // Sichtweiten-LOD: das Detailmodell (Pool-Instanz) nur nah am Spieler, sonst
 // der leichte Kasten - siehe Begruendung in BeginPlay. Der Wechsel selbst
 // (Sichtbarkeit umschalten) passiert nur bei einem tatsaechlichen Uebergang,
 // nicht jedes Bild - waere sonst derselbe unnoetige Zustandswechsel
 // hunderte Male pro Sekunde.
 if (bPoolGenutzt) {
  const APawn* SpielerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
  const bool bSollDetail = SpielerPawn && FVector::DistSquared(SpielerPawn->GetActorLocation(), GetActorLocation()) < LOD_ABSTAND * LOD_ABSTAND;
  if (bSollDetail != bDetailliert) {
   bDetailliert = bSollDetail;
   Netz->SetVisibility(!bDetailliert);
   if (!bDetailliert && ALaLaBergAutoPool::Instanz) ALaLaBergAutoPool::Instanz->Verstecke(PoolIndizes);
   // Derselbe Umschalter fuer den gepoolten Kasten (geparkte Autos, siehe
   // SetzeLack) - der bewegt sich nie, GetActorTransform() bleibt also ueber
   // die ganze Standzeit richtig.
   if (bKastenGepoolt && ALaLaBergKastenPool::Instanz) {
    if (bDetailliert) ALaLaBergKastenPool::Instanz->Verstecke(KastenGriff);
    else ALaLaBergKastenPool::Instanz->Aktualisiere(KastenGriff, GetActorTransform());
   }
  }
  if (bDetailliert && ALaLaBergAutoPool::Instanz)
   ALaLaBergAutoPool::Instanz->Aktualisiere(PoolIndizes, FTransform(GetActorRotation() + FRotator(0, -90, 0), GetActorLocation()));
 }
}

// ILaLaBergFarbbar: nur kurz abbremsen. Der Klecks ist schon das Decal der
// Kugel - der Wagen behaelt seinen Lack, ein Treffer faerbt ihn nicht um.
void ALaLaBergVerkehrsauto::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 StoerungBis = GetWorld()->GetTimeSeconds() + 1.4f;
}
