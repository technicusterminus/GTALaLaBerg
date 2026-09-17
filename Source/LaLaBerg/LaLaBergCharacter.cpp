#include "LaLaBergCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "LaLaBergMenueSteuerung.h"
#include "LaLaBergWagen.h"
#include "LaLaBergVerkehrsauto.h"
#include "LaLaBergWaffe.h"
#include "LaLaBergKoerperTeile.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
 // Kein "using namespace"/"using" hier: anonyme Namespaces sind pro
 // Uebersetzungseinheit vereinigt, nicht pro Datei - im Unity-Build wirkt
 // eine using-Deklaration hier sonst in andere .cpp-Dateien mit eigenem
 // "kasten()" (z.B. LaLaBergWaffe.cpp) hinein und macht dessen Aufrufe
 // mehrdeutig. Stattdessen wird unten LaLaBergKoerperTeile::kasten() usw.
 // voll qualifiziert aufgerufen.
 const FLinearColor JACKE(0.20f, 0.22f, 0.25f), HOSE(0.15f, 0.16f, 0.18f), HAUT(0.79f, 0.63f, 0.51f);
 constexpr float HUEFT_GRAD = 22.0f, KNIE_GRAD = 38.0f, SCHULTER_GRAD = 16.0f;
 // Grundrichtung des rechten Arms (Kamera-Pitch=0): schraeg nach vorn-rechts-
 // unten, als wuerde die Figur die Waffe waagerecht vor sich halten. Der
 // Pitch (in Grad, positiv = nach oben zielen) kippt diese Richtung um die
 // lokale Rechts-Achse (Y) - der Oberarm haengt an Schulter->Kapsel, deren
 // Gier bereits der Kamera folgt (bUseControllerRotationYaw), nur der Pitch
 // fehlt hier noch. Ueber FindBetweenVectors statt eines geratenen
 // FRotator-Winkels: das Glied haengt lokal an -Z, und ob ein positiver
 // Pitch es nach vorn oder nach hinten kippt, ist ohne Testbild nicht
 // zuverlaessig zu erraten.
 FQuat ArmDrehungFuerPitch(float PitchGrad) {
  const FVector Basis(0.75f, 0.4f, -0.35f);
  const FVector Richtung = Basis.RotateAngleAxis(-PitchGrad, FVector(0, 1, 0));
  return FQuat::FindBetweenVectors(FVector(0, 0, -1), Richtung.GetSafeNormal());
 }
 // Wie LaLaBergPassantKI::FaerbeSkelett - das Farmer-Paket bringt nur eine
 // Kleidungsfarbe je Material-Slot mit, ohne dies saehe die Spielfigur wie
 // jeder KI-Passant aus.
 void FaerbeSkelett(USkeletalMeshComponent* Komp, const FLinearColor& Farbe) {
  if (!Komp || !Komp->GetSkeletalMeshAsset()) return;
  const auto& Materials = Komp->GetSkeletalMeshAsset()->GetMaterials();
  for (int32 i = 0; i < Materials.Num(); i++) {
   const FString Name = Materials[i].MaterialSlotName.ToString();
   if (Name == TEXT("Skin") || Name == TEXT("Eye") || Name == TEXT("Eyebrows") || Name == TEXT("Moustache")) continue;
   if (auto* MID = Komp->CreateDynamicMaterialInstance(i))
    MID->SetVectorParameterValue(TEXT("DiffuseColor"), Farbe);
  }
 }
 // Wie LaLaBergPassantKI::EINZEL_FIGUREN - dieselben zusaetzlichen Figuren
 // aus demselben Paket, auch fuer die Spielfigur.
 struct FEinzelFigur { const TCHAR* Name; };
 const FEinzelFigur EINZEL_FIGUREN[] = { { TEXT("Casual") }, { TEXT("Worker") } };
 const int32 EINZEL_FIGUREN_ANZAHL = UE_ARRAY_COUNT(EINZEL_FIGUREN);
 FString EinzelAnimPfad(const TCHAR* Figur, const TCHAR* Anim) {
  return FString::Printf(TEXT("/Game/Art/People/%s/SK_%sCharacterArmature_%s.SK_%sCharacterArmature_%s"), Figur, Figur, Anim, Figur, Anim);
 }
}

// Dritte Person statt Ego-Perspektive: ohne einen sichtbaren Koerper haengt
// die Waffe sonst freischwebend an der Kamera, ganz ohne erkennbaren Traeger.
ALaLaBergCharacter::ALaLaBergCharacter() {
 GetCapsuleComponent()->InitCapsuleSize(34.0f, 88.0f);
 const float GroesseM = 1.78f;
 BeinL = GroesseM * 46.0f;
 OberschenkelL = BeinL * 0.52f;
 UnterschenkelL = BeinL * 0.48f;
 OberarmL = GroesseM * 30.0f;
 const float BodenZ = -GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Netz"));
 Netz->SetupAttachment(GetCapsuleComponent());
 Netz->SetRelativeLocation(FVector(0, 0, BodenZ));
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 // Eigenes Gesicht bekommt nur der Spieler zu sehen, wenn er sich selbst
 // betrachten koennte - in dritter Person stoert das nicht, bleibt aber
 // fuer die eigene Kamera unsichtbar, falls der Ausleger einmal einklappt.
 Netz->bOwnerNoSee = false;

 const float HueftZ = BeinL + BodenZ;
 const float SchulterZ = (BeinL + GroesseM * 46.0f) + BodenZ - GroesseM * 6.0f;
 for (int32 s = 0; s < 2; s++) {
  const float Seite = s == 0 ? 1.0f : -1.0f;
  Huefte[s] = CreateDefaultSubobject<USceneComponent>(*FString::Printf(TEXT("Huefte%d"), s));
  Huefte[s]->SetupAttachment(GetCapsuleComponent());
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
  Schulter[s]->SetupAttachment(GetCapsuleComponent());
  Schulter[s]->SetRelativeLocation(FVector(0, Seite * 20.0f, SchulterZ));

  Oberarm[s] = CreateDefaultSubobject<UProceduralMeshComponent>(*FString::Printf(TEXT("Oberarm%d"), s));
  Oberarm[s]->SetupAttachment(Schulter[s]);
  Oberarm[s]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 }
 // Rechter Oberarm haengt sonst gerade nach unten (wie beim Passanten) -
 // fest nach vorn angehoben, als wuerde die Figur dauerhaft anlegen. Der
 // Grundwinkel (Kamera-Pitch=0) hier gesetzt, Tick() dreht ihn danach jeden
 // Frame nach der tatsaechlichen Zielrichtung weiter (siehe dort).
 Schulter[0]->SetRelativeRotation(ArmDrehungFuerPitch(0.0f).Rotator());

 // Griffpunkt an seinem unteren Ende - keine eigene Drehung: seine lokale
 // +X-Achse zeigt durch die Elternkette (Schulter->Oberarm) bereits in die
 // Armrichtung, dieselbe Achse, die die Waffe selbst als Muendungsrichtung
 // benutzt (siehe ModellInfo in LaLaBergWaffe.cpp).
 WaffenHalter = CreateDefaultSubobject<USceneComponent>(TEXT("WaffenHalter"));
 WaffenHalter->SetupAttachment(Oberarm[0]);
 WaffenHalter->SetRelativeLocation(FVector(4.0f, 0, -OberarmL));

 Ausleger = CreateDefaultSubobject<USpringArmComponent>(TEXT("Ausleger"));
 Ausleger->SetupAttachment(GetCapsuleComponent());
 Ausleger->SetRelativeLocation(FVector(0, 0, GroesseM * 40.0f));
 Ausleger->TargetArmLength = 320.0f;
 Ausleger->SocketOffset = FVector(0, 45.0f, 15.0f);   // etwas ueber die rechte Schulter
 Ausleger->bUsePawnControlRotation = true;
 Ausleger->bDoCollisionTest = true;
 Ausleger->bEnableCameraLag = true;
 Ausleger->CameraLagSpeed = 12.0f;

 // Skeletal-Mesh-Alternative zum Kasten-Rig oben (siehe BeginPlay) - vier
 // Teile desselben modularen Pakets wie LaLaBergPassantKI, am selben
 // Bodenversatz wie Netz.
 auto BaueSkelettTeil = [this, BodenZ](const TCHAR* Name) {
  auto* Teil = CreateDefaultSubobject<USkeletalMeshComponent>(Name);
  Teil->SetupAttachment(GetCapsuleComponent());
  Teil->SetRelativeLocation(FVector(0, 0, BodenZ));
  Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Teil->SetVisibility(false);
  Teil->bOwnerNoSee = false;
  return Teil;
 };
 SkelettKoerper = BaueSkelettTeil(TEXT("SkelettKoerper"));
 SkelettKopf = BaueSkelettTeil(TEXT("SkelettKopf"));
 SkelettFuesse = BaueSkelettTeil(TEXT("SkelettFuesse"));
 SkelettBeine = BaueSkelettTeil(TEXT("SkelettBeine"));

 Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
 Kamera->SetupAttachment(Ausleger);
 Kamera->bUsePawnControlRotation = false;
 bUseControllerRotationYaw = true;
 GetCharacterMovement()->MaxWalkSpeed = 450;
 GetCharacterMovement()->JumpZVelocity = 420;
}

// Kopf, Hals, Rumpf - unbewegt, ein Kasten-Aufbau wie bei LaLaBergPassantKI.
// Alle Koordinaten hier sind bodenrelativ (Fuesse = 0) - Netz selbst sitzt
// bereits um BodenZ verschoben (siehe Konstruktor), ein zweites +BodenZ in
// den Vertex-Koordinaten wuerde den ganzen Oberkoerper zusaetzlich absacken
// lassen (das lag hier zunaechst falsch und liess die Arme, deren Gelenke
// direkt an der Kapsel haengen, relativ dazu viel zu hoch wirken).
void ALaLaBergCharacter::BaueOberkoerper() {
 const float GroesseM = 1.78f;
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 const float RumpfOben = BeinL + GroesseM * 46.0f;
 LaLaBergKoerperTeile::kasten(P, K, F, JACKE, FVector(0, 0, (BeinL + RumpfOben) * 0.5f), FVector(15.0f, 11.0f, (RumpfOben - BeinL) * 0.5f));
 const float HalsOben = RumpfOben + GroesseM * 4.0f;
 LaLaBergKoerperTeile::kasten(P, K, F, HAUT, FVector(0, 0, (RumpfOben + HalsOben) * 0.5f), FVector(5.0f, 5.0f, (HalsOben - RumpfOben) * 0.5f + 0.5f));
 LaLaBergKoerperTeile::kasten(P, K, F, HAUT, FVector(0, 0, HalsOben + GroesseM * 9.0f), FVector(9.0f, 9.5f, GroesseM * 9.0f));

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 LaLaBergKoerperTeile::normalen(P, K, Normalen);
 for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 60.0, P[i].Y / 60.0));
 Netz->ClearAllMeshSections();
 Netz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) Netz->SetMaterial(0, M);
}

// Derselbe Baustein wie in LaLaBergPassantKI::BaueGlied - Ursprung oben am
// Gelenk, reicht um "Laenge" nach unten.
void ALaLaBergCharacter::BaueGlied(UProceduralMeshComponent* GliedNetz, const FLinearColor& Farbe, float HalbBreite, float Laenge) {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 LaLaBergKoerperTeile::kasten(P, K, F, Farbe, FVector(0, 0, -Laenge * 0.5f), FVector(HalbBreite, HalbBreite, Laenge * 0.5f));
 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 LaLaBergKoerperTeile::normalen(P, K, Normalen);
 for (const FVector& Pt : P) UVs.Add(FVector2D(Pt.X / 40.0, Pt.Y / 40.0));
 GliedNetz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) GliedNetz->SetMaterial(0, M);
}
// Ohne diese Sperre faellt die Figur durch das Gelaende: sie erscheint noch im
// selben Bild, in dem die Stadt gebaut wird, und beginnt zu fallen, bevor die
// Kollisionskoerper in der Physikszene angemeldet sind. Einmal beschleunigt,
// durchschlaegt die Kapsel das duenne Dreiecksnetz und faellt endlos weiter.
void ALaLaBergCharacter::BeginPlay() {
 Super::BeginPlay();
 GetCharacterMovement()->SetMovementMode(MOVE_None);
 GetWorldTimerManager().SetTimer(BodenUhr,this,&ALaLaBergCharacter::WarteAufBoden,0.1f,true);

 BaueOberkoerper();
 for (int32 s = 0; s < 2; s++) {
  BaueGlied(Oberschenkel[s], HOSE, 8.5f, OberschenkelL);
  BaueGlied(Unterschenkel[s], HOSE, 7.5f, UnterschenkelL);
  BaueGlied(Oberarm[s], JACKE, 6.0f, OberarmL);
 }

 // Bevorzugt das lizenzierte Skeletal Mesh (siehe Header) - alle vier Teile
 // muessen laden, sonst bleibt es konsistent beim Kasten-Rig statt eine
 // Figur halb echt, halb Kasten zusammenzusetzen. Der Kasten-Rig bleibt in
 // beiden Faellen gebaut (siehe oben) - WaffenHalter haengt an Oberarm[0]
 // und braucht dessen Transform als Aufhaengepunkt weiter, auch unsichtbar.
 // Zufaellig eine von mehreren Figuren, wie bei den KI-Passanten (siehe
 // LaLaBergPassantKI::BeginPlay fuer die ausfuehrliche Begruendung): 0 =
 // Farmer (modular, vier Teile + externe Animation), 1..N = einfachere
 // Einzel-Figur (ein Mesh mit eigener Animation).
 auto LadeSpielerTeil = [](const TCHAR* Name) {
  return LoadObject<USkeletalMesh>(nullptr, *FString::Printf(TEXT("/Game/Art/People/Farmer/SK_Farmer_%s.SK_Farmer_%s"), Name, Name));
 };
 // Reihenfolge zufaellig mischen und der Reihe nach versuchen, statt bei
 // der ersten (zufaellig gezogenen) Figur mit fehlendem Asset direkt auf
 // das Kasten-Fallback-Rig zurueckzufallen - das Rig bleibt so nur noch
 // reserviert fuer den Fall, dass wirklich KEINE der Figuren laedt.
 TArray<int32> Reihenfolge;
 for (int32 i = 0; i <= EINZEL_FIGUREN_ANZAHL; i++) Reihenfolge.Add(i);
 for (int32 i = Reihenfolge.Num() - 1; i > 0; i--) Reihenfolge.SwapMemory(i, FMath::RandRange(0, i));
 for (int32 Wahl : Reihenfolge) {
  if (bSkelettGenutzt) break;
  if (Wahl == 0) {
   USkeletalMesh* MeshKoerper = LadeSpielerTeil(TEXT("Body"));
   USkeletalMesh* MeshKopf = LadeSpielerTeil(TEXT("Head"));
   USkeletalMesh* MeshFuesse = LadeSpielerTeil(TEXT("Feet"));
   USkeletalMesh* MeshBeine = LadeSpielerTeil(TEXT("Legs"));
   if (MeshKoerper && MeshKopf && MeshFuesse && MeshBeine) {
    bSkelettGenutzt = true;
    FigurTyp = 0;
    SkelettKoerper->SetSkeletalMesh(MeshKoerper);
    SkelettKopf->SetSkeletalMesh(MeshKopf);
    SkelettFuesse->SetSkeletalMesh(MeshFuesse);
    SkelettBeine->SetSkeletalMesh(MeshBeine);
    SkelettKopf->SetLeaderPoseComponent(SkelettKoerper);
    SkelettFuesse->SetLeaderPoseComponent(SkelettKoerper);
    SkelettBeine->SetLeaderPoseComponent(SkelettKoerper);
    for (USkeletalMeshComponent* Teil : { SkelettKoerper, SkelettKopf, SkelettFuesse, SkelettBeine }) {
     Teil->SetVisibility(true);
     FaerbeSkelett(Teil, JACKE);
    }
    if (auto* Anim = LoadObject<UAnimSequence>(nullptr, TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_Idle_Neutral.Anim_HumansCharacterArmature_Idle_Neutral")))
     SkelettKoerper->PlayAnimation(Anim, true);
   }
  } else {
   const TCHAR* Name = EINZEL_FIGUREN[Wahl - 1].Name;
   if (USkeletalMesh* NeuesMesh = LoadObject<USkeletalMesh>(nullptr,
       *FString::Printf(TEXT("/Game/Art/People/%s/SK_%s.SK_%s"), Name, Name, Name))) {
    bSkelettGenutzt = true;
    FigurTyp = Wahl;
    SkelettKoerper->SetSkeletalMesh(NeuesMesh);
    SkelettKoerper->SetVisibility(true);
    FaerbeSkelett(SkelettKoerper, JACKE);
    if (auto* Anim = LoadObject<UAnimSequence>(nullptr, *EinzelAnimPfad(Name, TEXT("Idle_Neutral"))))
     SkelettKoerper->PlayAnimation(Anim, true);
   }
  }
 }
 if (bSkelettGenutzt) {
  Netz->SetVisibility(false);
  for (int32 s = 0; s < 2; s++) {
   Oberschenkel[s]->SetVisibility(false); Unterschenkel[s]->SetVisibility(false); Oberarm[s]->SetVisibility(false);
  }
 }

 // Am rechten Arm statt an der Kamera: sonst haengt die Waffe in dritter
 // Person freischwebend im Raum, ganz ohne erkennbaren Traeger. Die Position
 // folgt dem Arm (relativ zum Griffpunkt), die Drehung bleibt bewusst an
 // die Blickrichtung der Figur gebunden statt an den Armwinkel - sonst
 // muesste jede Armhaltung exakt zur Muendungsrichtung passen.
 FActorSpawnParameters Params; Params.Owner = this; Params.Instigator = this;
 Waffe = GetWorld()->SpawnActor<ALaLaBergWaffe>(GetActorLocation(), GetActorRotation(), Params);
 if (Waffe) {
  Waffe->AttachToComponent(WaffenHalter, FAttachmentTransformRules::KeepRelativeTransform);
  // Position relativ zum Griffpunkt (klein, am Ende des Arms) - die Drehung
  // dagegen bewusst in Weltkoordinaten nach dem Anheften gesetzt, nicht
  // relativ: sie soll der Blickrichtung der Figur folgen, nicht dem
  // Armwinkel, sonst muesste jede Armhaltung exakt zur Muendung passen.
  Waffe->SetActorRelativeLocation(FVector(2, 0, 0));
  Waffe->SetActorRotation(GetActorRotation());
 }
}

void ALaLaBergCharacter::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (bFeuerKnopf) Feuern();
 PruefeSchritt(DeltaSeconds);

 // Gang wie bei den KI-Passanten (siehe LaLaBergPassantKI::Tick), an das
 // tatsaechliche Tempo gekoppelt statt an einen festen Takt. Der rechte Arm
 // haelt die Waffe fest und schwingt nicht mit, sonst zielt sie ins Leere.
 const float Tempo = GetCharacterMovement() && GetCharacterMovement()->MovementMode == MOVE_Walking
  ? GetVelocity().Size2D() : 0.0f;
 const float Faktor = FMath::Clamp(Tempo / 250.0f, 0.0f, 1.0f);
 Gehphase += DeltaSeconds * Faktor * (Tempo / 45.0f + 0.001f);
 for (int32 s = 0; s < 2; s++) {
  const float Phase = Gehphase + (s == 0 ? 0.0f : PI);
  const float HueftGrad = HUEFT_GRAD * FMath::Sin(Phase) * Faktor;
  const float KnieGrad = KNIE_GRAD * FMath::Max(0.0f, FMath::Sin(Phase + HALF_PI * 0.5f)) * Faktor;
  Huefte[s]->SetRelativeRotation(FRotator(HueftGrad, 0, 0));
  Knie[s]->SetRelativeRotation(FRotator(-KnieGrad, 0, 0));
  if (s == 1) Schulter[s]->SetRelativeRotation(FRotator(-SCHULTER_GRAD * FMath::Sin(Phase) * Faktor, 0, 0));
 }
 // Rechter Arm (Waffe) folgt der Zielrichtung: Kamera-Pitch begrenzt auf
 // einen plausiblen Schulterbereich, sonst zeigt die Muendung durch den
 // eigenen Koerper (zu weit unten) oder unnatuerlich weit ueber Kopf.
 if (Controller) {
  const float Pitch = FMath::Clamp(FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch), -50.0f, 70.0f);
  Schulter[0]->SetRelativeRotation(ArmDrehungFuerPitch(Pitch).Rotator());
 }

 if (bSkelettGenutzt) {
  // Wie bei LaLaBergPassantKI::Tick: nur beim Wechsel zwischen Stehen und
  // Gehen die AnimSequence tauschen, nicht jedes Bild neu abspielen.
  const bool bLaeuftJetzt = Faktor > 0.05f;
  if (bLaeuftJetzt != bLaeuftGerade) {
   bLaeuftGerade = bLaeuftJetzt;
   const TCHAR* AnimName = bLaeuftGerade ? TEXT("Walk") : TEXT("Idle_Neutral");
   const FString Pfad = FigurTyp == 0
    ? TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_") + FString(AnimName) + TEXT(".Anim_HumansCharacterArmature_") + FString(AnimName)
    : EinzelAnimPfad(EINZEL_FIGUREN[FigurTyp - 1].Name, AnimName);
   if (auto* Anim = LoadObject<UAnimSequence>(nullptr, *Pfad)) SkelettKoerper->PlayAnimation(Anim, true);
  }
 }
}

// Weg statt Zeit als Takt: schneller gehen heisst schneller wiederkehrende
// Tritte, nicht nur ein Metronom im Hintergrund.
void ALaLaBergCharacter::PruefeSchritt(float Zeit) {
 if (!GetCharacterMovement() || GetCharacterMovement()->MovementMode != MOVE_Walking) { SchrittWeg = 0.0f; return; }
 const float Tempo = GetVelocity().Size2D();
 if (Tempo < 20.0f) { SchrittWeg = 0.0f; return; }
 SchrittWeg += Tempo * Zeit;
 constexpr float Schrittlaenge = 140.0f;
 if (SchrittWeg < Schrittlaenge) return;
 SchrittWeg = 0.0f;
 if (auto* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Schritt.SFX_Schritt")))
  UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 0.7f, FMath::FRandRange(0.9f, 1.1f));
}

void ALaLaBergCharacter::WarteAufBoden() {
 FHitResult Hit;
 FCollisionQueryParams Params; Params.AddIgnoredActor(this);
 const FVector P=GetActorLocation();
 // Von weit oben suchen: je nach Bild erscheint die Figur auch schon einmal
 // unterhalb der Gelaendeflaeche, und von dort trifft ein Strahl nach unten
 // nichts mehr.
 if(!GetWorld()->LineTraceSingleByChannel(Hit,FVector(P.X,P.Y,30000),FVector(P.X,P.Y,-30000),ECC_Visibility,Params)) return;
 GetWorldTimerManager().ClearTimer(BodenUhr);
 SetActorLocation(Hit.ImpactPoint+FVector(0,0,95),false,nullptr,ETeleportType::TeleportPhysics);
 GetCharacterMovement()->SetMovementMode(MOVE_Walking);
 UE_LOG(LogTemp,Display,TEXT("LALABERG_BODEN z=%.0f"),Hit.ImpactPoint.Z);
}

void ALaLaBergCharacter::SetupPlayerInputComponent(UInputComponent* Input) {
 Super::SetupPlayerInputComponent(Input);
 Input->BindAxis("Forward",this,&ALaLaBergCharacter::Forward);
 Input->BindAxis("Right",this,&ALaLaBergCharacter::Right);
 Input->BindAxis("Turn",this,&APawn::AddControllerYawInput);
 Input->BindAxis("Look",this,&APawn::AddControllerPitchInput);
 Input->BindAction("Jump",IE_Pressed,this,&ACharacter::Jump);
 Input->BindAction("Jump",IE_Released,this,&ACharacter::StopJumping);
 Input->BindAction("Recover",IE_Pressed,this,&ALaLaBergCharacter::Recover);
 Input->BindAction("Quit",IE_Pressed,this,&ALaLaBergCharacter::Quit);
 Input->BindAction("Einsteigen",IE_Pressed,this,&ALaLaBergCharacter::Einsteigen);
 Input->BindAction("Feuern",IE_Pressed,this,&ALaLaBergCharacter::FeuerStart);
 Input->BindAction("Feuern",IE_Released,this,&ALaLaBergCharacter::FeuerStop);
 Input->BindAction("Waffe1",IE_Pressed,this,&ALaLaBergCharacter::Waffe1);
 Input->BindAction("Waffe2",IE_Pressed,this,&ALaLaBergCharacter::Waffe2);
 Input->BindAction("Waffe3",IE_Pressed,this,&ALaLaBergCharacter::Waffe3);
 Input->BindAction("Waffe4",IE_Pressed,this,&ALaLaBergCharacter::Waffe4);
}
// Schuss aus Blickrichtung, ein Stueck vor der Kamera - sonst trifft die
// Kugel im selben Bild die eigene Kapsel.
void ALaLaBergCharacter::Feuern() {
 if (!Waffe) return;
 Waffe->Feuern(Kamera->GetComponentLocation() + Kamera->GetForwardVector() * 70.0f, Kamera->GetForwardVector());
}
void ALaLaBergCharacter::Waffe1() { if (Waffe) Waffe->SetzeArt(ELaLaBergWaffenArt::Pistole); }
void ALaLaBergCharacter::Waffe2() { if (Waffe) Waffe->SetzeArt(ELaLaBergWaffenArt::Maschine); }
void ALaLaBergCharacter::Waffe3() { if (Waffe) Waffe->SetzeArt(ELaLaBergWaffenArt::Schrotflinte); }
void ALaLaBergCharacter::Waffe4() { if (Waffe) Waffe->SetzeArt(ELaLaBergWaffenArt::Raketenwerfer); }
void ALaLaBergCharacter::Forward(float Value) {
 if (Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::X),Value);
}
void ALaLaBergCharacter::Right(float Value) {
 if (Controller) AddMovementInput(FRotationMatrix(FRotator(0,Controller->GetControlRotation().Yaw,0)).GetUnitAxis(EAxis::Y),Value);
}
void ALaLaBergCharacter::Recover() {
 FHitResult Hit;
 FCollisionQueryParams Params; Params.AddIgnoredActor(this);
 if(GetWorld()->LineTraceSingleByChannel(Hit,FVector(0,0,20000),FVector(0,0,-20000),ECC_Visibility,Params)) {
  GetCharacterMovement()->StopMovementImmediately();
  SetActorLocation(Hit.ImpactPoint+FVector(0,0,110),false,nullptr,ETeleportType::TeleportPhysics);
 }
}
// Escape beendete das Spiel auf der Stelle - ohne Rueckfrage, ohne Weg
// zurueck. Jetzt oeffnet es das Menue.
void ALaLaBergCharacter::Quit() {
 if (UGameInstance* Spiel = GetGameInstance()) {
  if (auto* Menue = Spiel->GetSubsystem<ULaLaBergMenueSteuerung>()) { Menue->Umschalten(); return; }
 }
 FGenericPlatformMisc::RequestExit(false);
}


// Sucht den naechsten Wagen im Umkreis von acht Metern und uebernimmt ihn.
// Findet sich dort kein fahrbarer Wagen, aber ein KI-Auto: das wird an Ort
// und Stelle zu einem fahrbaren Wagen, das KI-Auto verschwindet dafuer -
// wie in einem echten GTA soll jedes Auto auf der Strasse nehmbar sein,
// nicht nur das eine dafuer vorgesehene.
void ALaLaBergCharacter::Einsteigen() {
 APlayerController* PC = Cast<APlayerController>(GetController());
 if (!PC) return;
 ALaLaBergWagen* Naechster = nullptr;
 float Beste = 800.0f;
 for (TActorIterator<ALaLaBergWagen> It(GetWorld()); It; ++It) {
  const float Abstand = FVector::Dist(It->GetActorLocation(), GetActorLocation());
  if (Abstand < Beste) { Beste = Abstand; Naechster = *It; }
 }
 if (!Naechster) {
  ALaLaBergVerkehrsauto* NaechstesKI = nullptr;
  float BesteKI = 800.0f;
  for (TActorIterator<ALaLaBergVerkehrsauto> It(GetWorld()); It; ++It) {
   const float Abstand = FVector::Dist(It->GetActorLocation(), GetActorLocation());
   if (Abstand < BesteKI) { BesteKI = Abstand; NaechstesKI = *It; }
  }
  if (NaechstesKI) {
   Naechster = GetWorld()->SpawnActor<ALaLaBergWagen>(NaechstesKI->GetActorLocation(), NaechstesKI->GetActorRotation());
   if (Naechster) { Beste = BesteKI; NaechstesKI->Destroy(); }
  }
 }
 if (!Naechster) return;
 Naechster->SetzeFahrer(this);
 SetActorHiddenInGame(true);
 SetActorEnableCollision(false);
 GetCharacterMovement()->SetMovementMode(MOVE_None);
 PC->Possess(Naechster);
 UE_LOG(LogTemp,Display,TEXT("LALABERG_EINGESTIEGEN abstand=%.0f"),Beste);
}
