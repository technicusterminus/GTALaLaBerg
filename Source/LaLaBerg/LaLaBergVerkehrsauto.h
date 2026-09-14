#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergWegfolger.h"
#include "LaLaBergKastenPool.h"
#include "LaLaBergVerkehrsauto.generated.h"

// Ein KI-Auto: faehrt seine Strasse ab und zurueck (siehe Tools/Export/
// prepare-verkehr.cjs), ohne Federung oder Motorphysik - anders als der
// fahrbare Wagen (ALaLaBergWagen) bewegt es sich kinematisch entlang seiner
// Wegpunkte. Dieselbe Karosserie aus wagen.json wie jeder geparkte und der
// fahrbare Wagen (LaLaBergWagenForm).
UCLASS()
class LALABERG_API ALaLaBergVerkehrsauto : public AActor, public ILaLaBergFarbbar {
 GENERATED_BODY()
public:
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
 // Vor BeginPlay setzen (siehe SetzeRoute): die eigene Strassenklasse und
 // welche der obigen Kreuzungen auf der eigenen Route liegen.
 void SetzeKreuzung(int32 Klasse, const TArray<int32>& Indizes) { EigeneKlasse = Klasse; MeineKreuzungen = Indizes; }
 int32 HoleKlasse() const { return EigeneKlasse; }
 const TArray<int32>& HoleKreuzungen() const { return MeineKreuzungen; }

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
 TArray<int32> PoolIndizes;
 bool bPoolGenutzt = false;
 bool bDetailliert = false;
 FLaLaBergWegfolger Weg;
 float Tempo = 900.0f;          // cm/s
 float StoerungBis = -10.0f;    // ein Treffer bremst kurz ab
 // Seitlicher Versatz zum Ausweichen vor einem Hindernis (siehe Tick) -
 // weicht sanft aus und wieder zurueck, statt starr auf der Route zu bremsen.
 float Seitversatz = 0.0f;
 FLinearColor Lack = FLinearColor(0.6f, 0.6f, 0.6f);
 bool bNetzGebaut = false;
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
};
