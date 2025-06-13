// Copyright Mateusz Wojt. All Rights Reserved.

#include "TextureGeneratorStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FTextureGeneratorStyle::StyleInstance = nullptr;

void FTextureGeneratorStyle::Initialize()
{
    if (!StyleInstance.IsValid())
    {
        StyleInstance = Create();
        FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
    }
}

void FTextureGeneratorStyle::Shutdown()
{
    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    ensure(StyleInstance.IsUnique());
    StyleInstance.Reset();
}

FName FTextureGeneratorStyle::GetStyleSetName()
{
    static FName StyleSetName(TEXT("TextureGeneratorStyle"));
    return StyleSetName;
}

const ISlateStyle& FTextureGeneratorStyle::Get()
{
    return *StyleInstance;
}

const FSlateBrush* FTextureGeneratorStyle::GetBrush(FName PropertyName, const ANSICHAR* Specifier, const ISlateStyle* RequestingStyle)
{
    return StyleInstance->GetBrush(PropertyName, Specifier, RequestingStyle);
}

void FTextureGeneratorStyle::ReloadTextures()
{
    if (FSlateApplication::IsInitialized())
    {
        FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
    }
}

TSharedRef<FSlateStyleSet> FTextureGeneratorStyle::Create()
{
    const FVector2D Icon40x40(40.0f, 40.0f);
    const FVector2D Icon128x128(128.0f, 128.0f);
    
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("TextureGeneratorStyle"));
    Style->SetContentRoot(IPluginManager::Get().FindPlugin("TextureGenerator")->GetBaseDir() / TEXT("Resources"));
    
    // Register the plugin's icon
    Style->Set("TextureGenerator.PluginAction", new IMAGE_BRUSH_SVG(TEXT("Icon128"), Icon40x40));
    Style->Set("TextureGenerator.Logo", new IMAGE_BRUSH_SVG(TEXT("Icon128"), Icon128x128));
    
    return Style;
}
