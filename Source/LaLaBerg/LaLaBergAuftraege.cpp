#include "LaLaBergAuftraege.h"
#include "LaLaBergHUD.h"
#include "LaLaBergKonto.h"
#include "LaLaBergWagen.h"
#include "LaLaBergVerkehrsauto.h"
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

UStaticMeshComponent* ALaLaBergAuftraege::BaueTeil(const TCHAR* Name, const FLinearColor& Farbe, bool bDurchsichtig) {
 auto* Teil = NewObject<UStaticMeshComponent>(this, Name);
 Teil->SetupAttachment(RootComponent);
 Teil->SetMobility(EComponentMobility::Movable);
 Teil->SetUsingAbsoluteLocation(true);
 Teil->SetUsingAbsoluteScale(true);
 Teil->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
 // Eine Saeule ist ein Zeichen, kein Bauwerk: durchscheinend und
 // selbstleuchtend (M_Saeule, siehe Tools/baue_saeule.py). Die Ringe am
 // Boden behalten das feste Grundmaterial.
 auto* Stoff = bDurchsichtig ? LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Saeule.M_Saeule"))
                             : nullptr;
 if (!Stoff) Stoff = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 Teil->SetMaterial(0, Stoff);
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
 StartRing = BaueTeil(TEXT("StartRing"), BLAU, false);
 StartSaeule = BaueTeil(TEXT("StartSaeule"), BLAU, true);
 ZielRing = BaueTeil(TEXT("ZielRing"), GELB, false);
 ZielSaeule = BaueTeil(TEXT("ZielSaeule"), GELB, true);
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

int32 ALaLaBergAuftraege::HoleGeld() const {
 const ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this);
 return Konto ? Konto->HoleGeld() : 0;
}

int32 ALaLaBergAuftraege::Strafe(int32 Betrag) {
 ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this);
 return Konto ? Konto->Strafe(Betrag) : 0;
}

float ALaLaBergAuftraege::HoleRestzeit() const {
 return bUnterwegs ? FMath::Max(0.0f, static_cast<float>(Frist - GetWorld()->GetTimeSeconds())) : 0.0f;
}

void ALaLaBergAuftraege::Melde(const FString& Text) const {
 if (auto* PC = GetWorld()->GetFirstPlayerController())
  if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
}

// Rennstrecke: vier Kontrollpunkte, jeder 300 bis 900 m vom vorigen weg -
// so entsteht ein Rundkurs durch die Stadt statt einer Geraden. Einsatz und
// Preisgeld haengen an der Gesamtlaenge.
void ALaLaBergAuftraege::BereiteRennen(float& Zeit) {
 Strecke.Reset();
 int32 Von = AktZiel;
 float Laenge = 0.0f;
 for (int32 Runde = 0; Runde < 4; Runde++) {
  TArray<int32> Moeglich;
  for (int32 i = 0; i < Ziele.Num(); i++) {
   if (i == Von || Strecke.Contains(i) || i == StartZiel) continue;
   const float D = Waagerecht(Ziele[i].Ort, Ziele[Von].Ort);
   if (D >= 30000.0f && D <= 90000.0f) Moeglich.Add(i);
  }
  if (Moeglich.IsEmpty()) break;
  const int32 Naechster = Moeglich[FMath::RandRange(0, Moeglich.Num() - 1)];
  Laenge += Waagerecht(Ziele[Naechster].Ort, Ziele[Von].Ort);
  Strecke.Add(Naechster);
  Von = Naechster;
 }
 if (Strecke.IsEmpty()) { Art = ELaLaBergAuftragsart::Lieferung; return; }
 AktZiel = Strecke[0];
 Punkt = 0;
 // 14 m/s ist zuegiges Stadttempo; der Umweg ueber die Strassen kostet die
 // Haelfte obendrauf, dazu zwanzig Sekunden Anlauf.
 Zeit = FMath::RoundToFloat(Laenge / 100.0f * 1.5f / 14.0f + 20.0f);
 Frist = GetWorld()->GetTimeSeconds() + Zeit;
 GesamtZeit = Zeit;
 Einsatz = 150;
 Lohn = 150 + FMath::RoundToInt(Laenge / 100.0f / 100.0f) * 45;
 if (ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this)) Konto->Bezahle(FMath::Min(Einsatz, Konto->HoleGeld()));
 Zeige(false, true, Ziele[AktZiel].Ort);
}

// Verfolgung: ein fahrendes Auto in 300 bis 1200 m Entfernung, moeglichst
// weit weg vom Spieler. Es wird auffaellig lackiert, damit man weiss, wen
// man sucht.
bool ALaLaBergAuftraege::SucheBeute() {
 auto* PC = GetWorld()->GetFirstPlayerController();
 const FVector Wo = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : StartOrt;
 ALaLaBergVerkehrsauto* Beste = nullptr;
 float BesteD = 0.0f;
 for (ALaLaBergVerkehrsauto* Auto : ALaLaBergVerkehrsauto::Alle) {
  if (!Auto || Auto->IstGeparkt() || Auto->IstAusgeschaltet()) continue;
  const float D = FVector::Dist2D(Auto->GetActorLocation(), Wo);
  if (D < 30000.0f || D > 120000.0f) continue;
  if (D > BesteD) { BesteD = D; Beste = Auto; }
 }
 if (!Beste) return false;
 Beute = Beste;
 Beste->SetzeLack(FLinearColor(0.85f, 0.05f, 0.55f));
 Lohn = 450 + FMath::RoundToInt(BesteD / 100.0f / 10.0f) * 10;
 return true;
}

FVector ALaLaBergAuftraege::HoleWegpunkt() const {
 if (!bUnterwegs) return StartOrt;
 if (Art == ELaLaBergAuftragsart::Verfolgung) return Beute.IsValid() ? Beute->GetActorLocation() : StartOrt;
 return Ziele.IsValidIndex(AktZiel) ? Ziele[AktZiel].Ort : StartOrt;
}

FString ALaLaBergAuftraege::HoleZielName() const {
 if (!bUnterwegs) return FString();
 if (Art == ELaLaBergAuftragsart::Verfolgung) return TEXT("Der flüchtende Wagen");
 return Ziele.IsValidIndex(AktZiel) ? Ziele[AktZiel].N : FString();
}

void ALaLaBergAuftraege::NimmAn() {
 // Jede zweite Saeule ist ein Taxiauftrag - aber nur, wer im Wagen sitzt,
 // kann einen Fahrgast mitnehmen.
 // Vier Arten, gleich haeufig - bis auf die Lieferung, die als einzige
 // auch zu Fuss geht und deshalb immer einspringt.
 static const ELaLaBergAuftragsart ARTEN[] = { ELaLaBergAuftragsart::Lieferung, ELaLaBergAuftragsart::Taxi,
                                               ELaLaBergAuftragsart::Rennen, ELaLaBergAuftragsart::Verfolgung };
 Art = bTaxiErzwungen ? ELaLaBergAuftragsart::Taxi
     : bArtErzwungen  ? ErzwungeneArt
                      : ARTEN[FMath::RandRange(0, UE_ARRAY_COUNT(ARTEN) - 1)];
 bTaxiErzwungen = false;
 bArtErzwungen = false;
 auto* PC = GetWorld()->GetFirstPlayerController();
 const bool bImWagen = PC && PC->GetPawn() && PC->GetPawn()->IsA<ALaLaBergWagen>();
 if (Art != ELaLaBergAuftragsart::Lieferung && !bImWagen) {
  const ELaLaBergAuftragsart Gewollt = Art;
  Art = ELaLaBergAuftragsart::Lieferung;
  const double Jetzt = GetWorld()->GetTimeSeconds();
  if (Jetzt - LetzterHinweis > 12.0) {
   LetzterHinweis = Jetzt;
   Melde(Gewollt == ELaLaBergAuftragsart::Taxi ? TEXT("Hier wartet auch ein Fahrgast – mit dem Wagen vorfahren")
        : Gewollt == ELaLaBergAuftragsart::Rennen ? TEXT("Hier startet auch ein Rennen – mit dem Wagen vorfahren")
                                                  : TEXT("Hier gäbe es auch eine Verfolgung – mit dem Wagen vorfahren"));
  }
 }
 Strecke.Reset();
 Punkt = 0;
 Einsatz = 0;
 Beute.Reset();
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
 // Nicht const: das Rennen rechnet die Frist nach seiner eigenen Strecke
 // neu (siehe BereiteRennen).
 float Zeit = FMath::RoundToFloat(Meter * 1.5f / 10.0f + 30.0f);
 // Taxi: Grundpreis plus Streckenanteil, deutlich mehr als eine Lieferung -
 // dafuer sitzt jemand im Wagen, der es eilig hat (Trinkgeld, siehe
 // Erledige).
 Lohn = Art == ELaLaBergAuftragsart::Taxi
  ? 160 + FMath::RoundToInt(Meter / 100.0f) * 24
  : 100 + FMath::RoundToInt(Meter / 5.0f / 10.0f) * 10;
 Frist = GetWorld()->GetTimeSeconds() + Zeit;
 GesamtZeit = Zeit;
 if (Art == ELaLaBergAuftragsart::Rennen) BereiteRennen(Zeit);
 else if (Art == ELaLaBergAuftragsart::Verfolgung && !SucheBeute()) {
  // Kein Wagen in Reichweite: dann eben eine Lieferung.
  Art = ELaLaBergAuftragsart::Lieferung;
 }
 bUnterwegs = true;
 bAngebot = false;
 Zeige(true, false, StartOrt);
 Zeige(false, true, Ziele[AktZiel].Ort);
 // Zwei getrennte Aufrufe: Printf prueft die Formatzeichenkette zur
 // Uebersetzungszeit und nimmt keine, die erst zur Laufzeit feststeht.
 const int32 Min = FMath::FloorToInt(Zeit / 60.0f), Sek = FMath::FloorToInt(Zeit) % 60;
 if (Art == ELaLaBergAuftragsart::Rennen)
  Melde(FString::Printf(TEXT("Rennen gewonnen! +%d € (Einsatz %d €)"), Lohn, Einsatz));
 else if (Art == ELaLaBergAuftragsart::Verfolgung)
  Melde(FString::Printf(TEXT("Gestellt! +%d €"), Lohn));
 else Melde(Art == ELaLaBergAuftragsart::Taxi
       ? FString::Printf(TEXT("Fahrgast nach %s – %d:%02d Minuten, %d € plus Trinkgeld"), *Ziele[AktZiel].N, Min, Sek, Lohn)
       : FString::Printf(TEXT("Lieferung zu %s – %d:%02d Minuten, %d €"), *Ziele[AktZiel].N, Min, Sek, Lohn));
 // Alle vier Arten benennen - die Zeile stand sonst auch bei einem Rennen
 // auf "lieferung" und log dabei.
 const TCHAR* ArtName = Art == ELaLaBergAuftragsart::Taxi ? TEXT("taxi")
                      : Art == ELaLaBergAuftragsart::Rennen ? TEXT("rennen")
                      : Art == ELaLaBergAuftragsart::Verfolgung ? TEXT("verfolgung")
                                                                : TEXT("lieferung");
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG start art=%s ziel=%s luftlinie=%.0fm zeit=%.0fs lohn=%d"),
        ArtName, *Ziele[AktZiel].N, Meter, Zeit, Lohn);
}

void ALaLaBergAuftraege::Erledige() {
 // Rennen: erst der letzte Kontrollpunkt zahlt aus, die davor schalten nur
 // weiter.
 if (Art == ELaLaBergAuftragsart::Rennen && Punkt + 1 < Strecke.Num()) {
  Zeige(false, false, Ziele[AktZiel].Ort);
  Punkt++;
  AktZiel = Strecke[Punkt];
  Zeige(false, true, Ziele[AktZiel].Ort);
  Melde(FString::Printf(TEXT("Kontrollpunkt %d von %d – weiter zu %s"), Punkt, Strecke.Num(), *Ziele[AktZiel].N));
  return;
 }
 // Trinkgeld beim Taxi: wer die Haelfte der Frist noch uebrig hat, bekommt
 // 40 Prozent obendrauf, linear abnehmend bis auf null.
 int32 Trinkgeld = 0;
 if (Art == ELaLaBergAuftragsart::Rennen) Rennen++;
 if (Art == ELaLaBergAuftragsart::Verfolgung) Verfolgungen++;
 if (Art == ELaLaBergAuftragsart::Taxi) {
  const float Rest = FMath::Max(0.0f, HoleRestzeit());
  const float Anteil = FMath::Clamp(Rest / FMath::Max(1.0f, GesamtZeit) / 0.5f, 0.0f, 1.0f);
  Trinkgeld = FMath::RoundToInt(Lohn * 0.4f * Anteil);
  Taxifahrten++;
 }
 ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this);
 if (Konto) {
  Konto->Gutschrift(Lohn + Trinkgeld);
  Konto->ZaehleAuftrag();
  // Ruf: ein erledigter Auftrag zaehlt, ein Rennen und eine Verfolgung
  // doppelt - davon spricht man in der Stadt.
  Konto->Uebe(ULaLaBergKonto::EWert::Ruf,
              Art == ELaLaBergAuftragsart::Lieferung ? 14.0f
            : Art == ELaLaBergAuftragsart::Taxi ? 18.0f : 32.0f);
 }
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
 Melde(Art == ELaLaBergAuftragsart::Taxi
       ? FString::Printf(TEXT("Angekommen! +%d €%s – nächster Auftrag an der blauen Säule"), Lohn + Trinkgeld,
                         Trinkgeld > 0 ? *FString::Printf(TEXT(" (davon %d %s Trinkgeld)"), Trinkgeld, TEXT("€")) : TEXT(""))
       : FString::Printf(TEXT("Geliefert! +%d € – nächster Auftrag an der blauen Säule"), Lohn));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_AUFTRAG erledigt ziel=%s geld=%d naechster=%s"), *Hier.N, HoleGeld(),
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
 if (bUnterwegs && Art == ELaLaBergAuftragsart::Verfolgung) {
  // Gestellt ist, wer steht: ausgeschaltet durch Farbe oder Rammen.
  if (!Beute.IsValid()) Scheitere();
  else if (Beute->IstAusgeschaltet()) Erledige();
  else if (GetWorld()->GetTimeSeconds() > Frist) Scheitere();
 } else if (bUnterwegs) {
  const FVector& Ziel = Ziele[AktZiel].Ort;
  if (Waagerecht(Wo, Ziel) < ZIEL_RADIUS && FMath::Abs(Wo.Z - Ziel.Z) < HOEHE_SPIEL) Erledige();
  else if (GetWorld()->GetTimeSeconds() > Frist) Scheitere();
 } else if (bAngebot) {
  const bool bDrin = Waagerecht(Wo, StartOrt) < ANNAHME_RADIUS && FMath::Abs(Wo.Z - StartOrt.Z) < HOEHE_SPIEL;
  if (!bDrin) bErstHinaus = false;
  else if (!bErstHinaus) NimmAn();
 }
}
