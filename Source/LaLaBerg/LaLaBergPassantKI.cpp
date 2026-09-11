#include "LaLaBergPassantKI.h"
#include "Components/CapsuleComponent.h"
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
 // Ein Kasten, Sichtseiten nach aussen - derselbe Aufbau wie in LaLaBergWaffe.cpp.
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
 // Landsberger Strassenbild in Kleidung: gedeckte Hosen, kraeftigere Jacken.
 const FLinearColor HOSEN[] = { FLinearColor(0.17f,0.19f,0.22f), FLinearColor(0.23f,0.24f,0.26f), FLinearColor(0.29f,0.24f,0.20f) };
 const FLinearColor JACKEN[] = { FLinearColor(0.55f,0.23f,0.20f), FLinearColor(0.18f,0.29f,0.36f), FLinearColor(0.24f,0.35f,0.25f),
                                 FLinearColor(0.71f,0.65f,0.55f), FLinearColor(0.22f,0.23f,0.26f), FLinearColor(0.43f,0.30f,0.48f) };
 const FLinearColor HAUT(0.79f, 0.63f, 0.51f);
}

ALaLaBergPassantKI::ALaLaBergPassantKI() {
 PrimaryActorTick.bCanEverTick = true;
 Huelle = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Huelle"));
 Huelle->InitCapsuleSize(28.0f, 86.0f);
 Huelle->SetRelativeLocation(FVector(0, 0, 86));
 Huelle->SetCollisionProfileName(TEXT("BlockAllDynamic"));
 SetRootComponent(Huelle);

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Netz"));
 Netz->SetupAttachment(Huelle);
 Netz->SetRelativeLocation(FVector(0, 0, -86));
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 const float T = FMath::FRand();
 Jacke = JACKEN[FMath::RandRange(0, UE_ARRAY_COUNT(JACKEN) - 1)];
 Groesse = 1.60f + T * 0.24f;
}

void ALaLaBergPassantKI::BaueModell() {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const FLinearColor Hose = HOSEN[FMath::RandRange(0, UE_ARRAY_COUNT(HOSEN) - 1)];
 const float BeinL = Groesse * 46.0f;
 for (float Seite : {-1.0f, 1.0f})
  kasten(P, K, F, Hose, FVector(0, Seite * 10.5f, BeinL * 0.5f), FVector(8.5f, 8.0f, BeinL * 0.5f));
 const float RumpfOben = BeinL + Groesse * 46.0f;
 kasten(P, K, F, Jacke, FVector(0, 0, (BeinL + RumpfOben) * 0.5f), FVector(15.0f, 11.0f, (RumpfOben - BeinL) * 0.5f));
 for (float Seite : {-1.0f, 1.0f})
  kasten(P, K, F, Jacke, FVector(0, Seite * 20.0f, (BeinL + RumpfOben) * 0.5f - 4.0f),
         FVector(6.0f, 6.0f, (RumpfOben - BeinL) * 0.42f));
 kasten(P, K, F, HAUT, FVector(0, 0, RumpfOben + Groesse * 9.0f), FVector(9.0f, 9.5f, Groesse * 9.0f));

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 Normalen.Init(FVector::ZeroVector, P.Num());
 for (int32 i = 0; i + 2 < K.Num(); i += 3) {
  const FVector N = FVector::CrossProduct(P[K[i + 2]] - P[K[i]], P[K[i + 1]] - P[K[i]]);
  Normalen[K[i]] += N; Normalen[K[i + 1]] += N; Normalen[K[i + 2]] += N;
 }
 for (FVector& N : Normalen) N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
 for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 60.0, P[i].Y / 60.0));
 Netz->ClearAllMeshSections();
 Netz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) Netz->SetMaterial(0, M);
}

void ALaLaBergPassantKI::SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh) {
 Weg.Route = Punkte;
 Tempo = TempoKmh / 3.6f;
 if (Weg.Gueltig()) SetActorLocation(Weg.Start());
}

void ALaLaBergPassantKI::BeginPlay() {
 Super::BeginPlay();
 BaueModell();
}

void ALaLaBergPassantKI::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Weg.Gueltig()) return;
 const bool bStolpert = GetWorld()->GetTimeSeconds() < StolpertBis;
 FVector Ort = GetActorLocation();
 const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * (bStolpert ? 0.15f : 1.0f));
 SetActorLocation(Ort);
 if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 4.0f));
}

// ILaLaBergFarbbar: die Jacke faerbt sich um, ein Treffer bringt kurz aus
// dem Tritt - mehr Reaktion ist ohne Skelett und Animation nicht redlich
// zu versprechen.
void ALaLaBergPassantKI::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 Jacke = Farbe;
 BaueModell();
 StolpertBis = GetWorld()->GetTimeSeconds() + 1.6f;
}
