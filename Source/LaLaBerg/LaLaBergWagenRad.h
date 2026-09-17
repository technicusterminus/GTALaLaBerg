#pragma once
#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "LaLaBergWagenRad.generated.h"

// Lenkbares, nicht antreibendes Vorderrad fuer ALaLaBergWagen. Lenkung und
// Antrieb haengen an der UChaosVehicleWheel-Klasse selbst, nicht an der
// einzelnen Radposition in FChaosWheelSetup - deshalb zwei Klassen statt
// einer mit Parametern.
UCLASS()
class LALABERG_API ULaLaBergRadVorn : public UChaosVehicleWheel {
 GENERATED_BODY()
public:
 ULaLaBergRadVorn();
};

// Antreibendes, nicht lenkendes Hinterrad mit Handbremse - siehe ULaLaBergRadVorn.
UCLASS()
class LALABERG_API ULaLaBergRadHinten : public UChaosVehicleWheel {
 GENERATED_BODY()
public:
 ULaLaBergRadHinten();
};
