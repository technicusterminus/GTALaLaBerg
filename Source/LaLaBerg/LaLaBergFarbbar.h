#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LaLaBergFarbbar.generated.h"

UINTERFACE(MinimalAPI)
class ULaLaBergFarbbar : public UInterface { GENERATED_BODY() };

// Wer diese Schnittstelle traegt, reagiert auf einen Paintball-Treffer:
// der fahrbare Wagen faerbt sich um, ein Verkehrswagen ebenso, ein Passant
// faerbt seine Kleidung und stolpert kurz. Alles andere in der Stadt ist
// gebaute Geometrie ohne eigene Reaktion - dort bleibt nur der Farbklecks.
class LALABERG_API ILaLaBergFarbbar {
 GENERATED_BODY()
public:
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) = 0;
};
