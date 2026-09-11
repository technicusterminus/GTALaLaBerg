#pragma once
#include "CoreMinimal.h"

// Eine Route abfahren, vorwaerts bis zum letzten Wegpunkt, dann rueckwaerts
// wieder zurueck - ein Autoverkehr oder ein Fussweg, ohne dass am Ende
// sichtbar zu einem neuen Startpunkt gesprungen wird. Geteilt zwischen
// Verkehrswagen und Passanten, die sich sonst nur in Tempo und Aussehen
// unterscheiden.
struct FLaLaBergWegfolger {
 TArray<FVector> Route;
 int32 Index = 0;
 int32 Richtung = 1;      // +1 vorwaerts durch die Route, -1 zurueck

 bool Gueltig() const { return Route.Num() >= 2; }
 FVector Start() const { return Gueltig() ? Route[0] : FVector::ZeroVector; }

 // Bewegt Ort bis zu "Strecke" Zentimeter entlang der Route. Liefert die
 // Blickrichtung (letzte Bewegungsrichtung, waagerecht) oder den Nullvektor,
 // wenn sich in diesem Bild nichts bewegt hat (z.B. an einer roten Ampel) -
 // der Aufrufer soll dann die bisherige Ausrichtung beibehalten, statt
 // gegen einen willkuerlichen Standardwert zu drehen. Ohne gueltige Route
 // bleibt Ort unveraendert.
 FVector Bewege(FVector& Ort, float Strecke) {
  if (!Gueltig()) return FVector::ZeroVector;
  FVector LetzteRichtung = FVector::ZeroVector;
  // Mehrere Wegpunkte je Bild sind moeglich (kurze Segmente, hohes Tempo) -
  // eine feste Obergrenze verhindert eine Endlosschleife bei einer Route
  // aus zwei identischen Punkten.
  for (int32 Versuch = 0; Versuch < 8 && Strecke > 0.01f; Versuch++) {
   Index = FMath::Clamp(Index, 0, Route.Num() - 1);
   const FVector Ziel = Route[Index];
   FVector Delta = Ziel - Ort;
   const float Abstand = Delta.Size();
   if (Abstand < 1.0f) {
    // Wegpunkt erreicht: naechsten ansteuern, am Ende umkehren.
    if (Index + Richtung < 0 || Index + Richtung >= Route.Num()) Richtung = -Richtung;
    else Index += Richtung;
    continue;
   }
   LetzteRichtung = Delta.GetSafeNormal();
   if (Abstand <= Strecke) { Ort = Ziel; Strecke -= Abstand; }
   else { Ort += LetzteRichtung * Strecke; Strecke = 0.0f; }
  }
  return LetzteRichtung;
 }
};
