#pragma once
#include "CoreMinimal.h"

class UStaticMesh;
class UStaticMeshComponent;
class USceneComponent;

// Realistische Fahrzeugtypen aus dem kostenlosen "City Sample Vehicles"-Paket
// (Epic Games, Fab "Free For Life", siehe README "Datenquellen und Lizenzen")
// unter Content/CitySampleVehicles - Vielfalt zusaetzlich zum bisherigen
// CarConcept-Modell (LaLaBergWagenForm), nicht dessen Ersatz.
//
// Jeder Typ liefert dieselben zwei Ebenen wie CarConcept: einzelne Detail-
// Teile (Karosserie ohne Raeder + 4 Raeder, siehe BaueTeile) und ein bereits
// zusammengefasstes Fern-Mesh (LadeLod - fehlt es wie bei vehicle07_Car,
// liefert LadeLod nullptr und der Aufrufer faellt auf die Karosserie allein
// zurueck). vehicle08_Trailer bleibt aussen vor - ein Anhaenger ohne Zugwagen
// ist kein eigenstaendiges Auto.
namespace LaLaBergWagenTypen {
 struct FTyp {
  const TCHAR* Ordner;   // z.B. "vehicle02_Car"
  const TCHAR* Praefix;  // z.B. "vehCar_vehicle02" - Teil jedes Dateinamens
 };
 extern const FTyp TYPEN[];
 extern const int32 TYPEN_ANZAHL;

 FString ContentPfad(const FTyp& Typ, const FString& Dateiname);

 // Karosserie ohne Raeder + Rad vorne links/rechts, hinten links/rechts, in
 // dieser Reihenfolge - siehe TEIL_ANZAHL. Liefert false, wenn nicht einmal
 // die Karosserie existiert (dann ist der Typ nicht nutzbar).
 static constexpr int32 TEIL_ANZAHL = 5;
 bool BaueTeile(USceneComponent* Traeger, const FTyp& Typ, TArray<TObjectPtr<UStaticMeshComponent>>& Teile);

 // Fuer ALaLaBergAutoPool: dieselbe Teileliste als Pfade statt als
 // Komponenten, in derselben Reihenfolge wie BaueTeile.
 void TeilPfade(const FTyp& Typ, TArray<FString>& Pfade);

 // Einzelnes, bereits zusammengefasstes Mesh fuer die Fernansicht - nullptr,
 // wenn der Typ keins mitbringt.
 UStaticMesh* LadeLod(const FTyp& Typ);
}
