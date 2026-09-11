#include "LaLaBergEinschlagblitz.h"
#include "Components/PointLightComponent.h"

ALaLaBergEinschlagblitz::ALaLaBergEinschlagblitz() {
 PrimaryActorTick.bCanEverTick = true;
 Licht = CreateDefaultSubobject<UPointLightComponent>(TEXT("Licht"));
 SetRootComponent(Licht);
 Licht->SetCastShadows(false);
 Licht->SetIntensityUnits(ELightUnits::Lumens);
 Licht->Intensity = 0.0f;
 Licht->AttenuationRadius = 260.0f;
 SetActorEnableCollision(false);
}

void ALaLaBergEinschlagblitz::Einrichten(const FLinearColor& Farbe, float StaerkeLux, float Radius, float DauerSek) {
 Staerke = StaerkeLux; Dauer = FMath::Max(0.02f, DauerSek);
 Licht->SetLightColor(Farbe);
 Licht->AttenuationRadius = Radius;
 Licht->Intensity = Staerke;
}

// Linear abklingend statt exponentiell: bei so kurzer Dauer (0.1-0.2s) faellt
// der Unterschied ohnehin nicht auf, linear ist die einfachere Rechnung.
void ALaLaBergEinschlagblitz::Tick(float Zeit) {
 Super::Tick(Zeit);
 Alter += Zeit;
 if (Alter >= Dauer) { Destroy(); return; }
 Licht->Intensity = Staerke * (1.0f - Alter / Dauer);
}
