#include "LaLaBergWagenBewegung.h"

bool ULaLaBergWagenBewegung::CanCreateVehicle() const {
 // Wie UChaosWheeledVehicleMovementComponent::CanCreateVehicle(), nur ohne
 // die Bone-Namen-Pflicht - siehe Header fuer den Grund.
 if (!UChaosVehicleMovementComponent::CanCreateVehicle()) return false;
 for (const FChaosWheelSetup& Setup : WheelSetups)
  if (Setup.WheelClass == nullptr) return false;
 return true;
}
