#include "LaLaBergSchiessbude.h"
#include "LaLaBergHUD.h"
#include "LaLaBergKonto.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

TWeakObjectPtr<ALaLaBergSchiessbude> ALaLaBergSchiessbude::Instanz;

namespace {
 constexpr int32 SCHEIBEN = 3;            // gleichzeitig sichtbar
 constexpr float RUNDE = 45.0f;           // Sekunden je Runde
 constexpr float RING = 500.0f;           // so nah startet die Runde
 constexpr int32 LOHN_JE_TREFFER = 40;
 // Wo die Scheiben auftauchen koennen: ein Halbkreis vor dem Stand,
 // 12 bis 30 m weit, in Brusthoehe bis Kopfhoehe.
 FVector Platz(const FVector& Mitte, int32 Wurf) {
  const float Winkel = FMath::DegreesToRadians(200.0f + (Wurf * 47 % 140));
  const float Weite = 1200.0f + (Wurf * 311 % 1800);
  const float Hoehe = 150.0f + (Wurf * 97 % 160);
  return Mitte + FVector(FMath::Cos(Winkel) * Weite, FMath::Sin(Winkel) * Weite, Hoehe);
 }
}

ALaLaBergSchiessbude::ALaLaBergSchiessbude() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.1f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

void ALaLaBergSchiessbude::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
}

void ALaLaBergSchiessbude::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

void ALaLaBergSchiessbude::Stelle(const FVector& NeuerOrt) {
 Ort = NeuerOrt;
 auto* Zylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 auto* Grund = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 if (!Zylinder) return;
 auto Baue = [&](const FVector& Wo, const FVector& Groesse, const FLinearColor& Farbe, bool bSichtbar) {
  auto* Teil = NewObject<UStaticMeshComponent>(this);
  Teil->SetMobility(EComponentMobility::Movable);
  Teil->SetupAttachment(RootComponent);
  Teil->SetUsingAbsoluteLocation(true);
  Teil->SetUsingAbsoluteRotation(true);
  Teil->SetUsingAbsoluteScale(true);
  Teil->SetStaticMesh(Zylinder);
  Teil->SetWorldLocation(Wo);
  Teil->SetWorldScale3D(Groesse);
  // Die Scheiben muessen getroffen werden koennen.
  Teil->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  Teil->SetCollisionProfileName(TEXT("BlockAllDynamic"));
  Teil->SetCastShadow(false);
  Teil->SetVisibility(bSichtbar);
  Teil->RegisterComponent();
  if (Grund) if (auto* MID = Teil->CreateDynamicMaterialInstance(0, Grund))
   MID->SetVectorParameterValue(TEXT("Color"), Farbe);
  return Teil;
 };
 // Ring am Boden - hier faengt die Runde an.
 Ring = Baue(Ort + FVector(0, 0, 12), FVector(9.0f, 9.0f, 0.08f), FLinearColor(0.98f, 0.55f, 0.08f), true);
 Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 for (int32 i = 0; i < SCHEIBEN; i++) {
  auto* Scheibe = Baue(Platz(Ort, i), FVector(0.9f, 0.9f, 0.12f), FLinearColor(0.95f, 0.16f, 0.18f), false);
  Scheibe->SetWorldRotation(FRotator(90.0f, 0, 0));      // hochkant, wie eine Zielscheibe
  Scheiben.Add(Scheibe);
 }
 bAufgestellt = true;
 UE_LOG(LogTemp, Display, TEXT("LALABERG_BUDE steht bei %s"), *Ort.ToString());
}

void ALaLaBergSchiessbude::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

float ALaLaBergSchiessbude::HoleRestzeit() const {
 return bLaeuft ? FMath::Max(0.0f, static_cast<float>(Frist - GetWorld()->GetTimeSeconds())) : 0.0f;
}

void ALaLaBergSchiessbude::SetzeScheibe(int32 Nummer) {
 if (!Scheiben.IsValidIndex(Nummer)) return;
 // Aus der Trefferzahl und der Scheibennummer: dieselbe Runde, dieselbe
 // Folge - aber unvorhersehbar genug.
 Scheiben[Nummer]->SetWorldLocation(Platz(Ort, Treffer_ * 3 + Nummer + 1));
 Scheiben[Nummer]->SetVisibility(bLaeuft);
}

void ALaLaBergSchiessbude::Starte() {
 if (!bAufgestellt || bLaeuft) return;
 bLaeuft = true;
 Treffer_ = 0;
 Frist = GetWorld()->GetTimeSeconds() + RUNDE;
 for (int32 i = 0; i < Scheiben.Num(); i++) SetzeScheibe(i);
 Melde(TEXT("Schießbude – 45 Sekunden, triff, was auftaucht!"));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_BUDE start"));
}

void ALaLaBergSchiessbude::Treffer(UStaticMeshComponent* Scheibe) {
 if (!bLaeuft) return;
 const int32 Nummer = Scheiben.IndexOfByKey(Scheibe);
 if (Nummer == INDEX_NONE) return;
 Treffer_++;
 SetzeScheibe(Nummer);
}

bool ALaLaBergSchiessbude::TestTrefferAufErste() {
 if (!bLaeuft || Scheiben.IsEmpty()) return false;
 Treffer(Scheiben[0]);
 return true;
}

void ALaLaBergSchiessbude::Beende() {
 bLaeuft = false;
 for (auto& Scheibe : Scheiben) if (Scheibe) Scheibe->SetVisibility(false);
 const int32 Lohn = Treffer_ * LOHN_JE_TREFFER;
 FString Zusatz;
 if (auto* Konto = ULaLaBergKonto::Hole(this)) {
  Konto->Gutschrift(Lohn);
  // Auch das Zielen wird besser, wenn man uebt.
  Konto->Uebe(ULaLaBergKonto::EWert::Zielsicherheit, Treffer_ * 8.0f);
  if (Treffer_ > Konto->HoleBesteBude()) {
   Konto->SetzeBesteBude(Treffer_);
   Zusatz = TEXT(" – neue Bestleistung");
  }
 }
 Melde(FString::Printf(TEXT("%d Treffer, %d €%s"), Treffer_, Lohn, *Zusatz));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_BUDE ende treffer=%d lohn=%d"), Treffer_, Lohn);
 bErstHinaus = true;
}

void ALaLaBergSchiessbude::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!bAufgestellt) return;
 auto* PC = GetWorld()->GetFirstPlayerController();
 APawn* Figur = PC ? PC->GetPawn() : nullptr;
 if (!Figur) return;
 if (bLaeuft) {
  if (GetWorld()->GetTimeSeconds() > Frist) Beende();
  return;
 }
 // Im Ring und zu Fuss: eine neue Runde. Erst wieder hinaus, dann noch
 // einmal - sonst startet die naechste Runde sofort von selbst.
 const bool bDrin = FVector::Dist2D(Figur->GetActorLocation(), Ort) < RING
                 && FMath::Abs(Figur->GetActorLocation().Z - Ort.Z) < 500.0f
                 && Figur->IsA<ACharacter>();
 if (!bDrin) { bErstHinaus = false; return; }
 if (!bErstHinaus) Starte();
}
