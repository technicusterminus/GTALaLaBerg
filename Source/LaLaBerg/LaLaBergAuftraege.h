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
//
// Was an der blauen Saeule wartet. Die Lieferung war die erste Art; das
// Taxi verlangt einen Wagen und zahlt nach Strecke, mit Trinkgeld fuer eine
// zuegige Fahrt.
UENUM()
// Rennen: vier Kontrollpunkte der Reihe nach gegen die Uhr, mit Einsatz und
// Preisgeld. Verfolgung: ein Wagen faehrt davon und muss gestellt werden -
// mit Farbe beschiessen oder rammen, bis er steht.
enum class ELaLaBergAuftragsart : uint8 { Lieferung, Taxi, Rennen, Verfolgung };

// Verwaltet Angebot, laufenden Auftrag, Frist und Lohn - siehe oben.
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
 // Bei der Verfolgung ist das Ziel der fluechtende Wagen und bewegt sich.
 FVector HoleWegpunkt() const;
 FString HoleZielName() const;
 // Rennen: der wievielte von wie vielen Kontrollpunkten.
 int32 HolePunkt() const { return Punkt + 1; }
 int32 HolePunkte() const { return Strecke.Num(); }
 int32 HoleRennen() const { return Rennen; }
 int32 HoleVerfolgungen() const { return Verfolgungen; }
 float HoleRestzeit() const;
 // Fuer das HUD: welche Art gerade laeuft oder angeboten wird.
 ELaLaBergAuftragsart HoleArt() const { return Art; }
 int32 HoleTaxifahrten() const { return Taxifahrten; }
 // Das Geld liegt im Konto (ULaLaBergKonto) und ueberdauert das Spielende.
 int32 HoleGeld() const;
 int32 HoleLohn() const { return Lohn; }
 int32 HoleErledigt() const { return Erledigt; }
 int32 HoleGescheitert() const { return Gescheitert; }
 int32 HoleZielzahl() const { return Ziele.Num(); }

 // Nach einer Festnahme (siehe ALaLaBergPolizei): laufender Auftrag faellt
 // weg, die blaue Saeule bleibt. Strafe zieht hoechstens ab, was da ist, und
 // liefert den tatsaechlich abgezogenen Betrag.
 void Abbrechen();
 int32 Strafe(int32 Betrag);

 // Fuer -LaLaBergTaxiTest: die naechste Annahme ist ein Taxiauftrag, statt
 // die Art zu wuerfeln.
 void TestErzwingeTaxi() { bTaxiErzwungen = true; }
 // Fuer -LaLaBergRennTest / -LaLaBergJagdTest.
 void TestErzwinge(ELaLaBergAuftragsart NeueArt) { ErzwungeneArt = NeueArt; bArtErzwungen = true; }
 // Der fluechtende Wagen der laufenden Verfolgung, sonst nullptr.
 class ALaLaBergVerkehrsauto* TestHoleBeute() const { return Beute.Get(); }
 // Wohin der laufende Auftrag geht - der Test setzt den Wagen dorthin.
 FVector HoleZielOrt() const { return bUnterwegs ? Ziele[AktZiel].Ort : StartOrt; }
 // Fuer -LaLaBergAuftragTest: die Frist sofort ablaufen lassen.
 void TestAblaufen() { Frist = GetWorld()->GetTimeSeconds() - 0.01; }

private:
 struct FZiel { FString N; FVector Ort; };
 TArray<FZiel> Ziele;
 void LadeZiele();
 void NimmAn();
 void Erledige();
 void Scheitere();
 // Siehe NimmAn: Rennstrecke aufbauen bzw. einen Wagen zur Beute erklaeren.
 void BereiteRennen(float& Zeit);
 bool SucheBeute();
 void Melde(const FString& Text) const;
 void Zeige(bool bStart, bool bSichtbar, const FVector& Ort);

 FVector StartOrt = FVector::ZeroVector;
 int32 StartZiel = INDEX_NONE; // welches Ziel die blaue Saeule ist, falls eins
 int32 AktZiel = INDEX_NONE;
 bool bAngebot = false, bUnterwegs = false, bErstHinaus = false;
 double Frist = 0.0;
 int32 Lohn = 0, Erledigt = 0, Gescheitert = 0, Taxifahrten = 0;
 ELaLaBergAuftragsart Art = ELaLaBergAuftragsart::Lieferung;
 // Ein Taxi nimmt man nur mit dem Wagen an; ohne Wagen bleibt es beim
 // Hinweis, und die Meldung soll nicht in jedem Bild neu kommen.
 double LetzterHinweis = -10.0;
 bool bTaxiErzwungen = false;
 bool bArtErzwungen = false;
 ELaLaBergAuftragsart ErzwungeneArt = ELaLaBergAuftragsart::Lieferung;
 // Rennen: die Kontrollpunkte (Indizes in Ziele) und der naechste davon.
 TArray<int32> Strecke;
 int32 Punkt = 0, Einsatz = 0, Rennen = 0, Verfolgungen = 0;
 // Verfolgung: der fluechtende Wagen. TWeakObjectPtr, weil er unterwegs
 // eingesammelt werden kann (siehe ALaLaBergAutoPool).
 TWeakObjectPtr<class ALaLaBergVerkehrsauto> Beute;
 float GesamtZeit = 1.0f;          // fuer den Trinkgeldanteil

 // Je Marke ein flacher Ring am Boden und eine hohe Saeule, die man ueber
 // die Daecher sieht.
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StartRing = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StartSaeule = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> ZielRing = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> ZielSaeule = nullptr;
 // bDurchsichtig: die hohen Saeulen scheinen durch, die flachen Ringe am
 // Boden nicht.
 class UStaticMeshComponent* BaueTeil(const TCHAR* Name, const FLinearColor& Farbe, bool bDurchsichtig);
};
