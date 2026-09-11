#include "LaLaBergAmpel.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"

namespace {
 void hinzu(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
           const FVector& A, const FVector& B, const FVector& C, const FVector& D) {
  const int32 i = P.Num();
  P.Append({ A, B, C, D }); F.Append({ Farbe, Farbe, Farbe, Farbe });
  K.Append({ i, i + 1, i + 2, i, i + 2, i + 3 });
 }
 void kasten(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
            const FVector& Mitte, const FVector& Halb) {
  const FVector M = Mitte, H = Halb;
  const auto E = [&](float x, float y, float z) { return M + FVector(x * H.X, y * H.Y, z * H.Z); };
  hinzu(P, K, F, Farbe, E(1, -1, -1), E(1, 1, -1), E(1, 1, 1), E(1, -1, 1));
  hinzu(P, K, F, Farbe, E(-1, 1, -1), E(-1, -1, -1), E(-1, -1, 1), E(-1, 1, 1));
  hinzu(P, K, F, Farbe, E(1, 1, -1), E(-1, 1, -1), E(-1, 1, 1), E(1, 1, 1));
  hinzu(P, K, F, Farbe, E(-1, -1, -1), E(1, -1, -1), E(1, -1, 1), E(-1, -1, 1));
  hinzu(P, K, F, Farbe, E(-1, -1, 1), E(1, -1, 1), E(1, 1, 1), E(-1, 1, 1));
  hinzu(P, K, F, Farbe, E(1, -1, -1), E(-1, -1, -1), E(-1, 1, -1), E(1, 1, -1));
 }
 void zylinder(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
              float z0, float z1, float R, int32 Seiten) {
  TArray<FVector> Unten, Oben;
  for (int32 i = 0; i < Seiten; i++) {
   const float A = 2 * PI * i / Seiten, x = FMath::Cos(A) * R, y = FMath::Sin(A) * R;
   Unten.Add(FVector(x, y, z0)); Oben.Add(FVector(x, y, z1));
  }
  for (int32 i = 0; i < Seiten; i++) { const int32 j = (i + 1) % Seiten; hinzu(P, K, F, Farbe, Unten[i], Unten[j], Oben[j], Oben[i]); }
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
 // Nicht alle im Gleichtakt: eigener Zeitversatz je Ampel.
 Versatz = FMath::FRandRange(0.0f, ROT_S + GRUEN_S + GELB_S);
 Zustand = 0; Bis = GetWorld()->GetTimeSeconds() + Versatz + ROT_S;
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
 kasten(P, K, F, FLinearColor(0.08f, 0.08f, 0.09f), FVector(0, 0, 330.0f), FVector(9.0f, 9.0f, 32.0f));
 const FLinearColor Farben[3] = { Zustand == 0 ? ROT_HELL : DUNKEL, Zustand == 1 ? GRUEN_HELL : DUNKEL, Zustand == 2 ? GELB_HELL : DUNKEL };
 const float ZHoehen[3] = { 352.0f, 330.0f, 308.0f };
 for (int32 i = 0; i < 3; i++) kasten(P, K, F, Farben[i], FVector(9.2f, 0, ZHoehen[i]), FVector(0.6f, 6.0f, 6.0f));

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
