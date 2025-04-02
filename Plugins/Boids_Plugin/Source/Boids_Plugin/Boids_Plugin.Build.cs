// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Boids_Plugin : ModuleRules
{
    public Boids_Plugin(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;


        PrivateIncludePaths.AddRange(new string[]
            {
                "Runtime/Renderer/Private",
                "Boids_Plugin/Private"
            });

        if (Target.bBuildEditor) { PublicDependencyModuleNames.Add("TargetPlatform"); }

        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "Engine",
            "MaterialShaderQualitySettings",
            "CoreUObject",
            "Renderer",
            "RenderCore",
            "RHI",
            "Projects"
        });

        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new string[]
            {
                "UnrealEd",
                "MaterialUtilities",
                "SlateCore",
                "Slate"
            });

            CircularlyReferencedDependentModules.AddRange(new string[] 
            {
                "UnrealEd",
                "MaterialUtilities",
            });
        }
    }
}
