// Copyright Mateusz Wojt. All Rights Reserved.

#include "CoreMinimal.h"
#include "TextureGeneratorSettings.generated.h"

UCLASS(Config = TextureGeneratorSettings, DefaultConfig, NotPlaceable)
class TEXTUREGENERATOR_API UTextureGeneratorSettings : public UObject
{
	GENERATED_BODY()

public:
	/* Stability API Key. Required to run the image generation process. Visit your Stability AI account page to generate the API key and paste it here. */
	UPROPERTY(Config, EditAnywhere, Category = "API", Meta = (DisplayName="API Key"))
	FString APIKey;

	/* Default path where the generated assets are going to be saved. Use a trailing slash at the end of the path. */
	UPROPERTY(Config, EditAnywhere, Category = "Paths", Meta = (DisplayName="Default Asset Path"))
	FString DefaultAssetPath = TEXT("/Game/StabilityAI/");
};