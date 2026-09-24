#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbdosen.generated.h"

// Fuenfzig versteckte Farbdosen in der ganzen Stadt - das Sammelspiel neben
// der Geschichte. Die Orte stehen nicht in einer Datei, sondern entstehen
// aus dem Strassengraphen (netz.json): jeder n-te Knoten, seitlich versetzt,
// damit die Dose neben der Fahrbahn liegt und nicht darauf. Gleiche Daten,
// gleiche Orte - wer sie einmal gefunden hat, findet sie wieder.
//
// Belohnt wird gestaffelt (siehe Einsammeln): Geld je Dose, dazu etwas
// Handfestes bei 10, 25 und 50.
UCLASS()
class LALABERG_API ALaLaBergFarbdosen : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergFarbdosen();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float Zeit) override;

 static TWeakObjectPtr<ALaLaBergFarbdosen> Instanz;
 static constexpr int32 ANZAHL = 50;

 int32 HoleGefunden() const;
 // Fuer -LaLaBergDosenTest: die naechste Dose einsammeln.
 bool TestSammle();

private:
 void LadeOrte();
 void Einsammeln(int32 Nummer);
 void Melde(const FString& Text) const;

 struct FDose { FVector Ort = FVector::ZeroVector; class UStaticMeshComponent* Netz = nullptr; };
 TArray<FDose> Dosen;
 float Drehung = 0.0f;
 bool bGeladen = false;
};
