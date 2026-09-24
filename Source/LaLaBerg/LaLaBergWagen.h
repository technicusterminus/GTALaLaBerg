#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergWagen.generated.h"

// Ein fahrbarer Wagen ohne Skelettnetz: die Karosserie entsteht zur Laufzeit
// aus denselben Querschnitten wie die geparkten Fahrzeuge (oder aus den
// CarConcept-/CitySample-Teilen, siehe BaueKarosserie). Die Physik selbst
// laeuft ueber UChaosWheeledVehicleMovementComponent (Bewegung) - echtes
// Motor-/Getriebe-Kennfeld statt vier Federstrahlen, ohne dass die
// unsichtbare Rumpf-Kollisionsbox dafuer ein Skelettnetz braucht.
UCLASS()
class LALABERG_API ALaLaBergWagen : public APawn, public ILaLaBergFarbbar, public ILaLaBergVerletzbar {
 GENERATED_BODY()
public:
 // ILaLaBergVerletzbar: 100 Punkte. Bei null steht der Motor - man kann
 // aussteigen und zu Fuss weiter, der Wagen bleibt liegen.
 virtual void Verletze(float Schaden, const FVector& AusRichtung, ELaLaBergSchaden Art) override;
 virtual bool IstAusgeschaltet() const override { return Leben <= 0.0f; }
 virtual float Lebensanteil() const override { return FMath::Clamp(Leben / 100.0f, 0.0f, 1.0f); }
 // ILaLaBergFarbbar: ein Paintball-Treffer faerbt den Lack um und stoesst
 // leicht in Trefferrichtung - ein Wagen soll spuerbar getroffen wirken,
 // nicht nur die Farbe wechseln.
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) override;
 ALaLaBergWagen();
 virtual void Tick(float Zeit) override;
 virtual void SetupPlayerInputComponent(UInputComponent* Eingabe) override;

 // Farbe der Karosserie, bevor das Netz gebaut wird
 // Der Lack kann auch nach dem Erscheinen gesetzt werden; dann wird die
 // Karosserie neu aufgebaut. Sonst blieb jeder Wagen silbern.
 // bModell: auch das echte Fahrzeugmodell umlackieren (Lackiererei,
 // gespeicherter Lack) - sonst gilt die Farbe nur fuer die Ersatzform, und
 // jedes Modell behaelt seine eigene Lackierung wie bisher.
 void SetzeLack(const FLinearColor& Farbe, bool bModell = false);
 // Ein uebernommenes KI-Auto behaelt sein Modell und seinen Lack: vor
 // FinishSpawning setzen, denn BeginPlay baut die Karosserie schon. Ohne das
 // sass man nach jedem Einsteigen im roten CarConcept-Flitzer, egal welches
 // Auto man angehalten hatte.
 void UebernimmModell(int32 Typ, const FLinearColor& Farbe) { FahrzeugTyp = Typ; Lack = Farbe; }
 // Fuer -LaLaBergUebernahmeTest: welches Modell dieser Wagen fahrt.
 int32 HoleFahrzeugTyp() const { return FahrzeugTyp; }
 // Wer eingestiegen ist, steigt auch wieder aus - der Wagen haelt die Figur.
 void SetzeFahrer(class ACharacter* Figur);
 // Autoradio: 0 = aus, sonst der Sender (siehe SENDER in der .cpp).
 // Umschalten mit der Radiotaste; der Stand haelt im Spielstand.
 void SchalteRadio();
 int32 HoleSender() const { return Sender; }
 FString HoleSendername() const;
 // Fuer den Fahrtest: Gas und Lenkung ohne Tastatur setzen.
 // Fuer den Fahrtest: Gas und Lenkung ohne Tastatur setzen. Solange das
 // gilt, ueberschreibt die Eingabeachse die Werte nicht - sonst stellte die
 // unberuehrte Tastatur das Gas in jedem Bild auf null zurueck.
 void TestSteuerung(float Gas, float Lenken) { GasWert = Gas; LenkWert = Lenken; bTest = true; }
 int32 RaederAmBoden() const { return LetzteRaeder; }
 void TestAnhalten() { TestSteuerung(0.0f, 0.0f); Bremsen(); }
 void TestAussteigen() { Aussteigen(); }
 // Tempo in Fahrtrichtung, rueckwaerts negativ - fuer den Tacho.
 float TempoKmh() const { return FVector::DotProduct(GetVelocity(), GetActorForwardVector()) * 0.036f; }
 bool BremstGerade() const { return bBremse; }
 // Fuer -LaLaBergAntriebTest: Karosseriemasse, Motor und Raeder in einer
 // Zeile - um zu klaeren, warum der Antrieb das Hundertfache braucht.
 FString Antriebsbefund() const;
 void SetzeTestDrehmoment(float Nm);
 // Schubkraft in Newton, jedes Bild nach vorn auf den Rumpf - ohne Motor.
 void TestSchub(float Newton) { SchubNewton = Newton; }

protected:
 virtual void BeginPlay() override;

private:
 public:
 void BaueKarosserie();
 // Form aus wagen.json; false, wenn die Datei fehlt - dann die einfache Form.
 bool BaueAusVorlage();
 private:
 void Gas(float Wert);
 void Lenken(float Wert);
 void Umsehen(float Wert);
 void Nicken(float Wert);
 void Aussteigen();
 void Bremsen();
 void Loesen();
 // Dreht/lenkt die sichtbaren CarConcept-Felgen nach dem Chaos-Radzustand -
 // nur kosmetisch, nur wenn das gewaehlte Fahrzeugmodell benannte
 // Felgenteile hat (siehe LaLaBergWagenForm::CARCONCEPT_TEILE).
 void AktualisiereRaeder();

 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Rumpf = nullptr;
 // Gemeinsamer Anschlusspunkt auf Fahrbahnhoehe fuer Netz (Procedural-
 // Fallback) UND die CarConcept-Teile - beide sollen an derselben Stelle
 // sitzen, nur einer davon ist zur Laufzeit tatsaechlich sichtbar.
 UPROPERTY() TObjectPtr<class USceneComponent> Karosseriepunkt = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> CarConceptTeile;
 // Lack auf die Lackteile des Modells - siehe SetzeLack.
 bool bModellLack = false;
 void FaerbeModell();
 // Fahrzeugvielfalt (siehe LaLaBergWagenTypen, LaLaBergVerkehrsauto): -2 =
 // noch nicht gewuerfelt, -1 = CarConcept, 0..TYPEN_ANZAHL-1 = CitySample-Typ.
 // Einmal in BaueKarosserie gewaehlt und behalten - SetzeLack faerbt das
 // gewaehlte Modell nur um, wuerfelt nicht neu.
 int32 FahrzeugTyp = -2;
 float Leben = 100.0f;
 int32 Sender = 0;
 UPROPERTY() TObjectPtr<class UAudioComponent> Radio = nullptr;
 UPROPERTY() TObjectPtr<class USpringArmComponent> Ausleger = nullptr;
 UPROPERTY() TObjectPtr<class UCameraComponent> Kamera = nullptr;
 UPROPERTY() TObjectPtr<class UAudioComponent> Motorklang = nullptr;
 // Echte Chaos-Vehicle-Simulation statt vier Federstrahlen: Motor mit
 // Drehmomentkurve, Automatikgetriebe, Vorderradlenkung, Hinterradantrieb.
 UPROPERTY() TObjectPtr<class ULaLaBergWagenBewegung> Bewegung = nullptr;

 UPROPERTY() TObjectPtr<class ACharacter> Fahrer = nullptr;
 FLinearColor Lack = FLinearColor(0.72f, 0.74f, 0.76f);
 float GasWert = 0.0f;
 float LenkWert = 0.0f;
 float Lenkung = 0.0f;          // gefiltert, damit der Wagen nicht springt
 bool bBremse = false;
 bool bGebaut = false;
 int32 LetzteRaeder = 0;
 bool bTest = false;
 float SchubNewton = 0.0f;
 float EinstiegZeit = -10.0f;
};
