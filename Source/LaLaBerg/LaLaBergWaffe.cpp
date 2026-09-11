#include "LaLaBergWaffe.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "LaLaBergFarbkugel.h"
#include "Engine/World.h"

namespace {
 // Kasten und Zylinder, Sichtseiten nach aussen, eigene Eckpunkte je Flaeche
 // (scharfe Kanten) - derselbe Aufbau wie die Karosserie in LaLaBergWagen.cpp.
 void hinzu(TArray<FVector>& Punkte, TArray<int32>& Kanten, TArray<FLinearColor>& Farben, const FLinearColor& F,
           const FVector& A, const FVector& B, const FVector& C, const FVector& D) {
  const int32 i = Punkte.Num();
  Punkte.Append({ A, B, C, D }); Farben.Append({ F, F, F, F });
  Kanten.Append({ i, i + 1, i + 2, i, i + 2, i + 3 });
 }
 void kasten(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
            const FVector& Mitte, const FVector& Halb) {
  const FVector M = Mitte, H = Halb;
  const auto E = [&](float x, float y, float z) { return M + FVector(x * H.X, y * H.Y, z * H.Z); };
  hinzu(P, K, F, Farbe, E(1, -1, -1), E(1, 1, -1), E(1, 1, 1), E(1, -1, 1));       // +X
  hinzu(P, K, F, Farbe, E(-1, 1, -1), E(-1, -1, -1), E(-1, -1, 1), E(-1, 1, 1));   // -X
  hinzu(P, K, F, Farbe, E(1, 1, -1), E(-1, 1, -1), E(-1, 1, 1), E(1, 1, 1));       // +Y
  hinzu(P, K, F, Farbe, E(-1, -1, -1), E(1, -1, -1), E(1, -1, 1), E(-1, -1, 1));   // -Y
  hinzu(P, K, F, Farbe, E(-1, -1, 1), E(1, -1, 1), E(1, 1, 1), E(-1, 1, 1));       // +Z
  hinzu(P, K, F, Farbe, E(1, -1, -1), E(-1, -1, -1), E(-1, 1, -1), E(1, 1, -1));   // -Z
 }
 // Zylinder, Achse entlang lokal X, Basis bei x0, Deckel bei x1, quer um dy/dz versetzt.
 void zylinder(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
              float x0, float x1, float R, int32 Seiten, float dy = 0, float dz = 0) {
  TArray<FVector> Unten, Oben;
  for (int32 i = 0; i < Seiten; i++) {
   const float A = 2 * PI * i / Seiten, y = FMath::Cos(A) * R + dy, z = FMath::Sin(A) * R + dz;
   Unten.Add(FVector(x0, y, z)); Oben.Add(FVector(x1, y, z));
  }
  for (int32 i = 0; i < Seiten; i++) {
   const int32 j = (i + 1) % Seiten;
   hinzu(P, K, F, Farbe, Unten[i], Unten[j], Oben[j], Oben[i]);
  }
  const int32 mu = P.Num(); P.Add(FVector(x0, dy, dz)); F.Add(Farbe);
  for (int32 i = 0; i < Seiten; i++) { const int32 j = (i + 1) % Seiten; K.Append({ mu, mu + 1 + j, mu + 1 + i }); P.Add(Unten[j]); F.Add(Farbe); }
  const int32 mo = P.Num(); P.Add(FVector(x1, dy, dz)); F.Add(Farbe);
  for (int32 i = 0; i < Seiten; i++) { const int32 j = (i + 1) % Seiten; K.Append({ mo, mo + 1 + i, mo + 1 + j }); P.Add(Oben[i]); F.Add(Farbe); }
 }
 // Die kraeftigen Farben eines Paintballs - Fuellfarbe pro Kugel, nicht die
 // Markerfarbe (die bleibt neutrales Grau/Gelb/Oliv).
 const FLinearColor KUGELFARBEN[] = {
  FLinearColor(0.92f, 0.05f, 0.35f), FLinearColor(0.95f, 0.75f, 0.05f), FLinearColor(0.10f, 0.70f, 0.25f),
  FLinearColor(0.10f, 0.45f, 0.90f), FLinearColor(0.95f, 0.35f, 0.05f), FLinearColor(0.70f, 0.10f, 0.85f),
 };

 // Eine Kennzahl je Waffenart - so bleibt die Feuerlogik fuer alle vier
 // gleich, nur die Zahlen dahinter unterscheiden sich.
 struct FKennzahl {
  float Feuerrate;       // Sekunden zwischen zwei Schuessen (Schrotflinte: zwischen zwei Salven)
  float Tempo;           // Muendungsgeschwindigkeit, cm/s
  float Schwerkraft;     // Schwerkraftfaktor des Projektils
  int32 KugelnJeSchuss;
  float StreuungGrad;    // Kegeloeffnung um die Zielrichtung
  float KugelRadius;
  float KleckMin, KleckMax;
 };
 const FKennzahl& Kennzahl(ELaLaBergWaffenArt Art) {
  static const FKennzahl T[] = {
   /* Pistole       */ { 0.28f, 3200.f, 0.55f, 1, 0.6f,  1.4f,  9.f, 15.f },
   /* Maschine      */ { 0.09f, 3600.f, 0.55f, 1, 2.2f,  1.2f,  7.f, 12.f },
   /* Schrotflinte  */ { 0.75f, 2400.f, 0.60f, 7, 8.5f,  0.9f,  5.f,  9.f },
   /* Raketenwerfer */ { 1.40f, 1500.f, 0.35f, 1, 0.0f,  6.0f, 42.f, 58.f },
  };
  return T[static_cast<uint8>(Art)];
 }
}

ALaLaBergWaffe::ALaLaBergWaffe() {
 PrimaryActorTick.bCanEverTick = true;
 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Netz"));
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Netz->SetCastShadow(false);          // ein Ansichtsmodell wirft keinen Schatten in die eigene Kamera
 SetRootComponent(Netz);
}

FString ALaLaBergWaffe::ArtName() const {
 switch (Art) {
  case ELaLaBergWaffenArt::Pistole: return TEXT("Paintball-Pistole");
  case ELaLaBergWaffenArt::Maschine: return TEXT("Paintball-MP");
  case ELaLaBergWaffenArt::Schrotflinte: return TEXT("Paintball-Schrotflinte");
  case ELaLaBergWaffenArt::Raketenwerfer: return TEXT("Paintball-Werfer");
  default: return TEXT("?");
 }
}

void ALaLaBergWaffe::SetzeArt(ELaLaBergWaffenArt Neu) {
 if (Art == Neu && Netz->GetNumSections() > 0) return;
 Art = Neu;
 if (HasActorBegunPlay()) BaueModell();
}

// Vier Silhouetten aus denselben Bausteinen (Kasten, Zylinder): eine
// kompakte Pistole, eine MP mit Magazin und langem Lauf, eine Schrotflinte
// mit zwei Laeufen und Schaft, ein schultergestuetzter Werfer mit weitem
// Rohr. Niemand fasst das Modell an - die Silhouette muss nur erkennbar
// unterschiedlich sein.
void ALaLaBergWaffe::BaueModell() {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const FLinearColor Koerper(0.07f, 0.075f, 0.08f), Lauf(0.04f, 0.04f, 0.045f), Trichter(0.85f, 0.72f, 0.08f),
                    Oliv(0.20f, 0.22f, 0.14f), Rohr(0.16f, 0.17f, 0.15f);
 switch (Art) {
  case ELaLaBergWaffenArt::Pistole:
   kasten(P, K, F, Koerper, FVector(0, 0, 0), FVector(9, 3.2f, 4.2f));
   zylinder(P, K, F, Lauf, 9.0f, 23.0f, 1.3f, 10);
   kasten(P, K, F, Trichter, FVector(-1, 0, 8.5f), FVector(3.4f, 3.0f, 4.4f));
   zylinder(P, K, F, Koerper, -12.0f, -3.0f, 2.1f, 8);
   break;
  case ELaLaBergWaffenArt::Maschine:
   kasten(P, K, F, Oliv, FVector(0, 0, 0), FVector(13, 3.4f, 4.6f));
   zylinder(P, K, F, Lauf, 13.0f, 34.0f, 1.4f, 10);
   kasten(P, K, F, Trichter, FVector(0, 0, 9.0f), FVector(3.2f, 2.8f, 4.2f));
   kasten(P, K, F, Koerper, FVector(-2, 0, -6.5f), FVector(2.6f, 2.4f, 6.5f));           // Magazin
   kasten(P, K, F, Oliv, FVector(-15, 0, -1), FVector(4.0f, 1.6f, 1.8f));                // Schulterstuetze
   break;
  case ELaLaBergWaffenArt::Schrotflinte:
   kasten(P, K, F, Oliv, FVector(0, 0, 0), FVector(11, 3.6f, 5.0f));
   zylinder(P, K, F, Lauf, 11.0f, 33.0f, 1.6f, 10, 1.7f, 0);
   zylinder(P, K, F, Lauf, 11.0f, 33.0f, 1.6f, 10, -1.7f, 0);
   kasten(P, K, F, Koerper, FVector(-16, 0, 1), FVector(6.0f, 2.6f, 3.4f));               // Schaft
   break;
  case ELaLaBergWaffenArt::Raketenwerfer:
   zylinder(P, K, F, Rohr, -22.0f, 26.0f, 6.5f, 14);
   kasten(P, K, F, Oliv, FVector(-6, 0, 6.5f), FVector(5.0f, 3.4f, 2.2f));                // Griff/Visier
   kasten(P, K, F, Koerper, FVector(-20, 0, 0), FVector(3.6f, 3.6f, 3.6f));               // Schulterkappe
   break;
 }

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 Normalen.Init(FVector::ZeroVector, P.Num());
 for (int32 i = 0; i + 2 < K.Num(); i += 3) {
  const FVector N = FVector::CrossProduct(P[K[i + 2]] - P[K[i]], P[K[i + 1]] - P[K[i]]);
  Normalen[K[i]] += N; Normalen[K[i + 1]] += N; Normalen[K[i + 2]] += N;
 }
 for (FVector& N : Normalen) N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
 for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 40.0, P[i].Y / 40.0));
 Netz->ClearAllMeshSections();
 Netz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) Netz->SetMaterial(0, M);
}

void ALaLaBergWaffe::BeginPlay() {
 Super::BeginPlay();
 BaueModell();
}

// Folgt der Sichtbarkeit des Halters (Owner): steigt die Figur in den Wagen
// und wird ausgeblendet, verschwindet der Marker mit ihr, ohne dass Wagen
// oder Charakter voneinander wissen muessten.
void ALaLaBergWaffe::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (const AActor* Halter = GetOwner()) SetActorHiddenInGame(Halter->IsHidden());
}

bool ALaLaBergWaffe::Feuern(const FVector& Ort, const FVector& Richtung) {
 const FKennzahl& K = Kennzahl(Art);
 const float Jetzt = GetWorld()->GetTimeSeconds();
 if (Jetzt - LetzterSchuss < K.Feuerrate) return false;
 LetzterSchuss = Jetzt;
 // Die Kugeln einer Salve (Schrotflinte) entstehen im selben Bild dicht
 // nebeneinander und sollen sich nicht gegenseitig treffen.
 TArray<ALaLaBergFarbkugel*> Salve;
 for (int32 i = 0; i < K.KugelnJeSchuss; i++) {
  const FVector Kegel = K.StreuungGrad > 0.0f ? FMath::VRandCone(Richtung, FMath::DegreesToRadians(K.StreuungGrad)) : Richtung;
  // Verzoegert spawnen: Farbe und Groesse muessen stehen, bevor BeginPlay
  // die Kugel aus ihnen aufbaut - sonst waere jede Kugel zunaechst die
  // magentafarbene Grundeinstellung gewesen.
  if (auto* Kugel = GetWorld()->SpawnActorDeferred<ALaLaBergFarbkugel>(ALaLaBergFarbkugel::StaticClass(),
      FTransform(Kegel.Rotation(), Ort), GetOwner(), GetInstigator(), ESpawnActorCollisionHandlingMethod::AlwaysSpawn)) {
   Kugel->Einrichten(KUGELFARBEN[FMath::RandRange(0, UE_ARRAY_COUNT(KUGELFARBEN) - 1)],
                     K.KugelRadius, K.KleckMin, K.KleckMax, K.Schwerkraft);
   Kugel->FinishSpawning(FTransform(Kegel.Rotation(), Ort));
   Kugel->Abschiessen(Kegel, K.Tempo);
   Salve.Add(Kugel);
  }
 }
 for (ALaLaBergFarbkugel* A : Salve) for (ALaLaBergFarbkugel* B : Salve) if (A != B) A->IgnoriereGeschwister(B);
 SchussZahl++;
 return true;
}
