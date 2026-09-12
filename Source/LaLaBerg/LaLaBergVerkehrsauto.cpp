#include "LaLaBergVerkehrsauto.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "LaLaBergWagenForm.h"
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

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Karosserie"));
 Netz->SetupAttachment(Rumpf);
 Netz->SetRelativeLocation(FVector(0, 0, -75));
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ALaLaBergVerkehrsauto::SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh) {
 Weg.Route = Punkte;
 Tempo = TempoKmh / 3.6f;
 if (Weg.Gueltig()) SetActorLocation(Weg.Start());
}

void ALaLaBergVerkehrsauto::BeginPlay() {
 Super::BeginPlay();
 LaLaBergWagenForm::BaueNetz(Netz, Lack);
 Alle.Add(this);
}

void ALaLaBergVerkehrsauto::EndPlay(const EEndPlayReason::Type Grund) {
 Alle.RemoveSingleSwap(this);
 Super::EndPlay(Grund);
}

void ALaLaBergVerkehrsauto::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Weg.Gueltig()) return;
 // Nach einem Treffer kurz fast stehen bleiben, dann wieder auf Tempo -
 // sonst waere ein Treffer nur eine Farbe, kein Ereignis.
 const bool bGestoert = GetWorld()->GetTimeSeconds() < StoerungBis;
 FVector Ort = GetActorLocation();
 const FVector Vorwaerts = GetActorForwardVector();
 const float Bremse = FMath::Min(FMath::Min(BremseVorAndauto(this, Ort, Vorwaerts), BremseVorAmpel(Ort, Vorwaerts)),
                                  BremseVorSpieler(GetWorld(), Ort, Vorwaerts));
 const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * (bGestoert ? 0.08f : 1.0f) * Bremse);
 SetActorLocation(Ort);
 if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 5.0f));
}

// ILaLaBergFarbbar: nur kurz abbremsen. Der Klecks ist schon das Decal der
// Kugel - der Wagen behaelt seinen Lack, ein Treffer faerbt ihn nicht um.
void ALaLaBergVerkehrsauto::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 StoerungBis = GetWorld()->GetTimeSeconds() + 1.4f;
}
