// Copyright Mateusz Wojt. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

/** 
 * Implements the visual style of the Texture Generator plugin
 */
class FTextureGeneratorStyle
{
public:
    // Initialize the style set
    static void Initialize();
    
    // Clean up the style set
    static void Shutdown();
    
    // Reloads textures used by slate renderer
    static void ReloadTextures();
    
    // Returns the style set name
    static FName GetStyleSetName();
    
    // Returns the singleton instance of the style set
    static const ISlateStyle& Get();
    
    // Helper function to get a brush by name
    static const FSlateBrush* GetBrush(FName PropertyName, const ANSICHAR* Specifier = NULL, const ISlateStyle* RequestingStyle = nullptr);
    
private:
    // Creates the style set
    static TSharedRef<class FSlateStyleSet> Create();
    
    // Singleton instance of the style set
    static TSharedPtr<class FSlateStyleSet> StyleInstance;
};
