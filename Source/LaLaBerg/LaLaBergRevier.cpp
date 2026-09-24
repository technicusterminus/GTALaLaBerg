#include "LaLaBergRevier.h"
#include "LaLaBergHUD.h"
#include "LaLaBergKonto.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergPolizei.h"
#include "LaLaBergVerkehrsauto.h"
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
 // Reihenfolge wie in Docs/Geschichte.md: erst die Gelben an der Bahn, dann
 // die Blauen am Lech, dann die Gruenen ums Klinikum, zuletzt die Weissen
 // in der Altstadt. Das eigene Viertel (Klinikum) faellt spaet - man nimmt
 // es den Gruenen ab, nicht umgekehrt.
 struct FVorgabe {
  const TCHAR* Revier; const TCHAR* Mannschaft; const TCHAR* Kopf;
  FLinearColor Farbe; float Leben; float Tempo; bool bPolizei;
  const TCHAR* Marken[4];
 };
 const FVorgabe VORGABEN[] = {
  { TEXT("Vorstadt-Nord"), TEXT("Die Gelben"), TEXT("Der Dispatcher"),
    FLinearColor(0.95f, 0.78f, 0.10f), 100.0f, 58.0f, false,
    { TEXT("Bahnhof"), TEXT("Polizeiinspektion"), TEXT("Sankt Katharina"), TEXT("Stadtverwaltung") } },
  { TEXT("Lechviertel"), TEXT("Die Blauen"), TEXT("Die Fährfrau"),
    FLinearColor(0.10f, 0.35f, 0.92f), 160.0f, 72.0f, false,
    { TEXT("Mutterturm"), TEXT("Johanniskirche"), TEXT("Färbertor"), TEXT("Stadttheater") } },
  { TEXT("Klinikum und Süd"), TEXT("Die Grünen"), TEXT("Der Förster"),
    FLinearColor(0.10f, 0.62f, 0.24f), 220.0f, 64.0f, true,
    { TEXT("Klinikum"), TEXT("kbo-Lech-Mangfall-Klinik"), TEXT("Christuskirche"), TEXT("Friedhofskirche zur Heiligen Dreifaltigkeit") } },
  { TEXT("Altstadt"), TEXT("Die Weißen"), TEXT("Der Wirt vom Hauptplatz"),
    FLinearColor(0.92f, 0.92f, 0.90f), 300.0f, 68.0f, true,
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
  R.Kopf = V.Kopf;
  R.Farbe = V.Farbe;
  R.Leben = V.Leben;
  R.Tempo = V.Tempo;
  R.bRuftPolizei = V.bPolizei;
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
 // Markiert ist erst die halbe Miete: jetzt kommt der Kopf der Mannschaft.
 RufeKopf(Revier);
}

FString ALaLaBergRevier::HoleKopfName() const {
 return Reviere.IsValidIndex(KopfRevier) ? Reviere[KopfRevier].Kopf : FString();
}

FVector ALaLaBergRevier::HoleKopfOrt() const {
 return Kopf.IsValid() ? Kopf->GetActorLocation() : FVector::ZeroVector;
}

// Der Kopf ist ein fahrender Wagen in der Farbe der Mannschaft: weit genug
// weg, um ihn suchen zu muessen, zaeh genug, um ihn nicht mit drei Treffern
// zu erledigen. Der Foerster und der Wirt rufen zusaetzlich die Polizei -
// sie haben sie auf ihrer Seite.
void ALaLaBergRevier::RufeKopf(int32 Revier) {
 if (!Reviere.IsValidIndex(Revier) || Kopf.IsValid()) return;
 const FRevier& R = Reviere[Revier];
 auto* PC = GetWorld()->GetFirstPlayerController();
 const FVector Wo = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
 ALaLaBergVerkehrsauto* Beste = nullptr;
 float BesteD = 0.0f;
 for (ALaLaBergVerkehrsauto* Auto : ALaLaBergVerkehrsauto::Alle) {
  if (!Auto || Auto->IstGeparkt() || Auto->IstAusgeschaltet()) continue;
  const float D = FVector::Dist2D(Auto->GetActorLocation(), Wo);
  if (D < 20000.0f || D > 150000.0f) continue;
  if (D > BesteD) { BesteD = D; Beste = Auto; }
 }
 if (!Beste) {
  Melde(TEXT("Kein Wagen der Mannschaft unterwegs – später noch einmal versuchen"));
  return;
 }
 Kopf = Beste;
 KopfRevier = Revier;
 KopfFrist = GetWorld()->GetTimeSeconds() + 300.0;
 Beste->SetzeLack(R.Farbe);
 Beste->SetzeLeben(R.Leben);
 if (R.bRuftPolizei)
  if (auto* Polizei = ALaLaBergPolizei::Instanz.Get()) Polizei->TestSetzeSterne(2);
 Melde(FString::Printf(TEXT("%s ist unterwegs – stell ihn, dann gehört dir %s"), *R.Kopf, *R.Name));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KOPF %s revier=%d leben=%.0f abstand=%.0fm"),
        *R.Kopf, Revier, R.Leben, BesteD / 100.0f);
}

void ALaLaBergRevier::KopfGestellt(int32 Revier) {
 auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto || !Reviere.IsValidIndex(Revier)) return;
 Konto->NimmRevier(Revier);
 Konto->SetzeKapitel(Revier + 2);
 Konto->Uebe(ULaLaBergKonto::EWert::Ruf, 120.0f);
 Konto->Gutschrift(800);
 // Was der Kopf hinterlaesst - kein Gegenstand, ein Schalter in der Welt.
 static const TCHAR* FUNDNAMEN[] = { TEXT("den Werkstattschlüssel"), TEXT("den Rotorschlüssel"),
                                     TEXT("den Zündschlüssel"), TEXT("die Stadt") };
 if (Revier < 3) Konto->GibFund(static_cast<ULaLaBergKonto::EFund>(Revier));
 Melde(FString::Printf(TEXT("%s gestellt – %s gehört dir, und du hast %s"), *Reviere[Revier].Kopf,
                       *Reviere[Revier].Name, FUNDNAMEN[FMath::Min(Revier, 3)]));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KOPF gestellt revier=%d kapitel=%d funde=%d"),
        Revier, Konto->HoleKapitel(), Konto->HatFund(ULaLaBergKonto::EFund::Werkstatt) ? 1 : 0);
 Kopf.Reset();
 KopfRevier = INDEX_NONE;
}

void ALaLaBergRevier::TestStelleKopf() {
 if (!Kopf.IsValid()) return;
 Kopf->Verletze(9999.0f, FVector(1, 0, 0), ELaLaBergSchaden::Beschuss);
 // Sofort abrechnen statt auf den naechsten Tick zu warten - ein Test
 // liest das Ergebnis im selben Bild.
 if (Kopf->IstAusgeschaltet() && Reviere.IsValidIndex(KopfRevier)) KopfGestellt(KopfRevier);
}

void ALaLaBergRevier::TestMarkiereAlle() {
 const int32 Offen = HoleOffenes();
 if (Offen == INDEX_NONE) return;
 for (FMarke& M : Reviere[Offen].Marken) if (M.Saeule) Markiere(M.Saeule, FLinearColor(0.95f, 0.45f, 0.05f));
}

void ALaLaBergRevier::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!bGeladen) return;
 // Laeuft eine Jagd? Der Kopf gilt als gestellt, sobald sein Wagen steht.
 if (Kopf.IsValid() && Reviere.IsValidIndex(KopfRevier)) {
  if (Kopf->IstAusgeschaltet()) KopfGestellt(KopfRevier);
  else if (GetWorld()->GetTimeSeconds() > KopfFrist) {
   Melde(FString::Printf(TEXT("%s ist entkommen – die Wahrzeichen bleiben markiert"), *Reviere[KopfRevier].Kopf));
   Kopf.Reset();
   KopfRevier = INDEX_NONE;
  }
 } else if (!Kopf.IsValid() && KopfRevier != INDEX_NONE) {
  KopfRevier = INDEX_NONE;
 }
 // Nur die Saeulen des offenen Reviers stehen sichtbar in der Stadt - die
 // spaeteren Kapitel sollen nicht von Anfang an als Wald von Saeulen
 // herumstehen.
 const int32 Offen = HoleOffenes();
 for (int32 i = 0; i < Reviere.Num(); i++)
  for (const FMarke& M : Reviere[i].Marken)
   for (UStaticMeshComponent* Teil : { M.Saeule, M.Ring })
    if (Teil) Teil->SetVisibility(i == Offen || M.bMarkiert);
}
