#include "LaLaBergWaffe.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "LaLaBergFarbkugel.h"
#include "LaLaBergEinschlagblitz.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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
 // Runder Lauf mit weichem Rundungs-Schatten: die Ringpunkte der Mantelflaeche
 // sind zwischen benachbarten Feldern geteilt statt (wie bei kasten()) je
 // Flaeche neu angelegt, dadurch mitteln sich ihre Flaechennormalen in der
 // spaeteren Normalenberechnung zu einer runden Roehre statt zu einem
 // facettierten Vieleck - vorher sah jeder Lauf wie aus Pappe gefaltet aus.
 // R0/R1 erlauben eine leichte Verjuengung (Muendung schmaler als Wurzel).
 void zylinderGlatt(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
                    float x0, float x1, float R0, float R1, int32 Seiten, float dy = 0, float dz = 0,
                    bool KappeVorne = true, bool KappeHinten = false) {
  const int32 Basis = P.Num();
  for (int32 i = 0; i <= Seiten; i++) {
   const float A = 2 * PI * i / Seiten, cy = FMath::Cos(A), cz = FMath::Sin(A);
   P.Add(FVector(x0, cy * R0 + dy, cz * R0 + dz)); F.Add(Farbe);
   P.Add(FVector(x1, cy * R1 + dy, cz * R1 + dz)); F.Add(Farbe);
  }
  for (int32 i = 0; i < Seiten; i++) {
   const int32 a = Basis + i * 2, b = a + 2;
   K.Append({ a, a + 1, b + 1, a, b + 1, b });
  }
  if (KappeVorne) {
   const int32 mo = P.Num(); P.Add(FVector(x1, dy, dz)); F.Add(Farbe);
   for (int32 i = 0; i < Seiten; i++) K.Append({ mo, Basis + i * 2 + 1, Basis + (i + 1) * 2 + 1 });
  }
  if (KappeHinten) {
   const int32 mu = P.Num(); P.Add(FVector(x0, dy, dz)); F.Add(Farbe);
   for (int32 i = 0; i < Seiten; i++) K.Append({ mu, Basis + (i + 1) * 2, Basis + i * 2 });
  }
 }
 // Ein duenner rechteckiger Rahmen in der YZ-Ebene bei x - der Abzugsbuegel,
 // aus vier kasten()-Stegen statt einer Flaeche, damit die Mitte offen bleibt.
 void buegel(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FLinearColor& Farbe,
            float x, float y0, float z0, float y1, float z1, float Steg) {
  const float my = (y0 + y1) * 0.5f, mz = (z0 + z1) * 0.5f, hy = (y1 - y0) * 0.5f, hz = (z1 - z0) * 0.5f;
  kasten(P, K, F, Farbe, FVector(x, my, z0), FVector(Steg, hy, Steg * 0.5f));
  kasten(P, K, F, Farbe, FVector(x, my, z1), FVector(Steg, hy, Steg * 0.5f));
  kasten(P, K, F, Farbe, FVector(x, y0, mz), FVector(Steg, Steg * 0.5f, hz));
  kasten(P, K, F, Farbe, FVector(x, y1, mz), FVector(Steg, Steg * 0.5f, hz));
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
  float RueckstossGrad;  // Rueckstosskick des Laufs, in Grad
  float MuendungX;       // Muendungsort lokal auf der Laufachse, fuer Blitz und Sound
  float TonHoehe;        // Wiedergabetonhoehe des Schusssounds (Feuerrate/Kaliber-Gefuehl)
  const TCHAR* SoundName;
 };
 const FKennzahl& Kennzahl(ELaLaBergWaffenArt Art) {
  static const FKennzahl T[] = {
   /* Pistole       */ { 0.28f, 3200.f, 0.55f, 1, 0.6f,  1.4f,  9.f, 15.f, 3.2f, 22.5f, 1.05f, TEXT("SFX_Schuss_Pistole") },
   /* Maschine      */ { 0.09f, 3600.f, 0.55f, 1, 2.2f,  1.2f,  7.f, 12.f, 2.0f, 34.0f, 1.10f, TEXT("SFX_Schuss_Maschine") },
   /* Schrotflinte  */ { 0.75f, 2400.f, 0.60f, 7, 8.5f,  0.9f,  5.f,  9.f, 6.5f, 34.0f, 0.95f, TEXT("SFX_Schuss_Schrot") },
   /* Raketenwerfer */ { 1.40f, 1500.f, 0.35f, 1, 0.0f,  6.0f, 42.f, 58.f, 9.0f, 26.5f, 1.00f, TEXT("SFX_Schuss_Rakete") },
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
 const FLinearColor Griff(0.05f, 0.05f, 0.055f), Messing(0.55f, 0.42f, 0.10f);
 switch (Art) {
  case ELaLaBergWaffenArt::Pistole:
   kasten(P, K, F, Koerper, FVector(0, 0, 0), FVector(9.0f, 3.0f, 3.4f));                 // Rahmen
   kasten(P, K, F, Koerper, FVector(1.5f, 0, 4.2f), FVector(8.5f, 2.6f, 1.6f));           // Schlitten, schmaler und hoeher
   zylinderGlatt(P, K, F, Lauf, 8.5f, 22.5f, 1.15f, 1.0f, 12);                            // Lauf, leicht verjuengt
   kasten(P, K, F, Griff, FVector(-6.5f, 0, -6.0f), FVector(2.6f, 2.4f, 5.8f));           // Griff
   buegel(P, K, F, Koerper, 0.0f, -2.6f, -5.8f, 3.0f, -2.4f, 0.5f);                       // Abzugsbuegel
   kasten(P, K, F, Koerper, FVector(0.5f, 0, -3.4f), FVector(1.4f, 0.5f, 1.8f));          // Abzug
   kasten(P, K, F, Messing, FVector(-8.3f, 0, 5.3f), FVector(0.5f, 0.35f, 0.9f));         // Kimme
   kasten(P, K, F, Messing, FVector(9.6f, 0, 5.1f), FVector(0.5f, 0.30f, 0.7f));          // Korn
   break;
  case ELaLaBergWaffenArt::Maschine:
   kasten(P, K, F, Oliv, FVector(0, 0, 0), FVector(13.0f, 3.2f, 4.0f));                   // Gehaeuse
   kasten(P, K, F, Koerper, FVector(0, 0, 3.9f), FVector(12.5f, 2.4f, 1.1f));             // Schienenaufsatz
   zylinderGlatt(P, K, F, Lauf, 13.0f, 34.0f, 1.35f, 1.1f, 12);                           // Lauf
   kasten(P, K, F, Koerper, FVector(14.0f, 0, 1.6f), FVector(1.0f, 1.1f, 1.1f));          // Muendungsbremse
   kasten(P, K, F, Griff, FVector(-2.5f, 0, -6.2f), FVector(2.2f, 2.2f, 5.6f));           // Pistolengriff
   buegel(P, K, F, Oliv, -1.5f, -4.6f, -6.0f, 1.0f, -3.6f, 0.5f);                         // Abzugsbuegel
   kasten(P, K, F, Koerper, FVector(-2.0f, 0, -6.5f), FVector(2.4f, 2.2f, 6.5f));         // Magazin
   kasten(P, K, F, Oliv, FVector(-15.5f, 0, -0.5f), FVector(3.5f, 1.5f, 1.6f));           // Schaft
   kasten(P, K, F, Oliv, FVector(-19.0f, 0, 1.6f), FVector(0.9f, 1.5f, 1.5f));            // Schulterplatte
   kasten(P, K, F, Messing, FVector(8.5f, 0, 5.2f), FVector(0.4f, 0.3f, 0.8f));           // Visier
   break;
  case ELaLaBergWaffenArt::Schrotflinte:
   kasten(P, K, F, Oliv, FVector(-1, 0, 0), FVector(10.0f, 3.6f, 4.6f));                  // Gehaeuse
   zylinderGlatt(P, K, F, Lauf, 10.0f, 34.0f, 1.55f, 1.35f, 12, 1.75f, 0);                // Doppellauf
   zylinderGlatt(P, K, F, Lauf, 10.0f, 34.0f, 1.55f, 1.35f, 12, -1.75f, 0);
   kasten(P, K, F, Koerper, FVector(6.0f, 0, -2.6f), FVector(6.5f, 3.2f, 1.3f));          // Vorderschaft (Pumpe)
   kasten(P, K, F, Koerper, FVector(-8.0f, 0, -5.8f), FVector(1.8f, 1.9f, 4.4f));         // Pistolengriff
   buegel(P, K, F, Oliv, -8.0f, -4.4f, -6.2f, -1.6f, -4.6f, 0.5f);                        // Abzugsbuegel
   kasten(P, K, F, Griff, FVector(-19.0f, 0, 1.5f), FVector(7.5f, 3.0f, 3.6f));           // Schaft
   break;
  case ELaLaBergWaffenArt::Raketenwerfer:
   zylinderGlatt(P, K, F, Rohr, -22.0f, 24.0f, 6.5f, 5.8f, 16, 0, 0, false, false);       // Rohr, ohne Deckel
   zylinderGlatt(P, K, F, Rohr, -30.0f, -22.0f, 7.6f, 6.5f, 16, 0, 0, false, true);       // Trichter hinten
   zylinderGlatt(P, K, F, Koerper, 24.0f, 26.5f, 6.7f, 6.7f, 16, 0, 0, true, false);      // Muendungsrand
   kasten(P, K, F, Oliv, FVector(-4.0f, 0, 7.5f), FVector(4.5f, 2.2f, 3.4f));             // Visiereinheit
   kasten(P, K, F, Messing, FVector(-4.0f, 0, 10.7f), FVector(0.4f, 0.3f, 0.9f));         // Visierstift
   kasten(P, K, F, Griff, FVector(-9.0f, 0, -3.5f), FVector(2.0f, 2.0f, 5.0f));           // Abzugsgriff
   buegel(P, K, F, Oliv, -9.0f, -3.6f, -7.6f, -1.0f, -5.6f, 0.5f);                        // Abzugsbuegel
   kasten(P, K, F, Koerper, FVector(-24.0f, 0, 0), FVector(3.4f, 3.4f, 3.4f));            // Schulterkappe
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
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Waffenmetall.M_Waffenmetall"))) Netz->SetMaterial(0, M);
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
 // Rueckstoss klingt zuegig ab - der Lauf soll sichtbar hochschlagen und
 // gleich wieder auf Ziellinie sein, nicht lange nachwackeln.
 if (RueckstossGrad > 0.001f) {
  RueckstossGrad = FMath::FInterpTo(RueckstossGrad, 0.0f, Zeit, 14.0f);
  Netz->SetRelativeRotation(FRotator(RueckstossGrad, 0, 0));
 }
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

 // Rueckstoss, Sound und Muendungsblitz - dieselbe Kennzahl wie die
 // Ballistik gibt auch hier den Ausschlag je Waffenart vor.
 RueckstossGrad = FMath::Min(K.RueckstossGrad * 2.2f, RueckstossGrad + K.RueckstossGrad);
 if (auto* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Audio/%s.%s"), K.SoundName, K.SoundName)))
  UGameplayStatics::PlaySoundAtLocation(this, Sound, Ort, 1.0f, K.TonHoehe * FMath::FRandRange(0.97f, 1.03f));
 const FVector MuendungOrt = Netz->GetComponentTransform().TransformPosition(FVector(K.MuendungX, 0, 0));
 if (auto* Blitz = GetWorld()->SpawnActor<ALaLaBergEinschlagblitz>(MuendungOrt, FRotator::ZeroRotator))
  Blitz->Einrichten(FLinearColor(1.0f, 0.86f, 0.55f), 6000.0f, 320.0f, 0.06f);

 SchussZahl++;
 return true;
}
