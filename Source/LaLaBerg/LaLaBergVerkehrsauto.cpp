#include "LaLaBergVerkehrsauto.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "ProceduralMeshComponent.h"
#include "LaLaBergWagenForm.h"
#include "Engine/World.h"
#include "EngineUtils.h"

namespace {
 // Keine echte Verkehrssimulation - nur: bremsen, wenn ein anderer
 // Verkehrswagen naeher als 9 m voraus steht, sonst faehrt jeder stur durch
 // jeden hindurch. Ampeln und Vorfahrt gibt es damit weiterhin nicht.
 float BremseVorAndauto(UWorld* Welt, const AActor* Selbst, const FVector& Ort, const FVector& Vorwaerts) {
  float Bremse = 1.0f;
  for (TActorIterator<ALaLaBergVerkehrsauto> It(Welt); It; ++It) {
   if (*It == Selbst) continue;
   const FVector Diff = It->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > 900.0f || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) continue;   // nicht voraus
   Bremse = FMath::Min(Bremse, FMath::Clamp((Dist - 260.0f) / 640.0f, 0.05f, 1.0f));
  }
  return Bremse;
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
}

void ALaLaBergVerkehrsauto::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Weg.Gueltig()) return;
 // Nach einem Treffer kurz fast stehen bleiben, dann wieder auf Tempo -
 // sonst waere ein Treffer nur eine Farbe, kein Ereignis.
 const bool bGestoert = GetWorld()->GetTimeSeconds() < StoerungBis;
 FVector Ort = GetActorLocation();
 const float Bremse = BremseVorAndauto(GetWorld(), this, Ort, GetActorForwardVector());
 const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * (bGestoert ? 0.08f : 1.0f) * Bremse);
 SetActorLocation(Ort);
 if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 3.0f));
}

// ILaLaBergFarbbar: umlackieren und kurz abbremsen.
void ALaLaBergVerkehrsauto::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 Lack = Farbe;
 LaLaBergWagenForm::BaueNetz(Netz, Lack);
 StoerungBis = GetWorld()->GetTimeSeconds() + 1.4f;
}
