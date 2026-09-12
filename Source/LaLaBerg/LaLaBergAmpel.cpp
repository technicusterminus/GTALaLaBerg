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
 // Linker-/DLL-Interface-Konflikt meldet (C2487).
 constexpr float ROT_S = 7.0f, GRUEN_S = 6.0f, GELB_S = 1.6f;
 constexpr float ZYKLUS_S = ROT_S + GRUEN_S + GELB_S;

 // Zustand und Restzeit fuer einen Zeitpunkt "t" innerhalb eines Zyklus -
 // dieselbe Rot->Gruen->Gelb-Abfolge, aber aus der Uhrzeit berechnet statt
 // schrittweise gezaehlt. So lassen sich zwei Ampeln exakt synchron (gleiche
 // Gruppe, gleiche Phase) oder exakt gegenphasig (Phase 1 = Phase 0 + halber
 // Zyklus) halten, ohne dass eine die andere kennen muesste.
 void ZustandBei(float t, int32& Zustand, float& Rest) {
  t = FMath::Fmod(t, ZYKLUS_S); if (t < 0) t += ZYKLUS_S;
  if (t < ROT_S) { Zustand = 0; Rest = ROT_S - t; return; }
  t -= ROT_S;
  if (t < GRUEN_S) { Zustand = 1; Rest = GRUEN_S - t; return; }
  t -= GRUEN_S;
  Zustand = 2; Rest = GELB_S - t;
 }
}

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
 // Zykluszeit, bleibt aber fuer jede Ampel derselben Kreuzung gleich.
 const float GruppenVersatz = FMath::Fmod(Gruppe * 7.31f, ZYKLUS_S);
 // Phase 1 laeuft exakt gegenphasig zu Phase 0 derselben Kreuzung - so hat
 // eine kreuzende Fahrbahnachse nie gleichzeitig Gruen.
 const float PhasenVersatz = Phase == 1 ? ZYKLUS_S * 0.5f : 0.0f;
 float Rest;
 ZustandBei(GetWorld()->GetTimeSeconds() + GruppenVersatz + PhasenVersatz, Zustand, Rest);
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
 // Rot -> Gruen -> Gelb -> Rot, wie eine gewoehnliche Ampel.
 const float Dauer[3] = { ROT_S, GRUEN_S, GELB_S };
 Zustand = (Zustand + 1) % 3;
 Bis = GetWorld()->GetTimeSeconds() + Dauer[Zustand];
 BaueKopf();
}
