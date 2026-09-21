#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergLaeden.generated.h"

// Wofuer das Geld aus den Auftraegen gut ist: ein Paintball-Laden (zu Fuss
// hineingehen, Waffen kaufen) und eine Lackiererei (mit dem Wagen
// hineinfahren, neu lackieren - wer dabei ungesehen ist, ist wie in GTA auch
// die Fahndung los). Gruene Marken, auf Minikarte und Vollkarte. Im Laden
// waehlen Pfeil hoch/runter, Enter kauft, hinausgehen schliesst.
UCLASS()
class LALABERG_API ALaLaBergLaeden : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergLaeden();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float DeltaSeconds) override;
 static TWeakObjectPtr<ALaLaBergLaeden> Instanz;

 enum class EArt : uint8 { Waffe, Lack };
 struct FWare { FString Name; int32 Preis = 0; EArt Art = EArt::Waffe; uint8 Waffe = 0; FLinearColor Farbe = FLinearColor::White; };
 struct FLaden {
  FString Name; FVector Ort = FVector::ZeroVector; bool bImWagen = false; bool bAufgestellt = false;
  TArray<FWare> Waren;
  class UStaticMeshComponent* Ring = nullptr; class UStaticMeshComponent* Saeule = nullptr;
 };
 static constexpr int32 PAINTBALL = 0;
 static constexpr int32 LACKIEREREI = 1;

 // Von der Spielart gerufen, sobald die Stadtkollision steht.
 void Stelle(int32 Laden, const FVector& Ort);
 const TArray<FLaden>& HoleLaeden() const { return Laeden; }
 // Der Laden, in dem man gerade steht, sonst INDEX_NONE.
 int32 HoleOffen() const { return Offen; }
 int32 HoleAuswahl() const { return Auswahl; }
 // Schon gekauft (Waffe) bzw. gerade der eigene Lack.
 bool HatSchon(const FWare& Ware) const;
 // Kauft die Ware im offenen Laden; die Rueckmeldung erscheint im HUD.
 bool Kaufe(int32 Ware);

private:
 TArray<FLaden> Laeden;
 int32 Offen = INDEX_NONE;
 int32 Auswahl = 0;
 // Nach dem Verlassen erst wieder oeffnen, wenn man ganz draussen war.
 bool bErstHinaus = false;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Teile;
 class UStaticMeshComponent* BaueTeil(const FLinearColor& Farbe);
 void Melde(const FString& Text) const;
};
