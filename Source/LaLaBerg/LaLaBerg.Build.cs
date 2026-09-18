using UnrealBuildTool;
public class LaLaBerg : ModuleRules {
 public LaLaBerg(ReadOnlyTargetRules Target) : base(Target) {
 // Several existing translation units use identical private helper names.
 // Build each separately so adaptive unity grouping cannot break clean builds.
 bUseUnity = false;
 PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
 PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "Json", "RenderCore", "ProceduralMeshComponent",
  "Slate", "SlateCore", "ApplicationCore",
  // Fuer den StaticMesh-Import: FMeshDescription und FStaticMeshAttributes
  "MeshDescription", "StaticMeshDescription",
  // Fuer den fahrbaren Wagen: echte Chaos-Vehicle-Simulation statt Federstrahlen
  "ChaosVehicles", "PhysicsCore" });
 }
}
