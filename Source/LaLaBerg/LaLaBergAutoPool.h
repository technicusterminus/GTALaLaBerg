#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergAutoPool.generated.h"

// Eine eigene CarConcept-StaticMeshComponent je Wagen und Teil (46 Teile,
// siehe LaLaBergWagenForm::BaueCarConceptTeile) kostete auf allen 70
// KI-Autos zusammen ueber 3000 Draw-Calls und druckte die Bildrate auf
// einstellige Werte (siehe Commit-Nachricht). Dieser Pool haelt stattdessen
// je Teil EINE UHierarchicalInstancedStaticMeshComponent fuer alle Autos -
// eine Instanz pro Wagen und Teil, aber nur ein Draw-Call je Teil
// unabhaengig von der Wagenzahl. Ein einziges Exemplar pro Level (siehe
// LaLaBergGameMode::LadeVerkehr), von den KI-Autos ueber Instanz gefunden.
UCLASS()
class LALABERG_API ALaLaBergAutoPool : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergAutoPool();
 // Das einzige Exemplar im Level - vom GameMode vor den KI-Autos gebaut,
 // von jedem KI-Auto in BeginPlay gelesen. Kein TActorIterator noetig:
 // es gibt ohnehin nur eines.
 static TObjectPtr<ALaLaBergAutoPool> Instanz;

 // false, wenn die CarConcept-Assets fehlen - dann weicht der Aufrufer auf
 // die prozedurale Form aus BaueNetz aus.
 bool Gueltig() const { return !Pools.IsEmpty(); }
 // Eine Instanz je Teil-Pool an "Lage" (Weltkoordinaten). Die Indizes
 // (gleiche Reihenfolge wie LaLaBergWagenForm::CARCONCEPT_TEILE) haelt der
 // Aufrufer selbst und uebergibt sie an Aktualisiere/Verstecke.
 TArray<int32> FuegeHinzu(const FTransform& Lage);
 void Aktualisiere(const TArray<int32>& Indizes, const FTransform& Lage);
 // Kein echtes Entfernen (siehe .cpp): ein KI-Auto, das der Spieler
 // uebernimmt, verschwindet nur aus dem Bild, seine Instanz bleibt
 // reserviert - RemoveInstance vertauscht sonst Indizes mit der letzten
 // Instanz jedes Pools und wuerde fremde, noch aktive Autos verspringen
 // lassen.
 void Verstecke(const TArray<int32>& Indizes);

 // Dieselbe Idee fuer die realistischen City-Sample-Fahrzeugtypen (siehe
 // LaLaBergWagenTypen) zusaetzlich zum CarConcept-Modell oben - TypIndex
 // 0..LaLaBergWagenTypen::TYPEN_ANZAHL-1. Ein eigener Teile-Pool-Satz je Typ,
 // damit sich z.B. alle Instanzen desselben Sedan-Typs einen Draw-Call teilen.
 bool TypGueltig(int32 TypIndex) const;
 TArray<int32> FuegeTypHinzu(int32 TypIndex, const FTransform& Lage);
 void AktualisiereTyp(int32 TypIndex, const TArray<int32>& Indizes, const FTransform& Lage);
 void VersteckeTyp(int32 TypIndex, const TArray<int32>& Indizes);

 // Fern-Mesh je Typ (LaLaBergWagenTypen::LadeLod - ein bereits fertiges,
 // einzelnes Mesh statt Einzelteilen): ein Draw-Call je Typ statt je Auto,
 // keine Farbvielfalt noetig (anders als ALaLaBergKastenPool fuer CarConcept,
 // wo die prozedurale Form erst gefaerbt werden muss). -1, wenn der Typ kein
 // Fern-Mesh mitbringt (z.B. vehicle07_Car) - der Aufrufer bleibt dann auf
 // dem Detail-Pool oben.
 int32 FuegeTypFernHinzu(int32 TypIndex, const FTransform& Lage);
 void AktualisiereTypFern(int32 TypIndex, int32 Index, const FTransform& Lage);
 void VersteckeTypFern(int32 TypIndex, int32 Index);

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;

private:
 UPROPERTY() TObjectPtr<class USceneComponent> Wurzel = nullptr;
 UPROPERTY() TArray<TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> Pools;
 // Alle Teile aller City-Sample-Typen hintereinander in einem flachen Array -
 // TypPoolsStart[TypIndex] ist der erste, TypPoolsStart[TypIndex+1]-1 der
 // letzte Index darin (kein nested TArray<TArray<...>> als UPROPERTY noetig).
 UPROPERTY() TArray<TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> TypPoolsFlach;
 TArray<int32> TypPoolsStart;
 // Ein HISM je Typ (Index = TypIndex), nullptr wo LadeLod nichts fand.
 UPROPERTY() TArray<TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> TypFernPools;
};
