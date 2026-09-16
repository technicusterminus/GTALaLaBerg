#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "LaLaBergGameMode.generated.h"
class FJsonObject;

UCLASS()
class LALABERG_API ALaLaBergGameMode : public AGameModeBase {
 GENERATED_BODY()
public:
 ALaLaBergGameMode();
 virtual void InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
private:
 bool bSceneReady=false;
 int32 BuildingCount=0;
 // Laedt die Stadt aus fertigen StaticMesh-Assets. Liefert false, wenn noch
 // keine da sind - dann baut InitGame sie wie bisher zur Laufzeit.
 bool LadeAusAssets(TSharedPtr<FJsonObject>& Metadaten);
 // Naechste freie Stelle auf einer Strasse, ausgerichtet entlang der Fahrbahn.
 FTransform SucheFahrbahn(const FVector& Nahe,bool& bGefunden) const;
 FVector FahrtStart=FVector::ZeroVector;
 FTimerHandle FahrtUhr;
 // Bildratenmessung waehrend des Fahrtests
 uint64 FahrtBilder=0, FahrtLetzteBilder=0;
 double FahrtZeit=0, FahrtLetzteUhr=0;
 float FahrtSchlechteste=1000.0f;
 static void Beleg(const FString& Zeile);
 // Sonne und Belichtung bleiben greifbar: der Fotomodus stellt sie um, und
 // der Tageszeitwechsel haengt daran (siehe AktualisiereTageszeit).
 UPROPERTY() TObjectPtr<class ULightComponent> SonnenLicht=nullptr;
 UPROPERTY() TObjectPtr<class USkyLightComponent> SkyLicht=nullptr;
 UPROPERTY() TObjectPtr<class APostProcessVolume> Belichtung=nullptr;
 // Tageszeit in Stunden (0-24, 12=Mittag) - eine volle Umdrehung alle
 // TAGESLAENGE_SEKUNDEN echte Sekunden. Dreht SonnenLicht und passt Staerke/
 // Farbe an (Sonnenauf-/-untergang waermer, Nacht schwaecher und blaeulich).
 // Die feste Ausgangsdrehung aus InitGame (FRotator(-50,152,0)) bleibt der
 // Bezug fuer Mittag - der Azimut (152) aendert sich nicht, nur die Elevation,
 // eine bewusste Vereinfachung statt eines echten Sonnenstands.
 float Tageszeit=12.0f;
 static constexpr float TAGESLAENGE_SEKUNDEN=600.0f;
 static constexpr float SONNEN_AZIMUT=152.0f;
 void AktualisiereTageszeit(float DeltaSeconds);
 UPROPERTY() TObjectPtr<class APlayerStart> Startpunkt=nullptr;
 int32 FarbtrefferZahl=0;
 // Wegpunkte fuer KI-Verkehr und Passanten laden und die Figuren dazu
 // erzeugen (Tools/Export/prepare-verkehr.cjs -> Content/SourceData/Verkehr).
 void LadeVerkehr();
 int32 AutoZahl=0, PassantZahl=0, AmpelZahl=0, GeparktZahl=0;
 // Fuer -LaLaBergAmpelTest: die laengste tatsaechlich vorkommende
 // Phasenzahl bestimmt, wie lang ein voller Kreuzungszyklus maximal dauert
 // (siehe dort) - bei mehr als zwei Phasen (mehr als vier Kreuzungsarme)
 // laenger als der alte, fest angenommene Zweiphasen-Zyklus.
 int32 GroessteAnzahlPhasen=2;
public:
 virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
 // Von jeder Farbkugel beim Aufprall gerufen - fuer HUD und Waffentest.
 void ZaehleFarbtreffer() { FarbtrefferZahl++; }
 int32 HoleFarbtreffer() const { return FarbtrefferZahl; }
};
