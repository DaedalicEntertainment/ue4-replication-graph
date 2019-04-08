using UnrealBuildTool;
using System.Collections.Generic;

public class RepGraphTarget : TargetRules
{
	public RepGraphTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "RepGraph" } );
	}
}
