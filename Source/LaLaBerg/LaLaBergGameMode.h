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
 // spaeter haengt daran der Tageszeitwechsel.
 UPROPERTY() TObjectPtr<class ULightComponent> SonnenLicht=nullptr;
 UPROPERTY() TObjectPtr<class APostProcessVolume> Belichtung=nullptr;
 UPROPERTY() TObjectPtr<class APlayerStart> Startpunkt=nullptr;
public:
 virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
