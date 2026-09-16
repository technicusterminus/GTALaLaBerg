#include "LaLaBergAmpel.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "LaLaBergKoerperTeile.h"

namespace {
 // hinzu()/kasten() kommen aus LaLaBergKoerperTeile.h (voll qualifiziert,
 // kein "using namespace" - siehe LaLaBergCharacter.cpp fuer den Grund:
 // anonyme Namespaces sind pro Uebersetzungseinheit vereinigt, nicht pro
 // Datei, ein zweites lokales "kasten()" hier kollidierte im Unity-Build
 // mit dem gleichnamigen aus LaLaBergWaffe.cpp).
 void zylinder(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
              float z0, float z1, float R, int32 Seiten) {
  TArray<FVector> Unten, Oben;
  for (int32 i = 0; i < Seiten; i++) {
   const float A = 2 * PI * i / Seiten, x = FMath::Cos(A) * R, y = FMath::Sin(A) * R;
   Unten.Add(FVector(x, y, z0)); Oben.Add(FVector(x, y, z1));
  }
  for (int32 i = 0; i < Seiten; i++) { const int32 j = (i + 1) % Seiten; LaLaBergKoerperTeile::hinzu(P, K, F, Farbe, Unten[i], Unten[j], Oben[j], Oben[i]); }
 }
 // Lit/unlit statt echtem Leuchten (kein Emissive-Material in diesem
 // Projekt) - satt vs. stumpf grau, so bleibt der Zustand auch bei Tageslicht
 // klar erkennbar.
 const FLinearColor DUNKEL(0.12f, 0.12f, 0.13f);
 const FLinearColor ROT_HELL(0.95f, 0.08f, 0.05f), GELB_HELL(0.95f, 0.75f, 0.05f), GRUEN_HELL(0.10f, 0.85f, 0.20f);
 // Schaltzeiten in Sekunden - ausserhalb der UCLASS, weil MSVC bei statischen
 // constexpr-Feldern in einer DLL-Schnittstellenklasse (LALABERG_API) einen
 // Linker-/DLL-Interface-Konflikt meldet (C2487). GRUEN_S+GELB_S ist das
 // Zeitfenster genau einer Phase; der volle Zyklus ist so viele Fenster
 // hintereinander, wie die Kreuzung Phasen hat (siehe ZustandBei) - bei den
 // ueblichen zwei Phasen (vier Kreuzungsarme) exakt dieselbe Aufteilung wie
 // zuvor mit dem festen Halbzyklus-Versatz, nur allgemein fuer beliebig
 // viele Phasen statt nur zwei.
 constexpr float GRUEN_S = 6.0f, GELB_S = 1.6f;
 constexpr float PHASENFENSTER_S = GRUEN_S + GELB_S;

 // Zustand und Restzeit fuer einen Zeitpunkt "t" innerhalb des Zyklus einer
 // Kreuzung mit "AnzahlPhasen" Phasen, aus Sicht der Phase "MeinePhase" -
 // aus der Uhrzeit berechnet statt schrittweise gezaehlt, damit alle Ampeln
 // derselben Kreuzung exakt im Takt bleiben, ohne einander zu kennen. Jede
 // Phase bekommt ihr eigenes, nicht ueberlappendes Zeitfenster im Zyklus
 // (PHASENFENSTER_S lang, beginnend bei MeinePhase*PHASENFENSTER_S) - so
 // zeigt nie mehr als eine Phase gleichzeitig Gruen, unabhaengig davon, wie
 // viele Phasen die Kreuzung hat.
 void ZustandBei(float t, int32 MeinePhase, int32 AnzahlPhasen, int32& Zustand, float& Rest) {
  const float Zyklus = PHASENFENSTER_S * AnzahlPhasen;
  t = FMath::Fmod(t, Zyklus); if (t < 0) t += Zyklus;
  float Rel = FMath::Fmod(t - MeinePhase * PHASENFENSTER_S, Zyklus); if (Rel < 0) Rel += Zyklus;
  if (Rel < GRUEN_S) { Zustand = 1; Rest = GRUEN_S - Rel; return; }
  if (Rel < PHASENFENSTER_S) { Zustand = 2; Rest = PHASENFENSTER_S - Rel; return; }
  Zustand = 0; Rest = Zyklus - Rel;
 }
}

float ALaLaBergAmpel::PhasenfensterS() { return PHASENFENSTER_S; }

ALaLaBergAmpel::ALaLaBergAmpel() {
 PrimaryActorTick.bCanEverTick = true;
 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Netz"));
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 SetRootComponent(Netz);
}

TArray<ALaLaBergAmpel*> ALaLaBergAmpel::Alle;

void ALaLaBergAmpel::BeginPlay() {
 Super::BeginPlay();
 // Kreuzungen sollen nicht alle im Gleichtakt schalten - ein fester, aus der
 // Gruppen-ID abgeleiteter Versatz (statt Zufall) verteilt sie ueber die
 // Zykluszeit, bleibt aber fuer jede Ampel derselben Kreuzung gleich. Die
 // eigene Phase bekommt ihr Zeitfenster direkt in ZustandBei - kein
 // zusaetzlicher Versatz hier mehr noetig, das deckt auch mehr als zwei
 // Phasen ab (siehe dort).
 const float GruppenVersatz = FMath::Fmod(Gruppe * 7.31f, PHASENFENSTER_S * AnzahlPhasen);
 float Rest;
 ZustandBei(GetWorld()->GetTimeSeconds() + GruppenVersatz, Phase, AnzahlPhasen, Zustand, Rest);
 Bis = GetWorld()->GetTimeSeconds() + Rest;
 BaueKopf();
 Alle.Add(this);
}

void ALaLaBergAmpel::EndPlay(const EEndPlayReason::Type Grund) {
 Alle.RemoveSingleSwap(this);
 Super::EndPlay(Grund);
}

// Mast, Kasten, drei Linsen - eine leuchtet, je nach Zustand.
void ALaLaBergAmpel::BaueKopf() {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const FLinearColor Grau(0.30f, 0.30f, 0.32f);
 zylinder(P, K, F, Grau, 0.0f, 300.0f, 6.0f, 10);
 LaLaBergKoerperTeile::kasten(P, K, F, FLinearColor(0.08f, 0.08f, 0.09f), FVector(0, 0, 330.0f), FVector(9.0f, 9.0f, 32.0f));
 const FLinearColor Farben[3] = { Zustand == 0 ? ROT_HELL : DUNKEL, Zustand == 1 ? GRUEN_HELL : DUNKEL, Zustand == 2 ? GELB_HELL : DUNKEL };
 const float ZHoehen[3] = { 352.0f, 330.0f, 308.0f };
 for (int32 i = 0; i < 3; i++) LaLaBergKoerperTeile::kasten(P, K, F, Farben[i], FVector(9.2f, 0, ZHoehen[i]), FVector(0.6f, 6.0f, 6.0f));

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 Normalen.Init(FVector::ZeroVector, P.Num());
 for (int32 i = 0; i + 2 < K.Num(); i += 3) {
  const FVector N = FVector::CrossProduct(P[K[i + 2]] - P[K[i]], P[K[i + 1]] - P[K[i]]);
  Normalen[K[i]] += N; Normalen[K[i + 1]] += N; Normalen[K[i + 2]] += N;
 }
 for (FVector& N : Normalen) N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
 for (const FVector& Pt : P) UVs.Add(FVector2D(Pt.X / 40.0, Pt.Y / 40.0));
 Netz->ClearAllMeshSections();
 Netz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Lack.M_Lack"))) Netz->SetMaterial(0, M);
}

void ALaLaBergAmpel::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (GetWorld()->GetTimeSeconds() < Bis) return;
 // Rot -> Gruen -> Gelb -> Rot, wie eine gewoehnliche Ampel. Die Rot-Dauer
 // ist nicht mehr fest: sie ist der Rest des Zyklus, den die uebrigen
 // Phasen belegen (AnzahlPhasen-1 Zeitfenster) - bei den ueblichen zwei
 // Phasen genau ein Fenster lang, wie zuvor mit festem ROT_S.
 const float Dauer[3] = { PHASENFENSTER_S * FMath::Max(1, AnzahlPhasen - 1), GRUEN_S, GELB_S };
 Zustand = (Zustand + 1) % 3;
 Bis = GetWorld()->GetTimeSeconds() + Dauer[Zustand];
 BaueKopf();
}
