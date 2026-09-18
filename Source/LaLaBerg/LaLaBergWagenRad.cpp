#include "LaLaBergWagenRad.h"

// Nm: Chaos konvertiert diese Werte beim Anwenden des Bremsmoments.
// Die fruehere pauschale 100x-Korrektur erzeugte einen abrupten Stopp.
constexpr float BREMSMOMENT_KORRIGIERT = 1500.0f;
constexpr float HANDBREMSMOMENT_KORRIGIERT = 3000.0f;

ULaLaBergRadVorn::ULaLaBergRadVorn() {
 WheelRadius = 33.0f;
 WheelWidth = 22.0f;
 AxleType = EAxleType::Front;
 bAffectedBySteering = true;
 bAffectedByEngine = false;
 bAffectedByHandbrake = false;
 MaxSteerAngle = 34.0f;
 MaxBrakeTorque = BREMSMOMENT_KORRIGIERT;
 // Die importierten Strassen-Meshes haben nur komplexe Kollision
 // (CollisionTraceFlag=CTF_UseComplexAsSimple, siehe LaLaBergImportCommandlet)
 // - ohne ComplexSweep trafen die Radstrahlen den Boden nie (federweg blieb
 // per -LaLaBergFahrtest FAIL immer bei 1.00, kontakt immer 0).
 SweepType = ESweepType::ComplexSweep;
}

ULaLaBergRadHinten::ULaLaBergRadHinten() {
 WheelRadius = 33.0f;
 WheelWidth = 22.0f;
 AxleType = EAxleType::Rear;
 bAffectedBySteering = false;
 bAffectedByEngine = true;
 bAffectedByHandbrake = true;
 MaxSteerAngle = 0.0f;
 MaxBrakeTorque = BREMSMOMENT_KORRIGIERT;
 MaxHandBrakeTorque = HANDBREMSMOMENT_KORRIGIERT;
 SweepType = ESweepType::ComplexSweep;
}
