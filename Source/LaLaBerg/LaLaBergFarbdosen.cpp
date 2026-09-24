#include "LaLaBergFarbdosen.h"
#include "LaLaBergHUD.h"
#include "LaLaBergKonto.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TWeakObjectPtr<ALaLaBergFarbdosen> ALaLaBergFarbdosen::Instanz;

namespace {
 // So nah muss man heran, um eine Dose mitzunehmen - zu Fuss wie im Wagen.
 constexpr float GRIFF = 260.0f;
 // Die Dose steht 12 m neben dem Strassenknoten: neben der Fahrbahn, nicht
 // darauf.
 constexpr float VERSATZ = 1200.0f;
 const FLinearColor DOSENFARBE(0.98f, 0.35f, 0.75f);
}

ALaLaBergFarbdosen::ALaLaBergFarbdosen() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.1f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

void ALaLaBergFarbdosen::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 LadeOrte();
}

void ALaLaBergFarbdosen::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

int32 ALaLaBergFarbdosen::HoleGefunden() const {
 const auto* Konto = ULaLaBergKonto::Hole(this);
 return Konto ? Konto->HoleDosen() : 0;
}

void ALaLaBergFarbdosen::LadeOrte() {
 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Verkehr/netz.json");
 if (!FFileHelper::LoadFileToString(Text, *Datei) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_DOSEN kein Strassennetz: %s"), *Datei);
  return;
 }
 const auto& P = Wurzel->GetArrayField(TEXT("p"));
 const int32 Knoten = P.Num() / 3;
 if (Knoten < ANZAHL) return;
 auto* Zylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 auto* Stoff = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Saeule.M_Saeule"));
 if (!Stoff) Stoff = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 const auto* Konto = ULaLaBergKonto::Hole(this);
 // Gleichmaessig ueber den Graphen verteilt, aber nicht in einer Reihe: der
 // Schritt ist teilerfremd zur Knotenzahl, der Versatz haengt an der Nummer.
 const int32 Schritt = FMath::Max(1, Knoten / ANZAHL);
 for (int32 i = 0; i < ANZAHL; i++) {
  const int32 K = (i * Schritt + i * 7) % Knoten;
  FVector Ort(P[K * 3]->AsNumber(), P[K * 3 + 1]->AsNumber(), P[K * 3 + 2]->AsNumber());
  const float Winkel = i * 2.399963f;                 // goldener Winkel: keine Reihen
  Ort += FVector(FMath::Cos(Winkel), FMath::Sin(Winkel), 0.0f) * VERSATZ;
  FHitResult Boden;
  const FVector Oben(Ort.X, Ort.Y, Ort.Z + 4000.0f);
  if (GetWorld()->LineTraceSingleByChannel(Boden, Oben, Oben - FVector(0, 0, 20000.0f), ECC_Visibility))
   Ort.Z = Boden.ImpactPoint.Z;
  FDose D;
  D.Ort = Ort + FVector(0, 0, 70.0f);
  if (Zylinder) {
   auto* Netz = NewObject<UStaticMeshComponent>(this);
   Netz->SetMobility(EComponentMobility::Movable);
   Netz->SetupAttachment(RootComponent);
   Netz->SetUsingAbsoluteLocation(true);
   Netz->SetUsingAbsoluteRotation(true);
   Netz->SetUsingAbsoluteScale(true);
   Netz->SetStaticMesh(Zylinder);
   Netz->SetWorldLocation(D.Ort);
   Netz->SetWorldScale3D(FVector(0.45f, 0.45f, 0.6f));
   Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Netz->SetCastShadow(false);
   Netz->SetMaterial(0, Stoff);
   Netz->RegisterComponent();
   if (auto* MID = Netz->CreateDynamicMaterialInstance(0)) MID->SetVectorParameterValue(TEXT("Color"), DOSENFARBE);
   // Schon gefundene Dosen bleiben verschwunden.
   if (Konto && Konto->HatDose(i)) Netz->SetVisibility(false);
   D.Netz = Netz;
  }
  Dosen.Add(D);
 }
 bGeladen = true;
 UE_LOG(LogTemp, Display, TEXT("LALABERG_DOSEN gestellt=%d gefunden=%d"), Dosen.Num(), HoleGefunden());
}

void ALaLaBergFarbdosen::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

// Gestaffelte Belohnung, wie man sie aus dem Vorbild kennt: jede Dose zahlt,
// und bei 10, 25 und 50 gibt es etwas, das man sonst kaufen muesste.
void ALaLaBergFarbdosen::Einsammeln(int32 Nummer) {
 auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto || !Dosen.IsValidIndex(Nummer) || Konto->HatDose(Nummer)) return;
 Konto->NimmDose(Nummer);
 if (Dosen[Nummer].Netz) Dosen[Nummer].Netz->SetVisibility(false);
 const int32 Stand = Konto->HoleDosen();
 Konto->Gutschrift(120);
 FString Zusatz;
 if (Stand == 10) { Konto->Gutschrift(2000); Zusatz = TEXT(" – 2000 € obendrauf"); }
 else if (Stand == 25) { Konto->GibWaffe(2); Zusatz = TEXT(" – die Schrotflinte gehört dir"); }
 else if (Stand == ANZAHL) { Konto->Gutschrift(15000); Zusatz = TEXT(" – alle gefunden! 15000 €"); }
 Melde(FString::Printf(TEXT("Farbdose %d von %d%s"), Stand, ANZAHL, *Zusatz));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_DOSE %d stand=%d"), Nummer, Stand);
}

bool ALaLaBergFarbdosen::TestSammle() {
 const auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto) return false;
 for (int32 i = 0; i < Dosen.Num(); i++)
  if (!Konto->HatDose(i)) { Einsammeln(i); return true; }
 return false;
}

void ALaLaBergFarbdosen::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!bGeladen) return;
 auto* PC = GetWorld()->GetFirstPlayerController();
 APawn* Figur = PC ? PC->GetPawn() : nullptr;
 if (!Figur) return;
 const auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto) return;
 // Langsam drehende Dosen: fuenfzig Komponenten, eine Drehung - das kostet
 // nichts und macht sie von weitem erkennbar.
 Drehung = FMath::Fmod(Drehung + 90.0f * Zeit, 360.0f);
 const FVector Wo = Figur->GetActorLocation();
 for (int32 i = 0; i < Dosen.Num(); i++) {
  if (Konto->HatDose(i)) continue;
  if (Dosen[i].Netz) Dosen[i].Netz->SetWorldRotation(FRotator(0, Drehung, 22.0f));
  if (FVector::Dist(Wo, Dosen[i].Ort) < GRIFF) Einsammeln(i);
 }
}
