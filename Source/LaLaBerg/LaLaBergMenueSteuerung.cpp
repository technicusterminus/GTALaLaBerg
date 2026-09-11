#include "LaLaBergMenueSteuerung.h"
#include "LaLaBergMenue.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/SWeakWidget.h"
#include "Sound/SoundClass.h"

void ULaLaBergMenueSteuerung::Initialize(FSubsystemCollectionBase& Sammlung) {
 Super::Initialize(Sammlung);
 if (auto* Einstellungen = UGameUserSettings::GetGameUserSettings()) {
  const FIntPoint Jetzt = Einstellungen->GetScreenResolution();
  for (int32 i = 0; i < AnzahlAufloesungen(); i++) {
   if (HoleAufloesung(i) == Jetzt) { AufloesungsIndex = i; break; }
  }
  if (AufloesungsIndex < 0) AufloesungsIndex = AnzahlAufloesungen() - 1;
 }
}

void ULaLaBergMenueSteuerung::Deinitialize() {
 SchliesseMenue();
 Super::Deinitialize();
}

void ULaLaBergMenueSteuerung::SetzeEingabe(bool bMenue) {
 UWorld* Welt = GetWorld();
 APlayerController* PC = Welt ? Welt->GetFirstPlayerController() : nullptr;
 if (!PC) return;
 if (bMenue) {
  FInputModeUIOnly Modus;
  Modus.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
  PC->SetInputMode(Modus);
  PC->bShowMouseCursor = true;
 } else {
  PC->SetInputMode(FInputModeGameOnly());
  PC->bShowMouseCursor = false;
 }
 // Die Welt laeuft weiter, aber die Figur haelt an: ein Pausenbild, in dem
 // die Stadt weiterlebt, wirkt lebendiger als ein eingefrorenes Bild.
 UGameplayStatics::SetGamePaused(Welt, bMenue);
}

void ULaLaBergMenueSteuerung::ZeigeMenue(bool bImStartbild) {
 if (Overlay.IsValid()) return;
 UWorld* Welt = GetWorld();
 if (!Welt || !Welt->GetGameViewport()) return;
 bStartbild = bImStartbild;
 TSharedRef<SLaLaBergMenue> Menue = SNew(SLaLaBergMenue).Steuerung(this);
 Overlay = Menue;
 Welt->GetGameViewport()->AddViewportWidgetContent(
  SNew(SWeakWidget).PossiblyNullContent(Menue), 100);
 SetzeEingabe(true);
 FSlateApplication::Get().SetKeyboardFocus(Menue);
}

void ULaLaBergMenueSteuerung::SchliesseMenue() {
 if (!Overlay.IsValid()) return;
 UWorld* Welt = GetWorld();
 if (Welt && Welt->GetGameViewport()) {
  Welt->GetGameViewport()->RemoveViewportWidgetContent(Overlay.ToSharedRef());
 }
 Overlay.Reset();
 bStartbild = false;
 SetzeEingabe(false);
}

void ULaLaBergMenueSteuerung::Umschalten() {
 if (Overlay.IsValid()) SchliesseMenue(); else ZeigeMenue(false);
}

void ULaLaBergMenueSteuerung::SetzeStufe(int32 Stufe) {
 auto* E = UGameUserSettings::GetGameUserSettings();
 if (!E) return;
 Stufe = FMath::Clamp(Stufe, 0, 3);
 E->SetOverallScalabilityLevel(Stufe);
 E->ApplySettings(false);
}

int32 ULaLaBergMenueSteuerung::HoleStufe() const {
 auto* E = UGameUserSettings::GetGameUserSettings();
 return E ? FMath::Clamp(E->GetOverallScalabilityLevel(), 0, 3) : 2;
}

// Eine kurze, feste Liste - Auflösungen, die auf jedem Bildschirm sinnvoll
// sind. Eine Abfrage der Anzeigemodi liefert Dutzende Eintraege, durch die
// sich niemand klicken will.
static const FIntPoint GAufloesungen[] = {
 FIntPoint(1280, 720), FIntPoint(1600, 900), FIntPoint(1920, 1080),
 FIntPoint(2560, 1440), FIntPoint(3840, 2160)
};

int32 ULaLaBergMenueSteuerung::AnzahlAufloesungen() const { return UE_ARRAY_COUNT(GAufloesungen); }
FIntPoint ULaLaBergMenueSteuerung::HoleAufloesung(int32 Index) const {
 return GAufloesungen[FMath::Clamp(Index, 0, AnzahlAufloesungen() - 1)];
}

void ULaLaBergMenueSteuerung::SetzeAufloesungsIndex(int32 Index) {
 auto* E = UGameUserSettings::GetGameUserSettings();
 if (!E) return;
 AufloesungsIndex = FMath::Clamp(Index, 0, AnzahlAufloesungen() - 1);
 E->SetScreenResolution(HoleAufloesung(AufloesungsIndex));
 E->ApplyResolutionSettings(false);
}

void ULaLaBergMenueSteuerung::SetzeVollbild(bool bVollbild) {
 auto* E = UGameUserSettings::GetGameUserSettings();
 if (!E) return;
 E->SetFullscreenMode(bVollbild ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
 E->ApplyResolutionSettings(false);
}

bool ULaLaBergMenueSteuerung::IstVollbild() const {
 auto* E = UGameUserSettings::GetGameUserSettings();
 return E && E->GetFullscreenMode() != EWindowMode::Windowed;
}

void ULaLaBergMenueSteuerung::SetzeLautstaerke(float Wert) {
 Lautstaerke = FMath::Clamp(Wert, 0.0f, 1.0f);
 if (GEngine) GEngine->Exec(GetWorld(), *FString::Printf(TEXT("au.MasterVolume %.2f"), Lautstaerke));
}
