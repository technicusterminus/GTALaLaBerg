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

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;

private:
 UPROPERTY() TObjectPtr<class USceneComponent> Wurzel = nullptr;
 UPROPERTY() TArray<TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> Pools;
};
