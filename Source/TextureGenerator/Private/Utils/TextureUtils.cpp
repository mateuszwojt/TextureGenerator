// Copyright Mateusz Wojt. All Rights Reserved.

#include "Utils/TextureUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "TextureGeneratorModule.h"
#include "TextureGeneratorSettings.h"
#include "Engine/Texture2D.h"
#include "Factories/MaterialFactoryNew.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"


UTexture2D* FTextureUtils::CreateTextureFromImageData(const TArray<uint8>& ImageData, const FString& BaseName, FString& OutPackageName)
{
    if (ImageData.Num() == 0)
    {
        UE_LOG(LogTextureGenerator, Error, TEXT("Image data is empty."));
        return nullptr;
    }

    IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
    TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

    // Set the compressed data for the image wrapper
    if (!ImageWrapper.IsValid() || !ImageWrapper->SetCompressed(ImageData.GetData(), ImageData.Num()))
    {
        UE_LOG(LogTemp, Error, TEXT("Cannot set the compressed image data."));
        return nullptr;
    }

    int32 Width = ImageWrapper->GetWidth();
    int32 Height = ImageWrapper->GetHeight();
    
    if (Width <= 0 || Height <= 0)
    {
        UE_LOG(LogTextureGenerator, Error, TEXT("Invalid image dimensions: %dx%d"), Width, Height);
        return nullptr;
    }

    // Get the raw image data
    TArray<uint8> RawData;
    if (!ImageWrapper->GetRaw(ERGBFormat::BGRA, 8, RawData))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to get raw image data"));
        return nullptr;
    }

    // Create a unique package name
    FString PackagePath = GetMutableDefault<UTextureGeneratorSettings>()->DefaultAssetPath;
    FString TextureName = FString::Printf(TEXT("T_%s"), *BaseName);
    OutPackageName = PackagePath + TextureName;

    // Create the package
    UPackage* Package = CreatePackage(*OutPackageName);
    if (!Package)
    {
        UE_LOG(LogTextureGenerator, Error, TEXT("Failed to create package: %s"), *OutPackageName);
        return nullptr;
    }

    // Create a new texture in the package
    UTexture2D* NewTexture = NewObject<UTexture2D>(
        Package,
        FName(*TextureName),
        RF_Public | RF_Standalone
    );

    if (!NewTexture)
    {
        UE_LOG(LogTextureGenerator, Error, TEXT("Failed to create texture object"));
        return nullptr;
    }

    // Set the texture properties
    NewTexture->NeverStream = true;
    NewTexture->CompressionSettings = TC_Default;
    NewTexture->SRGB = true;
    NewTexture->MipGenSettings = TMGS_NoMipmaps;
    NewTexture->AddressX = TA_Clamp;
    NewTexture->AddressY = TA_Clamp;

    // Initialize the texture source with the decoded data
    FTextureSource& TextureSource = NewTexture->Source;
    
    // Use BGRA8 format since we decoded to BGRA
    TextureSource.Init(
        Width,
        Height,
        1, // NumSlices
        1, // NumMips
        TSF_BGRA8,
        RawData.GetData()
    );

    // Update the texture resource
    NewTexture->UpdateResource();

    // Mark the package dirty so it will be saved
    Package->MarkPackageDirty();

    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(NewTexture);

    return NewTexture;
}

UMaterial* FTextureUtils::CreateMaterialForTexture(UTexture2D* Texture, const FString& BaseName, FString& OutPackageName)
{
    if (!Texture)
    {
        UE_LOG(LogTextureGenerator, Error, TEXT("Invalid texture object passed. Cannot create material."));
        return nullptr;
    }

    // Get the asset tools module
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    
    // Create a unique name for the material
    FString MaterialName = TEXT("M_" + BaseName);
    FString PackagePath = GetMutableDefault<UTextureGeneratorSettings>()->DefaultAssetPath;
    OutPackageName = PackagePath + MaterialName;
    
    // Create a new material
    UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
    UMaterial* NewMaterial = (UMaterial*)AssetTools.CreateAsset(
        MaterialName,
        PackagePath,
        UMaterial::StaticClass(),
        MaterialFactory
    );
    
    if (!NewMaterial)
    {
        return nullptr;
    }
    
    // Set up the material
    NewMaterial->Modify();
    
    // Create a texture parameter for the base color
    UMaterialExpressionTextureSampleParameter2D* TextureSample = NewObject<UMaterialExpressionTextureSampleParameter2D>(NewMaterial);
    TextureSample->ParameterName = FName(*FString::Printf(TEXT("BaseColor_%s"), *FGuid::NewGuid().ToString().Left(8)));
    TextureSample->Texture = Texture;
    TextureSample->SamplerType = SAMPLERTYPE_Color;
    // Offset the position of the node in the material graph
    TextureSample->MaterialExpressionEditorX = -400;
    TextureSample->MaterialExpressionEditorY = 0;

    // CRITICAL: Add the expression to the material's expression collection, otherwise connected texture won't trigger the material compilation
    NewMaterial->GetEditorOnlyData()->ExpressionCollection.Expressions.Add(TextureSample);
    
    // Connect the texture to the base color
    FExpressionOutput& Output = TextureSample->GetOutputs()[0];
    FExpressionInput& BaseColorInput = NewMaterial->GetEditorOnlyData()->BaseColor;
    BaseColorInput.Expression = TextureSample;
    BaseColorInput.OutputIndex = 0;  // Use OutputIndex instead of individual mask components
    BaseColorInput.Mask = 0;
    BaseColorInput.MaskR = 1;
    BaseColorInput.MaskG = 1;
    BaseColorInput.MaskB = 1;
    BaseColorInput.MaskA = 0;
    
    // Set some default properties
    NewMaterial->SetShadingModel(MSM_DefaultLit);
    NewMaterial->TwoSided = false;
    NewMaterial->BlendMode = BLEND_Opaque;
    
    // Compile the material
    NewMaterial->PreEditChange(nullptr);
    NewMaterial->ForceRecompileForRendering();
    NewMaterial->PostEditChange();
    NewMaterial->MarkPackageDirty();
    
    // Notify the asset registry
    FAssetRegistryModule::AssetCreated(NewMaterial);
    
    return NewMaterial;
}

TArray64<uint8> FTextureUtils::GetTextureImageData(UTexture2D* Texture)
{
    TArray64<uint8> OutData;

    FImage Image;
    if (IsValid(Texture) && FImageUtils::GetTexture2DSourceImage(Texture, Image))
    {
        if (!FImageUtils::CompressImage(OutData, TEXT("png"), Image))
        {
            UE_LOG(LogTextureGenerator, Error, TEXT("Failed compressing raw image data using PNG format."));
        }
    }

    return OutData;
}
