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

 UPROPERTY() TObjectPtr<class UCapsuleComponent> Huelle = nullptr;
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Netz = nullptr;         // Kopf, Hals, Rumpf - unbewegt

 // Je Seite: 0 = rechts, 1 = links.
 UPROPERTY() TObjectPtr<class USceneComponent> Huefte[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Oberschenkel[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> Knie[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Unterschenkel[2] = {};
 UPROPERTY() TObjectPtr<class USceneComponent> Schulter[2] = {};
 UPROPERTY() TObjectPtr<class UProceduralMeshComponent> Oberarm[2] = {};

 FLaLaBergWegfolger Weg;
 float Tempo = 140.0f;             // cm/s, gewoehnliches Gehtempo
 float StolpertBis = -10.0f;       // ein Treffer bremst kurz
 float Seitversatz = 0.0f;         // seitliches Ausweichen, siehe Tick
 float Gehphase = 0.0f;
 FLinearColor Jacke;
 float Groesse = 1.72f;
 float BeinL = 0.0f;                // Beinlaenge in cm, aus Groesse abgeleitet
 float OberschenkelL = 0.0f, UnterschenkelL = 0.0f, OberarmL = 0.0f;
};
