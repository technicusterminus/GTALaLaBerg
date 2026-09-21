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

 void Speichere();
 const FString& HoleSlot() const { return Slot; }
 // Fuer -LaLaBergLadenTest: den gespeicherten Stand frisch von der Platte.
 ULaLaBergSpielstand* LadeVonPlatte() const;

private:
 UPROPERTY() TObjectPtr<ULaLaBergSpielstand> Stand = nullptr;
 FString Slot;
};
