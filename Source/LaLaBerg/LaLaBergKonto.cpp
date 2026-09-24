#include "LaLaBergKonto.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"

void ULaLaBergKonto::Initialize(FSubsystemCollectionBase& Sammlung) {
 Super::Initialize(Sammlung);
 const bool bPruefung = FString(FCommandLine::Get()).Contains(TEXT("-LaLaBerg"));
 Slot = bPruefung ? TEXT("LaLaBergTest") : TEXT("LaLaBerg");
 if (!bPruefung) Stand = Cast<ULaLaBergSpielstand>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
 if (!Stand) Stand = Cast<ULaLaBergSpielstand>(UGameplayStatics::CreateSaveGameObject(ULaLaBergSpielstand::StaticClass()));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KONTO slot=%s geld=%d waffen=%d lack=%d"), *Slot, Stand->Geld, Stand->Waffen, Stand->bHatLack ? 1 : 0);
}

void ULaLaBergKonto::Deinitialize() {
 Speichere();
 Super::Deinitialize();
}

ULaLaBergKonto* ULaLaBergKonto::Hole(const UObject* Kontext) {
 const UWorld* Welt = Kontext ? Kontext->GetWorld() : nullptr;
 const UGameInstance* Spiel = Welt ? Welt->GetGameInstance() : nullptr;
 return Spiel ? Spiel->GetSubsystem<ULaLaBergKonto>() : nullptr;
}

void ULaLaBergKonto::Speichere() {
 if (Stand) UGameplayStatics::SaveGameToSlot(Stand, Slot, 0);
}

ULaLaBergSpielstand* ULaLaBergKonto::LadeVonPlatte() const {
 return Cast<ULaLaBergSpielstand>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
}

void ULaLaBergKonto::Gutschrift(int32 Betrag) {
 Stand->Geld += FMath::Max(0, Betrag);
 Speichere();
}

bool ULaLaBergKonto::Bezahle(int32 Betrag) {
 if (Betrag > Stand->Geld) return false;
 Stand->Geld -= Betrag;
 Speichere();
 return true;
}

int32 ULaLaBergKonto::Strafe(int32 Betrag) {
 const int32 Ab = FMath::Clamp(Betrag, 0, Stand->Geld);
 Stand->Geld -= Ab;
 Speichere();
 return Ab;
}

void ULaLaBergKonto::GibWaffe(uint8 Art) {
 Stand->Waffen |= 1 << Art;
 Speichere();
}

bool ULaLaBergKonto::HatLack(FLinearColor& Aus) const {
 if (!Stand || !Stand->bHatLack) return false;
 Aus = Stand->Lack;
 return true;
}

void ULaLaBergKonto::SetzeLack(const FLinearColor& Farbe) {
 Stand->bHatLack = true;
 Stand->Lack = Farbe;
 Speichere();
}

void ULaLaBergKonto::Uebe(EWert Wert, float Punkte) {
 if (!Stand || Punkte <= 0.0f) return;
 int32* Ziel = Wert == EWert::Ausdauer ? &Stand->Ausdauer
             : Wert == EWert::Zielsicherheit ? &Stand->Zielsicherheit
             : Wert == EWert::Fahren ? &Stand->Fahren
                                     : &Stand->Ruf;
 const int32 Vorher = *Ziel;
 *Ziel = FMath::Clamp(*Ziel + FMath::RoundToInt(Punkte), 0, 1000);
 // Nur beim Stufenwechsel speichern und melden - sonst schriebe jeder
 // Schritt auf die Platte.
 if (Vorher / 200 != *Ziel / 200) {
  UE_LOG(LogTemp, Display, TEXT("LALABERG_WERT %d stufe=%d"), static_cast<int32>(Wert), *Ziel / 200 + 1);
  Speichere();
 }
}

int32 ULaLaBergKonto::HoleWert(EWert Wert) const {
 if (!Stand) return 0;
 switch (Wert) {
  case EWert::Ausdauer: return Stand->Ausdauer;
  case EWert::Zielsicherheit: return Stand->Zielsicherheit;
  case EWert::Fahren: return Stand->Fahren;
  case EWert::Ruf: return Stand->Ruf;
 }
 return 0;
}

void ULaLaBergKonto::NimmRevier(int32 Nummer) {
 if (!Stand || HatRevier(Nummer)) return;
 Stand->Reviere |= (1 << Nummer);
 UE_LOG(LogTemp, Display, TEXT("LALABERG_REVIER genommen=%d alle=%d"), Nummer, Stand->Reviere);
 Speichere();
}

void ULaLaBergKonto::ZaehleArt(int32 Art) {
 if (!Stand) return;
 switch (Art) {
  case 0: Stand->Lieferungen++; break;
  case 1: Stand->Taxifahrten++; break;
  case 2: Stand->Rennen++; break;
  case 3: Stand->Verfolgungen++; break;
  default: return;
 }
 Speichere();
}

int32 ULaLaBergKonto::HoleArtZahl(int32 Art) const {
 if (!Stand) return 0;
 switch (Art) {
  case 0: return Stand->Lieferungen;
  case 1: return Stand->Taxifahrten;
  case 2: return Stand->Rennen;
  case 3: return Stand->Verfolgungen;
 }
 return 0;
}

bool ULaLaBergKonto::HatDose(int32 Nummer) const {
 if (!Stand || Nummer < 0 || Nummer >= 50) return false;
 return Nummer < 25 ? (Stand->DosenA & (1 << Nummer)) != 0 : (Stand->DosenB & (1 << (Nummer - 25))) != 0;
}

void ULaLaBergKonto::NimmDose(int32 Nummer) {
 if (!Stand || Nummer < 0 || Nummer >= 50 || HatDose(Nummer)) return;
 if (Nummer < 25) Stand->DosenA |= (1 << Nummer);
 else Stand->DosenB |= (1 << (Nummer - 25));
 Speichere();
}

int32 ULaLaBergKonto::HoleDosen() const {
 if (!Stand) return 0;
 return FMath::CountBits(static_cast<uint32>(Stand->DosenA)) + FMath::CountBits(static_cast<uint32>(Stand->DosenB));
}

void ULaLaBergKonto::SetzeBesteBude(int32 Treffer) {
 if (!Stand || Treffer <= Stand->BesteBude) return;
 Stand->BesteBude = Treffer;
 Speichere();
}

void ULaLaBergKonto::SetzeMission(int32 Nummer) {
 if (!Stand || Nummer < 0 || Nummer >= 32 || HatMission(Nummer)) return;
 Stand->Missionen |= (1 << Nummer);
 Speichere();
}

void ULaLaBergKonto::GibFund(EFund Fund) {
 if (!Stand || HatFund(Fund)) return;
 Stand->Funde |= (1 << static_cast<int32>(Fund));
 UE_LOG(LogTemp, Display, TEXT("LALABERG_FUND %d alle=%d"), static_cast<int32>(Fund), Stand->Funde);
 Speichere();
}

void ULaLaBergKonto::SetzeKapitel(int32 Neu) {
 if (!Stand || Neu <= Stand->Kapitel) return;
 Stand->Kapitel = Neu;
 UE_LOG(LogTemp, Display, TEXT("LALABERG_KAPITEL %d"), Neu);
 Speichere();
}

void ULaLaBergKonto::ZaehleAuftrag() {
 Stand->Erledigt++;
 Speichere();
}
