#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergWegfolger.h"
#include "LaLaBergVerkehrsauto.generated.h"

// Ein KI-Auto: faehrt seine Strasse ab und zurueck (siehe Tools/Export/
// prepare-verkehr.cjs), ohne Federung oder Motorphysik - anders als der
// fahrbare Wagen (ALaLaBergWagen) bewegt es sich kinematisch entlang seiner
// Wegpunkte. Dieselbe Karosserie aus wagen.json wie jeder geparkte und der
// fahrbare Wagen (LaLaBergWagenForm).
UCLASS()
class LALABERG_API ALaLaBergVerkehrsauto : public AActor, public ILaLaBergFarbbar {
 GENERATED_BODY()
public:
 ALaLaBergVerkehrsauto();
 virtual void Tick(float Zeit) override;
 // Vor BeginPlay setzen: die Wegpunkte in Unreal-Zentimetern.
 void SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh);
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) override;

protected:
 virtual void BeginPlay() override;

private:
 UPROPERTY() TObjectPtr<class UBoxComponent> Rumpf = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 FLaLaBergWegfolger Weg;
 float Tempo = 900.0f;          // cm/s
 float StoerungBis = -10.0f;    // ein Treffer bremst kurz ab
 FLinearColor Lack = FLinearColor(0.6f, 0.6f, 0.6f);
};
