#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergRevier.generated.h"

// Die vier Reviere des Farbkriegs (siehe Docs/Geschichte.md). Jedes gehoert
// einer Mannschaft und haengt an vier Wahrzeichen aus orte.json. Wer alle
// vier mit Farbe trifft und die Bewaehrung des Kapitels besteht, nimmt das
// Revier.
//
// Ein Wahrzeichen ist im Spiel kein eigenes Gebaeude, sondern ein Ort in den
// Daten - markiert wird deshalb eine Saeule, die dort steht: sichtbar von
// weitem, zu treffen aus jeder Richtung.
UCLASS()
class LALABERG_API ALaLaBergRevier : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergRevier();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float Zeit) override;

 static TWeakObjectPtr<ALaLaBergRevier> Instanz;

 // Vier Reviere, in der Reihenfolge der Kapitel.
 static constexpr int32 ANZAHL = 4;
 struct FMarke {
  FString Name;
  FVector Ort = FVector::ZeroVector;
  bool bMarkiert = false;
  class UStaticMeshComponent* Saeule = nullptr;
 };
 struct FRevier {
  FString Name;          // "Vorstadt-Nord"
  FString Mannschaft;    // "Die Gelben"
  FLinearColor Farbe;
  TArray<FMarke> Marken;
 };

 const TArray<FRevier>& HoleReviere() const { return Reviere; }
 // Welches Revier gerade dran ist (Kapitel 1 = Klinikum ... ), sonst
 // INDEX_NONE.
 int32 HoleOffenes() const;
 // Wie viele Wahrzeichen des offenen Reviers schon markiert sind.
 int32 HoleMarkiert(int32 Revier) const;
 // Von der Farbkugel gerufen: ein Treffer auf diese Saeule.
 void Markiere(class UStaticMeshComponent* Saeule, const FLinearColor& Farbe);
 // Fuer -LaLaBergRevierTest: alle Wahrzeichen des offenen Reviers treffen.
 void TestMarkiereAlle();

private:
 void LadeMarken();
 void Melde(const FString& Text) const;
 // Alle vier markiert und die Bewaehrung geschafft: Revier uebernehmen.
 void PruefeUebernahme(int32 Revier);

 TArray<FRevier> Reviere;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Teile;
 bool bGeladen = false;
};
