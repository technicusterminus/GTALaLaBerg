#include "LaLaBergHUD.h"
#include "LaLaBergWagen.h"
#include "LaLaBergVerkehrsauto.h"
#include "LaLaBergWaffe.h"
#include "LaLaBergCharacter.h"
#include "LaLaBergMenueSteuerung.h"
#include "LaLaBergAuftraege.h"
#include "LaLaBergPolizei.h"
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
  Minikarte();
  Auftrag();
  Fahndung();
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
 Schrift(Stadt.IsEmpty() ? FString(TEXT("Stadtplan")) : Stadt, Canvas->ClipX * 0.5f, 22 * S, 24, Weiss, true, true);
 if (const ALaLaBergAuftraege* A = ALaLaBergAuftraege::Instanz.Get(); A && (A->IstUnterwegs() || A->HatAngebot())) {
  const FVector2D P = Bildort(A->HoleWegpunkt());
  const FLinearColor F = A->IstUnterwegs() ? GELB_MARKE : BLAU_MARKE;
  Tafel(P.X - 8 * S, P.Y - 8 * S, 16 * S, 16 * S, FLinearColor(0.02f, 0.02f, 0.03f, 1.0f));
  Tafel(P.X - 6 * S, P.Y - 6 * S, 12 * S, 12 * S, F);
  const FString Name = A->IstUnterwegs() ? A->HoleZielName() : FString(TEXT("Auftrag"));
  Schrift(Name, P.X + 12 * S, P.Y - 10 * S, 14, Weiss, true);
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
 Tastenleiste(TEXT("M  Karte schließen"));
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
 Schrift(bUnterwegs ? TEXT("LIEFERUNG NACH") : TEXT("AUFTRAG VERFÜGBAR"), TX, Y + 12 * S, 11, Leise, true);
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
