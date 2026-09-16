#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergAmpel.generated.h"

// Eine Lichtsignalanlage an einem echten amtlichen Standort (siehe
// Tools/Export/prepare-verkehr.cjs, aus OSM highway=traffic_signals).
// Wechselt selbststaendig Rot/Gelb/Gruen; KI-Autos fragen HaeltAn() ab und
// bremsen vor einer roten Ampel. Die Exportkette gruppiert nahe beieinander
// liegende Ampeln zu Kreuzungen und teilt sie in so viele Phasen (je eine
// pro tatsaechlich unterschiedlicher Fahrbahnachse) wie die Kreuzung Arme
// hat - kein echter OSM-Kreuzungsgraph, aber genug, damit sich kreuzende
// Strassen an derselben Kreuzung nie gleichzeitig Gruen zeigen, auch bei
// mehr als vier Armen (siehe SetzeGruppe).
UCLASS()
class LALABERG_API ALaLaBergAmpel : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergAmpel();
 // Fuer KI-Autos: steht die Ampel gerade auf Rot oder Gelb (also: anhalten)?
 bool HaeltAn() const { return Zustand != 1; }
 // Vor BeginPlay setzen: Gruppe = Kreuzungs-ID, Phase = die eigene von
 // NeueAnzahlPhasen nicht ueberlappenden Zeitfenstern derselben Kreuzung
 // (siehe Tools/Export/prepare-verkehr.cjs).
 void SetzeGruppe(int32 NeueGruppe, int32 NeuePhase, int32 NeueAnzahlPhasen) {
  Gruppe = NeueGruppe; Phase = NeuePhase; AnzahlPhasen = FMath::Max(1, NeueAnzahlPhasen);
 }
 int32 HoleGruppe() const { return Gruppe; }
 int32 HolePhase() const { return Phase; }
 int32 HoleAnzahlPhasen() const { return AnzahlPhasen; }
 // Fenster je Phase in Sekunden (Gruen+Gelb) - fuer den Aufrufer, der die
 // laengste vorkommende Zyklusdauer wissen will (siehe LaLaBergGameMode.cpp
 // -LaLaBergAmpelTest), ohne die privaten Zeitkonstanten selbst zu kennen.
 static float PhasenfensterS();
 // Kurze eigene Liste statt TActorIterator - siehe LaLaBergVerkehrsauto.h.
 static TArray<ALaLaBergAmpel*> Alle;

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float Zeit) override;

private:
 void BaueKopf();

 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 // 0 = Rot, 1 = Gruen, 2 = Gelb
 int32 Zustand = 0;
 float Bis = 0.0f;
 int32 Gruppe = 0;
 int32 Phase = 0;
 int32 AnzahlPhasen = 2;
};
