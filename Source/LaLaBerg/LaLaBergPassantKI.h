#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergWegfolger.h"
#include "LaLaBergPassantKI.generated.h"

// Ein KI-Passant: geht seinen Gehweg ab und zurueck (siehe Tools/Export/
// prepare-verkehr.cjs). Eine blockige Figur aus Kaesten, im selben Baustil
// wie Wagen und Waffen dieses Projekts - keine Animation, nur Bewegung
// entlang der Route und eine Kopfdrehung in die Gehrichtung.
UCLASS()
class LALABERG_API ALaLaBergPassantKI : public AActor, public ILaLaBergFarbbar {
 GENERATED_BODY()
public:
 ALaLaBergPassantKI();
 virtual void Tick(float Zeit) override;
 void SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh);
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) override;

protected:
 virtual void BeginPlay() override;

private:
 void BaueModell();

 UPROPERTY() TObjectPtr<class UCapsuleComponent> Huelle = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 FLaLaBergWegfolger Weg;
 float Tempo = 140.0f;             // cm/s, gewoehnliches Gehtempo
 float StolpertBis = -10.0f;       // ein Treffer bremst kurz und faerbt die Jacke
 FLinearColor Jacke;
 float Groesse = 1.72f;
};
