#include "LaLaBergFarbkugel.h"
#include "EngineUtils.h"
#include "Components/SphereComponent.h"
#include "ProceduralMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/DecalComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergKonto.h"
#include "LaLaBergRevier.h"
#include "LaLaBergSchiessbude.h"
#include "Components/StaticMeshComponent.h"
#include "LaLaBergGameMode.h"
#include "LaLaBergEinschlagblitz.h"
#include "Sound/SoundBase.h"

namespace {
 // Kleine Vollkugel aus Breiten- und Laengenringen - genuegt bei 1-6 cm
 // Radius vollauf, mehr Facetten sieht man ohnehin nicht mehr.
 void kugelNetz(float R, TArray<FVector>& Punkte, TArray<int32>& Kanten) {
  constexpr int32 Ringe = 6, Segmente = 8;
  for (int32 r = 0; r <= Ringe; r++) {
   const float Phi = PI * r / Ringe;                 // 0 (oben) bis Pi (unten)
   for (int32 s = 0; s <= Segmente; s++) {
    const float Theta = 2 * PI * s / Segmente;
    Punkte.Add(FVector(FMath::Sin(Phi) * FMath::Cos(Theta), FMath::Sin(Phi) * FMath::Sin(Theta), FMath::Cos(Phi)) * R);
   }
  }
  for (int32 r = 0; r < Ringe; r++) {
   for (int32 s = 0; s < Segmente; s++) {
    const int32 a = r * (Segmente + 1) + s, b = a + Segmente + 1;
    Kanten.Append({ a, b, a + 1, a + 1, b, b + 1 });
   }
  }
 }
}

ALaLaBergFarbkugel::ALaLaBergFarbkugel() {
 PrimaryActorTick.bCanEverTick = false;

 Huelle = CreateDefaultSubobject<USphereComponent>(TEXT("Huelle"));
 Huelle->InitSphereRadius(1.4f);
 Huelle->SetCollisionProfileName(TEXT("BlockAllDynamic"));
 Huelle->SetNotifyRigidBodyCollision(true);
 SetRootComponent(Huelle);

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Netz"));
 Netz->SetupAttachment(Huelle);
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 Flugbahn = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Flugbahn"));
 Flugbahn->UpdatedComponent = Huelle;
 Flugbahn->ProjectileGravityScale = 0.55f;
 Flugbahn->bRotationFollowsVelocity = true;
 Flugbahn->bShouldBounce = false;

 SetLifeSpan(8.0f);          // falls sie nichts trifft: nicht ewig fliegen
}

void ALaLaBergFarbkugel::Einrichten(const FLinearColor& NeueFarbe, float KugelRadius, float NeuKleckMin, float NeuKleckMax, float Schwerkraft) {
 Farbe = NeueFarbe; Radius = KugelRadius; KleckMin = NeuKleckMin; KleckMax = NeuKleckMax;
 Huelle->SetSphereRadius(Radius);
 Flugbahn->ProjectileGravityScale = Schwerkraft;
}

void ALaLaBergFarbkugel::BeginPlay() {
 Super::BeginPlay();
 Huelle->OnComponentHit.AddDynamic(this, &ALaLaBergFarbkugel::Aufprall);
 if (AActor* Halter = GetOwner()) Huelle->IgnoreActorWhenMoving(Halter, true);

 TArray<FVector> Punkte; TArray<int32> Kanten;
 kugelNetz(Radius, Punkte, Kanten);
 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FLinearColor> Farben; TArray<FProcMeshTangent> Tangenten;
 for (const FVector& P : Punkte) { Normalen.Add(P.GetSafeNormal()); UVs.Add(FVector2D(0, 0)); Farben.Add(Farbe); }
 Netz->CreateMeshSection_LinearColor(0, Punkte, Kanten, Normalen, UVs, Farben, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Lack.M_Lack"))) Netz->SetMaterial(0, M);
}

void ALaLaBergFarbkugel::Abschiessen(const FVector& Richtung, float Tempo) {
 Flugbahn->Velocity = Richtung.GetSafeNormal() * Tempo;
}

void ALaLaBergFarbkugel::IgnoriereGeschwister(AActor* Anderer) {
 if (Anderer) Huelle->IgnoreActorWhenMoving(Anderer, true);
}

// Ein Farbklecks auf der getroffenen Flaeche - Beton, Fassade, Wagen, alles
// nimmt Farbe an. Traegt der Getroffene ILaLaBergFarbbar (Wagen, Passant,
// Verkehrswagen), bekommt er zusaetzlich eine eigene Reaktion; alles andere
// in der Stadt ist gebaute Geometrie, dort bleibt nur der Klecks.
void ALaLaBergFarbkugel::Aufprall(UPrimitiveComponent* TroffeneKomponente, AActor* AndererActor,
                                 UPrimitiveComponent* AndereKomponente, FVector NormalImpuls, const FHitResult& Treffer) {
 // Zwei Kugeln, die sich treffen, sind kein Treffer - nur ihre Ignorierliste
 // hat es einmal nicht rechtzeitig erfasst (z.B. Schrotflinte auf weite
 // Distanz, wo sich Flugbahnen erst spaeter kreuzen).
 if (AndererActor == this || AndererActor == GetOwner() || Cast<ALaLaBergFarbkugel>(AndererActor)) return;
 auto* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Farbklecks.M_Farbklecks"));
 if (Material) {
  const FRotator Ausrichtung = Treffer.ImpactNormal.Rotation();
  const float Groesse = FMath::FRandRange(KleckMin, KleckMax);
  // An die getroffene Flaeche angeheftet, nicht frei im Raum plaziert: sonst
  // bliebe der Klecks auf einem fahrenden Wagen oder einem gehenden
  // Passanten an der Trefferstelle in der Luft haengen, statt mit der
  // Flaeche mitzufahren. Fuer unbewegliche Waende bewegt sich der Elternteil
  // ohnehin nie, das Ergebnis ist dasselbe wie vorher.
  UDecalComponent* Decal = AndereKomponente
   ? UGameplayStatics::SpawnDecalAttached(Material, FVector(4.0f, Groesse, Groesse), AndereKomponente, NAME_None,
                                          Treffer.Location, Ausrichtung, EAttachLocation::KeepWorldPosition, 45.0f)
   : UGameplayStatics::SpawnDecalAtLocation(this, Material, FVector(4.0f, Groesse, Groesse), Treffer.Location, Ausrichtung, 45.0f);
  if (Decal) {
   if (auto* Dyn = Decal->CreateDynamicMaterialInstance()) Dyn->SetVectorParameterValue(TEXT("Farbe"), Farbe);
  }
 }
 if (auto* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Klecks.SFX_Klecks")))
  UGameplayStatics::PlaySoundAtLocation(this, Sound, Treffer.Location, 1.0f, FMath::FRandRange(0.92f, 1.08f));
 if (auto* Blitz = GetWorld()->SpawnActor<ALaLaBergEinschlagblitz>(Treffer.Location, FRotator::ZeroRotator))
  Blitz->Einrichten(Farbe, 3500.0f, 220.0f, 0.10f);
 // Wahrzeichen des Farbkriegs: eine getroffene Saeule gilt als markiert
 // (siehe LaLaBergRevier und Docs/Geschichte.md).
 if (auto* Getroffen = Cast<UStaticMeshComponent>(AndereKomponente)) {
  if (auto* Revier = ALaLaBergRevier::Instanz.Get()) Revier->Markiere(Getroffen, Farbe);
  // Dieselbe Stelle fuer die Scheiben der Schiessbude.
  if (auto* Bude = ALaLaBergSchiessbude::Instanz.Get()) Bude->Treffer(Getroffen);
 }
 if (auto* Reaktion = Cast<ILaLaBergFarbbar>(AndererActor)) {
  Reaktion->ErhalteFarbe(Farbe, GetVelocity());
  // Getroffen zu haben uebt das Zielen - eine Hauswand zaehlt nicht.
  if (auto* Konto = ULaLaBergKonto::Hole(this)) Konto->Uebe(ULaLaBergKonto::EWert::Zielsicherheit, 6.0f);
 }
 // Panzerkanone: Wirkung im Umkreis, nach aussen linear abnehmend. Eine
 // Paintballkugel hat keine Wucht und ueberspringt das hier.
 if (Wucht > 0.0f && WuchtRadius > 0.0f) {
  int32 Getroffen = 0;
  for (TActorIterator<AActor> It(GetWorld()); It; ++It) {
   auto* Verletzbar = Cast<ILaLaBergVerletzbar>(*It);
   if (!Verletzbar || *It == GetOwner()) continue;
   const FVector Weg = It->GetActorLocation() - Treffer.Location;
   const float Abstand = Weg.Size();
   if (Abstand > WuchtRadius) continue;
   Verletzbar->Verletze(Wucht * (1.0f - Abstand / WuchtRadius),
                        Abstand > 1.0f ? Weg / Abstand : FVector::UpVector, ELaLaBergSchaden::Sprengung);
   Getroffen++;
  }
  UE_LOG(LogTemp, Display, TEXT("LALABERG_WUCHT radius=%.0fm getroffen=%d"), WuchtRadius / 100.0f, Getroffen);
 }
 if (auto* Modus = Cast<ALaLaBergGameMode>(UGameplayStatics::GetGameMode(this))) Modus->ZaehleFarbtreffer();
 UE_LOG(LogTemp, Display, TEXT("LALABERG_FARBKLECKS bei %s auf %s"), *Treffer.Location.ToString(),
        AndererActor ? *AndererActor->GetName() : TEXT("?"));
 Destroy();
}
