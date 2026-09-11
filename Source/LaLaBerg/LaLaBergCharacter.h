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
};
