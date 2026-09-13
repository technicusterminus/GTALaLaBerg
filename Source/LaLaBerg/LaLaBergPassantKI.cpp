#include "LaLaBergPassantKI.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "LaLaBergKoerperTeile.h"

namespace {
 // Kein "using namespace" hier: anonyme Namespaces sind pro Uebersetzungs-
 // einheit vereinigt, nicht pro Datei - im Unity-Build wirkt eine using-
 // Deklaration sonst in andere .cpp-Dateien mit eigenem "kasten()" hinein
 // und macht deren Aufrufe mehrdeutig (siehe LaLaBergCharacter.cpp).
 // Landsberger Strassenbild in Kleidung: gedeckte Hosen, kraeftigere Jacken.
 const FLinearColor HOSEN[] = { FLinearColor(0.17f,0.19f,0.22f), FLinearColor(0.23f,0.24f,0.26f), FLinearColor(0.29f,0.24f,0.20f) };
 const FLinearColor JACKEN[] = { FLinearColor(0.55f,0.23f,0.20f), FLinearColor(0.18f,0.29f,0.36f), FLinearColor(0.24f,0.35f,0.25f),
                                 FLinearColor(0.71f,0.65f,0.55f), FLinearColor(0.22f,0.23f,0.26f), FLinearColor(0.43f,0.30f,0.48f) };
 const FLinearColor HAUT(0.79f, 0.63f, 0.51f);
 // Gelenkwinkel in Grad: wie weit Huefte/Knie/Schulter im Schritt ausschlagen.
 constexpr float PASSANT_HUEFT_GRAD = 22.0f, PASSANT_KNIE_GRAD = 38.0f, PASSANT_SCHULTER_GRAD = 16.0f;

 // Bremst KI-Passanten vor einem anderen KI-Passanten voraus, damit zwei
 // Figuren nicht sichtbar ineinander hineinlaufen - kein Ausweichen zur
 // Seite, nur ein Anhalten.
 float BremseVorPassant(const AActor* Selbst, const FVector& Ort, const FVector& Vorwaerts) {
  float Bremse = 1.0f;
  for (ALaLaBergPassantKI* Andere : ALaLaBergPassantKI::Alle) {
   if (!Andere || Andere == Selbst) continue;
   const FVector Diff = Andere->GetActorLocation() - Ort;
   const float Dist = Diff.Size();
   if (Dist > 250.0f || Dist < 1.0f) continue;
   if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) continue;
   Bremse = FMath::Min(Bremse, FMath::Clamp((Dist - 70.0f) / 180.0f, 0.05f, 1.0f));
  }
  return Bremse;
 }
 // Bremst vor dem Spieler genau wie vor einem anderen Passanten - bislang
 // liefen Figuren dem Spieler ungebremst hinterher bzw. durch ihn hindurch.
 float BremseVorSpieler(const UWorld* Welt, const FVector& Ort, const FVector& Vorwaerts) {
  const APawn* SpielerPawn = UGameplayStatics::GetPlayerPawn(Welt, 0);
  if (!SpielerPawn) return 1.0f;
  const FVector Diff = SpielerPawn->GetActorLocation() - Ort;
  const float Dist = Diff.Size();
  if (Dist > 250.0f || Dist < 1.0f) return 1.0f;
  if (FVector::DotProduct(Diff / Dist, Vorwaerts) < 0.5f) return 1.0f;
  return FMath::Clamp((Dist - 70.0f) / 180.0f, 0.05f, 1.0f);
 }
 // Seitlicher Versatz zum Ausweichen, kleiner als bei Autos (siehe
 // LaLaBergVerkehrsauto.cpp) - ein Gehweg bietet weniger Platz.
 constexpr float MAX_SEITVERSATZ = 55.0f;
}

TArray<ALaLaBergPassantKI*> ALaLaBergPassantKI::Alle;

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
 BeinL = Groesse * 46.0f;
 OberschenkelL = BeinL * 0.52f;
 UnterschenkelL = BeinL * 0.48f;
 OberarmL = Groesse * 30.0f;
 Gehphase = FMath::FRandRange(0.0f, 6.28f);    // nicht alle im Gleichschritt

 // Huefte/Knie: Ursprung des Kindglieds liegt am Gelenk selbst, das Bein
 // haengt lokal nach unten - eine Drehung des Gelenks biegt es wie an einem
 // Scharnier, ohne dass ein Netz je neu aufgebaut werden muss.
 const float HueftZ = BeinL - 86.0f;
 const float SchulterZ = (BeinL + Groesse * 46.0f) - 86.0f - Groesse * 6.0f;
 for (int32 s = 0; s < 2; s++) {
  const float Seite = s == 0 ? 1.0f : -1.0f;
  Huefte[s] = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Huefte%d"), s));
  Huefte[s]->SetupAttachment(Huelle);
  Huefte[s]->SetRelativeLocation(FVector(0, Seite * 10.5f, HueftZ));

  Oberschenkel[s] = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Oberschenkel%d"), s));
  Oberschenkel[s]->SetupAttachment(Huefte[s]);
  Oberschenkel[s]->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  Knie[s] = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Knie%d"), s));
  Knie[s]->SetupAttachment(Huefte[s]);
  Knie[s]->SetRelativeLocation(FVector(0, 0, -OberschenkelL));

  Unterschenkel[s] = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Unterschenkel%d"), s));
  Unterschenkel[s]->SetupAttachment(Knie[s]);
  Unterschenkel[s]->SetCollisionEnabled(ECollisionEnabled::NoCollision);

  Schulter[s] = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Schulter%d"), s));
  Schulter[s]->SetupAttachment(Huelle);
  Schulter[s]->SetRelativeLocation(FVector(0, Seite * 20.0f, SchulterZ));

  Oberarm[s] = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Oberarm%d"), s));
  Oberarm[s]->SetupAttachment(Schulter[s]);
  Oberarm[s]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }

 // Skeletal-Mesh-Alternative zum Kasten-Rig oben (siehe BeginPlay) - vier
 // Teile desselben modularen Pakets, am Boden (Huelle-relativ -86, wie
 // Netz) statt in Huelle-Mitte, weil das importierte Skelett seine eigene
 // Bodenreferenz mitbringt.
 SkelettKoerper = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelettKoerper"));
 SkelettKoerper->SetupAttachment(Huelle);
 SkelettKoerper->SetRelativeLocation(FVector(0, 0, -86));
 SkelettKoerper->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 SkelettKoerper->SetVisibility(false);
 // Dieselbe Bodenversatz -86 wie SkelettKoerper: SetLeaderPoseComponent
 // uebernimmt nur die Knochen-Transforms, nicht die eigene Komponenten-
 // Transform - ohne diesen Versatz schwebte der Kopf um 86 Einheiten zu
 // hoch ueber dem Koerper (im Test bestaetigt, deutlich sichtbar).
 SkelettKopf = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelettKopf"));
 SkelettKopf->SetupAttachment(Huelle);
 SkelettKopf->SetRelativeLocation(FVector(0, 0, -86));
 SkelettKopf->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 SkelettKopf->SetVisibility(false);
 SkelettFuesse = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelettFuesse"));
 SkelettFuesse->SetupAttachment(Huelle);
 SkelettFuesse->SetRelativeLocation(FVector(0, 0, -86));
 SkelettFuesse->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 SkelettFuesse->SetVisibility(false);
 SkelettBeine = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkelettBeine"));
 SkelettBeine->SetupAttachment(Huelle);
 SkelettBeine->SetRelativeLocation(FVector(0, 0, -86));
 SkelettBeine->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 SkelettBeine->SetVisibility(false);
}

// Ein Kasten, dessen Ursprung oben am Gelenk liegt und der um "Laenge" nach
// unten reicht - der Baustein fuer jedes Glied des Rigs.
void ALaLaBergPassantKI::BaueGlied(UProceduralMeshComponent* GliedNetz, const FLinearColor& Farbe, float HalbBreite, float Laenge) {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 LaLaBergKoerperTeile::kasten(P, K, F, Farbe, FVector(0, 0, -Laenge * 0.5f), FVector(HalbBreite, HalbBreite, Laenge * 0.5f));
 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 LaLaBergKoerperTeile::normalen(P, K, Normalen);
 for (const FVector& Pt : P) UVs.Add(FVector2D(Pt.X / 40.0, Pt.Y / 40.0));
 GliedNetz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) GliedNetz->SetMaterial(0, M);
}

// Kopf, Hals, Rumpf - unbewegt, nur die Jackenfarbe kann sich neu bauen.
void ALaLaBergPassantKI::BaueOberkoerper() {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const float RumpfOben = BeinL + Groesse * 46.0f;
 LaLaBergKoerperTeile::kasten(P, K, F, Jacke, FVector(0, 0, (BeinL + RumpfOben) * 0.5f), FVector(15.0f, 11.0f, (RumpfOben - BeinL) * 0.5f));
 const float HalsOben = RumpfOben + Groesse * 4.0f;
 LaLaBergKoerperTeile::kasten(P, K, F, HAUT, FVector(0, 0, (RumpfOben + HalsOben) * 0.5f), FVector(5.0f, 5.0f, (HalsOben - RumpfOben) * 0.5f + 0.5f));
 LaLaBergKoerperTeile::kasten(P, K, F, HAUT, FVector(0, 0, HalsOben + Groesse * 9.0f), FVector(9.0f, 9.5f, Groesse * 9.0f));

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 LaLaBergKoerperTeile::normalen(P, K, Normalen);
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

// Bevorzugt das lizenzierte Skeletal Mesh (siehe Header) - alle vier Teile
// muessen laden, sonst bleibt es konsistent beim Kasten-Rig statt eine
// Figur halb echt, halb Kasten zusammenzusetzen.
namespace {
 USkeletalMesh* LadeNPCTeil(const TCHAR* Name) {
  return LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Art/People/Farmer/SK_Farmer_%s.SK_Farmer_%s"), Name, Name));
 }
}

void ALaLaBergPassantKI::BeginPlay() {
 Super::BeginPlay();

 USkeletalMesh* MeshKoerper = LadeNPCTeil(TEXT("Body"));
 USkeletalMesh* MeshKopf = LadeNPCTeil(TEXT("Head"));
 USkeletalMesh* MeshFuesse = LadeNPCTeil(TEXT("Feet"));
 USkeletalMesh* MeshBeine = LadeNPCTeil(TEXT("Legs"));
 if (MeshKoerper && MeshKopf && MeshFuesse && MeshBeine) {
  bSkelettGenutzt = true;
  SkelettKoerper->SetSkeletalMesh(MeshKoerper);
  SkelettKopf->SetSkeletalMesh(MeshKopf);
  SkelettFuesse->SetSkeletalMesh(MeshFuesse);
  SkelettBeine->SetSkeletalMesh(MeshBeine);
  // Kopf/Fuesse/Beine folgen der Pose von SkelettKoerper, statt selbst eine
  // AnimSequence abzuspielen - vier Teile, eine Animation.
  SkelettKopf->SetLeaderPoseComponent(SkelettKoerper);
  SkelettFuesse->SetLeaderPoseComponent(SkelettKoerper);
  SkelettBeine->SetLeaderPoseComponent(SkelettKoerper);
  for (USkeletalMeshComponent* Teil : { SkelettKoerper, SkelettKopf, SkelettFuesse, SkelettBeine }) Teil->SetVisibility(true);
  Netz->SetVisibility(false);
  for (int32 s = 0; s < 2; s++) {
   Oberschenkel[s]->SetVisibility(false); Unterschenkel[s]->SetVisibility(false); Oberarm[s]->SetVisibility(false);
  }
  if (auto* Anim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_Idle_Neutral.Anim_HumansCharacterArmature_Idle_Neutral")))
   SkelettKoerper->PlayAnimation(Anim, true);
 } else {
  BaueOberkoerper();
  const FLinearColor Hose = HOSEN[FMath::RandRange(0, UE_ARRAY_COUNT(HOSEN) - 1)];
  for (int32 s = 0; s < 2; s++) {
   BaueGlied(Oberschenkel[s], Hose, 8.0f, OberschenkelL);
   BaueGlied(Unterschenkel[s], Hose, 7.0f, UnterschenkelL);
   BaueGlied(Oberarm[s], Jacke, 5.5f, OberarmL);
  }
 }
 Alle.Add(this);
}

void ALaLaBergPassantKI::EndPlay(const EEndPlayReason::Type Grund) {
 Alle.RemoveSingleSwap(this);
 Super::EndPlay(Grund);
}

void ALaLaBergPassantKI::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Weg.Gueltig()) return;
 const bool bStolpert = GetWorld()->GetTimeSeconds() < StolpertBis;
 FVector Ort = GetActorLocation();
 const FVector Vorwaerts = GetActorForwardVector();
 const float Bremse = FMath::Min(BremseVorPassant(this, Ort, Vorwaerts), BremseVorSpieler(GetWorld(), Ort, Vorwaerts));
 const float Faktor = (bStolpert ? 0.15f : 1.0f) * Bremse;
 const FVector Richtung = Weg.Bewege(Ort, Tempo * Zeit * Faktor);
 SetActorLocation(Ort);
 if (!Richtung.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(), Richtung.Rotation(), Zeit, 4.0f));

 // Weicht einer Person oder dem Spieler direkt voraus seitlich aus, statt
 // nur davor stehenzubleiben - sanft ein- und wieder ausgeblendet.
 const float SeitZiel = Bremse < 0.9f ? MAX_SEITVERSATZ : 0.0f;
 Seitversatz = FMath::FInterpTo(Seitversatz, SeitZiel, Zeit, 0.7f);
 if (FMath::Abs(Seitversatz) > 0.5f) SetActorLocation(GetActorLocation() + GetActorRightVector() * Seitversatz);

 if (bSkelettGenutzt) {
  // Echte Animation statt Gelenkwinkel von Hand: nur beim Wechsel zwischen
  // Stehen und Gehen die AnimSequence tauschen, nicht jedes Bild neu
  // abspielen (das rissee sie sonst staendig auf Bild 0 zurueck).
  const bool bLaeuftJetzt = Faktor > 0.05f;
  if (bLaeuftJetzt != bLaeuftGerade) {
   bLaeuftGerade = bLaeuftJetzt;
   const TCHAR* Pfad = bLaeuftGerade
    ? TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_Walk.Anim_HumansCharacterArmature_Walk")
    : TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_Idle_Neutral.Anim_HumansCharacterArmature_Idle_Neutral");
   if (auto* Anim = LoadObject<UAnimSequence>(nullptr, Pfad)) SkelettKoerper->PlayAnimation(Anim, true);
  }
  return;
 }
 // Schrittfrequenz an das tatsaechliche Tempo gekoppelt: schneller gehen
 // heisst schneller schwingende Gelenke, nicht nur schnellere Fuesse ueber
 // denselben traegen Takt. Steht die Figur (Faktor nahe 0), bleiben die
 // Gelenke stehen statt auf der Stelle weiterzutreten.
 Gehphase += Zeit * Faktor * (Tempo / 45.0f);
 for (int32 s = 0; s < 2; s++) {
  const float Phase = Gehphase + (s == 0 ? 0.0f : PI);
  const float HueftGrad = PASSANT_HUEFT_GRAD * FMath::Sin(Phase);
  const float KnieGrad = PASSANT_KNIE_GRAD * FMath::Max(0.0f, FMath::Sin(Phase + HALF_PI * 0.5f));
  Huefte[s]->SetRelativeRotation(FRotator(HueftGrad, 0, 0));
  Knie[s]->SetRelativeRotation(FRotator(-KnieGrad, 0, 0));
  // Arme schwingen gegenlaeufig zum gleichseitigen Bein - wie beim Gehen ueblich.
  Schulter[s]->SetRelativeRotation(FRotator(-PASSANT_SCHULTER_GRAD * FMath::Sin(Phase), 0, 0));
 }
}

// ILaLaBergFarbbar: ein Treffer bringt kurz aus dem Tritt. Der Klecks ist
// schon das Decal der Kugel - die Kleidung behaelt ihre eigene Farbe, ein
// Treffer faerbt sie nicht um.
void ALaLaBergPassantKI::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 StolpertBis = GetWorld()->GetTimeSeconds() + 1.6f;
}
