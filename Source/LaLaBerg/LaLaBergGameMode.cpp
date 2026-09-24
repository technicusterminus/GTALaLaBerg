#include "LaLaBergGameMode.h"
#include "LaLaBergCharacter.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "PhysicsEngine/BodySetup.h"
#include "Materials/MaterialInterface.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Components/PointLightComponent.h"
#include "Engine/SkyLight.h"
#include "Engine/TextureCube.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Components/LightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "TimerManager.h"
#include "Engine/GameInstance.h"
#include "LaLaBergMenueSteuerung.h"
#include "LaLaBergWagen.h"
#include "LaLaBergAuftraege.h"
#include "LaLaBergPolizei.h"
#include "LaLaBergRevier.h"
#include "LaLaBergLaeden.h"
#include "LaLaBergKonto.h"
#include "LaLaBergHUD.h"
#include "LaLaBergWaffe.h"
#include "LaLaBergVerkehrsauto.h"
#include "LaLaBergAutoPool.h"
#include "LaLaBergKastenPool.h"
#include "LaLaBergPassantKI.h"
#include "LaLaBergVerletzbar.h"
#include "LaLaBergSonderfahrzeug.h"
#include "LaLaBergAmpel.h"
#include "LaLaBergHUD.h"
#include "UObject/UObjectIterator.h"
#include "EngineUtils.h"
#include "UnrealClient.h"
#include "Engine/GameViewportClient.h"
#include "HAL/FileManager.h"

ALaLaBergGameMode::ALaLaBergGameMode() {
 DefaultPawnClass=ALaLaBergCharacter::StaticClass();
 HUDClass=ALaLaBergHUD::StaticClass();
 PrimaryActorTick.bCanEverTick=true;
}
void ALaLaBergGameMode::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 AktualisiereTageszeit(DeltaSeconds);
}
// Dreht die Sonne ueber den Tag statt sie fest zu lassen (siehe InitGame,
// SonnenLage) - Elevation nach einer einfachen Kosinuskurve um den Mittag,
// derselbe Azimut wie im Ausgangswert (SONNEN_AZIMUT). Staerke und Farbe
// folgen der Elevation: voll und neutral tagsueber, waermer nahe dem
// Horizont, schwach und leicht blaeulich nachts - nie ganz aus, sonst
// wirkte eine Gasse nachts als reines Schwarz ohne jede Kontur.
void ALaLaBergGameMode::AktualisiereTageszeit(float DeltaSeconds) {
 if(!SonnenLicht) return;
 Tageszeit=FMath::Fmod(Tageszeit+DeltaSeconds*(24.0f/TAGESLAENGE_SEKUNDEN),24.0f);
 const float Stundenwinkel=2.0f*PI*(Tageszeit-12.0f)/24.0f;
 const float ElevationGrad=70.0f*FMath::Cos(Stundenwinkel)-10.0f;
 SonnenLicht->GetOwner()->SetActorRotation(FRotator(-ElevationGrad,SONNEN_AZIMUT,0));
 // Ueber der Horizontlinie (0 Grad) voll, darunter rasch schwaecher - kein
 // hartes Abschneiden, sonst springt die Stadt sichtbar von hell auf dunkel.
 const float TagAnteil=FMath::Clamp((ElevationGrad+8.0f)/16.0f,0.0f,1.0f);
 // Farbtemperatur nach Sonnenhoehe (warmes Orange knapp ueber dem Horizont,
 // neutrales Weiss hoch am Himmel), zusaetzlich zur Nacht hin ins Blaue
 // gedreht - ueber TagAnteil ineinander geblendet statt zweier getrennter
 // Faelle, damit der Wechsel sichtbar weich bleibt.
 if(auto* D=Cast<UDirectionalLightComponent>(SonnenLicht)) {
  D->SetIntensity(FMath::Lerp(0.05f,10.0f,TagAnteil));
  const float Hoehenanteil=FMath::Clamp((ElevationGrad+10.0f)/70.0f,0.0f,1.0f);
  const FLinearColor Tagesfarbe=FMath::Lerp(FLinearColor(1.0f,0.55f,0.32f),FLinearColor(1.0f,0.97f,0.92f),Hoehenanteil);
  D->SetLightColor(FMath::Lerp(FLinearColor(0.5f,0.6f,0.9f),Tagesfarbe,TagAnteil));
 }
 // Die feste Tageslicht-Cubemap selbst kennt keine Tageszeit (siehe InitGame -
 // die Echtzeitaufnahme blieb bislang schwarz, Ursache offen). SetLightColor
 // faerbt ihre Ausgabe aber trotzdem ein, ohne das Cubemap-Bild selbst zu
 // aendern - dieselbe Tag-Nacht-Faerbung wie bei der Sonne oben, damit das
 // Umgebungslicht wenigstens die Farbtemperatur mitmacht, nicht nur die Staerke.
 if(SkyLicht) {
  SkyLicht->SetIntensity(FMath::Lerp(0.15f,1.2f,TagAnteil));
  const float Hoehenanteil=FMath::Clamp((ElevationGrad+10.0f)/70.0f,0.0f,1.0f);
  const FLinearColor Tagesfarbe=FMath::Lerp(FLinearColor(1.0f,0.55f,0.32f),FLinearColor(1.0f,0.97f,0.92f),Hoehenanteil);
  SkyLicht->SetLightColor(FMath::Lerp(FLinearColor(0.5f,0.6f,0.9f),Tagesfarbe,TagAnteil));
 }
 // Das enge Automatikfenster (siehe InitGame, 0.95-1.70) haelt tagsueber
 // bewusst gegen jedes Pumpen beim Blick in einen Torbogen - unveraendert
 // liesse es die Belichtung nachts aber vergeblich gegen ein taghelles Ziel
 // hochregeln (bzw. am oberen Anschlag haengen bleiben), die Stadt bliebe
 // nachts unnatuerlich hell. Das Fenster wandert deshalb mit TagAnteil nach
 // unten, gleich breit wie tagsueber, nur um ein dunkleres Ziel herum.
 if(Belichtung) {
  FPostProcessSettings& PP=Belichtung->Settings;
  PP.AutoExposureMinBrightness=FMath::Lerp(0.08f,0.95f,TagAnteil);
  PP.AutoExposureMaxBrightness=FMath::Lerp(0.35f,1.70f,TagAnteil);
 }
}
void ALaLaBergGameMode::InitGame(const FString& MapName,const FString& Options,FString& ErrorMessage) {
 Super::InitGame(MapName,Options,ErrorMessage);
 const double Start=FPlatformTime::Seconds();
 FString Text;
 TSharedPtr<FJsonObject> Data;
 // Claude: Wenn die Stadt bereits als StaticMesh-Assets vorliegt, wird sie
 // geladen statt gebaut. Das ist Codex' Importweg; er bringt Nanite mit und
 // spart die Sekunden, die das Netz sonst jedes Mal kostet.
 const bool bAusAssets=LadeAusAssets(Data);

 // Claude: die stadtweite Ausleitung hat Vorrang, der Hauptplatz bleibt Rueckfall.
 const FString Stadt=FPaths::ProjectContentDir()/TEXT("SourceData/stadt.json");
 const FString Quelle=bAusAssets
  ? FPaths::ProjectContentDir()/TEXT("SourceData/Sectors/manifest.json")
  : (FPaths::FileExists(Stadt)?Stadt:FPaths::ProjectContentDir()/TEXT("SourceData/hauptplatz.json"));
 // Das kleine Manifest wurde im Asset-Lader bereits geparst. Die 84-MB-
 // Stadtdatei ein zweites Mal nur fuer Spawn und Gebaeudezahl einzulesen,
 // kostete acht Sekunden ohne sichtbaren Nutzen.
 if(!bAusAssets) {
  if(!FFileHelper::LoadFileToString(Text,*Quelle) ||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Data) || !Data.IsValid()) {
   ErrorMessage=TEXT("Missing or invalid city source data");
   UE_LOG(LogTemp,Error,TEXT("LALABERG_IMPORT_FAILED %s"),*ErrorMessage); return;
  }
 }
 // Aus Assets geladen: die Netze stehen schon, nur die Angaben zum Start
 // kommen aus dem bereits geparsten Manifest.
 TArray<TSharedPtr<FJsonValue>> Leer;
 const TArray<TSharedPtr<FJsonValue>>& Sektionen=bAusAssets?Leer:Data->GetArrayField(TEXT("sections"));
 UE_LOG(LogTemp,Display,TEXT("LALABERG_ZEIT parse=%.1fs"),FPlatformTime::Seconds()-Start);
 double TangentSekunden=0, MeshSekunden=0;
 BuildingCount=Data->GetIntegerField(TEXT("buildingCount"));
 // Claude: bis hierher zeichnete die ganze Stadt mit dem unbeleuchteten
 // VertexColorMaterial aus den Engine-Debugmaterialien. Jede Klasse bekommt
 // jetzt ihr eigenes beleuchtetes Material mit eigener Rauheit.
 auto* Ersatz=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/EngineDebugMaterials/VertexColorMaterial.VertexColorMaterial"));
 auto Laden=[Ersatz](const TCHAR* Pfad) {
  auto* M=LoadObject<UMaterialInterface>(nullptr,Pfad);
  if(!M) UE_LOG(LogTemp,Warning,TEXT("LALABERG_MATERIAL_FEHLT %s"),Pfad);
  return M?M:Ersatz;
 };
 UMaterialInterface* MPutz=Laden(TEXT("/Game/Art/Materials/M_Putz.M_Putz"));
 UMaterialInterface* MZiegel=Laden(TEXT("/Game/Art/Materials/M_Ziegel.M_Ziegel"));
 UMaterialInterface* MAsphalt=Laden(TEXT("/Game/Art/Materials/M_Asphalt.M_Asphalt"));
 UMaterialInterface* MBoden=Laden(TEXT("/Game/Art/Materials/M_Boden.M_Boden"));
 UMaterialInterface* MWasser=Laden(TEXT("/Game/Art/Materials/M_Wasser.M_Wasser"));
 UMaterialInterface* MLaub=Laden(TEXT("/Game/Art/Materials/M_Laub.M_Laub"));
 UMaterialInterface* MStein=Laden(TEXT("/Game/Art/Materials/M_Stein.M_Stein"));
 UMaterialInterface* MLack=Laden(TEXT("/Game/Art/Materials/M_Lack.M_Lack"));
 UMaterialInterface* MGlas=Laden(TEXT("/Game/Art/Materials/M_Glas.M_Glas"));
 UMaterialInterface* MStoff=Laden(TEXT("/Game/Art/Materials/M_Stoff.M_Stoff"));
 for(const auto& Value:Sektionen) {
  const auto Obj=Value->AsObject();
  const auto& Positions=Obj->GetArrayField(TEXT("positions"));
  TArray<FVector> Vertices; TArray<int32> Indices; TArray<FVector2D> UVs;
  TArray<FLinearColor> Colors; TArray<FVector> Normals; TArray<FProcMeshTangent> Tangents;
  const auto& RGB=Obj->GetArrayField(TEXT("color"));
  const FLinearColor Color(RGB[0]->AsNumber(),RGB[1]->AsNumber(),RGB[2]->AsNumber(),1);
  // Waende bringen ihre eigene Texturkoordinate mit: waagerecht die Fenster-
  // achse, senkrecht das Geschoss. Alles andere wird weltbezogen projiziert
  // und braucht hier nur einen Platzhalter.
  const TArray<TSharedPtr<FJsonValue>>* Koordinaten=nullptr;
  const bool EigeneUV=Obj->TryGetArrayField(TEXT("uvs"),Koordinaten) &&
   Koordinaten->Num()*3==Positions.Num()*2;
  for(int32 i=0;i<Positions.Num();i+=3) {
   const FVector P(Positions[i]->AsNumber(),Positions[i+1]->AsNumber(),Positions[i+2]->AsNumber());
   Vertices.Add(P);
   const int32 k=i/3*2;
   UVs.Add(EigeneUV ? FVector2D((*Koordinaten)[k]->AsNumber(),(*Koordinaten)[k+1]->AsNumber())
                    : FVector2D(P.X/200,P.Y/200));
   Colors.Add(Color);
  }
  for(const auto& Index:Obj->GetArrayField(TEXT("indices"))) Indices.Add(static_cast<int32>(Index->AsNumber()));
  if(Vertices.IsEmpty() || Indices.IsEmpty()) continue;
  const FString Klasse=Obj->GetStringField(TEXT("name"));
  UMaterialInterface* Material=MBoden;
  if(Klasse.StartsWith(TEXT("Wall"))) Material=MPutz;
  else if(Klasse.StartsWith(TEXT("Roof"))) Material=MZiegel;
  else if(Klasse.StartsWith(TEXT("Road")) || Klasse.StartsWith(TEXT("Rail")) || Klasse.StartsWith(TEXT("Plaza"))) Material=MAsphalt;
 else if(Klasse.StartsWith(TEXT("Sidewalk")) || Klasse.StartsWith(TEXT("Gehweg"))) Material=MStein;
  else if(Klasse.StartsWith(TEXT("Water"))) Material=MWasser;
  else if(Klasse.StartsWith(TEXT("Tree")) || Klasse.StartsWith(TEXT("Trunk"))) Material=MLaub;
  else if(Klasse.StartsWith(TEXT("Stone")) || Klasse.StartsWith(TEXT("Figure"))) Material=MStein;
  else if(Klasse.StartsWith(TEXT("Auto"))) Material=MLack;
  else if(Klasse.StartsWith(TEXT("Glas"))) Material=MGlas;
  else if(Klasse.StartsWith(TEXT("Reifen"))) Material=MAsphalt;
  else if(Klasse.StartsWith(TEXT("Stoff")) || Klasse.StartsWith(TEXT("Haut"))) Material=MStoff;
  else if(Klasse.StartsWith(TEXT("Sockel")) || Klasse.StartsWith(TEXT("Gesims")) ||
          Klasse.StartsWith(TEXT("Laden")) || Klasse.StartsWith(TEXT("Kamin"))) Material=MStein;
  auto* Actor=GetWorld()->SpawnActor<AActor>();
  auto* Mesh=NewObject<UProceduralMeshComponent>(Actor);
  Actor->SetRootComponent(Mesh); Actor->AddInstanceComponent(Mesh);
  // Beweglich anmelden: ohne gebautes Licht traegt eine statische Flaeche in
  // einer statischen Beleuchtung nichts bei und bleibt schwarz.
  Mesh->SetMobility(EComponentMobility::Movable);
  Mesh->RegisterComponent();
  Mesh->bUseComplexAsSimpleCollision=true;
  Mesh->bUseAsyncCooking=false;
  Mesh->SetCollisionProfileName(TEXT("BlockAll"));
  Mesh->GetBodySetup()->bDoubleSidedGeometry=true;
  // Claude: CalculateTangentsForMesh verschweisst intern und brauchte fuer die
  // stadtweite Ausleitung 328 der 332 Sekunden Ladezeit. Die Stuetzpunkte sind
  // ohnehin je Flaeche eigenstaendig, darum genuegt die Flaechennormale.
  const double T0=FPlatformTime::Seconds();
  Normals.Init(FVector::ZeroVector,Vertices.Num());
  for(int32 i=0;i+2<Indices.Num();i+=3) {
   // Reihenfolge wie beim Zeichnen: erst die dritte, dann die zweite Ecke,
   // sonst zeigt die Normale in den Koerper hinein.
   const FVector N=FVector::CrossProduct(Vertices[Indices[i+2]]-Vertices[Indices[i]],Vertices[Indices[i+1]]-Vertices[Indices[i]]);
   Normals[Indices[i]]+=N; Normals[Indices[i+1]]+=N; Normals[Indices[i+2]]+=N;
  }
  for(FVector& N:Normals) N=N.GetSafeNormal(UE_SMALL_NUMBER,FVector::UpVector);
  const double T1=FPlatformTime::Seconds();
  Mesh->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UVs,Colors,Tangents,true);
  TangentSekunden+=T1-T0; MeshSekunden+=FPlatformTime::Seconds()-T1;
  if(Material) Mesh->SetMaterial(0,Material);
  Actor->Tags.Add(FName(*Klasse));
 }
 // Sonne, Himmel und Atmosphaere entstehen aufgeschoben: erst einstellen,
 // dann anmelden. Nachtraeglich gesetzte Werte wie bAtmosphereSunLight oder
 // die Beweglichkeit erreichen den bereits gebauten Renderzustand nicht mehr
 // - die Stadt blieb deshalb schwarz unter einem Daemmerungshimmel.
 const FTransform SonnenLage(FRotator(-50,152,0),FVector(0,0,15000));
 auto* Sun=GetWorld()->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(),SonnenLage);
 auto* Directional=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
 if(Directional) {
  Directional->SetMobility(EComponentMobility::Movable);
  Directional->Intensity=11.0f;                             // Lux im Engine-Massstab
  Directional->LightColor=FColor(255,246,232);
  Directional->bAtmosphereSunLight=true;
  Directional->bPerPixelAtmosphereTransmittance=true;
  Directional->DynamicShadowDistanceMovableLight=60000.0f;   // Schatten bis 600 m
  Directional->DynamicShadowCascades=4;
  Directional->CascadeDistributionExponent=2.8f;
  Directional->LightSourceAngle=0.55f;
  // Zwei Richtungslichter streiten sonst darum, welches das Vorwaertsschattieren
  // fuehrt - die Engine meldet das als Warnung ins Bild. Die Sonne fuehrt.
  Directional->ForwardShadingPriority=10;
  Directional->bCastVolumetricShadow=false;
  // Probe: -LaLaBergOhneSchatten schaltet den Schattenwurf der Sonne ab.
  if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergOhneSchatten"))) Directional->CastShadows=false;
  if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergHellSonne"))) Directional->Intensity=40.0f;
  if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergOhneAtmo"))) {
   Directional->bAtmosphereSunLight=false;
   Directional->bPerPixelAtmosphereTransmittance=false;
  }
 }
 Sun->FinishSpawning(SonnenLage);
 // Die Lage aus dem Spawn kommt beim Licht nicht an; erst diese Zuweisung
 // stellt die Sonne wirklich auf den gewuenschten Stand.
 Sun->SetActorRotation(FRotator(-50,152,0));
 SonnenLicht=Sun->GetLightComponent();
 SonnenLicht->MarkRenderStateDirty();

 // Aufhelllicht von der Gegenseite, ohne Schatten. In einer Altstadtgasse
 // sieht eine Schattenfassade nur einen schmalen Streifen Himmel; ohne diese
 // Fuellung bleibt sie fast schwarz.
 const FTransform FuellLage(FRotator(-28,-20,0),FVector(0,0,15000));
 auto* Fuell=GetWorld()->SpawnActorDeferred<ADirectionalLight>(ADirectionalLight::StaticClass(),FuellLage);
 if(auto* F=Cast<UDirectionalLightComponent>(Fuell->GetLightComponent())) {
  F->SetMobility(EComponentMobility::Movable);
  F->Intensity=1.0f;                                        // nur noch Hauch
  F->LightColor=FColor(206,222,255);
  F->CastShadows=false;
  F->bAtmosphereSunLight=false;
  F->bAffectsWorld=true;
  F->ForwardShadingPriority=0;
 }
 Fuell->FinishSpawning(FuellLage);
 Fuell->SetActorRotation(FRotator(-28,-20,0));

 GetWorld()->SpawnActor<ASkyAtmosphere>();

 auto* Sky=GetWorld()->SpawnActorDeferred<ASkyLight>(ASkyLight::StaticClass(),FTransform(FVector(0,0,20000)));
 auto* SkyComp=Cast<USkyLightComponent>(Sky->GetLightComponent());
 if(SkyComp) {
  SkyComp->SetMobility(EComponentMobility::Movable);
  // Fest vorgegebener Tageshimmel. Die Echtzeitaufnahme der Atmosphaere
  // lieferte in dieser Welt nachweislich nichts: mit abgeschaltetem Richtungs-
  // licht war jede Flaeche absolut schwarz, mit diesem Cubemap hell und
  // farbig. Alle Engine-Bedingungen fuer die Aufnahme waren erfuellt; die
  // Ursache ist offen. -LaLaBergHimmelEchtzeit schaltet sie zum Pruefen ein.
  if(!FParse::Param(FCommandLine::Get(),TEXT("LaLaBergHimmelEchtzeit"))) {
   SkyComp->SourceType=SLS_SpecifiedCubemap;
   SkyComp->Cubemap=LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"));
   SkyComp->bRealTimeCapture=false;
   UE_LOG(LogTemp,Display,TEXT("LALABERG_HIMMEL cubemap=%d"),SkyComp->Cubemap!=nullptr);
  } else {
   SkyComp->SourceType=SLS_CapturedScene;                    // Himmelsfarbe aus der Atmosphaere
   SkyComp->bRealTimeCapture=true;
  }
  // Ohne kraeftiges Himmelslicht sind die Schattenseiten in einer Gasse
  // vollstaendig schwarz - eine Altstadt lebt aber vom Streulicht. 1.6 war
  // zu viel: Sonnen- und Schattenseite sahen gleich aus, die Stadt flach
  // und blass (2026-09-21); 1.2 laesst die Gassen hell genug.
  SkyComp->Intensity=1.2f;
  SkyComp->bLowerHemisphereIsBlack=false;                    // Bodenlicht statt schwarzer Unterseite
 }
 Sky->FinishSpawning(FTransform(FVector(0,0,20000)));
 SkyLicht=SkyComp;

 auto* Nebel=GetWorld()->SpawnActor<AExponentialHeightFog>();
 if(auto* NebelComp=Nebel->GetComponent()) {
  // Nur so viel Dunst, dass Entfernung lesbar wird. Mehr davon nimmt der
  // Stadt jede Farbe - die Daecher wurden grau statt ziegelrot.
  NebelComp->SetFogDensity(0.0022f);
  NebelComp->SetFogHeightFalloff(0.22f);
  NebelComp->SetStartDistance(1200.0f);
  NebelComp->SetFogInscatteringColor(FLinearColor(0.44f,0.52f,0.60f));
 }

 // Feste Belichtung nach Kameradaten: Blende 11, 1/250 s, ISO 100 ergibt
 // rund EV100 15 - der Wert fuer klaren Tag. Automatik ist hier falsch, sie
 // pumpt bei jedem Blick in einen Torbogen.
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();
 Belichtung=Post;
 Post->bUnbound=true;
 FPostProcessSettings& PP=Post->Settings;
 // Enges Automatikfenster: die Helligkeit steht praktisch fest, ohne dass die
 // Werte an eine bestimmte Sonnenstaerke gekettet sind.
 PP.bOverride_AutoExposureMethod=true;         PP.AutoExposureMethod=AEM_Histogram;
 PP.bOverride_AutoExposureMinBrightness=true;  PP.AutoExposureMinBrightness=0.95f;
 PP.bOverride_AutoExposureMaxBrightness=true;  PP.AutoExposureMaxBrightness=1.70f;
 PP.bOverride_AutoExposureSpeedUp=true;        PP.AutoExposureSpeedUp=3.0f;
 PP.bOverride_AutoExposureSpeedDown=true;      PP.AutoExposureSpeedDown=1.5f;
 PP.bOverride_AutoExposureBias=true;           PP.AutoExposureBias=0.0f;
 PP.bOverride_BloomIntensity=true;          PP.BloomIntensity=0.35f;
 PP.bOverride_VignetteIntensity=true;       PP.VignetteIntensity=0.18f;
 PP.bOverride_FilmGrainIntensity=true;      PP.FilmGrainIntensity=0.0f;
 // Saettigung und Kontrast: die amtlichen Farbwerte sind blass, unter
 // blauem Himmelslicht wirken sie sonst grau. Mit 1.04/1.05 sah die Altstadt
 // noch ausgewaschen aus (Rueckmeldung 2026-09-21).
 PP.bOverride_ColorSaturation=true;         PP.ColorSaturation=FVector4(1.13f,1.11f,1.06f,1.0f);
 PP.bOverride_ColorContrast=true;           PP.ColorContrast=FVector4(1.10f,1.10f,1.10f,1.0f);
 PP.bOverride_MotionBlurAmount=true;        PP.MotionBlurAmount=0.0f;
 PP.bOverride_AmbientOcclusionIntensity=true; PP.AmbientOcclusionIntensity=0.70f;
 PP.bOverride_AmbientOcclusionRadius=true;  PP.AmbientOcclusionRadius=120.0f;
 // Startpunkt: die Ausleitung nennt ihn beim Namen, sonst der Nullpunkt.
 double Ground=Data->GetNumberField(TEXT("spawnHeightCm"));
 FVector StartOrt(0,0,Ground+110); FRotator Blick(0,0,0); FString StartName=TEXT("Hauptplatz");
 const TSharedPtr<FJsonObject>* Ort;
 if(Data->TryGetObjectField(TEXT("spawn"),Ort)) {
  StartOrt=FVector((*Ort)->GetNumberField(TEXT("x")),(*Ort)->GetNumberField(TEXT("y")),(*Ort)->GetNumberField(TEXT("z"))+110);
  Blick=FRotator(0,(*Ort)->GetNumberField(TEXT("blick")),0);
  StartName=(*Ort)->GetStringField(TEXT("name"));
  Ground=StartOrt.Z-110;
 }
 // Ohne diese Zuweisung sucht die Spielart sich selbst einen Startpunkt und
 // landet am Nullpunkt - die Figur stand dann am Hauptplatz statt am Klinikum.
 Startpunkt=GetWorld()->SpawnActor<APlayerStart>(StartOrt,Blick);
 UE_LOG(LogTemp,Display,TEXT("LALABERG_START_ORT %s %s"),*StartName,*StartOrt.ToString());

 // Ein fahrbarer Wagen neben dem Startpunkt. Er wird erst abgesetzt, wenn
 // die Kollision der Stadt steht - sonst faellt er durch die Fahrbahn.
 {
  FTimerHandle H;
  // Beim Klinikum-Parkplatz. Ein Stellplatz selbst taugt nicht: dort stand
  // der Wagen mal auf einem Auto, mal fuhr er gleich ins naechste. Deshalb
  // die naechste freie Fahrbahn suchen und den Wagen entlang der Strasse
  // ausrichten.
  const FVector WagenOrt(-153670,12480,StartOrt.Z);
  GetWorldTimerManager().SetTimer(H,[this,WagenOrt]() {
   bool bFrei=false;
   const FTransform Platz=SucheFahrbahn(WagenOrt,bFrei);
   auto* Wagen=GetWorld()->SpawnActor<ALaLaBergWagen>(Platz.GetLocation(),Platz.Rotator());
   if(Wagen) {
    // Einmal in der Lackiererei gewesen, behaelt der Wagen seine Farbe.
    FLinearColor Lack(0.16f,0.22f,0.34f);
    const ULaLaBergKonto* Konto=ULaLaBergKonto::Hole(this);
    const bool bGespeichert=Konto&&Konto->HatLack(Lack);
    Wagen->SetzeLack(Lack,bGespeichert);
    UE_LOG(LogTemp,Display,TEXT("LALABERG_WAGEN %s gier=%.0f frei=%d"),*Platz.GetLocation().ToString(),Platz.Rotator().Yaw,bFrei?1:0);
   }
  },2.0f,false);
 }
 // Lieferauftraege: die erste blaue Saeule zwischen Startpunkt und Wagen, auf
 // einer Fahrbahn - erst wenn die Stadtkollision steht (Bodenhoehe, Ziele).
 {
  FTimerHandle H;
  const FVector Mitte((-167210.0-158830.0)*0.5,(14530.0+20580.0)*0.5,StartOrt.Z);
  GetWorldTimerManager().SetTimer(H,[this,Mitte]() {
   // Die Polizei gleich mit - sie braucht keinen Ort, nur das Strassennetz.
   GetWorld()->SpawnActor<ALaLaBergPolizei>();
   // Die Reviere des Farbkriegs: brauchen die Stadtkollision, weil ihre
   // Saeulen auf dem Boden stehen (siehe Docs/Geschichte.md).
   GetWorld()->SpawnActor<ALaLaBergRevier>();
   auto* Auftraege=GetWorld()->SpawnActor<ALaLaBergAuftraege>();
   bool bFrei=false;
   if(Auftraege) Auftraege->SetzeStartOrt(SucheFahrbahn(Mitte,bFrei).GetLocation());
   // Laeden: die Lackiererei nah am Start, damit man sie mit dem ersten
   // Wagen gleich findet; der Paintball-Laden am Weg in die Stadt, beim
   // Bahnhof. Beides auf einer Fahrbahn, wie die Auftragssaeulen - Hoehe
   // Fahrbahn, nicht die Absetzhoehe eines Wagens (SucheFahrbahn: +92 cm).
   if(auto* Laeden=GetWorld()->SpawnActor<ALaLaBergLaeden>()) {
    Laeden->Stelle(ALaLaBergLaeden::LACKIEREREI,SucheFahrbahn(FVector(-150000.0f,12000.0f,Mitte.Z),bFrei).GetLocation()-FVector(0,0,92.0f));
    Laeden->Stelle(ALaLaBergLaeden::PAINTBALL,SucheFahrbahn(FVector(-37630.0f,26000.0f,Mitte.Z),bFrei).GetLocation()-FVector(0,0,92.0f));
   }
  },2.5f,false);
 }
 // Sonderfahrzeuge: nichts weist auf sie hin, keine Saeule, kein Eintrag auf
 // der Karte. Der Panzer steht im Waldstueck westlich der Stadt (die Stelle
 // hat den dichtesten Baumbestand bei mindestens 45 m Abstand zu jeder
 // Strasse und 60 m zu jedem Haus), der Hubschrauber auf dem Dach des
 // Klinikums - dort, wo ein Rettungshubschrauber auch landen wuerde. Beide
 // erst nach der Stadtkollision, sonst faellt der Panzer durchs Gelaende
 // und der Hubschrauber findet kein Dach.
 {
  FTimerHandle H;
  GetWorldTimerManager().SetTimer(H,[this]() {
   auto Absetzen=[this](ELaLaBergSonderart Art,const FVector2D& XY,float Suchhoehe,float Ueber) {
    FHitResult Treffer;
    const FVector Oben(XY.X,XY.Y,Suchhoehe);
    if(!GetWorld()->LineTraceSingleByChannel(Treffer,Oben,Oben-FVector(0,0,60000.0f),ECC_Visibility)) {
     UE_LOG(LogTemp,Warning,TEXT("LALABERG_SONDER kein Boden bei %s"),*XY.ToString());
     return;
    }
    const FVector Ort=Treffer.ImpactPoint+FVector(0,0,Ueber);
    auto* Fahrzeug=GetWorld()->SpawnActorDeferred<ALaLaBergSonderfahrzeug>(ALaLaBergSonderfahrzeug::StaticClass(),
     FTransform(FRotator(0,Art==ELaLaBergSonderart::Panzer?35.0f:180.0f,0),Ort),
     nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Fahrzeug) return;
    Fahrzeug->SetzeArt(Art);
    Fahrzeug->FinishSpawning(FTransform(FRotator(0,Art==ELaLaBergSonderart::Panzer?35.0f:180.0f,0),Ort));
   };
   Absetzen(ELaLaBergSonderart::Panzer,FVector2D(-187070.0f,40760.0f),20000.0f,110.0f);
   // Vom Dach aus nach unten: der Strahl faengt hoch genug an, um ueber dem
   // Klinikum zu beginnen, und trifft damit das Dach statt des Vorplatzes.
   Absetzen(ELaLaBergSonderart::Hubschrauber,FVector2D(-159000.0f,14230.0f),30000.0f,150.0f);
  },3.0f,false);
 }
 // KI-Verkehr und Passanten, aus demselben Grund erst verzoegert wie der
 // fahrbare Wagen: die Stadtkollision muss stehen, bevor jemand darauf
 // faehrt oder geht.
 {
  FTimerHandle H;
  GetWorldTimerManager().SetTimer(H,[this]() { LadeVerkehr(); },2.3f,false);
 }
 UE_LOG(LogTemp,Display,TEXT("LALABERG_LICHT sonne=%d lux=%.1f richtung=%s atmo=%d himmel=%d"),
  (int32)Sun->GetLightComponent()->Mobility.GetValue(),Sun->GetLightComponent()->Intensity,
  *Sun->GetActorForwardVector().ToString(),Directional?(Directional->bAtmosphereSunLight?1:0):-1,
  SkyComp?(int32)SkyComp->Mobility.GetValue():-1);
 bSceneReady=true;
 UE_LOG(LogTemp,Display,TEXT("LALABERG_SCENE_READY buildings=%d spawnGround=%.2f source=%s gesamt=%.1fs tangenten=%.1fs mesh=%.1fs"),BuildingCount,Ground,*FPaths::GetCleanFilename(Quelle),FPlatformTime::Seconds()-Start,TangentSekunden,MeshSekunden);
}
// Testergebnisse auch als Datei: Shipping-Builds schreiben kein Protokoll,
// dort ist diese Datei der einzige Beleg. Liegt unter Saved/Logs.
void ALaLaBergGameMode::Beleg(const FString& Zeile) {
 UE_LOG(LogTemp,Display,TEXT("%s"),*Zeile);
 const FString Datei=FPaths::ProjectSavedDir()/TEXT("Logs/LaLaBerg-Test.txt");
 FFileHelper::SaveStringToFile(FDateTime::Now().ToString(TEXT("%H:%M:%S "))+Zeile+LINE_TERMINATOR,*Datei,
  FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
}

// Welche Art Flaeche liegt unter einem Punkt? Liefert den Abschnittsnamen
// (Road, Plaza, Ground, Roof, Auto ...) und die Hoehe der Oberflaeche.
static bool BodenArt(UWorld* Welt,const FVector& P,FString& Art,float& Z) {
 FHitResult Boden;
 if(!Welt->LineTraceSingleByChannel(Boden,FVector(P.X,P.Y,P.Z+3000),FVector(P.X,P.Y,P.Z-3000),ECC_Visibility)) return false;
 const auto* Netz=Cast<UStaticMeshComponent>(Boden.GetComponent());
 Art=Netz&&Netz->GetStaticMesh()?Netz->GetStaticMesh()->GetName():FString();
 Z=Boden.ImpactPoint.Z;
 return true;
}

FTransform ALaLaBergGameMode::SucheFahrbahn(const FVector& Nahe,bool& bGefunden) const {
 UWorld* Welt=GetWorld();
 bGefunden=false;
 // In Ringen nach aussen, alle 4 m ein Punkt, bis 120 m. Ein Punkt taugt,
 // wenn er auf Fahrbahn liegt, frei ist und in einer von 16 Richtungen noch
 // 16 m voraus und 4 m zurueck Fahrbahn folgt - also eine Strasse ist und
 // kein Fleck Asphalt.
 for(int32 Ring=0;Ring<=30;Ring++) {
  const int32 Schritte=FMath::Max(1,Ring*6);
  for(int32 s=0;s<Schritte;s++) {
   const float Winkel=2.0f*PI*s/Schritte;
   const FVector P=Nahe+FVector(FMath::Cos(Winkel),FMath::Sin(Winkel),0)*Ring*400.0f;
   FString Art; float Z;
   if(!BodenArt(Welt,P,Art,Z)||!Art.Contains(TEXT("_Road"))) continue;
   for(int32 r=0;r<16;r++) {
    const FRotator Richtung(0,r*22.5f,0);
    const FVector V=Richtung.Vector();
    // Auch 1,5 m links und rechts muss Fahrbahn sein - sonst stand der
    // Wagen mit zwei Raedern in der Wiese.
    const FVector Q=FVector(-V.Y,V.X,0)*150.0f;
    bool bStrasse=true;
    for(float D:{-400.0f,0.0f,400.0f,800.0f,1200.0f,1600.0f}) {
     for(const FVector& Seite:{FVector::ZeroVector,Q,-Q}) {
      FString A2; float Z2;
      if(!BodenArt(Welt,P+V*D+Seite,A2,Z2)||!A2.Contains(TEXT("_Road"))||FMath::Abs(Z2-Z)>250.0f) { bStrasse=false; break; }
     }
     if(!bStrasse) break;
    }
    if(!bStrasse) continue;
    const FVector Ort(P.X,P.Y,Z+92);
    // Kasten etwas kleiner als der Wagen, leicht angehoben und nach vorn
    // verlaengert: Boden und Bordstein zaehlen nicht, ein Auto oder eine
    // Mauer auf den ersten zehn Metern schon.
    if(Welt->OverlapAnyTestByChannel(Ort+V*400.0f+FVector(0,0,40),FQuat(Richtung),ECC_WorldStatic,
        FCollisionShape::MakeBox(FVector(650,100,50)))) continue;
    bGefunden=true;
    return FTransform(Richtung,Ort);
   }
  }
 }
 return FTransform(FRotator(0,198,0),Nahe);
}

AActor* ALaLaBergGameMode::ChoosePlayerStart_Implementation(AController* Player) {
 return Startpunkt?static_cast<AActor*>(Startpunkt):Super::ChoosePlayerStart_Implementation(Player);
}

void ALaLaBergGameMode::BeginPlay() {
 Super::BeginPlay();
 // -LaLaBergNacht springt sofort auf Mitternacht - zum Pruefen der
 // Nachtfaerbung, ohne die vollen 600s eines Tageszyklus abzuwarten.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergNacht"))) Tageszeit=0.0f;
 if(auto* PC=GetWorld()->GetFirstPlayerController()) {
  PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false;
 }
 // Startbild, sofern nicht gerade ein Rauchtest oder ein Bildlauf laeuft.
 // Jeder automatische Lauf braucht eine laufende Welt. Das Startbild
 // pausiert sie - der Fahrtest stand deshalb bei Bild 2 still, weil keiner
 // seiner Zeitgeber je auslief.
 const bool bAutomatisch=FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSmoke")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergFoto")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergFahrtest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergWaffentest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergVerkehrFoto")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergLechFoto")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergKoerperFoto")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAmpelTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergUeberholTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAbbiegeTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAuftragTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergPolizeiTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergLadenTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAntriebTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergUebernahmeTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSperrTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSonderTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSchadenTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergTaxiTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergFigurTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergRevierTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergKopfTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergRennTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergJagdTest")) ||
                         FParse::Param(FCommandLine::Get(),TEXT("LaLaBergZielFoto"));
 if(bAutomatisch) Beleg(FString::Printf(TEXT("LALABERG_SPIELBEGINN nach %.1fs Programmlaufzeit, %d Gebaeude"),FPlatformTime::Seconds()-GStartTime,BuildingCount));
 if(!bAutomatisch) {
  if(UGameInstance* Spiel=GetGameInstance()) {
   if(auto* Menue=Spiel->GetSubsystem<ULaLaBergMenueSteuerung>()) {
    FTimerHandle H;
    GetWorldTimerManager().SetTimer(H,[Menue]() { Menue->ZeigeMenue(true); },0.4f,false);
   }
  }
 }
 if(auto* PC0=GetWorld()->GetFirstPlayerController()) {
  if(APawn* Pawn0=PC0->GetPawn())
   UE_LOG(LogTemp,Display,TEXT("LALABERG_START pawn=%s start=%d"),*Pawn0->GetActorLocation().ToString(),GetWorld()->GetAuthGameMode()!=nullptr);
 }
 // Claude: Belegbilder ohne Handgriff. -LaLaBergFoto stellt die Figur nach-
 // einander an drei Stellen auf, loest je eine Aufnahme aus und beendet sich.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergFoto"))) {
  struct FStandort { FVector Ort; FRotator Blick; bool bLuft=false; };
  TArray<FStandort> Orte = {
   // Freier Standpunkt im Hauptplatz - aus den Platzdaten gesucht, knapp
   // 40 m von der naechsten Hausmitte - mit Blick nach Osten zum Schmalzturm.
   {FVector(-1700,-2600,0),FRotator(-1,42,0)},         // Hauptplatz mit Marienbrunnen
   {FVector(2200,-9700,0),FRotator(-5,-43,0)},         // Strasse mit parkenden Wagen
   {FVector(-154430,13040,0),FRotator(-6,-36,0)},        // Der fahrbare Wagen am Klinikum
   // Luftbild ueber dem unteren Hauptplatz Richtung Schmalzturm: auf ihm
   // sieht man, welches Dach auf welchem Haus sitzt - vom Platz aus nicht.
   {FVector(-5200,-6400,0),FRotator(-30,40,0),true},
  };
  // -LaLaBergReihe stellt statt der drei Ansichten eine Messreihe: gleicher
  // Blick, verschiedene Sonnen- und Belichtungswerte. So laesst sich klaeren,
  // ob ein dunkles Bild an der Belichtung oder am Licht selbst haengt.
  const bool Reihe=FParse::Param(FCommandLine::Get(),TEXT("LaLaBergReihe"));
  if(Reihe) { Orte.Empty(); for(int32 i=0;i<6;i++) Orte.Add({FVector(-9000,12000,9000),FRotator(-28,-52,0)}); }
  for(int32 i=0;i<Orte.Num();i++) {
   FTimerHandle H;
   GetWorldTimerManager().SetTimer(H,[this,i,Orte,Reihe]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    APawn* Pawn=PC?PC->GetPawn():nullptr;
    if(!PC||!Pawn) return;
    FVector Ort=Orte[i].Ort;
    FRotator Blick=Orte[i].Blick;
    // Der Wagen steht dort, wo die Fahrbahnsuche ihn hingestellt hat -
    // die dritte Ansicht folgt ihm, statt auf einer festen Zahl zu stehen.
    if(i==2&&!Reihe) {
     for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) {
      Ort=It->GetActorLocation()+It->GetActorForwardVector()*560.0f+It->GetActorRightVector()*380.0f;
      Blick=(It->GetActorLocation()-FVector(Ort.X,Ort.Y,It->GetActorLocation().Z+120.0f)).Rotation();
      break;
     }
    }
    // Alle drei Standorte stehen jetzt auf dem Boden. Vorher blieb einer auf
    // seiner Zahl stehen - und die lag unter dem Gelaende.
    {                                                 // auf dem Boden absetzen
     FHitResult Hit; FCollisionQueryParams Params; Params.AddIgnoredActor(Pawn);
     if(GetWorld()->LineTraceSingleByChannel(Hit,FVector(Ort.X,Ort.Y,30000),FVector(Ort.X,Ort.Y,-30000),ECC_Visibility,Params)) Ort.Z=Hit.ImpactPoint.Z+180;
    }
    if(Orte[i].bLuft) {                               // 45 m ueber dem Boden schweben
     Ort.Z+=4500.0f;
     Pawn->GetMovementComponent()->StopMovementImmediately();
     if(auto* Bewegung=Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent())) Bewegung->SetMovementMode(MOVE_Flying);
    }
    Pawn->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation(Blick);
    if(auto* Anzeige=Cast<ALaLaBergHUD>(PC->GetHUD())) Anzeige->OrtSofort();
    if(Reihe) {
     static const float Lux[6]={75000,75000,75000,10,10,120000};
     static const float Bias[6]={0,6,12,6,12,0};
     static const bool Atmo[6]={true,true,true,true,false,true};
     if(SonnenLicht) { SonnenLicht->SetIntensity(Lux[i]); if(auto* D=Cast<UDirectionalLightComponent>(SonnenLicht)) { D->bAtmosphereSunLight=Atmo[i]; D->MarkRenderStateDirty(); } }
     if(Belichtung) { Belichtung->Settings.AutoExposureBias=Bias[i]; }
     UE_LOG(LogTemp,Display,TEXT("LALABERG_REIHE %d lux=%.0f bias=%.0f atmo=%d"),i,Lux[i],Bias[i],Atmo[i]?1:0);
    }
    // Probe: ein Engine-Wuerfel mit Engine-Material vor der Kamera. Bleibt der
    // dunkel, liegt es am Licht; ist er hell, liegt es an unserem Netz.
    if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergWuerfel")) && i==1) {
     const FVector Vorn=Ort+Orte[i].Blick.Vector()*3000.0;
     auto* Wuerfel=GetWorld()->SpawnActor<AStaticMeshActor>(Vorn,FRotator::ZeroRotator);
     auto* Netz=Wuerfel->GetStaticMeshComponent();
     Netz->SetMobility(EComponentMobility::Movable);
     Netz->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
     Netz->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
     Wuerfel->SetActorScale3D(FVector(12,12,12));
     UE_LOG(LogTemp,Display,TEXT("LALABERG_WUERFEL bei %s netz=%d"),*Vorn.ToString(),Netz->GetStaticMesh()!=nullptr);
    }
    // Probe: eine helle Punktlampe unmittelbar an der Kamera. Bleibt das Bild
    // auch damit schwarz, liegt es an Netz oder Material, nicht an der Sonne.
    if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergLampe")) && i==1) {
     auto* Lampe=GetWorld()->SpawnActorDeferred<APointLight>(APointLight::StaticClass(),FTransform(Ort+FVector(0,0,500)));
     if(auto* LK=Cast<UPointLightComponent>(Lampe->GetLightComponent())) {
      LK->SetMobility(EComponentMobility::Movable);
      LK->Intensity=50000000.0f; LK->AttenuationRadius=40000.0f; LK->bUseInverseSquaredFalloff=false;
     }
     Lampe->FinishSpawning(FTransform(Ort+FVector(0,0,500)));
     UE_LOG(LogTemp,Display,TEXT("LALABERG_LAMPE gesetzt"));
    }
    PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
    UE_LOG(LogTemp,Display,TEXT("LALABERG_FOTO %d bei %s"),i,*Ort.ToString());
   },8.0f+i*3.0f,false);
  }
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[]() { FPlatformMisc::RequestExitWithStatus(false,0); },8.0f+Orte.Num()*3.0f,false);
 }
 // Eigener Sichttest am Lech: Blick quer über das Wasser, mit genug Abstand
 // zur Ufergeometrie. So ist das animierte Material tatsächlich im Bild.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergLechFoto"))) {
  FTimerHandle LechBild;
  GetWorldTimerManager().SetTimer(LechBild,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Pawn=PC?PC->GetPawn():nullptr;
   if(!PC||!Pawn) return;
   const FVector Ort(-11000.0f,25000.0f,1450.0f);
   Pawn->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
   if(auto* Bewegung=Cast<UCharacterMovementComponent>(Pawn->GetMovementComponent())) Bewegung->SetMovementMode(MOVE_Flying);
   PC->SetControlRotation(FRotator(-19.0f,5.0f,0.0f));
   PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
   UE_LOG(LogTemp,Display,TEXT("LALABERG_LECH_FOTO ort=%s"),*Ort.ToString());
  },8.0f,false);
  FTimerHandle LechEnde;
  GetWorldTimerManager().SetTimer(LechEnde,[]() { FPlatformMisc::RequestExitWithStatus(false,0); },13.0f,false);
 }
 // Sichttest fuer die Spielfigur selbst: waagerechte Kamera auf freiem Feld,
 // ohne den schraegen Blickwinkel des Waffentests (der zum Wagen schaut) -
 // klaert, ob Koerper/Arm/Waffe an sich richtig sitzen oder nur die
 // Kameraneigung des anderen Tests den Eindruck verzerrt.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergKoerperFoto"))) {
  FTimerHandle Bild;
  GetWorldTimerManager().SetTimer(Bild,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(!PC) return;
   PC->SetControlRotation(FRotator(0,0,0));
   // Sitzt die Waffe in der Hand? Sie hing zuvor am Kasten-Arm, der bei
   // einer Skelettfigur unsichtbar ist - im Bild schwebte sie dann neben
   // der Figur. Am Bild allein ist das nur zu sehen, wenn man hinschaut;
   // der Abstand zum Handpunkt macht es pruefbar.
   float Waffenabstand=0.0f;
   if(auto* Held=Cast<ALaLaBergCharacter>(PC->GetPawn()))
    if(Held->HoleWaffenabstand(Waffenabstand))
     Beleg(FString::Printf(TEXT("LALABERG_WAFFE_GEHALTEN %s abstand=%.0fcm"),
      Waffenabstand<25.0f?TEXT("PASS"):TEXT("FAIL"),Waffenabstand));
   PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },4.0f,false);
  // Zweites Bild von der Seite: von hinten laesst sich nicht erkennen, ob
  // die Waffe nach vorn zeigt oder zu Boden haengt. Die Figur bleibt
  // stehen, nur die Kamera schwenkt um 90 Grad.
  FTimerHandle Seite;
  GetWorldTimerManager().SetTimer(Seite,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(!PC) return;
   if(APawn* Held=PC->GetPawn()) Held->SetActorRotation(FRotator(0,0,0));
   PC->SetControlRotation(FRotator(-5,-90,0));   // rechte Seite: dort sitzt die Waffe
  },5.0f,false);
  // Von der Seite jede der vier Waffen einmal: jede sitzt mit ihrem eigenen
  // Griffpunkt in der Hand (siehe ALaLaBergWaffe::GriffOrt), ein Bild nur
  // von der Pistole sagte ueber die anderen drei nichts. Je Waffe ein Bild
  // und die gemessene Griff-Hand-Distanz.
  for(int32 i=0;i<4;i++) {
   FTimerHandle Wechsel;
   GetWorldTimerManager().SetTimer(Wechsel,[this,i]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    auto* Held=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
    if(Held && Held->HoleWaffe()) Held->HoleWaffe()->SetzeArt(static_cast<ELaLaBergWaffenArt>(i));
   },5.5f+i*1.5f,false);
   FTimerHandle Bild2;
   GetWorldTimerManager().SetTimer(Bild2,[this]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    if(!PC) return;
    auto* Held=Cast<ALaLaBergCharacter>(PC->GetPawn());
    float Abstand=0.0f;
    if(Held && Held->HoleWaffe() && Held->HoleWaffenabstand(Abstand))
     Beleg(FString::Printf(TEXT("LALABERG_WAFFE_GEHALTEN %s abstand=%.0fcm waffe=%s"),
      Abstand<25.0f?TEXT("PASS"):TEXT("FAIL"),Abstand,*Held->HoleWaffe()->ArtName()));
    PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
   },6.3f+i*1.5f,false);
  }
  // Zuletzt ein paar Schritte laufen und von der Seite fotografieren: im
  // Stand ist nur die Standpose zu sehen, die Laufpose (Run_Shoot) sonst
  // nie. Die Kamera steht noch seitlich (siehe oben), die Figur laeuft
  // quer durchs Bild.
  GetWorldTimerManager().SetTimer(Laufen,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController())
    if(APawn* Held=PC->GetPawn()) Held->AddMovementInput(FVector::ForwardVector,1.0f);
  },0.016f,true,12.0f);
  FTimerHandle BildLauf;
  GetWorldTimerManager().SetTimer(BildLauf,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },13.6f,false);
  // Dann aus der normalen Spielkamera von hinten: so sieht der Spieler die
  // Figur. Erst vorwaerts laufend, dann mit Eingabe nach rechts (wie mit D):
  // die Figur soll sich dabei in die Laufrichtung drehen und vorwaerts
  // rennen, statt wie frueher seitwaerts zu rutschen.
  FTimerHandle Hinten;
  GetWorldTimerManager().SetTimer(Hinten,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(!PC) return;
   PC->SetControlRotation(FRotator(-8,0,0));
  },14.0f,false);
  FTimerHandle BildHinten;
  GetWorldTimerManager().SetTimer(BildHinten,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },15.2f,false);
  FTimerHandle Seitwaerts;
  GetWorldTimerManager().SetTimer(Seitwaerts,[this]() {
   GetWorldTimerManager().ClearTimer(Laufen);
   GetWorldTimerManager().SetTimer(Laufen,[this]() {
    if(auto* PC=GetWorld()->GetFirstPlayerController())
     if(APawn* Held=PC->GetPawn()) Held->AddMovementInput(FVector::RightVector,1.0f);
   },0.016f,true);
  },15.6f,false);
  FTimerHandle BildSeitwaerts;
  GetWorldTimerManager().SetTimer(BildSeitwaerts,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },16.8f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   GetWorldTimerManager().ClearTimer(Laufen);
   FPlatformMisc::RequestExitWithStatus(false,0);
  },18.0f,false);
 }
 // Fahrtest: Wagen uebernehmen, vier Sekunden Gas geben, Weg messen. Ohne
 // diesen Test waere "der Wagen faehrt" eine Behauptung.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergFahrtest"))) {
  // Erst als Figur neben den Wagen treten - dort muss "E Einsteigen"
  // erscheinen - dann ueber denselben Weg einsteigen wie mit der Taste.
  FTimerHandle Hin;
  GetWorldTimerManager().SetTimer(Hin,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It&&Figur;++It) {
    const FVector Ziel=It->GetActorLocation()-It->GetActorForwardVector()*450.0f-It->GetActorRightVector()*250.0f+FVector(0,0,40);
    Figur->SetActorLocation(Ziel,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((It->GetActorLocation()-Ziel).Rotation()+FRotator(-8,0,0));
    break;
   }
  },4.6f,false);
  FTimerHandle Hinweis;
  GetWorldTimerManager().SetTimer(Hinweis,[this]() {
   FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir()/TEXT("Fahrtest_Hinweis.png"),true,false);
  },5.4f,false);
  // Mitten in der Fahrt die GPU-Zeiten je Renderschritt ins Log - daran
  // sieht man, was ein Bild kostet. Nur im Editor-Spiel, nicht in Shipping.
  if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergGpu"))) {
   FTimerHandle Gpu;
   GetWorldTimerManager().SetTimer(Gpu,[this]() {
    if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("ProfileGPU"));
   },7.6f,false);
  }
  FTimerHandle Tacho;
  GetWorldTimerManager().SetTimer(Tacho,[this]() {
   FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir()/TEXT("Fahrtest_Tacho.png"),true,false);
  },8.6f,false);
  FTimerHandle Start;
  GetWorldTimerManager().SetTimer(Start,[this]() {
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) {
    auto* PC=GetWorld()->GetFirstPlayerController();
    auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
    if(Figur) Figur->Einsteigen();
    bFahrtestEinstieg = Figur && PC && PC->GetPawn()==*It;
    Beleg(FString::Printf(TEXT("LALABERG_EINSTIEG_TEST %s"),bFahrtestEinstieg?TEXT("PASS"):TEXT("FAIL")));
    if (!bFahrtestEinstieg) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
    It->TestSteuerung(1.0f,0.0f);
    FahrtStart=It->GetActorLocation();
    FahrtBilder=GFrameCounter; FahrtZeit=FPlatformTime::Seconds(); FahrtSchlechteste=1000.0f;
    Beleg(FString::Printf(TEXT("LALABERG_FAHRTEST start %s nach %.1fs Programmlaufzeit"),*FahrtStart.ToString(),FPlatformTime::Seconds()-GStartTime));
    break;
   }
  },6.0f,false);
  // Halbsekundlich Tempo, Weg und Radkontakt - daran sieht man, ob der Wagen
  // gegen etwas faehrt, abhebt oder nur schwach beschleunigt.
  GetWorldTimerManager().SetTimer(FahrtUhr,[this]() {
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) {
    // Bildrate je halbe Sekunde; die schlechteste zaehlt fuer das Ergebnis.
    const uint64 Bilder=GFrameCounter; const double Uhr=FPlatformTime::Seconds();
    const float Fps=FahrtLetzteUhr>0?(Bilder-FahrtLetzteBilder)/(Uhr-FahrtLetzteUhr):0.0f;
    if(FahrtLetzteUhr>0) FahrtSchlechteste=FMath::Min(FahrtSchlechteste,Fps);
    FahrtLetzteBilder=Bilder; FahrtLetzteUhr=Uhr;
    Beleg(FString::Printf(TEXT("LALABERG_FAHRT tempo=%.1fkmh weg=%.1fm raeder=%d z=%.0f fps=%.0f"),
     It->GetVelocity().Size()*0.036f,FVector::Dist2D(It->GetActorLocation(),FahrtStart)/100.0f,
     It->RaederAmBoden(),It->GetActorLocation().Z,Fps));
    break;
   }
  },0.5f,true,6.5f);
  // Kurz vor Ende ein Bild aus der Wagenkamera: Beleg fuer die Fahrt und
  // zugleich ein Blick auf die Fassaden am Klinikum.
  FTimerHandle Bild;
  GetWorldTimerManager().SetTimer(Bild,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },9.2f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) {
    const FVector Jetzt=It->GetActorLocation();
    const float Weg=FVector::Dist2D(Jetzt,FahrtStart);
    const float Tempo=It->GetVelocity().Size()*0.036f;   // cm/s in km/h
    const bool bAufraedern=FVector::DotProduct(It->GetActorUpVector(),FVector::UpVector)>0.7f;
    // Weg ODER Tempo: die Strecke in diesen 4,5 Sekunden haengt an der
    // Bildrate (im Editor 25 fps und 12 m, im gepackten Spiel 13 fps und
    // 6 m bei gleicher Physik). Beide Zahlen stehen im Beleg.
    bFahrtestBestanden = bFahrtestEinstieg && (Weg>800.0f || Tempo>25.0f) && bAufraedern && It->RaederAmBoden()>=2;
    It->TestAnhalten();
    const float Mittel=(GFrameCounter-FahrtBilder)/FMath::Max(0.001,FPlatformTime::Seconds()-FahrtZeit);
    Beleg(FString::Printf(TEXT("LALABERG_FAHRTEST %s weg=%.1fm tempo=%.0fkmh aufraedern=%d fps_mittel=%.0f fps_schlechteste=%.0f aufloesung=%s"),
     bFahrtestBestanden?TEXT("PASS"):TEXT("FAIL"),Weg/100.0f,Tempo,bAufraedern?1:0,Mittel,FahrtSchlechteste,
     GEngine&&GEngine->GameViewport?*FString::Printf(TEXT("%dx%d"),GEngine->GameViewport->Viewport->GetSizeXY().X,GEngine->GameViewport->Viewport->GetSizeXY().Y):TEXT("?")));
    break;
   }
   // Erst anhalten, dann aussteigen. ALaLaBergWagen::Aussteigen verweigert
   // den Ausstieg oberhalb von 1 m/s ("zuerst mit der Leertaste anhalten") -
   // seit der Wagen durch die Antriebskraft-Korrektur wirklich faehrt, lief
   // der Test genau in diese Sicherung: er stieg stur 5 s nach dem Bremsen
   // aus, da waren noch 32 km/h drauf, und AUSSTIEG/KRANKENHAUS schlugen
   // fehl. Jetzt wird auf den Stillstand gewartet statt auf die Uhr, und die
   // gebrauchte Bremszeit steht im Beleg - daran sieht man zugleich, ob die
   // Bremse ueberhaupt wirkt.
   const double BremsStart=GetWorld()->GetTimeSeconds();
   FVector BremsOrt=FVector::ZeroVector;
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) { BremsOrt=It->GetActorLocation(); break; }
   GetWorldTimerManager().SetTimer(BremsUhr,[this,BremsStart,BremsOrt]() {
    for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) {
     const float Tempo=It->GetVelocity().Size()*0.036f;
     // Alles, was nach dem ClearTimer noch gebraucht wird, zuerst auf den
     // Stapel - auch this: ClearTimer gibt die Timerdaten frei, und darin
     // liegt diese Lambda mitsamt ihren Kopien. Im Editor blieb der Speicher
     // gueltig, im gepackten Spiel stuerzte der Fahrtest beim ersten Zugriff
     // auf ein Feld der Spielart ab (0xffff..., 2026-09-22).
     ALaLaBergGameMode* Selbst=this;
     const double Start=BremsStart;
     const FVector Ort=BremsOrt;
     const FVector Jetzt=It->GetActorLocation();
     ALaLaBergWagen* Wagen=*It;
     const double Gebraucht=GetWorld()->GetTimeSeconds()-Start;
     if(Tempo>3.0f && Gebraucht<12.0) return;               // weiter bremsen
     Selbst->GetWorldTimerManager().ClearTimer(Selbst->BremsUhr);
     const bool bBremsPass=Tempo<=3.0f;
     Selbst->bFahrtestBestanden=Selbst->bFahrtestBestanden && bBremsPass;
     Beleg(FString::Printf(TEXT("LALABERG_BREMS_TEST %s tempo=%.2f nach=%.2fs_spiel bremsweg=%.2fm"),
      bBremsPass?TEXT("PASS"):TEXT("FAIL"),Tempo,Gebraucht,FVector::Dist2D(Jetzt,Ort)/100.0f));
     if(Wagen) Wagen->TestAussteigen();
     FScreenshotRequest::RequestScreenshot(FPaths::ScreenShotDir()/TEXT("Fahrtest_Ausstieg.png"),true,false);
     FTimerHandle Abschluss;
     Selbst->GetWorldTimerManager().SetTimer(Abschluss,[Selbst]() {
      auto* PC=Selbst->GetWorld()->GetFirstPlayerController();
      auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
      const bool bAusgestiegen=Figur && !Figur->IsHidden() && Figur->GetActorEnableCollision();
      Beleg(FString::Printf(TEXT("LALABERG_AUSSTIEG_TEST %s"),bAusgestiegen?TEXT("PASS"):TEXT("FAIL")));
      const bool bPass=Selbst->bFahrtestBestanden && bAusgestiegen;
      Beleg(FString::Printf(TEXT("LALABERG_KRANKENHAUS_TEST %s"),bPass?TEXT("PASS"):TEXT("FAIL")));
      FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
     },1.5f,false);
     return;
    }
   },0.5f,true,0.5f);
  },10.5f,false);
 }
 // Waffentest: Figur vor den fahrbaren Wagen stellen (er ist die einzige
 // eigenstaendige Figur, die einen Treffer auch sichtbar aendert), auf ihn
 // zielen und mit jeder Waffenart einmal feuern. PASS ab einem gezaehlten
 // Treffer.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergWaffentest"))) {
  FTimerHandle Hin;
  GetWorldTimerManager().SetTimer(Hin,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It&&Figur;++It) {
    const FVector Ziel=It->GetActorLocation()-It->GetActorForwardVector()*550.0f+FVector(0,0,40);
    Figur->SetActorLocation(Ziel,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((It->GetActorLocation()-Ziel).Rotation());
    break;
   }
  },4.6f,false);
  const TArray<ELaLaBergWaffenArt> Arten={ELaLaBergWaffenArt::Pistole,ELaLaBergWaffenArt::Maschine,
   ELaLaBergWaffenArt::Schrotflinte,ELaLaBergWaffenArt::Raketenwerfer};
  for(int32 i=0;i<Arten.Num();i++) {
   FTimerHandle Ausruestung;
   GetWorldTimerManager().SetTimer(Ausruestung,[this,Art=Arten[i]]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
    if(Figur&&Figur->HoleWaffe()) Figur->HoleWaffe()->SetzeArt(Art);
   },5.2f+i*1.0f,false);
   // Eigenes Belegbild je Waffenart VOR dem Schuss: sonst zeigt nur die
   // zuletzt ausgeruestete Waffe (der Werfer) ihr Ansichtsmodell, die
   // anderen drei blieben unbelegt - genau die Luecke, die den falsch
   // gedrehten Werfer zunaechst unbemerkt liess.
   FTimerHandle Bild;
   GetWorldTimerManager().SetTimer(Bild,[this]() {
    if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 800x450"));
   },5.2f+i*1.0f+0.4f,false);
   FTimerHandle Schuss;
   GetWorldTimerManager().SetTimer(Schuss,[this]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
    if(Figur) Figur->Feuern();
   },5.2f+i*1.0f+0.6f,false);
  }
  FTimerHandle Bild;
  GetWorldTimerManager().SetTimer(Bild,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },9.6f,false);
  // Dicht vor eine Hauswand treten und einmal schiessen: der Klecks selbst
  // ist auf dem umlackierten Wagen kaum zu sehen, weil jeder Treffer den
  // ganzen Wagen neu einfaerbt.
  FTimerHandle Wand;
  GetWorldTimerManager().SetTimer(Wand,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   if(!Figur) return;
   // Der Wagen zaehlt hier nicht als Wand: er faerbt sich selbst um, und ein
   // Treffer auf ihn zeigt keinen bleibenden Klecks.
   FHitResult Treffer; FCollisionQueryParams Params; Params.AddIgnoredActor(Figur);
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It;++It) Params.AddIgnoredActor(*It);
   const FVector Start=Figur->GetActorLocation();
   for(const FVector& Richtung:{FVector(1,0,0),FVector(-1,0,0),FVector(0,1,0),FVector(0,-1,0)}) {
    if(GetWorld()->LineTraceSingleByChannel(Treffer,Start,Start+Richtung*3000.0f,ECC_Visibility,Params)) {
     const FVector Ziel=Treffer.ImpactPoint-Richtung*260.0f;
     Figur->SetActorLocation(Ziel,false,nullptr,ETeleportType::TeleportPhysics);
     PC->SetControlRotation(Richtung.Rotation());
     if(auto* Anzeige=Cast<ALaLaBergHUD>(PC->GetHUD())) Anzeige->OrtSofort();
     break;
    }
   }
  },10.6f,false);
  FTimerHandle SchussWand;
  GetWorldTimerManager().SetTimer(SchussWand,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   if(Figur&&Figur->HoleWaffe()) { Figur->HoleWaffe()->SetzeArt(ELaLaBergWaffenArt::Pistole); Figur->Feuern(); }
  },11.0f,false);
  FTimerHandle BildWand;
  GetWorldTimerManager().SetTimer(BildWand,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },11.6f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   const bool bPass=HoleFarbtreffer()>0;
   Beleg(FString::Printf(TEXT("LALABERG_WAFFENTEST %s treffer=%d"),bPass?TEXT("PASS"):TEXT("FAIL"),HoleFarbtreffer()));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },12.6f,false);
 }
 // Verkehrsfoto: zum ersten KI-Auto und zum ersten KI-Passanten teleportieren
 // und je ein Bild machen, nachdem sie sich eine Weile bewegt haben konnten -
 // ein Bild an einem zufaelligen Standpunkt zeigt sie sonst so gut wie nie.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergVerkehrFoto"))) {
  FTimerHandle ZumAuto;
  GetWorldTimerManager().SetTimer(ZumAuto,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Pawn=PC?PC->GetPawn():nullptr;
   for(TActorIterator<ALaLaBergVerkehrsauto> It(GetWorld());It&&Pawn;++It) {
    const FVector Ort=It->GetActorLocation()-It->GetActorForwardVector()*900.0f+FVector(0,0,250);
    Pawn->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((It->GetActorLocation()-Ort).Rotation());
    if(auto* Anzeige=Cast<ALaLaBergHUD>(PC->GetHUD())) Anzeige->OrtSofort();
    Beleg(FString::Printf(TEXT("LALABERG_VERKEHR_AUTO bei %s"),*It->GetActorLocation().ToString()));
    break;
   }
  },3.4f,false);
  FTimerHandle BildAuto;
  GetWorldTimerManager().SetTimer(BildAuto,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },5.4f,false);
  FTimerHandle ZumPassant;
  GetWorldTimerManager().SetTimer(ZumPassant,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Pawn=PC?PC->GetPawn():nullptr;
   for(TActorIterator<ALaLaBergPassantKI> It(GetWorld());It&&Pawn;++It) {
    // Von der Seite statt von hinten: nur im Profil ist erkennbar, ob der
    // Passant in Laufrichtung schaut oder seitwaerts geht - von hinten
    // sieht beides fast gleich aus.
    const FVector Ort=It->GetActorLocation()+It->GetActorRightVector()*260.0f+FVector(0,0,60);
    Pawn->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
    // Die eigene Figur stand zwischen Kamera und Passant und verdeckte ihn -
    // fuer dieses Bild ausblenden, samt der an ihr haengenden Waffe.
    Pawn->SetActorHiddenInGame(true);
    if(auto* Held=Cast<ALaLaBergCharacter>(Pawn)) if(Held->HoleWaffe()) Held->HoleWaffe()->SetActorHiddenInGame(true);
    PC->SetControlRotation((It->GetActorLocation()-Ort).Rotation());
    if(auto* Anzeige=Cast<ALaLaBergHUD>(PC->GetHUD())) Anzeige->OrtSofort();
    Beleg(FString::Printf(TEXT("LALABERG_VERKEHR_PASSANT bei %s"),*It->GetActorLocation().ToString()));
    break;
   }
  },6.4f,false);
  FTimerHandle BildPassant;
  GetWorldTimerManager().SetTimer(BildPassant,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },7.0f,false);   // kurz nach dem Hinstellen, bevor der Passant aus dem Bild laeuft
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   // Stehen die Figuren auf dem Boden? Unter der Sohle (tiefste sichtbare
   // Stelle, siehe HoleSohleZ) darf keine Flaeche mehr liegen - sonst steckt
   // die Figur darin, wie zuletzt bis zur Huefte in der Wiese.
   int32 Geprueft=0,Versenkt=0; float Tiefste=0.0f;
   for(ALaLaBergPassantKI* P:ALaLaBergPassantKI::Alle) {
    if(!P||Geprueft>=200) break;
    const FVector Ort=P->GetActorLocation();
    FHitResult Boden; FCollisionQueryParams Params; Params.AddIgnoredActor(P);
    if(!GetWorld()->LineTraceSingleByChannel(Boden,Ort+FVector(0,0,300),Ort-FVector(0,0,300),ECC_Visibility,Params)) continue;
    Geprueft++;
    const float Tief=Boden.ImpactPoint.Z-P->HoleSohleZ();   // > 0: Boden ueber den Sohlen
    // Toleranz 25 cm: die Mesh-Grenzen sind etwas grosszuegiger als die Figur.
    if(Tief>25.0f) { Versenkt++; Tiefste=FMath::Max(Tiefste,Tief); }
    if(Geprueft<=3) UE_LOG(LogTemp,Display,TEXT("LALABERG_PASSANT_PROBE aktor=%.0f sohle=%.0f boden=%.0f"),Ort.Z,P->HoleSohleZ(),Boden.ImpactPoint.Z);
   }
   const bool bPass=Versenkt==0;
   Beleg(FString::Printf(TEXT("LALABERG_VERKEHRFOTO %s autos=%d passanten=%d boden_geprueft=%d versenkt=%d tiefste=%.0fcm"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),AutoZahl,PassantZahl,Geprueft,Versenkt,Tiefste));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },9.4f,false);
 }
 // Prueft ueber eine volle Zyklusdauer, ob zwei Ampeln derselben Kreuzung
 // (gleiche Gruppe, verschiedene Phase - siehe LaLaBergAmpel::SetzeGruppe)
 // je gleichzeitig Gruen zeigen. Ohne diesen Test waere "kreuzende Strassen
 // haben nie gleichzeitig Gruen" nur eine Behauptung ueber den Code, der die
 // Zeitrechnung dafuer aufstellt, nicht ueber das tatsaechliche Verhalten.
 // Der Kopf der Mannschaft: markieren ruft ihn, stellen gibt das Revier
 // samt Fundstueck. Geprueft wird die ganze Kette bis zum verschlossenen
 // Hubschrauber, der sich danach oeffnet.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergKopfTest"))) {
  static bool bKopfLaeuft=false, bFundVorher=false, bFundNachher=false;
  static int32 Reviere=0, Kapitel=0;
  FTimerHandle Markieren;
  GetWorldTimerManager().SetTimer(Markieren,[this]() {
   auto* R=ALaLaBergRevier::Instanz.Get();
   auto* Konto=ULaLaBergKonto::Hole(this);
   if(!R||!Konto) return;
   bFundVorher=Konto->HatFund(ULaLaBergKonto::EFund::Werkstatt);
   R->TestMarkiereAlle();            // ruft den Kopf (Revier 0 braucht keinen Ruf)
   bKopfLaeuft=R->KopfLaeuft();
  },4.0f,false);
  FTimerHandle Stellen;
  GetWorldTimerManager().SetTimer(Stellen,[this]() {
   if(auto* R=ALaLaBergRevier::Instanz.Get()) R->TestStelleKopf();
  },5.0f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   if(auto* Konto=ULaLaBergKonto::Hole(this)) {
    bFundNachher=Konto->HatFund(ULaLaBergKonto::EFund::Werkstatt);
    Reviere=Konto->HoleReviere();
    Kapitel=Konto->HoleKapitel();
   }
   const bool bPass=bKopfLaeuft&&!bFundVorher&&bFundNachher&&Reviere==1&&Kapitel>=2;
   Beleg(FString::Printf(TEXT("LALABERG_KOPFTEST %s jagd=%d fund=%d->%d reviere=%d kapitel=%d"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),bKopfLaeuft?1:0,bFundVorher?1:0,bFundNachher?1:0,
                         Reviere,Kapitel));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },6.5f,false);
 }
 // Reviere: stehen die Saeulen an den Wahrzeichen, zaehlt ein Treffer, und
 // wechselt das Revier erst bei genug Ruf den Besitzer?
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergRevierTest"))) {
  static int32 Marken=0, OhneRuf=-1, MitRuf=-1, Kapitel=-1, Offen=-1;
  FTimerHandle Erst;
  GetWorldTimerManager().SetTimer(Erst,[this]() {
   auto* R=ALaLaBergRevier::Instanz.Get();
   auto* Konto=ULaLaBergKonto::Hole(this);
   if(!R||!Konto) return;
   Offen=R->HoleOffenes();
   Marken=R->HoleReviere().IsValidIndex(0)?R->HoleReviere()[0].Marken.Num():0;
   // Erst ohne Ruf: markieren zaehlt, danach muss noch der Kopf der
   // Mannschaft gestellt werden - erst dann wechselt das Revier.
   R->TestMarkiereAlle();
   R->TestStelleKopf();
   OhneRuf=Konto->HoleReviere();
   // Das erste Revier verlangt keinen Ruf (0 * 150) - fuer den Test wird
   // deshalb gleich das zweite geprueft: Ruf geben und weitermachen.
   Konto->Uebe(ULaLaBergKonto::EWert::Ruf,200.0f);
   R->TestMarkiereAlle();
   R->TestStelleKopf();
   MitRuf=Konto->HoleReviere();
   Kapitel=Konto->HoleKapitel();
   // Blick auf die erste Saeule des naechsten Reviers.
   auto* PC=GetWorld()->GetFirstPlayerController();
   const int32 Jetzt=R->HoleOffenes();
   if(PC&&PC->GetPawn()&&R->HoleReviere().IsValidIndex(Jetzt)&&R->HoleReviere()[Jetzt].Marken.Num()) {
    const FVector Ziel=R->HoleReviere()[Jetzt].Marken[0].Ort;
    const FVector Ort=Ziel+FVector(4200,4200,2600);
    PC->GetPawn()->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((Ziel+FVector(0,0,1500)-Ort).Rotation());
    if(auto* Anzeige=Cast<ALaLaBergHUD>(PC->GetHUD())) Anzeige->OrtSofort();
   }
  },4.0f,false);
  FTimerHandle Bild;
  GetWorldTimerManager().SetTimer(Bild,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },5.0f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   // Revier 0 faellt ohne Ruf (verlangt 0), Revier 1 erst mit Ruf 150.
   const bool bPass=Marken==4&&Offen==0&&OhneRuf==1&&MitRuf==3&&Kapitel>=3;
   Beleg(FString::Printf(TEXT("LALABERG_REVIERTEST %s marken=%d offen=%d ohne_ruf=%d mit_ruf=%d kapitel=%d"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),Marken,Offen,OhneRuf,MitRuf,Kapitel));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },6.0f,false);
 }
 // Charakterwerte: waechst der Wert durch Uebung, stimmt die Stufe, wirkt
 // die Ausdauer aufs Lauftempo, und ueberlebt alles einen Speicherlauf?
 // Dazu ein Bild der Vollkarte mit dem Figurblatt.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergFigurTest"))) {
  static int32 StufeVorher=0, StufeNachher=0, WertPlatte=-1;
  static float TempoVorher=0.0f, TempoNachher=0.0f;
  FTimerHandle Ueben;
  GetWorldTimerManager().SetTimer(Ueben,[this]() {
   auto* Konto=ULaLaBergKonto::Hole(this);
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   if(!Konto||!Figur) return;
   StufeVorher=Konto->Stufe(ULaLaBergKonto::EWert::Ausdauer);
   TempoVorher=Figur->GetCharacterMovement()->MaxWalkSpeed;
   // 620 Punkte: das sind vier Stufen, genug fuer einen sichtbaren Sprung.
   Konto->Uebe(ULaLaBergKonto::EWert::Ausdauer,620.0f);
   Konto->Uebe(ULaLaBergKonto::EWert::Zielsicherheit,210.0f);
   Konto->Uebe(ULaLaBergKonto::EWert::Ruf,410.0f);
   Konto->NimmRevier(0);
   Konto->NimmRevier(2);
   Konto->Speichere();
   StufeNachher=Konto->Stufe(ULaLaBergKonto::EWert::Ausdauer);
   if(auto* Platte=Konto->LadeVonPlatte()) WertPlatte=Platte->Ausdauer;
   if(auto* HUD=Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeVollkarte(true);
  },4.0f,false);
  FTimerHandle Bild;
  GetWorldTimerManager().SetTimer(Bild,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr)
    TempoNachher=Figur->GetCharacterMovement()->MaxWalkSpeed;
   if(PC) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },5.0f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   const bool bPass=StufeVorher==1&&StufeNachher==4&&WertPlatte==620&&TempoNachher>TempoVorher*1.15f;
   Beleg(FString::Printf(TEXT("LALABERG_FIGURTEST %s stufe=%d->%d platte=%d tempo=%.0f->%.0f"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),StufeVorher,StufeNachher,WertPlatte,TempoVorher,TempoNachher));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },6.0f,false);
 }
 // Rennen und Verfolgung: derselbe Ablauf wie beim Taxi - zum Wagen,
 // einsteigen, an der blauen Saeule annehmen -, danach je nach Art die
 // Kontrollpunkte abfahren oder den fluechtenden Wagen stellen.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergRennTest"))||FParse::Param(FCommandLine::Get(),TEXT("LaLaBergJagdTest"))) {
  const bool bJagd=FParse::Param(FCommandLine::Get(),TEXT("LaLaBergJagdTest"));
  static int32 Punkte=0, Gewonnen=0, Gestellt=0, Geld=0; static bool bArt=false;
  FTimerHandle ZumWagen;
  GetWorldTimerManager().SetTimer(ZumWagen,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It&&Figur;++It) {
    Figur->SetActorLocation(It->GetActorLocation()-It->GetActorRightVector()*250.0f+FVector(0,0,60),
                            false,nullptr,ETeleportType::TeleportPhysics);
    break;
   }
  },4.0f,false);
  FTimerHandle Ein;
  GetWorldTimerManager().SetTimer(Ein,[this,bJagd]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen();
   if(auto* A=ALaLaBergAuftraege::Instanz.Get()) {
    A->TestErzwinge(bJagd?ELaLaBergAuftragsart::Verfolgung:ELaLaBergAuftragsart::Rennen);
    if(APawn* Wagen=PC?PC->GetPawn():nullptr)
     Wagen->SetActorLocation(A->HoleWegpunkt()+FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
   }
  },4.5f,false);
  // Rennen: alle 0,6 s zum naechsten Kontrollpunkt springen. Jagd: den
  // fluechtenden Wagen mit Farbe stellen.
  FTimerHandle Fahren;
  GetWorldTimerManager().SetTimer(Fahren,[this,bJagd]() {
   auto* A=ALaLaBergAuftraege::Instanz.Get();
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(!A||!A->IstUnterwegs()||!PC||!PC->GetPawn()) return;
   bArt=A->HoleArt()==(bJagd?ELaLaBergAuftragsart::Verfolgung:ELaLaBergAuftragsart::Rennen);
   if(bJagd) {
    if(auto* Beute=A->TestHoleBeute()) Beute->Verletze(40.0f,FVector(1,0,0),ELaLaBergSchaden::Beschuss);
   } else {
    Punkte=FMath::Max(Punkte,A->HolePunkt());
    PC->GetPawn()->SetActorLocation(A->HoleWegpunkt()+FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
   }
  },0.6f,true,5.2f);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this,bJagd]() {
   if(auto* A=ALaLaBergAuftraege::Instanz.Get()) {
    Gewonnen=A->HoleRennen(); Gestellt=A->HoleVerfolgungen(); Geld=A->HoleGeld();
   }
   const bool bPass=bArt&&Geld>0&&(bJagd?Gestellt==1:(Gewonnen==1&&Punkte>=2));
   Beleg(FString::Printf(TEXT("LALABERG_%s %s art=%d punkte=%d gewonnen=%d gestellt=%d geld=%d"),
                         bJagd?TEXT("JAGDTEST"):TEXT("RENNTEST"),bPass?TEXT("PASS"):TEXT("FAIL"),
                         bArt?1:0,Punkte,Gewonnen,Gestellt,Geld));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },12.0f,false);
 }
 // Taxi: einsteigen, an der blauen Saeule halten, Fahrgast aufnehmen, zum
 // Ziel fahren. Geprueft wird, dass die Art wirklich Taxi ist, dass die
 // Fahrt zaehlt und dass Lohn samt Trinkgeld auf dem Konto landet.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergTaxiTest"))) {
  static int32 GeldVorher=0, GeldNachher=0, Fahrten=0; static bool bArtTaxi=false;
  // Erst zum Wagen: Einsteigen greift nur in neun Metern Umkreis, und am
  // Startpunkt steht die Figur nicht neben ihm.
  FTimerHandle ZumWagen;
  GetWorldTimerManager().SetTimer(ZumWagen,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It&&Figur;++It) {
    Figur->SetActorLocation(It->GetActorLocation()-It->GetActorRightVector()*250.0f+FVector(0,0,60),
                            false,nullptr,ETeleportType::TeleportPhysics);
    break;
   }
  },4.0f,false);
  FTimerHandle Ein;
  GetWorldTimerManager().SetTimer(Ein,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen();
   if(auto* A=ALaLaBergAuftraege::Instanz.Get()) {
    A->TestErzwingeTaxi();
    GeldVorher=A->HoleGeld();
    // Mit dem Wagen auf die blaue Saeule stellen - das nimmt den Auftrag an.
    if(APawn* Wagen=PC?PC->GetPawn():nullptr)
     Wagen->SetActorLocation(A->HoleZielOrt()+FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
   }
  },4.5f,false);
  FTimerHandle Hin;
  GetWorldTimerManager().SetTimer(Hin,[this]() {
   auto* A=ALaLaBergAuftraege::Instanz.Get();
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(!A||!PC||!PC->GetPawn()) return;
   bArtTaxi=A->HoleArt()==ELaLaBergAuftragsart::Taxi&&A->IstUnterwegs();
   PC->GetPawn()->SetActorLocation(A->HoleZielOrt()+FVector(0,0,120),false,nullptr,ETeleportType::TeleportPhysics);
  },6.0f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   if(auto* A=ALaLaBergAuftraege::Instanz.Get()) { GeldNachher=A->HoleGeld(); Fahrten=A->HoleTaxifahrten(); }
   const bool bPass=bArtTaxi&&Fahrten==1&&GeldNachher>GeldVorher;
   Beleg(FString::Printf(TEXT("LALABERG_TAXITEST %s art_taxi=%d fahrten=%d geld=%d->%d"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),bArtTaxi?1:0,Fahrten,GeldVorher,GeldNachher));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },7.5f,false);
 }
 // Schaden: Lebenspunkte bei Passanten, Spieler und Fahrzeugen. Geprueft
 // wird die Rechnung (halber Schaden, halbes Leben), der Zusammenbruch mit
 // Aufwachen im Klinikum und der Weg, den die Panzerkanone nimmt: Einschlag,
 // Wirkung im Umkreis, umgefallener Passant.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSchadenTest"))) {
  static float PassantAnteil=-1.0f, SpielerLeben=-1.0f, SpielerNachKlinik=-1.0f;
  static int32 Besuche=-1; static float KlinikAbstand=-1.0f;
  static bool bPassantUmgefallen=false, bAutoAus=false;
  FTimerHandle Rechnung;
  GetWorldTimerManager().SetTimer(Rechnung,[this]() {
   // Passant: 50 Punkte Schaden lassen die Haelfte uebrig.
   for(ALaLaBergPassantKI* P:ALaLaBergPassantKI::Alle) {
    if(!P) continue;
    P->Verletze(50.0f,FVector(1,0,0),ELaLaBergSchaden::Beschuss);
    PassantAnteil=P->Lebensanteil();
    P->Verletze(60.0f,FVector(1,0,0),ELaLaBergSchaden::Anprall);
    bPassantUmgefallen=P->IstAusgeschaltet();
    break;
   }
   // Verkehrsauto: ein Treffer mit voller Wucht schaltet es ab.
   for(ALaLaBergVerkehrsauto* A:ALaLaBergVerkehrsauto::Alle) {
    if(!A||A->IstGeparkt()) continue;
    A->Verletze(130.0f,FVector(1,0,0),ELaLaBergSchaden::Sprengung);
    bAutoAus=A->IstAusgeschaltet();
    break;
   }
   // Spieler: 40 Punkte, dann der Rest - danach muss er im Klinikum stehen.
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) {
    Figur->Verletze(40.0f,FVector(1,0,0),ELaLaBergSchaden::Beschuss);
    SpielerLeben=Figur->HoleLeben();
    Figur->Verletze(70.0f,FVector(1,0,0),ELaLaBergSchaden::Beschuss);
    SpielerNachKlinik=Figur->HoleLeben();
    Besuche=Figur->HoleKrankenhausbesuche();
    KlinikAbstand=FVector::Dist2D(Figur->GetActorLocation(),FVector(-159000.0f,15600.0f,0.0f))/100.0f;
   }
  },4.0f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   const bool bPass=FMath::IsNearlyEqual(PassantAnteil,0.5f,0.02f)&&bPassantUmgefallen&&bAutoAus
                   &&FMath::IsNearlyEqual(SpielerLeben,60.0f,0.5f)&&Besuche==1
                   &&FMath::IsNearlyEqual(SpielerNachKlinik,100.0f,0.5f)&&KlinikAbstand<120.0f;
   Beleg(FString::Printf(TEXT("LALABERG_SCHADENTEST %s passant=%.2f umgefallen=%d auto_aus=%d spieler=%.0f klinik=%d leben_danach=%.0f abstand=%.0fm"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),PassantAnteil,bPassantUmgefallen?1:0,bAutoAus?1:0,
                         SpielerLeben,Besuche,SpielerNachKlinik,KlinikAbstand));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },6.0f,false);
 }
 // Panzer und Hubschrauber: hinstellen, einsteigen, ein Stueck fahren bzw.
 // steigen - und je ein Bild, damit man sieht, was da im Wald steht und auf
 // dem Klinikumsdach wartet.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSonderTest"))) {
  static float PanzerWeg=-1.0f, HeliHoehe=-1.0f;
  static int32 PanzerSchuesse=0;
  static FVector PanzerStart=FVector::ZeroVector, HeliStart=FVector::ZeroVector;
  auto Hin=[this](ELaLaBergSonderart Art,float Hinter,float Hoch) {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   if(!Figur) return;
   for(TActorIterator<ALaLaBergSonderfahrzeug> It(GetWorld());It;++It) {
    if(It->HoleArt()!=Art) continue;
    const FVector Ort=It->GetActorLocation()-It->GetActorForwardVector()*Hinter+FVector(0,0,Hoch);
    Figur->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((It->GetActorLocation()-Ort).Rotation());
    if(auto* Anzeige=Cast<ALaLaBergHUD>(PC->GetHUD())) Anzeige->OrtSofort();
    break;
   }
  };
  auto Bild=[this](float Wann) {
   FTimerHandle H;
   GetWorldTimerManager().SetTimer(H,[this]() {
    if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
   },Wann,false);
  };
  FTimerHandle ZumPanzer;
  GetWorldTimerManager().SetTimer(ZumPanzer,[this,Hin]() { Hin(ELaLaBergSonderart::Panzer,700.0f,150.0f); },4.0f,false);
  Bild(4.6f);
  FTimerHandle PanzerRein;
  GetWorldTimerManager().SetTimer(PanzerRein,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen();
   if(auto* Sonder=PC?Cast<ALaLaBergSonderfahrzeug>(PC->GetPawn()):nullptr) {
    PanzerStart=Sonder->GetActorLocation();
    Sonder->TestSteuerung(1.0f,0.0f,0.0f);
   }
  },5.0f,false);
  // Kanone: zweimal feuern, das zweite Mal muss die Ladezeit abwarten.
  FTimerHandle Schuss;
  GetWorldTimerManager().SetTimer(Schuss,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Sonder=PC?Cast<ALaLaBergSonderfahrzeug>(PC->GetPawn()):nullptr) {
    Sonder->TestFeuern();
    Sonder->TestFeuern();                     // zu frueh: darf nicht zaehlen
    PanzerSchuesse=Sonder->HoleSchuesse();
   }
  },7.6f,false);
  // Erst gut anderthalb Sekunden spaeter fotografieren: das Geschoss
  // braucht die Zeit bis zum Einschlag, vorher sieht man keinen Klecks.
  Bild(9.2f);
  FTimerHandle PanzerAus;
  GetWorldTimerManager().SetTimer(PanzerAus,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Sonder=PC?Cast<ALaLaBergSonderfahrzeug>(PC->GetPawn()):nullptr) {
    PanzerWeg=FVector::Dist2D(PanzerStart,Sonder->GetActorLocation())/100.0f;
    Sonder->TestSteuerung(0.0f,0.0f,0.0f);
    Beleg(FString::Printf(TEXT("LALABERG_SONDER panzer weg=%.0fm tempo=%.0fkmh schuesse=%d"),
                          PanzerWeg,Sonder->TempoKmh(),PanzerSchuesse));
   }
  },9.0f,false);
  Bild(9.2f);
  FTimerHandle ZumHeli;
  GetWorldTimerManager().SetTimer(ZumHeli,[this,Hin]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   // Erst aussteigen: die Figur sitzt noch im Panzer, und solange sie das
   // tut, ist der Pawn kein Character mehr - das Hinstellen liefe ins Leere.
   if(auto* Sonder=PC?Cast<ALaLaBergSonderfahrzeug>(PC->GetPawn()):nullptr) {
    Sonder->TestSteuerung(0.0f,0.0f,0.0f);
    Sonder->TestAussteigen();
   }
  },10.0f,false);
  FTimerHandle ZumHeli2;
    // Nicht weiter als 5 m weg hinstellen: einsteigen geht nur bis 9 m, und
  // aus 9 m plus Hoehe war die Figur schon zu weit entfernt.
  GetWorldTimerManager().SetTimer(ZumHeli2,[this,Hin]() { Hin(ELaLaBergSonderart::Hubschrauber,500.0f,120.0f); },10.4f,false);
  Bild(11.0f);
  FTimerHandle HeliRein;
  GetWorldTimerManager().SetTimer(HeliRein,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen();
   if(auto* Sonder=PC?Cast<ALaLaBergSonderfahrzeug>(PC->GetPawn()):nullptr) {
    HeliStart=Sonder->GetActorLocation();
    Sonder->TestSteuerung(0.4f,0.0f,1.0f);
   }
  },11.4f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Sonder=PC?Cast<ALaLaBergSonderfahrzeug>(PC->GetPawn()):nullptr;
   if(Sonder) HeliHoehe=(Sonder->GetActorLocation().Z-HeliStart.Z)/100.0f;
   if(PC) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
   // Genau ein Schuss: der zweite kam vor Ablauf der Ladezeit.
   const bool bPass=PanzerWeg>8.0f&&HeliHoehe>8.0f&&PanzerSchuesse==1;
   Beleg(FString::Printf(TEXT("LALABERG_SONDERTEST %s panzer_weg=%.0fm heli_stieg=%.0fm schuesse=%d"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),PanzerWeg,HeliHoehe,PanzerSchuesse));
   FTimerHandle Schluss;
   GetWorldTimerManager().SetTimer(Schluss,[bPass]() { FPlatformMisc::RequestExitWithStatus(false,bPass?0:1); },1.2f,false);
  },15.0f,false);
 }
 // Ab drei Sternen sperrt die Polizei die Strasse voraus. Der Test setzt
 // vier Sterne, setzt sich in einen Wagen und faehrt los; danach muss eine
 // Sperre stehen, und zwar voraus auf der Strasse, nicht irgendwo.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSperrTest"))) {
  FTimerHandle Start;
  GetWorldTimerManager().SetTimer(Start,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen();
   if(auto* P=ALaLaBergPolizei::Instanz.Get()) P->TestSetzeSterne(4);
  },4.0f,false);
  FTimerHandle Gas;
  GetWorldTimerManager().SetTimer(Gas,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Wagen=PC?Cast<ALaLaBergWagen>(PC->GetPawn()):nullptr) Wagen->TestSteuerung(1.0f,0.0f);
  },5.0f,true);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   auto* P=ALaLaBergPolizei::Instanz.Get();
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Figur=PC?PC->GetPawn():nullptr;
   const bool bSteht=P&&P->SperreSteht();
   const float Weit=bSteht&&Figur?FVector::Dist2D(Figur->GetActorLocation(),P->HoleSperrOrt())/100.0f:-1.0f;
   if(PC&&bSteht) {
    // Hinsehen: das Bild soll die Sperre zeigen, nicht die Windschutzscheibe.
    // Schraeg von oben, 35 m ueber der Sperre: aus Augenhoehe verdeckt die
    // erste Bake die zweite, und man sieht nicht, ob die Strasse wirklich zu
    // ist.
    // 80 m zurueck und 35 m hoch: naeher als 60 m gilt die Sperre als
    // passiert, dann raeumt die Polizei sie noch vor dem Bild wieder ab.
    const FVector Ort=P->HoleSperrOrt()+FVector(0,0,3500)-(P->HoleSperrOrt()-Figur->GetActorLocation()).GetSafeNormal2D()*8000.0f;
    Figur->SetActorLocation(Ort,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((P->HoleSperrOrt()-Ort).Rotation());
    // Erst im naechsten Bild fotografieren: im selben Tick steht die Kamera
    // noch am alten Platz.
    FTimerHandle Bild;
    GetWorldTimerManager().SetTimer(Bild,[this]() {
     if(auto* PC2=GetWorld()->GetFirstPlayerController()) PC2->ConsoleCommand(TEXT("HighResShot 1600x900"));
    },0.5f,false);
   }
   const bool bPass=bSteht&&Weit>20.0f;
   Beleg(FString::Printf(TEXT("LALABERG_SPERRTEST %s steht=%d abstand=%.0fm"),bPass?TEXT("PASS"):TEXT("FAIL"),bSteht?1:0,Weit));
   FTimerHandle Schluss;
   GetWorldTimerManager().SetTimer(Schluss,[bPass]() { FPlatformMisc::RequestExitWithStatus(false,bPass?0:1); },1.5f,false);
  },16.0f,false);
 }
 // Wer ein KI-Auto uebernimmt, soll in genau diesem Auto sitzen. Vorher
 // wuerfelte der neue Wagen sein Modell selbst, und man sass regelmaessig im
 // roten CarConcept-Flitzer statt in dem Lieferwagen, den man angehalten
 // hatte. Der Test haelt fest, welches Modell das KI-Auto fuhr, und
 // vergleicht es mit dem, in dem man nach dem Einsteigen sitzt.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergUebernahmeTest"))) {
  // Hinstellen und Einsteigen im selben Zug: ein KI-Auto faehrt weiter und
  // waere eine halbe Sekunde spaeter schon ausser Reichweite.
  FTimerHandle Steig;
  GetWorldTimerManager().SetTimer(Steig,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   if(!Figur) return;
   for(ALaLaBergVerkehrsauto* Auto:ALaLaBergVerkehrsauto::Alle) {
    if(!Auto||Auto->IstGeparkt()) continue;
    UebernahmeTyp=Auto->HoleFahrzeugTyp();
    const FVector Ziel=Auto->GetActorLocation()+Auto->GetActorRightVector()*260.0f+FVector(0,0,60);
    Figur->SetActorLocation(Ziel,false,nullptr,ETeleportType::TeleportPhysics);
    PC->SetControlRotation((Auto->GetActorLocation()-Ziel).Rotation());
    Figur->Einsteigen();
    break;
   }
  },4.6f,false);
  FTimerHandle Bild;
  GetWorldTimerManager().SetTimer(Bild,[this]() {
   if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
  },5.6f,false);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Wagen=PC?Cast<ALaLaBergWagen>(PC->GetPawn()):nullptr;
   const int32 Jetzt=Wagen?Wagen->HoleFahrzeugTyp():-99;
   const bool bPass=Wagen&&Jetzt==UebernahmeTyp;
   Beleg(FString::Printf(TEXT("LALABERG_UEBERNAHME %s ki_typ=%d eigener_typ=%d"),
                         bPass?TEXT("PASS"):TEXT("FAIL"),UebernahmeTyp,Jetzt));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },6.4f,false);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAmpelTest"))) {
  auto Pruefe=[this]() {
   int32 Verstoesse=0;
   TMap<int32,TArray<ALaLaBergAmpel*>> Gruppen;
   for(ALaLaBergAmpel* A:ALaLaBergAmpel::Alle) if(A) Gruppen.FindOrAdd(A->HoleGruppe()).Add(A);
   for(const auto& Paar:Gruppen) {
    const TArray<ALaLaBergAmpel*>& Liste=Paar.Value;
    for(int32 i=0;i<Liste.Num();i++) for(int32 j=i+1;j<Liste.Num();j++) {
     if(Liste[i]->HolePhase()==Liste[j]->HolePhase()) continue;
     if(!Liste[i]->HaeltAn() && !Liste[j]->HaeltAn()) Verstoesse++;
    }
   }
   return Verstoesse;
  };
  static int32 GesamtVerstoesse=0;
  FTimerHandle Takt;
  GetWorldTimerManager().SetTimer(Takt,[this,Pruefe]() { GesamtVerstoesse+=Pruefe(); },0.5f,true,2.0f);
  // Mindestens einen vollen Zyklus lang pruefen - bei Kreuzungen mit mehr
  // als zwei Phasen (mehr als vier Armen) dauert der laenger als die alten,
  // fest angenommenen 18s (Zweiphasen-Zyklus plus Vorlauf).
  const float TestDauer=FMath::Max(18.0f,2.0f+ALaLaBergAmpel::PhasenfensterS()*GroessteAnzahlPhasen+2.0f);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   Beleg(FString::Printf(TEXT("LALABERG_AMPELTEST %s verstoesse=%d ampeln=%d"),
    GesamtVerstoesse==0?TEXT("PASS"):TEXT("FAIL"),GesamtVerstoesse,AmpelZahl));
   FPlatformMisc::RequestExitWithStatus(false,0);
  },TestDauer,false);
 }
 // Beweist die Ueberhollogik (siehe ALaLaBergVerkehrsauto::Tick) isoliert von
 // echtem Stadtverkehr: zwei synthetische Autos weit ab jeder echten Strasse
 // (sonst wuerde LadeVerkehr's Stadtverkehr die Kreuzungs-/Ampel-Baelle
 // verfaelschen) - eines kriecht fast im Stillstand, das andere faehrt normal
 // dahinter auf. Organische Beobachtung im echten Verkehr (mehrere 100s Spielzeit
 // ueber mehrere Kalibrierungen) loeste das Ueberholen nie aus - zu selten die
 // richtige Konstellation bei nur 70 verteilten Autos in einer ganzen Stadt.
 // Dieser Test stellt die Konstellation gezielt her, statt auf Zufall zu warten.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergUeberholTest"))) {
  const FVector Start(500000.0f,500000.0f,10000.0f);
  const FVector Achse(1.0f,0.0f,0.0f);
  TArray<FVector> RouteVorne={Start+Achse*700.0f,Start+Achse*30700.0f};
  TArray<FVector> RouteHinten={Start,Start+Achse*30000.0f};
  ALaLaBergVerkehrsauto* Vorne=GetWorld()->SpawnActor<ALaLaBergVerkehrsauto>(RouteVorne[0],FRotator::ZeroRotator);
  ALaLaBergVerkehrsauto* Hinten=GetWorld()->SpawnActor<ALaLaBergVerkehrsauto>(RouteHinten[0],FRotator::ZeroRotator);
  if(Vorne) { Vorne->SetzeRoute(RouteVorne,3.0f); Vorne->SetzeStrassenbreite(7.0f); }
  if(Hinten) { Hinten->SetzeRoute(RouteHinten,30.0f); Hinten->SetzeStrassenbreite(7.0f); }
  // Seit die Autos ihr Tempo wirklich fahren (2026-09-18), ist der
  // Ueberholvorgang nach wenigen Sekunden vorbei - geprueft wird deshalb, ob
  // er stattfand und der hintere Wagen danach vorn liegt, nicht ob er zu
  // einem festen Zeitpunkt gerade laeuft.
  static bool bHatUeberholt=false;
  FTimerHandle Takt;
  GetWorldTimerManager().SetTimer(Takt,[Hinten]() { if(Hinten&&Hinten->IstAmUeberholen()) bHatUeberholt=true; },0.1f,true);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this,Hinten,Vorne]() {
   const float Vorsprung=Hinten&&Vorne?(Hinten->GetActorLocation().X-Vorne->GetActorLocation().X)/100.0f:0.0f;
   const bool bPass=bHatUeberholt&&Vorsprung>5.0f;
   Beleg(FString::Printf(TEXT("LALABERG_UEBERHOLTEST %s ueberholt=%d vorsprung=%.1fm"),bPass?TEXT("PASS"):TEXT("FAIL"),bHatUeberholt?1:0,Vorsprung));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },10.0f,false);
 }
 // Biegen die KI-Autos im laufenden Spiel wirklich ab? Seit die Routen ueber
 // den Strassengraphen mehrere Strassen verketten (siehe Tools/Export/
 // prepare-verkehr.cjs), soll ein Auto an einer Kreuzung die Strasse
 // wechseln, statt nur seine eine Strasse vor und zurueck zu fahren. Zaehlt
 // nur zuegige Richtungsaenderungen (mehr als 4 Grad je Viertelsekunde):
 // eine sanft gekruemmte Strasse dreht das Auto deutlich langsamer als eine
 // Abbiegung, sonst waere jede Kurvenfahrt schon ein "Abbiegen".
 // -LaLaBergAuftragTest: ein ganzer Lieferauftrag ohne Tastatur. Die Figur
 // wird vor die blaue Saeule gestellt (Foto), hineingesetzt (Auftrag laeuft,
 // Foto Richtung Ziel), vor das Ziel (Foto) und hinein (Geld). Dann in die
 // naechste blaue Saeule, die Frist laeuft ab - der Auftrag muss scheitern.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAuftragTest"))) {
  auto Stelle=[this](const FVector& Ort,const FVector& Blick) {
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Figur=PC?PC->GetPawn():nullptr;
   if(!Figur) return;
   Figur->SetActorLocation(Ort+FVector(0,0,110),false,nullptr,ETeleportType::TeleportPhysics);
   PC->SetControlRotation(FRotator(-8.0f,(Blick-Ort).Rotation().Yaw,0));
   Figur->SetActorRotation(FRotator(0,(Blick-Ort).Rotation().Yaw,0));
   if(auto* HUD=Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->OrtSofort();
  };
  auto Foto=[this]() { if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900")); };
  struct FSchritt { float Zeit; TFunction<void()> Tu; };
  TArray<FSchritt> Plan={
   {4.5f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get()) {
      const FVector S=A->HoleWegpunkt(); Stelle(S+FVector(-2500,-1200,0),S); } }},
   {6.0f,Foto},
   {7.0f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get()) {
      const FVector S=A->HoleWegpunkt(); Stelle(S,S+FVector(1000,0,0)); } }},
   {7.6f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get(); A&&A->IstUnterwegs()) {
      const FVector Z=A->HoleWegpunkt(); auto* PC=GetWorld()->GetFirstPlayerController();
      if(PC&&PC->GetPawn()) Stelle(PC->GetPawn()->GetActorLocation()-FVector(0,0,110),Z); } }},
   // Vollkarte mit der Route zum Ziel (M-Taste), dann wieder zu.
   {8.0f,[this]() { if(auto* PC=GetWorld()->GetFirstPlayerController()) if(auto* HUD=Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeVollkarte(true); }},
   {8.5f,Foto},
   {8.8f,[this]() { if(auto* PC=GetWorld()->GetFirstPlayerController()) if(auto* HUD=Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->ZeigeVollkarte(false); }},
   {9.3f,Foto},
   {10.0f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get(); A&&A->IstUnterwegs()) {
      const FVector Z=A->HoleWegpunkt(); auto* PC=GetWorld()->GetFirstPlayerController();
      const FVector Von=PC&&PC->GetPawn()?PC->GetPawn()->GetActorLocation():Z;
      Stelle(Z+(Von-Z).GetSafeNormal2D()*3500.0f,Z); } }},
   {11.5f,Foto},
   // Ins Ziel: erledigt, die naechste blaue Saeule steht woanders.
   {12.0f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get()) { const FVector Z=A->HoleWegpunkt(); Stelle(Z,Z+FVector(1000,0,0)); } }},
   {12.6f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get(); A&&!A->IstUnterwegs()) {
      auto* PC=GetWorld()->GetFirstPlayerController();
      if(PC&&PC->GetPawn()) Stelle(PC->GetPawn()->GetActorLocation()-FVector(0,0,110),A->HoleWegpunkt()); } }},
   {13.4f,Foto},
   {14.0f,[this,Stelle]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get(); A&&!A->IstUnterwegs()) { const FVector S=A->HoleWegpunkt(); Stelle(S,S+FVector(1000,0,0)); } }},
   {15.0f,[]() { if(auto* A=ALaLaBergAuftraege::Instanz.Get(); A&&A->IstUnterwegs()) A->TestAblaufen(); }},
   {16.0f,[]() {
     auto* A=ALaLaBergAuftraege::Instanz.Get();
     const bool bPass=A&&A->HoleZielzahl()>=10&&A->HoleErledigt()==1&&A->HoleGescheitert()==1&&A->HoleGeld()>0&&!A->IstUnterwegs();
     Beleg(FString::Printf(TEXT("LALABERG_AUFTRAGTEST %s ziele=%d erledigt=%d gescheitert=%d geld=%d"),bPass?TEXT("PASS"):TEXT("FAIL"),
      A?A->HoleZielzahl():0,A?A->HoleErledigt():0,A?A->HoleGescheitert():0,A?A->HoleGeld():0));
     FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
   }},
  };
  for(const FSchritt& S:Plan) { FTimerHandle H; TFunction<void()> Tu=S.Tu; GetWorldTimerManager().SetTimer(H,MoveTemp(Tu),S.Zeit,false); }
 }
 // -LaLaBergPolizeiTest: zwei echte Taten (Passant beschossen, Streife
 // beschossen) ergeben zwei Sterne. Die Figur bleibt stehen; die Streifen
 // muessen ueber das Strassennetz heranfahren und sie festnehmen (Bild beim
 // Eintreffen, Bild der Vollkarte). Danach ein Stern an der Inspektion, die
 // Figur wird 1,5 km weit weggesetzt und muss die Fahndung abschuetteln.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergPolizeiTest"))) {
  static double FestnahmeZeit=-1.0, AbhaengStart=-1.0;
  static bool bBildGemacht=false, bAbhaengen=false, bNahbild=false, bSireneGehoert=false;
  static FVector InspektionOrt=FVector::ZeroVector;
  FTimerHandle Tat;
  GetWorldTimerManager().SetTimer(Tat,[this]() {
   ALaLaBergPolizei::Melde(ELaLaBergTat::PassantBeschossen);
   ALaLaBergPolizei::Melde(ELaLaBergTat::PolizeiBeschossen);
   const auto* P=ALaLaBergPolizei::Instanz.Get();
   Beleg(FString::Printf(TEXT("LALABERG_POLIZEITEST taten sterne=%d"),P?P->HoleSterne():-1));
  },3.5f,false);
  FTimerHandle Karte;
  GetWorldTimerManager().SetTimer(Karte,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* HUD=PC?Cast<ALaLaBergHUD>(PC->GetHUD()):nullptr) HUD->ZeigeVollkarte(true);
   FTimerHandle Bild; GetWorldTimerManager().SetTimer(Bild,[this]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    if(PC) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
    FTimerHandle Zu; GetWorldTimerManager().SetTimer(Zu,[this]() {
     auto* PC=GetWorld()->GetFirstPlayerController();
     if(auto* HUD=PC?Cast<ALaLaBergHUD>(PC->GetHUD()):nullptr) HUD->ZeigeVollkarte(false);
    },0.6f,false);
   },0.8f,false);
  },10.0f,false);
  FTimerHandle Takt;
  GetWorldTimerManager().SetTimer(Takt,[this]() {
   auto* P=ALaLaBergPolizei::Instanz.Get();
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Figur=PC?PC->GetPawn():nullptr;
   if(!P||!Figur) return;
   const double Jetzt=GetWorld()->GetTimeSeconds();
   // Alle 5 s Spielzeit: wie nah die naechste Streife ist - sonst bleibt bei
   // einem FAIL offen, ob sie feststeckte oder nur zu langsam war.
   static double LetzterStand=0.0;
   if(P->HoleSterne()>0&&Jetzt-LetzterStand>5.0) {
    LetzterStand=Jetzt;
    TArray<FVector> Orte; P->HoleStreifen(Orte);
    FString Liste; for(const FVector& O:Orte) Liste+=FString::Printf(TEXT(" %.0f,%.0f"),O.X,O.Y);
    Beleg(FString::Printf(TEXT("LALABERG_POLIZEITEST stand t=%.0fs naechste=%.0fm gesehen=%d streifen=%s"),Jetzt,
     FMath::Min(P->NaechsteStreifeCm()/100.0f,99999.0f),P->WirdGesehen()?1:0,*Liste));
   }
   // Eintreffen der ersten Streife: Blick hin, Bild.
   if(!bBildGemacht&&P->HoleSterne()>0&&P->NaechsteStreifeCm()<3000.0f) {
    bBildGemacht=true;
    TArray<FVector> Orte; P->HoleStreifen(Orte);
    FVector Naechste=Orte.Num()?Orte[0]:Figur->GetActorLocation();
    for(const FVector& O:Orte) if(FVector::Dist(O,Figur->GetActorLocation())<FVector::Dist(Naechste,Figur->GetActorLocation())) Naechste=O;
    PC->SetControlRotation(FRotator(-10.0f,(Naechste-Figur->GetActorLocation()).Rotation().Yaw,0));
    Beleg(FString::Printf(TEXT("LALABERG_POLIZEITEST streife_da nach=%.1fs abstand=%.0fm"),Jetzt,P->NaechsteStreifeCm()/100.0f));
    // Erst im naechsten Bild dreht die Kamera - das Foto etwas spaeter.
    FTimerHandle Bild; GetWorldTimerManager().SetTimer(Bild,[this]() {
     if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
    },0.5f,false);
   }
   // Naher Blick auf den Streifenwagen: Blaulicht sichtbar?
   if(bBildGemacht&&!bNahbild&&P->HoleSterne()>0&&P->NaechsteStreifeCm()<1500.0f) {
    bNahbild=true;
    TArray<FVector> Orte; P->HoleStreifen(Orte);
    FVector Naechste=Orte.Num()?Orte[0]:Figur->GetActorLocation();
    for(const FVector& O:Orte) if(FVector::Dist(O,Figur->GetActorLocation())<FVector::Dist(Naechste,Figur->GetActorLocation())) Naechste=O;
    PC->SetControlRotation(FRotator(-30.0f,(Naechste-Figur->GetActorLocation()).Rotation().Yaw,0));
    FTimerHandle Bild; GetWorldTimerManager().SetTimer(Bild,[this]() {
     if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900"));
    },0.3f,false);
   }
   if(FestnahmeZeit<0.0&&P->HoleFestnahmen()>=1) {
    FestnahmeZeit=Jetzt;
    InspektionOrt=Figur->GetActorLocation();
    Beleg(FString::Printf(TEXT("LALABERG_POLIZEITEST festnahme nach=%.1fs ort=%s"),Jetzt,*InspektionOrt.ToString()));
   }
   // Phase 2: an der Inspektion ein Stern, dann weit weg.
   if(FestnahmeZeit>0.0&&!bAbhaengen&&Jetzt>FestnahmeZeit+2.0) {
    bAbhaengen=true; AbhaengStart=Jetzt;
    P->TestSetzeSterne(1);
    bool bFrei=false;
    const FTransform Weit=SucheFahrbahn(FVector(-153670,12480,InspektionOrt.Z),bFrei);
    Figur->SetActorLocation(Weit.GetLocation()+FVector(0,0,20),false,nullptr,ETeleportType::TeleportPhysics);
    Beleg(FString::Printf(TEXT("LALABERG_POLIZEITEST abhaengen von=%s nach=%s weite=%.0fm"),*InspektionOrt.ToString(),
     *Weit.GetLocation().ToString(),FVector::Dist2D(InspektionOrt,Weit.GetLocation())/100.0f));
   }
   // Mindestens eine Streife im Einsatz muss dabei das Martinshorn fahren.
   for(const ALaLaBergVerkehrsauto* Auto:ALaLaBergVerkehrsauto::Alle)
    if(Auto&&Auto->SireneAn()) { bSireneGehoert=true; break; }
   const bool bFertig=bAbhaengen&&P->HoleSterne()==0;
   const bool bZeitUm=Jetzt>150.0||(bAbhaengen&&Jetzt>AbhaengStart+30.0);
   if(bFertig||bZeitUm) {
    const bool bPass=bFertig&&P->HoleFestnahmen()==1&&bBildGemacht&&bSireneGehoert;
    Beleg(FString::Printf(TEXT("LALABERG_POLIZEITEST %s festnahmen=%d abgehaengt_nach=%.1fs bild=%d sirene=%d"),bPass?TEXT("PASS"):TEXT("FAIL"),
     P->HoleFestnahmen(),bAbhaengen?Jetzt-AbhaengStart:-1.0,bBildGemacht?1:0,bSireneGehoert?1:0));
    FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
   }
  },0.25f,true);
 }
 // -LaLaBergLadenTest: 2000 € aufs Konto, im Paintball-Laden die MP
 // kaufen, in den Wagen, zwei Sterne, in der Lackiererei ungesehen neu
 // lackieren - die Fahndung muss weg sein. Zum Schluss den Spielstand frisch
 // von der Platte lesen: Geld, Waffe und Lack muessen drinstehen.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergLadenTest"))) {
  static bool bMp=false, bLack=false;
  auto Foto=[this]() { if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900")); };
  struct FSchritt { float Zeit; TFunction<void()> Tu; };
  TArray<FSchritt> Plan={
   {3.0f,[this]() { if(auto* K=ULaLaBergKonto::Hole(this)) K->Gutschrift(2000); }},
   {4.5f,[this]() {
     auto* L=ALaLaBergLaeden::Instanz.Get(); auto* PC=GetWorld()->GetFirstPlayerController();
     if(!L||!PC||!PC->GetPawn()) return;
     const FVector Ort=L->HoleLaeden()[ALaLaBergLaeden::PAINTBALL].Ort;
     PC->GetPawn()->SetActorLocation(Ort+FVector(0,0,110),false,nullptr,ETeleportType::TeleportPhysics);
     PC->SetControlRotation(FRotator(-10,30,0));
     if(auto* HUD=Cast<ALaLaBergHUD>(PC->GetHUD())) HUD->OrtSofort(); }},
   {5.6f,Foto},
   {6.2f,[this]() {
     auto* L=ALaLaBergLaeden::Instanz.Get(); auto* K=ULaLaBergKonto::Hole(this);
     bMp=L&&L->HoleOffen()==ALaLaBergLaeden::PAINTBALL&&L->Kaufe(0)&&K&&K->HatWaffe(1)&&K->HoleGeld()==1700;
     Beleg(FString::Printf(TEXT("LALABERG_LADENTEST mp=%d geld=%d"),bMp?1:0,K?K->HoleGeld():-1)); }},
   {7.0f,[this]() {
     auto* PC=GetWorld()->GetFirstPlayerController();
     auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
     for(TActorIterator<ALaLaBergWagen> It(GetWorld());It&&Figur;++It) {
      Figur->SetActorLocation(It->GetActorLocation()-It->GetActorRightVector()*300.0f+FVector(0,0,60),false,nullptr,ETeleportType::TeleportPhysics);
      break;
     } }},
   {7.6f,[this]() {
     auto* PC=GetWorld()->GetFirstPlayerController();
     if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen(); }},
   {8.6f,[this]() {
     auto* L=ALaLaBergLaeden::Instanz.Get(); auto* PC=GetWorld()->GetFirstPlayerController();
     auto* Wagen=PC?Cast<ALaLaBergWagen>(PC->GetPawn()):nullptr;
     if(!L||!Wagen) { Beleg(TEXT("LALABERG_LADENTEST kein Wagen")); return; }
     const FVector Ort=L->HoleLaeden()[ALaLaBergLaeden::LACKIEREREI].Ort;
     Wagen->SetActorLocationAndRotation(Ort+FVector(0,0,90),FRotator(0,PC->GetControlRotation().Yaw,0),false,nullptr,ETeleportType::TeleportPhysics); }},
   // Erst hier gesucht: die Streifen setzen 250 m entfernt ein, ausser Sicht -
   // sonst entscheidet der Zufall des Einsatzorts, ob die Polizei zusieht.
   {9.8f,[this]() { if(auto* P=ALaLaBergPolizei::Instanz.Get()) P->TestSetzeSterne(2); }},
   {10.0f,Foto},
   {10.3f,[this]() {
     auto* L=ALaLaBergLaeden::Instanz.Get(); auto* K=ULaLaBergKonto::Hole(this); auto* P=ALaLaBergPolizei::Instanz.Get();
     const bool bOffen=L&&L->HoleOffen()==ALaLaBergLaeden::LACKIEREREI;
     const bool bGekauft=bOffen&&L->Kaufe(3);
     FLinearColor Farbe;
     bLack=bGekauft&&K&&K->HatLack(Farbe)&&P&&P->HoleSterne()==0&&K->HoleGeld()==1550;
     Beleg(FString::Printf(TEXT("LALABERG_LADENTEST lack=%d offen=%d sterne=%d geld=%d"),bLack?1:0,bOffen?1:0,P?P->HoleSterne():-1,K?K->HoleGeld():-1)); }},
   {11.4f,Foto},
   {12.0f,[this]() {
     auto* K=ULaLaBergKonto::Hole(this);
     const ULaLaBergSpielstand* Platte=K?K->LadeVonPlatte():nullptr;
     const bool bGespeichert=Platte&&Platte->Geld==1550&&(Platte->Waffen&2)&&Platte->bHatLack;
     const bool bPass=bMp&&bLack&&bGespeichert;
     Beleg(FString::Printf(TEXT("LALABERG_LADENTEST %s mp=%d lack=%d gespeichert=%d slot=%s platte_geld=%d"),bPass?TEXT("PASS"):TEXT("FAIL"),
      bMp?1:0,bLack?1:0,bGespeichert?1:0,K?*K->HoleSlot():TEXT("-"),Platte?Platte->Geld:-1));
     FPlatformMisc::RequestExitWithStatus(false,bPass?0:1); }},
  };
  for(const FSchritt& S:Plan) { FTimerHandle H; TFunction<void()> Tu=S.Tu; GetWorldTimerManager().SetTimer(H,MoveTemp(Tu),S.Zeit,false); }
 }
 // -LaLaBergAntriebTest [-LaLaBergDrehmoment=<Nm>]: einsteigen, Vollgas,
 // halbsekuendlich Karosseriemasse, Motor und Raeder ins Log. Klaert, warum
 // der Antrieb das Hundertfache des realen Moments braucht (siehe
 // Docs/ChaosVehicle-ForumAnfrage.md). Kein PASS/FAIL - ein Befund.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAntriebTest"))) {
  FTimerHandle Hin;
  GetWorldTimerManager().SetTimer(Hin,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr;
   for(TActorIterator<ALaLaBergWagen> It(GetWorld());It&&Figur;++It) {
    Figur->SetActorLocation(It->GetActorLocation()-It->GetActorRightVector()*300.0f+FVector(0,0,60),false,nullptr,ETeleportType::TeleportPhysics);
    break;
   }
  },4.6f,false);
  FTimerHandle Los;
  GetWorldTimerManager().SetTimer(Los,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Figur=PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr) Figur->Einsteigen();
   auto* Wagen=PC?Cast<ALaLaBergWagen>(PC->GetPawn()):nullptr;
   if(!Wagen) { Beleg(TEXT("LALABERG_ANTRIEB kein Wagen")); return; }
   float Nm=0.0f;
   if(FParse::Value(FCommandLine::Get(),TEXT("LaLaBergDrehmoment="),Nm)&&Nm>0.0f) Wagen->SetzeTestDrehmoment(Nm);
   Beleg(TEXT("LALABERG_ANTRIEB stand ")+Wagen->Antriebsbefund());
   // -LaLaBergSchub: statt Motor eine von Halbsekunde zu Halbsekunde
   // wachsende Schubkraft - rollt der Wagen bei realistischer Kraft nicht,
   // haelt ihn etwas ausserhalb des Antriebs fest.
   if(!FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSchub"))) Wagen->TestSteuerung(1.0f,0.0f);
  },6.0f,false);
  FTimerHandle Takt;
  GetWorldTimerManager().SetTimer(Takt,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   if(auto* Wagen=PC?Cast<ALaLaBergWagen>(PC->GetPawn()):nullptr) {
    static float Schub=0.0f;
    if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSchub"))) { Schub+=1000.0f; Wagen->TestSchub(Schub); }
    Beleg(TEXT("LALABERG_ANTRIEB ")+Wagen->Antriebsbefund());
   }
  },0.5f,true,6.5f);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[]() { FPlatformMisc::RequestExitWithStatus(false,0); },12.0f,false);
 }
 // -LaLaBergZielFoto: folgt der Oberkoerper beim Zielen der Kamera? Von der
 // Seite je ein Bild bei -35, 0 und +35 Grad Neigung, dazu die gemessene
 // Neigung der Waffe. PASS, wenn die Waffe zwischen -35 und +35 um mehr als
 // 30 Grad mitkippt. Zum Schluss ein Bild von hinten mit echter Kameraneigung.
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergZielFoto"))) {
  static float WaffenNeigung[3]={0,0,0};
  auto Held=[this]() { auto* PC=GetWorld()->GetFirstPlayerController(); return PC?Cast<ALaLaBergCharacter>(PC->GetPawn()):nullptr; };
  auto Foto=[this]() { if(auto* PC=GetWorld()->GetFirstPlayerController()) PC->ConsoleCommand(TEXT("HighResShot 1600x900")); };
  struct FSchritt { float Zeit; TFunction<void()> Tu; };
  TArray<FSchritt> Plan={{4.5f,[this,Held]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    if(auto* H=Held()) { H->SetActorRotation(FRotator(0,0,0)); PC->SetControlRotation(FRotator(-5,-90,0)); } }}};
  const float Winkel[3]={-35.0f,0.0f,35.0f};
  for(int32 i=0;i<3;i++) {
   Plan.Add({5.0f+i*1.6f,[Held,W=Winkel[i]]() { if(auto* H=Held()) H->TestNeigung(W); }});
   Plan.Add({5.9f+i*1.6f,[Held,i]() {
     auto* H=Held();
     WaffenNeigung[i]=H&&H->HoleWaffe()?H->HoleWaffe()->GetActorForwardVector().Rotation().Pitch:0.0f;
     Beleg(FString::Printf(TEXT("LALABERG_ZIEL soll=%.0f neigung=%.1f waffe=%.1f"),i==0?-35.0f:(i==1?0.0f:35.0f),H?H->HoleNeigung():0.0f,WaffenNeigung[i])); }});
   Plan.Add({6.1f+i*1.6f,Foto});
  }
  Plan.Add({10.0f,[this,Held]() {
    auto* PC=GetWorld()->GetFirstPlayerController();
    if(auto* H=Held()) { H->TestNeigung(NAN); PC->SetControlRotation(FRotator(25,0,0)); } }});
  Plan.Add({11.0f,Foto});
  Plan.Add({11.8f,[this]() {
    const float Spanne=WaffenNeigung[2]-WaffenNeigung[0];
    const bool bPass=Spanne>30.0f;
    Beleg(FString::Printf(TEXT("LALABERG_ZIELTEST %s waffe_unten=%.1f waffe_mitte=%.1f waffe_oben=%.1f spanne=%.1f"),bPass?TEXT("PASS"):TEXT("FAIL"),
     WaffenNeigung[0],WaffenNeigung[1],WaffenNeigung[2],Spanne));
    FPlatformMisc::RequestExitWithStatus(false,bPass?0:1); }});
  for(const FSchritt& S:Plan) { FTimerHandle H; TFunction<void()> Tu=S.Tu; GetWorldTimerManager().SetTimer(H,MoveTemp(Tu),S.Zeit,false); }
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergAbbiegeTest"))) {
  static TMap<ALaLaBergVerkehrsauto*,float> LetzteGier, GierSumme;
  FTimerHandle Takt;
  GetWorldTimerManager().SetTimer(Takt,[this]() {
   for(ALaLaBergVerkehrsauto* A:ALaLaBergVerkehrsauto::Alle) {
    if(!A) continue;
    const float Gier=A->GetActorRotation().Yaw;
    if(const float* Vorher=LetzteGier.Find(A)) {
     const float Schritt=FMath::Abs(FRotator::NormalizeAxis(Gier-*Vorher));
     if(Schritt>4.0f) GierSumme.FindOrAdd(A)+=Schritt;
    }
    LetzteGier.Add(A,Gier);
   }
  },0.25f,true,3.0f);
  FTimerHandle Ende;
  GetWorldTimerManager().SetTimer(Ende,[this]() {
   int32 Abgebogen=0; float Groesste=0.0f;
   for(const auto& Paar:GierSumme) {
    if(Paar.Value>60.0f) Abgebogen++;
    Groesste=FMath::Max(Groesste,Paar.Value);
   }
   // Wendemanoever zaehlen: seit die Routen geschlossene Rundkurse sind
   // (siehe Tools/Export/prepare-verkehr.cjs), darf kein Auto mehr am
   // Routenende auf der Stelle umkehren. Die aufsummierte Drehung oben kann
   // Abbiegen und Wenden nicht unterscheiden - erst dieser Zaehler belegt es.
   int32 Wenden=0;
   for(ALaLaBergVerkehrsauto* A:ALaLaBergVerkehrsauto::Alle) if(A) Wenden+=A->HoleWenden();
   // Fuenf von 70 Autos als Untergrenze: genug, um einen Totalausfall
   // (gar kein Abbiegen mehr) sicher zu erkennen, ohne dass der Test an
   // roten Ampeln oder einem zufaellig geraden Streckenabschnitt scheitert.
   const bool bPass=Abgebogen>=5 && Wenden==0;
   Beleg(FString::Printf(TEXT("LALABERG_ABBIEGETEST %s abgebogen=%d von=%d groesste_drehung=%.0f wenden=%d"),
    bPass?TEXT("PASS"):TEXT("FAIL"),Abgebogen,ALaLaBergVerkehrsauto::Alle.Num(),Groesste,Wenden));
   FPlatformMisc::RequestExitWithStatus(false,bPass?0:1);
  },38.0f,false);
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("LaLaBergSmoke"))) {
  FTimerHandle Handle;
  GetWorldTimerManager().SetTimer(Handle,[this]() {
   auto* PC=GetWorld()->GetFirstPlayerController();
   APawn* Pawn=PC?PC->GetPawn():nullptr;
   FHitResult Hit; FCollisionQueryParams Params; if(Pawn) Params.AddIgnoredActor(Pawn);
   const FVector P=Pawn?Pawn->GetActorLocation():FVector::ZeroVector;
   const bool Ground=GetWorld()->LineTraceSingleByChannel(Hit,P,P-FVector(0,0,300),ECC_Visibility,Params);
   const bool Passed=bSceneReady && BuildingCount>0 && Pawn && Ground;
   FHitResult Weit; const bool Tief=GetWorld()->LineTraceSingleByChannel(Weit,FVector(0,0,20000),FVector(0,0,-20000),ECC_Visibility,Params);
   int32 Fertig=0,Gesamt=0;
   for(TObjectIterator<UProceduralMeshComponent> It;It;++It) { if(It->GetWorld()!=GetWorld()) continue; Gesamt++; if(It->GetBodySetup() && It->GetBodySetup()->bCreatedPhysicsMeshes) Fertig++; }
   UE_LOG(LogTemp,Display,TEXT("LALABERG_PHYSIK fertig=%d von=%d"),Fertig,Gesamt);
   UE_LOG(LogTemp,Display,TEXT("LALABERG_SMOKE %s buildings=%d pawn=%d ground=%d z=%.0f v=%.0f tief=%d trefferZ=%.0f"),Passed?TEXT("PASS"):TEXT("FAIL"),BuildingCount,Pawn!=nullptr,Ground,P.Z,Pawn?Pawn->GetVelocity().Z:0.0,Tief,Tief?Weit.ImpactPoint.Z:0.0);
   FPlatformMisc::RequestExitWithStatus(false,Passed?0:1);
  },3.0f,false);
 }
}


// Die Stadt aus fertigen Assets: je Sektor und Klasse ein Netz, alle in
// Weltkoordinaten gebaut, also am Ursprung eingesetzt. Fehlt das Verzeichnis
// oder ein Netz, kehrt die Funktion zurueck und der Laufzeitweg uebernimmt.
// Wegpunkte fuer KI-Verkehr und Passanten: ein Auto je Route, ein Tempo
// zwischen 28 und 46 km/h zufaellig je Wagen, damit nicht die ganze Stadt im
// Gleichschritt faehrt. Passanten gehen mit gewoehnlichem Gehtempo.
void ALaLaBergGameMode::LadeVerkehr() {
 FString Text;
 TSharedPtr<FJsonObject> Wurzel;
 const FString Datei=FPaths::ProjectContentDir()/TEXT("SourceData/Verkehr/verkehr.json");
 if(!FFileHelper::LoadFileToString(Text,*Datei) ||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Wurzel) || !Wurzel.IsValid()) {
  UE_LOG(LogTemp,Warning,TEXT("LALABERG_VERKEHR fehlt: %s"),*Datei);
  return;
 }
 auto LiesRoute=[](const TSharedPtr<FJsonObject>& Obj)->TArray<FVector> {
  TArray<FVector> Route;
  const auto& Zahlen=Obj->GetArrayField(TEXT("p"));
  for(int32 i=0;i+2<Zahlen.Num();i+=3)
   Route.Add(FVector(Zahlen[i]->AsNumber(),Zahlen[i+1]->AsNumber(),Zahlen[i+2]->AsNumber()));
  return Route;
 };
 // Echte Kreuzungen aus dem Strassengraphen (siehe Tools/Export/prepare-
 // verkehr.cjs "kreuzungen") - vor den Autos einlesen, die brauchen die
 // Liste schon fuer SetzeKreuzung unten. Keine Hoehe (siehe LiesRoute vs.
 // hier x/y statt x/y/z) - BremseVorKreuzung vergleicht nur in der Ebene.
 ALaLaBergVerkehrsauto::KreuzungOrte.Empty();
 ALaLaBergVerkehrsauto::KreuzungKlassen.Empty();
 ALaLaBergVerkehrsauto::KreuzungBreiten.Empty();
 for(const auto& Wert:Wurzel->GetArrayField(TEXT("kreuzungen"))) {
  const auto Obj=Wert->AsObject();
  ALaLaBergVerkehrsauto::KreuzungOrte.Add(FVector(Obj->GetNumberField(TEXT("x")),Obj->GetNumberField(TEXT("y")),0));
  ALaLaBergVerkehrsauto::KreuzungKlassen.Add(Obj->GetIntegerField(TEXT("klasse")));
  // "breite" fehlt nur bei sehr alten verkehr.json-Staenden - 3.8 m
  // (Klasse 3, mittlere Breite) als Rueckfall wie zuvor der feste Puffer.
  ALaLaBergVerkehrsauto::KreuzungBreiten.Add(Obj->HasField(TEXT("breite"))?Obj->GetNumberField(TEXT("breite")):3.8f);
 }
 // Vor den KI-Autos: die brauchen ALaLaBergAutoPool::Instanz schon in ihrem
 // eigenen BeginPlay (siehe dort).
 GetWorld()->SpawnActor<ALaLaBergAutoPool>();
 for(const auto& Wert:Wurzel->GetArrayField(TEXT("autos"))) {
  const auto AutoObj=Wert->AsObject();
  const TArray<FVector> Route=LiesRoute(AutoObj);
  if(Route.Num()<2) continue;
  if(auto* Auto=GetWorld()->SpawnActor<ALaLaBergVerkehrsauto>(Route[0],FRotator::ZeroRotator)) {
   Auto->SetzeRoute(Route,FMath::FRandRange(28.0f,46.0f));
   // Geschlossener Rundkurs (siehe Tools/Export/prepare-verkehr.cjs): das
   // Auto haengt hinter dem letzten Wegpunkt wieder den ersten an, statt am
   // Ende auf der Stelle zu wenden. Aeltere verkehr.json ohne das Feld
   // bleiben beim alten Hin-und-Zurueck.
   Auto->SetzeRundkurs(AutoObj->HasField(TEXT("rund")) && AutoObj->GetBoolField(TEXT("rund")));
   TArray<int32> Kreuzungen;
   const TArray<TSharedPtr<FJsonValue>>* KreuzungenJson=nullptr;
   if(AutoObj->TryGetArrayField(TEXT("kreuzungen"),KreuzungenJson))
    for(const auto& K:*KreuzungenJson) Kreuzungen.Add(static_cast<int32>(K->AsNumber()));
   Auto->SetzeKreuzung(AutoObj->HasField(TEXT("klasse"))?AutoObj->GetIntegerField(TEXT("klasse")):3,Kreuzungen);
   // Echte Fahrbahnbreite (Meter, siehe Tools/Export/prepare-verkehr.cjs
   // "w") statt eines fuer jede Strasse gleichen Spur-Versatzes.
   if(AutoObj->HasField(TEXT("w"))) Auto->SetzeStrassenbreite(AutoObj->GetNumberField(TEXT("w")));
   // Breite und Klasse je Wegpunkt ("bp"/"kp"): eine Fahrt verkettet seit
   // der Graph-Routenplanung mehrere Strassen, beide Werte gelten also
   // nicht mehr fuer die ganze Route (siehe SetzeSpurdaten). Fehlen sie,
   // bleibt es bei den Route-Werten oben.
   const TArray<TSharedPtr<FJsonValue>>* BreitenJson=nullptr;
   const TArray<TSharedPtr<FJsonValue>>* KlassenJson=nullptr;
   if(AutoObj->TryGetArrayField(TEXT("bp"),BreitenJson) && AutoObj->TryGetArrayField(TEXT("kp"),KlassenJson)) {
    TArray<float> Breiten; TArray<int32> Klassen;
    for(const auto& B:*BreitenJson) Breiten.Add(B->AsNumber());
    for(const auto& K:*KlassenJson) Klassen.Add(static_cast<int32>(K->AsNumber()));
    if(Breiten.Num()==Route.Num() && Klassen.Num()==Route.Num())
     Auto->SetzeSpurdaten(Breiten,Klassen);
    else
     UE_LOG(LogTemp,Warning,TEXT("LALABERG_VERKEHR spurdaten passen nicht: %d/%d zu %d Wegpunkten"),
      Breiten.Num(),Klassen.Num(),Route.Num());
   }
   AutoZahl++;
  }
 }
 for(const auto& Wert:Wurzel->GetArrayField(TEXT("passanten"))) {
  const TArray<FVector> Route=LiesRoute(Wert->AsObject());
  if(Route.Num()<2) continue;
  if(auto* Passant=GetWorld()->SpawnActor<ALaLaBergPassantKI>(Route[0],FRotator::ZeroRotator)) {
   Passant->SetzeRoute(Route,FMath::FRandRange(4.2f,5.6f));
   PassantZahl++;
  }
 }
 for(const auto& Wert:Wurzel->GetArrayField(TEXT("ampeln"))) {
  const auto Obj=Wert->AsObject();
  const FVector Ort(Obj->GetNumberField(TEXT("x")),Obj->GetNumberField(TEXT("y")),Obj->GetNumberField(TEXT("z")));
  const FTransform Lage(FRotator(0,Obj->GetNumberField(TEXT("gier")),0),Ort);
  // Gruppe/Phase (Kreuzungszuordnung aus prepare-verkehr.cjs) muessen vor
  // BeginPlay stehen - SpawnActor riefe BeginPlay schon auf, bevor SetzeGruppe
  // je zum Zug kaeme (siehe LaLaBergFarbkugel fuer dasselbe Vorgehen).
  if(auto* Ampel=GetWorld()->SpawnActorDeferred<ALaLaBergAmpel>(ALaLaBergAmpel::StaticClass(),Lage)) {
   // "phasen" fehlt nur bei sehr alten verkehr.json-Staenden vor der
   // Mehrphasen-Umstellung - 2 als Rueckfall wie zuvor der feste Wert.
   Ampel->SetzeGruppe(Obj->GetIntegerField(TEXT("gruppe")),Obj->GetIntegerField(TEXT("phase")),
    Obj->HasField(TEXT("phasen"))?Obj->GetIntegerField(TEXT("phasen")):2);
   Ampel->FinishSpawning(Lage);
   AmpelZahl++;
   GroessteAnzahlPhasen=FMath::Max(GroessteAnzahlPhasen,Ampel->HoleAnzahlPhasen());
  }
 }
 // Geparkte Autos: dieselben amtlichen Stellplaetze, die frueher als Kasten-
 // Geometrie ins Stadt-Mesh gebacken waren (siehe Tools/Export/prepare-
 // stadt.cjs) - jetzt echte, stehende KI-Auto-Akteure ohne Route. Dieselbe
 // Klasse wie die fahrenden KI-Autos: ohne SetzeRoute bleibt Weg ungueltig,
 // Tick() bewegt nichts, zeigt aber trotzdem das Sichtweiten-LOD-Modell und
 // laesst sich wie jedes andere KI-Auto uebernehmen (LaLaBergCharakter::
 // Einsteigen). Vor der Schleife: der Kasten-Pool (siehe LaLaBergKastenPool),
 // damit SetzeLack unten schon eine Instanz statt eines eigenen Netzes bekommt.
 GetWorld()->SpawnActor<ALaLaBergKastenPool>();
 for(const auto& Wert:Wurzel->GetArrayField(TEXT("geparkt"))) {
  const auto Obj=Wert->AsObject();
  const FVector Ort(Obj->GetNumberField(TEXT("x")),Obj->GetNumberField(TEXT("y")),Obj->GetNumberField(TEXT("z")));
  const FRotator Blick(0,Obj->GetNumberField(TEXT("gier")),0);
  if(auto* Auto=GetWorld()->SpawnActor<ALaLaBergVerkehrsauto>(Ort,Blick)) {
   const auto& Rgb=Obj->GetArrayField(TEXT("lack"));
   Auto->SetzeLack(FLinearColor(Rgb[0]->AsNumber(),Rgb[1]->AsNumber(),Rgb[2]->AsNumber()));
   GeparktZahl++;
  }
 }
 UE_LOG(LogTemp,Display,TEXT("LALABERG_VERKEHR autos=%d passanten=%d ampeln=%d geparkt=%d kreuzungen=%d"),AutoZahl,PassantZahl,AmpelZahl,GeparktZahl,ALaLaBergVerkehrsauto::KreuzungOrte.Num());
}

bool ALaLaBergGameMode::LadeAusAssets(TSharedPtr<FJsonObject>& Metadaten) {
 const FString Wurzel=FPaths::ProjectContentDir()/TEXT("SourceData/Sectors");
 FString Text;
 TSharedPtr<FJsonObject> Verzeichnis;
 if(!FFileHelper::LoadFileToString(Text,*(Wurzel/TEXT("manifest.json"))) ||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Verzeichnis) || !Verzeichnis.IsValid())
  return false;
 Metadaten=Verzeichnis;

 const double Start=FPlatformTime::Seconds();
 int32 Netze=0, Fehlend=0;
 for(const auto& Wert:Verzeichnis->GetArrayField(TEXT("sectors"))) {
  const auto Sektor=Wert->AsObject();
  const FString Id=Sektor->GetStringField(TEXT("id"));
  for(const auto& Name:Sektor->GetArrayField(TEXT("sections"))) {
   const FString Klasse=Name->AsString();
   const FString Pfad=FString::Printf(TEXT("/Game/City/Sectors/%s/SM_%s_%s.SM_%s_%s"),
    *Id,*Id,*Klasse,*Id,*Klasse);
   auto* Netz=LoadObject<UStaticMesh>(nullptr,*Pfad);
   if(!Netz) { Fehlend++; continue; }
   auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(FVector::ZeroVector,FRotator::ZeroRotator);
   if(!Actor) { Fehlend++; continue; }
   auto* Teil=Actor->GetStaticMeshComponent();
   // Beweglich, nicht statisch: ohne gebautes Licht bekommt statische
   // Geometrie ihr indirektes Licht aus einem leeren Zwischenspeicher, und
   // jede Schattenseite wird schwarz - Himmels- und Fuelllicht kamen nicht an.
   Teil->SetMobility(EComponentMobility::Movable);
   Teil->SetStaticMesh(Netz);
   Teil->SetCollisionProfileName(TEXT("BlockAll"));
   Actor->Tags.Add(FName(*Klasse));
   Netze++;
  }
 }
 if(Netze==0) return false;
 if(Fehlend>0) UE_LOG(LogTemp,Warning,TEXT("LALABERG_ASSETS_UNVOLLSTAENDIG fehlend=%d"),Fehlend);
 BuildingCount=Verzeichnis->GetIntegerField(TEXT("buildingCount"));
 UE_LOG(LogTemp,Display,TEXT("LALABERG_ASSETS netze=%d fehlend=%d dauer=%.1fs"),
  Netze,Fehlend,FPlatformTime::Seconds()-Start);
 return true;
}
