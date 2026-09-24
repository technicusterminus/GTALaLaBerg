#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergFarbbar.h"
#include "LaLaBergWegfolger.h"
#include "LaLaBergPassantKI.generated.h"

// Ein KI-Passant: geht seinen Gehweg ab und zurueck (siehe Tools/Export/
// prepare-verkehr.cjs). Kein importiertes Skelett-Mesh (dafuer fehlt eine
// Rigging-Pipeline in diesem Projekt) - stattdessen ein von Hand gebautes
// Gelenk-Rig aus SceneComponents: Huefte und Knie je Bein, Schulter je Arm.
// Jedes Gelenk dreht sich prozedural nach der Gehphase, die Glieder selbst
// sind unbewegte Kaesten, einmal gebaut und am jeweiligen Gelenk befestigt -
// kein Nachbau der Netz-Abschnitte mehr bei jedem Schritt.
UCLASS()
class LALABERG_API ALaLaBergPassantKI : public AActor, public ILaLaBergFarbbar {
 GENERATED_BODY()
public:
 ALaLaBergPassantKI();
 virtual void Tick(float Zeit) override;
 void SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh);
 virtual void ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) override;
 float HoleTempo() const { return Tempo; }
 // Welthoehe der tiefsten sichtbaren Stelle (die Sohle). Die Bodenprobe in
 // -LaLaBergVerkehrFoto vergleicht sie mit der Oberflaeche darunter - die
 // Aktorhoehe allein sagt nichts darueber, ob die Figur im Boden steckt.
 float HoleSohleZ() const;
 // Kurze eigene Liste statt TActorIterator - siehe LaLaBergVerkehrsauto.h.
 static TArray<ALaLaBergPassantKI*> Alle;

protected:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;

private:
 void BaueOberkoerper();
 // Ein Kasten-Netz an einem Gelenk, Ursprung oben am Drehpunkt, nach unten
 // um "Laenge" ausgedehnt - so biegt eine Drehung des Gelenks das Glied wie
 // an einem echten Scharnier.
 void BaueGlied(class UProceduralMeshComponent* Netz, const FLinearColor& Farbe, float HalbBreite, float Laenge);
 // Die Route bringt die Hoehe des ausgeraeumten Gelaendes mit (Tools/Export/
 // prepare-verkehr.cjs), die sichtbare Stadt liegt daruber: Wiese, Gehweg-
 // platten und Absaetze sind bis zu einem knappen Meter hoeher. Ohne
 // Nachmessen standen die Figuren dort bis zur Huefte im Gras. Darum je
 // Viertelsekunde ein Strahl nach unten; der gemessene Versatz zur Routen-
 // hoehe wird sanft nachgefuehrt, damit eine Bordkante die Figur nicht
 // springen laesst.
 void PruefeBoden(const FVector& RoutenOrt);

 UPROPERTY() TObjectPtr<class UCapsuleComponent> Huelle = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;         // Kopf, Hals, Rumpf - unbewegt

 // Je Seite: 0 = rechts, 1 = links.
 UPROPERTY() TObjectPtr<class USceneComponent> Huefte[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Oberschenkel[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> Knie[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Unterschenkel[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> Schulter[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Oberarm[2] = {};

 // Bevorzugt: ein echtes, lizenziertes Skeletal Mesh mit echter Animation
 // (CC0, Quaternius - siehe Content/SourceData/People/LIZENZ.md) statt des
 // von Hand gebauten Kasten-Rigs oben. Rumpf (SkelettKoerper) traegt die
 // Animation, Kopf/Fuesse/Beine folgen per LeaderPoseComponent derselben
 // Pose - vier Teile aus demselben modularen Paket, ein Skelett. Nur wenn
 // die Assets fehlen, bleibt es beim Kasten-Rig (siehe BeginPlay).
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettKoerper = nullptr;
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettKopf = nullptr;
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettFuesse = nullptr;
 UPROPERTY() TObjectPtr<class USkeletalMeshComponent> SkelettBeine = nullptr;
 bool bSkelettGenutzt = false;
 bool bLaeuftGerade = false;    // aktuell Walk- statt Idle-Animation
 // Welche Figur gewaehlt wurde (siehe BeginPlay): 0 = Farmer (modular,
 // externe Animation), 1..N = Index+1 in EINZEL_FIGUREN (eigenes Mesh samt
 // Animation). Nur gueltig, wenn bSkelettGenutzt.
 int32 FigurTyp = 0;

 FLaLaBergWegfolger Weg;
 float Tempo = 140.0f;             // cm/s, gewoehnliches Gehtempo
 float StolpertBis = -10.0f;       // ein Treffer bremst kurz
 float Seitversatz = 0.0f;         // seitliches Ausweichen, siehe Tick
 float Gehphase = 0.0f;
 FVector LetzterAusweichOffset = FVector::ZeroVector;
 FLinearColor Jacke;
 // Siehe PruefeBoden: Bodenversatz ist der gerade angewandte Aufschlag auf
 // die Routenhoehe, BodenversatzZiel der zuletzt gemessene.
 float Bodenversatz = 0.0f;
 float BodenversatzZiel = 0.0f;
 float NaechsteBodenpruefung = -1.0f;   // Weltzeit der naechsten Messung
 bool bBodenGemessen = false;           // erste Messung sofort, ohne Nachfuehren
 bool bSohleGesetzt = false;            // siehe Tick: Sohlenhoehe einmal nachgemessen
 // Sichtweiten-Abstufung (siehe Tick): weit weg wird die Figur unsichtbar,
 // tickt seltener und rechnet kein Ausweichen mehr. Bei 200 Figuren war
 // gerade das Ausweichen der Preistreiber - jede prueft jede.
 bool bNah = true;
 float Groesse = 1.72f;
 float BeinL = 0.0f;                // Beinlaenge in cm, aus Groesse abgeleitet
 float OberschenkelL = 0.0f, UnterschenkelL = 0.0f, OberarmL = 0.0f;
};
