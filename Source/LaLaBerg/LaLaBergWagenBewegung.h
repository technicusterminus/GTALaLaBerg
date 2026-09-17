#pragma once
#include "CoreMinimal.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "LaLaBergWagenBewegung.generated.h"

// UChaosWheeledVehicleMovementComponent verlangt zusaetzlich zur Basisklasse
// einen Bone-Namen je Rad (fuer ein Skelettnetz) - der fahrbare Wagen hat
// aber keins, nur eine unsichtbare Kollisionsbox (siehe ALaLaBergWagen::
// Rumpf) mit den Radpositionen als AdditionalOffset ab dem Fahrzeugursprung.
// Laut UChaosVehicleMovementComponent::LocateBoneOffset ist BoneName=NAME_None
// dafuer ausdruecklich vorgesehen - nur die Wheeled-Unterklasse prueft
// zusaetzlich (und fuer diesen Fall zu Unrecht) auf einen Bone-Namen. Diese
// Klasse ueberspringt nur genau diese eine Pruefung.
UCLASS()
class LALABERG_API ULaLaBergWagenBewegung : public UChaosWheeledVehicleMovementComponent {
 GENERATED_BODY()
public:
 virtual bool CanCreateVehicle() const override;
};
