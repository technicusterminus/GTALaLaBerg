#include "LaLaBergAuftraege.h"
#include "LaLaBergHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TWeakObjectPtr<ALaLaBergAuftraege> ALaLaBergAuftraege::Instanz;

namespace {
 // Wie nah man kommen muss (waagerecht, in cm): in die blaue Saeule zu Fuss
 // hineinlaufen, ans Ziel auch mit dem Wagen auf der Fahrbahn.
 constexpr float ANNAHME_RADIUS = 600.0f;
 constexpr float ZIEL_RADIUS = 800.0f;
 constexpr float HOEHE_SPIEL = 500.0f;
 // Ziele nach Luftlinie: nicht um die Ecke, nicht quer durch den Landkreis.
 constexpr float ZIEL_MIN = 40000.0f, ZIEL_MAX = 250000.0f;
 // Naechste blaue Saeule nach einem erledigten Auftrag: ein Ort in der Naehe.
 constexpr float NAECHSTE_MIN = 15000.0f, NAECHSTE_MAX = 90000.0f;
 const FLinearColor BLAU(0.05f, 0.45f, 1.0f), GELB(1.0f, 0.78f, 0.05f);

 float Waagerecht(const FVector& A, const FVector& B) { return FVector::Dist2D(A, B); }

 // Hoehe der Oberflaeche unter einem Punkt; Fahrbahn und Platz haben Vorrang
 // vor einem Auto oder Baum, das zufaellig darueber steht.
 bool Boden(UWorld* Welt, const FVector2D& P, float& Z) {
  TArray<FHitResult> Treffer;
  FCollisionQueryParams Q(TEXT("AuftragBoden"), true);
  Welt->LineTraceMultiByChannel(Treffer, FVector(P, 60000.0), FVector(P, -60000.0), ECC_Visibility, Q);
  for (const FHitResult& T : Treffer) {
   const auto* Netz = Cast<UStaticMeshComponent>(T.GetComponent());
   const FString Name = Netz && Netz->GetStaticMesh() ? Netz->GetStaticMesh()->GetName() : FString();
   if (Name.Contains(TEXT("_Road")) || Name.Contains(TEXT("_Plaza"))) { Z = T.ImpactPoint.Z; return true; }
  }
  FHitResult Einer;
  if (Welt->LineTraceSingleByChannel(Einer, FVector(P, 60000.0), FVector(P, -60000.0), ECC_Visibility, Q)) {
   Z = Einer.ImpactPoint.Z; return true;
  }
  return false;
 }
}

ALaLaBergAuftraege::ALaLaBergAuftraege() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.1f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

UStaticMeshComponent* ALaLaBergAuftraege::BaueTeil(const TCHAR* Name, const FLinearColor& Farbe) {
 auto* Teil = NewObject<UStaticMeshComponent>(this, Name);
 Teil->SetupAttachment(RootComponent);
 Teil->SetMobility(EComponentMobility::Movable);
 Teil->SetUsingAbsoluteLocation(true);
 Teil->SetUsingAbsoluteScale(true);
 Teil->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 Teil->SetMaterial(0, LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
 Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Teil->SetCastShadow(false);
 Teil->SetVisibility(false);
 Teil->RegisterComponent();
 if (auto* MID = Teil->CreateDynamicMaterialInstance(0)) MID->SetVectorParameterValue(TEXT("Color"), Farbe);
 return Teil;
}

void ALaLaBergAuftraege::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 // Der Zylinder der Engine: 1 m Durchmesser, 1 m hoch, Mitte im Mittelpunkt.
 StartRing = BaueTeil(TEXT("StartRing"), BLAU);
 StartSaeule = BaueTeil(TEXT("StartSaeule"), BLAU);
 ZielRing = BaueTeil(TEXT("ZielRing"), GELB);
 ZielSaeule = BaueTeil(TEXT("ZielSaeule"), GELB);
 LadeZiele();
}

void ALaLaBergAuftraege::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

// Die Wahrzeichen aus orte.json liegen mitten im Gebaeude. Als Ziel taugt der
// naechste Punkt auf einer Strasse davor, gesucht auf den Strassenzuegen aus
// derselben Datei - dort kommt man mit dem Wagen auch hin.
void ALaLaBergAuftraege::LadeZiele() {
 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Orte/orte.json");
 if (!FFileHelper::LoadFileToString(Text, *Datei) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_AUFTRAG keine Orte: %s"), *Datei);
  return;
 }
 TArray<TArray<FVector2D>> Zuege;
 for (const auto& S : Wurzel->GetArrayField(TEXT("strassen")))
  for (const auto& T : S->AsObject()->GetArrayField(TEXT("teile"))) {
   const auto& Z = T->AsArray();
   TArray<FVector2D>& Zug = Zuege.AddDefaulted_GetRef();
   for (int32 i = 0; i + 1 < Z.Num(); i += 2) Zug.Add(FVector2D(Z[i]->AsNumber(), Z[i + 1]->AsNumber()));
  }
 int32 Verworfen = 0;
 for (const auto& V : Wurzel->GetArrayField(TEXT("marken"))) {
  const auto O = V->AsObject();
  const FString N = O->GetStringField(TEXT("n"));
  if (Ziele.ContainsByPredicate([&](const FZiel& Z) { return Z.N == N; })) continue;
  const FVector2D M(O->GetNumberField(TEXT("x")), O->GetNumberField(TEXT("y")));
  FVector2D Best; float BestD = TNumericLimits<float>::Max();
  for (const auto& Zug : Zuege)
   for (int32 i = 0; i + 1 < Zug.Num(); i++) {
    const FVector2D AB = Zug[i + 1] - Zug[i];
    const float L = AB.SizeSquared();
    const float T = L > 0 ? FMath::Clamp(FVector2D::DotProduct(M - Zug[i], AB) / L, 0.0f, 1.0f) : 0.0f;
    const FVector2D P = Zug[i] + AB * T;
    const float D = FVector2D::Distance(M, P);
    if (D < BestD) { BestD = D; Best = P; }
   }
  float Z;
  // Weiter als 250 m von jeder Strasse: da kommt man nicht hin.
  if (BestD > 25000.0f || !Boden(GetWorld(), Best, Z)) { Verworfen++; continue; }
  Ziele.Add({ N, FVector(Best, Z) });
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG ziele=%d verworfen=%d"), Ziele.Num(), Verworfen);
}

void ALaLaBergAuftraege::Zeige(bool bStart, bool bSichtbar, const FVector& Ort) {
 UStaticMeshComponent* Ring = bStart ? StartRing : ZielRing;
 UStaticMeshComponent* Saeule = bStart ? StartSaeule : ZielSaeule;
 const float Durchmesser = (bStart ? ANNAHME_RADIUS : ZIEL_RADIUS) * 2.0f / 100.0f;
 // Flach und knapp ueber dem Boden, damit er nicht in der Fahrbahn flimmert.
 Ring->SetWorldLocation(Ort + FVector(0, 0, 12));
 Ring->SetWorldScale3D(FVector(Durchmesser, Durchmesser, 0.08f));
 // 60 m hoch - hoeher als jeder Kirchturm in der Altstadt ausser dem
 // Stadtpfarrturm, also von weitem zu sehen.
 Saeule->SetWorldLocation(Ort + FVector(0, 0, 3000));
 Saeule->SetWorldScale3D(FVector(2.5f, 2.5f, 60.0f));
 Ring->SetVisibility(bSichtbar);
 Saeule->SetVisibility(bSichtbar);
}

void ALaLaBergAuftraege::SetzeStartOrt(const FVector& Ort) {
 float Z = Ort.Z;
 Boden(GetWorld(), FVector2D(Ort), Z);
 StartOrt = FVector(Ort.X, Ort.Y, Z);
 StartZiel = INDEX_NONE;
 bAngebot = true;
 Zeige(true, true, StartOrt);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG angebot %s"), *StartOrt.ToString());
}

float ALaLaBergAuftraege::HoleRestzeit() const {
 return bUnterwegs ? FMath::Max(0.0f, static_cast<float>(Frist - GetWorld()->GetTimeSeconds())) : 0.0f;
}

void ALaLaBergAuftraege::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

void ALaLaBergAuftraege::NimmAn() {
 TArray<int32> Moeglich;
 for (int32 i = 0; i < Ziele.Num(); i++) {
  const float D = Waagerecht(Ziele[i].Ort, StartOrt);
  if (i != StartZiel && D >= ZIEL_MIN && D <= ZIEL_MAX) Moeglich.Add(i);
 }
 if (Moeglich.IsEmpty()) { UE_LOG(LogTemp, Warning, TEXT("LALABERG_AUFTRAG kein Ziel in Reichweite")); return; }
 AktZiel = Moeglich[FMath::RandRange(0, Moeglich.Num() - 1)];
 const float Meter = Waagerecht(Ziele[AktZiel].Ort, StartOrt) / 100.0f;
 // Die Strassen sind laenger als die Luftlinie; 10 m/s ist gemaechliches
 // Stadttempo, dazu eine halbe Minute fuers Einsteigen.
 const float Zeit = FMath::RoundToFloat(Meter * 1.5f / 10.0f + 30.0f);
 Lohn = 100 + FMath::RoundToInt(Meter / 5.0f / 10.0f) * 10;
 Frist = GetWorld()->GetTimeSeconds() + Zeit;
 bUnterwegs = true;
 bAngebot = false;
 Zeige(true, false, StartOrt);
 Zeige(false, true, Ziele[AktZiel].Ort);
 Melde(FString::Printf(TEXT("Lieferung zu %s – %d:%02d Minuten, %d €"), *Ziele[AktZiel].N,
                       FMath::FloorToInt(Zeit / 60.0f), FMath::FloorToInt(Zeit) % 60, Lohn));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG start ziel=%s luftlinie=%.0fm zeit=%.0fs lohn=%d"),
        *Ziele[AktZiel].N, Meter, Zeit, Lohn);
}

void ALaLaBergAuftraege::Erledige() {
 Geld += Lohn;
 Erledigt++;
 bUnterwegs = false;
 const FZiel Hier = Ziele[AktZiel];
 Zeige(false, false, Hier.Ort);
 // Naechste blaue Saeule: ein anderer Ort in der Naehe - nicht genau hier,
 // sonst stuende man schon drin und der naechste Auftrag liefe ungefragt los.
 TArray<int32> Nah;
 for (int32 i = 0; i < Ziele.Num(); i++) {
  const float D = Waagerecht(Ziele[i].Ort, Hier.Ort);
  if (i != AktZiel && D >= NAECHSTE_MIN && D <= NAECHSTE_MAX) Nah.Add(i);
 }
 if (!Nah.IsEmpty()) {
  StartZiel = Nah[FMath::RandRange(0, Nah.Num() - 1)];
  StartOrt = Ziele[StartZiel].Ort;
 }
 bAngebot = true;
 Zeige(true, true, StartOrt);
 Melde(FString::Printf(TEXT("Geliefert! +%d € – nächster Auftrag an der blauen Säule"), Lohn));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG erledigt ziel=%s geld=%d naechster=%s"), *Hier.N, Geld,
        StartZiel != INDEX_NONE ? *Ziele[StartZiel].N : TEXT("-"));
 AktZiel = INDEX_NONE;
}

void ALaLaBergAuftraege::Scheitere() {
 Gescheitert++;
 bUnterwegs = false;
 Zeige(false, false, Ziele[AktZiel].Ort);
 bAngebot = true;
 Zeige(true, true, StartOrt);
 // Wer noch in der blauen Saeule steht, bekommt nicht gleich den naechsten
 // Auftrag - erst wieder hinaus und hinein.
 bErstHinaus = true;
 Melde(TEXT("Zu spät – die Lieferung ist verfallen. Neuer Versuch an der blauen Säule"));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG gescheitert ziel=%s"), *Ziele[AktZiel].N);
 AktZiel = INDEX_NONE;
}

void ALaLaBergAuftraege::Abbrechen() {
 if (!bUnterwegs) return;
 bUnterwegs = false;
 Zeige(false, false, Ziele[AktZiel].Ort);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG abgebrochen ziel=%s"), *Ziele[AktZiel].N);
 AktZiel = INDEX_NONE;
 bAngebot = true;
 bErstHinaus = true;
 Zeige(true, true, StartOrt);
}

void ALaLaBergAuftraege::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 auto* PC = GetWorld()->GetFirstPlayerController();
 APawn* Figur = PC ? PC->GetPawn() : nullptr;
 if (!Figur) return;
 const FVector Wo = Figur->GetActorLocation();
 if (bUnterwegs) {
  const FVector& Ziel = Ziele[AktZiel].Ort;
  if (Waagerecht(Wo, Ziel) < ZIEL_RADIUS && FMath::Abs(Wo.Z - Ziel.Z) < HOEHE_SPIEL) Erledige();
  else if (GetWorld()->GetTimeSeconds() > Frist) Scheitere();
 } else if (bAngebot) {
  const bool bDrin = Waagerecht(Wo, StartOrt) < ANNAHME_RADIUS && FMath::Abs(Wo.Z - StartOrt.Z) < HOEHE_SPIEL;
  if (!bDrin) bErstHinaus = false;
  else if (!bErstHinaus) NimmAn();
 }
}
