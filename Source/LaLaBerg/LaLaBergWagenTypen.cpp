#include "LaLaBergWagenTypen.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

// vehicle08_Trailer fehlt hier absichtlich (siehe Header). vehicle07_Car hat
// kein "_LOD"-Mesh - LadeLod liefert dafuer nullptr, siehe Aufrufer.
const LaLaBergWagenTypen::FTyp LaLaBergWagenTypen::TYPEN[] = {
 { TEXT("vehicle01_Van"),   TEXT("vehVan_vehicle01") },
 { TEXT("vehicle02_Car"),   TEXT("vehCar_vehicle02") },
 { TEXT("vehicle03_Car"),   TEXT("vehCar_vehicle03") },
 { TEXT("vehicle04_Truck"), TEXT("vehTruck_vehicle04") },
 { TEXT("vehicle05_Car"),   TEXT("vehCar_vehicle05") },
 { TEXT("vehicle06_Car"),   TEXT("vehCar_vehicle06") },
 { TEXT("vehicle07_Car"),   TEXT("vehCar_vehicle07") },
 { TEXT("vehicle08_Truck"), TEXT("vehTruck_vehicle08") },
 { TEXT("vehicle09_Van"),   TEXT("vehVan_vehicle09") },
 { TEXT("vehicle10_Bus"),   TEXT("vehBus_vehicle10") },
 { TEXT("vehicle11_Truck"), TEXT("vehTruck_vehicle11") },
 { TEXT("vehicle12_Car"),   TEXT("vehCar_vehicle12") },
 { TEXT("vehicle13_Car"),   TEXT("vehCar_vehicle13") },
};
const int32 LaLaBergWagenTypen::TYPEN_ANZAHL = UE_ARRAY_COUNT(LaLaBergWagenTypen::TYPEN);

FString LaLaBergWagenTypen::ContentPfad(const FTyp& Typ, const FString& Dateiname) {
 return FString::Printf(TEXT("/Game/CitySampleVehicles/%s/Mesh/%s.%s"), Typ.Ordner, *Dateiname, *Dateiname);
}

static void SammleDateinamen(const LaLaBergWagenTypen::FTyp& Typ, TArray<FString>& Namen) {
 Namen = {
  FString::Printf(TEXT("SM_%s_No_Wheel"), Typ.Praefix),
  FString::Printf(TEXT("SM_Wheel_Front_L_%s"), Typ.Praefix),
  FString::Printf(TEXT("SM_Wheel_Front_R_%s"), Typ.Praefix),
  FString::Printf(TEXT("SM_Wheel_Rear_L_%s"), Typ.Praefix),
  FString::Printf(TEXT("SM_Wheel_Rear_R_%s"), Typ.Praefix),
 };
}

void LaLaBergWagenTypen::TeilPfade(const FTyp& Typ, TArray<FString>& Pfade) {
 TArray<FString> Namen;
 SammleDateinamen(Typ, Namen);
 Pfade.Reset(Namen.Num());
 for (const FString& Name : Namen) Pfade.Add(ContentPfad(Typ, Name));
}

bool LaLaBergWagenTypen::BaueTeile(USceneComponent* Traeger, const FTyp& Typ, TArray<TObjectPtr<UStaticMeshComponent>>& Teile) {
 if (!Traeger) return false;
 AActor* Besitzer = Traeger->GetOwner();
 if (!Besitzer) return false;
 TArray<FString> Namen;
 SammleDateinamen(Typ, Namen);
 for (const FString& Name : Namen) {
  UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ContentPfad(Typ, Name));
  if (!Mesh) continue;
  auto* Teil = NewObject<UStaticMeshComponent>(Besitzer, MakeUniqueObjectName(Besitzer, UStaticMeshComponent::StaticClass(), *Name));
  Teil->SetStaticMesh(Mesh);
  Teil->SetupAttachment(Traeger);
  Teil->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Teil->SetCastShadow(true);
  Teil->RegisterComponent();
  Teile.Add(Teil);
 }
 // Die Karosserie (erstes Teil) muss dabei sein - reine Raeder ohne
 // Karosserie waeren kein brauchbares Auto.
 return Teile.Num() > 0;
}

UStaticMesh* LaLaBergWagenTypen::LadeLod(const FTyp& Typ) {
 const FString Name = FString::Printf(TEXT("SM_%s_LOD"), Typ.Praefix);
 return LoadObject<UStaticMesh>(nullptr, *ContentPfad(Typ, Name));
}
