#include "LaLaBergMenue.h"
#include "LaLaBergMenueSteuerung.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"

namespace {
 // Ein knapper, ruhiger Satz Farben. Kontrast gegen den hellen Himmel und
 // gegen dunkle Gassen, damit die Schrift in beiden Faellen lesbar bleibt.
 const FLinearColor Hintergrundfarbe(0.05f, 0.06f, 0.08f, 0.86f);
 const FLinearColor Schrift(0.94f, 0.93f, 0.90f, 1.0f);
 const FLinearColor Gedaempft(0.72f, 0.71f, 0.68f, 1.0f);

 FSlateFontInfo Schriftart(int32 Groesse) {
  return FCoreStyle::GetDefaultFontStyle("Regular", Groesse);
 }
 FSlateFontInfo Fett(int32 Groesse) {
  return FCoreStyle::GetDefaultFontStyle("Bold", Groesse);
 }
}

void SLaLaBergMenue::Construct(const FArguments& InArgs) {
 Steuerung = InArgs._Steuerung;
 ChildSlot[
  SNew(SBorder)
  .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
  .BorderBackgroundColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.55f))
  .HAlign(HAlign_Center).VAlign(VAlign_Center)
  [
   SNew(SBorder)
   .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
   .BorderBackgroundColor(Hintergrundfarbe)
   .Padding(FMargin(56, 40))
   [
    SAssignNew(Inhalt, SBox).WidthOverride(520)[ BaueSeite() ]
   ]
  ]
 ];
}

// Ein Knopf: gleiche Hoehe, gleicher Abstand, gleiche Schrift - damit die
// Liste ruhig wirkt und mit der Tastatur durchlaufen werden kann.
static TSharedRef<SWidget> Knopf(const FText& Beschriftung, FOnClicked BeiKlick, bool bHervor = false) {
 return SNew(SButton)
  .HAlign(HAlign_Center).VAlign(VAlign_Center)
  .ContentPadding(FMargin(18, 12))
  .OnClicked(BeiKlick)
  [
   SNew(STextBlock)
   .Text(Beschriftung)
   .Font(bHervor ? Fett(19) : Schriftart(17))
   .ColorAndOpacity(FSlateColor(bHervor ? Schrift : Gedaempft))
  ];
}

static TSharedRef<SWidget> Zeile(const FText& Titel, TAttribute<FText> Wert,
                                 FOnClicked Zurueck, FOnClicked Vor) {
 return SNew(SHorizontalBox)
  + SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
  [ SNew(STextBlock).Text(Titel).Font(Schriftart(16)).ColorAndOpacity(FSlateColor(Gedaempft)) ]
  + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
  [ SNew(SButton).ContentPadding(FMargin(12, 6)).OnClicked(Zurueck)
    [ SNew(STextBlock).Text(FText::FromString(TEXT("<"))).Font(Fett(16)) ] ]
  + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
  [ SNew(SBox).WidthOverride(160).HAlign(HAlign_Center)
    [ SNew(STextBlock).Text(Wert).Font(Schriftart(16)).ColorAndOpacity(FSlateColor(Schrift)) ] ]
  + SHorizontalBox::Slot().AutoWidth().Padding(6, 0)
  [ SNew(SButton).ContentPadding(FMargin(12, 6)).OnClicked(Vor)
    [ SNew(STextBlock).Text(FText::FromString(TEXT(">"))).Font(Fett(16)) ] ];
}

TSharedRef<SWidget> SLaLaBergMenue::BaueSeite() {
 const bool bStart = Steuerung.IsValid() && Steuerung->IstStartbild();
 if (Seite == ESeite::Haupt) {
  return SNew(SVerticalBox)
   + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
   [ SNew(STextBlock).Text(FText::FromString(TEXT("GTA LaLaBerg")))
     .Font(Fett(38)).ColorAndOpacity(FSlateColor(Schrift)) ]
   + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 4, 0, 26)
   [ SNew(STextBlock).Text(FText::FromString(TEXT("Landsberg am Lech, 1:1")))
     .Font(Schriftart(15)).ColorAndOpacity(FSlateColor(Gedaempft)) ]
   + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
   [ Knopf(FText::FromString(bStart ? TEXT("Spiel starten") : TEXT("Weiterspielen")),
           FOnClicked::CreateSP(this, &SLaLaBergMenue::Weiter), true) ]
   + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
   [ Knopf(FText::FromString(TEXT("Einstellungen")),
           FOnClicked::CreateSP(this, &SLaLaBergMenue::ZeigeEinstellungen)) ]
   + SVerticalBox::Slot().AutoHeight().Padding(0, 5)
   [ Knopf(FText::FromString(TEXT("Beenden")),
           FOnClicked::CreateSP(this, &SLaLaBergMenue::Beenden)) ]
   + SVerticalBox::Slot().AutoHeight().Padding(0, 26, 0, 2).HAlign(HAlign_Center)
   [ SNew(STextBlock)
     .Text(FText::FromString(TEXT("WASD gehen   ·   Maus umsehen   ·   Leertaste springen")))
     .Font(Schriftart(13)).ColorAndOpacity(FSlateColor(Gedaempft)) ]
   + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)
   [ SNew(STextBlock)
     .Text(FText::FromString(TEXT("R zurücksetzen   ·   Esc Menü")))
     .Font(Schriftart(13)).ColorAndOpacity(FSlateColor(Gedaempft)) ];
 }

 return SNew(SVerticalBox)
  + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(0, 0, 0, 24)
  [ SNew(STextBlock).Text(FText::FromString(TEXT("Einstellungen")))
    .Font(Fett(28)).ColorAndOpacity(FSlateColor(Schrift)) ]
  + SVerticalBox::Slot().AutoHeight().Padding(0, 7)
  [ Zeile(FText::FromString(TEXT("Auflösung")),
          TAttribute<FText>::Create([this]() { return AufloesungsText(); }),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::AufloesungAendern, -1),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::AufloesungAendern, 1)) ]
  + SVerticalBox::Slot().AutoHeight().Padding(0, 7)
  [ Zeile(FText::FromString(TEXT("Anzeige")),
          TAttribute<FText>::Create([this]() { return FenstermodusText(); }),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::FenstermodusWechseln),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::FenstermodusWechseln)) ]
  + SVerticalBox::Slot().AutoHeight().Padding(0, 7)
  [ Zeile(FText::FromString(TEXT("Grafikqualität")),
          TAttribute<FText>::Create([this]() { return StufenText(); }),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::StufeAendern, -1),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::StufeAendern, 1)) ]
  + SVerticalBox::Slot().AutoHeight().Padding(0, 7)
  [ Zeile(FText::FromString(TEXT("Lautstärke")),
          TAttribute<FText>::Create([this]() { return LautstaerkeText(); }),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::LautstaerkeAendern, -1),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::LautstaerkeAendern, 1)) ]
  + SVerticalBox::Slot().AutoHeight().Padding(0, 26, 0, 0)
  [ Knopf(FText::FromString(TEXT("Zurück")),
          FOnClicked::CreateSP(this, &SLaLaBergMenue::ZurueckZumMenue), true) ];
}

// Escape fuehrt zurueck: aus den Einstellungen ins Menue, aus dem Menue ins
// Spiel. Im Startbild bleibt es stehen - dort gibt es kein "zurueck".
FReply SLaLaBergMenue::OnKeyDown(const FGeometry& Geometrie, const FKeyEvent& Taste) {
 if (Taste.GetKey() != EKeys::Escape) return FReply::Unhandled();
 if (Seite == ESeite::Einstellungen) return ZurueckZumMenue();
 if (Steuerung.IsValid() && Steuerung->IstStartbild()) return FReply::Handled();
 return Weiter();
}

FReply SLaLaBergMenue::Weiter() {
 if (Steuerung.IsValid()) Steuerung->SchliesseMenue();
 return FReply::Handled();
}

FReply SLaLaBergMenue::Beenden() {
 FGenericPlatformMisc::RequestExit(false);
 return FReply::Handled();
}

FReply SLaLaBergMenue::ZeigeEinstellungen() {
 Seite = ESeite::Einstellungen;
 if (Inhalt.IsValid()) Inhalt->SetContent(BaueSeite());
 return FReply::Handled();
}

FReply SLaLaBergMenue::ZurueckZumMenue() {
 Seite = ESeite::Haupt;
 if (Inhalt.IsValid()) Inhalt->SetContent(BaueSeite());
 return FReply::Handled();
}

FReply SLaLaBergMenue::StufeAendern(int32 Richtung) {
 if (Steuerung.IsValid()) Steuerung->SetzeStufe(FMath::Clamp(Steuerung->HoleStufe() + Richtung, 0, 3));
 return FReply::Handled();
}

FReply SLaLaBergMenue::AufloesungAendern(int32 Richtung) {
 if (Steuerung.IsValid()) {
  const int32 Anzahl = Steuerung->AnzahlAufloesungen();
  Steuerung->SetzeAufloesungsIndex(FMath::Clamp(Steuerung->HoleAufloesungsIndex() + Richtung, 0, Anzahl - 1));
 }
 return FReply::Handled();
}

FReply SLaLaBergMenue::FenstermodusWechseln() {
 if (Steuerung.IsValid()) Steuerung->SetzeVollbild(!Steuerung->IstVollbild());
 return FReply::Handled();
}

FReply SLaLaBergMenue::LautstaerkeAendern(int32 Richtung) {
 if (Steuerung.IsValid()) Steuerung->SetzeLautstaerke(Steuerung->HoleLautstaerke() + Richtung * 0.1f);
 return FReply::Handled();
}

FText SLaLaBergMenue::StufenText() const {
 static const TCHAR* Namen[] = { TEXT("Niedrig"), TEXT("Mittel"), TEXT("Hoch"), TEXT("Episch") };
 const int32 Stufe = Steuerung.IsValid() ? Steuerung->HoleStufe() : 2;
 return FText::FromString(Namen[FMath::Clamp(Stufe, 0, 3)]);
}

FText SLaLaBergMenue::AufloesungsText() const {
 if (!Steuerung.IsValid()) return FText::GetEmpty();
 const FIntPoint A = Steuerung->HoleAufloesung(Steuerung->HoleAufloesungsIndex());
 return FText::FromString(FString::Printf(TEXT("%d x %d"), A.X, A.Y));
}

FText SLaLaBergMenue::FenstermodusText() const {
 const bool bVoll = Steuerung.IsValid() && Steuerung->IstVollbild();
 return FText::FromString(bVoll ? TEXT("Vollbild") : TEXT("Fenster"));
}

FText SLaLaBergMenue::LautstaerkeText() const {
 const float L = Steuerung.IsValid() ? Steuerung->HoleLautstaerke() : 0.8f;
 return FText::FromString(FString::Printf(TEXT("%d %%"), FMath::RoundToInt(L * 100)));
}
