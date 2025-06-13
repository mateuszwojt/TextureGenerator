// Copyright Mateusz Wojt. All Rights Reserved.

#include "API/StabilityAPIClient.h"
#include "Misc/Base64.h"
#include "Misc/FileHelper.h"
#include "Engine/Texture2D.h"
#include "Utils/TextureUtils.h"
#include "TextureGeneratorModule.h"

FStabilityAPIClient::FStabilityAPIClient()
    : CurrentModel(EImageGenerationModel::StableImageCore)
    , ReferenceTexture(nullptr)
    , CurrentSeed(-1)
    , CurrentStrength(0)
{
}

FStabilityAPIClient::~FStabilityAPIClient()
{
    CancelRequest();
}

void FStabilityAPIClient::SetAPIKey(const FString& InAPIKey)
{
    APIKey = InAPIKey;
}

void FStabilityAPIClient::GenerateImage(
    const FString& InPrompt,
    const FString& InNegativePrompt,
    UTexture2D* InReferenceTexture,
    float InStrength,
    EImageGenerationModel InModel)
{
    // Cancel any existing request
    CancelRequest();

    // Store parameters
    CurrentPrompt = InPrompt;
    CurrentNegativePrompt = InNegativePrompt;
    ReferenceTexture = InReferenceTexture;
    CurrentStrength = InStrength;
    CurrentModel = InModel;

    // Convert texture to raw image data
    if (IsValid(ReferenceTexture))
    {
        CurrentReferenceImage = FTextureUtils::GetTextureImageData(ReferenceTexture);
    }
    else
    {
        // Clean-up any existing handle to the image, so no leftovers are used when sending API queries
        CurrentReferenceImage.Empty();
    }

    // Create the HTTP request
    CurrentRequest = FHttpModule::Get().CreateRequest();
    if (!CurrentRequest.IsValid())
    {
        OnError.ExecuteIfBound(TEXT("Failed to create HTTP request"));
        return;
    }

    // Set up the request URL with API key
    FString Url = GetModelEndpoint(CurrentModel);
    CurrentRequest->SetURL(Url);
    CurrentRequest->SetVerb(TEXT("POST"));

    // Set authorization header with Bearer token
    CurrentRequest->SetHeader(TEXT("Authorization"), FString::Printf(TEXT("Bearer %s"), *APIKey));

    // IMPORTANT: Request binary image response, NOT JSON
    CurrentRequest->SetHeader(TEXT("Accept"), TEXT("image/*"));

    // Build multipart form data
    const FString Boundary = FString::Printf(TEXT("----formdata-unreal-%d"), FMath::Rand());
    CurrentRequest->SetHeader(TEXT("Content-Type"), FString::Printf(TEXT("multipart/form-data; boundary=%s"), *Boundary));

    // Build the multipart body
    TArray<uint8> RequestBody = BuildMultipartFormData(Boundary);
    CurrentRequest->SetContent(RequestBody);
        
    // Bind the response callback
    CurrentRequest->OnProcessRequestComplete().BindRaw(this, &FStabilityAPIClient::OnResponseReceived);
        
    // Send the request
    if (!CurrentRequest->ProcessRequest())
    {
        OnError.ExecuteIfBound(TEXT("Failed to process HTTP request"));
        CurrentRequest.Reset();
    }
}

void FStabilityAPIClient::CancelRequest()
{
    if (CurrentRequest.IsValid() && CurrentRequest->GetStatus() == EHttpRequestStatus::Processing)
    {
        CurrentRequest->CancelRequest();
    }
    CurrentRequest.Reset();
}

void FStabilityAPIClient::OnResponseReceived(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
{
    // Clear the current request
    CurrentRequest.Reset();
        
    // Check if the request was successful
    if (!bWasSuccessful || !Response.IsValid())
    {
        OnError.ExecuteIfBound(TEXT("Request failed"));
        return;
    }
        
    // Check the response code
    const int32 ResponseCode = Response->GetResponseCode();
    if (ResponseCode != 200)
    {
        // For errors, the response might be JSON with error details
        const FString ResponseStr = Response->GetContentAsString();
        UE_LOG(LogTextureGenerator, Error, TEXT("API Error Response: %s"), *ResponseStr);
        
        FString ErrorMessage = FString::Printf(TEXT("Texture generation request failed with code %d: %s"), 
             ResponseCode, *ResponseStr);
        OnError.ExecuteIfBound(ErrorMessage);
        return;
    }
        
    // Process the binary image response
    ProcessStabilityResponse(Response);
}

void FStabilityAPIClient::ProcessStabilityResponse(const FHttpResponsePtr& Response)
{
    if (!Response.IsValid())
    {
        OnError.ExecuteIfBound(TEXT("Invalid response"));
        return;
    }

    // Get the raw binary data from the response
    const TArray<uint8>& ResponseData = Response->GetContent();
    
    if (ResponseData.Num() == 0)
    {
        OnError.ExecuteIfBound(TEXT("Empty response data"));
        return;
    }
    
    // The response data is already the binary image data - pass it directly
    OnImageGenerated.ExecuteIfBound(ResponseData);
}

FString FStabilityAPIClient::GetModelEndpoint(EImageGenerationModel Model) const
{
    switch (Model)
    {
    case EImageGenerationModel::StableImageUltra:
        return TEXT("https://api.stability.ai/v2beta/stable-image/generate/ultra");
    case EImageGenerationModel::StableImageCore:
        return TEXT("https://api.stability.ai/v2beta/stable-image/generate/core");
    case EImageGenerationModel::StableDiffusion:
        return TEXT("https://api.stability.ai/v2beta/stable-image/generate/sd3");
    default:
        return TEXT("");
    }
}

TArray<uint8> FStabilityAPIClient::BuildMultipartFormData(const FString& Boundary) const
{
    TArray<uint8> FormData;
    const FString LineEnding = TEXT("\r\n");

    // Helper lambda to append string data as UTF-8 bytes
    auto AppendString = [&](const FString& Str) {
        FTCHARToUTF8 UTF8String(*Str);
        FormData.Append(reinterpret_cast<const uint8*>(UTF8String.Get()), UTF8String.Length());
    };
    
    // Helper lambda to append binary data directly
    auto AppendBinary = [&](const TArray<uint8>& Data) {
        FormData.Append(Data);
    };

    // Add prompt field
    AppendString(FString::Printf(TEXT("--%s%s"), *Boundary, *LineEnding));
    AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"prompt\"%s%s"), *LineEnding, *LineEnding));
    AppendString(FString::Printf(TEXT("%s%s"), *CurrentPrompt, *LineEnding));

    // Add output format
    AppendString(FString::Printf(TEXT("--%s%s"), *Boundary, *LineEnding));
    AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"output_format\"%s%s"), *LineEnding, *LineEnding));
    AppendString(FString::Printf(TEXT("png%s"), *LineEnding));

    // Add negative prompt if set
    if (!CurrentNegativePrompt.IsEmpty())
    {
        AppendString(FString::Printf(TEXT("--%s%s"), *Boundary, *LineEnding));
        AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"negative_prompt\"%s%s"), *LineEnding, *LineEnding));
        AppendString(FString::Printf(TEXT("%s%s"), *CurrentNegativePrompt, *LineEnding));
    }

    // Add seed if specified (> 0)
    if (CurrentSeed > 0)
    {
        AppendString(FString::Printf(TEXT("--%s%s"), *Boundary, *LineEnding));
        AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"seed\"%s%s"), *LineEnding, *LineEnding));
        AppendString(FString::Printf(TEXT("%d%s"), CurrentSeed, *LineEnding));
    }

    // Add reference image (for img2img workflows)
    if (!CurrentReferenceImage.IsEmpty())
    {
        AppendString(FString::Printf(TEXT("--%s%s"), *Boundary, *LineEnding));
        AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"image\"; filename=\"%s\"%s"), 
            TEXT("reference.png"), *LineEnding));
        AppendString(FString::Printf(TEXT("Content-Type: image/png%s%s"), *LineEnding, *LineEnding));
        
        // Convert binary data to string representation
        AppendBinary(CurrentReferenceImage);
        AppendString(LineEnding);

        // Strength param is required when passing a reference image.
        // A value of 0 would yield an image that is identical to the input. A value of 1 would be as if you passed in no image at all.
        AppendString(FString::Printf(TEXT("--%s%s"), *Boundary, *LineEnding));
        AppendString(FString::Printf(TEXT("Content-Disposition: form-data; name=\"strength\"%s%s"), *LineEnding, *LineEnding));
        AppendString(FString::Printf(TEXT("%.1f%s"), FMath::Clamp(CurrentStrength, 0.0f, 1.0f), *LineEnding));
    }

    // End boundary
    AppendString(FString::Printf(TEXT("--%s--%s"), *Boundary, *LineEnding));

    return FormData;
}