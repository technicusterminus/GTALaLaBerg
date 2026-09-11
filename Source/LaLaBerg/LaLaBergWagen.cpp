#include "LaLaBergWagen.h"
#include "ProceduralMeshComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "LaLaBergWagenForm.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

namespace {
 // Dieselben neun Querschnitte wie bei den geparkten Wagen, in Zentimetern.
 // u laeuft in Fahrtrichtung, w ist die Breite, y0/y1 sind Unter- und
 // Oberkante des Aufbaus ueber der Fahrbahn.
 struct FSchnitt { float u, w, y0, y1; };
 const FSchnitt Karosse[] = {
  { -212, 160,  42,  86 }, { -186, 175,  34, 102 }, { -152, 179,  30, 108 },
  {  -96, 179,  28, 110 }, {    0, 179,  28, 110 }, {   86, 178,  29, 108 },
  {  154, 174,  32, 100 }, {  192, 164,  38,  90 }, {  213, 148,  46,  80 },
 };
 const FSchnitt Kabine[] = {
  { -144, 144, 102, 134 }, { -110, 153, 104, 144 }, { -30, 156, 105, 146 },
  {   42, 153, 105, 144 }, {  102, 142, 104, 116 },
 };

 void Ring(const FSchnitt& S, TArray<FVector>& Aus) {
  const float hw = S.w * 0.5f, c = S.w * 0.17f, r = (S.y1 - S.y0) * 0.22f;
  Aus.Reset();
  Aus.Add(FVector(0,  hw,      S.y0 + r));
  Aus.Add(FVector(0,  hw,      S.y1 - r));
  Aus.Add(FVector(0,  hw - c,  S.y1));
  Aus.Add(FVector(0, -hw + c,  S.y1));
  Aus.Add(FVector(0, -hw,      S.y1 - r));
  Aus.Add(FVector(0, -hw,      S.y0 + r));
  Aus.Add(FVector(0, -hw + c,  S.y0));
  Aus.Add(FVector(0,  hw - c,  S.y0));
 }

 // Ein Zug ueber die Schnitte, mit Deckeln vorn und hinten.
 void Loft(const FSchnitt* Schnitte, int32 Anzahl, TArray<FVector>& Punkte, TArray<int32>& Kanten) {
  TArray<FVector> Vorher, Jetzt;
  int32 VorherIndex = -1;
  for (int32 i = 0; i < Anzahl; i++) {
   Ring(Schnitte[i], Jetzt);
   const int32 Basis = Punkte.Num();
   for (const FVector& P : Jetzt) Punkte.Add(FVector(Schnitte[i].u, P.Y, P.Z));
   if (VorherIndex >= 0) {
    for (int32 k = 0; k < 8; k++) {
     const int32 j = (k + 1) % 8;
     Kanten.Append({ VorherIndex + k, VorherIndex + j, Basis + j });
     Kanten.Append({ VorherIndex + k, Basis + j, Basis + k });
    }
   }
   if (i == 0) for (int32 k = 1; k + 1 < 8; k++) Kanten.Append({ Basis, Basis + k + 1, Basis + k });
   if (i == Anzahl - 1) for (int32 k = 1; k + 1 < 8; k++) Kanten.Append({ Basis, Basis + k, Basis + k + 1 });
   VorherIndex = Basis;
  }
 }

 void Rad(const FVector& Mitte, float r, float halb, TArray<FVector>& Punkte, TArray<int32>& Kanten) {
  const int32 Seiten = 12, Basis = Punkte.Num();
  for (int32 i = 0; i < Seiten; i++) {
   const float a = i * 2.0f * PI / Seiten;
   const FVector Rand(FMath::Cos(a) * r, 0, FMath::Sin(a) * r);
   Punkte.Add(Mitte + Rand - FVector(0, halb, 0));
   Punkte.Add(Mitte + Rand + FVector(0, halb, 0));
  }
  for (int32 i = 0; i < Seiten; i++) {
   const int32 j = (i + 1) % Seiten;
   Kanten.Append({ Basis + i * 2, Basis + j * 2, Basis + j * 2 + 1 });
   Kanten.Append({ Basis + i * 2, Basis + j * 2 + 1, Basis + i * 2 + 1 });
  }
  // Deckel auf beiden Seiten. Ohne sie war das Rad ein offener Ring und sah
  // aus der Naehe wie ein gebogener Blechstreifen aus. Eigene Randpunkte,
  // damit die Normalen von Deckel und Laufflaeche sich nicht mitteln - sonst
  // wirkte jedes Rad wie eine Kugel.
  const int32 Deckel = Punkte.Num();
  for (int32 i = 0; i < Seiten * 2; i++) { const FVector Kopie = Punkte[Basis + i]; Punkte.Add(Kopie); }
  const int32 Innen = Punkte.Num();
  Punkte.Add(Mitte - FVector(0, halb, 0));
  Punkte.Add(Mitte + FVector(0, halb, 0));
  for (int32 i = 0; i < Seiten; i++) {
   const int32 j = (i + 1) % Seiten;
   Kanten.Append({ Innen + 1, Deckel + i * 2 + 1, Deckel + j * 2 + 1 });   // nach +Y
   Kanten.Append({ Innen, Deckel + j * 2, Deckel + i * 2 });               // nach -Y
  }
 }
}

ALaLaBergWagen::ALaLaBergWagen() {
 PrimaryActorTick.bCanEverTick = true;

 Rumpf = CreateDefaultSubobject<UBoxComponent>(TEXT("Rumpf"));
 Rumpf->InitBoxExtent(FVector(210, 88, 55));
 SetRootComponent(Rumpf);

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Karosserie"));
 Netz->SetupAttachment(Rumpf);
 // Der Wagen ruht auf der Federung: die Federstrahlen beginnen 55 cm unter
 // dem Kastenmittelpunkt und sind 88 cm lang; im Stand federt jedes Rad
 // 1250 kg * 9,8 / 4 / 34000 = 9 cm ein. Die Fahrbahn liegt also
 // 55 + 88 - 9 = 134 cm unter der Mitte. Mit den frueher angesetzten 88 cm
 // schwebte der Wagen sichtbar eine Handbreit ueber der Strasse.
 Netz->SetRelativeLocation(FVector(0, 0, -134));
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 Ausleger = CreateDefaultSubobject<USpringArmComponent>(TEXT("Ausleger"));
 Ausleger->SetupAttachment(Rumpf);
 Ausleger->TargetArmLength = 620.0f;
 Ausleger->SetRelativeLocation(FVector(0, 0, 70));
 Ausleger->SetRelativeRotation(FRotator(-12, 0, 0));
 Ausleger->bEnableCameraLag = true;
 Ausleger->CameraLagSpeed = 6.0f;
 Ausleger->bDoCollisionTest = true;
 // Die Kamera folgt der Fahrtrichtung, kippt aber nicht mit dem Wagen:
 // quer zum Hang stand sonst der ganze Horizont schief.
 Ausleger->bInheritRoll = false;
 Ausleger->bInheritPitch = false;

 Kamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Kamera"));
 Kamera->SetupAttachment(Ausleger);

 // Leerlaufbrummen, das mit Tempo/Gas in Tonhoehe und Lautstaerke steigt -
 // ein einzelner schleifenfaehiger Klang genuegt, siehe Tick().
 Motorklang = CreateDefaultSubobject<UAudioComponent>(TEXT("Motorklang"));
 Motorklang->SetupAttachment(Rumpf);
 Motorklang->bAutoActivate = false;
}

void ALaLaBergWagen::BeginPlay() {
 Super::BeginPlay();
 // Physik erst am echten Actor konfigurieren. Im Konstruktor lief dieselbe
 // Folge auch fuer das Class Default Object, bevor GEngine bereit war; die
 // daraus entstehenden Materialfehler konnten ein echtes Fahrzeug mit
 // unvollstaendigen Body-Parametern hinterlassen.
 Rumpf->SetCollisionProfileName(TEXT("PhysicsActor"));
 Rumpf->SetSimulatePhysics(true);
 Rumpf->SetLinearDamping(0.04f);
 Rumpf->SetAngularDamping(3.5f);
 Rumpf->SetMassOverrideInKg(NAME_None, 1250.0f, true);
 // Schwerpunkt tief: sonst kippt der Wagen in der ersten Kurve um.
 Rumpf->SetCenterOfMass(FVector(0, 0, -45));
 Rumpf->WakeAllRigidBodies();
 BaueKarosserie();
 bGebaut = true;
 if (auto* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SFX_Motor.SFX_Motor"))) {
  Motorklang->SetSound(Sound);
  Motorklang->Play();
 }
}

// Die Form der geparkten Wagen, aus prepare-wagen.cjs: Radkaesten, Fenster,
// Leuchten, Felgen. Geteilt mit den KI-Verkehrswagen (LaLaBergWagenForm).
bool ALaLaBergWagen::BaueAusVorlage() { return LaLaBergWagenForm::BaueNetz(Netz, Lack); }

// ILaLaBergFarbbar: nur ein kleiner Stoss. Der Klecks selbst ist schon das
// Decal, das die Kugel beim Aufprall setzt (siehe LaLaBergFarbkugel) - der
// Wagen behaelt seinen eigenen Lack, ein Treffer faerbt nicht das ganze Auto um.
void ALaLaBergWagen::ErhalteFarbe(const FLinearColor& Farbe, const FVector& AusRichtung) {
 if (Rumpf && Rumpf->IsSimulatingPhysics()) Rumpf->AddImpulse(AusRichtung.GetSafeNormal() * 1800.0f * Rumpf->GetMass());
}

void ALaLaBergWagen::BaueKarosserie() {
 if (BaueAusVorlage()) return;
 TArray<FVector> Punkte; TArray<int32> Kanten;
 Loft(Karosse, UE_ARRAY_COUNT(Karosse), Punkte, Kanten);
 Loft(Kabine, UE_ARRAY_COUNT(Kabine), Punkte, Kanten);
 for (const FVector& Nabe : { FVector(131, 79, 33), FVector(131, -79, 33),
                              FVector(-131, 79, 33), FVector(-131, -79, 33) }) {
  Rad(Nabe, 33, 10.5f, Punkte, Kanten);
 }

 TArray<FVector> Normalen; TArray<FVector2D> UVs;
 TArray<FLinearColor> Farben; TArray<FProcMeshTangent> Tangenten;
 Normalen.Init(FVector::ZeroVector, Punkte.Num());
 for (int32 i = 0; i + 2 < Kanten.Num(); i += 3) {
  const FVector N = FVector::CrossProduct(Punkte[Kanten[i + 2]] - Punkte[Kanten[i]],
                                          Punkte[Kanten[i + 1]] - Punkte[Kanten[i]]);
  Normalen[Kanten[i]] += N; Normalen[Kanten[i + 1]] += N; Normalen[Kanten[i + 2]] += N;
 }
 for (FVector& N : Normalen) N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
 for (const FVector& P : Punkte) {
  UVs.Add(FVector2D(P.X / 200.0, P.Y / 200.0));
  // Raeder und Reifen dunkel, der Rest im Lack des Wagens
  Farben.Add(P.Z < 70 && FMath::Abs(P.Y) > 66 ? FLinearColor(0.10f, 0.10f, 0.11f) : Lack);
 }
 Netz->CreateMeshSection_LinearColor(0, Punkte, Kanten, Normalen, UVs, Farben, Tangenten, false);
 if (auto* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Lack.M_Lack")))
  Netz->SetMaterial(0, M);
}

void ALaLaBergWagen::SetupPlayerInputComponent(UInputComponent* Eingabe) {
 Super::SetupPlayerInputComponent(Eingabe);
 Eingabe->BindAxis("Forward", this, &ALaLaBergWagen::Gas);
 Eingabe->BindAxis("Right", this, &ALaLaBergWagen::Lenken);
 Eingabe->BindAxis("Turn", this, &ALaLaBergWagen::Umsehen);
 Eingabe->BindAxis("Look", this, &ALaLaBergWagen::Nicken);
 Eingabe->BindAction("Jump", IE_Pressed, this, &ALaLaBergWagen::Bremsen);
 Eingabe->BindAction("Jump", IE_Released, this, &ALaLaBergWagen::Loesen);
 // Aus- wie Einsteigen mit E; R bleibt als gewohnte zweite Taste.
 Eingabe->BindAction("Einsteigen", IE_Pressed, this, &ALaLaBergWagen::Aussteigen);
 Eingabe->BindAction("Recover", IE_Pressed, this, &ALaLaBergWagen::Aussteigen);
}

void ALaLaBergWagen::Gas(float Wert) { if (!bTest) GasWert = FMath::Clamp(Wert, -1.0f, 1.0f); }
void ALaLaBergWagen::Lenken(float Wert) { if (!bTest) LenkWert = FMath::Clamp(Wert, -1.0f, 1.0f); }
void ALaLaBergWagen::Umsehen(float Wert) { if (Ausleger) Ausleger->AddRelativeRotation(FRotator(0, Wert * 2.0f, 0)); }
void ALaLaBergWagen::Nicken(float Wert) {
 if (!Ausleger) return;
 FRotator R = Ausleger->GetRelativeRotation();
 R.Pitch = FMath::Clamp(R.Pitch - Wert * 1.5f, -60.0f, 15.0f);
 Ausleger->SetRelativeRotation(R);
}
void ALaLaBergWagen::SetzeFahrer(ACharacter* Figur) {
 Fahrer = Figur;
 EinstiegZeit = GetWorld()->GetTimeSeconds();
}

// Getrennt statt umschalten: wer die Leertaste noch als Figur drueckte und im
// Wagen losliess, fuhr sonst mit angezogener Bremse los.
void ALaLaBergWagen::Bremsen() { bBremse = true; }
void ALaLaBergWagen::Loesen() { bBremse = false; }

// Aussteigen: die Figur wird neben dem Wagen abgesetzt, auf der Fahrerseite,
// und uebernimmt wieder die Steuerung.
void ALaLaBergWagen::Aussteigen() {
 APlayerController* PC = Cast<APlayerController>(GetController());
 if (!PC || !Fahrer) return;
 // Einsteigen und Aussteigen liegen auf derselben Taste. Ohne diese Frist
 // koennte derselbe Tastendruck den Wagen gleich wieder verlassen.
 if (GetWorld()->GetTimeSeconds() - EinstiegZeit < 0.4f) return;
 const FVector Neben = GetActorLocation() - GetActorRightVector() * 190.0f + FVector(0, 0, 40);
 FHitResult Boden;
 FCollisionQueryParams Fragen; Fragen.AddIgnoredActor(this); Fragen.AddIgnoredActor(Fahrer);
 FVector Ziel = Neben;
 if (GetWorld()->LineTraceSingleByChannel(Boden, Neben + FVector(0, 0, 400),
                                          Neben - FVector(0, 0, 800), ECC_Visibility, Fragen)) {
  Ziel = Boden.ImpactPoint + FVector(0, 0, 95);
 }
 PC->UnPossess();
 Fahrer->SetActorLocation(Ziel, false, nullptr, ETeleportType::TeleportPhysics);
 Fahrer->SetActorHiddenInGame(false);
 Fahrer->SetActorEnableCollision(true);
 if (auto* Bewegung = Fahrer->GetCharacterMovement()) Bewegung->SetMovementMode(MOVE_Walking);
 PC->Possess(Fahrer);
 PC->SetControlRotation(FRotator(0, GetActorRotation().Yaw, 0));
 Fahrer = nullptr;
}

void ALaLaBergWagen::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Rumpf || !Rumpf->IsSimulatingPhysics()) return;

 const FTransform Lage = Rumpf->GetComponentTransform();
 const FVector Vorne = Lage.GetUnitAxis(EAxis::X);
 const FVector Rechts = Lage.GetUnitAxis(EAxis::Y);
 const FVector Oben = Lage.GetUnitAxis(EAxis::Z);

 // Vier Federstrahlen. Ruhelaenge 55 cm, Rad 33 cm - der Wagen haengt also
 // knapp ueber der Fahrbahn und faengt Bordsteine ab.
 const FVector Naben[Raeder] = {
  FVector(131,  79, -55), FVector(131, -79, -55),
  FVector(-131,  79, -55), FVector(-131, -79, -55)
 };
 const float Ruhe = 55.0f, Steifigkeit = 34000.0f, Daempfung = 2600.0f;
 int32 AmBoden = 0;

 for (int32 i = 0; i < Raeder; i++) {
  const FVector Ansatz = Lage.TransformPosition(Naben[i]);
  FHitResult Treffer;
  FCollisionQueryParams Fragen; Fragen.AddIgnoredActor(this);
  const FVector Ende = Ansatz - Oben * (Ruhe + 33.0f);
  if (!GetWorld()->LineTraceSingleByChannel(Treffer, Ansatz, Ende, ECC_Visibility, Fragen)) {
   Einfederung[i] = 0.0f;
   continue;
  }
  AmBoden++;
  const float Abstand = (Treffer.ImpactPoint - Ansatz).Size();
  const float Weg = FMath::Max(0.0f, (Ruhe + 33.0f) - Abstand);
  const float Geschwindigkeit = (Weg - Einfederung[i]) / FMath::Max(Zeit, 0.001f);
  Einfederung[i] = Weg;
  // Begrenzt auf das Dreifache der Radlast. Ohne Deckel warf ein Rad, das
  // auf einer Boeschung tief einfedert, den ganzen Wagen auf das Dach.
  const float Radlast = 1250.0f * 980.0f / Raeder;
  const float Kraft = FMath::Clamp(Weg * Steifigkeit + Geschwindigkeit * Daempfung,
                                   0.0f, Radlast * 3.0f);
  Rumpf->AddForceAtLocation(Oben * Kraft, Ansatz);
 }

 LetzteRaeder = AmBoden;
 if (AmBoden == 0) return;
 const float Anteil = static_cast<float>(AmBoden) / Raeder;

 // Lenkung nachziehen, nicht schlagartig setzen
 Lenkung = FMath::FInterpTo(Lenkung, LenkWert, Zeit, 6.0f);

 const FVector Tempo = Rumpf->GetPhysicsLinearVelocity();
 const float VorwaertsTempo = FVector::DotProduct(Tempo, Vorne);
 const float SeitTempo = FVector::DotProduct(Tempo, Rechts);

 // Antrieb, begrenzt auf rund 120 km/h
 const float Grenze = 3300.0f;
 if (!bBremse && FMath::Abs(GasWert) > 0.02f && FMath::Abs(VorwaertsTempo) < Grenze) {
  Rumpf->AddForce(Vorne * GasWert * 900000.0f * Anteil);
 }
 // Bremse und Rollwiderstand
 // Bremse stark, Motorbremse maessig, unter Gas nur Rollwiderstand.
 const float Verzoegerung = bBremse ? 5.0f : (FMath::Abs(GasWert) < 0.02f ? 0.30f : 0.06f);
 Rumpf->AddForce(-Vorne * VorwaertsTempo * 260.0f * Verzoegerung * Anteil);

 // Seitenfuehrung: ohne sie schwimmt der Wagen wie auf Eis
 Rumpf->AddForce(-Rechts * SeitTempo * 2600.0f * Anteil);

 // Parkbremse, solange niemand faehrt. Ohne sie rollte der abgestellte Wagen
 // am Hang von der Strasse in die Wiese, bevor man einsteigen konnte.
 if (!GetController()) Rumpf->AddForce(-Tempo * 1250.0f * 25.0f * Anteil);

 // Lenken wirkt nur, wenn der Wagen rollt - wie im Stand mit Servo aus
 const float Wirkung = FMath::Clamp(FMath::Abs(VorwaertsTempo) / 700.0f, 0.0f, 1.0f);
 const float Richtung = VorwaertsTempo >= 0 ? 1.0f : -1.0f;
 Rumpf->AddTorqueInRadians(Oben * Lenkung * Richtung * Wirkung * 5.4e8f * Anteil);

 // Motorklang: Leerlauf brummt leise und tief, Vollgas hoch und laut - aus
 // Gaspedal (sofort) und Tempo (traege) gemischt, wie eine Drehzahl.
 if (Motorklang && Motorklang->IsPlaying()) {
  const float Drehzahl = FMath::Clamp(FMath::Abs(GasWert) * 0.6f + FMath::Abs(VorwaertsTempo) / Grenze * 0.4f, 0.0f, 1.0f);
  Motorklang->SetPitchMultiplier(FMath::Lerp(0.6f, 1.8f, Drehzahl));
  Motorklang->SetVolumeMultiplier(FMath::Lerp(0.35f, 1.0f, Drehzahl));
 }
}
