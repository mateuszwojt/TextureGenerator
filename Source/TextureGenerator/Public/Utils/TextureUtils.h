// Copyright Mateusz Wojt. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"

/**
 * Utility class for texture and material creation
 */
class TEXTUREGENERATOR_API FTextureUtils
{
public:
    /**
     * Creates a new texture from raw image data
     * @param ImageData The raw image data
     * @param BaseName Base name for the new texture
     * @param OutPackageName Output parameter for the created package name
     * @return The created texture, or nullptr if creation failed
     */
    static UTexture2D* CreateTextureFromImageData(const TArray<uint8>& ImageData, const FString& BaseName, FString& OutPackageName);
    
    /**
     * Creates a new material with the given texture as the base color
     * @param Texture The texture to use as the base color
     * @param BaseName Base name for the new material
     * @param OutPackageName Output parameter for the created package name
     * @return The created material, or nullptr if creation failed
     */
    static UMaterial* CreateMaterialForTexture(UTexture2D* Texture, const FString& BaseName, FString& OutPackageName);

    /**
    * Extracts UTexture raw image data into PNG compressed binary representation.
    * @param Texture The texture to extract raw image data from.
    * @return Raw image data, converted into PNG format.
    */
    static TArray64<uint8> GetTextureImageData(UTexture2D* Texture);
};
