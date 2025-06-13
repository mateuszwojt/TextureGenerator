using UnrealBuildTool;

public class TextureGenerator : ModuleRules
{
    public TextureGenerator(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
        
        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "UnrealEd",
                "LevelEditor",
                "Slate",
                "SlateCore",
                "EditorStyle",
                "AssetTools",
                "AssetRegistry",
                "HTTP",
                "ImageCore",
                "Json",
                "JsonUtilities",
                "Projects",
                "PropertyEditor",
                "ImageWrapper",
                "ImageWriteQueue",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "EditorStyle",
                "AssetTools",
                "AssetRegistry",
                "ContentBrowser",
                "CoreUObject",
                "Engine"
            }
            );
            
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "EditorFramework",
                "ToolMenus",
                "ContentBrowser",
                "MainFrame"
            }
            );
    }
}
