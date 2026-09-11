#pragma once
#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

// Haupt-, Pause- und Einstellungsmenue. Bewusst als Slate im C++ gebaut: das
// Projekt hat keine Blueprint-Oberflaechen, und so bleibt die Bedienung
// zusammen mit dem Rest des Spiels versionierbar.
class SLaLaBergMenue : public SCompoundWidget {
public:
 SLATE_BEGIN_ARGS(SLaLaBergMenue) {}
  SLATE_ARGUMENT(TWeakObjectPtr<class ULaLaBergMenueSteuerung>, Steuerung)
 SLATE_END_ARGS()

 void Construct(const FArguments& InArgs);
 virtual bool SupportsKeyboardFocus() const override { return true; }
 virtual FReply OnKeyDown(const FGeometry& Geometrie, const FKeyEvent& Taste) override;

private:
 enum class ESeite : uint8 { Haupt, Einstellungen };
 TSharedRef<class SWidget> BaueSeite();
 FReply Weiter();
 FReply Beenden();
 FReply ZeigeEinstellungen();
 FReply ZurueckZumMenue();
 FReply StufeAendern(int32 Richtung);
 FReply AufloesungAendern(int32 Richtung);
 FReply FenstermodusWechseln();
 FReply LautstaerkeAendern(int32 Richtung);
 FText StufenText() const;
 FText AufloesungsText() const;
 FText FenstermodusText() const;
 FText LautstaerkeText() const;

 TWeakObjectPtr<class ULaLaBergMenueSteuerung> Steuerung;
 TSharedPtr<class SBox> Inhalt;
 ESeite Seite = ESeite::Haupt;
};
