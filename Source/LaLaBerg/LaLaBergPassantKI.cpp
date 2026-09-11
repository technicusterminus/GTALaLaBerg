#include "LaLaBergPassantKI.h"
#include "Components/CapsuleComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "EngineUtils.h"

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
 void normalen(const TArray<FVector>& P, const TArray<int32>& K, TArray<FVector>& N) {
  N.Init(FVector::ZeroVector, P.Num());
  for (int32 i = 0; i + 2 < K.Num(); i += 3) {
   const FVector Fl = FVector::CrossProduct(P[K[i + 2]] - P[K[i]], P[K[i + 1]] - P[K[i]]);
   N[K[i]] += Fl; N[K[i + 1]] += Fl; N[K[i + 2]] += Fl;
  }
  for (FVector& X : N) X = X.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
 }
 // Landsberger Strassenbild in Kleidung: gedeckte Hosen, kraeftigere Jacken.
 const FLinearColor HOSEN[] = { FLinearColor(0.17f,0.19f,0.22f), FLinearColor(0.23f,0.24f,0.26f), FLinearColor(0.29f,0.24f,0.20f) };
 const FLinearColor JACKEN[] = { FLinearColor(0.55f,0.23f,0.20f), FLinearColor(0.18f,0.29f,0.36f), FLinearColor(0.24f,0.35f,0.25f),
                                 FLinearColor(0.71f,0.65f,0.55f), FLinearColor(0.22f,0.23f,0.26f), FLinearColor(0.43f,0.30f,0.48f) };
 const FLinearColor HAUT(0.79f, 0.63f, 0.51f);
 constexpr float SCHRITTWEITE = 11.0f;     // cm, wie weit ein Bein vor/zurueck schwingt

 // Keine echte Menschenmenge - nur: bremsen, wenn ein anderer Passant naeher
 // als 2,5 m voraus steht, damit zwei Figuren nicht sichtbar ineinander
 // hineinlaufen. Ausweichen zur Seite gibt es damit weiterhin nicht.
 float BremseVorPassant(UWorld* Welt, const AActor* Selbst, const FVector& Ort, const FVector& Vorwaerts) {
  float Bremse = 1.0f;
  for (TActorIterator<ALaLaBergPassantKI> It(Welt); It; ++It) {
   if (*It == Selbst) continue;
   const FVector Diff = It->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > 250.0f || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) continue;
   Bremse = FMath::Min(Bremse, FMath::Clamp((Dist - 70.0f) / 180.0f, 0.05f, 1.0f));
  }
  return Bremse;
 }
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
 Hose = HOSEN[FMath::RandRange(0, UE_ARRAY_COUNT(HOSEN) - 1)];
 Groesse = 1.60f + T * 0.24f;
 BeinL = Groesse * 46.0f;
 Gehphase = FMath::FRandRange(0.0f, 6.28f);    // nicht alle im Gleichschritt
}

// Kopf, Hals, Rumpf, Arme - alles, was beim Gehen nicht schwingt.
void ALaLaBergPassantKI::BaueOberkoerper() {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const float RumpfOben = BeinL + Groesse * 46.0f;
 kasten(P, K, F, Jacke, FVector(0, 0, (BeinL + RumpfOben) * 0.5f), FVector(15.0f, 11.0f, (RumpfOben - BeinL) * 0.5f));
 for (float Seite : {-1.0f, 1.0f})
  kasten(P, K, F, Jacke, FVector(0, Seite * 20.0f, (BeinL + RumpfOben) * 0.5f - 4.0f),
         FVector(6.0f, 6.0f, (RumpfOben - BeinL) * 0.42f));
 // Hals: schmaler als der Kopf, in Hautfarbe - vermeidet den Eindruck, der
 // Kopf sei nur lose auf den Rumpf gesetzt.
 const float HalsOben = RumpfOben + Groesse * 4.0f;
 kasten(P, K, F, HAUT, FVector(0, 0, (RumpfOben + HalsOben) * 0.5f), FVector(5.0f, 5.0f, (HalsOben - RumpfOben) * 0.5f + 0.5f));
 kasten(P, K, F, HAUT, FVector(0, 0, HalsOben + Groesse * 9.0f), FVector(9.0f, 9.5f, Groesse * 9.0f));

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 normalen(P, K, Normalen);
 for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 60.0, P[i].Y / 60.0));
 Netz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) Netz->SetMaterial(0, M);
}

// Zwei Beinkaesten, je Bild neu am Fusspunkt der Schrittbewegung. Immer
// dieselbe Anzahl Eckpunkte in derselben Reihenfolge, damit UpdateMeshSection
// (statt eines vollen Neuaufbaus) reicht.
void ALaLaBergPassantKI::AktualisiereBeine(float Phase) {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const float Schwung = FMath::Sin(Phase) * SCHRITTWEITE;
 for (float Seite : {-1.0f, 1.0f}) {
  // Ein Bein vorn, das andere hinten - im Gegentakt zueinander.
  const float U = Seite > 0 ? Schwung : -Schwung;
  kasten(P, K, F, Hose, FVector(U, Seite * 10.5f, BeinL * 0.5f), FVector(8.5f, 8.0f, BeinL * 0.5f));
 }
 if (Netz->GetNumSections() > 1) {
  TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
  normalen(P, K, Normalen);
  for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 60.0, P[i].Y / 60.0));
  Netz->UpdateMeshSection_LinearColor(1, P, Normalen, UVs, F, Tangenten);
 } else {
  TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
  normalen(P, K, Normalen);
  for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 60.0, P[i].Y / 60.0));
  Netz->CreateMeshSection_LinearColor(1, P, K, Normalen, UVs, F, Tangenten, false);
  if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) Netz->SetMaterial(1, M);
 }
}

void ALaLaBergPassantKI::SetzeRoute(const TArray<FVector>& Punkte, float TempoKmh) {
 Weg.Route = Punkte;
 Tempo = TempoKmh / 3.6f;
 if (Weg.Gueltig()) SetActorLocation(Weg.Start());
}

void ALaLaBergPassantKI::BeginPlay() {
 Super::BeginPlay();
 BaueOberkoerper();
 AktualisiereBeine(Gehphase);
}

void ALaLaBergPassantKI::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Weg.Gueltig()) return;
 const bool bStolpert = GetWorld()->GetTimeSeconds() < StolpertBis;
 FVector Ort = GetActorLocation();
 const float Bremse = BremseVorPassant(GetWorld(), this, Ort, GetActorForwardVector());
 const float Faktor = (bStolpert ? 0.15f : 1.0f) * Bremse;
 const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * Faktor);
 SetActorLocation(Ort);
 if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 4.0f));
 // Schrittfrequenz an das tatsaechliche Tempo gekoppelt: schneller gehen
 // heisst schneller schwingende Beine, nicht nur schnellere Fuesse ueber
 // denselben trägen Takt.
 Gehphase += Zeit * Faktor * (Tempo / 45.0f);
 AktualisiereBeine(Gehphase);
 // Wippen im Schritt: zweimal je Schwingung, wie ein echter Gang.
 const float Bob = FMath::Abs(FMath::Sin(Gehphase)) * 3.0f;
 Netz->SetRelativeLocation(FVector(0, 0, -86.0f + Bob));
}

// ILaLaBergFarbbar: die Jacke faerbt sich um, ein Treffer bringt kurz aus
// dem Tritt - mehr Reaktion ist ohne Skelett und Animation nicht redlich
// zu versprechen.
void ALaLaBergPassantKI::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 Jacke = Farbe;
 BaueOberkoerper();
 StolpertBis = GetWorld()->GetTimeSeconds() + 1.6f;
}
