#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LaLaBergVerletzbar.h"
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
class LALABERG_API ALaLaBergSonderfahrzeug : public APawn, public ILaLaBergVerletzbar {
 GENERATED_BODY()
public:
 // ILaLaBergVerletzbar: auch die beiden Fundstuecke halten nicht alles aus.
 // Der Panzer steckt Farbkugeln weg, wie es sich fuer eine Wanne gehoert -
 // eine Sprenggranate aber nicht; die duenne Zelle des Hubschraubers nimmt
 // jeden Treffer krumm. Bei null faellt die Maschine aus: der Panzer bleibt
 // stehen, der Hubschrauber sinkt zu Boden, und wer drin sitzt, steigt aus.
 virtual void Verletze(float Schaden, const FVector& AusRichtung, ELaLaBergSchaden Schadensart) override;
 virtual bool IstAusgeschaltet() const override { return bKaputt; }
 virtual float Lebensanteil() const override { return FMath::Clamp(Leben / FMath::Max(1.0f, VollesLeben), 0.0f, 1.0f); }
 float HoleLeben() const { return Leben; }
 // Fuer -LaLaBergSonderSchadenTest: Treffer ohne Schuetzen.
 void TestVerletze(float Schaden, ELaLaBergSchaden Schadensart) { Verletze(Schaden, -GetActorForwardVector(), Schadensart); }

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
 // Fuer -LaLaBergKanonenTest: einmal feuern, ohne Maustaste.
 bool TestFeuern() { return Feuern(); }
 int32 HoleSchuesse() const { return Schuesse; }
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
 // Die Kanone des Panzers: ein Schuss alle drei Sekunden, danach faehrt das
 // Rohr zurueck und die Wanne setzt sich. Liefert false, wenn noch geladen
 // wird oder es kein Panzer ist.
 bool Feuern();
 // Die Eingabe erwartet eine Funktion ohne Rueckgabe.
 void FeuerTaste() { Feuern(); }
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
 // Kanone: Zeitpunkt des letzten Schusses, Rueckstoss (1 unmittelbar nach
 // dem Schuss, klingt ab) und die Zahl der Schuesse fuer den Test.
 double LetzterSchuss = -10.0;
 float Rueckstoss = 0.0f;
 // Lebenspunkte: der Panzer haelt ein Vielfaches des Hubschraubers aus.
 float Leben = 0.0f, VollesLeben = 0.0f;
 bool bKaputt = false;
 // Faerbt Wanne, Turm und Rotoren um - einmal beim Bauen, einmal beim Ausfall.
 void Faerbe(const FLinearColor& Farbe);
 int32 Schuesse = 0;
 UPROPERTY() TObjectPtr<class UPointLightComponent> Muendungsfeuer = nullptr;
 bool bTest = false;
 double EinstiegZeit = -10.0;
};
