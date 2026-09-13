#pragma once
#include "CoreMinimal.h"

// Vorwaertsdeklaration im globalen Namensraum - innerhalb des Namensraums
// unten wuerde "class UProceduralMeshComponent" sonst einen eigenen,
// zweiten Typ LaLaBergWagenForm::UProceduralMeshComponent erzeugen statt
// auf den echten (globalen) Engine-Typ zu verweisen.
class UProceduralMeshComponent;
class UStaticMeshComponent;
class USceneComponent;
class UStaticMesh;

// Die Autoform aus wagen.json (siehe Tools/Export/wagen-form.cjs), geteilt
// zwischen dem fahrbaren Wagen und den KI-Verkehrswagen - beide sollen
// gleich aussehen, nur einer davon hat eine Federung.
namespace LaLaBergWagenForm {
 // Baut das Netz aus der Vorlage in "Netz", lackiert in "Lack". Liefert
 // false, wenn die Vorlagedatei fehlt - dann bleibt das Netz leer und der
 // Aufrufer weicht auf eine einfache Form aus.
 bool BaueNetz(UProceduralMeshComponent* Netz, const FLinearColor& Lack);

 // Dieselbe Form wie BaueNetz, aber als eigenstaendiges (transientes)
 // UStaticMesh statt einer ProceduralMeshComponent-Sektion je Auto - fuer
 // ALaLaBergKastenPool: ein Draw-Call je Lackfarbe statt einer je Auto (siehe
 // dort fuer den Grund). Nur im Editor-Build verfuegbar wie der Sektor-Import
 // (UStaticMesh::BuildFromMeshDescriptions) - liefert sonst nullptr, dann
 // bleibt der Aufrufer bei BaueNetz.
 UStaticMesh* BaueKastenMesh(const FLinearColor& Lack);

 // Haengt die sichtbaren Aussenteile des lizenzierten CarConcept-Fahrzeugs
 // (CC BY 4.0, siehe Content/SourceData/Vehicles/CarConcept-LICENSE.md) an
 // "Traeger" - dessen lokaler Ursprung liegt auf Fahrbahnhoehe, wie bei
 // BaueNetz. Innenraum/Motor/Pedale bleiben aussen vor: ohne Innenraum-
 // Kamera in diesem Projekt waeren sie nie im Bild, kosten aber Draw-Calls.
 // Liefert false (kein Teil angelegt), wenn die Assets fehlen - dann bleibt
 // es bei der ProceduralMesh-Form aus BaueNetz. Fuer den einen fahrbaren
 // Wagen gedacht - fuer die vielen KI-Autos siehe ALaLaBergAutoPool
 // (Instanced Static Mesh statt einer eigenen Komponente je Wagen und Teil).
 bool BaueCarConceptTeile(USceneComponent* Traeger, TArray<TObjectPtr<UStaticMeshComponent>>& Teile);

 // Dieselbe Teileliste, offen fuer ALaLaBergAutoPool - siehe BaueCarConceptTeile
 // fuer die Begruendung jedes einzelnen Teils.
 extern const TCHAR* const CARCONCEPT_TEILE[];
 extern const int32 CARCONCEPT_TEILE_ANZAHL;
 FString CarConceptPfad(const TCHAR* Teilname);
}
