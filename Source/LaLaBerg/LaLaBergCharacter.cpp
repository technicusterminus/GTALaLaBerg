#include "LaLaBergCharacter.h"
#include "LaLaBergPolizei.h"
#include "LaLaBergKonto.h"
#include "LaLaBergZielAnim.h"
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
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "LaLaBergWagen.h"
#include "LaLaBergSonderfahrzeug.h"
#include "LaLaBergHUD.h"
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
 // Handflaeche hinter dem Unterarmende (das Rig hat keinen Hand-Knochen,
 // siehe BeginPlay) - dorthin kommt der Griff der Waffe.
 constexpr float HANDFLAECHE = 8.0f;
 // Grundrichtung des rechten Arms (Kamera-Pitch=0): schraeg nach vorn-rechts-
 // unten, als wuerde die Figur die Waffe waagerecht vor sich halten. Der
 // Pitch (in Grad, positiv = nach oben zielen) kippt diese Richtung um die
 // lokale Rechts-Achse (Y) - der Oberarm haengt an Schulter->Kapsel, deren
 // Gier beim Zielen der Kamera folgt (siehe Tick), nur der Pitch
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
 // Standpose der Spielfigur. Per -LaLaBergPose=<Name> austauschbar, um die
 // Posen des Figurenpakets (Idle_Gun, Idle_Gun_Pointing, ...) im Bild zu
 // vergleichen, ohne jedes Mal neu zu bauen - siehe -LaLaBergKoerperFoto.
 FString StehPose() {
  FString Pose = TEXT("Idle_Gun_Pointing");
  FParse::Value(FCommandLine::Get(), TEXT("LaLaBergPose="), Pose);
  return Pose;
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
  // Imported humanoids face +Y; gameplay and camera forward is +X.
  Teil->SetRelativeRotation(FRotator(0, -90.0f, 0));
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
 // Wie in GTA: beim Laufen dreht sich die Figur in die Bewegungsrichtung
 // und rennt vorwaerts; nur beim Schiessen schaut sie in Kamerarichtung
 // (siehe Tick). Mit fest an die Kamera gebundener Blickrichtung
 // (bUseControllerRotationYaw) lief sie bei A/D seitwaerts - mit der
 // Vorwaerts-Laufanimation sah das aus, als rutsche sie zur Seite.
 bUseControllerRotationYaw = false;
 GetCharacterMovement()->bOrientRotationToMovement = true;
 GetCharacterMovement()->RotationRate = FRotator(0, 720, 0);
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
    if (auto* Anim = LoadObject<UAnimSequence>(nullptr, *FString::Printf(TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_%s.Anim_HumansCharacterArmature_%s"), *StehPose(), *StehPose())))
     SpieleAnim(Anim);
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
    if (auto* Anim = LoadObject<UAnimSequence>(nullptr, *EinzelAnimPfad(Name, *StehPose())))
     SpieleAnim(Anim);
   }
  }
 }
 if (bSkelettGenutzt) {
  Netz->SetVisibility(false);
  for (int32 s = 0; s < 2; s++) {
   Oberschenkel[s]->SetVisibility(false); Unterschenkel[s]->SetVisibility(false); Oberarm[s]->SetVisibility(false);
  }
  // Griffpunkt an den Arm des Skeletts umhaengen. Er haengt sonst weiter am
  // Kasten-Arm (Oberarm[0]), der hier gerade unsichtbar geschaltet wurde -
  // die Waffe schwebte dadurch neben der Figur, statt gehalten zu werden.
  //
  // Das Rig aller drei Figuren (Farmer, Casual, Worker - dasselbe
  // CharacterArmature) endet bei LowerArm_R, einen Hand-Knochen gibt es
  // nicht. Der Griffpunkt kommt deshalb ans aeussere Ende des Unterarms:
  // dort, wo die Hand waere. Laenge und Richtung stammen aus dem Skelett
  // selbst (Oberarm als Mass fuer den etwa gleich langen Unterarm), damit
  // kein geratener Zahlenwert nachjustiert werden muss.
  const int32 Ellbogen = SkelettKoerper->GetBoneIndex(TEXT("LowerArm_R"));
  if (Ellbogen != INDEX_NONE) {
   const FVector EllbogenOrt = SkelettKoerper->GetBoneLocation(TEXT("LowerArm_R"));
   const FVector Arm = EllbogenOrt - SkelettKoerper->GetBoneLocation(TEXT("UpperArm_R"));
   const FTransform Knochen = SkelettKoerper->GetBoneTransform(Ellbogen);
   WaffenHalter->AttachToComponent(SkelettKoerper,
    FAttachmentTransformRules::KeepRelativeTransform, TEXT("LowerArm_R"));
   // Die Knochen dieses Rigs tragen den Massstab 100 (Blender-Export: 18 cm
   // Oberarm sind im Knochenraum 0,18). Ohne absoluten Massstab erbt der
   // Griffpunkt ihn, und alles daran haengende waechst mit: der 2-cm-Versatz
   // der Waffe wurde zu 2 m, die Waffe selbst hundertfach so gross - per
   // -LaLaBergKoerperFoto gemessen hing sie 201 cm neben der Hand. Die Lage
   // (unten) rechnet der Knochenmassstab dagegen richtig um.
   WaffenHalter->SetUsingAbsoluteScale(true);
   WaffenHalter->SetWorldScale3D(FVector::OneVector);
   WaffenHalter->SetRelativeLocation(Knochen.InverseTransformPosition(EllbogenOrt + Arm));
   // +X des Griffpunkts entlang des Unterarms ausrichten - das ist die
   // Achse, die die Waffe als Muendungsrichtung benutzt (siehe ModellInfo
   // in LaLaBergWaffe.cpp). Ueber die Armrichtung statt ueber die lokalen
   // Achsen des Knochens: welche davon den Arm entlang zeigt, haengt am
   // Export der Figur und waere geraten.
   // Dazu "oben" festlegen: MakeFromX allein laesst die Drehung um die
   // Laengsachse offen - der Griff zeigte dann seitlich weg statt nach unten
   // (im -LaLaBergKoerperFoto-Seitenbild: nur ein Rohr, kein Griff zu sehen).
   const FQuat Entlang = FRotationMatrix::MakeFromXZ(Arm.GetSafeNormal(), GetActorUpVector()).ToQuat();
   WaffenHalter->SetRelativeRotation((Knochen.GetRotation().Inverse() * Entlang).Rotator());
   // Die Unterarm-Richtung im Knochenraum merken: RichteWaffeAus richtet
   // die Waffe damit jedes Bild neu auf (siehe dort).
   UnterarmAchse = Knochen.InverseTransformVectorNoScale(Arm).GetSafeNormal();
  }
 }

 // Am Arm statt an der Kamera: sonst haengt die Waffe in dritter Person
 // freischwebend im Raum, ganz ohne erkennbaren Traeger. Der Griffpunkt
 // sitzt am Ende des Unterarms und zeigt mit +X den Arm entlang (siehe
 // oben) - die Waffe uebernimmt ihn unveraendert, Lage und Muendungs-
 // richtung kommen damit beide aus der Armhaltung. Frueher wurde die
 // Drehung stattdessen einmalig auf die Blickrichtung der Figur gesetzt;
 // das stammt aus der Zeit, als die Waffe am unsichtbaren Kasten-Arm hing
 // und ihre Haltung ohnehin zu nichts Sichtbarem passen musste.
 FActorSpawnParameters Params; Params.Owner = this; Params.Instigator = this;
 Waffe = GetWorld()->SpawnActor<ALaLaBergWaffe>(GetActorLocation(), GetActorRotation(), Params);
 if (Waffe) {
  Waffe->AttachToComponent(WaffenHalter, FAttachmentTransformRules::KeepRelativeTransform);
  Waffe->SetActorRelativeRotation(FRotator::ZeroRotator);
  RichteWaffeAus();
 }
}

void ALaLaBergCharacter::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (bFeuerKnopf) Feuern();
 PruefeSchritt(DeltaSeconds);
 PruefeAnprall(DeltaSeconds);

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
 // Beim Schiessen in Kamerarichtung drehen und seitlich gehen (mit den
 // Richtungs-Laufanimationen unten), sonst in Laufrichtung drehen - siehe
 // Konstruktor.
 const bool bZielt = bFeuerKnopf && Controller;
 GetCharacterMovement()->bOrientRotationToMovement = !bZielt;
 if (bZielt) SetActorRotation(FRotator(0, Controller->GetControlRotation().Yaw, 0));
 RichteWaffeAus();

 if (bSkelettGenutzt) {
  // Zielen nach oben und unten: der Oberkoerper beugt sich um die
  // Kameraneigung, Arme und Waffe folgen der Brust (ULaLaBergZielAnim). Im
  // Stand immer - die Waffe zeigt dann, wohin man schaut -, im Laufen nur
  // beim Schiessen, sonst lehnte sich die Figur beim Umsehen im Rennen
  // zurueck. Weich nachgefuehrt statt sprunghaft.
  float Soll = 0.0f;
  if (!FMath::IsNaN(TestNeigungGrad)) Soll = TestNeigungGrad;
  else if (Controller && (bZielt || Faktor <= 0.05f)) Soll = FRotator::NormalizeAxis(Controller->GetControlRotation().Pitch);
  Neigung = FMath::FInterpTo(Neigung, Soll, DeltaSeconds, 12.0f);
  if (auto* Ziel = Cast<ULaLaBergZielAnim>(SkelettKoerper->GetAnimInstance())) Ziel->SetzeNeigung(Neigung, GetActorRightVector());

  // Waffenhaltung statt neutraler Pose: die Spielfigur traegt immer eine
  // Waffe (siehe Waffe unten). Im Stand "Idle_Gun_Pointing" (Arm nach vorn,
  // Waffe in Blickrichtung) - per -LaLaBergKoerperFoto mit -LaLaBergPose
  // verglichen: "Idle_Gun" laesst den Arm haengen, die Waffe zeigte zu
  // Boden; "Idle_Gun_Shoot" hat dauernd Rueckstoss.
  //
  // In Bewegung die Laufanimation nach der Richtung relativ zur Figur: beim
  // Schiessen schaut sie in Kamerarichtung (siehe oben), mit A/D oder S
  // bewegt sie sich dann seitlich oder rueckwaerts. Mit einer
  // einzigen Vorwaerts-Laufanimation rutschte sie dabei sichtbar
  // seitwaerts, die Beine liefen nach vorn (im -LaLaBergKoerperFoto-Bild
  // "seitwaerts" gesehen). Das Figurenpaket bringt Run_Left/Run_Right/
  // Run_Back mit. Etwas Nachlauf beim Umschalten, damit schraeges Laufen
  // nicht jedes Bild zwischen zwei Animationen flackert.
  FString AnimName = StehPose();
  if (Faktor > 0.05f) {
   const FVector V = GetVelocity().GetSafeNormal2D();
   const float Vor = FVector::DotProduct(V, GetActorForwardVector());
   const float Rechts = FVector::DotProduct(V, GetActorRightVector());
   const bool bWarSeitlich = AktuelleAnim == TEXT("Run_Left") || AktuelleAnim == TEXT("Run_Right");
   const float Nachlauf = 0.15f;
   const bool bSeitlich = bWarSeitlich ? FMath::Abs(Rechts) + Nachlauf > FMath::Abs(Vor)
                                       : FMath::Abs(Rechts) > FMath::Abs(Vor) + Nachlauf;
   if (bSeitlich) AnimName = Rechts >= 0.0f ? TEXT("Run_Right") : TEXT("Run_Left");
   else AnimName = Vor >= 0.0f ? TEXT("Run_Shoot") : TEXT("Run_Back");
  }
  // Nur beim Wechsel tauschen, nicht jedes Bild neu abspielen.
  if (AnimName != AktuelleAnim) {
   AktuelleAnim = AnimName;
   const FString Pfad = FigurTyp == 0
    ? TEXT("/Game/Art/People/Animations/Anim_HumansCharacterArmature_") + AnimName + TEXT(".Anim_HumansCharacterArmature_") + AnimName
    : EinzelAnimPfad(EINZEL_FIGUREN[FigurTyp - 1].Name, *AnimName);
   if (auto* Anim = LoadObject<UAnimSequence>(nullptr, *Pfad)) SpieleAnim(Anim);
  }
 }
}

void ALaLaBergCharacter::SpieleAnim(UAnimSequence* Anim) {
 if (!Anim || !SkelettKoerper) return;
 if (!Cast<ULaLaBergZielAnim>(SkelettKoerper->GetAnimInstance())) {
  SkelettKoerper->SetAnimationMode(EAnimationMode::AnimationBlueprint);
  SkelettKoerper->SetAnimInstanceClass(ULaLaBergZielAnim::StaticClass());
 }
 if (auto* Ziel = Cast<ULaLaBergZielAnim>(SkelettKoerper->GetAnimInstance())) Ziel->Spiele(Anim);
 else SkelettKoerper->PlayAnimation(Anim, true);
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
 // Auf Asphalt und Pflaster klackt der Absatz, auf der Wiese knistert es.
 // Welcher Belag es ist, sagt der Name des Netzes unter den Fuessen (die
 // Stadtteile heissen Road, Sidewalk, Gehweg, Plaza, Ground - siehe
 // LaLaBergImportCommandlet).
 bool bHart = true;
 {
  FHitResult Boden;
  FCollisionQueryParams Fragen; Fragen.AddIgnoredActor(this);
  const FVector Fuss = GetActorLocation();
  if (GetWorld()->LineTraceSingleByChannel(Boden, Fuss, Fuss - FVector(0, 0, 250.0f), ECC_Visibility, Fragen)) {
   const auto* Belag = Cast<UStaticMeshComponent>(Boden.GetComponent());
   const FString Name = Belag && Belag->GetStaticMesh() ? Belag->GetStaticMesh()->GetName() : FString();
   bHart = Name.Contains(TEXT("Road")) || Name.Contains(TEXT("Sidewalk")) || Name.Contains(TEXT("Gehweg"))
        || Name.Contains(TEXT("Plaza"))
        || Name.Contains(TEXT("Rail")) || Name.Contains(TEXT("Roof")) || Name.Contains(TEXT("Wall"));
  }
 }
 const TCHAR* Pfad = bHart ? TEXT("/Game/Audio/SFX_Schritt.SFX_Schritt")
                           : TEXT("/Game/Audio/SFX_Schritt_Gras.SFX_Schritt_Gras");
 USoundBase* Sound = LoadObject<USoundBase>(nullptr, Pfad);
 if (!Sound) Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Schritt.SFX_Schritt"));
 if (Sound) UGameplayStatics::PlaySoundAtLocation(this, Sound, GetActorLocation(), 0.7f, FMath::FRandRange(0.9f, 1.1f));
}

// Angefahren werden. Die Fahrzeuge bewegen sich kinematisch (Verkehrsautos)
// oder haengen an Chaos (der eigene Wagen) - ein Stossimpuls aus der Physik
// kommt bei der Figur nie an. Deshalb jedes Bild selbst nachsehen: was ist
// nah, wie schnell ist es, und kommt es auf mich zu?
void ALaLaBergCharacter::PruefeAnprall(float Zeit) {
 if (Leben <= 0.0f) return;
 const float Jetzt = GetWorld()->GetTimeSeconds();
 if (Jetzt - LetzterAnprall < 1.2f) return;          // ein Stoss, nicht dreissig
 const FVector Wo = GetActorLocation();
 auto Pruefe = [&](AActor* Fahrzeug) {
  if (!Fahrzeug) return false;
  const FVector Weg = Wo - Fahrzeug->GetActorLocation();
  if (Weg.SizeSquared2D() > FMath::Square(320.0f) || FMath::Abs(Weg.Z) > 260.0f) return false;
  const FVector Fahrt = Fahrzeug->GetVelocity();
  const float TempoKmh = Fahrt.Size() * 0.036f;
  if (TempoKmh < 12.0f) return false;
  // Nur, wenn das Fahrzeug auch in meine Richtung faehrt.
  if (FVector::DotProduct(Fahrt.GetSafeNormal2D(), Weg.GetSafeNormal2D()) < 0.2f) return false;
  LetzterAnprall = Jetzt;
  Verletze(FMath::GetMappedRangeValueClamped(FVector2D(12.0f, 80.0f), FVector2D(8.0f, 95.0f), TempoKmh),
           Fahrt.GetSafeNormal2D(), ELaLaBergSchaden::Anprall);
  return true;
 };
 for (ALaLaBergVerkehrsauto* Auto : ALaLaBergVerkehrsauto::Alle) if (Pruefe(Auto)) return;
 for (TActorIterator<ALaLaBergWagen> It(GetWorld()); It; ++It) if (Pruefe(*It)) return;
}

void ALaLaBergCharacter::Verletze(float Schaden, const FVector& AusRichtung, ELaLaBergSchaden Art) {
 if (Leben <= 0.0f) return;
 Leben -= Schaden;
 // Ein Stoss wirft die Figur ein Stueck - sichtbar, aber ohne Kontrolle zu
 // nehmen.
 if (Art == ELaLaBergSchaden::Anprall || Art == ELaLaBergSchaden::Sprengung)
  LaunchCharacter(AusRichtung.GetSafeNormal2D() * 420.0f + FVector(0, 0, 260.0f), true, true);
 if (auto* PC = Cast<APlayerController>(GetController()))
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD()))
   HUD->ZeigeRueckmeldung(Leben > 0.0f ? FString::Printf(TEXT("Getroffen – %d Punkte übrig"), FMath::CeilToInt(Leben))
                                       : FString(TEXT("Umgehauen – ab ins Klinikum")));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_SCHADEN spieler=%.0f art=%d"), Leben, static_cast<int32>(Art));
 if (Leben <= 0.0f) InsKrankenhaus();
}

// Aufwachen im Klinikum. Der Startpunkt des Spiels ist ohnehin dort - hier
// wird er zum Krankenhaus im Wortsinn.
void ALaLaBergCharacter::InsKrankenhaus() {
 Leben = 100.0f;
 Krankenhausbesuche++;
 auto* PC = Cast<APlayerController>(GetController());
 // Behandlungskosten: ein Viertel des Geldes, hoechstens 500 Euro.
 int32 Kosten = 0;
 if (ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this)) {
  Kosten = FMath::Min(500, FMath::Max(0, Konto->HoleGeld() / 4));
  if (Kosten > 0) Konto->Bezahle(Kosten);
 }
 // Die Fahndung ist mit dem Zusammenbruch vorbei.
 if (ALaLaBergPolizei* Polizei = ALaLaBergPolizei::Instanz.Get()) Polizei->Verwische();
 // Auf den Vorplatz des Klinikums setzen, auf dem Boden.
 FVector Ziel(-159000.0f, 15600.0f, 30000.0f);
 FHitResult Boden;
 FCollisionQueryParams Fragen; Fragen.AddIgnoredActor(this);
 if (GetWorld()->LineTraceSingleByChannel(Boden, Ziel, Ziel - FVector(0, 0, 60000.0f), ECC_Visibility, Fragen))
  Ziel = Boden.ImpactPoint + FVector(0, 0, GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 10.0f);
 SetActorLocation(Ziel, false, nullptr, ETeleportType::TeleportPhysics);
 if (auto* Bewegung = GetCharacterMovement()) Bewegung->StopMovementImmediately();
 if (PC) {
  PC->SetControlRotation(FRotator(0, -90.0f, 0));
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD()))
   HUD->ZeigeRueckmeldung(Kosten > 0 ? FString::Printf(TEXT("Klinikum – wieder auf den Beinen, Behandlung %d €"), Kosten)
                                     : FString(TEXT("Klinikum – wieder auf den Beinen")));
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KRANKENHAUS besuch=%d kosten=%d"), Krankenhausbesuche, Kosten);
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
// Setzt die Waffe so, dass ihr Griff (siehe ALaLaBergWaffe::GriffOrt) in
// der Handflaeche liegt - jedes Bild, weil der Griff je Waffenart woanders
// sitzt und die Art per 1-4 wechselt.
void ALaLaBergCharacter::RichteWaffeAus() {
 if (Waffe) Waffe->SetActorRelativeLocation(FVector(HANDFLAECHE, 0, 0) - Waffe->GriffOrt());
 // Aufrecht halten: Laufrichtung entlang des aktuellen Unterarms, "oben"
 // aber immer Welt-oben. Nur beim Anheften in der Ruhepose ausgerichtet,
 // drehte die Zeige-Animation den Unterarm um seine Laengsachse mit - der
 // Griff zeigte zur Seite, der Werfer umschloss den Arm (im Seitenbild
 // von -LaLaBergKoerperFoto gesehen).
 if (bSkelettGenutzt && !UnterarmAchse.IsNearlyZero()) {
  const int32 Ellbogen = SkelettKoerper->GetBoneIndex(TEXT("LowerArm_R"));
  if (Ellbogen != INDEX_NONE) {
   const FVector Richtung = SkelettKoerper->GetBoneTransform(Ellbogen).TransformVectorNoScale(UnterarmAchse);
   WaffenHalter->SetWorldRotation(FRotationMatrix::MakeFromXZ(Richtung, FVector::UpVector).Rotator());
  }
 }
}

bool ALaLaBergCharacter::HoleWaffenabstand(float& AusAbstandCm) const {
 if (!Waffe || !bSkelettGenutzt || !SkelettKoerper) return false;
 const int32 Ellbogen = SkelettKoerper->GetBoneIndex(TEXT("LowerArm_R"));
 if (Ellbogen == INDEX_NONE) return false;
 const FVector EllbogenOrt = SkelettKoerper->GetBoneLocation(TEXT("LowerArm_R"));
 const FVector Arm = EllbogenOrt - SkelettKoerper->GetBoneLocation(TEXT("UpperArm_R"));
 // Griff der Waffe gegen die Handflaeche: HANDFLAECHE hinter dem Ende des
 // Unterarms, dort wo RichteWaffeAus den Griff hinsetzt.
 const FVector Handflaeche = EllbogenOrt + Arm + Arm.GetSafeNormal() * HANDFLAECHE;
 AusAbstandCm = FVector::Dist(Waffe->GetActorTransform().TransformPosition(Waffe->GriffOrt()), Handflaeche);
 return true;
}

void ALaLaBergCharacter::Feuern() {
 if (!Waffe) return;
 // Vor dem Schuss in Schussrichtung drehen: der Schuss geht aus der Kamera,
 // die Figur soll dabei sichtbar dorthin zielen, auch nach dem Laufen.
 if (Controller) SetActorRotation(FRotator(0, Controller->GetControlRotation().Yaw, 0));
 Waffe->Feuern(Kamera->GetComponentLocation() + Kamera->GetForwardVector() * 70.0f, Kamera->GetForwardVector());
}
void ALaLaBergCharacter::Waffe1() { if (Waffe) Waffe->SetzeArt(ELaLaBergWaffenArt::Pistole); }
// Nur die Pistole hat man von Anfang an; die anderen gibt es im
// Paintball-Laden (siehe ALaLaBergLaeden, gespeichert im Konto).
static bool Besitzt(const ALaLaBergCharacter* Figur, ELaLaBergWaffenArt Art, const TCHAR* Name) {
 const ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(Figur);
 if (!Konto || Konto->HatWaffe(static_cast<uint8>(Art))) return true;
 if (const auto* PC = Cast<APlayerController>(Figur->GetController()))
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD()))
   HUD->ZeigeRueckmeldung(FString::Printf(TEXT("%s gibt es im Paintball-Laden – grüne Marke auf der Karte (M)"), Name));
 return false;
}
void ALaLaBergCharacter::Waffe2() { if (Waffe && Besitzt(this, ELaLaBergWaffenArt::Maschine, TEXT("Die Paintball-MP"))) Waffe->SetzeArt(ELaLaBergWaffenArt::Maschine); }
void ALaLaBergCharacter::Waffe3() { if (Waffe && Besitzt(this, ELaLaBergWaffenArt::Schrotflinte, TEXT("Die Paintball-Schrotflinte"))) Waffe->SetzeArt(ELaLaBergWaffenArt::Schrotflinte); }
void ALaLaBergCharacter::Waffe4() { if (Waffe && Besitzt(this, ELaLaBergWaffenArt::Raketenwerfer, TEXT("Den Paintball-Werfer"))) Waffe->SetzeArt(ELaLaBergWaffenArt::Raketenwerfer); }
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
 // Panzer und Hubschrauber zuerst: wer vor dem Panzer steht, will in den
 // Panzer, auch wenn daneben ein Auto parkt.
 {
  ALaLaBergSonderfahrzeug* Sonder = nullptr;
  float BesteS = 900.0f;
  for (TActorIterator<ALaLaBergSonderfahrzeug> It(GetWorld()); It; ++It) {
   if (It->HatFahrer()) continue;
   const float Abstand = FVector::Dist(It->GetActorLocation(), GetActorLocation());
   if (Abstand < BesteS) { BesteS = Abstand; Sonder = *It; }
  }
  if (Sonder) {
   Sonder->SetzeFahrer(this);
   SetActorHiddenInGame(true);
   SetActorEnableCollision(false);
   GetCharacterMovement()->SetMovementMode(MOVE_None);
   PC->Possess(Sonder);
   if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD()))
    HUD->ZeigeRueckmeldung(Sonder->HoleArt() == ELaLaBergSonderart::Panzer
     ? TEXT("Panzer - W/S fahren, A/D drehen, Turm folgt dem Blick, E aussteigen.")
     : TEXT("Hubschrauber - Leertaste steigen, W/S fliegen, A/D drehen, E aussteigen."));
   UE_LOG(LogTemp, Display, TEXT("LALABERG_SONDER eingestiegen art=%d abstand=%.0f"),
          static_cast<int32>(Sonder->HoleArt()), BesteS);
   return;
  }
 }
 ALaLaBergWagen* Naechster = nullptr;
 float Beste = 800.0f;
 for (TActorIterator<ALaLaBergWagen> It(GetWorld()); It; ++It) {
  if (It->GetController()) continue;
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
   // Aufgeschoben erzeugt (SpawnActorDeferred), damit Modell und Lack des
   // uebernommenen Autos noch vor BeginPlay feststehen - dort wird die
   // Karosserie gebaut. Vorher wuerfelte der neue Wagen sein Modell selbst
   // und man sass in einem ganz anderen Auto, als man angehalten hatte.
   const FTransform Stelle(NaechstesKI->GetActorRotation(), NaechstesKI->GetActorLocation());
   Naechster = GetWorld()->SpawnActorDeferred<ALaLaBergWagen>(ALaLaBergWagen::StaticClass(), Stelle);
   if (Naechster) {
    Naechster->UebernimmModell(NaechstesKI->HoleFahrzeugTyp(), NaechstesKI->HoleLack());
    Naechster->FinishSpawning(Stelle);
    Beste = BesteKI;
    // Wer ein fahrendes Auto uebernimmt, stiehlt es - und ein Streifenwagen
    // wiegt doppelt.
    const bool bStreife = NaechstesKI->bPolizei;
    NaechstesKI->Destroy();
    ALaLaBergPolizei::Melde(ELaLaBergTat::AutoGestohlen);
    if (bStreife) ALaLaBergPolizei::Melde(ELaLaBergTat::PolizeiBeschossen);
   }
  }
 }
 if (!Naechster) {
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD()))
   HUD->ZeigeRueckmeldung(TEXT("Kein freies Fahrzeug in Reichweite. Bitte naeher herangehen."));
  return;
 }
 Naechster->SetzeFahrer(this);
 SetActorHiddenInGame(true);
 SetActorEnableCollision(false);
 GetCharacterMovement()->SetMovementMode(MOVE_None);
 PC->Possess(Naechster);
 if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD()))
  HUD->ZeigeRueckmeldung(TEXT("Eingestiegen - W/S fahren, Leertaste bremsen, E aussteigen."));
 UE_LOG(LogTemp,Display,TEXT("LALABERG_EINGESTIEGEN abstand=%.0f"),Beste);
}
