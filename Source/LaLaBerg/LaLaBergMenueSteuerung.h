#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "LaLaBergMenueSteuerung.generated.h"

// Haelt das Menue am Leben und kennt den Spielzustand. Als Subsystem, damit
// die Oberflaeche nichts ueber Spielart oder Figur wissen muss.
UCLASS()
class LALABERG_API ULaLaBergMenueSteuerung : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Sammlung) override;
 virtual void Deinitialize() override;

 void ZeigeMenue(bool bStartbild);
 void SchliesseMenue();
 void Umschalten();
 bool IstOffen() const { return Overlay.IsValid(); }
 bool IstStartbild() const { return bStartbild; }

 // Einstellungen, die das Menue anfasst
 void SetzeStufe(int32 Stufe);
 int32 HoleStufe() const;
 void SetzeAufloesungsIndex(int32 Index);
 int32 HoleAufloesungsIndex() const { return AufloesungsIndex; }
 FIntPoint HoleAufloesung(int32 Index) const;
 int32 AnzahlAufloesungen() const;
 void SetzeVollbild(bool bVollbild);
 bool IstVollbild() const;
 void SetzeLautstaerke(float Wert);
 float HoleLautstaerke() const { return Lautstaerke; }

private:
 void SetzeEingabe(bool bMenue);
 TSharedPtr<class SWidget> Overlay;
 bool bStartbild = false;
 int32 AufloesungsIndex = -1;
 float Lautstaerke = 0.8f;
};
