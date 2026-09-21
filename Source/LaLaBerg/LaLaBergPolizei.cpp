#include "LaLaBergPolizei.h"
#include "LaLaBergVerkehrsauto.h"
#include "LaLaBergWagen.h"
#include "LaLaBergAuftraege.h"
#include "LaLaBergHUD.h"
#include "Engine/World.h"
#include "Algo/Reverse.h"
#include "GameFramework/PawnMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TWeakObjectPtr<ALaLaBergPolizei> ALaLaBergPolizei::Instanz;

namespace {
 // Wer so nah ist, sieht den Spieler auch ohne freie Sichtlinie (um die Ecke
 // hoert man ihn); wer weiter weg ist als die Weite, sieht ihn gar nicht.
 constexpr float SICHT_NAH = 4000.0f, SICHT_WEIT = 15000.0f;
 // Neue Streifen setzen ausser Sicht ein: 250-450 m vom Spieler.
 constexpr float EINSATZ_MIN = 25000.0f, EINSATZ_MAX = 45000.0f;
 // Ab hier faehrt die Streife geradewegs auf den Spieler zu statt ueber
 // den naechsten Graphknoten - er steht ja nicht immer auf der Fahrbahn.
 constexpr float DIREKT = 3500.0f;
 // Weiter weg als 900 m: diese Streife hat verloren, sie wird abgezogen.
 constexpr float VERLOREN = 90000.0f;
 constexpr float FESTNAHME_ABSTAND = 800.0f;
 // Polizeiinspektion (orte.json "marken") - dort kommt man nach einer
 // Festnahme wieder frei.
 const FVector INSPEKTION(-42590.0f, 46150.0f, 0.0f);

 float Gewicht(ELaLaBergTat Tat) {
  switch (Tat) {
   case ELaLaBergTat::PassantBeschossen: return 1.0f;
   case ELaLaBergTat::PassantAngefahren: return 2.0f;
   case ELaLaBergTat::AutoGestohlen: return 2.0f;
   case ELaLaBergTat::PolizeiBeschossen: return 4.0f;
  }
  return 1.0f;
 }
 const TCHAR* Name(ELaLaBergTat Tat) {
  switch (Tat) {
   case ELaLaBergTat::PassantBeschossen: return TEXT("passant_beschossen");
   case ELaLaBergTat::PassantAngefahren: return TEXT("passant_angefahren");
   case ELaLaBergTat::AutoGestohlen: return TEXT("auto_gestohlen");
   case ELaLaBergTat::PolizeiBeschossen: return TEXT("polizei_beschossen");
  }
  return TEXT("?");
 }
 // Dieselbe Tat zaehlt hoechstens einmal je Sekunde - sonst brachte ein
 // einziger Feuerstoss aus der MP gleich drei Sterne.
 double LetzteTat[4] = { -10.0, -10.0, -10.0, -10.0 };

 void Meldung(UWorld* Welt, const FString& Text) {
  if (auto* PC = Welt->GetFirstPlayerController())
   if (auto* HUD = Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeRueckmeldung(Text);
 }
}

ALaLaBergPolizei::ALaLaBergPolizei() {
 PrimaryActorTick.bCanEverTick = true;
 PrimaryActorTick.TickInterval = 0.1f;
 RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Wurzel"));
}

void ALaLaBergPolizei::BeginPlay() {
 Super::BeginPlay();
 Instanz = this;
 for (double& T : LetzteTat) T = -10.0;
 LadeNetz();
}

void ALaLaBergPolizei::EndPlay(const EEndPlayReason::Type Grund) {
 if (Instanz.Get() == this) Instanz.Reset();
 Super::EndPlay(Grund);
}

void ALaLaBergPolizei::LadeNetz() {
 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Verkehr/netz.json");
 if (!FFileHelper::LoadFileToString(Text, *Datei) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_POLIZEI kein Strassennetz: %s"), *Datei);
  return;
 }
 const auto& P = Wurzel->GetArrayField(TEXT("p"));
 const auto& E = Wurzel->GetArrayField(TEXT("e"));
 for (int32 i = 0; i + 2 < P.Num(); i += 3) Knoten.Add(FVector(P[i]->AsNumber(), P[i + 1]->AsNumber(), P[i + 2]->AsNumber()));
 // Nach Startknoten zaehlen, dann einsortieren (CSR).
 KantenAb.Init(0, Knoten.Num() + 1);
 for (int32 i = 0; i + 2 < E.Num(); i += 3) KantenAb[static_cast<int32>(E[i]->AsNumber()) + 1]++;
 for (int32 i = 0; i < Knoten.Num(); i++) KantenAb[i + 1] += KantenAb[i];
 KantenNach.SetNumUninitialized(E.Num() / 3);
 TArray<int32> Fuellung(KantenAb);
 for (int32 i = 0; i + 2 < E.Num(); i += 3) KantenNach[Fuellung[static_cast<int32>(E[i]->AsNumber())]++] = static_cast<int32>(E[i + 1]->AsNumber());
 UE_LOG(LogTemp, Display, TEXT("LALABERG_POLIZEI netz knoten=%d kanten=%d"), Knoten.Num(), KantenNach.Num());
}

int32 ALaLaBergPolizei::NaechsterKnoten(const FVector& Ort, const FVector* Vorwaerts) const {
 int32 Beste = INDEX_NONE;
 float BesteD = TNumericLimits<float>::Max();
 for (int32 i = 0; i < Knoten.Num(); i++) {
  const FVector D = Knoten[i] - Ort;
  const float Abstand = D.Size2D();
  // Nur Knoten voraus - sonst dreht der Wagen bei jeder Neuplanung zu dem
  // Knoten um, an dem er gerade vorbei ist.
  if (Vorwaerts && Abstand > 500.0f && FVector::DotProduct(D, *Vorwaerts) < 0.0f) continue;
  if (Abstand < BesteD) { BesteD = Abstand; Beste = i; }
 }
 return Beste;
}

bool ALaLaBergPolizei::SuchePfad(int32 Von, int32 Nach, TArray<int32>& Pfad) const {
 Pfad.Reset();
 if (!Knoten.IsValidIndex(Von) || !Knoten.IsValidIndex(Nach)) return false;
 // A* mit der Luftlinie als Schaetzung.
 TArray<float> G; G.Init(TNumericLimits<float>::Max(), Knoten.Num());
 TArray<int32> Vorg; Vorg.Init(INDEX_NONE, Knoten.Num());
 TArray<uint8> Fertig; Fertig.Init(0, Knoten.Num());
 struct FOffen { float F; int32 K; };
 TArray<FOffen> Offen;
 auto Kleiner = [](const FOffen& A, const FOffen& B) { return A.F < B.F; };
 G[Von] = 0.0f;
 Offen.HeapPush({ static_cast<float>(FVector::Dist2D(Knoten[Von], Knoten[Nach])), Von }, Kleiner);
 while (Offen.Num()) {
  FOffen Oben; Offen.HeapPop(Oben, Kleiner);
  const int32 K = Oben.K;
  if (Fertig[K]) continue;
  Fertig[K] = 1;
  if (K == Nach) break;
  for (int32 k = KantenAb[K]; k < KantenAb[K + 1]; k++) {
   const int32 N = KantenNach[k];
   const float Neu = G[K] + (float)FVector::Dist2D(Knoten[K], Knoten[N]);
   if (Neu < G[N]) {
    G[N] = Neu; Vorg[N] = K;
    Offen.HeapPush({ Neu + static_cast<float>(FVector::Dist2D(Knoten[N], Knoten[Nach])), N }, Kleiner);
   }
  }
 }
 if (!Fertig[Nach]) return false;
 for (int32 K = Nach; K != INDEX_NONE; K = Vorg[K]) Pfad.Add(K);
 Algo::Reverse(Pfad);
 return true;
}

void ALaLaBergPolizei::Melde(ELaLaBergTat Tat) {
 ALaLaBergPolizei* P = Instanz.Get();
 if (!P) return;
 const double Jetzt = P->GetWorld()->GetTimeSeconds();
 double& Letzte = LetzteTat[static_cast<int32>(Tat)];
 if (Jetzt - Letzte < 1.0) return;
 Letzte = Jetzt;
 UE_LOG(LogTemp, Display, TEXT("LALABERG_TAT %s"), Name(Tat));
 P->Erhoehe(Gewicht(Tat));
}

void ALaLaBergPolizei::Erhoehe(float Gewicht) {
 const int32 Vorher = Sterne;
 Punkte += Gewicht;
 // Stern n ab n*n Punkten: der erste kommt sofort, der fuenfte erst nach
 // einer ganzen Reihe von Taten.
 Sterne = FMath::Clamp(FMath::FloorToInt(FMath::Sqrt(Punkte)), 1, 5);
 // Eine Tat ist immer auch gesehen worden - die Suche beginnt von vorn,
 // am Tatort.
 ZuletztGesehen = GetWorld()->GetTimeSeconds();
 bGesehen = true;
 Ungesehen = 0.0f;
 if (const auto* PC = GetWorld()->GetFirstPlayerController(); PC && PC->GetPawn()) LetzterOrt = PC->GetPawn()->GetActorLocation();
 if (Sterne != Vorher) UE_LOG(LogTemp, Display, TEXT("LALABERG_FAHNDUNG sterne=%d punkte=%.0f"), Sterne, Punkte);
 if (Vorher == 0) NaechsterEinsatz = GetWorld()->GetTimeSeconds() + 1.0;
}

void ALaLaBergPolizei::TestSetzeSterne(int32 Anzahl) {
 Punkte = static_cast<float>(Anzahl * Anzahl);
 const int32 Vorher = Sterne;
 Sterne = FMath::Clamp(Anzahl, 0, 5);
 ZuletztGesehen = GetWorld()->GetTimeSeconds();
 bGesehen = Sterne > 0;
 Ungesehen = 0.0f;
 if (const auto* PC = GetWorld()->GetFirstPlayerController(); PC && PC->GetPawn()) LetzterOrt = PC->GetPawn()->GetActorLocation();
 if (Vorher == 0) NaechsterEinsatz = GetWorld()->GetTimeSeconds();
 UE_LOG(LogTemp, Display, TEXT("LALABERG_FAHNDUNG sterne=%d punkte=%.0f"), Sterne, Punkte);
}

float ALaLaBergPolizei::HoleSuchAnteil() const {
 if (Sterne == 0 || bGesehen) return 0.0f;
 return FMath::Clamp(Ungesehen / Suchdauer(), 0.0f, 1.0f);
}

void ALaLaBergPolizei::HoleStreifen(TArray<FVector>& Orte) const {
 for (const FStreife& S : Streifen)
  if (IsValid(S.Auto) && !S.Auto->IstGeparkt()) Orte.Add(S.Auto->GetActorLocation());
}

float ALaLaBergPolizei::NaechsteStreifeCm() const {
 const auto* PC = GetWorld()->GetFirstPlayerController();
 const APawn* Spieler = PC ? PC->GetPawn() : nullptr;
 float Beste = TNumericLimits<float>::Max();
 if (!Spieler) return Beste;
 for (const FStreife& S : Streifen)
  if (S.bImEinsatz && IsValid(S.Auto))
   Beste = FMath::Min(Beste, FVector::Dist2D(S.Auto->GetActorLocation(), Spieler->GetActorLocation()));
 return Beste;
}

bool ALaLaBergPolizei::Sieht(const ALaLaBergVerkehrsauto* Auto, const APawn* Spieler) const {
 const FVector Von = Auto->GetActorLocation() + FVector(0, 0, 180.0f);
 const FVector Nach = Spieler->GetActorLocation();
 const float D = FVector::Dist(Von, Nach);
 if (D < SICHT_NAH) return true;
 if (D > SICHT_WEIT) return false;
 FCollisionQueryParams Q(TEXT("PolizeiSicht"), false, Auto);
 Q.AddIgnoredActor(Spieler);
 return !GetWorld()->LineTraceTestByChannel(Von, Nach, ECC_Visibility, Q);
}

bool ALaLaBergPolizei::Entsende(FStreife& S, const FVector& Ziel) {
 const FVector& Spieler = Ziel;
 // Faehrt der Wagen noch vom letzten Einsatz davon, dreht er einfach um.
 if (IsValid(S.Auto) && !S.Auto->IstGeparkt()) {
  S.bImEinsatz = true;
  Plane(S, Spieler);
  return true;
 }
 TArray<int32> Kandidaten;
 for (int32 i = 0; i < Knoten.Num(); i++) {
  const float D = FVector::Dist2D(Knoten[i], Spieler);
  if (D >= EINSATZ_MIN && D <= EINSATZ_MAX) Kandidaten.Add(i);
 }
 if (Kandidaten.IsEmpty()) return false;
 const int32 K = Kandidaten[FMath::RandRange(0, Kandidaten.Num() - 1)];
 const FVector Ort = Knoten[K];
 // Blick auf den ersten Nachbarn - sonst faehrt der Wagen die erste
 // Neuplanung quer aus dem Stand an.
 FRotator Blick = (Spieler - Ort).Rotation();
 if (KantenAb[K] < KantenAb[K + 1]) Blick = (Knoten[KantenNach[KantenAb[K]]] - Ort).Rotation();
 Blick.Pitch = 0.0f; Blick.Roll = 0.0f;
 if (!IsValid(S.Auto)) {
  auto* Auto = GetWorld()->SpawnActorDeferred<ALaLaBergVerkehrsauto>(ALaLaBergVerkehrsauto::StaticClass(), FTransform(Blick, Ort),
                                                                   nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
  if (!Auto) return false;
  Auto->bPolizei = true;
  Auto->WunschTyp = -1;
  Auto->SetzeStrassenbreite(6.0f);
  Auto->FinishSpawning(FTransform(Blick, Ort));
  S.Auto = Auto;
  StreifenHalter.Add(Auto);
 } else {
  S.Auto->SetActorLocationAndRotation(Ort, Blick);
 }
 S.bImEinsatz = true;
 S.NaechstePlanung = 0.0;
 Plane(S, Spieler);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_STREIFE einsatz bei %s abstand=%.0fm"), *Ort.ToString(), FVector::Dist2D(Ort, Spieler) / 100.0f);
 return true;
}

void ALaLaBergPolizei::Plane(FStreife& S, const FVector& Ziel) {
 const FVector& Spieler = Ziel;
 ALaLaBergVerkehrsauto* Auto = S.Auto;
 const FVector Ort = Auto->HoleRoutenOrt();
 const FVector Vorwaerts = Auto->GetActorForwardVector();
 // Mit jedem Stern etwas schneller: 58 km/h bei einem, 90 km/h bei fuenf.
 const float Tempo = 50.0f + 8.0f * FMath::Max(1, Sterne);
 TArray<FVector> Route;
 Route.Add(Ort);
 // Das letzte Stueck geradewegs auf das Ziel zu, 2,5 m davor anhalten: wer
 // neben der Strasse steht, soll trotzdem erreicht werden - die Route endete
 // sonst am naechsten Strassenknoten, womoeglich 50 m entfernt.
 auto Haltepunkt = [&](const FVector& Von) {
  FVector Halt = Spieler - (Spieler - Von).GetSafeNormal2D() * 250.0f;
  FHitResult Boden;
  FCollisionQueryParams Q(TEXT("PolizeiBoden"), false, Auto);
  if (const auto* PC = GetWorld()->GetFirstPlayerController()) Q.AddIgnoredActor(PC->GetPawn());
  Halt.Z = GetWorld()->LineTraceSingleByChannel(Boden, Halt + FVector(0, 0, 300), Halt - FVector(0, 0, 1500), ECC_Visibility, Q)
   ? Boden.ImpactPoint.Z : Von.Z;
  return Halt;
 };
 const TArray<int32> AlterPfad = MoveTemp(S.Pfad);
 S.Pfad.Reset();
 if (FVector::Dist2D(Ort, Spieler) >= DIREKT) {
  // Am Knoten weiter, den der Wagen gerade ansteuert. Nach der Blickrichtung
  // allein gewaehlt, drehte ein Wagen mitten in der Kurve bei jeder
  // Neuplanung zu einem anderen Knoten - und kreiste im Test 50 s auf der
  // Stelle.
  int32 Von = INDEX_NONE;
  const int32 Ziel = Auto->HoleWegIndex() - 1;
  if (AlterPfad.IsValidIndex(Ziel) && FVector::Dist2D(Knoten[AlterPfad[Ziel]], Ort) < 6000.0f) Von = AlterPfad[Ziel];
  if (Von == INDEX_NONE) Von = NaechsterKnoten(Ort + Vorwaerts * 400.0f, &Vorwaerts);
  if (Von == INDEX_NONE) Von = NaechsterKnoten(Ort);
  TArray<int32> Pfad;
  if (!SuchePfad(Von, NaechsterKnoten(Spieler), Pfad)) { Auto->Parke(); S.bImEinsatz = false; return; }
  for (int32 K : Pfad) Route.Add(Knoten[K]);
  S.Pfad = Pfad;
 }
 // Querfeldein nur auf den letzten 150 m - weiter weg vom Strassennetz bleibt
 // die Streife am letzten Knoten stehen.
 if (FVector::Dist2D(Route.Last(), Spieler) < 15000.0f) Route.Add(Haltepunkt(Route.Last()));
 Auto->FolgeWeg(Route, Tempo);
}

void ALaLaBergPolizei::Einstellen(const TCHAR* Grund) {
 UE_LOG(LogTemp, Display, TEXT("LALABERG_FAHNDUNG sterne=0 grund=%s"), Grund);
 Sterne = 0; Punkte = 0.0f; FestnahmeUhr = 0.0f; bGesehen = false;
 // Die Streifen fahren ab - irgendwohin weit weg, und verschwinden, sobald
 // sie ausser Sicht sind (siehe Tick).
 const auto* PC = GetWorld()->GetFirstPlayerController();
 const FVector Spieler = PC && PC->GetPawn() ? PC->GetPawn()->GetActorLocation() : FVector::ZeroVector;
 for (FStreife& S : Streifen) {
  S.bImEinsatz = false;
  if (!IsValid(S.Auto) || S.Auto->IstGeparkt()) continue;
  int32 Weit = INDEX_NONE;
  for (int32 Versuch = 0; Versuch < 40 && Weit == INDEX_NONE; Versuch++) {
   const int32 K = FMath::RandRange(0, Knoten.Num() - 1);
   if (FVector::Dist2D(Knoten[K], Spieler) > 60000.0f) Weit = K;
  }
  const FVector Vorwaerts = S.Auto->GetActorForwardVector();
  TArray<int32> Pfad;
  if (Weit == INDEX_NONE || !SuchePfad(NaechsterKnoten(S.Auto->GetActorLocation(), &Vorwaerts), Weit, Pfad)) { S.Auto->Parke(); continue; }
  TArray<FVector> Route = { S.Auto->HoleRoutenOrt() };
  for (int32 K : Pfad) Route.Add(Knoten[K]);
  S.Auto->FolgeWeg(Route, 50.0f);
 }
}

void ALaLaBergPolizei::Festnahme() {
 Festnahmen++;
 auto* PC = GetWorld()->GetFirstPlayerController();
 int32 Strafe = 0;
 if (auto* A = ALaLaBergAuftraege::Instanz.Get()) {
  A->Abbrechen();
  Strafe = A->Strafe(100 + A->HoleGeld() / 10);
 }
 // Aus dem Wagen und zur Polizeiinspektion - dort geht es weiter.
 if (auto* Wagen = Cast<ALaLaBergWagen>(PC ? PC->GetPawn() : nullptr)) Wagen->TestAussteigen();
 const int32 K = NaechsterKnoten(INSPEKTION);
 if (PC && PC->GetPawn() && Knoten.IsValidIndex(K)) {
  PC->GetPawn()->SetActorLocation(Knoten[K] + FVector(0, 0, 110.0f), false, nullptr, ETeleportType::TeleportPhysics);
  PC->GetPawn()->GetMovementComponent()->StopMovementImmediately();
 }
 for (FStreife& S : Streifen) { S.bImEinsatz = false; if (IsValid(S.Auto)) S.Auto->Parke(); }
 Sterne = 0; Punkte = 0.0f; FestnahmeUhr = 0.0f; bGesehen = false;
 Meldung(GetWorld(), FString::Printf(TEXT("Festgenommen – %d € Strafe. Wieder frei an der Polizeiinspektion"), Strafe));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_FESTNAHME strafe=%d ort=%s"), Strafe,
        Knoten.IsValidIndex(K) ? *Knoten[K].ToString() : TEXT("-"));
}

void ALaLaBergPolizei::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 auto* PC = GetWorld()->GetFirstPlayerController();
 APawn* Spieler = PC ? PC->GetPawn() : nullptr;
 if (!Spieler || Knoten.IsEmpty()) return;
 const double Jetzt = GetWorld()->GetTimeSeconds();
 const FVector Wo = Spieler->GetActorLocation();
 // Gestohlene Streifenwagen sind weg (siehe ALaLaBergCharacter::Einsteigen).
 Streifen.RemoveAll([](const FStreife& S) { return !IsValid(S.Auto); });
 StreifenHalter.RemoveAll([](const TObjectPtr<ALaLaBergVerkehrsauto>& A) { return !IsValid(A); });

 if (Sterne == 0) {
  // Nach der Fahndung: abziehende Wagen verschwinden ausser Sicht.
  for (FStreife& S : Streifen)
   if (!S.Auto->IstGeparkt() && (S.Auto->AmZiel() || FVector::Dist2D(S.Auto->GetActorLocation(), Wo) > 25000.0f)) S.Auto->Parke();
  return;
 }

 bGesehen = false;
 for (const FStreife& S : Streifen) if (S.bImEinsatz && Sieht(S.Auto, Spieler)) { bGesehen = true; break; }
 if (bGesehen) { ZuletztGesehen = Jetzt; LetzterOrt = Wo; Ungesehen = 0.0f; }
 else {
  Ungesehen += DeltaSeconds * (ImSuchgebiet(Wo) ? 1.0f / 3.0f : 1.0f);
  if (Ungesehen > Suchdauer()) {
   Einstellen(TEXT("abgehaengt"));
   Meldung(GetWorld(), TEXT("Abgehängt – die Fahndung ist eingestellt"));
   return;
  }
 }

 // Je Stern eine Streife; neue kommen nicht alle auf einmal.
 int32 Aktiv = 0;
 for (const FStreife& S : Streifen) if (S.bImEinsatz) Aktiv++;
 if (Aktiv < Sterne && Jetzt >= NaechsterEinsatz) {
  FStreife* Frei = Streifen.FindByPredicate([](const FStreife& S) { return !S.bImEinsatz; });
  if (!Frei) Frei = &Streifen.AddDefaulted_GetRef();
  Entsende(*Frei, bGesehen ? Wo : LetzterOrt);
  NaechsterEinsatz = Jetzt + 3.0;
 }
 for (FStreife& S : Streifen) {
  if (!S.bImEinsatz) continue;
  if (FVector::Dist2D(S.Auto->GetActorLocation(), Wo) > VERLOREN) { S.Auto->Parke(); S.bImEinsatz = false; continue; }
  if (Jetzt < S.NaechstePlanung) continue;
  S.NaechstePlanung = Jetzt + 1.5;
  if (bGesehen) { S.bSucht = false; Plane(S, Wo); continue; }
  // Ungesehen: erst zum letzten bekannten Ort, dort dann von Knoten zu
  // Knoten im Suchgebiet.
  const FVector Hier = S.Auto->GetActorLocation();
  if (!S.bSucht && FVector::Dist2D(Hier, LetzterOrt) > 4000.0f) { Plane(S, LetzterOrt); continue; }
  if (!S.bSucht || FVector::Dist2D(Hier, S.Suchziel) < 1500.0f || S.Auto->AmZiel()) {
   for (int32 Versuch = 0; Versuch < 30; Versuch++) {
    const FVector& K = Knoten[FMath::RandRange(0, Knoten.Num() - 1)];
    if (FVector::Dist2D(K, LetzterOrt) < Suchradius() && FVector::Dist2D(K, Hier) > 5000.0f) { S.Suchziel = K; S.bSucht = true; break; }
   }
   if (!S.bSucht) S.Suchziel = LetzterOrt;
   S.bSucht = true;
  }
  Plane(S, S.Suchziel);
 }

 // Festnahme: neben einem Streifenwagen stehen bleiben (zu Fuss wie im
 // Wagen). Wer wieder losfaehrt oder -rennt, baut den Balken ab.
 const float Grenze = Cast<ACharacter>(Spieler) ? 250.0f : 300.0f;
 if (NaechsteStreifeCm() < FESTNAHME_ABSTAND && Spieler->GetVelocity().Size() < Grenze) {
  FestnahmeUhr += DeltaSeconds;
  if (FestnahmeUhr >= FESTNAHME_DAUER) Festnahme();
 } else {
  FestnahmeUhr = FMath::Max(0.0f, FestnahmeUhr - DeltaSeconds * 2.0f);
 }
}
