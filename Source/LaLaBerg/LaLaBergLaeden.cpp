#include "LaLaBergLaeden.h"
#include "LaLaBergKonto.h"
#include "LaLaBergPolizei.h"
#include "LaLaBergWagen.h"
#include "LaLaBergHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Materials/MaterialInstanceDynamic.h"

TWeakObjectPtr<ALaLaBergLaeden> ALaLaBergLaeden::Instanz;

namespace {
 constexpr float RADIUS = 700.0f, HOEHE_SPIEL = 500.0f;
 const FLinearColor GRUEN(0.1f, 0.85f, 0.35f);
}

ALaLaBergLaeden::ALaLaBergLaeden() {
 // Jedes Bild: sonst gehen Tastendruecke im Laden verloren.
 PrimaryActorTick.bCanEverTick = true;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

UStaticMeshComponent* ALaLaBergLaeden::BaueTeil(const FLinearColor& Farbe) {
 auto* Teil = NewObject<UStaticMeshComponent>(this);
 Teil->SetMobility(EComponentMobility::Movable);
 Teil->SetupAttachment(RootComponent);
 Teil->SetUsingAbsoluteLocation(true);
 Teil->SetUsingAbsoluteScale(true);
 Teil->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 Teil->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
 Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Teil->SetCastShadow(false);
 Teil->SetVisibility(false);
 Teil->RegisterComponent();
 if (auto* MID = Teil->CreateDynamicMaterialInstance(0)) MID->SetVectorParameterValue(TEXT("Color"), Farbe);
 Teile.Add(Teil);
 return Teil;
}

void ALaLaBergLaeden::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 // Die Waffen sind Paintball-Waffen (siehe LaLaBergWaffe) - der Laden also
 // ein Paintball-Laden, kein Waffengeschaeft.
 FLaden Paintball;
 Paintball.Name = TEXT("Paintball-Laden");
 Paintball.Waren = {
  { TEXT("Paintball-MP"), 300, EArt::Waffe, 1 },
  { TEXT("Paintball-Schrotflinte"), 600, EArt::Waffe, 2 },
  { TEXT("Paintball-Werfer"), 1500, EArt::Waffe, 3 },
 };
 FLaden Lack;
 Lack.Name = TEXT("Lackiererei");
 Lack.bImWagen = true;
 for (const auto& [Name, Farbe] : TArray<TPair<const TCHAR*, FLinearColor>>{
       { TEXT("Tiefschwarz"), FLinearColor(0.02f, 0.02f, 0.025f) }, { TEXT("Alpinweiss"), FLinearColor(0.85f, 0.86f, 0.86f) },
       { TEXT("Signalrot"), FLinearColor(0.6f, 0.03f, 0.03f) }, { TEXT("Enzianblau"), FLinearColor(0.03f, 0.1f, 0.5f) },
       { TEXT("Tannengruen"), FLinearColor(0.03f, 0.18f, 0.07f) }, { TEXT("Rapsgelb"), FLinearColor(0.8f, 0.6f, 0.03f) } })
  Lack.Waren.Add({ Name, 150, EArt::Lack, 0, Farbe });
 Laeden = { Paintball, Lack };
 for (FLaden& L : Laeden) { L.Ring = BaueTeil(GRUEN); L.Saeule = BaueTeil(GRUEN); }
}

void ALaLaBergLaeden::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

void ALaLaBergLaeden::Stelle(int32 Index, const FVector& Ort) {
 if (!Laeden.IsValidIndex(Index)) return;
 FLaden& L = Laeden[Index];
 L.Ort = Ort;
 L.bAufgestellt = true;
 const float D = RADIUS * 2.0f / 100.0f;
 L.Ring->SetWorldLocation(Ort + FVector(0, 0, 12));
 L.Ring->SetWorldScale3D(FVector(D, D, 0.08f));
 // Niedriger als die Auftragssaeulen (20 m statt 60 m): ein Laden ist
 // kein Ziel, das man ueber die Stadt hinweg suchen muss - dafuer die Karte.
 // Am Rand des Rings, nicht in der Mitte: im Laden steht man ja drin, und
 // die Kamera steckte sonst mitten in der Saeule.
 L.Saeule->SetWorldLocation(Ort + FVector(0, RADIUS + 150.0f, 1000));
 L.Saeule->SetWorldScale3D(FVector(1.2f, 1.2f, 20.0f));
 L.Ring->SetVisibility(true);
 L.Saeule->SetVisibility(true);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_LADEN %s bei %s"), *L.Name, *Ort.ToString());
}

bool ALaLaBergLaeden::HatSchon(const FWare& Ware) const {
 const ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto) return false;
 if (Ware.Art == EArt::Waffe) return Konto->HatWaffe(Ware.Waffe);
 FLinearColor Jetzt;
 return Konto->HatLack(Jetzt) && Jetzt.Equals(Ware.Farbe, 0.01f);
}

void ALaLaBergLaeden::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

bool ALaLaBergLaeden::Kaufe(int32 Index) {
 ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto || !Laeden.IsValidIndex(Offen) || !Laeden[Offen].Waren.IsValidIndex(Index)) return false;
 const FWare& Ware = Laeden[Offen].Waren[Index];
 if (Ware.Art == EArt::Waffe && HatSchon(Ware)) { Melde(FString::Printf(TEXT("%s hast du schon"), *Ware.Name)); return false; }
 if (!Konto->Bezahle(Ware.Preis)) {
  Melde(FString::Printf(TEXT("Zu wenig Geld – %s kostet %d €, du hast %d €"), *Ware.Name, Ware.Preis, Konto->HoleGeld()));
  return false;
 }
 if (Ware.Art == EArt::Waffe) {
  Konto->GibWaffe(Ware.Waffe);
  Melde(FString::Printf(TEXT("%s gekauft – Taste %d"), *Ware.Name, Ware.Waffe + 1));
 } else {
  auto* PC = GetWorld()->GetFirstPlayerController();
  if (auto* Wagen = Cast<ALaLaBergWagen>(PC ? PC->GetPawn() : nullptr)) Wagen->SetzeLack(Ware.Farbe, true);
  Konto->SetzeLack(Ware.Farbe);
  // Neu lackiert erkennt einen die Polizei nicht wieder - wenn sie gerade
  // nicht zusieht.
  ALaLaBergPolizei* Polizei = ALaLaBergPolizei::Instanz.Get();
  if (Polizei && Polizei->HoleSterne() > 0) {
   if (Polizei->WirdGesehen()) Melde(FString::Printf(TEXT("Neu lackiert in %s – aber die Polizei hat zugesehen"), *Ware.Name));
   else { Polizei->Verwische(); Melde(FString::Printf(TEXT("Neu lackiert in %s – die Polizei sucht jetzt einen anderen Wagen"), *Ware.Name)); }
  } else {
   Melde(FString::Printf(TEXT("Neu lackiert in %s"), *Ware.Name));
  }
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KAUF %s preis=%d geld=%d"), *Ware.Name, Ware.Preis, Konto->HoleGeld());
 return true;
}

void ALaLaBergLaeden::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 auto* PC = GetWorld()->GetFirstPlayerController();
 APawn* Figur = PC ? PC->GetPawn() : nullptr;
 if (!Figur) return;
 const FVector Wo = Figur->GetActorLocation();
 const bool bImWagen = Figur->IsA<ALaLaBergWagen>();
 int32 Drin = INDEX_NONE;
 for (int32 i = 0; i < Laeden.Num(); i++) {
  const FLaden& L = Laeden[i];
  if (L.bAufgestellt && FVector::Dist2D(Wo, L.Ort) < RADIUS && FMath::Abs(Wo.Z - L.Ort.Z) < HOEHE_SPIEL) Drin = i;
 }
 if (Drin == INDEX_NONE) { Offen = INDEX_NONE; bErstHinaus = false; return; }
 if (Offen == INDEX_NONE) {
  if (bErstHinaus) return;
  // Die Lackiererei nur mit dem Wagen, den Laden nur zu Fuss.
  if (Laeden[Drin].bImWagen != bImWagen) {
   Melde(Laeden[Drin].bImWagen ? TEXT("In die Lackiererei mit dem Wagen hineinfahren") : TEXT("Den Paintball-Laden zu Fuß betreten"));
   bErstHinaus = true;
   return;
  }
  Offen = Drin;
  Auswahl = 0;
  UE_LOG(LogTemp, Display, TEXT("LALABERG_LADEN offen %s"), *Laeden[Offen].Name);
 }
 const int32 Anzahl = Laeden[Offen].Waren.Num();
 if (PC->WasInputKeyJustPressed(EKeys::Up)) Auswahl = (Auswahl + Anzahl - 1) % Anzahl;
 if (PC->WasInputKeyJustPressed(EKeys::Down)) Auswahl = (Auswahl + 1) % Anzahl;
 if (PC->WasInputKeyJustPressed(EKeys::Enter)) Kaufe(Auswahl);
}
