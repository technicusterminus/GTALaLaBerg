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

void ULaLaBergKonto::ZaehleAuftrag() {
 Stand->Erledigt++;
 Speichere();
}
