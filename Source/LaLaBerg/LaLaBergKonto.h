#pragma once
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LaLaBergKonto.generated.h"

// Was ueber das Spielende hinaus bleibt. Alles andere (Ort, Fahndung,
// laufender Auftrag) faengt beim naechsten Start neu an.
UCLASS()
class LALABERG_API ULaLaBergSpielstand : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY() int32 Version = 1;
 UPROPERTY() int32 Geld = 0;
 // Je Waffenart ein Bit (ELaLaBergWaffenArt); die Pistole hat man immer.
 UPROPERTY() int32 Waffen = 1;
 UPROPERTY() bool bHatLack = false;
 UPROPERTY() FLinearColor Lack = FLinearColor::White;
 UPROPERTY() int32 Erledigt = 0;

 // ------------------------------------------------------- Charakterwerte
 // Drei Faehigkeiten, die durch Benutzung wachsen, wie in dem Spiel, dem
 // das hier nachempfunden ist: wer laeuft, wird ausdauernder; wer trifft,
 // zielt besser; wer faehrt, faehrt sicherer. Gespeichert wird der
 // Erfahrungswert (0 bis 1000), nicht die Stufe - die rechnet sich daraus.
 UPROPERTY() int32 Ausdauer = 0;
 UPROPERTY() int32 Zielsicherheit = 0;
 UPROPERTY() int32 Fahren = 0;
 // Ruf oeffnet die Geschichte: Reviere, Gegner, Ausruestung.
 UPROPERTY() int32 Ruf = 0;
 // Je Revier ein Bit: wem die Altstadt, die Vorstadt, das Klinikum und das
 // Lechviertel gerade gehoeren (siehe LaLaBergRevier).
 UPROPERTY() int32 Reviere = 0;
 // Abschnitt der Geschichte, an dem man steht.
 UPROPERTY() int32 Kapitel = 0;
 // Je Fundstueck ein Bit (siehe ULaLaBergKonto::EFund): Werkstatt-,
 // Rotor- und Zuendschluessel. Sie liegen nicht im Rucksack, sie sind
 // Schalter in der Welt.
 UPROPERTY() int32 Funde = 0;
 // Wie viele Auftraege welcher Art erledigt sind - die Bewaehrung der
 // Kapitel haengt daran (siehe Docs/Geschichte.md).
 UPROPERTY() int32 Lieferungen = 0;
 UPROPERTY() int32 Taxifahrten = 0;
 UPROPERTY() int32 Rennen = 0;
 UPROPERTY() int32 Verfolgungen = 0;
 // Je Mission des Drehbuchs ein Bit (siehe LaLaBergDrehbuch).
 UPROPERTY() int32 Missionen = 0;
};

// Konto, gekaufte Waffen und Lack des eigenen Wagens - ein Subsystem der
// Spielinstanz wie die Menuesteuerung, damit Auftraege, Polizei, Laeden und
// HUD dasselbe Konto sehen. Speichert nach jeder Aenderung (Spielstand
// "LaLaBerg"). Laeuft das Spiel mit einem -LaLaBerg...-Schalter (Pruefläufe,
// Fotomodus), bleibt der echte Spielstand unberuehrt: dann beginnt das Konto
// leer und schreibt in "LaLaBergTest".
UCLASS()
class LALABERG_API ULaLaBergKonto : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Sammlung) override;
 virtual void Deinitialize() override;
 static ULaLaBergKonto* Hole(const UObject* Kontext);

 int32 HoleGeld() const { return Stand ? Stand->Geld : 0; }
 void Gutschrift(int32 Betrag);
 // Nur wenn genug da ist; liefert, ob abgebucht wurde.
 bool Bezahle(int32 Betrag);
 // Zieht hoechstens ab, was da ist, und liefert den tatsaechlichen Betrag.
 int32 Strafe(int32 Betrag);
 bool HatWaffe(uint8 Art) const { return Art == 0 || (Stand && (Stand->Waffen & (1 << Art))); }
 void GibWaffe(uint8 Art);
 bool HatLack(FLinearColor& Aus) const;
 void SetzeLack(const FLinearColor& Farbe);
 void ZaehleAuftrag();
 int32 HoleErledigt() const { return Stand ? Stand->Erledigt : 0; }

 // ------------------------------------------------------- Charakterwerte
 // Welche Faehigkeit. Die Reihenfolge steht auch im Spielstand.
 enum class EWert : uint8 { Ausdauer, Zielsicherheit, Fahren, Ruf };
 // Erfahrung dazurechnen. Wachsen darf nur langsam: die Werte sind auf
 // 1000 gedeckelt, und ein Punkt ist wenig - eine Stufe kostet 200.
 void Uebe(EWert Wert, float Punkte);
 int32 HoleWert(EWert Wert) const;
 // 0 bis 1: der Anteil am Hoechstwert, fuer Anzeigen und Wirkung.
 float Anteil(EWert Wert) const { return FMath::Clamp(HoleWert(Wert) / 1000.0f, 0.0f, 1.0f); }
 // 1 bis 5 - die Stufe, die im HUD steht.
 int32 Stufe(EWert Wert) const { return FMath::Clamp(HoleWert(Wert) / 200 + 1, 1, 5); }
 // Reviere und Kapitel - die Geschichte (siehe LaLaBergRevier).
 bool HatRevier(int32 Nummer) const { return Stand && (Stand->Reviere & (1 << Nummer)); }
 void NimmRevier(int32 Nummer);
 int32 HoleReviere() const { return Stand ? Stand->Reviere : 0; }
 int32 HoleKapitel() const { return Stand ? Stand->Kapitel : 0; }
 void SetzeKapitel(int32 Neu);
 // Fundstuecke: was ein geschlagener Kopf hinterlaesst.
 enum class EFund : uint8 { Werkstatt, Rotor, Zuendung };
 bool HatFund(EFund Fund) const { return Stand && (Stand->Funde & (1 << static_cast<int32>(Fund))); }
 void GibFund(EFund Fund);
 // Erledigte Auftraege nach Art. Die Reihenfolge ist die von
 // ELaLaBergAuftragsart (Lieferung, Taxi, Rennen, Verfolgung).
 void ZaehleArt(int32 Art);
 int32 HoleArtZahl(int32 Art) const;
 // Geschaffte Missionen des Drehbuchs.
 bool HatMission(int32 Nummer) const { return Stand && Nummer >= 0 && Nummer < 32 && (Stand->Missionen & (1 << Nummer)); }
 void SetzeMission(int32 Nummer);

 void Speichere();
 const FString& HoleSlot() const { return Slot; }
 // Fuer -LaLaBergLadenTest: den gespeicherten Stand frisch von der Platte.
 ULaLaBergSpielstand* LadeVonPlatte() const;

private:
 UPROPERTY() TObjectPtr<ULaLaBergSpielstand> Stand = nullptr;
 FString Slot;
};
