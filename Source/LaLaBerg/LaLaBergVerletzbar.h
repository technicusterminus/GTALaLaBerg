#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "LaLaBergVerletzbar.generated.h"

// Woher der Schaden kommt. Die Art entscheidet nicht ueber die Hoehe (die
// steht im Aufruf), sondern ueber die Reaktion: ein Anprall wirft die Figur
// um, ein Treffer der Panzerkanone laesst nichts mehr stehen.
UENUM()
enum class ELaLaBergSchaden : uint8 {
 Anprall,     // angefahren, aufgeprallt
 Beschuss,    // Paintball-Treffer
 Sprengung,   // Panzerkanone
 Sturz,       // aus der Hoehe gefallen
};

UINTERFACE(MinimalAPI)
class ULaLaBergVerletzbar : public UInterface { GENERATED_BODY() };

// Wer diese Schnittstelle traegt, hat Lebenspunkte: Spieler, Passanten,
// Fahrzeuge. Alles andere in der Stadt ist gebaute Geometrie und bleibt
// heil - ein Haus faellt nicht um, weil man dagegen faehrt.
class LALABERG_API ILaLaBergVerletzbar {
 GENERATED_BODY()
public:
 // Schaden in Lebenspunkten, Richtung des Stosses (Weltkoordinaten,
 // normiert) und wodurch. Wer schon ausgeschaltet ist, nimmt nichts mehr.
 virtual void Verletze(float Schaden, const FVector& AusRichtung, ELaLaBergSchaden Art) = 0;
 // Umgefallen, ausgebrannt, festgenommen - wie auch immer: es reagiert
 // nicht mehr.
 virtual bool IstAusgeschaltet() const = 0;
 // 0 bis 1, fuer Anzeigen.
 virtual float Lebensanteil() const = 0;
};
