#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LaLaBergDrehbuch.generated.h"

// Das Drehbuch: die Geschichte als Daten statt als Code. Jede Mission steht
// in Content/SourceData/Story/missionen.json, gehoert zu einem Kapitel,
// beginnt an einem Ort aus orte.json und besteht aus Stufen. Neue Missionen
// brauchen keine Zeile C++ - nur einen Eintrag in der Datei.
//
// Stufenarten:
//   hin      - den Ort erreichen (wahlweise nur mit dem Wagen)
//   markiere - die Saeule an diesem Ort mit Farbe treffen
//   stelle   - den gekennzeichneten Wagen ausser Gefecht setzen
//   bring    - mit dem Wagen zum Ort (wie "hin", aber Wagen zwingend)
//   halte    - so lange durchhalten, wahlweise mit Fahndungsstufe
UENUM()
enum class ELaLaBergStufe : uint8 { Hin, Markiere, Stelle, Bring, Halte };

UCLASS()
class LALABERG_API ALaLaBergDrehbuch : public AActor {
 GENERATED_BODY()
public:
 ALaLaBergDrehbuch();
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type Grund) override;
 virtual void Tick(float Zeit) override;

 static TWeakObjectPtr<ALaLaBergDrehbuch> Instanz;

 struct FStufe {
  ELaLaBergStufe Art = ELaLaBergStufe::Hin;
  FString OrtName;
  FVector Ort = FVector::ZeroVector;
  FString Text;
  float Zeit = 0.0f;        // 0 = ohne Frist
  int32 Sterne = 0;         // nur "halte": Fahndung, die dabei anliegt
  bool bImWagen = false;
 };
 struct FMission {
  FString Id, Name, StartName;
  FVector Start = FVector::ZeroVector;
  int32 Kapitel = 1, Lohn = 0, Ruf = 0;
  // Reihenfolge innerhalb des Kapitels. Nicht die Stelle im Feld: die ist
  // die Kennung im Spielstand (ULaLaBergKonto::HatMission) und darf sich
  // nie verschieben, sonst gelten in einem alten Spielstand ploetzlich
  // andere Missionen als geschafft. Neue Missionen werden deshalb hinten
  // angehaengt und ueber "reihe" an ihren Platz gestellt.
  int32 Reihe = 0;
  TArray<FStufe> Stufen;
 };

 // Fuer das HUD.
 bool Laeuft() const { return AktMission != INDEX_NONE; }
 FString HoleMissionsname() const;
 FString HoleStufentext() const;
 FVector HoleStufenort() const;
 float HoleRestzeit() const;
 int32 HoleStufe() const { return AktStufe + 1; }
 int32 HoleStufen() const;
 // Die naechste offene Mission des laufenden Kapitels, sonst INDEX_NONE.
 int32 HoleAngebot() const { return Angebot; }
 FString HoleAngebotsname() const;
 FVector HoleAngebotsort() const;

 // Fuer -LaLaBergStoryTest.
 bool TestStarte();
 void TestStufeGeschafft();
 // Fuer -LaLaBergStoryTest: wie viele Missionen geladen sind und wie viele
 // Ortsnamen darin nicht in orte.json stehen (siehe .cpp).
 int32 HoleMissionszahl() const { return Missionen.Num(); }
 int32 HoleFehlendeOrte() const { return FehlendeOrte; }
 int32 HoleReihe(int32 Nummer) const { return Missionen.IsValidIndex(Nummer) ? Missionen[Nummer].Reihe : -1; }
 int32 HoleKapitelVon(int32 Nummer) const { return Missionen.IsValidIndex(Nummer) ? Missionen[Nummer].Kapitel : -1; }
 int32 HoleGeschafft() const { return Geschafft; }

private:
 void LadeDrehbuch();
 void SucheAngebot();
 void Starte(int32 Mission);
 void NaechsteStufe();
 void Vollende();
 void Scheitere(const FString& Grund);
 void Melde(const FString& Text) const;
 bool StufeGeschafft(const FStufe& Stufe, APawn* Figur);
 // Fuer "stelle": einen Wagen kennzeichnen, der gestellt werden soll.
 bool SucheGegner();

 TArray<FMission> Missionen;

 // Ortsnamen aus missionen.json, die orte.json nicht kennt.

 int32 FehlendeOrte = 0;
 int32 AktMission = INDEX_NONE, AktStufe = 0, Angebot = INDEX_NONE, Geschafft = 0;
 double Frist = 0.0;
 double HalteBis = 0.0;
 TWeakObjectPtr<class ALaLaBergVerkehrsauto> Gegner;

 // Ring und Saeule am Startort der angebotenen Mission - gruen, damit man
 // sie von der blauen Auftragssaeule unterscheidet.
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StartRing = nullptr;
 UPROPERTY() TObjectPtr<class UStaticMeshComponent> StartSaeule = nullptr;
 bool bGeladen = false;
};
