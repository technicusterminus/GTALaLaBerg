#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergSchiessbude.generated.h"

// Die Schiessbude am Paintball-Laden: das zweite Minispiel. Wer den Ring
// betritt, hat 45 Sekunden Zeit; drei Scheiben stehen gleichzeitig, jede
// getroffene verschwindet und taucht woanders wieder auf. Bezahlt wird nach
// Treffern, die Bestleistung steht im Spielstand.
//
// Getroffen wird ueber dieselbe Stelle wie die Reviersaeulen: die Farbkugel
// meldet die getroffene Komponente (siehe LaLaBergFarbkugel::Aufprall).
UCLASS()
class LALABERG_API ALaLaBergSchiessbude : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergSchiessbude();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float Zeit) override;

 static TWeakObjectPtr<ALaLaBergSchiessbude> Instanz;

 // Von der Spielart gerufen, sobald der Laden steht.
 void Stelle(const FVector& Ort);
 // Von der Farbkugel: Treffer auf diese Komponente.
 void Treffer(class UStaticMeshComponent* Scheibe);

 bool Laeuft() const { return bLaeuft; }
 int32 HoleTreffer() const { return Treffer_; }
 float HoleRestzeit() const;
 // Fuer -LaLaBergBudeTest.
 void TestStarte() { Starte(); }
 bool TestTrefferAufErste();
 // Die Runde sofort abrechnen, ohne die 45 Sekunden abzuwarten.
 void TestBeende() { if (bLaeuft) Beende(); }

private:
 void Starte();
 void Beende();
 void SetzeScheibe(int32 Nummer);
 void Melde(const FString& Text) const;

 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Scheiben;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Ring = nullptr;
 FVector Ort = FVector::ZeroVector;
 bool bAufgestellt = false, bLaeuft = false, bErstHinaus = false;
 int32 Treffer_ = 0;
 double Frist = 0.0;
};
