using UnrealBuildTool;
public class LaLaBerg : ModuleRules {
 public LaLaBerg(ReadOnlyTargetRules Target) : base(Target) {
 PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
 PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "ProceduralMeshComponent",
  "Slate", "SlateCore", "ApplicationCore",
  // Fuer den StaticMesh-Import: FMeshDescription und FStaticMeshAttributes
  "MeshDescription", "StaticMeshDescription" });
 }
}
