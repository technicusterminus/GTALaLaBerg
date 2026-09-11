using UnrealBuildTool;
public class GTALaLaBergTarget : TargetRules {
 public GTALaLaBergTarget(TargetInfo Target) : base(Target) {
 Type = TargetType.Game; DefaultBuildSettings = BuildSettingsVersion.Latest;
 IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
 ExtraModuleNames.Add("LaLaBerg");
 }
}
