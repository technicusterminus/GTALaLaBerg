#pragma once
#include "CoreMinimal.h"

// Vorwaertsdeklaration im globalen Namensraum - innerhalb des Namensraums
// unten wuerde "class UProceduralMeshComponent" sonst einen eigenen,
// zweiten Typ LaLaBergWagenForm::UProceduralMeshComponent erzeugen statt
// auf den echten (globalen) Engine-Typ zu verweisen.
class UProceduralMeshComponent;

// Die Autoform aus wagen.json (siehe Tools/Export/wagen-form.cjs), geteilt
// zwischen dem fahrbaren Wagen und den KI-Verkehrswagen - beide sollen
// gleich aussehen, nur einer davon hat eine Federung.
namespace LaLaBergWagenForm {
 // Baut das Netz aus der Vorlage in "Netz", lackiert in "Lack". Liefert
 // false, wenn die Vorlagedatei fehlt - dann bleibt das Netz leer und der
 // Aufrufer weicht auf eine einfache Form aus.
 bool BaueNetz(UProceduralMeshComponent* Netz, const FLinearColor& Lack);
}
