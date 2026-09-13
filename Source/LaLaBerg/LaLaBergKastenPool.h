#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergKastenPool.generated.h"

// Ein eigenes UProceduralMeshComponent je geparktem Auto (1078 Stueck) kostete
// ebenso viele Draw-Calls wie zuvor das CarConcept-Detailmodell bei den 70
// KI-Autos (siehe ALaLaBergAutoPool, "3000 Draw-Calls, einstellige fps") -
// derselbe Trick hier fuer den leichten Kasten-Fallback: ein UStaticMesh je
// vorkommender Lackfarbe (LaLaBergWagenForm::BaueKastenMesh, lazy gebaut),
// eine UHierarchicalInstancedStaticMeshComponent je Farbe - alle geparkten
// Autos derselben Farbe teilen sich denselben Draw-Call.
//
// Nur fuer geparkte (routenlose) Autos gedacht - die 70 fahrenden KI-Autos
// und der fahrbare Wagen behalten ihr eigenes ProceduralMeshComponent
// (LaLaBergWagenForm::BaueNetz), weil sie sich bewegen und nur wenige sind.
UCLASS()
class LALABERG_API ALaLaBergKastenPool : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergKastenPool();
 // Das einzige Exemplar im Level - vom GameMode vor den geparkten Autos
 // gebaut, von jedem geparkten Auto in SetzeLack gelesen.
 static TObjectPtr<ALaLaBergKastenPool> Instanz;

 // Griff auf eine reservierte Instanz - siehe Verstecke fuer den Grund, warum
 // kein echtes RemoveInstance (wie bei ALaLaBergAutoPool::FGriff).
 struct FGriff {
  int32 Farbindex = INDEX_NONE;
  int32 InstanzIndex = INDEX_NONE;
  bool Gueltig() const { return Farbindex != INDEX_NONE && InstanzIndex != INDEX_NONE; }
 };
 // Baut bei Bedarf eine neue Farbvariante (UStaticMesh + HISM) und legt eine
 // Instanz an "Lage" hinein. Liefert einen ungueltigen Griff, wenn wagen.json
 // fehlt oder kein Editor-Build vorliegt - der Aufrufer weicht dann auf
 // LaLaBergWagenForm::BaueNetz aus.
 FGriff FuegeHinzu(const FLinearColor& Lack, const FTransform& Lage);
 void Aktualisiere(const FGriff& Griff, const FTransform& Lage);
 // Weit weg versetzt statt entfernt - ein RemoveInstance vertauscht bei HISM
 // Indizes mit der letzten Instanz und wuerde ein fremdes, noch sichtbares
 // Auto verspringen lassen (siehe ALaLaBergAutoPool::Verstecke).
 void Verstecke(const FGriff& Griff);

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;

private:
 UPROPERTY() TObjectPtr<class USceneComponent> Wurzel = nullptr;
 UPROPERTY() TArray<TObjectPtr<class UHierarchicalInstancedStaticMeshComponent>> Farbtoepfe;
 TArray<FLinearColor> Farben;
};
