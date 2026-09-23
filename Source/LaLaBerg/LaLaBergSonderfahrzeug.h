#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LaLaBergSonderfahrzeug.generated.h"

// Die beiden versteckten Fahrzeuge: ein Panzer und ein Hubschrauber. Beide
// sind in Blender gebaut (Tools/baue_panzer_heli.py) und fahren bzw. fliegen
// kinematisch - kein Chaos-Fahrwerk, keine Rotorphysik. Sie sollen sich
// finden und benutzen lassen, nicht simuliert werden.
//
// Gefunden werden sie nicht auf der Karte: der Panzer steht abseits im
// Waldstueck, der Hubschrauber auf dem Dach des Klinikums (siehe
// LaLaBergGameMode, Abschnitt Sonderfahrzeuge).
UENUM()
enum class ELaLaBergSonderart : uint8 { Panzer, Hubschrauber };

UCLASS()
class LALABERG_API ALaLaBergSonderfahrzeug : public APawn {
 GENERATED_BODY()
public:
 ALaLaBergSonderfahrzeug();
 virtual void Tick(float Zeit) override;
 virtual void SetupPlayerInputComponent(class UInputComponent* Eingabe) override;

 // Vor FinishSpawning setzen: welche der beiden Maschinen das hier wird.
 void SetzeArt(ELaLaBergSonderart NeueArt) { Art = NeueArt; }
 ELaLaBergSonderart HoleArt() const { return Art; }
 // Wer eingestiegen ist (siehe ALaLaBergCharacter::Einsteigen).
 void SetzeFahrer(class ACharacter* Figur);
 bool HatFahrer() const { return Fahrer != nullptr; }
 // Fuer -LaLaBergSonderTest: Steuerung ohne Tastatur.
 void TestSteuerung(float Schub, float Drehen, float Steigen);
 void TestAussteigen() { Aussteigen(); }
 float TempoKmh() const;

protected:
 virtual void BeginPlay() override;

private:
 void Schub(float Wert) { if (!bTest) SchubWert = Wert; }
 void Drehen(float Wert) { if (!bTest) DrehWert = Wert; }
 void Umsehen(float Wert);
 void Nicken(float Wert);
 void Steigen();
 void Sinken();
 void Aussteigen();
 // Boden unter dem Fahrzeug; false, wenn dort nichts ist.
 bool Bodenhoehe(float& Z) const;

 UPROPERTY() TObjectPtr<class UBoxComponent> Rumpf = nullptr;
 // Panzer: Wanne und Turm. Hubschrauber: Zelle, Haupt- und Heckrotor.
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Koerper = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Dreher = nullptr;      // Turm bzw. Hauptrotor
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Heckrotor = nullptr;
 UPROPERTY() TObjectPtr<class USpringArmComponent> Ausleger = nullptr;
 UPROPERTY() TObjectPtr<class UCameraComponent> Kamera = nullptr;
 UPROPERTY() TObjectPtr<class UAudioComponent> Klang = nullptr;
 UPROPERTY() TObjectPtr<class ACharacter> Fahrer = nullptr;

 ELaLaBergSonderart Art = ELaLaBergSonderart::Panzer;
 float SchubWert = 0.0f, DrehWert = 0.0f, SteigWert = 0.0f;
 float Tempo = 0.0f;          // cm/s laengs
 float Sinkflug = 0.0f;       // cm/s senkrecht (nur Hubschrauber)
 float Drehphase = 0.0f;      // Rotorstellung in Grad
 bool bTest = false;
 double EinstiegZeit = -10.0;
};
