#include "LaLaBergHUD.h"
#include "LaLaBergWagen.h"
#include "LaLaBergVerkehrsauto.h"
#include "LaLaBergWaffe.h"
#include "LaLaBergCharacter.h"
#include "LaLaBergMenueSteuerung.h"
#include "LaLaBergAuftraege.h"
#include "LaLaBergPolizei.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergLaeden.h"
#include "LaLaBergKonto.h"
#include "LaLaBergRevier.h"
#include "LaLaBergDrehbuch.h"
#include "ImageUtils.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Engine/Canvas.h"
#include "Camera/PlayerCameraManager.h"
#include "RenderUtils.h"
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

void ALaLaBergHUD::ZeigeRueckmeldung(const FString& Text) {
 Rueckmeldung = Text;
 RueckmeldungBis = GetWorld()->GetRealTimeSeconds() + 3.5;
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
 if (!Rueckmeldung.IsEmpty() && GetWorld()->GetRealTimeSeconds() < RueckmeldungBis) {
  const float MaxBreite = Canvas->ClipX - 40.0f * Massstab;
  const float TextBreite = Breite(Rueckmeldung, 18, false);
  const float Punkt = 18.0f * FMath::Min(1.0f, (MaxBreite - 28.0f * Massstab) / FMath::Max(1.0f, TextBreite));
  const float B = FMath::Min(MaxBreite, Breite(Rueckmeldung, Punkt, false) + 28.0f * Massstab);
  const float Y = Canvas->ClipY * 0.76f;
  Tafel((Canvas->ClipX-B)*0.5f, Y, B, 46.0f*Massstab, FLinearColor(0.025f,0.03f,0.04f,0.88f));
  Schrift(Rueckmeldung, Canvas->ClipX*0.5f, Y+10.0f*Massstab, Punkt, Weiss, false, true);
 }

 APawn* Figur = PlayerOwner->GetPawn();
 if (Figur) {
  // Zweimal je Sekunde reicht: zu Fuss wie im Wagen wechselt der Ort nicht
  // schneller, und die Suche laeuft ueber alle Strassen der Stadt.
  const float Jetzt = GetWorld()->GetTimeSeconds();
  if (Jetzt - OrtGeprueft > 0.5f) { OrtGeprueft = Jetzt; BestimmeOrt(Figur->GetActorLocation()); }
  Ortsanzeige();
  if (!bKarteGeladen) LadeKarte();
  if (PlayerOwner->WasInputKeyJustPressed(EKeys::M)) bVollkarte = !bVollkarte;
  if (bVollkarte) { Vollkarte(); return; }
  AktualisiereRoute();
  Minikarte();
  Auftrag();
  Fahndung();
  Leben();
  Kapitel();
  Mission();
  Laden();
 }
 if (auto* Wagen = Cast<ALaLaBergWagen>(Figur)) {
  Tacho(Wagen);
  Tastenleiste(TEXT("W/S  Gas / Rückwärts      A/D  Lenken      Leertaste  Bremse      Maus  Umsehen      E  Aussteigen      M  Karte      Esc  Menü"));
  return;
 }
 if (!Figur) return;

 bool bWagenNah = false;
 for (TActorIterator<ALaLaBergWagen> It(GetWorld()); It; ++It) {
  if (It->GetController()) continue;
  if (FVector::Dist(It->GetActorLocation(), Figur->GetActorLocation()) < Reichweite) { bWagenNah = true; break; }
 }
 // Entry also supports traffic vehicles; use the same range as Character.
 if (!bWagenNah) {
  for (TActorIterator<ALaLaBergVerkehrsauto> It(GetWorld()); It; ++It) {
   if (FVector::Dist(It->GetActorLocation(), Figur->GetActorLocation()) < Reichweite) {
    bWagenNah = true; break;
   }
  }
 }
 if (bWagenNah) Hinweis(TEXT("E"), TEXT("Einsteigen"));
 if (auto* Held = Cast<ALaLaBergCharacter>(Figur)) Fadenkreuz(Held->HoleWaffe());
 Tastenleiste(TEXT("WASD  Gehen      Maus  Umsehen      Leertaste  Springen      Maus links  Feuern      1-4  Waffe      E  Einsteigen      M  Karte      Esc  Menü"));
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

 // Der laufende Sender ueber dem Tacho - wie ein Display im Armaturenbrett.
 if (Wagen->HoleSender() > 0) {
  // Das Display ist so breit wie der Tacho: vom Titel steht da, was neben
  // dem Sendernamen noch hineinpasst, der Rest wird abgeschnitten.
  const float Platz = B - 40 * S;
  FString Zeile = FString::Printf(TEXT("♪  %s"), *Wagen->HoleSendername());
  FString Titel = Wagen->HoleTitelname();
  for (FString Versuch = Titel; !Versuch.IsEmpty(); ) {
   const FString Voll = FString::Printf(TEXT("%s  ·  %s%s"), *Zeile, *Versuch,
                                        Versuch.Len() == Titel.Len() ? TEXT("") : TEXT("…"));
   if (Breite(Voll, 13, true) <= Platz) { Zeile = Voll; break; }
   if (Versuch.Len() <= 4) break;                  // passt nur der Sender
   Versuch = Versuch.Left(Versuch.Len() - 2).TrimEnd();
  }
  Tafel(X, Y - 26 * S, B, 22 * S, Tinte);
  Schrift(Zeile, X + 22 * S, Y - 23 * S, 13, FLinearColor(0.95f, 0.72f, 0.12f), true);
 }
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

void ALaLaBergHUD::Pfeil(const FVector2D& M, float WinkelGrad, float R, const FLinearColor& Farbe) {
 const float Winkel = FMath::DegreesToRadians(WinkelGrad);
 const FVector2D Vor(FMath::Sin(Winkel), -FMath::Cos(Winkel));
 const FVector2D Quer(-Vor.Y, Vor.X);
 // Zwei Dreiecke von der Spitze zur Kerbe - ein Pfeil mit Einschnitt hinten.
 const FVector2D Spitze = M + Vor * R, Kerbe = M - Vor * R * 0.3f;
 for (const float Seite : { 1.0f, -1.0f }) {
  FCanvasTriangleItem Haelfte(Spitze, M - Vor * R * 0.6f + Quer * R * 0.62f * Seite, Kerbe, GWhiteTexture);
  Haelfte.SetColor(Farbe);
  Canvas->DrawItem(Haelfte);
 }
}

void ALaLaBergHUD::Stern(const FVector2D& M, float R, const FLinearColor& Farbe) {
 FVector2D P[10];
 for (int32 i = 0; i < 10; i++) {
  const float W = FMath::DegreesToRadians(-90.0f + i * 36.0f);
  P[i] = M + FVector2D(FMath::Cos(W), FMath::Sin(W)) * (i % 2 ? R * 0.45f : R);
 }
 for (int32 i = 0; i < 10; i++) {
  FCanvasTriangleItem Teil(M, P[i], P[(i + 1) % 10], GWhiteTexture);
  Teil.SetColor(Farbe);
  Canvas->DrawItem(Teil);
 }
}

void ALaLaBergHUD::LadeKarte() {
 bKarteGeladen = true;
 FString Text;
 TSharedPtr<FJsonObject> Info;
 const FString Ordner = FPaths::ProjectContentDir() / TEXT("SourceData/Karte");
 if (!FFileHelper::LoadFileToString(Text, *(Ordner / TEXT("karte.json"))) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Info) || !Info.IsValid()) {
  UE_LOG(LogTemp, Warning, TEXT("LALABERG_KARTE fehlt: %s"), *Ordner);
  return;
 }
 const double Cm = Info->GetNumberField(TEXT("cmProPixel"));
 KarteUrsprung = FVector2D(Info->GetNumberField(TEXT("x0")), Info->GetNumberField(TEXT("y0")));
 KarteMass = FVector2D(Info->GetNumberField(TEXT("breite")), Info->GetNumberField(TEXT("hoehe"))) * Cm;
 auto Lade = [&](const TCHAR* Datei) -> UTexture2D* {
  UTexture2D* Bild = FImageUtils::ImportFileAsTexture2D(Ordner / Datei);
  if (!Bild) return nullptr;
  // Am Rand nicht wiederholen: die Minikarte schaut am Stadtrand ueber das
  // Bild hinaus.
  Bild->AddressX = TA_Clamp; Bild->AddressY = TA_Clamp;
  Bild->UpdateResource();
  return Bild;
 };
 KarteBild = Lade(TEXT("karte.png"));
 KarteKlein = Lade(TEXT("karte-klein.png"));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KARTE gross=%d klein=%d"), KarteBild != nullptr, KarteKlein != nullptr);
}

namespace {
 const FLinearColor BLAU_MARKE(0.25f, 0.6f, 1.0f), GELB_MARKE(1.0f, 0.78f, 0.05f);
}

void ALaLaBergHUD::AktualisiereRoute() {
 const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get();
 const ALaLaBergPolizei* Netz = ALaLaBergPolizei::Instanz.Get();
 APawn* Figur = PlayerOwner->GetPawn();
 if (!A || !Netz || !Figur || (!A->IstUnterwegs() && !A->HatAngebot())) { Route.Reset(); return; }
 const FVector Ziel = A->HoleWegpunkt();
 const double Jetzt = GetWorld()->GetRealTimeSeconds();
 // Neues Ziel sofort, sonst einmal je Sekunde - die Suche laeuft ueber
 // 6000 Knoten, jedes Bild waere Verschwendung.
 if (Jetzt - RouteZeit < 1.0 && Ziel.Equals(RouteZiel, 1.0f)) return;
 RouteZeit = Jetzt;
 RouteZiel = Ziel;
 if (!Netz->Route(Figur->GetActorLocation(), Ziel, Route)) Route.Reset();
}

namespace {
 // Liang-Barsky: Strecke auf ein Rechteck zuschneiden; false, wenn sie ganz
 // ausserhalb liegt.
 bool Schneide(FVector2D& A, FVector2D& B, const FBox2D& R) {
  const FVector2D D = B - A;
  float T0 = 0.0f, T1 = 1.0f;
  const float P[4] = { -D.X, D.X, -D.Y, D.Y };
  const float Q[4] = { A.X - R.Min.X, R.Max.X - A.X, A.Y - R.Min.Y, R.Max.Y - A.Y };
  for (int32 i = 0; i < 4; i++) {
   if (FMath::IsNearlyZero(P[i])) { if (Q[i] < 0.0f) return false; continue; }
   const float T = Q[i] / P[i];
   if (P[i] < 0.0f) T0 = FMath::Max(T0, T); else T1 = FMath::Min(T1, T);
   if (T0 > T1) return false;
  }
  const FVector2D A0 = A;
  A = A0 + D * T0;
  B = A0 + D * T1;
  return true;
 }
}

void ALaLaBergHUD::ZeichneRoute(TFunctionRef<FVector2D(const FVector&)> Bildort, const FBox2D* Rahmen, float Dicke) {
 const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get();
 if (Route.Num() < 2 || !A) return;
 const FLinearColor Farbe = A->IstUnterwegs() ? FLinearColor(1.0f, 0.78f, 0.05f, 0.95f) : FLinearColor(0.25f, 0.6f, 1.0f, 0.95f);
 // Erst ein dunkler, breiterer Strich, dann die Farbe - so bleibt die Route
 // auf hellen Plaetzen und dunklen Strassen gleich gut sichtbar.
 for (int32 Durchgang = 0; Durchgang < 2; Durchgang++) {
  for (int32 i = 0; i + 1 < Route.Num(); i++) {
   FVector2D P0 = Bildort(Route[i]), P1 = Bildort(Route[i + 1]);
   if (Rahmen && !Schneide(P0, P1, *Rahmen)) continue;
   FCanvasLineItem Strich(P0, P1);
   Strich.LineThickness = Durchgang == 0 ? Dicke + 3.0f * Massstab : Dicke;
   Strich.SetColor(Durchgang == 0 ? FLinearColor(0.02f, 0.02f, 0.03f, 0.7f) : Farbe);
   Canvas->DrawItem(Strich);
  }
 }
}

// Unten links ueber der Ortsanzeige, Norden oben, der Spieler in der Mitte.
// Zu Fuss 300 m im Blick, im Wagen 500 m.
void ALaLaBergHUD::Minikarte() {
 APawn* Figur = PlayerOwner->GetPawn();
 if (!KarteBild || !Figur) return;
 const float S = Massstab;
 const float G = 250 * S;
 const float X = 44 * S, Y = Canvas->ClipY - 138 * S - 26 * S - G;
 const FVector Wo = Figur->GetActorLocation();
 const float Halb = Cast<ALaLaBergCharacter>(Figur) ? 15000.0f : 25000.0f;
 const FVector2D Mitte = (FVector2D(Wo) - KarteUrsprung) / KarteMass;
 const FVector2D Spanne = FVector2D(Halb, Halb) / KarteMass;
 Tafel(X - 3 * S, Y - 3 * S, G + 6 * S, G + 6 * S, FLinearColor(0.02f, 0.025f, 0.035f, 0.85f));
 FCanvasTileItem Bild(FVector2D(X, Y), KarteBild->GetResource(), FVector2D(G, G), Mitte - Spanne, Mitte + Spanne, FLinearColor::White);
 Bild.BlendMode = SE_BLEND_Opaque;
 Canvas->DrawItem(Bild);

 const FVector2D Zentrum(X + G * 0.5f, Y + G * 0.5f);
 auto Bildort = [&](const FVector& W) { return Zentrum + (FVector2D(W) - FVector2D(Wo)) / (2.0f * Halb) * G; };
 const FBox2D Rahmen(FVector2D(X, Y), FVector2D(X + G, Y + G));
 ZeichneRoute(Bildort, &Rahmen, 3.0f * S);
 auto Eingesperrt = [&](FVector2D P, float Rand) {
  return FVector2D(FMath::Clamp(P.X, X + Rand, X + G - Rand), FMath::Clamp(P.Y, Y + Rand, Y + G - Rand));
 };
 // Auftrag: ausserhalb des Ausschnitts am Rand festgehalten - man sieht, in
 // welcher Richtung es weitergeht.
 if (const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get(); A && (A->IstUnterwegs() || A->HatAngebot())) {
  const FVector2D P = Eingesperrt(Bildort(A->HoleWegpunkt()), 8 * S);
  Tafel(P.X - 7 * S, P.Y - 7 * S, 14 * S, 14 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
  Tafel(P.X - 5 * S, P.Y - 5 * S, 10 * S, 10 * S, A->IstUnterwegs() ? GELB_MARKE : BLAU_MARKE);
 }
 // Laeden: gruen, nur wenn im Ausschnitt - man sucht sie auf der Vollkarte.
 if (const ALaLaBergLaeden* L = ALaLaBergLaeden::Instanz.Get())
  for (const auto& Laden : L->HoleLaeden()) {
   if (!Laden.bAufgestellt) continue;
   const FVector2D P = Bildort(Laden.Ort);
   if (P.X < X || P.Y < Y || P.X > X + G || P.Y > Y + G) continue;
   Tafel(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
   Tafel(P.X - 4 * S, P.Y - 4 * S, 8 * S, 8 * S, FLinearColor(0.1f, 0.85f, 0.35f));
  }
 // Wahrzeichen des offenen Reviers: Raute in der Farbe der Mannschaft,
 // markierte in Orange.
 if (const ALaLaBergRevier* R = ALaLaBergRevier::Instanz.Get()) {
  const int32 Offen = R->HoleOffenes();
  if (R->HoleReviere().IsValidIndex(Offen))
   for (const auto& M : R->HoleReviere()[Offen].Marken) {
    const FVector2D P = Bildort(M.Ort);
    if (P.X < X || P.Y < Y || P.X > X + G || P.Y > Y + G) continue;
    Tafel(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
    Tafel(P.X - 4 * S, P.Y - 4 * S, 8 * S, 8 * S,
          M.bMarkiert ? FLinearColor(0.95f, 0.45f, 0.05f) : R->HoleReviere()[Offen].Farbe);
   }
 }
 if (const ALaLaBergPolizei* Pol = ALaLaBergPolizei::Instanz.Get()) {
  TArray<FVector> Orte; Pol->HoleStreifen(Orte);
  const bool bRot = FMath::Frac(GetWorld()->GetRealTimeSeconds() * 2.5f) < 0.5f;
  for (const FVector& O : Orte) {
   const FVector2D P = Bildort(O);
   if (P.X < X || P.Y < Y || P.X > X + G || P.Y > Y + G) continue;
   // Rot und Weiss im Wechsel - Blau ist schon die Auftragsfarbe.
   Tafel(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
   Tafel(P.X - 4 * S, P.Y - 4 * S, 8 * S, 8 * S, bRot ? FLinearColor(1.0f, 0.2f, 0.2f) : FLinearColor(1.0f, 1.0f, 1.0f));
  }
 }
 // Blickrichtung der Figur bzw. Fahrtrichtung: Gier 0 zeigt nach Osten,
 // also rechts - im Pfeilwinkel (0 = oben) 90 Grad.
 Pfeil(Zentrum + FVector2D(1, 1) * S, Figur->GetActorRotation().Yaw + 90.0f, 11 * S, FLinearColor(0, 0, 0, 0.6f));
 Pfeil(Zentrum, Figur->GetActorRotation().Yaw + 90.0f, 11 * S, Weiss);
 Schrift(TEXT("N"), X + G * 0.5f, Y + 4 * S, 12, Weiss, true, true);
}

// Taste M: die ganze Stadt, abgedunkelter Hintergrund, Auftrag und Streifen.
void ALaLaBergHUD::Vollkarte() {
 APawn* Figur = PlayerOwner->GetPawn();
 if (!KarteKlein || !Figur) return;
 const float S = Massstab;
 Tafel(0, 0, Canvas->ClipX, Canvas->ClipY, FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
 const float H = Canvas->ClipY - 150 * S;
 const float B = H * KarteMass.X / KarteMass.Y;
 const float X = (Canvas->ClipX - B) * 0.5f, Y = 70 * S;
 FCanvasTileItem Bild(FVector2D(X, Y), KarteKlein->GetResource(), FVector2D(B, H), FLinearColor::White);
 Bild.BlendMode = SE_BLEND_Opaque;
 Canvas->DrawItem(Bild);
 auto Bildort = [&](const FVector& W) { return FVector2D(X, Y) + (FVector2D(W) - KarteUrsprung) / KarteMass * FVector2D(B, H); };
 ZeichneRoute(Bildort, nullptr, 3.0f * S);
 Schrift(Stadt.IsEmpty() ? FString(TEXT("Stadtplan")) : Stadt, Canvas->ClipX * 0.5f, 22 * S, 24, Weiss, true, true);
 if (const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get(); A && (A->IstUnterwegs() || A->HatAngebot())) {
  const FVector2D P = Bildort(A->HoleWegpunkt());
  const FLinearColor F = A->IstUnterwegs() ? GELB_MARKE : BLAU_MARKE;
  Tafel(P.X - 8 * S, P.Y - 8 * S, 16 * S, 16 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
  Tafel(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S, F);
  const FString Name = A->IstUnterwegs() ? A->HoleZielName() : FString(TEXT("Auftrag"));
  Schrift(Name, P.X + 12 * S, P.Y - 10 * S, 14, Weiss, true);
 }
 if (const ALaLaBergRevier* R = ALaLaBergRevier::Instanz.Get()) {
  const int32 Offen = R->HoleOffenes();
  if (R->HoleReviere().IsValidIndex(Offen))
   for (const auto& M : R->HoleReviere()[Offen].Marken) {
    const FVector2D P = Bildort(M.Ort);
    Tafel(P.X - 8 * S, P.Y - 8 * S, 16 * S, 16 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
    Tafel(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S,
          M.bMarkiert ? FLinearColor(0.95f, 0.45f, 0.05f) : R->HoleReviere()[Offen].Farbe);
    Schrift(M.Name, P.X + 12 * S, P.Y - 9 * S, 12, M.bMarkiert ? Leise : Weiss, true);
   }
 }
 if (const ALaLaBergLaeden* L = ALaLaBergLaeden::Instanz.Get())
  for (const auto& Laden : L->HoleLaeden()) {
   if (!Laden.bAufgestellt) continue;
   const FVector2D P = Bildort(Laden.Ort);
   Tafel(P.X - 7 * S, P.Y - 7 * S, 14 * S, 14 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
   Tafel(P.X - 5 * S, P.Y - 5 * S, 10 * S, 10 * S, FLinearColor(0.1f, 0.85f, 0.35f));
   Schrift(Laden.Name, P.X + 11 * S, P.Y - 9 * S, 12, Weiss, true);
  }
 if (const ALaLaBergPolizei* Pol = ALaLaBergPolizei::Instanz.Get()) {
  TArray<FVector> Orte; Pol->HoleStreifen(Orte);
  for (const FVector& O : Orte) {
   const FVector2D P = Bildort(O);
   Tafel(P.X - 5 * S, P.Y - 5 * S, 10 * S, 10 * S, FLinearColor(1.0f, 0.2f, 0.2f));
  }
 }
 const FVector2D Ich = Bildort(Figur->GetActorLocation());
 Pfeil(Ich + FVector2D(1, 1) * S, Figur->GetActorRotation().Yaw + 90.0f, 13 * S, FLinearColor(0, 0, 0, 0.7f));
 Pfeil(Ich, Figur->GetActorRotation().Yaw + 90.0f, 13 * S, Weiss);
 Figurblatt();
 Tastenleiste(TEXT("M  Karte schließen"));
}

// Das Blatt zur Figur, unten links auf der Vollkarte: was sie kann, was sie
// hat und wie weit sie in der Stadt gekommen ist. Die Werte wachsen durch
// Benutzung (siehe ULaLaBergKonto::Uebe).
void ALaLaBergHUD::Figurblatt() {
 const auto* Konto = ULaLaBergKonto::Hole(this);
 if (!Konto) return;
 const float S = Massstab;
 // Hoch genug fuer die Revierzeile - bei 216 stiess sie unten an den Rand.
 const float B = 300 * S, H = 248 * S;
 const float X = 40 * S, Y = Canvas->ClipY - H - 40 * S;
 Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.88f));
 Tafel(X, Y, 5 * S, H, FLinearColor(0.95f, 0.72f, 0.12f));
 Schrift(TEXT("FIGUR"), X + 22 * S, Y + 16 * S, 12, Leise, true);
 const FString Geld = FString::Printf(TEXT("%d €"), Konto->HoleGeld());
 Schrift(Geld, X + B - 22 * S - Breite(Geld, 18, true), Y + 12 * S, 18, Weiss, true);

 struct FZeile { const TCHAR* Name; ULaLaBergKonto::EWert Wert; };
 static const FZeile ZEILEN[] = {
  { TEXT("Ausdauer"), ULaLaBergKonto::EWert::Ausdauer },
  { TEXT("Zielsicherheit"), ULaLaBergKonto::EWert::Zielsicherheit },
  { TEXT("Fahren"), ULaLaBergKonto::EWert::Fahren },
  { TEXT("Ruf"), ULaLaBergKonto::EWert::Ruf },
 };
 float ZY = Y + 46 * S;
 for (const FZeile& Z : ZEILEN) {
  Schrift(Z.Name, X + 22 * S, ZY, 14, Weiss);
  // Fuenf Kaestchen: gefuellte fuer die erreichte Stufe.
  const int32 Stufe = Konto->Stufe(Z.Wert);
  for (int32 i = 0; i < 5; i++) {
   const float KX = X + 168 * S + i * 24 * S;
   Tafel(KX, ZY + 2 * S, 18 * S, 12 * S, FLinearColor(1, 1, 1, 0.12f));
   if (i < Stufe) Tafel(KX, ZY + 2 * S, 18 * S, 12 * S, FLinearColor(0.95f, 0.72f, 0.12f));
  }
  ZY += 30 * S;
 }
 const FString Auftraege = FString::Printf(TEXT("%d Aufträge erledigt"), Konto->HoleErledigt());
 Schrift(Auftraege, X + 22 * S, ZY + 6 * S, 13, Leise);
 // Reviere: vier Kaestchen, gefuellt, was einem gehoert.
 Schrift(TEXT("Reviere"), X + 22 * S, ZY + 30 * S, 13, Leise);
 for (int32 i = 0; i < 4; i++) {
  const float KX = X + 168 * S + i * 24 * S;
  Tafel(KX, ZY + 32 * S, 18 * S, 12 * S, FLinearColor(1, 1, 1, 0.12f));
  if (Konto->HatRevier(i)) Tafel(KX, ZY + 32 * S, 18 * S, 12 * S, FLinearColor(0.2f, 0.78f, 0.34f));
 }
}

// Oben rechts unter der Auftragstafel: an welchem Kapitel man steht und wie
// weit das offene Revier markiert ist. Ohne das stehen die Saeulen ohne
// Erklaerung in der Stadt herum.
void ALaLaBergHUD::Kapitel() {
 const ALaLaBergRevier* R = ALaLaBergRevier::Instanz.Get();
 const auto* Konto = ULaLaBergKonto::Hole(this);
 if (!R || !Konto) return;
 const int32 Offen = R->HoleOffenes();
 const float S = Massstab;
 const float B = 340 * S, H = 64 * S;
 const float X = Canvas->ClipX - B - 40 * S;
 // Unter der Auftragstafel (40 + 104) beziehungsweise unter den Sternen.
 const float Y = 40 * S + 116 * S;
 if (!R->HoleReviere().IsValidIndex(Offen)) {
  // Alles genommen - die Stadt gehoert dem Spieler.
  Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.78f));
  Tafel(X, Y, 5 * S, H, FLinearColor(0.95f, 0.72f, 0.12f));
  Schrift(TEXT("DER FARBKRIEG"), X + 24 * S, Y + 12 * S, 11, Leise, true);
  Schrift(TEXT("Die Stadt gehört dir"), X + 24 * S, Y + 30 * S, 17, Weiss, true);
  return;
 }
 const auto& Revier = R->HoleReviere()[Offen];
 const int32 Stand = R->HoleMarkiert(Offen), Ganz = Revier.Marken.Num();
 Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.78f));
 Tafel(X, Y, 5 * S, H, Revier.Farbe);
 Schrift(FString::Printf(TEXT("KAPITEL %d · %s"), Konto->HoleKapitel() + 1, *Revier.Mannschaft),
         X + 24 * S, Y + 10 * S, 11, Leise, true);
 // Laeuft die Jagd auf den Kopf, steht sein Name da statt des Reviers -
 // und der Pfeil zeigt, wo er gerade faehrt.
 if (R->KopfLaeuft()) {
  Schrift(R->HoleKopfName(), X + 24 * S, Y + 26 * S, 16, FLinearColor(1.0f, 0.42f, 0.32f), true);
  APawn* Ich = PlayerOwner->GetPawn();
  if (Ich) {
   const float Kamera = PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw
                                                          : PlayerOwner->GetControlRotation().Yaw;
   Pfeil(FVector2D(X + B - 40 * S, Y + H * 0.5f),
         (R->HoleKopfOrt() - Ich->GetActorLocation()).Rotation().Yaw - Kamera, 18 * S,
         FLinearColor(1.0f, 0.42f, 0.32f));
  }
  return;
 }
 Schrift(Revier.Name, X + 24 * S, Y + 26 * S, 16, Weiss, true);
 // Sind alle Wahrzeichen markiert, fehlt nur noch die Bewaehrung - dann
 // steht sie hier statt der Kaestchen.
 if (Stand >= Ganz) {
  int32 Erledigt = 0, Noetig = 0;
  R->HoleBewaehrung(Offen, Erledigt, Noetig);
  if (Erledigt < Noetig) {
   static const TCHAR* ARTNAMEN[] = { TEXT("Lieferungen"), TEXT("Taxifahrten"), TEXT("Rennen"), TEXT("Verfolgungen") };
   const FString Text = FString::Printf(TEXT("%d/%d %s"), Erledigt, Noetig,
                                        ARTNAMEN[FMath::Clamp(Revier.Bewaehrungsart, 0, 3)]);
   Schrift(Text, X + B - 24 * S - Breite(Text, 14, true), Y + 28 * S, 14, FLinearColor(0.95f, 0.72f, 0.12f), true);
   return;
  }
 }
 // Vier Kaestchen fuer die vier Wahrzeichen.
 for (int32 i = 0; i < Ganz; i++) {
  const float KX = X + B - 24 * S - (Ganz - i) * 26 * S;
  Tafel(KX, Y + 30 * S, 20 * S, 12 * S, FLinearColor(1, 1, 1, 0.14f));
  if (i < Stand) Tafel(KX, Y + 30 * S, 20 * S, 12 * S, FLinearColor(0.95f, 0.45f, 0.05f));
 }
}

// Die laufende Mission, unter der Kapitelzeile: was gerade zu tun ist, die
// wievielte Stufe von wie vielen, die Frist und ein Pfeil zum Ziel. Ohne
// laufende Mission steht dort, wo die naechste wartet.
void ALaLaBergHUD::Mission() {
 const ALaLaBergDrehbuch* D = ALaLaBergDrehbuch::Instanz.Get();
 APawn* Figur = PlayerOwner->GetPawn();
 if (!D || !Figur) return;
 const bool bLaeuft = D->Laeuft();
 if (!bLaeuft && D->HoleAngebot() == INDEX_NONE) return;
 const float S = Massstab;
 const float B = 340 * S, H = 76 * S;
 const float X = Canvas->ClipX - B - 40 * S;
 const float Y = 40 * S + 116 * S + 76 * S;      // unter der Kapitelzeile
 const FLinearColor Farbe = bLaeuft ? FLinearColor(0.12f, 0.85f, 0.42f) : FLinearColor(0.12f, 0.6f, 0.34f);
 Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.78f));
 Tafel(X, Y, 5 * S, H, Farbe);
 const FVector Ziel = bLaeuft ? D->HoleStufenort() : D->HoleAngebotsort();
 const float Kamera = PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw
                                                        : PlayerOwner->GetControlRotation().Yaw;
 Pfeil(FVector2D(X + B - 34 * S, Y + H * 0.5f), (Ziel - Figur->GetActorLocation()).Rotation().Yaw - Kamera, 16 * S, Farbe);
 if (!bLaeuft) {
  Schrift(TEXT("MISSION WARTET"), X + 24 * S, Y + 10 * S, 11, Leise, true);
  Schrift(D->HoleAngebotsname(), X + 24 * S, Y + 26 * S, 16, Weiss, true);
  const float Meter = FVector::Dist2D(Figur->GetActorLocation(), Ziel) / 100.0f;
  Schrift(Meter >= 1000.0f ? FString::Printf(TEXT("%.1f km"), Meter / 1000.0f).Replace(TEXT("."), TEXT(","))
                           : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meter)),
          X + 24 * S, Y + 50 * S, 13, Leise);
  return;
 }
 Schrift(FString::Printf(TEXT("%s · STUFE %d/%d"), *D->HoleMissionsname().ToUpper(), D->HoleStufe(), D->HoleStufen()),
         X + 24 * S, Y + 10 * S, 11, Leise, true);
 const FString Text = D->HoleStufentext();
 const float Platz = B - 80 * S;
 const float Punkt = 15.0f * FMath::Min(1.0f, Platz / FMath::Max(1.0f, Breite(Text, 15, true)));
 Schrift(Text, X + 24 * S, Y + 28 * S, Punkt, Weiss, true);
 const float Rest = D->HoleRestzeit();
 if (Rest > 0.0f) {
  const int32 R = FMath::CeilToInt(Rest);
  Schrift(FString::Printf(TEXT("%d:%02d"), R / 60, R % 60), X + 24 * S, Y + 50 * S, 14,
          R <= 15 ? FLinearColor(1.0f, 0.42f, 0.32f) : Leise, true);
 }
}

void ALaLaBergHUD::Leben() {
 const auto* PC = GetOwningPlayerController();
 const APawn* Figur = PC ? PC->GetPawn() : nullptr;
 const auto* Verletzbar = Cast<ILaLaBergVerletzbar>(Figur);
 if (!Verletzbar) return;
 const float Anteil = Verletzbar->Lebensanteil();
 const float S = Massstab;
 const float X = 30 * S, Y = 26 * S, B = 260 * S, H = 16 * S;
 Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.78f));
 // Gruen, gelb, rot - nach Anteil, nicht nach fester Schwelle.
 const FLinearColor Farbe = Anteil > 0.55f ? FLinearColor(0.20f, 0.78f, 0.34f)
                          : Anteil > 0.25f ? FLinearColor(0.92f, 0.72f, 0.16f)
                                           : FLinearColor(0.88f, 0.22f, 0.18f);
 Tafel(X + 2 * S, Y + 2 * S, (B - 4 * S) * Anteil, H - 4 * S, Farbe);
 Schrift(TEXT("Leben"), X + B + 12 * S, Y + 1 * S, 12, Leise, true);
}

void ALaLaBergHUD::Fahndung() {
 const ALaLaBergPolizei* Pol = ALaLaBergPolizei::Instanz.Get();
 if (!Pol) return;
 const float S = Massstab;
 const int32 Sterne = Pol->HoleSterne();
 if (Sterne > 0) {
  const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get();
  const bool bTafel = A && (A->IstUnterwegs() || A->HatAngebot());
  const float Y = (bTafel ? 40 + 104 + 14 : 40) * S;
  const float R = 14 * S, Abstand = 33 * S;
  const float Rechts = Canvas->ClipX - 40 * S;
  // Ungesehen blinken die Sterne - wie lange noch, zeigt der Balken darunter.
  const float Deckung = Pol->WirdGesehen() ? 1.0f : 0.35f + 0.65f * FMath::Abs(FMath::Sin(GetWorld()->GetRealTimeSeconds() * 4.0f));
  for (int32 i = 0; i < 5; i++) {
   const FVector2D M(Rechts - R - (4 - i) * Abstand, Y + R);
   Stern(M + FVector2D(1.5f, 1.5f) * S, R, FLinearColor(0, 0, 0, 0.55f));
   Stern(M, R, i < Sterne ? FLinearColor(1.0f, 0.95f, 0.85f, Deckung) : FLinearColor(0.3f, 0.3f, 0.3f, 0.6f));
  }
  if (!Pol->WirdGesehen()) {
   const float BB = 4 * Abstand + 2 * R, BX = Rechts - BB, BY = Y + 2 * R + 8 * S;
   Tafel(BX, BY, BB, 4 * S, FLinearColor(1, 1, 1, 0.18f));
   Tafel(BX, BY, BB * Pol->HoleSuchAnteil(), 4 * S, FLinearColor(0.35f, 0.85f, 0.55f));
   const FString Text = TEXT("Ungesehen – abhängen");
   Schrift(Text, Rechts - Breite(Text, 11, false), BY + 8 * S, 11, Leise);
  }
 }
 const float Anteil = Pol->HoleFestnahmeAnteil();
 if (Anteil > 0.0f) {
  const float B = 360 * S, X = (Canvas->ClipX - B) * 0.5f, Y = Canvas->ClipY * 0.62f;
  Tafel(X, Y, B, 52 * S, Tinte);
  Schrift(TEXT("Festnahme – weiterfahren oder wegrennen!"), Canvas->ClipX * 0.5f, Y + 8 * S, 15, Weiss, true, true);
  Tafel(X + 14 * S, Y + 36 * S, B - 28 * S, 6 * S, FLinearColor(1, 1, 1, 0.18f));
  Tafel(X + 14 * S, Y + 36 * S, (B - 28 * S) * FMath::Clamp(Anteil, 0.0f, 1.0f), 6 * S, FLinearColor(1.0f, 0.35f, 0.3f));
 }
}

// Mitte links: der Laden, in dem man steht. Die Liste sagt selbst, was
// geht - gekauft, zu teuer oder zu haben - und unten, mit welchen Tasten.
void ALaLaBergHUD::Laden() {
 const ALaLaBergLaeden* L = ALaLaBergLaeden::Instanz.Get();
 const ULaLaBergKonto* Konto = ULaLaBergKonto::Hole(this);
 if (!L || !Konto || L->HoleOffen() == INDEX_NONE) return;
 const auto& Laden = L->HoleLaeden()[L->HoleOffen()];
 const float S = Massstab;
 const float Zeile = 38 * S, B = 420 * S;
 const float H = 86 * S + Laden.Waren.Num() * Zeile + 44 * S;
 const float X = 60 * S, Y = (Canvas->ClipY - H) * 0.42f;
 Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.88f));
 Tafel(X, Y, 5 * S, H, FLinearColor(0.1f, 0.85f, 0.35f));
 Schrift(Laden.Name, X + 24 * S, Y + 16 * S, 22, Weiss, true);
 const FString Stand = FString::Printf(TEXT("Konto %d €"), Konto->HoleGeld());
 Schrift(Stand, X + B - 20 * S - Breite(Stand, 14, true), Y + 24 * S, 14, Leise, true);
 float ZY = Y + 70 * S;
 for (int32 i = 0; i < Laden.Waren.Num(); i++) {
  const auto& Ware = Laden.Waren[i];
  const bool bGewaehlt = i == L->HoleAuswahl();
  const bool bSchon = L->HatSchon(Ware);
  const bool bZuTeuer = !bSchon && Ware.Preis > Konto->HoleGeld();
  if (bGewaehlt) Tafel(X + 12 * S, ZY - 4 * S, B - 24 * S, Zeile - 2 * S, FLinearColor(1, 1, 1, 0.12f));
  float TX = X + 24 * S;
  if (Ware.Art == ALaLaBergLaeden::EArt::Lack) {
   // Farbfeld vor dem Namen - man kauft eine Farbe, nicht ein Wort.
   Tafel(TX, ZY + 4 * S, 22 * S, 22 * S, FLinearColor(0.6f, 0.6f, 0.6f, 1.0f));
   Tafel(TX + 2 * S, ZY + 6 * S, 18 * S, 18 * S, Ware.Farbe);
   TX += 32 * S;
  }
  const FLinearColor Farbe = bSchon || bZuTeuer ? Leise : Weiss;
  Schrift(Ware.Name, TX, ZY + 4 * S, 17, Farbe, bGewaehlt);
  const FString Preis = bSchon ? (Ware.Art == ALaLaBergLaeden::EArt::Waffe ? FString(TEXT("gekauft")) : FString(TEXT("aktuell")))
                               : FString::Printf(TEXT("%d €"), Ware.Preis);
  Schrift(Preis, X + B - 24 * S - Breite(Preis, 16, true), ZY + 5 * S, 16,
          bZuTeuer ? FLinearColor(1.0f, 0.42f, 0.32f) : Farbe, true);
  ZY += Zeile;
 }
 Schrift(TEXT("Pfeil hoch/runter  wählen      Enter  kaufen      hinausgehen  schließen"), X + 24 * S, Y + H - 32 * S, 12, Leise);
}

void ALaLaBergHUD::Auftrag() {
 const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get();
 APawn* Figur = PlayerOwner->GetPawn();
 if (!A || !Figur || (!A->IstUnterwegs() && !A->HatAngebot())) return;
 const float S = Massstab;
 const bool bUnterwegs = A->IstUnterwegs();
 const FVector Ziel = A->HoleWegpunkt();
 const FVector Wo = Figur->GetActorLocation();
 const float Meter = FVector::Dist2D(Wo, Ziel) / 100.0f;
 const FString Weg = Meter >= 1000.0f ? FString::Printf(TEXT("%.1f km"), Meter / 1000.0f).Replace(TEXT("."), TEXT(","))
                                      : FString::Printf(TEXT("%d m"), FMath::RoundToInt(Meter));
 const FLinearColor Farbe = bUnterwegs ? FLinearColor(1.0f, 0.78f, 0.05f) : FLinearColor(0.25f, 0.6f, 1.0f);

 const float B = 340 * S, H = 104 * S;
 const float X = Canvas->ClipX - B - 40 * S, Y = 40 * S;
 // Dichter als die anderen Tafeln: sie steht oben vor hellem Himmel.
 Tafel(X, Y, B, H, FLinearColor(0.02f, 0.025f, 0.035f, 0.78f));
 Tafel(X, Y, 5 * S, H, Farbe);

 // Pfeil in einem Kreis links: oben ist, wohin die Kamera schaut.
 const float PX = X + 52 * S, PY = Y + H * 0.5f, R = 30 * S;
 const float Kamera = PlayerOwner->PlayerCameraManager ? PlayerOwner->PlayerCameraManager->GetCameraRotation().Yaw
                                                        : PlayerOwner->GetControlRotation().Yaw;
 Pfeil(FVector2D(PX, PY), (Ziel - Wo).Rotation().Yaw - Kamera, R, Farbe);

 const float TX = X + 100 * S;
 // Die Zeile ueber dem Ziel sagt, was man gerade faehrt.
 const ELaLaBergAuftragsart Art = A->HoleArt();
 const TCHAR* Kopfzeile =
   Art == ELaLaBergAuftragsart::Taxi ? TEXT("FAHRGAST NACH")
 : Art == ELaLaBergAuftragsart::Krankenwagen ? TEXT("VERLETZTER NACH")
 : Art == ELaLaBergAuftragsart::Streife ? TEXT("STREIFE")
 : Art == ELaLaBergAuftragsart::Verfolgung ? TEXT("VERFOLGUNG")
 : Art == ELaLaBergAuftragsart::Rennen ? TEXT("RENNEN")
                                       : TEXT("LIEFERUNG NACH");
 Schrift(bUnterwegs ? Kopfzeile : TEXT("AUFTRAG VERFÜGBAR"), TX, Y + 12 * S, 11, Leise, true);
 const FString Titel = bUnterwegs ? A->HoleZielName() : FString(TEXT("Zur blauen Säule"));
 const float Platz = B - (TX - X) - 16 * S;
 const float Punkt = 19.0f * FMath::Min(1.0f, Platz / FMath::Max(1.0f, Breite(Titel, 19, true)));
 Schrift(Titel, TX, Y + 30 * S, Punkt, Weiss, true);
 FString Zeile = Weg;
 FLinearColor ZeilenFarbe = Leise;
 if (bUnterwegs) {
  const int32 Rest = FMath::CeilToInt(A->HoleRestzeit());
  Zeile += FString::Printf(TEXT("   ·   %d:%02d"), Rest / 60, Rest % 60);
  // Die letzten 20 Sekunden in Rot - dann lohnt kein Umweg mehr.
  if (Rest <= 20) ZeilenFarbe = FLinearColor(1.0f, 0.42f, 0.32f);
 }
 Schrift(Zeile, TX, Y + 60 * S, 15, ZeilenFarbe);
 const FString Geld = bUnterwegs ? FString::Printf(TEXT("+%d €"), A->HoleLohn()) : FString::Printf(TEXT("%d €"), A->HoleGeld());
 Schrift(Geld, X + B - 16 * S - Breite(Geld, 15, true), Y + 60 * S, 15, bUnterwegs ? Farbe : Weiss, true);
 // Kontostand auch waehrend der Fahrt, klein darunter.
 if (bUnterwegs) {
  const FString Konto = FString::Printf(TEXT("Konto %d €"), A->HoleGeld());
  Schrift(Konto, X + B - 16 * S - Breite(Konto, 11, false), Y + 82 * S, 11, Leise);
 }
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
