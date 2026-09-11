#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergWaffe.generated.h"

// Ein abgerundetes Spektrum statt einer einzelnen Waffe: praezise bis
// grossflaechig, alle mit Farbkugeln statt Geschossen.
UENUM(BlueprintType)
enum class ELaLaBergWaffenArt : uint8 {
 Pistole      UMETA(DisplayName = "Paintball-Pistole"),
 Maschine     UMETA(DisplayName = "Paintball-MP"),
 Schrotflinte UMETA(DisplayName = "Paintball-Schrotflinte"),
 Raketenwerfer UMETA(DisplayName = "Paintball-Werfer"),
};

// Der Paintball-Marker: ein Ansichtsmodell, an der Kamera der Figur
// befestigt, dazu das Abfeuern der Farbkugeln. Keine Munition, die zur
// Neige geht - eine Feuerrate begrenzt, wie schnell hintereinander
// geschossen wird, so bleibt "Munition" ein Wort ohne Nachladetaste.
UCLASS()
class LALABERG_API ALaLaBergWaffe : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergWaffe();
 void SetzeArt(ELaLaBergWaffenArt Neu);
 ELaLaBergWaffenArt HoleArt() const { return Art; }
 FString ArtName() const;
 // Loest einen Schuss aus, wenn die Feuerrate es zulaesst. Liefert true bei
 // tatsaechlichem Schuss - fuer den Waffentest.
 bool Feuern(const FVector& Ort, const FVector& Richtung);
 int32 Schuesse() const { return SchussZahl; }

protected:
 virtual void BeginPlay() override;
 virtual void Tick(float Zeit) override;

private:
 void BaueModell();

 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 ELaLaBergWaffenArt Art = ELaLaBergWaffenArt::Pistole;
 float LetzterSchuss = -10.0f;
 int32 SchussZahl = 0;

 // Sichtbarer/hoerbarer Rueckstoss: der Lauf kickt auf Schuss kurz nach oben
 // und klingt wieder ab, dazu ein Muendungsblitz und ein Schusssound je
 // Waffenart (Tonhoehe/Lautstaerke aus derselben Kennzahl wie die Ballistik).
 float RueckstossGrad = 0.0f;
};
