using UnrealBuildTool;
public class GTALaLaBergEditorTarget : TargetRules {
 public GTALaLaBergEditorTarget(TargetInfo Target) : base(Target) {
 Type = TargetType.Editor; DefaultBuildSettings = BuildSettingsVersion.Latest;
 IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
 ExtraModuleNames.Add("LaLaBerg");
 }
}
