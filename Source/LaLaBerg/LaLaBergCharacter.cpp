#include "LaLaBergCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "LaLaBergMenueSteuerung.h"
#include "LaLaBergWagen.h"
#include "LaLaBergWaffe.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

ALaLaBergCharacter::ALaLaBergCharacter() {
 Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
 Kamera->SetupAttachment(GetRootComponent());
 Kamera->SetRelativeLocation(FVector(0,0,64));
 Kamera->bUsePawnControlRotation = true;
 bUseControllerRotationYaw = true;
 GetCharacterMovement()->MaxWalkSpeed = 450;
 GetCharacterMovement()->JumpZVelocity = 420;
}
// Ohne diese Sperre faellt die Figur durch das Gelaende: sie erscheint noch im
// selben Bild, in dem die Stadt gebaut wird, und beginnt zu fallen, bevor die
// Kollisionskoerper in der Physikszene angemeldet sind. Einmal beschleunigt,
// durchschlaegt die Kapsel das duenne Dreiecksnetz und faellt endlos weiter.
void ALaLaBergCharacter::BeginPlay() {
 Super::BeginPlay();
 GetCharacterMovement()->SetMovementMode(MOVE_None);
 GetWorldTimerManager().SetTimer(BodenUhr,this,&ALaLaBergCharacter::WarteAufBoden,0.1f,true);

 // Der Paintball-Marker haengt an der Kamera wie ein Ansichtsmodell in
 // jedem Shooter - unten rechts im Bild, leicht nach vorn.
 FActorSpawnParameters Params; Params.Owner = this; Params.Instigator = this;
 Waffe = GetWorld()->SpawnActor<ALaLaBergWaffe>(GetActorLocation(), FRotator::ZeroRotator, Params);
 if (Waffe) Waffe->AttachToComponent(Kamera, FAttachmentTransformRules::KeepRelativeTransform);
 if (Waffe) Waffe->SetActorRelativeTransform(FTransform(FRotator(-2,4,0), FVector(24, 12, -14)));
}

void ALaLaBergCharacter::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if (bFeuerKnopf) Feuern();
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
