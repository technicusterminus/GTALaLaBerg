#include "LaLaBergKastenPool.h"
#include "LaLaBergWagenForm.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

TObjectPtr<ALaLaBergKastenPool> ALaLaBergKastenPool::Instanz = nullptr;

ALaLaBergKastenPool::ALaLaBergKastenPool() {
 Wurzel = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
 SetRootComponent(Wurzel);
}

void ALaLaBergKastenPool::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
}

void ALaLaBergKastenPool::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz == this) Instanz = nullptr;
 Super::EndPlay(Grund);
}

ALaLaBergKastenPool::FGriff ALaLaBergKastenPool::FuegeHinzu(const FLinearColor& Lack, const FTransform& Lage) {
 // Exakter Farbvergleich statt Toleranz: alle Aufrufer schoepfen aus derselben
 // kleinen Palette (siehe Tools/Export/prepare-verkehr.cjs LACKE) - dieselbe
 // Farbe kommt bitgleich wieder herein, keine Rundungsfrage.
 int32 Farbindex = Farben.IndexOfByKey(Lack);
 if (Farbindex == INDEX_NONE) {
  UStaticMesh* Mesh = LaLaBergWagenForm::BaueKastenMesh(Lack);
  if (!Mesh) return FGriff();
  auto* Pool = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
   MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("Kasten")));
  Pool->SetStaticMesh(Mesh);
  Pool->SetupAttachment(Wurzel);
  Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Pool->SetMobility(EComponentMobility::Movable);
  Pool->NumCustomDataFloats = 0;
  Pool->RegisterComponent();
  Farbindex = Farbtoepfe.Add(Pool);
  Farben.Add(Lack);
  UE_LOG(LogTemp, Display, TEXT("LALABERG_KASTENPOOL farben=%d"), Farbtoepfe.Num());
 }
 UHierarchicalInstancedStaticMeshComponent* Pool = Farbtoepfe[Farbindex];
 if (!Pool) return FGriff();
 FGriff Griff;
 Griff.Farbindex = Farbindex;
 Griff.InstanzIndex = Pool->AddInstance(Lage, true);
 return Griff;
}

void ALaLaBergKastenPool::Aktualisiere(const FGriff& Griff, const FTransform& Lage) {
 if (!Griff.Gueltig() || !Farbtoepfe.IsValidIndex(Griff.Farbindex)) return;
 if (UHierarchicalInstancedStaticMeshComponent* Pool = Farbtoepfe[Griff.Farbindex])
  Pool->UpdateInstanceTransform(Griff.InstanzIndex, Lage, true, false, true);
}

void ALaLaBergKastenPool::Verstecke(const FGriff& Griff) {
 Aktualisiere(Griff, FTransform(FQuat::Identity, FVector(0, 0, -500000.0f), FVector::ZeroVector));
}
