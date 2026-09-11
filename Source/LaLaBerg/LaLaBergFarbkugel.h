#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbkugel.generated.h"

// Eine Paintball-Kugel: fliegt mit Schwerkraft, hinterlaesst beim Aufprall
// einen Farbklecks (Decal) auf jeder getroffenen Flaeche. Trifft sie eine
// KI-Figur (Passant, Verkehrswagen) oder den fahrbaren Wagen, faerbt und
// stoert sie sie kurz - alles andere in der Stadt ist gebaute Geometrie ohne
// eigene Reaktion, ein Klecks bleibt dort die einzige Spur.
UCLASS()
class LALABERG_API ALaLaBergFarbkugel : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergFarbkugel();
 // Vor FinishSpawning() setzen: Farbe, Groesse der Kugel und ihres Kleckses,
 // Schwerkraftfaktor (ein Raketenwerfer fliegt flacher als eine Pistole).
 void Einrichten(const FLinearColor& Farbe, float KugelRadius, float KleckMin, float KleckMax, float Schwerkraft);
 void Abschiessen(const FVector& Richtung, float Tempo);
 // Fuer die Schrotflinte: die Kugeln einer Salve entstehen im selben Bild
 // dicht nebeneinander und trafen sich sonst gegenseitig statt des Ziels.
 void IgnoriereGeschwister(AActor* Anderer);

protected:
 virtual void BeginPlay() override;

private:
 UFUNCTION()
 void Aufprall(UPrimitiveComponent* TroffeneKomponente, AActor* AndererActor, UPrimitiveComponent* AndereKomponente,
               FVector NormalImpuls, const FHitResult& Treffer);

 UPROPERTY() TObjectPtr<class USphereComponent> Huelle = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 UPROPERTY() TObjectPtr<class UProjectileMovementComponent> Flugbahn = nullptr;
 FLinearColor Farbe = FLinearColor(0.8f, 0.1f, 0.5f, 1.0f);
 float Radius = 1.4f;
 float KleckMin = 9.0f, KleckMax = 15.0f;
};
