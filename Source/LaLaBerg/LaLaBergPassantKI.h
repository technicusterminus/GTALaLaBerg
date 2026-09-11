#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergWegfolger.h"
#include "LaLaBergPassantKI.generated.h"

// Ein KI-Passant: geht seinen Gehweg ab und zurueck (siehe Tools/Export/
// prepare-verkehr.cjs). Eine blockige Figur aus Kaesten, im selben Baustil
// wie Wagen und Waffen dieses Projekts - kein Skelett, aber die Beine
// schwingen im Schritt und der Koerper wippt dazu; das genuegt aus der
// Entfernung, in der man einer Stadtfigur begegnet.
UCLASS()
class LALABERG_API ALaLaBergPassantKI : public AActor, public ILaLaBergFarbbar {
 GENERATED_BODY()
public:
 ALaLaBergPassantKI();
 virtual void Tick(float Zeit) override;
 void SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh);
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) override;
 float HoleTempo() const { return Tempo; }

protected:
 virtual void BeginPlay() override;

private:
 // Kopf, Rumpf, Arme: einmal gebaut, Abschnitt 0. Aendert sich nur bei
 // einem Treffer (Jackenfarbe).
 void BaueOberkoerper();
 // Beine: Abschnitt 1, jedes Bild neu positioniert (Schrittbewegung) -
 // UpdateMeshSection statt CreateMeshSection, damit das guenstig bleibt.
 void AktualisiereBeine(float Phase);

 UPROPERTY() TObjectPtr<class UCapsuleComponent> Huelle = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 FLaLaBergWegfolger Weg;
 float Tempo = 140.0f;             // cm/s, gewoehnliches Gehtempo
 float StolpertBis = -10.0f;       // ein Treffer bremst kurz und faerbt die Jacke
 float Gehphase = 0.0f;
 FLinearColor Jacke;
 FLinearColor Hose;
 float Groesse = 1.72f;
 float BeinL = 0.0f;                // Beinlaenge in cm, aus Groesse abgeleitet
};
