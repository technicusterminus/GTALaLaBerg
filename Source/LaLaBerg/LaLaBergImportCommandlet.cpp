#include "LaLaBergImportCommandlet.h"

#if WITH_EDITOR
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "PhysicsEngine/BodySetup.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#endif

ULaLaBergImportCommandlet::ULaLaBergImportCommandlet()
{
 IsClient = false;
 IsEditor = true;
 IsServer = false;
 LogToConsole = true;
 ShowErrorCount = true;
}

#if WITH_EDITOR
namespace
{
 bool LoadJson(const FString& Filename, TSharedPtr<FJsonObject>& Out)
 {
  FString Text;
  return FFileHelper::LoadFileToString(Text, *Filename) &&
   FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Out) && Out.IsValid();
 }

 FString SafeName(const FString& Input)
 {
  FString Out;
  for (const TCHAR C : Input)
   Out.AppendChar(FChar::IsAlnum(C) || C == TEXT('_') ? C : TEXT('_'));
  return Out;
 }

 UMaterialInterface* MaterialFor(const FString& Name)
 {
  const TCHAR* Path = TEXT("/Game/Art/Materials/M_Boden.M_Boden");
  if (Name.StartsWith(TEXT("Wall"))) Path = TEXT("/Game/Art/Materials/M_Putz.M_Putz");
  else if (Name.StartsWith(TEXT("Roof"))) Path = TEXT("/Game/Art/Materials/M_Ziegel.M_Ziegel");
  // Claude: Pflaster matt statt Asphalt - gegen die Sonne spiegelte der
  // Hauptplatz sonst fast weiss.
  else if (Name.StartsWith(TEXT("Plaza"))) Path = TEXT("/Game/Art/Materials/M_Pflaster.M_Pflaster");
  else if (Name.StartsWith(TEXT("Road")) || Name.StartsWith(TEXT("Rail"))) Path = TEXT("/Game/Art/Materials/M_Asphalt.M_Asphalt");
  else if (Name.StartsWith(TEXT("Water"))) Path = TEXT("/Game/Art/Materials/M_Wasser.M_Wasser");
  else if (Name.StartsWith(TEXT("Tree")) || Name.StartsWith(TEXT("Trunk"))) Path = TEXT("/Game/Art/Materials/M_Laub.M_Laub");
  else if (Name.StartsWith(TEXT("Stone")) || Name.StartsWith(TEXT("Figure")) ||
           Name.StartsWith(TEXT("Sockel")) || Name.StartsWith(TEXT("Gesims")) ||
           Name.StartsWith(TEXT("Laden")) || Name.StartsWith(TEXT("Kamin"))) Path = TEXT("/Game/Art/Materials/M_Stein.M_Stein");
  // Claude: Fahrzeuge, Passanten und Tueren kamen nach dieser Tabelle dazu.
  // Ohne diese Zeilen bekamen sie das Vorgabematerial - die Wiese - und die
  // ganze Stadt stand in Olivgruen.
  else if (Name.StartsWith(TEXT("Auto"))) Path = TEXT("/Game/Art/Materials/M_Lack.M_Lack");
  else if (Name.StartsWith(TEXT("Glas"))) Path = TEXT("/Game/Art/Materials/M_Glas.M_Glas");
  else if (Name.StartsWith(TEXT("Reifen"))) Path = TEXT("/Game/Art/Materials/M_Asphalt.M_Asphalt");
  else if (Name.StartsWith(TEXT("Stoff")) || Name.StartsWith(TEXT("Haut"))) Path = TEXT("/Game/Art/Materials/M_Stoff.M_Stoff");
  else if (Name.StartsWith(TEXT("Tuer"))) Path = TEXT("/Game/Art/Materials/M_Stein.M_Stein");
  if (auto* Material = LoadObject<UMaterialInterface>(nullptr, Path)) return Material;
  return UMaterial::GetDefaultMaterial(MD_Surface);
 }

 bool IsSurfaceSection(const FString& Name)
 {
  return Name.StartsWith(TEXT("Ground")) || Name.StartsWith(TEXT("Road")) ||
   Name.StartsWith(TEXT("Rail")) || Name.StartsWith(TEXT("Plaza")) ||
   Name.StartsWith(TEXT("Water"));
 }

 bool UsesSmoothNormals(const FString& Name)
 {
  return IsSurfaceSection(Name) || Name.StartsWith(TEXT("Tree")) ||
   Name.StartsWith(TEXT("Trunk")) || Name.StartsWith(TEXT("Auto")) ||
   Name.StartsWith(TEXT("Glas")) || Name.StartsWith(TEXT("Reifen")) ||
   Name.StartsWith(TEXT("Stoff")) || Name.StartsWith(TEXT("Haut"));
 }

 bool BuildSection(const FString& SectorId, const TSharedPtr<FJsonObject>& Section, int32& OutTriangles)
 {
  const FString SectionName = Section->GetStringField(TEXT("name"));
  const TArray<TSharedPtr<FJsonValue>>& RawPositions = Section->GetArrayField(TEXT("positions"));
  const TArray<TSharedPtr<FJsonValue>>& RawIndices = Section->GetArrayField(TEXT("indices"));
  const TArray<TSharedPtr<FJsonValue>>* RawUVs = nullptr;
  Section->TryGetArrayField(TEXT("uvs"), RawUVs);
  if (RawPositions.Num() == 0 || RawPositions.Num() % 3 || RawIndices.Num() == 0 || RawIndices.Num() % 3)
  {
   UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_INVALID section=%s"), *SectionName);
   return false;
  }
  const int32 VertexCount = RawPositions.Num() / 3;
  if (RawUVs && RawUVs->Num() != VertexCount * 2)
  {
   UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_UV_INVALID section=%s"), *SectionName);
   return false;
  }

  FMeshDescription Description;
  FStaticMeshAttributes Attributes(Description);
  Attributes.Register();
  auto Positions = Attributes.GetVertexPositions();
  auto Normals = Attributes.GetVertexInstanceNormals();
  auto Tangents = Attributes.GetVertexInstanceTangents();
  auto BinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
  auto UVs = Attributes.GetVertexInstanceUVs();
  auto Colors = Attributes.GetVertexInstanceColors();
  auto SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
  UVs.SetNumChannels(1);

  TArray<FVertexID> Vertices;
  Vertices.Reserve(VertexCount);
  for (int32 Index = 0; Index < VertexCount; ++Index)
  {
   const FVertexID Vertex = Description.CreateVertex();
   Positions[Vertex] = FVector3f(
    RawPositions[Index * 3]->AsNumber(),
    RawPositions[Index * 3 + 1]->AsNumber(),
    RawPositions[Index * 3 + 2]->AsNumber());
   Vertices.Add(Vertex);
  }
  // Flaechen aus der Quelle sind im Uhrzeigersinn gespeichert. Fuer eine
  // nach aussen beziehungsweise nach oben zeigende Normale muss daher C x B
  // verwendet werden. B x C zeigte Boden, Strassen und Plaetze nach unten;
  // das erzeugte die dunklen Dreiecke und falsche WorldAlignedTexture-Seiten.
  const bool bSmooth = UsesSmoothNormals(SectionName);
  auto NormalKey = [](const FVector3f& Point) {
   return FIntVector(FMath::RoundToInt(Point.X), FMath::RoundToInt(Point.Y), FMath::RoundToInt(Point.Z));
  };
  TArray<FVector3f> FaceNormals;
  FaceNormals.SetNum(RawIndices.Num() / 3);
  TMap<FIntVector, FVector3f> NormalSums;
  for (int32 Index = 0; Index < RawIndices.Num(); Index += 3)
  {
   int32 SourceIndices[3];
   for (int32 Corner = 0; Corner < 3; ++Corner)
   {
    SourceIndices[Corner] = static_cast<int32>(RawIndices[Index + Corner]->AsNumber());
    if (!Vertices.IsValidIndex(SourceIndices[Corner]))
    {
     UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_INDEX_INVALID section=%s"), *SectionName);
     return false;
    }
   }
   const FVector3f A = Positions[Vertices[SourceIndices[0]]];
   const FVector3f B = Positions[Vertices[SourceIndices[1]]];
   const FVector3f C = Positions[Vertices[SourceIndices[2]]];
   const FVector3f WeightedNormal = FVector3f::CrossProduct(C - A, B - A);
   const FVector3f FaceNormal = WeightedNormal.GetSafeNormal();
   if (FaceNormal.IsNearlyZero())
   {
    UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_DEGENERATE section=%s triangle=%d"), *SectionName, Index / 3);
    return false;
   }
   FaceNormals[Index / 3] = FaceNormal;
   if (bSmooth) for (const int32 SourceIndex : SourceIndices)
    NormalSums.FindOrAdd(NormalKey(Positions[Vertices[SourceIndex]])) += WeightedNormal;
  }
  const FPolygonGroupID Group = Description.CreatePolygonGroup();
  const FName SlotName(*SafeName(SectionName));
  SlotNames[Group] = SlotName;

  // Claude: Scheitelfarben werden beim Netzbau nach sRGB codiert. Die Werte
  // in der Ausleitung sind aber schon die, die im Bild stehen sollen - ohne
  // Gegenrechnung wird jede Fassade um eine halbe Blende heller, aus Altrosa
  // wird Weiss. Also hier ins Lineare, damit die Codierung sie zurueckholt.
  auto InsLineare = [](double S) -> float {
   const float W = static_cast<float>(FMath::Clamp(S, 0.0, 1.0));
   return W <= 0.04045f ? W / 12.92f : FMath::Pow((W + 0.055f) / 1.055f, 2.4f);
  };
  const TArray<TSharedPtr<FJsonValue>>& RGB = Section->GetArrayField(TEXT("color"));
  const FVector4f Color(
   InsLineare(RGB.IsValidIndex(0) ? RGB[0]->AsNumber() : 1.0),
   InsLineare(RGB.IsValidIndex(1) ? RGB[1]->AsNumber() : 1.0),
   InsLineare(RGB.IsValidIndex(2) ? RGB[2]->AsNumber() : 1.0), 1.0f);

  for (int32 Index = 0; Index < RawIndices.Num(); Index += 3)
  {
   int32 SourceIndices[3];
   for (int32 Corner = 0; Corner < 3; ++Corner)
   {
    SourceIndices[Corner] = static_cast<int32>(RawIndices[Index + Corner]->AsNumber());
    if (!Vertices.IsValidIndex(SourceIndices[Corner]))
    {
     UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_INDEX_INVALID section=%s"), *SectionName);
     return false;
    }
   }
   const FVector3f A = Positions[Vertices[SourceIndices[0]]];
   const FVector3f FaceNormal = FaceNormals[Index / 3];
   TArray<FVertexInstanceID> Instances;
   Instances.Reserve(3);
   for (int32 Corner = 0; Corner < 3; ++Corner)
   {
    const int32 SourceIndex = SourceIndices[Corner];
    const FVector3f Point = Positions[Vertices[SourceIndex]];
    const FVertexInstanceID Instance = Description.CreateVertexInstance(Vertices[SourceIndex]);
    FVector3f Normal = FaceNormal;
    if (bSmooth)
     if (const FVector3f* Sum = NormalSums.Find(NormalKey(Point)))
      Normal = Sum->GetSafeNormal(UE_SMALL_NUMBER, FaceNormal);
    Normals[Instance] = Normal;
    const FVector3f Tangent = FVector3f::CrossProduct(FVector3f::UpVector, Normal)
     .GetSafeNormal(UE_SMALL_NUMBER, FVector3f::ForwardVector);
    Tangents[Instance] = Tangent;
    BinormalSigns[Instance] = 1.0f;
    Colors[Instance] = Color;
    UVs.Set(Instance, 0, RawUVs
     ? FVector2f((*RawUVs)[SourceIndex * 2]->AsNumber(), (*RawUVs)[SourceIndex * 2 + 1]->AsNumber())
     : FVector2f(Point.X / 200.0f, Point.Y / 200.0f));
    Instances.Add(Instance);
   }
   Description.CreatePolygon(Group, Instances);
  }

  const FString AssetName = FString::Printf(TEXT("SM_%s_%s"), *SafeName(SectorId), *SafeName(SectionName));
  const FString PackageName = FString::Printf(TEXT("/Game/City/Sectors/%s/%s"), *SafeName(SectorId), *AssetName);
  UPackage* Package = CreatePackage(*PackageName);
  Package->FullyLoad();
  UStaticMesh* Mesh = FindObject<UStaticMesh>(Package, *AssetName);
  const bool bNew = Mesh == nullptr;
  if (!Mesh) Mesh = NewObject<UStaticMesh>(Package, *AssetName, RF_Public | RF_Standalone);
  Mesh->GetStaticMaterials().Reset();
  UMaterialInterface* Material = MaterialFor(SectionName);
  Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, SlotName, SlotName));
  Mesh->CreateBodySetup();
  Mesh->GetBodySetup()->CollisionTraceFlag = CTF_UseComplexAsSimple;
  FMeshNaniteSettings Nanite = Mesh->GetNaniteSettings();
  Nanite.bEnabled = !SectionName.StartsWith(TEXT("Water")) && !SectionName.StartsWith(TEXT("Tree"));
  Mesh->SetNaniteSettings(Nanite);

  TArray<const FMeshDescription*> Descriptions{ &Description };
  UStaticMesh::FBuildMeshDescriptionsParams Build;   // in 5.8 in UStaticMesh geschachtelt
  Build.bUseHashAsGuid = true;
  Build.bBuildSimpleCollision = false;
  Build.bAllowCpuAccess = false;
  if (!Mesh->BuildFromMeshDescriptions(Descriptions, Build))
  {
   UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_BUILD_FAILED asset=%s"), *PackageName);
   return false;
  }
  Mesh->MarkPackageDirty();
  if (bNew) FAssetRegistryModule::AssetCreated(Mesh);
  const FString Filename = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
  FSavePackageArgs SaveArgs;
  SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
  SaveArgs.SaveFlags = SAVE_NoError;
  if (!UPackage::SavePackage(Package, Mesh, *Filename, SaveArgs))
  {
   UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_SAVE_FAILED file=%s"), *Filename);
   return false;
  }
  OutTriangles += RawIndices.Num() / 3;
  UE_LOG(LogTemp, Display, TEXT("LALABERG_STATIC_ASSET %s triangles=%d nanite=%d"), *PackageName, RawIndices.Num()/3, Nanite.bEnabled);
  return true;
 }
}
#endif

int32 ULaLaBergImportCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
 UE_LOG(LogTemp, Error, TEXT("LaLaBergImport requires an editor build"));
 return 1;
#else
 FString SectorId = TEXT("S_N1_N1");
 FParse::Value(*Params, TEXT("Sector="), SectorId);
 const bool bSurfaceOnly = FParse::Param(*Params, TEXT("SurfaceOnly"));
 if (SectorId.Contains(TEXT("..")) || SectorId.Contains(TEXT("/")) || SectorId.Contains(TEXT("\\")))
 {
  UE_LOG(LogTemp, Error, TEXT("Invalid sector id"));
  return 1;
 }
 const FString Root = FPaths::ProjectContentDir() / TEXT("SourceData/Sectors");
 TSharedPtr<FJsonObject> Manifest;
 if (!LoadJson(Root / TEXT("manifest.json"), Manifest))
 {
  UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_MANIFEST_FAILED"));
  return 1;
 }
 // Claude: "-Sector=alle" laeuft ueber das ganze Verzeichnis. Ein eigener
 // Editorstart je Sektor kostet bei 101 Sektoren mehr Zeit als der Import
 // selbst.
 TArray<FString> Aufgaben;
 const bool bAlle = SectorId.Equals(TEXT("alle"), ESearchCase::IgnoreCase) ||
                    SectorId.Equals(TEXT("all"), ESearchCase::IgnoreCase);
 for (const auto& Value : Manifest->GetArrayField(TEXT("sectors")))
 {
  const FString Id = Value->AsObject()->GetStringField(TEXT("id"));
  if (bAlle || Id == SectorId) Aufgaben.Add(Id);
 }
 if (Aufgaben.Num() == 0)
 {
  UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_UNKNOWN_SECTOR %s"), *SectorId);
  return 1;
 }

 int32 AssetCount = 0, TriangleCount = 0, Fehler = 0, Uebersprungen = 0;
 for (const FString& Id : Aufgaben)
 {
  TSharedPtr<FJsonObject> Sector;
  if (!LoadJson(Root / (Id + TEXT(".json")), Sector))
  {
   UE_LOG(LogTemp, Error, TEXT("LALABERG_STATIC_SECTOR_FAILED %s"), *Id);
   ++Fehler;
   continue;
  }
  for (const auto& Value : Sector->GetArrayField(TEXT("sections")))
  {
   const TSharedPtr<FJsonObject> Section = Value->AsObject();
   const FString SectionName = Section->GetStringField(TEXT("name"));
   if (bSurfaceOnly && !IsSurfaceSection(SectionName))
   {
    ++Uebersprungen;
    continue;
   }
   if (BuildSection(Id, Section, TriangleCount)) ++AssetCount;
   else ++Fehler;
  }
 }
 UE_LOG(LogTemp, Display, TEXT("LALABERG_STATIC_IMPORT %s modus=%s sektoren=%d assets=%d uebersprungen=%d triangles=%d fehler=%d"),
  Fehler == 0 ? TEXT("PASS") : TEXT("FAIL"), bSurfaceOnly ? TEXT("surface") : TEXT("all"),
  Aufgaben.Num(), AssetCount, Uebersprungen, TriangleCount, Fehler);
 return Fehler == 0 ? 0 : 1;
#endif
}
