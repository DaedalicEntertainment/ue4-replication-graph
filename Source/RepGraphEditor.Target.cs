using UnrealBuildTool;
using System.Collections.Generic;

public class RepGraphEditorTarget : TargetRules
{
	public RepGraphEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "RepGraph" } );
	}
}
