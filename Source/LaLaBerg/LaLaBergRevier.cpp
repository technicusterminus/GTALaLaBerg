#include "LaLaBergRevier.h"
#include "LaLaBergHUD.h"
#include "LaLaBergKonto.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TWeakObjectPtr<ALaLaBergRevier> ALaLaBergRevier::Instanz;

namespace {
 // Die Saeule steht am Wahrzeichen: 30 m hoch, damit man sie ueber die
 // Daecher sieht, und 1,6 m dick, damit man sie aus 50 m trifft.
 constexpr float SAEULE_HOCH = 3000.0f, SAEULE_DICK = 1.6f;

 // Welches Wahrzeichen zu welchem Revier gehoert. Die Namen stehen so in
 // orte.json (siehe Docs/Geschichte.md); was dort fehlt, wird still
 // uebersprungen - die Daten sind amtlich und aendern sich.
 struct FVorgabe { const TCHAR* Revier; const TCHAR* Mannschaft; FLinearColor Farbe; const TCHAR* Marken[4]; };
 const FVorgabe VORGABEN[] = {
  { TEXT("Klinikum und Süd"), TEXT("Die Grünen"), FLinearColor(0.10f, 0.62f, 0.24f),
    { TEXT("Klinikum"), TEXT("kbo-Lech-Mangfall-Klinik"), TEXT("Christuskirche"), TEXT("Friedhofskirche zur Heiligen Dreifaltigkeit") } },
  { TEXT("Vorstadt-Nord"), TEXT("Die Gelben"), FLinearColor(0.95f, 0.78f, 0.10f),
    { TEXT("Bahnhof"), TEXT("Polizeiinspektion"), TEXT("Sankt Katharina"), TEXT("Stadtverwaltung") } },
  { TEXT("Lechviertel"), TEXT("Die Blauen"), FLinearColor(0.10f, 0.35f, 0.92f),
    { TEXT("Mutterturm"), TEXT("Johanniskirche"), TEXT("Färbertor"), TEXT("Stadttheater") } },
  { TEXT("Altstadt"), TEXT("Die Weißen"), FLinearColor(0.92f, 0.92f, 0.90f),
    { TEXT("Historisches Rathaus"), TEXT("Schmalzturm"), TEXT("Bayertor"), TEXT("Stadtpfarrkirche Mariae Himmelfahrt") } },
 };
}

ALaLaBergRevier::ALaLaBergRevier() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.5f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

void ALaLaBergRevier::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 LadeMarken();
}

void ALaLaBergRevier::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

// Die Orte kommen aus derselben Datei wie die Auftragsziele. Gesucht wird
// nach Namen; doppelte Namen (es gibt drei "Klinikum") nimmt der erste.
void ALaLaBergRevier::LadeMarken() {
 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Orte/orte.json");
 if (!FFileHelper::LoadFileToString(Text, *Datei) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_REVIER keine Orte: %s"), *Datei);
  return;
 }
 auto* Wuerfel = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 // Durchscheinend wie die Auftrags- und Ladensaeulen (Tools/baue_saeule.py).
 auto* Basis = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Saeule.M_Saeule"));
 if (!Basis) Basis = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 const auto& Marken = Wurzel->GetArrayField(TEXT("marken"));
 for (const FVorgabe& V : VORGABEN) {
  FRevier R;
  R.Name = V.Revier;
  R.Mannschaft = V.Mannschaft;
  R.Farbe = V.Farbe;
  for (const TCHAR* Gesucht : V.Marken) {
   for (const auto& Wert : Marken) {
    const auto Obj = Wert->AsObject();
    if (!Obj.IsValid() || Obj->GetStringField(TEXT("n")) != Gesucht) continue;
    FMarke M;
    M.Name = Gesucht;
    M.Ort = FVector(Obj->GetNumberField(TEXT("x")), Obj->GetNumberField(TEXT("y")), 0.0f);
    // Hoehe des Bodens: der Strahl faengt hoch genug an, um ueber jedes
    // Dach zu kommen.
    FHitResult Boden;
    const FVector Oben(M.Ort.X, M.Ort.Y, 30000.0f);
    if (GetWorld()->LineTraceSingleByChannel(Boden, Oben, Oben - FVector(0, 0, 60000.0f), ECC_Visibility))
     M.Ort.Z = Boden.ImpactPoint.Z;
    if (Wuerfel) {
     auto* Saeule = NewObject<UStaticMeshComponent>(this);
     Saeule->SetMobility(EComponentMobility::Movable);
     Saeule->SetupAttachment(RootComponent);
     Saeule->SetUsingAbsoluteLocation(true);
     Saeule->SetUsingAbsoluteScale(true);
     Saeule->SetStaticMesh(Wuerfel);
     Saeule->SetWorldLocation(M.Ort + FVector(0, 0, SAEULE_HOCH * 0.5f));
     Saeule->SetWorldScale3D(FVector(SAEULE_DICK, SAEULE_DICK, SAEULE_HOCH / 100.0f));
     // Die Saeule muss getroffen werden koennen - sonst waere sie Deko.
     Saeule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
     Saeule->SetCollisionProfileName(TEXT("BlockAllDynamic"));
     Saeule->SetCastShadow(false);
     Saeule->RegisterComponent();
     if (Basis) if (auto* MID = Saeule->CreateDynamicMaterialInstance(0, Basis))
      MID->SetVectorParameterValue(TEXT("Color"), R.Farbe);
     M.Saeule = Saeule;
     Teile.Add(Saeule);
     // Ring am Fuss, wie bei Auftrag und Laden: von nahem sieht man die
     // Saeule sonst erst, wenn man den Kopf hebt.
     auto* Ring = NewObject<UStaticMeshComponent>(this);
     Ring->SetMobility(EComponentMobility::Movable);
     Ring->SetupAttachment(RootComponent);
     Ring->SetUsingAbsoluteLocation(true);
     Ring->SetUsingAbsoluteScale(true);
     Ring->SetStaticMesh(Wuerfel);
     Ring->SetWorldLocation(M.Ort + FVector(0, 0, 12.0f));
     Ring->SetWorldScale3D(FVector(14.0f, 14.0f, 0.08f));
     Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
     Ring->SetCastShadow(false);
     Ring->RegisterComponent();
     if (auto* Grund = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
      if (auto* MID = Ring->CreateDynamicMaterialInstance(0, Grund)) MID->SetVectorParameterValue(TEXT("Color"), R.Farbe);
     M.Ring = Ring;
     Teile.Add(Ring);
    }
    R.Marken.Add(M);
    break;
   }
  }
  Reviere.Add(R);
  UE_LOG(LogTemp, Display, TEXT("LALABERG_REVIER %s (%s) marken=%d"), *R.Name, *R.Mannschaft, R.Marken.Num());
 }
 bGeladen = true;
 // Schon uebernommene Reviere faerben ihre Saeulen sofort um.
 if (const auto* Konto = ULaLaBergKonto::Hole(this))
  for (int32 i = 0; i < Reviere.Num(); i++)
   if (Konto->HatRevier(i))
    for (FMarke& M : Reviere[i].Marken) {
     M.bMarkiert = true;
     for (UStaticMeshComponent* Teil : { M.Saeule, M.Ring })
      if (Teil) if (auto* MID = Cast<UMaterialInstanceDynamic>(Teil->GetMaterial(0)))
       MID->SetVectorParameterValue(TEXT("Color"), FLinearColor(0.95f, 0.45f, 0.05f));
    }
}

int32 ALaLaBergRevier::HoleOffenes() const {
 const auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto) return INDEX_NONE;
 // Der Reihe nach: das erste Revier, das einem noch nicht gehoert.
 for (int32 i = 0; i < Reviere.Num(); i++) if (!Konto->HatRevier(i)) return i;
 return INDEX_NONE;
}

int32 ALaLaBergRevier::HoleMarkiert(int32 Revier) const {
 if (!Reviere.IsValidIndex(Revier)) return 0;
 int32 Zahl = 0;
 for (const FMarke& M : Reviere[Revier].Marken) if (M.bMarkiert) Zahl++;
 return Zahl;
}

void ALaLaBergRevier::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

void ALaLaBergRevier::Markiere(UStaticMeshComponent* Saeule, const FLinearColor& Farbe) {
 const int32 Offen = HoleOffenes();
 if (Offen == INDEX_NONE || !Reviere.IsValidIndex(Offen)) return;
 for (FMarke& M : Reviere[Offen].Marken) {
  if (M.Saeule != Saeule || M.bMarkiert) continue;
  M.bMarkiert = true;
  for (UStaticMeshComponent* Teil : { M.Saeule, M.Ring })
   if (Teil) if (auto* MID = Cast<UMaterialInstanceDynamic>(Teil->GetMaterial(0)))
    MID->SetVectorParameterValue(TEXT("Color"), Farbe);
  const int32 Zahl = HoleMarkiert(Offen);
  Melde(FString::Printf(TEXT("%s markiert – %d von %d im Revier %s"), *M.Name, Zahl,
                        Reviere[Offen].Marken.Num(), *Reviere[Offen].Name));
  UE_LOG(LogTemp, Display, TEXT("LALABERG_MARKE %s revier=%s stand=%d/%d"), *M.Name, *Reviere[Offen].Name,
         Zahl, Reviere[Offen].Marken.Num());
  PruefeUebernahme(Offen);
  return;
 }
}

// Alle Wahrzeichen markiert und genug Ruf: das Revier gehoert dem Spieler.
// Der Ruf ist die Bewaehrung aus der Geschichte - man muss in der Stadt
// gearbeitet haben, nicht nur vier Saeulen angemalt.
void ALaLaBergRevier::PruefeUebernahme(int32 Revier) {
 auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto || !Reviere.IsValidIndex(Revier)) return;
 if (HoleMarkiert(Revier) < Reviere[Revier].Marken.Num()) return;
 const int32 NoetigerRuf = Revier * 150;
 if (Konto->HoleWert(ULaLaBergKonto::EWert::Ruf) < NoetigerRuf) {
  Melde(FString::Printf(TEXT("%s markiert – aber %s nehmen dich noch nicht ernst (Ruf %d von %d)"),
                        *Reviere[Revier].Name, *Reviere[Revier].Mannschaft,
                        Konto->HoleWert(ULaLaBergKonto::EWert::Ruf), NoetigerRuf));
  return;
 }
 Konto->NimmRevier(Revier);
 Konto->SetzeKapitel(Revier + 2);
 Konto->Uebe(ULaLaBergKonto::EWert::Ruf, 120.0f);
 Melde(FString::Printf(TEXT("%s gehört jetzt dir – %s sind erledigt"), *Reviere[Revier].Name,
                       *Reviere[Revier].Mannschaft));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_REVIER uebernommen=%d kapitel=%d"), Revier, Konto->HoleKapitel());
}

void ALaLaBergRevier::TestMarkiereAlle() {
 const int32 Offen = HoleOffenes();
 if (Offen == INDEX_NONE) return;
 for (FMarke& M : Reviere[Offen].Marken) if (M.Saeule) Markiere(M.Saeule, FLinearColor(0.95f, 0.45f, 0.05f));
}

void ALaLaBergRevier::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!bGeladen) return;
 // Nur die Saeulen des offenen Reviers stehen sichtbar in der Stadt - die
 // spaeteren Kapitel sollen nicht von Anfang an als Wald von Saeulen
 // herumstehen.
 const int32 Offen = HoleOffenes();
 for (int32 i = 0; i < Reviere.Num(); i++)
  for (const FMarke& M : Reviere[i].Marken)
   for (UStaticMeshComponent* Teil : { M.Saeule, M.Ring })
    if (Teil) Teil->SetVisibility(i == Offen || M.bMarkiert);
}
