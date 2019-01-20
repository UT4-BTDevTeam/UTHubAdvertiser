
namespace UnrealBuildTool.Rules
{
	public class UTHubAdvertiser : ModuleRules
	{
		public UTHubAdvertiser(TargetInfo Target)
        {
            PrivateIncludePaths.Add("UTHubAdvertiser/Private");

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
                    "Engine",
					"Http",
					"Json",
					"JsonUtilities",
					"UnrealTournament",
				}
			);
		}
	}
}