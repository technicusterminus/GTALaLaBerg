#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergPolizei.generated.h"

// Was die Polizei auf den Plan ruft. Das Gewicht steht in der .cpp.
enum class ELaLaBergTat : uint8 { PassantBeschossen, PassantAngefahren, AutoGestohlen, PolizeiBeschossen };

// Fahndung und Streifenwagen. Taten sammeln Punkte, die Punkte ergeben 0-5
// Sterne (1, 4, 9, 16, 25 Punkte - jeder weitere Stern verlangt mehr). Je
// Stern ein Streifenwagen, der ueber den Strassengraphen (netz.json, siehe
// Tools/Export/prepare-netz.cjs) faehrt; die Route wird alle 1,5 s neu
// geplant. Sieht eine Streife den Spieler, fahren alle zu ihm; sonst zu dem
// Ort, an dem er zuletzt gesehen wurde, und suchen dort die Strassen ab.
// Wer ungesehen bleibt, ist die Fahndung nach einer Weile los - ausserhalb
// des Suchgebiets dreimal so schnell wie mittendrin. Wer neben einem
// Streifenwagen stehen bleibt, wird festgenommen.
UCLASS()
class LALABERG_API ALaLaBergPolizei : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergPolizei();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float DeltaSeconds) override;

 static TWeakObjectPtr<ALaLaBergPolizei> Instanz;
 // Von ueberall gerufen, wo etwas passiert (Passant, Streifenwagen,
 // Einsteigen) - ohne Polizei im Spiel einfach nichts.
 static void Melde(ELaLaBergTat Tat);

 int32 HoleSterne() const { return Sterne; }
 // Sieht gerade ein Streifenwagen den Spieler? Sonst laeuft die Suche ab.
 bool WirdGesehen() const { return bGesehen; }
 // 0..1: wie weit man die Verfolger schon abgeschuettelt hat.
 float HoleSuchAnteil() const;
 // 0..1: wie weit eine Festnahme schon ist (siehe Tick).
 float HoleFestnahmeAnteil() const { return FestnahmeUhr / FESTNAHME_DAUER; }
 int32 HoleFestnahmen() const { return Festnahmen; }
 // Aktive Streifenwagen - fuer die Karte.
 void HoleStreifen(TArray<FVector>& Orte) const;
 // Neu lackiert und dabei ungesehen (siehe ALaLaBergLaeden): die Fahndung
 // endet, die Streifen fahren ab.
 void Verwische() { if (Sterne > 0) Einstellen(TEXT("lackiert")); }
 // Fuer -LaLaBergPolizeiTest.
 void TestSetzeSterne(int32 Anzahl);
 float NaechsteStreifeCm() const;
 // Fuer -LaLaBergSperrTest: steht gerade eine Strassensperre, und wo?
 bool SperreSteht() const { return bSperreSteht; }
 FVector HoleSperrOrt() const { return SperrOrt; }
 // Route fuer die Karte (siehe ALaLaBergHUD::Route): kuerzester Weg ueber
 // denselben Strassengraphen, den die Streifen fahren - mit Einbahnregel,
 // also eine Autoroute. Liefert false ohne Netz oder ohne Weg.
 bool Route(const FVector& Von, const FVector& Nach, TArray<FVector>& Wegpunkte) const;

 static constexpr float FESTNAHME_DAUER = 2.5f;

private:
 // Strassengraph: Knoten und gerichtete Kanten als Adjazenz in CSR-Form.
 TArray<FVector> Knoten;
 TArray<int32> KantenAb;    // Knoten i: Kanten KantenAb[i]..KantenAb[i+1]-1
 TArray<int32> KantenNach;
 void LadeNetz();
 int32 NaechsterKnoten(const FVector& Ort, const FVector* Vorwaerts = nullptr) const;
 bool SuchePfad(int32 Von, int32 Nach, TArray<int32>& Pfad) const;

 float Punkte = 0.0f;
 int32 Sterne = 0;
 bool bGesehen = false;
 double ZuletztGesehen = 0.0;
 float FestnahmeUhr = 0.0f;
 int32 Festnahmen = 0;
 double NaechsterEinsatz = 0.0;
 void Erhoehe(float Gewicht);
 void Einstellen(const TCHAR* Grund);
 void Festnahme();
 float Suchdauer() const { return 6.0f + 4.0f * Sterne; }
 // Wo der Spieler zuletzt gesehen wurde, und wie gross das Gebiet ist, das
 // die Streifen dort absuchen.
 FVector LetzterOrt = FVector::ZeroVector;
 float Suchradius() const { return 15000.0f + 5000.0f * Sterne; }
 float Ungesehen = 0.0f;
 bool ImSuchgebiet(const FVector& Wo) const { return FVector::Dist2D(Wo, LetzterOrt) < Suchradius(); }

 struct FStreife {
  class ALaLaBergVerkehrsauto* Auto = nullptr; double NaechstePlanung = 0.0; bool bImEinsatz = false;
  // Beim Absuchen: der Knoten, zu dem die Streife gerade faehrt.
  FVector Suchziel = FVector::ZeroVector; bool bSucht = false;
  // Die Knoten der aktuellen Route (Route[k] = Knoten[Pfad[k-1]]) - eine
  // Neuplanung setzt am angesteuerten Knoten fort, statt nach der
  // Blickrichtung zu raten.
  TArray<int32> Pfad;
 };
 TArray<FStreife> Streifen;
 UPROPERTY() TArray<TObjectPtr<class ALaLaBergVerkehrsauto>> StreifenHalter;   // haelt die Wagen fuer den GC

 // Strassensperre ab drei Sternen: ein Stueck voraus auf der Strasse, die
 // der Spieler gerade faehrt, stellen sich zwei Streifenwagen quer und
 // dazwischen stehen rot-weisse Baken. Wer trotzdem durchfaehrt, kommt
 // durch - es ist eine Sperre, keine Wand.
 void StelleSperre(const FVector& Spieler, const FVector& Fahrtrichtung);
 void RaeumeSperre();
 // Knoten ein Stueck voraus in Fahrtrichtung, plus die Strassenrichtung
 // dort. INDEX_NONE, wenn der Graph dort nicht weiterfuehrt.
 int32 KnotenVoraus(const FVector& Von, const FVector& Richtung, float WeiteCm, FVector& Strassenrichtung) const;
 UPROPERTY() TArray<TObjectPtr<class ALaLaBergVerkehrsauto>> Sperrwagen;
 UPROPERTY() TArray<TObjectPtr<class UStaticMeshComponent>> Baken;
 FVector SperrOrt = FVector::ZeroVector;
 bool bSperreSteht = false;
 double SperreSeit = 0.0;

 bool Entsende(FStreife& S, const FVector& Ziel);
 void Plane(FStreife& S, const FVector& Ziel);
 bool Sieht(const class ALaLaBergVerkehrsauto* Auto, const APawn* Spieler) const;
};
