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
 // Eigene, kurze Liste statt TActorIterator: bei 70 Autos, die einander
 // jedes Bild abfragen, durchsuchte TActorIterator sonst die ganze Stadt -
 // Tausende Akteure statt der paar Dutzend eigenen. Kostete spuerbar
 // Bildrate (26 auf 16 fps beim Hinzukommen der Ampeln).
 static TArray<ALaLaBergVerkehrsauto*> Alle;

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;

private:
 UPROPERTY() TObjectPtr<class UBoxComponent> Rumpf = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 FLaLaBergWegfolger Weg;
 float Tempo = 900.0f;          // cm/s
 float StoerungBis = -10.0f;    // ein Treffer bremst kurz ab
 // Seitlicher Versatz zum Ausweichen vor einem Hindernis (siehe Tick) -
 // weicht sanft aus und wieder zurueck, statt starr auf der Route zu bremsen.
 float Seitversatz = 0.0f;
 FLinearColor Lack = FLinearColor(0.6f, 0.6f, 0.6f);
};
