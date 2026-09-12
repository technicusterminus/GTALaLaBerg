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
#include "LaLaBergWaffe.h"
#include "LaLaBergKoerperTeile.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundBase.h"

namespace {
 using namespace LaLaBergKoerperTeile;
 const FLinearColor JACKE(0.20f, 0.22f, 0.25f), HOSE(0.15f, 0.16f, 0.18f), HAUT(0.79f, 0.63f, 0.51f);
 constexpr float HUEFT_GRAD = 22.0f, KNIE_GRAD = 38.0f, SCHULTER_GRAD = 16.0f;
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
 // fest nach vorn angehoben, als wuerde die Figur dauerhaft anlegen. Anders
 // als beim linken Arm schwingt dieser nicht mit dem Gang mit (siehe Tick).
 // Ueber FindBetweenVectors statt eines geratenen FRotator-Winkels: das
 // Glied haengt lokal an -Z, und ob ein positiver Pitch es nach vorn oder
 // nach hinten kippt, ist ohne Testbild nicht zuverlaessig zu erraten.
 const FQuat ArmDrehung = FQuat::FindBetweenVectors(FVector(0, 0, -1), FVector(0.75f, 0.4f, -0.35f).GetSafeNormal());
 Schulter[0]->SetRelativeRotation(ArmDrehung.Rotator());

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
 kasten(P, K, F, JACKE, FVector(0, 0, (BeinL + RumpfOben) * 0.5f), FVector(15.0f, 11.0f, (RumpfOben - BeinL) * 0.5f));
 const float HalsOben = RumpfOben + GroesseM * 4.0f;
 kasten(P, K, F, HAUT, FVector(0, 0, (RumpfOben + HalsOben) * 0.5f), FVector(5.0f, 5.0f, (HalsOben - RumpfOben) * 0.5f + 0.5f));
 kasten(P, K, F, HAUT, FVector(0, 0, HalsOben + GroesseM * 9.0f), FVector(9.0f, 9.5f, GroesseM * 9.0f));

 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 normalen(P, K, Normalen);
 for (int32 i = 0; i < P.Num(); i++) UVs.Add(FVector2D(P[i].X / 60.0, P[i].Y / 60.0));
 Netz->ClearAllMeshSections();
 Netz->CreateMeshSection_LinearColor(0, P, K, Normalen, UVs, F, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"))) Netz->SetMaterial(0, M);
}

// Derselbe Baustein wie in LaLaBergPassantKI::BaueGlied - Ursprung oben am
// Gelenk, reicht um "Laenge" nach unten.
void ALaLaBergCharacter::BaueGlied(UProceduralMeshComponent* GliedNetz, const FLinearColor& Farbe, float HalbBreite, float Laenge) {
 TArray<FVector> P; TArray<int32> K; TArray<FLinearColor> F;
 kasten(P, K, F, Farbe, FVector(0, 0, -Laenge * 0.5f), FVector(HalbBreite, HalbBreite, Laenge * 0.5f));
 TArray<FVector> Normalen; TArray<FVector2D> UVs; TArray<FProcMeshTangent> Tangenten;
 normalen(P, K, Normalen);
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
void ALaLaBergCharacter::Einsteigen() {
 APlayerController* PC = Cast<APlayerController>(GetController());
 if (!PC) return;
 ALaLaBergWagen* Naechster = nullptr;
 float Beste = 800.0f;
 for (TActorIterator<ALaLaBergWagen> It(GetWorld()); It; ++It) {
  const float Abstand = FVector::Dist(It->GetActorLocation(), GetActorLocation());
  if (Abstand < Beste) { Beste = Abstand; Naechster = *It; }
 }
 if (!Naechster) return;
 Naechster->SetzeFahrer(this);
 SetActorHiddenInGame(true);
 SetActorEnableCollision(false);
 GetCharacterMovement()->SetMovementMode(MOVE_None);
 PC->Possess(Naechster);
 UE_LOG(LogTemp,Display,TEXT("LALABERG_EINGESTIEGEN abstand=%.0f"),Beste);
}
