#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergEinschlagblitz.generated.h"

// Ein kurzer, sich selbst abbauender Lichtblitz - fuer Muendungsfeuer und den
// Einschlag einer Farbkugel. Kein Partikelsystem im Projekt (siehe README,
// "kein importiertes Skelett-Mesh" gilt sinngemaess auch fuer Niagara/
// Cascade) - ein Licht, das linear abklingt und sich danach selbst zerstoert,
// genuegt fuer die Rueckmeldung "hier ist gerade etwas passiert".
UCLASS()
class LALABERG_API ALaLaBergEinschlagblitz : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergEinschlagblitz();
 void Einrichten(const FLinearColor& Farbe, float StaerkeLux, float Radius, float DauerSek);

protected:
 virtual void Tick(float Zeit) override;

private:
 UPROPERTY() TObjectPtr<class UPointLightComponent> Licht = nullptr;
 float Staerke = 4000.0f;
 float Dauer = 0.12f;
 float Alter = 0.0f;
};
