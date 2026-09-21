#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergAuftraege.generated.h"

// Lieferauftraege - die erste Aufgabe im Spiel. An der blauen Saeule nimmt man
// eine Fahrt an, faehrt unter Zeitdruck zu einem echten Ort in Landsberg (die
// Wahrzeichen aus orte.json, auf die naechste Strasse gesetzt), bekommt Geld
// und findet die naechste blaue Saeule an einem Ort in der Naehe. Laeuft die
// Zeit ab, bleibt die blaue Saeule, wo sie war, und man kann es noch einmal
// versuchen.
UCLASS()
class LALABERG_API ALaLaBergAuftraege : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergAuftraege();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float DeltaSeconds) override;

 // Fuer das HUD: es gibt hoechstens eine Auftragsverwaltung.
 static TWeakObjectPtr<ALaLaBergAuftraege> Instanz;

 // Erste blaue Saeule - die Spielart setzt sie neben den Startpunkt.
 void SetzeStartOrt(const FVector& Ort);

 bool IstUnterwegs() const { return bUnterwegs; }
 bool HatAngebot() const { return bAngebot; }
 // Wohin der Pfeil zeigt: im Auftrag das Ziel, sonst die blaue Saeule.
 FVector HoleWegpunkt() const { return bUnterwegs ? Ziele[AktZiel].Ort : StartOrt; }
 FString HoleZielName() const { return bUnterwegs ? Ziele[AktZiel].N : FString(); }
 float HoleRestzeit() const;
 int32 HoleGeld() const { return Geld; }
 int32 HoleLohn() const { return Lohn; }
 int32 HoleErledigt() const { return Erledigt; }
 int32 HoleGescheitert() const { return Gescheitert; }
 int32 HoleZielzahl() const { return Ziele.Num(); }

 // Nach einer Festnahme (siehe ALaLaBergPolizei): laufender Auftrag faellt
 // weg, die blaue Saeule bleibt. Strafe zieht hoechstens ab, was da ist, und
 // liefert den tatsaechlich abgezogenen Betrag.
 void Abbrechen();
 int32 Strafe(int32 Betrag) { const int32 Ab = FMath::Clamp(Betrag, 0, Geld); Geld -= Ab; return Ab; }

 // Fuer -LaLaBergAuftragTest: die Frist sofort ablaufen lassen.
 void TestAblaufen() { Frist = GetWorld()->GetTimeSeconds() - 0.01; }

private:
 struct FZiel { FString N; FVector Ort; };
 TArray<FZiel> Ziele;
 void LadeZiele();
 void NimmAn();
 void Erledige();
 void Scheitere();
 void Melde(const FString& Text) const;
 void Zeige(bool bStart, bool bSichtbar, const FVector& Ort);

 FVector StartOrt = FVector::ZeroVector;
 int32 StartZiel = INDEX_NONE; // welches Ziel die blaue Saeule ist, falls eins
 int32 AktZiel = INDEX_NONE;
 bool bAngebot = false, bUnterwegs = false, bErstHinaus = false;
 double Frist = 0.0;
 int32 Lohn = 0, Geld = 0, Erledigt = 0, Gescheitert = 0;

 // Je Marke ein flacher Ring am Boden und eine hohe Saeule, die man ueber
 // die Daecher sieht.
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StartRing = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StartSaeule = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> ZielRing = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> ZielSaeule = nullptr;
 class UStaticMeshComponent* BaueTeil(const TCHAR* Name, const FLinearColor& Farbe);
};
