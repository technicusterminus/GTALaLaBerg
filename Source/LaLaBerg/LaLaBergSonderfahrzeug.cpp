#include "LaLaBergSonderfahrzeug.h"
#include "LaLaBergHUD.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

namespace {
 // Panzer: 43 km/h, dreht auf der Stelle wie ein Kettenfahrzeug.
 constexpr float PANZER_TEMPO = 1200.0f, PANZER_SCHUB = 500.0f, PANZER_DREHUNG = 38.0f;
 // Der Rumpf sitzt mit der Unterkante auf dem Boden: halbe Kastenhoehe.
 constexpr float PANZER_HALBHOCH = 110.0f;
 // Hubschrauber: 180 km/h, steigt 8 m/s.
 constexpr float HELI_TEMPO = 5000.0f, HELI_SCHUB = 1400.0f, HELI_DREHUNG = 55.0f;
 constexpr float HELI_STEIGEN = 800.0f, HELI_HALBHOCH = 150.0f;
 // Nie tiefer als das: sonst versinkt die Zelle im Gelaende.
 constexpr float HELI_BODENABSTAND = 40.0f;

 UStaticMesh* Lade(const TCHAR* Name) {
  return LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/Art/Vehicles/Sonder/%s.%s"), Name, Name));
 }
}

ALaLaBergSonderfahrzeug::ALaLaBergSonderfahrzeug() {
 PrimaryActorTick.bCanEverTick = true;
 Rumpf = CreateDefaultSubobject<UBoxComponent>(TEXT("Rumpf"));
 Rumpf->SetBoxExtent(FVector(360.0f, 175.0f, PANZER_HALBHOCH));
 Rumpf->SetCollisionProfileName(TEXT("Pawn"));
 SetRootComponent(Rumpf);

 Koerper = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Koerper"));
 Koerper->SetupAttachment(Rumpf);
 Koerper->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Dreher = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Dreher"));
 Dreher->SetupAttachment(Rumpf);
 Dreher->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Heckrotor = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Heckrotor"));
 Heckrotor->SetupAttachment(Rumpf);
 Heckrotor->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 Ausleger = CreateDefaultSubobject<USpringArmComponent>(TEXT("Ausleger"));
 Ausleger->SetupAttachment(Rumpf);
 Ausleger->TargetArmLength = 1100.0f;
 Ausleger->SetRelativeLocation(FVector(0, 0, 220.0f));
 Ausleger->SetRelativeRotation(FRotator(-14, 0, 0));
 Ausleger->bEnableCameraLag = true;
 Ausleger->CameraLagSpeed = 5.0f;
 Ausleger->bInheritRoll = false;
 Ausleger->bInheritPitch = false;
 Ausleger->bDoCollisionTest = true;
 Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Kamera"));
 Kamera->SetupAttachment(Ausleger);

 Klang = CreateDefaultSubobject<UAudioComponent>(TEXT("Klang"));
 Klang->SetupAttachment(Rumpf);
 Klang->bAutoActivate = false;
 FSoundAttenuationSettings Raum;
 Raum.bAttenuate = true;
 Raum.bSpatialize = true;
 Raum.AttenuationShape = EAttenuationShape::Sphere;
 Raum.AttenuationShapeExtents = FVector(900.0f, 0, 0);
 Raum.FalloffDistance = 9000.0f;
 Klang->AdjustAttenuation(Raum);
}

void ALaLaBergSonderfahrzeug::BeginPlay() {
 Super::BeginPlay();
 const bool bPanzer = Art == ELaLaBergSonderart::Panzer;
 // Das Modell ist in Metern gebaut und kommt in Zentimetern an; die Teile
 // haengen so am Rumpf, dass die Unterkante auf dem Boden steht.
 Koerper->SetStaticMesh(Lade(bPanzer ? TEXT("SM_Panzer_Wanne") : TEXT("SM_Heli_Rumpf")));
 Koerper->SetRelativeLocation(FVector(0, 0, bPanzer ? -PANZER_HALBHOCH : -HELI_HALBHOCH));
 Dreher->SetStaticMesh(Lade(bPanzer ? TEXT("SM_Panzer_Turm") : TEXT("SM_Heli_Rotor")));
 // Nabe und Ringkanal sitzen dort, wo das Modell sie hat (Blender:
 // Rotormast bei x 0,35 / z 2,72, Heckrotor bei x -5,55 / z 1,88) - die
 // Zelle haengt um HELI_HALBHOCH tiefer am Rumpf.
 Dreher->SetRelativeLocation(bPanzer ? FVector(0, 0, 80.0f)
                                     : FVector(35.0f, 0, 272.0f - HELI_HALBHOCH));
 if (!bPanzer) {
  Heckrotor->SetStaticMesh(Lade(TEXT("SM_Heli_Heckrotor")));
  Heckrotor->SetRelativeLocation(FVector(-555.0f, 0.0f, 188.0f - HELI_HALBHOCH));
 } else {
  Heckrotor->SetVisibility(false);
 }
 if (bPanzer) {
  Rumpf->SetBoxExtent(FVector(360.0f, 175.0f, PANZER_HALBHOCH));
 } else {
  Rumpf->SetBoxExtent(FVector(330.0f, 130.0f, HELI_HALBHOCH));
 }
 // Olivgruen der Panzer, weiss-rot der Hubschrauber - eine Farbe je Maschine
 // reicht, beide bestehen aus einem Stueck.
 auto* Basis = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 const FLinearColor Farbe = bPanzer ? FLinearColor(0.10f, 0.13f, 0.07f) : FLinearColor(0.80f, 0.82f, 0.84f);
 for (UStaticMeshComponent* Teil : { Koerper.Get(), Dreher.Get(), Heckrotor.Get() }) {
  if (!Teil || !Teil->GetStaticMesh() || !Basis) continue;
  for (int32 Slot = 0; Slot < Teil->GetNumMaterials(); Slot++)
   if (auto* M = Teil->CreateDynamicMaterialInstance(Slot, Basis)) M->SetVectorParameterValue(TEXT("Color"), Farbe);
 }
 if (auto* Ton = LoadObject<USoundBase>(nullptr, bPanzer ? TEXT("/Game/Audio/SFX_Panzer.SFX_Panzer")
                                                        : TEXT("/Game/Audio/SFX_Rotor.SFX_Rotor")))
  Klang->SetSound(Ton);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_SONDER %s bei %s"), bPanzer ? TEXT("Panzer") : TEXT("Hubschrauber"),
        *GetActorLocation().ToString());
}

void ALaLaBergSonderfahrzeug::SetupPlayerInputComponent(UInputComponent* Eingabe) {
 Super::SetupPlayerInputComponent(Eingabe);
 Eingabe->BindAxis("Forward", this, &ALaLaBergSonderfahrzeug::Schub);
 Eingabe->BindAxis("Right", this, &ALaLaBergSonderfahrzeug::Drehen);
 Eingabe->BindAxis("Turn", this, &ALaLaBergSonderfahrzeug::Umsehen);
 Eingabe->BindAxis("Look", this, &ALaLaBergSonderfahrzeug::Nicken);
 // Leertaste steigt, Strg sinkt - beim Panzer haelt die Leertaste an.
 Eingabe->BindAction("Jump", IE_Pressed, this, &ALaLaBergSonderfahrzeug::Steigen);
 Eingabe->BindAction("Jump", IE_Released, this, &ALaLaBergSonderfahrzeug::Sinken);
 Eingabe->BindAction("Einsteigen", IE_Pressed, this, &ALaLaBergSonderfahrzeug::Aussteigen);
 Eingabe->BindAction("Recover", IE_Pressed, this, &ALaLaBergSonderfahrzeug::Aussteigen);
}

void ALaLaBergSonderfahrzeug::Umsehen(float Wert) {
 if (auto* PC = Cast<APlayerController>(GetController())) PC->AddYawInput(Wert);
}
void ALaLaBergSonderfahrzeug::Nicken(float Wert) {
 if (auto* PC = Cast<APlayerController>(GetController())) PC->AddPitchInput(Wert);
}
void ALaLaBergSonderfahrzeug::Steigen() { if (!bTest) SteigWert = 1.0f; }
void ALaLaBergSonderfahrzeug::Sinken() { if (!bTest) SteigWert = 0.0f; }

void ALaLaBergSonderfahrzeug::TestSteuerung(float NeuerSchub, float NeuesDrehen, float NeuesSteigen) {
 bTest = true;
 SchubWert = NeuerSchub; DrehWert = NeuesDrehen; SteigWert = NeuesSteigen;
}

float ALaLaBergSonderfahrzeug::TempoKmh() const { return Tempo * 0.036f; }

void ALaLaBergSonderfahrzeug::SetzeFahrer(ACharacter* Figur) {
 Fahrer = Figur;
 EinstiegZeit = GetWorld()->GetTimeSeconds();
 SchubWert = DrehWert = SteigWert = 0.0f;
 if (Klang && Klang->Sound && !Klang->IsPlaying()) Klang->Play();
}

bool ALaLaBergSonderfahrzeug::Bodenhoehe(float& Z) const {
 FHitResult Boden;
 FCollisionQueryParams Fragen;
 Fragen.AddIgnoredActor(this);
 if (Fahrer) Fragen.AddIgnoredActor(Fahrer);
 const FVector Ort = GetActorLocation();
 if (!GetWorld()->LineTraceSingleByChannel(Boden, Ort + FVector(0, 0, 600.0f),
      Ort - FVector(0, 0, 20000.0f), ECC_Visibility, Fragen)) return false;
 Z = Boden.ImpactPoint.Z;
 return true;
}

void ALaLaBergSonderfahrzeug::Tick(float Zeit) {
 Super::Tick(Zeit);
 const bool bPanzer = Art == ELaLaBergSonderart::Panzer;
 const bool bFaehrt = Fahrer != nullptr;

 if (bPanzer) {
  const float Ziel = FMath::Clamp(SchubWert, -0.6f, 1.0f) * PANZER_TEMPO;
  Tempo = FMath::FInterpConstantTo(Tempo, bFaehrt ? Ziel : 0.0f, Zeit, PANZER_SCHUB);
  // Kettenfahrzeug: dreht auch im Stand, nur langsamer als in Fahrt.
  const float Drehrate = PANZER_DREHUNG * (0.45f + 0.55f * FMath::Abs(Tempo) / PANZER_TEMPO);
  AddActorWorldRotation(FRotator(0, DrehWert * Drehrate * Zeit, 0));
  // Ohne Sweep: der Panzer sitzt mit der Unterkante auf dem Boden, und ein
  // gefegter Kasten dieser Groesse blieb schon am ersten Grasbueschel
  // haengen (im Test: 43 km/h auf dem Tacho, 0 m zurueckgelegt). Die Hoehe
  // fuehrt ohnehin der Strahl nach unten nach - Baeume walzt er nieder,
  // was fuer einen Panzer die richtige Antwort ist.
  AddActorWorldOffset(GetActorForwardVector() * Tempo * Zeit, false);
  float Boden = 0.0f;
  if (Bodenhoehe(Boden)) {
   FVector Ort = GetActorLocation();
   // Sanft nachfuehren statt springen: ueber einen Bordstein soll der Panzer
   // rollen, nicht hupfen.
   Ort.Z = FMath::FInterpTo(Ort.Z, Boden + PANZER_HALBHOCH, Zeit, 9.0f);
   SetActorLocation(Ort);
  }
  // Der Turm folgt der Blickrichtung.
  if (Dreher) {
   const float Blick = Fahrer && GetController() ? GetController()->GetControlRotation().Yaw : GetActorRotation().Yaw;
   const FRotator Jetzt = Dreher->GetComponentRotation();
   Dreher->SetWorldRotation(FMath::RInterpTo(Jetzt, FRotator(0, Blick, 0), Zeit, 3.0f));
  }
 } else {
  const float Ziel = FMath::Clamp(SchubWert, -0.5f, 1.0f) * HELI_TEMPO;
  Tempo = FMath::FInterpConstantTo(Tempo, bFaehrt ? Ziel : 0.0f, Zeit, HELI_SCHUB);
  AddActorWorldRotation(FRotator(0, DrehWert * HELI_DREHUNG * Zeit, 0));
  // Steigen auf Leertaste, sonst sinkt die Maschine langsam von selbst -
  // stehenbleiben in der Luft geht, aber nur mit Hand am Steuer.
  const float SteigZiel = bFaehrt ? (SteigWert > 0.5f ? HELI_STEIGEN : -200.0f) : -900.0f;
  Sinkflug = FMath::FInterpTo(Sinkflug, SteigZiel, Zeit, 2.5f);
  FVector Versatz = GetActorForwardVector() * Tempo * Zeit;
  Versatz.Z = Sinkflug * Zeit;
  AddActorWorldOffset(Versatz, false);
  float Boden = 0.0f;
  if (Bodenhoehe(Boden)) {
   FVector Ort = GetActorLocation();
   const float Tiefstens = Boden + HELI_HALBHOCH + HELI_BODENABSTAND;
   if (Ort.Z < Tiefstens) { Ort.Z = Tiefstens; Sinkflug = FMath::Max(Sinkflug, 0.0f); SetActorLocation(Ort); }
  }
  // Nase leicht in Fahrtrichtung, Querlage in die Kurve - ohne diese Neigung
  // schwebt die Zelle waagerecht wie ein Aufzug.
  const FRotator Lage = GetActorRotation();
  const float NeigeZiel = -8.0f * (Tempo / HELI_TEMPO);
  const float RollZiel = 12.0f * FMath::Clamp(DrehWert, -1.0f, 1.0f) * (0.3f + 0.7f * FMath::Abs(Tempo) / HELI_TEMPO);
  SetActorRotation(FRotator(FMath::FInterpTo(Lage.Pitch, NeigeZiel, Zeit, 2.0f),
                            Lage.Yaw,
                            FMath::FInterpTo(Lage.Roll, RollZiel, Zeit, 2.0f)));
  // Rotoren: im Stand langsam, im Flug schnell.
  const float Umdrehung = bFaehrt ? 900.0f : 90.0f;
  Drehphase = FMath::Fmod(Drehphase + Umdrehung * Zeit, 360.0f);
  if (Dreher) Dreher->SetRelativeRotation(FRotator(0, Drehphase, 0));
  // Der Heckrotor liegt im Ringkanal und dreht sich um die Querachse; er
  // laeuft schneller als der Hauptrotor (beim Vorbild rund viermal).
  if (Heckrotor) Heckrotor->SetRelativeRotation(FRotator(0, 0, Drehphase * 3.8f));
 }

 if (Klang) {
  if (bFaehrt && !Klang->IsPlaying()) Klang->Play();
  else if (!bFaehrt && Klang->IsPlaying()) Klang->Stop();
  if (Klang->IsPlaying())
   Klang->SetPitchMultiplier(bPanzer ? FMath::Lerp(0.85f, 1.35f, FMath::Abs(Tempo) / PANZER_TEMPO)
                                     : FMath::Lerp(0.95f, 1.25f, FMath::Abs(Tempo) / HELI_TEMPO));
 }
}

void ALaLaBergSonderfahrzeug::Aussteigen() {
 APlayerController* PC = Cast<APlayerController>(GetController());
 if (!PC || !Fahrer) return;
 const auto Meldung = [PC](const TCHAR* Text) {
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
 };
 if (GetWorld()->GetTimeSeconds() - EinstiegZeit < 0.4f) return;
 float Boden = 0.0f;
 if (!Bodenhoehe(Boden)) { Meldung(TEXT("Hier geht es nicht hinaus.")); return; }
 if (GetActorLocation().Z - Boden > 400.0f) {
  Meldung(Art == ELaLaBergSonderart::Panzer ? TEXT("Zum Aussteigen zuerst anhalten.")
                                            : TEXT("Zum Aussteigen zuerst landen."));
  return;
 }
 const UCapsuleComponent* Kapsel = Fahrer->GetCapsuleComponent();
 const FVector Ziel = GetActorLocation() + GetActorRightVector() * 420.0f
  + FVector(0, 0, Kapsel->GetScaledCapsuleHalfHeight() + 60.0f);
 PC->UnPossess();
 Fahrer->SetActorLocation(Ziel, false, nullptr, ETeleportType::TeleportPhysics);
 Fahrer->SetActorHiddenInGame(false);
 Fahrer->SetActorEnableCollision(true);
 if (auto* Bewegung = Fahrer->GetCharacterMovement()) Bewegung->SetMovementMode(MOVE_Walking);
 PC->Possess(Fahrer);
 PC->SetControlRotation(FRotator(0, GetActorRotation().Yaw, 0));
 Meldung(TEXT("Ausgestiegen - WASD zum Gehen."));
 Fahrer = nullptr;
 SchubWert = DrehWert = SteigWert = 0.0f;
 bTest = false;
}
