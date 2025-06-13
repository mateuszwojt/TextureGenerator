// Copyright Mateusz Wojt. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

UENUM(BlueprintType)
enum class EImageGenerationModel : uint8
{
    StableImageUltra UMETA(DisplayName = "Stable Image Ultra"),
    StableImageCore UMETA(DisplayName = "Stable Image Core"),
    StableDiffusion UMETA(DisplayName = "Stable Diffusion 3.5")
};

DECLARE_DELEGATE_OneParam(FOnImageGenerated, const TArray<uint8>&);
DECLARE_DELEGATE_OneParam(FOnError, const FString&);
DECLARE_DELEGATE_OneParam(FOnProgress, float);

class TEXTUREGENERATOR_API FStabilityAPIClient
{
public:
    FStabilityAPIClient();
    ~FStabilityAPIClient();

    // Set the API key for authentication
    void SetAPIKey(const FString& InAPIKey);

    // Generate an image using the Stability AI API
    void GenerateImage(
        const FString& Prompt,
        const FString& NegativePrompt,
        UTexture2D* ReferenceTexture,
        float InStrength,
        EImageGenerationModel Model
    );

    // Cancel the current generation request
    void CancelRequest();

    // Delegates
    FOnImageGenerated OnImageGenerated;
    FOnError OnError;
    FOnProgress OnProgress;

private:
    // Handle the HTTP response
    void OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

    // Process Stability AI response
    void ProcessStabilityResponse(const FHttpResponsePtr& Response);

    FString GetModelEndpoint(EImageGenerationModel Model) const;
    TArray<uint8> BuildMultipartFormData(const FString& Boundary) const;

    // Current HTTP request
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> CurrentRequest;

    // API configuration
    FString APIKey;
    EImageGenerationModel CurrentModel;
    UTexture2D* ReferenceTexture;
    
    // Generation parameters
    FString CurrentPrompt;
    FString CurrentNegativePrompt;
    TArray<uint8> CurrentReferenceImage;
    int32 CurrentSeed;      // -1 for random, >0 for specific seed
    float CurrentStrength;  // 0-1, for img2img influence
};
