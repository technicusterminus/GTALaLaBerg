#include "LaLaBergDrehbuch.h"
#include "LaLaBergCharacter.h"
#include "LaLaBergHUD.h"
#include "LaLaBergKonto.h"
#include "LaLaBergPolizei.h"
#include "LaLaBergRevier.h"
#include "LaLaBergVerkehrsauto.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergWagen.h"
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

TWeakObjectPtr<ALaLaBergDrehbuch> ALaLaBergDrehbuch::Instanz;

namespace {
 // Wie nah man einem Ort kommen muss und wie hoch die Startsaeule ist.
 constexpr float ZIEL_RADIUS = 1400.0f, HOEHE_SPIEL = 900.0f;
 constexpr float SAEULE_HOCH = 2400.0f;
 const FLinearColor GRUEN(0.12f, 0.85f, 0.42f);

 ELaLaBergStufe ArtAus(const FString& Wort) {
  if (Wort == TEXT("markiere")) return ELaLaBergStufe::Markiere;
  if (Wort == TEXT("stelle")) return ELaLaBergStufe::Stelle;
  if (Wort == TEXT("bring")) return ELaLaBergStufe::Bring;
  if (Wort == TEXT("halte")) return ELaLaBergStufe::Halte;
  return ELaLaBergStufe::Hin;
 }
}

ALaLaBergDrehbuch::ALaLaBergDrehbuch() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.2f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

void ALaLaBergDrehbuch::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 LadeDrehbuch();
 SucheAngebot();
}

void ALaLaBergDrehbuch::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

// Orte kommen wie ueberall aus orte.json; die Missionen selbst aus
// Story/missionen.json. Fehlt ein Ort, faellt die Stufe auf den Startort
// zurueck - lieber eine Mission, die zu leicht ist, als eine, die haengt.
void ALaLaBergDrehbuch::LadeDrehbuch() {
 TMap<FString, FVector> Orte;
 {
  FString Text;
  TSharedPtr<FJsonObject> Wurzel;
  const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Orte/orte.json");
  if (FFileHelper::LoadFileToString(Text, *Datei) &&
      FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) && Wurzel.IsValid())
   for (const auto& Wert : Wurzel->GetArrayField(TEXT("marken"))) {
    const auto Obj = Wert->AsObject();
    if (!Obj.IsValid()) continue;
    const FString Name = Obj->GetStringField(TEXT("n"));
    if (!Orte.Contains(Name))
     Orte.Add(Name, FVector(Obj->GetNumberField(TEXT("x")), Obj->GetNumberField(TEXT("y")), 0.0f));
   }
 }
 // Hoehe je Ort: ein Strahl von oben auf die Stadt.
 auto AufDenBoden = [this](FVector Ort) {
  FHitResult Boden;
  const FVector Oben(Ort.X, Ort.Y, 30000.0f);
  if (GetWorld()->LineTraceSingleByChannel(Boden, Oben, Oben - FVector(0, 0, 60000.0f), ECC_Visibility))
   Ort.Z = Boden.ImpactPoint.Z;
  return Ort;
 };

 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Story/missionen.json");
 if (!FFileHelper::LoadFileToString(Text, *Datei) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_DREHBUCH keine Missionen: %s"), *Datei);
  return;
 }
 for (const auto& Wert : Wurzel->GetArrayField(TEXT("missionen"))) {
  const auto Obj = Wert->AsObject();
  if (!Obj.IsValid()) continue;
  FMission M;
  M.Id = Obj->GetStringField(TEXT("id"));
  M.Name = Obj->GetStringField(TEXT("name"));
  M.StartName = Obj->GetStringField(TEXT("start"));
  M.Kapitel = Obj->GetIntegerField(TEXT("kapitel"));
  M.Lohn = Obj->GetIntegerField(TEXT("lohn"));
  M.Ruf = Obj->GetIntegerField(TEXT("ruf"));
  if (const FVector* Gefunden = Orte.Find(M.StartName)) M.Start = AufDenBoden(*Gefunden);
  for (const auto& SWert : Obj->GetArrayField(TEXT("stufen"))) {
   const auto SObj = SWert->AsObject();
   if (!SObj.IsValid()) continue;
   FStufe S;
   S.Art = ArtAus(SObj->GetStringField(TEXT("art")));
   SObj->TryGetStringField(TEXT("ort"), S.OrtName);
   SObj->TryGetStringField(TEXT("text"), S.Text);
   double Zahl = 0.0;
   if (SObj->TryGetNumberField(TEXT("zeit"), Zahl)) S.Zeit = static_cast<float>(Zahl);
   if (SObj->TryGetNumberField(TEXT("sterne"), Zahl)) S.Sterne = static_cast<int32>(Zahl);
   SObj->TryGetBoolField(TEXT("imWagen"), S.bImWagen);
   const FVector* Gefunden = Orte.Find(S.OrtName);
   S.Ort = Gefunden ? AufDenBoden(*Gefunden) : M.Start;
   M.Stufen.Add(S);
  }
  Missionen.Add(M);
 }
 bGeladen = true;
 UE_LOG(LogTemp, Display, TEXT("LALABERG_DREHBUCH missionen=%d"), Missionen.Num());
}

// Die naechste Mission des laufenden Kapitels, die noch nicht geschafft ist.
// Ihr Startort bekommt eine gruene Saeule - blau ist der Lieferauftrag.
void ALaLaBergDrehbuch::SucheAngebot() {
 Angebot = INDEX_NONE;
 const auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto || !bGeladen) return;
 const int32 Kapitel = FMath::Max(1, Konto->HoleKapitel() + 1);
 for (int32 i = 0; i < Missionen.Num(); i++) {
  if (Missionen[i].Kapitel != Kapitel || Konto->HatMission(i)) continue;
  Angebot = i;
  break;
 }
 // Saeule und Ring bei Bedarf bauen, sonst nur umsetzen.
 if (Angebot == INDEX_NONE) {
  if (StartRing) StartRing->SetVisibility(false);
  if (StartSaeule) StartSaeule->SetVisibility(false);
  return;
 }
 auto* Zylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
 auto BaueTeil = [&](TObjectPtr<UStaticMeshComponent>& Ziel, bool bSaeule) {
  if (Ziel || !Zylinder) return;
  auto* Teil = NewObject<UStaticMeshComponent>(this);
  Teil->SetMobility(EComponentMobility::Movable);
  Teil->SetupAttachment(RootComponent);
  Teil->SetUsingAbsoluteLocation(true);
  Teil->SetUsingAbsoluteScale(true);
  Teil->SetStaticMesh(Zylinder);
  Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Teil->SetCastShadow(false);
  auto* Stoff = bSaeule ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Saeule.M_Saeule")) : nullptr;
  if (!Stoff) Stoff = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
  Teil->SetMaterial(0, Stoff);
  Teil->RegisterComponent();
  if (auto* MID = Teil->CreateDynamicMaterialInstance(0)) MID->SetVectorParameterValue(TEXT("Color"), GRUEN);
  Ziel = Teil;
 };
 BaueTeil(StartRing, false);
 BaueTeil(StartSaeule, true);
 const FVector Ort = Missionen[Angebot].Start;
 if (StartRing) {
  StartRing->SetWorldLocation(Ort + FVector(0, 0, 12));
  StartRing->SetWorldScale3D(FVector(13.0f, 13.0f, 0.08f));
  StartRing->SetVisibility(true);
 }
 if (StartSaeule) {
  StartSaeule->SetWorldLocation(Ort + FVector(0, 0, SAEULE_HOCH * 0.5f));
  StartSaeule->SetWorldScale3D(FVector(1.3f, 1.3f, SAEULE_HOCH / 100.0f));
  StartSaeule->SetVisibility(true);
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_DREHBUCH angebot=%s bei %s"), *Missionen[Angebot].Id, *Missionen[Angebot].StartName);
}

FString ALaLaBergDrehbuch::HoleMissionsname() const {
 return Missionen.IsValidIndex(AktMission) ? Missionen[AktMission].Name : FString();
}
FString ALaLaBergDrehbuch::HoleAngebotsname() const {
 return Missionen.IsValidIndex(Angebot) ? Missionen[Angebot].Name : FString();
}
FVector ALaLaBergDrehbuch::HoleAngebotsort() const {
 return Missionen.IsValidIndex(Angebot) ? Missionen[Angebot].Start : FVector::ZeroVector;
}
int32 ALaLaBergDrehbuch::HoleStufen() const {
 return Missionen.IsValidIndex(AktMission) ? Missionen[AktMission].Stufen.Num() : 0;
}
FString ALaLaBergDrehbuch::HoleStufentext() const {
 if (!Missionen.IsValidIndex(AktMission) || !Missionen[AktMission].Stufen.IsValidIndex(AktStufe)) return FString();
 return Missionen[AktMission].Stufen[AktStufe].Text;
}
FVector ALaLaBergDrehbuch::HoleStufenort() const {
 if (!Missionen.IsValidIndex(AktMission) || !Missionen[AktMission].Stufen.IsValidIndex(AktStufe)) return FVector::ZeroVector;
 const FStufe& S = Missionen[AktMission].Stufen[AktStufe];
 if (S.Art == ELaLaBergStufe::Stelle) return Gegner.IsValid() ? Gegner->GetActorLocation() : S.Ort;
 return S.Ort;
}
float ALaLaBergDrehbuch::HoleRestzeit() const {
 return Frist > 0.0 ? FMath::Max(0.0f, static_cast<float>(Frist - GetWorld()->GetTimeSeconds())) : 0.0f;
}

void ALaLaBergDrehbuch::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

void ALaLaBergDrehbuch::Starte(int32 Mission) {
 if (!Missionen.IsValidIndex(Mission)) return;
 AktMission = Mission;
 AktStufe = -1;
 if (StartRing) StartRing->SetVisibility(false);
 if (StartSaeule) StartSaeule->SetVisibility(false);
 Melde(FString::Printf(TEXT("%s – los geht's"), *Missionen[Mission].Name));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_MISSION start %s stufen=%d"), *Missionen[Mission].Id,
        Missionen[Mission].Stufen.Num());
 NaechsteStufe();
}

void ALaLaBergDrehbuch::NaechsteStufe() {
 const FMission& M = Missionen[AktMission];
 AktStufe++;
 if (!M.Stufen.IsValidIndex(AktStufe)) { Vollende(); return; }
 const FStufe& S = M.Stufen[AktStufe];
 Frist = S.Zeit > 0.0f && S.Art != ELaLaBergStufe::Halte ? GetWorld()->GetTimeSeconds() + S.Zeit : 0.0;
 HalteBis = S.Art == ELaLaBergStufe::Halte ? GetWorld()->GetTimeSeconds() + S.Zeit : 0.0;
 Gegner.Reset();
 if (S.Art == ELaLaBergStufe::Stelle && !SucheGegner()) {
  // Kein Wagen da: Stufe ueberspringen, statt die Mission haengen zu lassen.
  Melde(TEXT("Niemand unterwegs – weiter"));
  NaechsteStufe();
  return;
 }
 if (S.Sterne > 0)
  if (auto* Polizei = ALaLaBergPolizei::Instanz.Get()) Polizei->TestSetzeSterne(S.Sterne);
 Melde(S.Text);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_MISSION stufe %d/%d art=%d ort=%s"), AktStufe + 1, M.Stufen.Num(),
        static_cast<int32>(S.Art), *S.OrtName);
}

bool ALaLaBergDrehbuch::SucheGegner() {
 auto* PC = GetWorld()->GetFirstPlayerController();
 const FVector Wo = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
 ALaLaBergVerkehrsauto* Beste = nullptr;
 float BesteD = TNumericLimits<float>::Max();
 for (ALaLaBergVerkehrsauto* Auto : ALaLaBergVerkehrsauto::Alle) {
  if (!Auto || Auto->IstGeparkt() || Auto->IstAusgeschaltet()) continue;
  const float D = FVector::Dist2D(Auto->GetActorLocation(), Wo);
  if (D < 8000.0f || D > 90000.0f) continue;
  if (D < BesteD) { BesteD = D; Beste = Auto; }
 }
 if (!Beste) return false;
 Gegner = Beste;
 Beste->SetzeLack(FLinearColor(0.95f, 0.12f, 0.45f));
 Beste->SetzeLeben(120.0f);
 return true;
}

// Ist die Stufe geschafft? Jede Art hat ihre eigene Bedingung; alles andere
// (Frist, Tod) prueft der Tick.
bool ALaLaBergDrehbuch::StufeGeschafft(const FStufe& Stufe, APawn* Figur) {
 const FVector Wo = Figur->GetActorLocation();
 const bool bImWagen = Figur->IsA<ALaLaBergWagen>();
 switch (Stufe.Art) {
  case ELaLaBergStufe::Bring:
   if (!bImWagen) return false;
   // absichtlich weiter zu "Hin"
  case ELaLaBergStufe::Hin:
   if (Stufe.bImWagen && !bImWagen) return false;
   return FVector::Dist2D(Wo, Stufe.Ort) < ZIEL_RADIUS && FMath::Abs(Wo.Z - Stufe.Ort.Z) < HOEHE_SPIEL;
  case ELaLaBergStufe::Markiere: {
   // Die Saeule des Reviers an diesem Ort: markiert zaehlt.
   const auto* R = ALaLaBergRevier::Instanz.Get();
   if (!R) return false;
   for (const auto& Revier : R->HoleReviere())
    for (const auto& Marke : Revier.Marken)
     if (Marke.Name == Stufe.OrtName) return Marke.bMarkiert;
   return false;
  }
  case ELaLaBergStufe::Stelle:
   return !Gegner.IsValid() || Gegner->IstAusgeschaltet();
  case ELaLaBergStufe::Halte:
   return GetWorld()->GetTimeSeconds() >= HalteBis;
 }
 return false;
}

void ALaLaBergDrehbuch::Vollende() {
 const FMission& M = Missionen[AktMission];
 if (auto* Konto = ULaLaBergKonto::Hole(this)) {
  Konto->Gutschrift(M.Lohn);
  Konto->Uebe(ULaLaBergKonto::EWert::Ruf, static_cast<float>(M.Ruf));
  Konto->SetzeMission(AktMission);
 }
 Geschafft++;
 Melde(FString::Printf(TEXT("%s geschafft – %d €"), *M.Name, M.Lohn));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_MISSION fertig %s lohn=%d"), *M.Id, M.Lohn);
 AktMission = INDEX_NONE;
 Gegner.Reset();
 SucheAngebot();
}

void ALaLaBergDrehbuch::Scheitere(const FString& Grund) {
 const FString Name = HoleMissionsname();
 Melde(FString::Printf(TEXT("%s gescheitert – %s"), *Name, *Grund));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_MISSION gescheitert %s grund=%s"), *Name, *Grund);
 AktMission = INDEX_NONE;
 Gegner.Reset();
 SucheAngebot();
}

bool ALaLaBergDrehbuch::TestStarte() {
 if (Angebot == INDEX_NONE) return false;
 Starte(Angebot);
 return true;
}

void ALaLaBergDrehbuch::TestStufeGeschafft() {
 if (AktMission == INDEX_NONE) return;
 // Die laufende Stufe ueberspringen - fuer Pruefläufe, die nicht wirklich
 // quer durch die Stadt fahren koennen.
 HalteBis = 0.0;
 Gegner.Reset();
 NaechsteStufe();
}

void ALaLaBergDrehbuch::Tick(float Zeit) {
 Super::Tick(Zeit);
 if (!bGeladen) return;
 auto* PC = GetWorld()->GetFirstPlayerController();
 APawn* Figur = PC ? PC->GetPawn() : nullptr;
 if (!Figur) return;

 if (AktMission == INDEX_NONE) {
  // Angebot annehmen, indem man in den gruenen Ring faehrt oder geht.
  if (Angebot != INDEX_NONE) {
   const FVector& Start = Missionen[Angebot].Start;
   if (FVector::Dist2D(Figur->GetActorLocation(), Start) < ZIEL_RADIUS
       && FMath::Abs(Figur->GetActorLocation().Z - Start.Z) < HOEHE_SPIEL)
    Starte(Angebot);
  }
  return;
 }
 const FStufe& S = Missionen[AktMission].Stufen[AktStufe];
 if (StufeGeschafft(S, Figur)) { NaechsteStufe(); return; }
 if (Frist > 0.0 && GetWorld()->GetTimeSeconds() > Frist) { Scheitere(TEXT("die Zeit ist um")); return; }
 // Wer im Klinikum aufwacht, hat die Mission verloren.
 if (const auto* Verletzbar = Cast<ILaLaBergVerletzbar>(Figur))
  if (Verletzbar->IstAusgeschaltet()) Scheitere(TEXT("umgehauen"));
}
