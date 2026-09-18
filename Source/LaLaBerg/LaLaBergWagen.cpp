#include "LaLaBergWagen.h"
#include "LaLaBergHUD.h"
#include "Sound/SoundAttenuation.h"
#include "ProceduralMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
#include "LaLaBergWagenTypen.h"
#include "LaLaBergWagenRad.h"
#include "LaLaBergWagenBewegung.h"
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

 // Unsichtbare Kollisionsbox als Wurzel - Chaos braucht eine Mesh-Komponente
 // (kein reines Shape) als UpdatedComponent, sichtbar bleibt weiterhin nur
 // Netz bzw. CarConceptTeile unten an Karosseriepunkt. Mesh und Sichtbarkeit
 // setzt BeginPlay (siehe dort fuer den Grund).
 Rumpf = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rumpf"));
 SetRootComponent(Rumpf);

 Karosseriepunkt = CreateDefaultSubobject<USceneComponent>(TEXT("Karosseriepunkt"));
 Karosseriepunkt->SetupAttachment(Rumpf);
 // Der Wagen ruht auf der Federung: die Federstrahlen beginnen 55 cm unter
 // dem Kastenmittelpunkt und sind 88 cm lang; im Stand federt jedes Rad
 // 1250 kg * 9,8 / 4 / 34000 = 9 cm ein. Die Fahrbahn liegt also
 // 55 + 88 - 9 = 134 cm unter der Mitte. Mit den frueher angesetzten 88 cm
 // schwebte der Wagen sichtbar eine Handbreit ueber der Strasse.
 // The collision cube is scaled; imported visuals must retain centimetres.
 Karosseriepunkt->SetAbsolute(false, false, true);
 Karosseriepunkt->SetRelativeLocation(FVector(0, 0, -85.0f / 1.1f));

 Netz = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Karosserie"));
 Netz->SetupAttachment(Karosseriepunkt);
 Netz->SetCollisionEnabled(ECollisionEnabled::NoCollision);

 Ausleger = CreateDefaultSubobject<USpringArmComponent>(TEXT("Ausleger"));
 Ausleger->SetupAttachment(Rumpf);
 Ausleger->TargetArmLength = 620.0f;
 Ausleger->SetRelativeLocation(FVector(0, 0, 70));
 Ausleger->SetRelativeRotation(FRotator(-12, 0, 0));
 Ausleger->bEnableCameraLag = true;
 Ausleger->CameraLagSpeed = 6.0f;
 Ausleger->bUseCameraLagSubstepping = true;
 Ausleger->CameraLagMaxTimeStep = 1.0f / 60.0f;
 Ausleger->CameraLagMaxDistance = 90.0f;
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
 FSoundAttenuationSettings MotorRaum;
 MotorRaum.bAttenuate = true;
 MotorRaum.bSpatialize = true;
 MotorRaum.AttenuationShape = EAttenuationShape::Sphere;
 MotorRaum.AttenuationShapeExtents = FVector(650.0f, 0.0f, 0.0f);
 MotorRaum.FalloffDistance = 4500.0f;
 Motorklang->AdjustAttenuation(MotorRaum);

 // Chaos-Vehicle statt vier Federstrahlen: echtes Motor-/Getriebe-Kennfeld,
 // Vorderradlenkung, Hinterradantrieb, eigene Reifenreibung je Rad statt
 // einer pauschalen Seitenfuehrungskraft. Nabenpositionen laengs/quer wie
 // zuvor bei den Federstrahlen (131/-131, 79/-79). Die Hoehe (-17) ist NICHT
 // mehr vom alten Federstrahl-System uebernommen - die Einstiegsstelle (beim
 // Fahrtest wie beim normalen Einsteigen) setzt den Wagen so ab, dass die
 // Unterkante der Rumpf-Kollisionsbox (55 cm halbe Hoehe, siehe
 // SetRelativeScale3D) auf der Strasse aufsitzt. Die Nabe muss deshalb
 // relativ zu DIESER Boxhoehe sitzen, nicht an einer davon unabhaengigen
 // Zahl: -17 laesst das Rad (33 cm Radius, ±10 cm Federweg) zwischen -40 und
 // -60 reichen, die Fahrbahn bei -55 (Boxunterkante) liegt darin.
 Bewegung = CreateDefaultSubobject<ULaLaBergWagenBewegung>(TEXT("Bewegung"));
 // Space is a dedicated brake, never a request to reverse.
 Bewegung->bReverseAsBrake = false;
 Bewegung->WheelSetups.SetNum(4);
 Bewegung->WheelSetups[0].WheelClass = ULaLaBergRadVorn::StaticClass();
 Bewegung->WheelSetups[0].AdditionalOffset = FVector(131, 79, -50);
 Bewegung->WheelSetups[1].WheelClass = ULaLaBergRadVorn::StaticClass();
 Bewegung->WheelSetups[1].AdditionalOffset = FVector(131, -79, -50);
 Bewegung->WheelSetups[2].WheelClass = ULaLaBergRadHinten::StaticClass();
 Bewegung->WheelSetups[2].AdditionalOffset = FVector(-131, 79, -50);
 Bewegung->WheelSetups[3].WheelClass = ULaLaBergRadHinten::StaticClass();
 Bewegung->WheelSetups[3].AdditionalOffset = FVector(-131, -79, -50);

 Bewegung->Mass = 1250.0f;
 Bewegung->bEnableCenterOfMassOverride = true;
 // Tiefer Schwerpunkt, sonst kippt der Wagen in der ersten Kurve um -
 // derselbe Grund wie frueher bei Rumpf->SetCenterOfMass. Innerhalb der
 // 110 cm hohen Rumpf-Box (siehe SetRelativeScale3D), nicht darunter.
 Bewegung->CenterOfMassOverride = FVector(0, 0, -20);
 Bewegung->ChassisWidth = 176.0f;
 Bewegung->ChassisHeight = 110.0f;
 Bewegung->DragCoefficient = 0.35f;

 // 320 Nm waere ein realistischer Wert fuer diesen Wagen - hundertfach hoeher,
 // weil der Wagen sonst nicht faehrt: alle Rad-Werte (Kontakt, Federweg,
 // Reibung, Antriebsmoment) sahen per -LaLaBergFahrtest korrekt aus, die
 // Karosserie beschleunigte trotzdem nie; das Zehnfache reichte nicht, erst
 // das Hundertfache bewegt ihn. Die Ursache ist offen. Die frueher hier
 // stehende Erklaerung (Zentimeter-/Meter-Fehler bei DriveTorque / Re in
 // WheelSystem.cpp) war falsch: ChaosWheeledVehicleMovementComponent.cpp
 // rechnet das Moment vorher mit TorqueMToCm um, und die Bremse - die dieselbe
 // Rechnung durchlaeuft - braucht den Faktor nachweislich nicht (siehe
 // LaLaBergWagenRad.cpp und Docs/ChaosVehicle-ForumAnfrage.md).
 Bewegung->EngineSetup.MaxTorque = 32000.0f;
 Bewegung->EngineSetup.MaxRPM = 5500.0f;
 // Ohne eigene Kurve bleibt TorqueCurve leer - FillEngineSetup() teilt dann
 // durch den leeren Wertebereich (0) und liefert NaN-Drehmoment.
 FRichCurve* Drehmoment = Bewegung->EngineSetup.TorqueCurve.GetRichCurve();
 Drehmoment->AddKey(0.0f, 0.5f);
 Drehmoment->AddKey(1500.0f, 0.85f);
 Drehmoment->AddKey(3500.0f, 1.0f);
 Drehmoment->AddKey(5500.0f, 0.55f);

 Bewegung->DifferentialSetup.DifferentialType = EVehicleDifferential::RearWheelDrive;
 // Zur Sicherheit ausdruecklich statt auf Vorgabewerte zu vertrauen: die
 // Motordrehzahl blieb sonst exakt auf Standgas (EngineIdleRPM) haengen,
 // trotz Vollgas und eingelegtem ersten Gang (per -LaLaBergFahrtest FAIL
 // gefunden - Raeder hatten Bodenkontakt und ein geloggtes Antriebsmoment,
 // der Wagen bewegte sich trotzdem nicht).
 Bewegung->bMechanicalSimEnabled = true;
 Bewegung->bSuspensionEnabled = true;
 Bewegung->bWheelFrictionEnabled = true;
 // Die "aggressive Schlaflogik" (SleepThreshold=10 per Vorgabe) legte den
 // frisch erschienenen, fast unbewegten Wagen sofort wieder schlafen -
 // gegen das eigene WakeAllRigidBodies() in Tick() ein Tauziehen, in dem
 // die Schwerkraft nie genug ungestoerte Zeit bekam, um die Raeder auf den
 // Boden sinken zu lassen (federweg blieb per -LaLaBergFahrtest FAIL immer
 // bei 1.00). 0 schaltet sie ab.
 Bewegung->SleepThreshold = 0.0f;
}

void ALaLaBergWagen::BeginPlay() {
 Super::BeginPlay();
 // Physik erst am echten Actor konfigurieren. Im Konstruktor lief dieselbe
 // Folge auch fuer das Class Default Object, bevor GEngine bereit war; die
 // daraus entstehenden Materialfehler konnten ein echtes Fahrzeug mit
 // unvollstaendigen Body-Parametern hinterlassen.
 // Wuerfel nur als Kollisionshuelle (420x176x110 cm), nie sichtbar - die
 // eigentliche Karosserie zeigt Netz/CarConceptTeile an Karosseriepunkt.
 if (UStaticMesh* Wuerfel = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")))
  Rumpf->SetStaticMesh(Wuerfel);
 // 420x176x110 cm - dieselben Massse wie beim alten UBoxComponent-Rumpf.
 // Der eigentliche Fehler lag nicht in dieser Groesse, sondern darin, dass
 // die Radnaben (siehe Bewegung oben) einen von der Boxhoehe unabhaengigen
 // Fixwert (-101, aus dem alten Federstrahl-System) benutzten: die Einstiegs-
 // /Fahrtest-Platzierung setzt den Wagen so ab, dass die Box-Unterkante auf
 // der Strasse aufsitzt (per Kontrollstrahl bestaetigt: Fahrbahn traf exakt
 // Ursprung-minus-halbe-Boxhoehe) - bei -101 lagen die Naben dabei 60-75 cm
 // UNTER der Fahrbahn, die Raeder fanden nie Kontakt (federweg blieb per
 // -LaLaBergFahrtest FAIL immer bei 1.00, unabhaengig von jeder Boxgroesse,
 // die ich stattdessen anpasste). Jetzt sitzt die Nabenhoehe relativ zu
 // dieser Boxhoehe (siehe Kommentar oben bei Bewegung), nicht mehr isoliert.
 Rumpf->SetRelativeScale3D(FVector(4.2f, 1.76f, 1.1f));
 Rumpf->SetVisibility(false);
 Rumpf->SetCastShadow(false);
 // Eigenes Profil statt "PhysicsActor": WheelTraceCollisionResponses (siehe
 // Bewegung) ignoriert nur den Objekttyp "Vehicle" bei den Radstrahlen -
 // sonst traefen die eigenen Raeder den eigenen Rumpf.
 Rumpf->SetCollisionProfileName(TEXT("Vehicle"));
 Rumpf->SetSimulatePhysics(true);
 Rumpf->SetLinearDamping(0.04f);
 Rumpf->SetAngularDamping(3.5f);
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
 if (!CarConceptTeile.IsEmpty()) return;  // schon gebaut - faerbt sich nicht per SetzeLack um
 // Fahrzeugvielfalt wie bei den KI-Autos (siehe LaLaBergVerkehrsauto): einmal
 // zufaellig entweder CarConcept (-1) oder einer der realistischen City-
 // Sample-Typen. Einzelne, unpoolte Komponenten statt ALaLaBergAutoPool - nur
 // ein Wagen, kein Instanzieren noetig.
 if (FahrzeugTyp == -2) FahrzeugTyp = FMath::RandRange(-1, LaLaBergWagenTypen::TYPEN_ANZAHL - 1);
 if (FahrzeugTyp >= 0 && LaLaBergWagenTypen::BaueTeile(Karosseriepunkt, LaLaBergWagenTypen::TYPEN[FahrzeugTyp], CarConceptTeile)) {
  Karosseriepunkt->SetRelativeRotation(FRotator::ZeroRotator);
  Netz->SetVisibility(false);
  Netz->ClearAllMeshSections();
  return;
 }
 // Bevorzugt das lizenzierte CarConcept-Fahrzeug (CC BY 4.0, siehe
 // Content/SourceData/Vehicles/CarConcept-LICENSE.md) - deutlich mehr
 // Detail als die prozedurale Form. Das Rohmodell hat seine Laengsachse
 // (436 cm Ausdehnung) lokal auf Y statt auf X, deshalb die 90-Grad-
 // Drehung; ohne Zugriff auf einen Modell-Editor war das nur per
 // Testbild zu pruefen, nicht am Namen der Achsen abzulesen.
 FahrzeugTyp = -1;
 if (LaLaBergWagenForm::BaueCarConceptTeile(Karosseriepunkt, CarConceptTeile)) {
  Karosseriepunkt->SetRelativeRotation(FRotator(0, -90, 0));
  Netz->SetVisibility(false);
  Netz->ClearAllMeshSections();
  return;
 }
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
 GasWert = LenkWert = Lenkung = 0.0f;
 bBremse = bTest = false;
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
 const auto Meldung = [PC](const TCHAR* Text) {
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
 };
 // Einsteigen und Aussteigen liegen auf derselben Taste. Ohne diese Frist
 // koennte derselbe Tastendruck den Wagen gleich wieder verlassen.
 if (GetWorld()->GetTimeSeconds() - EinstiegZeit < 0.4f) {
  Meldung(TEXT("Kurz warten, dann E zum Aussteigen druecken."));
  return;
 }
 FCollisionQueryParams Fragen; Fragen.AddIgnoredActor(this); Fragen.AddIgnoredActor(Fahrer);
 // Do not eject the player at speed or into a wall. Check both sides and
 // the rear using the actual player capsule, not a fixed height.
 if (GetVelocity().SizeSquared() > FMath::Square(100.0f)) {
  Meldung(TEXT("Zum Aussteigen zuerst mit der Leertaste anhalten."));
  return;
 }
 const UCapsuleComponent* Kapsel = Fahrer->GetCapsuleComponent();
 const float Halbhoehe = Kapsel->GetScaledCapsuleHalfHeight();
 const FCollisionShape Form = FCollisionShape::MakeCapsule(Kapsel->GetScaledCapsuleRadius(), Halbhoehe);
 FVector Ziel = FVector::ZeroVector;
 bool bPlatz = false;
 for (const FVector& Versatz : { -GetActorRightVector() * 190.0f,
                                 GetActorRightVector() * 190.0f,
                                -GetActorForwardVector() * 320.0f }) {
  const FVector Neben = GetActorLocation() + Versatz;
  FHitResult Boden;
  if (!GetWorld()->LineTraceSingleByChannel(Boden, Neben + FVector(0,0,200),
       Neben - FVector(0,0,500), ECC_Visibility, Fragen)) continue;
  if (Boden.ImpactNormal.Z < Fahrer->GetCharacterMovement()->GetWalkableFloorZ()) continue;
  const FVector Kandidat = Boden.ImpactPoint + FVector(0,0,Halbhoehe + 3.0f);
  if (GetWorld()->OverlapBlockingTestByProfile(Kandidat, FQuat::Identity,
       Kapsel->GetCollisionProfileName(), Form, Fragen)) continue;
  FHitResult Hindernis;
  if (GetWorld()->SweepSingleByProfile(Hindernis, GetActorLocation(), Kandidat,
       FQuat::Identity, Kapsel->GetCollisionProfileName(), Form, Fragen)) continue;
  Ziel = Kandidat; bPlatz = true; break;
 }
 if (!bPlatz) {
  Meldung(TEXT("Kein sicherer Ausstieg. Bitte auf eine freie, ebene Stelle fahren."));
  return;
 }
 PC->UnPossess();
 Fahrer->SetActorLocation(Ziel, false, nullptr, ETeleportType::TeleportPhysics);
 Fahrer->SetActorHiddenInGame(false);
 Fahrer->SetActorEnableCollision(true);
 if (auto* FahrerBewegung = Fahrer->GetCharacterMovement()) FahrerBewegung->SetMovementMode(MOVE_Walking);
 PC->Possess(Fahrer);
 PC->SetControlRotation(FRotator(0, GetActorRotation().Yaw, 0));
 Meldung(TEXT("Ausgestiegen - WASD zum Gehen."));
 Fahrer = nullptr;
 GasWert = LenkWert = Lenkung = 0.0f;
 bBremse = bTest = false;
}

void ALaLaBergWagen::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!Bewegung || !Rumpf || !Rumpf->IsSimulatingPhysics()) return;

 // Lenkung nachziehen, nicht schlagartig setzen
 Lenkung = FMath::FInterpTo(Lenkung, LenkWert, Zeit, 6.0f);

 // Spurhalteassistenz: zwei Seitensonden knapp vor dem Wagen pruefen, ob
 // dort noch Fahrbahn liegt (Sektor-Actor mit Tag gleich dem Abschnitts-
 // namen, siehe ALaLaBergGameMode::LadeAusAssets "Actor->Tags.Add(*Klasse)")
 // - erkennt nur eine Seite die Strasse nicht mehr, schiebt eine sanfte
 // Lenkkorrektur zur anderen Seite zurueck. Wirkt nur, solange der Fahrer
 // selbst kaum lenkt (skaliert mit 1-|LenkWert|), uebersteuert also keine
 // absichtliche scharfe Kurve, und nur bei nennenswertem Tempo - im Stand
 // oder beim Rangieren waere die Vorausschau ohnehin bedeutungslos.
 if (GetController() && FMath::Abs(TempoKmh()) > 12.0f) {
  const FVector Ort = GetActorLocation();
  const FVector Vorwaerts = GetActorForwardVector();
  const FVector Rechts = GetActorRightVector();
  const FVector Voraus = Ort + Vorwaerts * 550.0f;
  auto AufFahrbahn = [this](const FVector& P) {
   FHitResult Treffer; FCollisionQueryParams Fragen; Fragen.AddIgnoredActor(this);
   if (!GetWorld()->LineTraceSingleByChannel(Treffer, P + FVector(0, 0, 150), P - FVector(0, 0, 400),
        ECC_Visibility, Fragen)) return false;
   if (!Treffer.GetActor()) return false;
   // Tag ist der volle Abschnittsname (z.B. "Road_111", siehe LadeAusAssets
   // "Actor->Tags.Add(*Klasse)") - kein exaktes "Road", deshalb Praefix-Test.
   for (const FName& Tag : Treffer.GetActor()->Tags)
    if (Tag.ToString().StartsWith(TEXT("Road"))) return true;
   return false;
  };
  const bool bLinksFrei = AufFahrbahn(Voraus - Rechts * 260.0f);
  const bool bRechtsFrei = AufFahrbahn(Voraus + Rechts * 260.0f);
  if (bLinksFrei != bRechtsFrei) {
   const float Korrektur = (bRechtsFrei ? -1.0f : 1.0f) * 0.22f * (1.0f - FMath::Abs(LenkWert));
   Lenkung = FMath::Clamp(Lenkung + Korrektur, -1.0f, 1.0f);
  }
 }

 const float VorwaertsTempo = Bewegung->GetForwardSpeed();
 const bool bRichtungswechsel = (GasWert > 0.02f && VorwaertsTempo < -100.0f)
                            || (GasWert < -0.02f && VorwaertsTempo > 100.0f);
 const bool bAnhalten = bBremse || bRichtungswechsel;
 if (!bAnhalten && FMath::Abs(GasWert) > 0.02f) {
  if (GasWert < 0.0f && Bewegung->GetTargetGear() >= 0)
   Bewegung->SetTargetGear(-1, true);
  else if (GasWert > 0.0f && Bewegung->GetTargetGear() <= 0)
   Bewegung->SetTargetGear(1, true);
 }
 Bewegung->SetThrottleInput(bAnhalten ? 0.0f : FMath::Abs(GasWert));
 Bewegung->SetSteeringInput(Lenkung);
 Bewegung->SetBrakeInput(bAnhalten ? 1.0f : 0.0f);
 // Handbremse statt Parkmodus, solange niemand faehrt - haelt den Wagen am
 // Hang, ohne (wie SetParked im Verdacht steht) auch bei aktivem Fahrer noch
 // nachzuwirken: trotz kontakt=1/federweg=0.75/drehmoment=985.9 blieb der
 // Wagen bei aktivem SetParked(false) unbeweglich (tempo/weg=0 per
 // -LaLaBergFahrtest FAIL, obwohl die Radphysik seit der Naben-Korrektur
 // korrekt aussah) - SetHandbrakeInput betrifft laut Konstruktor nur die
 // Hinterraeder und ist die etablierte, klar dokumentierte Bremse.
 Bewegung->SetHandbrakeInput(!GetController());
 // Ein ruhender Wagen schlaeft ein (Physik-Performance) und wacht nicht von
 // selbst auf, nur weil Gas anliegt - ohne dies stand der Wagen trotz Motor
 // still (per -LaLaBergFahrtest FAIL gefunden: simuliert=1 aber wach=0).
 if (GetController() && !Rumpf->RigidBodyIsAwake()) Rumpf->WakeAllRigidBodies();

 LetzteRaeder = 0;
 for (int32 i = 0; i < Bewegung->GetNumWheels(); i++)
  if (Bewegung->GetWheelState(i).bInContact) LetzteRaeder++;

 AktualisiereRaeder();

 // Motorklang: Leerlauf brummt leise und tief, Vollgas hoch und laut - aus
 // der tatsaechlichen Motordrehzahl statt einer Tempo-Naeherung.
 if (Motorklang && Motorklang->IsPlaying()) {
  const float Grenze = FMath::Max(Bewegung->GetEngineMaxRotationSpeed(), 1.0f);
  const float Drehzahl = FMath::Clamp(Bewegung->GetEngineRotationSpeed() / Grenze, 0.0f, 1.0f);
  Motorklang->SetPitchMultiplier(FMath::Lerp(0.6f, 1.8f, Drehzahl));
  Motorklang->SetVolumeMultiplier(FMath::Lerp(0.35f, 1.0f, Drehzahl));
 }
}

// Dreht/lenkt die sichtbaren CarConcept-Felgen nach dem Chaos-Radzustand -
// nur beim CarConcept-Modell (FahrzeugTyp -1) sind die Felgen eigene, klar
// benannte Teile (siehe LaLaBergWagenForm::CARCONCEPT_TEILE); die
// CitySample-Typen und die ProceduralMesh-Form backen die Raeder in ein
// Netz ein und bleiben deshalb ohne sichtbare Raddrehung.
void ALaLaBergWagen::AktualisiereRaeder() {
 if (FahrzeugTyp != -1 || CarConceptTeile.IsEmpty()) return;
 static const TCHAR* Namen[4] = { TEXT("WheelFrontLRim"), TEXT("WheelFrontRRim"),
                                  TEXT("WheelRearLRim"), TEXT("WheelRearRRim") };
 for (int32 i = 0; i < 4 && i < Bewegung->Wheels.Num(); i++) {
  UChaosVehicleWheel* Rad = Bewegung->Wheels[i];
  if (!Rad) continue;
  for (UStaticMeshComponent* Teil : CarConceptTeile) {
   if (!Teil || !Teil->GetName().Contains(Namen[i])) continue;
   Teil->SetRelativeRotation(FRotator(Rad->GetRotationAngle(), Rad->GetSteerAngle(), 0.0f));
   break;
  }
 }
}
