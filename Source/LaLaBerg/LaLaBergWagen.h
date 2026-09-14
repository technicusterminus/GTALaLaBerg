#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergWagen.generated.h"

// Ein fahrbarer Wagen ohne Skelettnetz: die Karosserie entsteht zur Laufzeit
// aus denselben Querschnitten wie die geparkten Fahrzeuge, die Federung aus
// vier Strahlen nach unten. Das reicht fuer eine Stadt, in der man faehrt,
// und kommt ohne vorbereitete Fahrzeug-Assets aus.
UCLASS()
class LALABERG_API ALaLaBergWagen : public APawn, public ILaLaBergFarbbar {
 GENERATED_BODY()
public:
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
 void SetzeLack(const FLinearColor& Farbe) { Lack = Farbe; if (bGebaut) BaueKarosserie(); }
 // Wer eingestiegen ist, steigt auch wieder aus - der Wagen haelt die Figur.
 void SetzeFahrer(class ACharacter* Figur);
 // Fuer den Fahrtest: Gas und Lenkung ohne Tastatur setzen.
 // Fuer den Fahrtest: Gas und Lenkung ohne Tastatur setzen. Solange das
 // gilt, ueberschreibt die Eingabeachse die Werte nicht - sonst stellte die
 // unberuehrte Tastatur das Gas in jedem Bild auf null zurueck.
 void TestSteuerung(float Gas, float Lenken) { GasWert = Gas; LenkWert = Lenken; bTest = true; }
 int32 RaederAmBoden() const { return LetzteRaeder; }
 // Tempo in Fahrtrichtung, rueckwaerts negativ - fuer den Tacho.
 float TempoKmh() const { return FVector::DotProduct(GetVelocity(), GetActorForwardVector()) * 0.036f; }
 bool BremstGerade() const { return bBremse; }

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

 UPROPERTY() TObjectPtr<class UBoxComponent> Rumpf = nullptr;
 // Gemeinsamer Anschlusspunkt auf Fahrbahnhoehe fuer Netz (Procedural-
 // Fallback) UND die CarConcept-Teile - beide sollen an derselben Stelle
 // sitzen, nur einer davon ist zur Laufzeit tatsaechlich sichtbar.
 UPROPERTY() TObjectPtr<class USceneComponent> Karosseriepunkt = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> CarConceptTeile;
 // Fahrzeugvielfalt (siehe LaLaBergWagenTypen, LaLaBergVerkehrsauto): -2 =
 // noch nicht gewuerfelt, -1 = CarConcept, 0..TYPEN_ANZAHL-1 = CitySample-Typ.
 // Einmal in BaueKarosserie gewaehlt und behalten - SetzeLack faerbt das
 // gewaehlte Modell nur um, wuerfelt nicht neu.
 int32 FahrzeugTyp = -2;
 UPROPERTY() TObjectPtr<class USpringArmComponent> Ausleger = nullptr;
 UPROPERTY() TObjectPtr<class UCameraComponent> Kamera = nullptr;
 UPROPERTY() TObjectPtr<class UAudioComponent> Motorklang = nullptr;

 UPROPERTY() TObjectPtr<class ACharacter> Fahrer = nullptr;
 FLinearColor Lack = FLinearColor(0.72f, 0.74f, 0.76f);
 float GasWert = 0.0f;
 float LenkWert = 0.0f;
 float Lenkung = 0.0f;          // gefiltert, damit der Wagen nicht springt
 bool bBremse = false;
 bool bGebaut = false;
 int32 LetzteRaeder = 0;
 bool bTest = false;
 float EinstiegZeit = -10.0f;

 // Federung: vier Aufhaengungspunkte in Fahrzeugkoordinaten
 static constexpr int32 Raeder = 4;
 float Einfederung[Raeder] = { 0, 0, 0, 0 };
};
