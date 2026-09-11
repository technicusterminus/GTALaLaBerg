#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergAmpel.generated.h"

// Eine Lichtsignalanlage an einem echten amtlichen Standort (siehe
// Tools/Export/prepare-verkehr.cjs, aus OSM highway=traffic_signals).
// Wechselt selbststaendig Rot/Gelb/Gruen; KI-Autos fragen HaeltAn() ab und
// bremsen vor einer roten Ampel. Kein Kreuzungsgraph, keine Grundlage fuer
// eine Ampelpaarung an derselben Kreuzung - jede Ampel schaltet fuer sich,
// mit eigenem Zeitversatz, damit nicht die ganze Stadt im Gleichtakt blinkt.
UCLASS()
class LALABERG_API ALaLaBergAmpel : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergAmpel();
 // Fuer KI-Autos: steht die Ampel gerade auf Rot oder Gelb (also: anhalten)?
 bool HaeltAn() const { return Zustand != 1; }
 // Kurze eigene Liste statt TActorIterator - siehe LaLaBergVerkehrsauto.h.
 static TArray<ALaLaBergAmpel*> Alle;

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float Zeit) override;

private:
 void BaueKopf();

 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 // 0 = Rot, 1 = Gruen, 2 = Gelb
 int32 Zustand = 0;
 float Bis = 0.0f;
 float Versatz = 0.0f;
};
