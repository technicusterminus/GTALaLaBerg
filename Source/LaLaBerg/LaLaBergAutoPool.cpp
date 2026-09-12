#include "LaLaBergAutoPool.h"
#include "LaLaBergWagenForm.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"

TObjectPtr<ALaLaBergAutoPool> ALaLaBergAutoPool::Instanz = nullptr;

ALaLaBergAutoPool::ALaLaBergAutoPool() {
 Wurzel = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
 SetRootComponent(Wurzel);
}

void ALaLaBergAutoPool::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 // Index-basiert statt range-for: "extern const TCHAR* const[]" hat ausserhalb
 // seiner definierenden .cpp keine bekannte Groesse mehr.
 for (int32 i = 0; i < LaLaBergWagenForm::CARCONCEPT_TEILE_ANZAHL; i++) {
  const TCHAR* Name = LaLaBergWagenForm::CARCONCEPT_TEILE[i];
  UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *LaLaBergWagenForm::CarConceptPfad(Name));
  if (!Mesh) continue;
  auto* Pool = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
   MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *FString(Name)));
  Pool->SetStaticMesh(Mesh);
  Pool->SetupAttachment(Wurzel);
  Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Pool->SetMobility(EComponentMobility::Movable);
  Pool->NumCustomDataFloats = 0;
  Pool->RegisterComponent();
  Pools.Add(Pool);
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUTOPOOL pools=%d von %d Teilen"), Pools.Num(), LaLaBergWagenForm::CARCONCEPT_TEILE_ANZAHL);
}

void ALaLaBergAutoPool::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz == this) Instanz = nullptr;
 Super::EndPlay(Grund);
}

TArray<int32> ALaLaBergAutoPool::FuegeHinzu(const FTransform& Lage) {
 TArray<int32> Indizes;
 Indizes.Reserve(Pools.Num());
 for (auto& Pool : Pools) Indizes.Add(Pool ? Pool->AddInstance(Lage, true) : -1);
 return Indizes;
}

void ALaLaBergAutoPool::Aktualisiere(const TArray<int32>& Indizes, const FTransform& Lage) {
 for (int32 i = 0; i < Indizes.Num() && i < Pools.Num(); i++)
  if (Indizes[i] >= 0 && Pools[i]) Pools[i]->UpdateInstanceTransform(Indizes[i], Lage, true, false, true);
}

// Nullskaliert statt entfernt - siehe Header. Weit unter die Stadt versetzt,
// falls eine Null-Skalierung allein irgendwo noch einen Schatten wuerfe.
void ALaLaBergAutoPool::Verstecke(const TArray<int32>& Indizes) {
 const FTransform Weg(FQuat::Identity, FVector(0, 0, -500000.0f), FVector::ZeroVector);
 Aktualisiere(Indizes, Weg);
}
