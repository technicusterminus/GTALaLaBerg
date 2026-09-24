#include "LaLaBergWaffe.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "LaLaBergFarbkugel.h"
#include "LaLaBergKonto.h"
#include "LaLaBergEinschlagblitz.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "LaLaBergKoerperTeile.h"

namespace {
 // hinzu()/kasten() kommen aus LaLaBergKoerperTeile.h (voll qualifiziert,
 // kein "using namespace": anonyme Namespaces sind pro Uebersetzungseinheit
 // vereinigt, nicht pro Datei - ein zweites, gleichnamiges "kasten()" in
 // einer im Unity-Build mitgebuendelten Datei fuehrte sonst zu einem
 // Definitionskonflikt, siehe LaLaBergCharacter.cpp).
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
  LaLaBergKoerperTeile::kasten(P, K, F, Farbe, FVector(x, my, z0), FVector(Steg, hy, Steg * 0.5f));
  LaLaBergKoerperTeile::kasten(P, K, F, Farbe, FVector(x, my, z1), FVector(Steg, hy, Steg * 0.5f));
  LaLaBergKoerperTeile::kasten(P, K, F, Farbe, FVector(x, y0, mz), FVector(Steg, Steg * 0.5f, hz));
  LaLaBergKoerperTeile::kasten(P, K, F, Farbe, FVector(x, y1, mz), FVector(Steg, Steg * 0.5f, hz));
 }
 // Zwei Kaesten am Griff: eine Hand darum, ein Unterarm schraeg nach hinten-
 // unten zur angenommenen Schulter. Kein Skelett-Mesh im Projekt (siehe
 // README) - ohne diese Kaesten schwebte die Waffe frei vor der Kamera, als
 // haette niemand sie in der Hand.
 void haende(TArray<FVector>& P, TArray<int32>& K, TArray<FLinearColor>& F, const FVector& GriffMitte) {
  const FLinearColor Haut(0.82f, 0.62f, 0.50f), Aermel(0.24f, 0.27f, 0.31f);
  LaLaBergKoerperTeile::kasten(P, K, F, Haut, GriffMitte + FVector(0.3f, 0, -0.8f), FVector(2.1f, 2.3f, 2.7f));
  LaLaBergKoerperTeile::kasten(P, K, F, Aermel, GriffMitte + FVector(-6.5f, 0, -6.2f), FVector(4.8f, 2.5f, 2.5f));
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

 // Das lizenzierte Ansichtsmodell je Waffenart (CC0, Quaternius - siehe
 // Content/SourceData/Waffen/LIZENZ.md). Skalierung und Versatz von Hand
 // abgeglichen: die Rohmodelle sind rund 2.5x groesser als die vorherigen
 // Kaesten. Der Quaternius-glTF-Export zeigt mit seiner eigenen Vorderseite
 // nach lokal -X; Wurzel (und die Kamera dahinter) erwarten +X als "nach
 // vorn" (siehe Kasten/Zylinder-Bauweise weiter unten) - ohne die 180-Grad-
 // Drehung um Z zeigte die Muendung zum Spieler statt von ihm weg.
 struct FModell {
  const TCHAR* Pfad;
  FVector Versatz;
  FRotator Drehung;
  float Skalierung;
  // Wo die Hand die Waffe haelt, in Koordinaten des Waffen-Actors (nach
  // Versatz, Drehung und Skalierung). Die Figur setzt die Waffe so, dass
  // dieser Punkt in ihrer Handflaeche liegt (siehe ALaLaBergCharacter::
  // Tick). Abgeleitet aus den gemessenen Bounding Boxes der Rohmeshes
  // (Tools/pruefe_waffenmasse.py, z.B. Pistole X -68..11, Z -12.7..35.5):
  // Griff im hinteren Viertel, unterhalb des Laufs - per
  // -LaLaBergKoerperFoto im Bild nachkontrolliert. Die Versatz-Werte oben
  // stammen noch aus der Zeit, als die Waffe vor der Kamera schwebte; ohne
  // diesen Griffpunkt lag die Pistole 23 bis 54 cm vor der Hand.
  FVector Griff;
 };
 const FModell& ModellInfo(ELaLaBergWaffenArt Art) {
  static const FModell M[] = {
   /* Pistole       */ { TEXT("/Game/Art/Waffen/Pistol/StaticMeshes/SM_Pistole.SM_Pistole"),
                          FVector(27.0f, 0, -1.0f), FRotator(0, 180, 0), 0.40f, FVector(29.0f, 0, 2.0f) },
   /* Maschine      */ { TEXT("/Game/Art/Waffen/Smg/StaticMeshes/SM_Maschine.SM_Maschine"),
                          FVector(40.0f, 0, -6.0f), FRotator(0, 180, 0), 0.40f, FVector(48.0f, 0, -6.0f) },
   /* Schrotflinte  */ { TEXT("/Game/Art/Waffen/Shotgun/StaticMeshes/SM_Schrotflinte.SM_Schrotflinte"),
                          FVector(48.0f, 0, -4.0f), FRotator(0, 180, 0), 0.40f, FVector(51.5f, 0, -6.0f) },
   // Deutlich weiter vorn als die anderen drei: die Rohmesh-Bounding-Box
   // (siehe Tools/pruefe_waffenmasse.py) ist mit Y=46.9/Z=74.6 fast viermal
   // so breit wie die Pistole - bei gleichem Versatz fuellte die Muendung,
   // fast am Kameraclip, den ganzen Bildschirm mit einer einzelnen grauen
   // Flaeche (im Test bestaetigt: LALABERG_WAFFENTEST-Screenshot).
   /* Raketenwerfer */ { TEXT("/Game/Art/Waffen/RocketLauncher/StaticMeshes/SM_Raketenwerfer.SM_Raketenwerfer"),
                          FVector(75.0f, 14.0f, -22.0f), FRotator(0, 180, 0), 0.40f, FVector(80.0f, 14.0f, -22.0f) },
  };
  return M[static_cast<uint8>(Art)];
 }
}

ALaLaBergWaffe::ALaLaBergWaffe() {
 PrimaryActorTick.bCanEverTick = true;
 // Eigene, unrotierte Wurzel: die Figur setzt per SetActorRelativeTransform()
 // den Kamera-Versatz einmal in BeginPlay - der Rueckstoss darf diese Basis
 // nicht ueberschreiben, sondern dreht Netz/NetzEcht obendrauf (siehe Tick()).
 Wurzel = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
 SetRootComponent(Wurzel);

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Netz"));
 Netz->SetupAttachment(Wurzel);
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Netz->SetCastShadow(false);          // ein Ansichtsmodell wirft keinen Schatten in die eigene Kamera

 NetzEcht = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NetzEcht"));
 NetzEcht->SetupAttachment(Wurzel);
 NetzEcht->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 NetzEcht->SetCastShadow(false);
 NetzEcht->SetVisibility(false);
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
 const bool bSchonGebaut = (NetzEcht && NetzEcht->GetStaticMesh()) || (Netz && Netz->GetNumSections() > 0);
 if (Art == Neu && bSchonGebaut) return;
 Art = Neu;
 if (HasActorBegunPlay()) BaueModell();
}

FVector ALaLaBergWaffe::GriffOrt() const {
 if (NetzEcht && NetzEcht->IsVisible()) return ModellInfo(Art).Griff;
 // Kasten-Fallback: dieselben Griffkaesten wie in BaueModell unten.
 switch (Art) {
  case ELaLaBergWaffenArt::Pistole:       return FVector(-6.5f, 0, -6.0f);
  case ELaLaBergWaffenArt::Maschine:      return FVector(-2.5f, 0, -6.2f);
  case ELaLaBergWaffenArt::Schrotflinte:  return FVector(-8.0f, 0, -5.8f);
  case ELaLaBergWaffenArt::Raketenwerfer: return FVector(-9.0f, 0, -3.5f);
 }
 return FVector::ZeroVector;
}

// Bevorzugt das lizenzierte GLB-Modell (siehe ModellInfo) - nur wenn das
// fehlt (z.B. frischer Checkout ohne Content/SourceData/Waffen), faellt die
// Waffe auf die alte, von Hand gebaute Silhouette aus Kaesten und Zylindern
// zurueck, dieselbe wie zuvor, samt Hand und Unterarm am Griff.
void ALaLaBergWaffe::BaueModell() {
 const FModell& M = ModellInfo(Art);
 if (auto* Mesh = LoadObject<UStaticMesh>(nullptr, M.Pfad)) {
  ModellDrehung = M.Drehung;
  NetzEcht->SetStaticMesh(Mesh);
  NetzEcht->SetRelativeLocation(M.Versatz);
  NetzEcht->SetRelativeRotation(M.Drehung);
  NetzEcht->SetRelativeScale3D(FVector(M.Skalierung));
  NetzEcht->SetVisibility(true);
  Netz->SetVisibility(false);
  Netz->ClearAllMeshSections();
  return;
 }
 ModellDrehung = FRotator::ZeroRotator;
 NetzEcht->SetVisibility(false);
 Netz->SetVisibility(true);

 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const FLinearColor Koerper(0.07f, 0.075f, 0.08f), Lauf(0.04f, 0.04f, 0.045f), Trichter(0.85f, 0.72f, 0.08f),
                    Oliv(0.20f, 0.22f, 0.14f), Rohr(0.16f, 0.17f, 0.15f);
 const FLinearColor Griff(0.05f, 0.05f, 0.055f), Messing(0.55f, 0.42f, 0.10f);
 switch (Art) {
  case ELaLaBergWaffenArt::Pistole:
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(0, 0, 0), FVector(9.0f, 3.0f, 3.4f));                 // Rahmen
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(1.5f, 0, 4.2f), FVector(8.5f, 2.6f, 1.6f));           // Schlitten, schmaler und hoeher
   zylinderGlatt(P, K, F, Lauf, 8.5f, 22.5f, 1.15f, 1.0f, 12);                            // Lauf, leicht verjuengt
   LaLaBergKoerperTeile::kasten(P, K, F, Griff, FVector(-6.5f, 0, -6.0f), FVector(2.6f, 2.4f, 5.8f));           // Griff
   buegel(P, K, F, Koerper, 0.0f, -2.6f, -5.8f, 3.0f, -2.4f, 0.5f);                       // Abzugsbuegel
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(0.5f, 0, -3.4f), FVector(1.4f, 0.5f, 1.8f));          // Abzug
   LaLaBergKoerperTeile::kasten(P, K, F, Messing, FVector(-8.3f, 0, 5.3f), FVector(0.5f, 0.35f, 0.9f));         // Kimme
   LaLaBergKoerperTeile::kasten(P, K, F, Messing, FVector(9.6f, 0, 5.1f), FVector(0.5f, 0.30f, 0.7f));          // Korn
   haende(P, K, F, FVector(-6.5f, 0, -6.0f));
   break;
  case ELaLaBergWaffenArt::Maschine:
   LaLaBergKoerperTeile::kasten(P, K, F, Oliv, FVector(0, 0, 0), FVector(13.0f, 3.2f, 4.0f));                   // Gehaeuse
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(0, 0, 3.9f), FVector(12.5f, 2.4f, 1.1f));             // Schienenaufsatz
   zylinderGlatt(P, K, F, Lauf, 13.0f, 34.0f, 1.35f, 1.1f, 12);                           // Lauf
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(14.0f, 0, 1.6f), FVector(1.0f, 1.1f, 1.1f));          // Muendungsbremse
   LaLaBergKoerperTeile::kasten(P, K, F, Griff, FVector(-2.5f, 0, -6.2f), FVector(2.2f, 2.2f, 5.6f));           // Pistolengriff
   buegel(P, K, F, Oliv, -1.5f, -4.6f, -6.0f, 1.0f, -3.6f, 0.5f);                         // Abzugsbuegel
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(-2.0f, 0, -6.5f), FVector(2.4f, 2.2f, 6.5f));         // Magazin
   LaLaBergKoerperTeile::kasten(P, K, F, Oliv, FVector(-15.5f, 0, -0.5f), FVector(3.5f, 1.5f, 1.6f));           // Schaft
   LaLaBergKoerperTeile::kasten(P, K, F, Oliv, FVector(-19.0f, 0, 1.6f), FVector(0.9f, 1.5f, 1.5f));            // Schulterplatte
   LaLaBergKoerperTeile::kasten(P, K, F, Messing, FVector(8.5f, 0, 5.2f), FVector(0.4f, 0.3f, 0.8f));           // Visier
   haende(P, K, F, FVector(-2.5f, 0, -6.2f));
   break;
  case ELaLaBergWaffenArt::Schrotflinte:
   LaLaBergKoerperTeile::kasten(P, K, F, Oliv, FVector(-1, 0, 0), FVector(10.0f, 3.6f, 4.6f));                  // Gehaeuse
   zylinderGlatt(P, K, F, Lauf, 10.0f, 34.0f, 1.55f, 1.35f, 12, 1.75f, 0);                // Doppellauf
   zylinderGlatt(P, K, F, Lauf, 10.0f, 34.0f, 1.55f, 1.35f, 12, -1.75f, 0);
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(6.0f, 0, -2.6f), FVector(6.5f, 3.2f, 1.3f));          // Vorderschaft (Pumpe)
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(-8.0f, 0, -5.8f), FVector(1.8f, 1.9f, 4.4f));         // Pistolengriff
   buegel(P, K, F, Oliv, -8.0f, -4.4f, -6.2f, -1.6f, -4.6f, 0.5f);                        // Abzugsbuegel
   LaLaBergKoerperTeile::kasten(P, K, F, Griff, FVector(-19.0f, 0, 1.5f), FVector(7.5f, 3.0f, 3.6f));           // Schaft
   haende(P, K, F, FVector(-8.0f, 0, -5.8f));
   break;
  case ELaLaBergWaffenArt::Raketenwerfer:
   zylinderGlatt(P, K, F, Rohr, -22.0f, 24.0f, 6.5f, 5.8f, 16, 0, 0, false, false);       // Rohr, ohne Deckel
   zylinderGlatt(P, K, F, Rohr, -30.0f, -22.0f, 7.6f, 6.5f, 16, 0, 0, false, true);       // Trichter hinten
   zylinderGlatt(P, K, F, Koerper, 24.0f, 26.5f, 6.7f, 6.7f, 16, 0, 0, true, false);      // Muendungsrand
   LaLaBergKoerperTeile::kasten(P, K, F, Oliv, FVector(-4.0f, 0, 7.5f), FVector(4.5f, 2.2f, 3.4f));             // Visiereinheit
   LaLaBergKoerperTeile::kasten(P, K, F, Messing, FVector(-4.0f, 0, 10.7f), FVector(0.4f, 0.3f, 0.9f));         // Visierstift
   LaLaBergKoerperTeile::kasten(P, K, F, Griff, FVector(-9.0f, 0, -3.5f), FVector(2.0f, 2.0f, 5.0f));           // Abzugsgriff
   buegel(P, K, F, Oliv, -9.0f, -3.6f, -7.6f, -1.0f, -5.6f, 0.5f);                        // Abzugsbuegel
   LaLaBergKoerperTeile::kasten(P, K, F, Koerper, FVector(-24.0f, 0, 0), FVector(3.4f, 3.4f, 3.4f));            // Schulterkappe
   haende(P, K, F, FVector(-9.0f, 0, -3.5f));
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
 if (auto* Metall = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Waffenmetall.M_Waffenmetall"))) Netz->SetMaterial(0, Metall);
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
 // Auf Netz UND NetzEcht - beide sitzen unrotiert an Wurzel, welche Figur
 // per SetActorRelativeTransform() den Kamera-Versatz traegt (siehe
 // BeginPlay in LaLaBergCharacter.cpp). Wuerde der Rueckstoss stattdessen
 // Wurzel selbst drehen, ueberschriebe er diesen Versatz bei jedem Schuss.
 if (RueckstossGrad > 0.001f) {
  RueckstossGrad = FMath::FInterpTo(RueckstossGrad, 0.0f, Zeit, 14.0f);
  const FRotator Kick = FRotator(RueckstossGrad, 0, 0) + ModellDrehung;
  Netz->SetRelativeRotation(Kick);
  NetzEcht->SetRelativeRotation(Kick);
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
  // Zielsicherheit halbiert die Streuung, wenn sie ausgebaut ist.
  float Streuung = K.StreuungGrad;
  if (const auto* Konto = ULaLaBergKonto::Hole(this))
   Streuung *= 1.0f - 0.5f * Konto->Anteil(ULaLaBergKonto::EWert::Zielsicherheit);
  const FVector Kegel = Streuung > 0.0f ? FMath::VRandCone(Richtung, FMath::DegreesToRadians(Streuung)) : Richtung;
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
 // Ueber Wurzel statt Netz/NetzEcht: welches der beiden gerade sichtbar ist,
 // haengt vom geladenen Modell ab (siehe BaueModell), die Muendung liegt in
 // beiden Faellen ungefaehr denselben Abstand vor der Wurzel.
 const FVector MuendungOrt = Wurzel->GetComponentLocation() + Wurzel->GetForwardVector() * K.MuendungX;
 if (auto* Blitz = GetWorld()->SpawnActor<ALaLaBergEinschlagblitz>(MuendungOrt, FRotator::ZeroRotator))
  Blitz->Einrichten(FLinearColor(1.0f, 0.86f, 0.55f), 6000.0f, 320.0f, 0.06f);

 SchussZahl++;
 return true;
}
