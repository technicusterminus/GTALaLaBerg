#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "LaLaBergCharacter.generated.h"

UCLASS()
class LALABERG_API ALaLaBergCharacter : public ACharacter {
 GENERATED_BODY()
public:
 ALaLaBergCharacter();
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
private:
 // Die Stadt entsteht erst zur Laufzeit. Bis ihre Kollision in der
 // Physikszene steht, haelt sich die Figur an Ort und Stelle fest.
 void WarteAufBoden();
 FTimerHandle BodenUhr;
 void Forward(float Value);
 void Right(float Value);
 void Recover();
 void Quit();
public:
 // Einsteigen in den naechsten Wagen und wieder heraus. Der Wagen merkt
 // sich, wer gefahren ist, damit die Figur danach wieder uebernimmt.
 // Oeffentlich, damit der Fahrtest denselben Weg nimmt wie die Taste.
 void Einsteigen();
 // Fuer den Waffentest: Schuss ohne Tastatur aus Blickrichtung.
 void Feuern();
 class ALaLaBergWaffe* HoleWaffe() const { return Waffe; }

private:
 UPROPERTY() TObjectPtr<class UCameraComponent> Kamera = nullptr;
 UPROPERTY() TObjectPtr<class ALaLaBergWaffe> Waffe = nullptr;
 void Waffe1(); void Waffe2(); void Waffe3(); void Waffe4();
 // Gedrueckt gehalten, feuert die Waffe weiter - ihre eigene Feuerrate
 // begrenzt, wie schnell. So wird aus der MP eine Dauerfeuerwaffe, ohne
 // dass jede Waffenart ihre eigene Tastenbehandlung braeuchte.
 bool bFeuerKnopf = false;
 void FeuerStart() { bFeuerKnopf = true; Feuern(); }
 void FeuerStop() { bFeuerKnopf = false; }

 // Schrittsound: nicht an eine Animation gekoppelt (es gibt kein Skelett-Mesh
 // fuer die Spielfigur), sondern rein an die zurueckgelegte Strecke am Boden -
 // alle ~140 cm ein Tritt, wie ein durchschnittlicher Schritt.
 void PruefeSchritt(float Zeit);
 float SchrittWeg = 0.0f;
};
