#include "LaLaBergHUD.h"
#include "LaLaBergWagen.h"
#include "LaLaBergWaffe.h"
#include "LaLaBergCharacter.h"
#include "LaLaBergMenueSteuerung.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "CanvasItem.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace {
 const FLinearColor Weiss(0.96f, 0.96f, 0.94f, 1.0f);
 const FLinearColor Leise(0.82f, 0.82f, 0.80f, 0.92f);
 const FLinearColor Tinte(0.02f, 0.02f, 0.03f, 0.55f);
 // Dieselbe Reichweite wie beim Einsteigen: der Hinweis erscheint genau dann,
 // wenn die Taste auch wirkt.
 constexpr float Reichweite = 800.0f;
}

FSlateFontInfo ALaLaBergHUD::Font(float Punkt, bool bFett) const {
 const UObject* Quelle = Roboto ? static_cast<const UObject*>(Roboto) : GEngine->GetMediumFont();
 return FSlateFontInfo(Quelle, FMath::Max(6, FMath::RoundToInt(Punkt * Massstab)), bFett ? FName("Bold") : FName("Regular"));
}

float ALaLaBergHUD::Breite(const FString& Text, float Punkt, bool bFett) const {
 if (!FSlateApplication::IsInitialized()) return 0.0f;
 // Slate misst in logischen Einheiten; der Canvas zeichnet in Pixeln, die
 // Schrift aber im selben Massstab - das passt bei Faktor 1.
 return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, Font(Punkt, bFett)).X;
}

float ALaLaBergHUD::Schrift(const FString& Text, float X, float Y, float Punkt, const FLinearColor& Farbe,
                            bool bFett, bool bMittig) {
 FCanvasTextItem Item(FVector2D(X, Y), FText::FromString(Text), Font(Punkt, bFett), Farbe);
 Item.bCentreX = bMittig;
 Item.EnableShadow(FLinearColor(0, 0, 0, 0.55f), FVector2D(1.0f, 1.0f) * Massstab);
 Canvas->DrawItem(Item);
 return Breite(Text, Punkt, bFett);
}

void ALaLaBergHUD::Tafel(float X, float Y, float B, float H, const FLinearColor& Farbe) {
 FCanvasTileItem Kachel(FVector2D(X, Y), FVector2D(B, H), Farbe);
 Kachel.BlendMode = SE_BLEND_Translucent;
 Canvas->DrawItem(Kachel);
}

void ALaLaBergHUD::DrawHUD() {
 Super::DrawHUD();
 if (!Canvas || !PlayerOwner) return;
 // Im Menue und im Startbild steht nichts ueber dem Menue.
 if (UGameInstance* Spiel = GetGameInstance()) {
  if (auto* Menue = Spiel->GetSubsystem<ULaLaBergMenueSteuerung>(); Menue && Menue->IstOffen()) return;
 }
 if (!Roboto) Roboto = LoadObject<UFont>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
 // Auf 1080 Zeilen bezogen, damit Schrift und Tafeln auf jedem Bildschirm
 // gleich gross wirken.
 Massstab = FMath::Max(0.6f, Canvas->ClipY / 1080.0f);

 APawn* Figur = PlayerOwner->GetPawn();
 if (Figur) {
  // Zweimal je Sekunde reicht: zu Fuss wie im Wagen wechselt der Ort nicht
  // schneller, und die Suche laeuft ueber alle Strassen der Stadt.
  const float Jetzt = GetWorld()->GetTimeSeconds();
  if (Jetzt - OrtGeprueft > 0.5f) { OrtGeprueft = Jetzt; BestimmeOrt(Figur->GetActorLocation()); }
  Ortsanzeige();
 }
 if (auto* Wagen = Cast<ALaLaBergWagen>(Figur)) {
  Tacho(Wagen);
  Tastenleiste(TEXT("W/S  Gas / Rückwärts      A/D  Lenken      Leertaste  Bremse      Maus  Umsehen      E  Aussteigen      Esc  Menü"));
  return;
 }
 if (!Figur) return;

 bool bWagenNah = false;
 for (TActorIterator<ALaLaBergWagen> It(GetWorld()); It; ++It) {
  if (It->GetController()) continue;
  if (FVector::Dist(It->GetActorLocation(), Figur->GetActorLocation()) < Reichweite) { bWagenNah = true; break; }
 }
 if (bWagenNah) Hinweis(TEXT("E"), TEXT("Einsteigen"));
 if (auto* Held = Cast<ALaLaBergCharacter>(Figur)) Fadenkreuz(Held->HoleWaffe());
 Tastenleiste(TEXT("WASD  Gehen      Maus  Umsehen      Leertaste  Springen      Maus links  Feuern      1-4  Waffe      E  Einsteigen      Esc  Menü"));
}

// Bildmitte: ein kleines Kreuz, darunter der Name der Waffe und wie oft
// schon geschossen wurde - ohne Munitionsknappheit genuegt eine Zaehlung
// als Rueckmeldung, dass ueberhaupt etwas passiert.
void ALaLaBergHUD::Fadenkreuz(ALaLaBergWaffe* Waffe) {
 if (!Waffe) return;
 const float S = Massstab, MX = Canvas->ClipX * 0.5f, MY = Canvas->ClipY * 0.5f, L = 9.0f * S, D = 3.0f * S;
 const FLinearColor Kreuz(0.94f, 0.94f, 0.90f, 0.85f);
 Tafel(MX - L, MY - 1.0f * S, L - D, 2.0f * S, Kreuz);
 Tafel(MX + D, MY - 1.0f * S, L - D, 2.0f * S, Kreuz);
 Tafel(MX - 1.0f * S, MY - L, 2.0f * S, L - D, Kreuz);
 Tafel(MX - 1.0f * S, MY + D, 2.0f * S, L - D, Kreuz);
 Schrift(FString::Printf(TEXT("%s  ·  %d Schuss"), *Waffe->ArtName(), Waffe->Schuesse()),
         MX, MY + 22.0f * S, 13, Leise, false, true);
}

// Rechts unten, gross und ruhig: Zahl mit Einheit dicht daneben und ein Band,
// das sich mit dem Tempo fuellt. Rueckwaerts zeigt ein R statt einer
// negativen Zahl.
void ALaLaBergHUD::Tacho(ALaLaBergWagen* Wagen) {
 const float Kmh = Wagen->TempoKmh();
 const float S = Massstab;
 const float B = 230 * S, H = 118 * S;
 const float X = Canvas->ClipX - B - 40 * S, Y = Canvas->ClipY - H - 70 * S;
 Tafel(X, Y, B, H, Tinte);

 const FString Zahl = FString::Printf(TEXT("%d"), FMath::RoundToInt(FMath::Abs(Kmh)));
 const float ZB = Schrift(Zahl, X + 22 * S, Y + 8 * S, 46, Weiss, true);
 // Grundlinie der Einheit auf die der Zahl: 46 pt gegen 16 pt
 Schrift(TEXT("km/h"), X + 22 * S + ZB + 8 * S, Y + 44 * S, 16, Leise);
 // Rechts oben in der Tafel, rechtsbuendig: Bremse und Rueckwaertsgang
 const float Rechts = X + B - 18 * S;
 if (Wagen->BremstGerade()) Schrift(TEXT("BREMSE"), Rechts - Breite(TEXT("BREMSE"), 11, true), Y + 12 * S, 11, FLinearColor(1.0f, 0.42f, 0.32f), true);
 if (Kmh < -1.0f) Schrift(TEXT("R"), Rechts - Breite(TEXT("R"), 20, true), Y + 34 * S, 20, FLinearColor(1.0f, 0.62f, 0.25f), true);

 // Band bis 120 km/h, der Hoechstgeschwindigkeit des Antriebs
 const float Anteil = FMath::Clamp(FMath::Abs(Kmh) / 120.0f, 0.0f, 1.0f);
 const float BX = X + 22 * S, BY = Y + H - 24 * S, BB = B - 44 * S, BH = 6 * S;
 Tafel(BX, BY, BB, BH, FLinearColor(1, 1, 1, 0.18f));
 Tafel(BX, BY, BB * Anteil, BH,
       FLinearColor::LerpUsingHSV(FLinearColor(0.35f, 0.85f, 0.55f), FLinearColor(1.0f, 0.45f, 0.25f), Anteil));
}

// Mitte, unter dem Blickpunkt: eine Tastenkappe und das, was sie tut. Die
// Tafel richtet sich nach dem Text, nicht umgekehrt.
void ALaLaBergHUD::Hinweis(const FString& Taste, const FString& Text) {
 const float S = Massstab;
 const float K = 34 * S, Rand = 10 * S;
 const float B = Rand + K + 14 * S + Breite(Text, 20, false) + 18 * S, H = K + 2 * Rand;
 const float X = (Canvas->ClipX - B) * 0.5f, Y = Canvas->ClipY * 0.66f;
 Tafel(X, Y, B, H, Tinte);
 Tafel(X + Rand, Y + Rand, K, K, FLinearColor(0.96f, 0.96f, 0.94f, 0.94f));
 Schrift(Taste, X + Rand + K * 0.5f, Y + Rand + 4 * S, 18, FLinearColor(0.05f, 0.05f, 0.06f), true, true);
 Schrift(Text, X + Rand + K + 14 * S, Y + Rand + 3 * S, 20, Weiss);
}

// Ganz unten, klein: welche Tasten jetzt gelten.
void ALaLaBergHUD::Tastenleiste(const FString& Text) {
 Schrift(Text, Canvas->ClipX * 0.5f, Canvas->ClipY - 38 * Massstab, 13, Leise, false, true);
}

namespace {
 TArray<FVector2D> Punkte(const TArray<TSharedPtr<FJsonValue>>& Zahlen) {
  TArray<FVector2D> Aus;
  for (int32 i = 0; i + 1 < Zahlen.Num(); i += 2) Aus.Add(FVector2D(Zahlen[i]->AsNumber(), Zahlen[i + 1]->AsNumber()));
  return Aus;
 }
 bool Innen(const TArray<FVector2D>& Umriss, const FVector2D& P) {
  bool bIn = false;
  for (int32 i = 0, j = Umriss.Num() - 1; i < Umriss.Num(); j = i++) {
   const FVector2D& A = Umriss[i]; const FVector2D& B = Umriss[j];
   if ((A.Y > P.Y) != (B.Y > P.Y) && P.X < (B.X - A.X) * (P.Y - A.Y) / (B.Y - A.Y) + A.X) bIn = !bIn;
  }
  return bIn;
 }
 float AbstandZuStrecke(const FVector2D& P, const FVector2D& A, const FVector2D& B) {
  const FVector2D AB = B - A;
  const float L = AB.SizeSquared();
  const float T = L > 0 ? FMath::Clamp(FVector2D::DotProduct(P - A, AB) / L, 0.0f, 1.0f) : 0.0f;
  return FVector2D::Distance(P, A + AB * T);
 }
}

void ALaLaBergHUD::LadeOrte() {
 bOrteGeladen = true;
 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Orte/orte.json");
 if (!FFileHelper::LoadFileToString(Text, *Datei) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_ORTE fehlen: %s"), *Datei);
  return;
 }
 Stadt = Wurzel->GetStringField(TEXT("stadt"));
 const auto& A = Wurzel->GetArrayField(TEXT("altstadt"));
 if (A.Num() == 4) Altstadt = FBox2D(FVector2D(FMath::Min(A[0]->AsNumber(), A[2]->AsNumber()), FMath::Min(A[1]->AsNumber(), A[3]->AsNumber())),
                                     FVector2D(FMath::Max(A[0]->AsNumber(), A[2]->AsNumber()), FMath::Max(A[1]->AsNumber(), A[3]->AsNumber())));
 for (const auto& V : Wurzel->GetArrayField(TEXT("strassen"))) {
  FStrasse S; S.N = V->AsObject()->GetStringField(TEXT("n"));
  for (const auto& T : V->AsObject()->GetArrayField(TEXT("teile"))) S.Teile.Add({ Punkte(T->AsArray()) });
  Strassen.Add(MoveTemp(S));
 }
 for (const auto& V : Wurzel->GetArrayField(TEXT("plaetze")))
  Plaetze.Add({ V->AsObject()->GetStringField(TEXT("n")), Punkte(V->AsObject()->GetArrayField(TEXT("p"))) });
 auto LesePunkte = [&](const TCHAR* Feld, TArray<FPunkt>& Ziel) {
  for (const auto& V : Wurzel->GetArrayField(Feld)) {
   const auto O = V->AsObject();
   Ziel.Add({ O->GetStringField(TEXT("n")), FVector2D(O->GetNumberField(TEXT("x")), O->GetNumberField(TEXT("y"))),
              static_cast<float>(O->GetNumberField(TEXT("r"))) });
  }
 };
 LesePunkte(TEXT("marken"), Marken);
 LesePunkte(TEXT("ortsteile"), Ortsteile);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_ORTE strassen=%d plaetze=%d marken=%d ortsteile=%d"),
        Strassen.Num(), Plaetze.Num(), Marken.Num(), Ortsteile.Num());
}

// Vorrang: auf einem Platz steht man auf dem Platz; nahe einem Wahrzeichen
// sieht man das Wahrzeichen; sonst gilt die naechste Strasse bis 35 m.
void ALaLaBergHUD::BestimmeOrt(const FVector& Wo) {
 if (!bOrteGeladen) LadeOrte();
 const FVector2D P(Wo.X, Wo.Y);
 FString Name;
 for (const FPlatz& Platz : Plaetze) if (Innen(Platz.Umriss, P)) { Name = Platz.N; break; }
 if (Name.IsEmpty()) {
  float Beste = 1.0f;
  for (const FPunkt& M : Marken) {
   const float Anteil = FVector2D::Distance(P, M.Ort) / M.R;
   if (Anteil < Beste) { Beste = Anteil; Name = M.N; }
  }
 }
 if (Name.IsEmpty()) {
  float Beste = 3500.0f;
  for (const FStrasse& S : Strassen) {
   for (const FLinienzug& L : S.Teile) {
    for (int32 i = 0; i + 1 < L.P.Num(); i++) {
     const float D = AbstandZuStrecke(P, L.P[i], L.P[i + 1]);
     if (D < Beste) { Beste = D; Name = S.N; }
    }
   }
  }
 }
 FString Teil;
 if (Altstadt.bIsValid && Altstadt.IsInside(P)) Teil = TEXT("Altstadt");
 else {
  float Beste = 1.0f;
  for (const FPunkt& O : Ortsteile) {
   const float Anteil = FVector2D::Distance(P, O.Ort) / O.R;
   if (Anteil < Beste) { Beste = Anteil; Teil = O.N; }
  }
 }
 const FString Zusatz = Teil.IsEmpty() ? Stadt : Teil + TEXT("  ·  ") + Stadt;
 if (Name != OrtName || Zusatz != OrtZusatz) {
  OrtName = Name; OrtZusatz = Zusatz;
  OrtSeit = GetWorld()->GetTimeSeconds() - (bOrtSofort ? 1.0f : 0.0f);
  UE_LOG(LogTemp, Display, TEXT("LALABERG_ORT %s | %s"), *OrtName, *OrtZusatz);
 }
 bOrtSofort = false;
}

// Links unten, ohne Tafel: der Name gross, darunter Ortsteil und Stadt.
// Wechselt der Ort, blendet der neue Name in einer halben Sekunde ein.
void ALaLaBergHUD::Ortsanzeige() {
 if (OrtName.IsEmpty() && OrtZusatz.IsEmpty()) return;
 const float S = Massstab;
 const float Deckung = FMath::Clamp((GetWorld()->GetTimeSeconds() - OrtSeit) / 0.5f, 0.0f, 1.0f);
 const float X = 44 * S, Y = Canvas->ClipY - 138 * S;
 FLinearColor Gross = Weiss; Gross.A *= Deckung;
 FLinearColor Klein = Leise; Klein.A *= Deckung;
 if (!OrtName.IsEmpty()) Schrift(OrtName, X, Y, 24, Gross, true);
 Schrift(OrtZusatz, X + 1 * S, Y + (OrtName.IsEmpty() ? 12 : 40) * S, 13, Klein);
}
