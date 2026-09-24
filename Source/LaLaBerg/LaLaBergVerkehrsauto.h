#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergWegfolger.h"
#include "LaLaBergKastenPool.h"
#include "LaLaBergVerkehrsauto.generated.h"

// Ein KI-Auto: faehrt seine Strasse ab und zurueck (siehe Tools/Export/
// prepare-verkehr.cjs), ohne Federung oder Motorphysik - anders als der
// fahrbare Wagen (ALaLaBergWagen) bewegt es sich kinematisch entlang seiner
// Wegpunkte. Dieselbe Karosserie aus wagen.json wie jeder geparkte und der
// fahrbare Wagen (LaLaBergWagenForm).
UCLASS()
class LALABERG_API ALaLaBergVerkehrsauto : public AActor, public ILaLaBergFarbbar, public ILaLaBergVerletzbar {
 GENERATED_BODY()
public:
 // ILaLaBergVerletzbar: 100 Punkte. Bei null bleibt der Wagen stehen, wird
 // russig und faehrt nicht mehr weiter - kein Wrack, kein Feuer.
 virtual void Verletze(float Schaden, const FVector& AusRichtung, ELaLaBergSchaden Art) override;
 virtual bool IstAusgeschaltet() const override { return Leben <= 0.0f; }
 virtual float Lebensanteil() const override { return FMath::Clamp(Leben / 100.0f, 0.0f, 1.0f); }
 ALaLaBergVerkehrsauto();
 virtual void Tick(float Zeit) override;
 // Vor BeginPlay setzen: die Wegpunkte in Unreal-Zentimetern.
 void SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh);
 // Fuer geparkte Autos (siehe LadeVerkehr): faerbt den Kasten-Fallback, falls
 // das CarConcept-Detailmodell (Sichtweiten-LOD) einmal nicht greift. Nach
 // SpawnActor (nicht deferred) lief BeginPlay schon mit der Default-Farbe -
 // baut das Netz bei Bedarf neu, statt grau zu bleiben.
 void SetzeLack(const FLinearColor& Farbe);
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) override;
 // Eigene, kurze Liste statt TActorIterator: bei 70 Autos, die einander
 // jedes Bild abfragen, durchsuchte TActorIterator sonst die ganze Stadt -
 // Tausende Akteure statt der paar Dutzend eigenen. Kostete spuerbar
 // Bildrate (26 auf 16 fps beim Hinzukommen der Ampeln).
 static TArray<ALaLaBergVerkehrsauto*> Alle;

 // Echte Kreuzungen aus dem Strassengraphen (siehe Tools/Export/prepare-
 // verkehr.cjs "kreuzungen") - einmalig von LaLaBergGameMode befuellt, bevor
 // die Autos selbst entstehen. Position in Unreal-Zentimetern, Klasse wie
 // roads[].c (0 = wichtigste Strasse). Fuer echtes Vorfahrtsrecht statt nur
 // Abstand zu jedem anderen Auto (siehe BremseVorKreuzung in der .cpp).
 static TArray<FVector> KreuzungOrte;
 static TArray<int32> KreuzungKlassen;
 // Zentimeter, parallel zu KreuzungOrte/-Klassen (Tools/Export/prepare-
 // verkehr.cjs "breite", aus der durchschnittlichen Fahrbahnbreite je
 // Strassenklasse geschaetzt) - fuer einen von der tatsaechlichen
 // Kreuzungsgroesse statt einem festen Wert abhaengigen Anhalteabstand
 // (siehe BremseVorKreuzung in der .cpp): eine breite oder mehrarmige
 // Kreuzung braucht mehr Abstand als eine schmale Nebenstrasse.
 static TArray<float> KreuzungBreiten;
 // Vor BeginPlay setzen (siehe SetzeRoute): die eigene Strassenklasse und
 // welche der obigen Kreuzungen auf der eigenen Route liegen.
 void SetzeKreuzung(int32 Klasse, const TArray<int32>& Indizes) { EigeneKlasse = Klasse; MeineKreuzungen = Indizes; }
 // Fahrbahnbreite (Meter) und Strassenklasse je Wegpunkt, parallel zur Route
 // aus SetzeRoute (Tools/Export/prepare-verkehr.cjs "bp"/"kp"). Seit eine
 // Fahrt ueber den Strassengraphen mehrere Strassen verkettet, gelten beide
 // nicht mehr fuer die ganze Route: wer von der Hauptstrasse in eine Gasse
 // abbiegt, faehrt ab dort auf schmalerer Fahrbahn und hat dort auch keine
 // Vorfahrt mehr. Leer = Rueckfall auf die Route-Werte aus SetzeKreuzung/
 // SetzeStrassenbreite.
 void SetzeSpurdaten(const TArray<float>& BreitenM, const TArray<int32>& Klassen);
 // Geschlossener Rundkurs statt Hin-und-Zurueck - siehe
 // FLaLaBergWegfolger::bRund. Nach SetzeRoute setzen.
 void SetzeRundkurs(bool bRund) { Weg.bRund = bRund; }
 // Wie oft dieses Auto am Routenende auf der Stelle umgekehrt ist - auf
 // einem Rundkurs immer 0 (siehe FLaLaBergWegfolger::Wenden).
 int32 HoleWenden() const { return Weg.Wenden; }
 // Modell und Lack dieses Autos - fuer die Uebernahme beim Einsteigen
 // (siehe ALaLaBergWagen::UebernimmModell): der Spieler soll in genau dem
 // Auto sitzen, das er angehalten hat.
 int32 HoleFahrzeugTyp() const { return FahrzeugTyp; }
 // Heult das Martinshorn dieses Wagens gerade? Fuer -LaLaBergPolizeiTest -
 // sonst waere "die Streife hat eine Sirene" nur eine Behauptung ueber den
 // Code statt ueber das, was man hoert.
 bool SireneAn() const { return bSireneLaeuft; }
 // Wagen einer Strassensperre: steht, aber im Einsatz - Blaulicht und Horn
 // laufen, obwohl keine Route anliegt.
 void SetzeSperre(bool bAn) { bSperre = bAn; }
 FLinearColor HoleLack() const { return Lack; }
 // Strassenklasse am aktuellen Wegpunkt (siehe SetzeSpurdaten) - nicht die
 // der ganzen Route. BremseVorKreuzung vergleicht damit den Vorfahrtsrang
 // dort, wo die Autos tatsaechlich aufeinandertreffen.
 int32 HoleKlasse() const {
  return KlasseJeWegpunkt.IsValidIndex(Weg.Index) ? KlasseJeWegpunkt[Weg.Index] : EigeneKlasse;
 }
 const TArray<int32>& HoleKreuzungen() const { return MeineKreuzungen; }
 // Echte Fahrbahnbreite der eigenen Route (Tools/Export/prepare-verkehr.cjs
 // "w", Meter aus den Strassendaten) statt eines fuer alle Autos gleichen
 // Werts - vor BeginPlay setzen wie SetzeKreuzung. BreiteM bleibt unter 3 m
 // nie unterschritten (siehe Export), deshalb kein Rueckfall auf 0 noetig.
 void SetzeStrassenbreite(float BreiteM) { StrassenBreite = BreiteM * 100.0f; }
 // Fahrbahnbreite in Zentimetern am aktuellen Wegpunkt - siehe SetzeSpurdaten.
 float BreiteJetzt() const {
  return BreiteJeWegpunkt.IsValidIndex(Weg.Index) ? BreiteJeWegpunkt[Weg.Index] : StrassenBreite;
 }
 // Fuer LaLaBergUeberholTest (siehe LaLaBergGameMode): ob dieses Auto
 // gerade ein anderes ueberholt (siehe Ueberholt unten und Tick in der .cpp).
 bool IstAmUeberholen() const { return Ueberholt.IsValid(); }

 // Streifenwagen (siehe ALaLaBergPolizei): vor FinishSpawning setzen. Faehrt
 // bei Rot und ohne Vorfahrt, traegt einen Blaulichtbalken und meldet einen
 // Paintball-Treffer als Tat.
 bool bPolizei = false;
 // -2 = zufaellig wie bisher, -1 = CarConcept, sonst ein Typ aus
 // LaLaBergWagenTypen - vor FinishSpawning setzen. Ein Streifenwagen soll
 // kein Bus sein.
 int32 WunschTyp = -2;
 // Neue Route mitten in der Fahrt (Polizei): ab dem ersten Punkt, am Ende
 // stehen bleiben statt umkehren. Aufnahme in Alle, falls noch nicht drin.
 void FolgeWeg(const TArray<FVector>& Punkte, float TempoKmh);
 // Streifenwagen ausser Dienst: Route weg, weit unter die Stadt, raus aus
 // Alle. Nicht zerstoert - die Pool-Instanzen werden nie zurueckgegeben
 // (siehe EndPlay), ein Wagen je Einsatz liefe sonst voll.
 void Parke();
 bool IstGeparkt() const { return !Weg.Gueltig(); }
 // Lage auf der Route, ohne den Seitversatz der eigenen Spur.
 FVector HoleRoutenOrt() const { return GetActorLocation() - Versatz; }
 // Welcher Wegpunkt gerade angesteuert wird (Index in der Route).
 int32 HoleWegIndex() const { return Weg.Index; }
 bool AmZiel() const { return Weg.Gueltig() && Weg.Index == Weg.Route.Num() - 1 && FVector::Dist(GetActorLocation(), Weg.Route.Last()) < 50.0f; }

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;

private:
 UPROPERTY() TObjectPtr<class UBoxComponent> Rumpf = nullptr;
 UPROPERTY() TObjectPtr<class USceneComponent> Karosseriepunkt = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;
 // Instanzen im gemeinsamen ALaLaBergAutoPool statt eigener Komponenten -
 // bei 70 Autos sonst zu viele Draw-Calls (siehe LaLaBergAutoPool.h). Auch
 // als Instanzen im Pool blieben alle 70 gleichzeitig sichtbaren Detail-
 // Autos zu teuer (Glas/Chrom-Material, viele Dreiecke) - deshalb zusaetzlich
 // ein einfaches Sichtweiten-LOD: nur Autos nah am Spieler zeigen das
 // Detailmodell, weiter entfernte den leichten Kasten aus wagen.json.
 double NaechsterPolizeiKontakt = 0.0;
 TArray<int32> PoolIndizes;
 bool bPoolGenutzt = false;
 bool bDetailliert = false;
 FLaLaBergWegfolger Weg;
 float Tempo = 900.0f;          // cm/s
 float StoerungBis = -10.0f;    // ein Treffer bremst kurz ab
 // Seitlicher Versatz zum Ausweichen vor einem Hindernis (siehe Tick) -
 // weicht sanft aus und wieder zurueck, statt starr auf der Route zu bremsen.
 float Seitversatz = 0.0f;
 // Der davon tatsaechlich angewandte Versatz in Weltkoordinaten - die
 // Route arbeitet mit der Lage ohne ihn (siehe Tick, HoleRoutenOrt).
 FVector Versatz = FVector::ZeroVector;
 // Waehrend eines Ueberholvorgangs das ueberholte Auto (siehe Tick) - leer,
 // solange kein Ueberholen laeuft. TWeakObjectPtr, weil das ueberholte Auto
 // unterwegs verschwinden kann (siehe ALaLaBergAutoPool-Verstecken/Zerstoeren).
 TWeakObjectPtr<class ALaLaBergVerkehrsauto> Ueberholt;
 FLinearColor Lack = FLinearColor(0.6f, 0.6f, 0.6f);
 float Leben = 100.0f;
 bool bNetzGebaut = false;
 // Blaulicht (nur bPolizei): zwei Leuchten auf dem Dach, im Wechsel.
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> Blaulicht[2] = {};
 UPROPERTY() TObjectPtr<class UPointLightComponent> Blitz = nullptr;
 // Martinshorn (nur bPolizei): laeuft, solange der Wagen im Einsatz ist -
 // also genau dann, wenn auch das Blaulicht blinkt.
 UPROPERTY() TObjectPtr<class UAudioComponent> Sirene = nullptr;
 bool bSireneLaeuft = false;
 bool bSperre = false;
 UPROPERTY() TObjectPtr<class UMaterialInstanceDynamic> BlauMaterial[2] = {};
 // Nur fuer geparkte Autos (siehe SetzeLack): der Kasten kommt dann aus
 // ALaLaBergKastenPool statt aus einem eigenen Netz - siehe dort fuer den
 // Grund (1078 einzelne Draw-Calls druckten die Bildrate auf 17 fps).
 bool bKastenGepoolt = false;
 ALaLaBergKastenPool::FGriff KastenGriff;

 // Fahrzeugvielfalt (siehe LaLaBergWagenTypen): -1 = CarConcept (Felder oben,
 // unveraendert), 0..TYPEN_ANZAHL-1 = LaLaBergWagenTypen::TYPEN[FahrzeugTyp].
 // Zufaellig in BeginPlay gewaehlt - siehe dort, warum nicht per Setter wie
 // SetzeLack (SpawnActor ruft BeginPlay schon vor jedem Setter auf).
 int32 FahrzeugTyp = -1;
 TArray<int32> TypPoolIndizes;
 // Fern-Mesh-Instanz fuer FahrzeugTyp (LaLaBergWagenTypen::LadeLod) - -1,
 // wenn der Typ keins mitbringt (z.B. vehicle07_Car); bTypFernBenutzt haelt
 // fest, ob das der Fall war, ohne bei jeder Pruefung neu nachzusehen.
 int32 TypFernIndex = -1;
 bool bTypFernBenutzt = false;

 // Eigene Strassenklasse (roads[].c, 0 = wichtigste) und die Indizes der
 // Kreuzungen (in KreuzungOrte/-Klassen) auf der eigenen Route - siehe
 // SetzeKreuzung. 3 = mittlere Klasse als Rueckfall, falls nie gesetzt.
 int32 EigeneKlasse = 3;
 TArray<int32> MeineKreuzungen;
 // Parallel zu Weg.Route, siehe SetzeSpurdaten. Breiten in Zentimetern.
 TArray<float> BreiteJeWegpunkt;
 TArray<int32> KlasseJeWegpunkt;
 // Zentimeter, siehe SetzeStrassenbreite. 300 cm (3 m) als Rueckfall, falls
 // nie gesetzt - dieselbe Mindestbreite wie im Export.
 float StrassenBreite = 300.0f;
};
