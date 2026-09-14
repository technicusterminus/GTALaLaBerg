#include "LaLaBergAutoPool.h"
#include "LaLaBergWagenForm.h"
#include "LaLaBergWagenTypen.h"
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

 // Dieselbe Idee je City-Sample-Fahrzeugtyp: Detail-Teile (5 je Typ, siehe
 // LaLaBergWagenTypen::TEIL_ANZAHL) hintereinander in TypPoolsFlach, plus ein
 // Fern-Mesh-Pool je Typ.
 TypPoolsStart.SetNum(LaLaBergWagenTypen::TYPEN_ANZAHL + 1);
 TypFernPools.SetNum(LaLaBergWagenTypen::TYPEN_ANZAHL);
 int32 TypTeileGesamt = 0;
 for (int32 t = 0; t < LaLaBergWagenTypen::TYPEN_ANZAHL; t++) {
  TypPoolsStart[t] = TypPoolsFlach.Num();
  const LaLaBergWagenTypen::FTyp& Typ = LaLaBergWagenTypen::TYPEN[t];
  TArray<FString> Pfade;
  LaLaBergWagenTypen::TeilPfade(Typ, Pfade);
  for (int32 i = 0; i < Pfade.Num(); i++) {
   UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Pfade[i]);
   if (!Mesh) continue;
   auto* Pool = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
    MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(),
     *FString::Printf(TEXT("Typ%d_Teil%d"), t, i)));
   Pool->SetStaticMesh(Mesh);
   Pool->SetupAttachment(Wurzel);
   Pool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Pool->SetMobility(EComponentMobility::Movable);
   Pool->NumCustomDataFloats = 0;
   Pool->RegisterComponent();
   TypPoolsFlach.Add(Pool);
   TypTeileGesamt++;
  }
  if (UStaticMesh* Lod = LaLaBergWagenTypen::LadeLod(Typ)) {
   auto* FernPool = NewObject<UHierarchicalInstancedStaticMeshComponent>(this,
    MakeUniqueObjectName(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), *FString::Printf(TEXT("Typ%d_Fern"), t)));
   FernPool->SetStaticMesh(Lod);
   FernPool->SetupAttachment(Wurzel);
   FernPool->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   FernPool->SetMobility(EComponentMobility::Movable);
   FernPool->NumCustomDataFloats = 0;
   FernPool->RegisterComponent();
   TypFernPools[t] = FernPool;
  }
 }
 TypPoolsStart[LaLaBergWagenTypen::TYPEN_ANZAHL] = TypPoolsFlach.Num();
 UE_LOG(LogTemp, Display, TEXT("LALABERG_WAGENTYPEN typen=%d teile=%d fern=%d"),
  LaLaBergWagenTypen::TYPEN_ANZAHL, TypTeileGesamt, TypFernPools.Num());
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

bool ALaLaBergAutoPool::TypGueltig(int32 TypIndex) const {
 return TypPoolsStart.IsValidIndex(TypIndex) && TypPoolsStart[TypIndex] < TypPoolsStart[TypIndex + 1];
}

TArray<int32> ALaLaBergAutoPool::FuegeTypHinzu(int32 TypIndex, const FTransform& Lage) {
 TArray<int32> Indizes;
 if (!TypGueltig(TypIndex)) return Indizes;
 const int32 Start = TypPoolsStart[TypIndex], Ende = TypPoolsStart[TypIndex + 1];
 Indizes.Reserve(Ende - Start);
 for (int32 i = Start; i < Ende; i++) Indizes.Add(TypPoolsFlach[i] ? TypPoolsFlach[i]->AddInstance(Lage, true) : -1);
 return Indizes;
}

void ALaLaBergAutoPool::AktualisiereTyp(int32 TypIndex, const TArray<int32>& Indizes, const FTransform& Lage) {
 if (!TypPoolsStart.IsValidIndex(TypIndex)) return;
 const int32 Start = TypPoolsStart[TypIndex];
 for (int32 i = 0; i < Indizes.Num(); i++) {
  const int32 Ziel = Start + i;
  if (Indizes[i] >= 0 && TypPoolsFlach.IsValidIndex(Ziel) && TypPoolsFlach[Ziel])
   TypPoolsFlach[Ziel]->UpdateInstanceTransform(Indizes[i], Lage, true, false, true);
 }
}

void ALaLaBergAutoPool::VersteckeTyp(int32 TypIndex, const TArray<int32>& Indizes) {
 const FTransform Weg(FQuat::Identity, FVector(0, 0, -500000.0f), FVector::ZeroVector);
 AktualisiereTyp(TypIndex, Indizes, Weg);
}

int32 ALaLaBergAutoPool::FuegeTypFernHinzu(int32 TypIndex, const FTransform& Lage) {
 if (!TypFernPools.IsValidIndex(TypIndex) || !TypFernPools[TypIndex]) return -1;
 return TypFernPools[TypIndex]->AddInstance(Lage, true);
}

void ALaLaBergAutoPool::AktualisiereTypFern(int32 TypIndex, int32 Index, const FTransform& Lage) {
 if (Index < 0 || !TypFernPools.IsValidIndex(TypIndex) || !TypFernPools[TypIndex]) return;
 TypFernPools[TypIndex]->UpdateInstanceTransform(Index, Lage, true, false, true);
}

void ALaLaBergAutoPool::VersteckeTypFern(int32 TypIndex, int32 Index) {
 AktualisiereTypFern(TypIndex, Index, FTransform(FQuat::Identity, FVector(0, 0, -500000.0f), FVector::ZeroVector));
}
