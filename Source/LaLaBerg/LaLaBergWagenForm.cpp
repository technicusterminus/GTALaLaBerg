#include "LaLaBergWagenForm.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool LaLaBergWagenForm::BaueNetz(UProceduralMeshComponent* Netz, const FLinearColor& Lack) {
 static TSharedPtr<FJsonObject> Vorlage;
 static bool bVersucht = false;
 if (!bVersucht) {
  bVersucht = true;
  FString Text;
  const FString Datei = FPaths::ProjectContentDir() / TEXT("SourceData/Fahrzeug/wagen.json");
  if (!FFileHelper::LoadFileToString(Text, *Datei) ||
      !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Vorlage) || !Vorlage.IsValid()) {
   Vorlage.Reset();
   UE_LOG(LogTemp, Warning, TEXT("LALABERG_WAGENFORM fehlt: %s - einfache Form"), *Datei);
  }
 }
 if (!Vorlage.IsValid() || !Netz) return false;
 auto Material = [](const FString& Klasse) -> UMaterialInterface* {
  const TCHAR* Pfad = TEXT("/Game/Art/Materials/M_Lack.M_Lack");
  if (Klasse == TEXT("Glas")) Pfad = TEXT("/Game/Art/Materials/M_Glas.M_Glas");
  else if (Klasse == TEXT("Reifen")) Pfad = TEXT("/Game/Art/Materials/M_Asphalt.M_Asphalt");
  else if (Klasse == TEXT("Stone")) Pfad = TEXT("/Game/Art/Materials/M_Stein.M_Stein");
  return LoadObject<UMaterialInterface>(nullptr, Pfad);
 };
 Netz->ClearAllMeshSections();
 int32 Nr = 0, Dreiecke = 0;
 for (const auto& Wert : Vorlage->GetArrayField(TEXT("teile"))) {
  const TSharedPtr<FJsonObject> Teil = Wert->AsObject();
  const auto& Zahlen = Teil->GetArrayField(TEXT("positions"));
  const auto& Ecken = Teil->GetArrayField(TEXT("indices"));
  const auto& Rgb = Teil->GetArrayField(TEXT("color"));
  const FLinearColor Farbe = Teil->GetBoolField(TEXT("lack")) ? Lack
   : FLinearColor(Rgb[0]->AsNumber(), Rgb[1]->AsNumber(), Rgb[2]->AsNumber());
  TArray<FVector> Punkte; TArray<int32> Kanten; TArray<FVector> Normalen;
  TArray<FVector2D> UVs; TArray<FLinearColor> Farben; TArray<FProcMeshTangent> Tangenten;
  for (int32 i = 0; i + 2 < Zahlen.Num(); i += 3)
   Punkte.Add(FVector(Zahlen[i]->AsNumber(), Zahlen[i + 1]->AsNumber(), Zahlen[i + 2]->AsNumber()));
  for (const auto& E : Ecken) Kanten.Add(static_cast<int32>(E->AsNumber()));
  // Jede Flaeche hat eigene Eckpunkte - die Flaechennormale genuegt, und die
  // Kanten zwischen Scheibe und Lack bleiben scharf.
  Normalen.Init(FVector::ZeroVector, Punkte.Num());
  for (int32 i = 0; i + 2 < Kanten.Num(); i += 3) {
   const FVector N = FVector::CrossProduct(Punkte[Kanten[i + 2]] - Punkte[Kanten[i]], Punkte[Kanten[i + 1]] - Punkte[Kanten[i]]);
   Normalen[Kanten[i]] += N; Normalen[Kanten[i + 1]] += N; Normalen[Kanten[i + 2]] += N;
  }
  for (FVector& N : Normalen) N = N.GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
  for (const FVector& P : Punkte) { UVs.Add(FVector2D(P.X / 200.0, P.Y / 200.0)); Farben.Add(Farbe); }
  Netz->CreateMeshSection_LinearColor(Nr, Punkte, Kanten, Normalen, UVs, Farben, Tangenten, false);
  if (UMaterialInterface* M = Material(Teil->GetStringField(TEXT("klasse")))) Netz->SetMaterial(Nr, M);
  Dreiecke += Kanten.Num() / 3;
  Nr++;
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_WAGENFORM teile=%d dreiecke=%d"), Nr, Dreiecke);
 return Nr > 0;
}
