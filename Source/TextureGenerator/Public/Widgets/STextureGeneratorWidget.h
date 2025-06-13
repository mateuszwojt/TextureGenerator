// Copyright Mateusz Wojt. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PropertyCustomizationHelpers.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SSlider.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "AssetRegistry/AssetData.h"
#include "API/StabilityAPIClient.h"


class FAssetThumbnailPool;
class SAssetDropTarget;

/**
 * Main widget for the Texture Generator
 */
class STextureGeneratorWidget : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(STextureGeneratorWidget) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    // UI Components
    TSharedPtr<SEditableTextBox> PromptTextBox;
    TSharedPtr<SEditableTextBox> NegativePromptTextBox;
    TSharedPtr<SComboBox<TSharedPtr<EImageGenerationModel>>> ModelComboBox;
    TSharedPtr<SNumericEntryBox<int32>> WidthBox;
    TSharedPtr<SNumericEntryBox<int32>> HeightBox;
    TSharedPtr<SObjectPropertyEntryBox> ReferenceTextureSelector;
    TSharedPtr<SSlider> StrengthSlider;
    TSharedPtr<SBox> StrengthContainer;
    TSharedPtr<SBox> ProgressBarContainer;
    TSharedPtr<SProgressBar> ProgressBar;
    TSharedPtr<SButton> GenerateButton;
    TSharedPtr<SButton> CancelButton;

    TSharedPtr<FAssetThumbnailPool> AssetThumbnailPool;

    // Model selection data
    TArray<TSharedPtr<EImageGenerationModel>> ModelOptions;
    TSharedPtr<EImageGenerationModel> SelectedModelOption;

    // Reference texture asset handle
    TWeakObjectPtr<UTexture2D> SelectedReferenceTexture;

    // UI Generation
    TSharedRef<SWidget> CreateHeader();
    TSharedRef<SWidget> CreatePromptSection();
    TSharedRef<SWidget> CreateImageSettingsSection();
    TSharedRef<SWidget> CreateActionButtons();

    // Model combobox handlers
    TSharedRef<SWidget> MakeModelComboWidget(TSharedPtr<EImageGenerationModel> InOption);
    FText GetModelComboText() const;
    FText GetModelDisplayName(EImageGenerationModel Model) const;
    FText GetModelDescription(EImageGenerationModel Model) const;
    void OnModelSelectionChanged(TSharedPtr<EImageGenerationModel> NewSelection, ESelectInfo::Type SelectInfo);

    // Reference image handlers
    void OnReferenceTextureChanged(const FAssetData& AssetData);

protected:
    // Stability AI API Client
    TUniquePtr<FStabilityAPIClient> Client;
    
    // Event Handlers
    FReply OnGenerateClicked();
    FReply OnCancelClicked();
    
    // API Callbacks
    void OnImageGenerated(const TArray<uint8>& ImageData);
    void OnGenerationError(const FString& ErrorMessage);

    float Strength = 0.5f;
    float GenerationProgress = 0.0f;
    bool bInProgress = false;

    // We simulate the progress to give user some visual feedback since Stability AI does not provide a way to track it
    FDateTime ProgressSimulationStartTime;
    FTimerHandle ProgressUpdateHandle;

private:
    // Methods for progress simulation
    void StartProgressSimulation();
    void StopProgressSimulation();
    void UpdateProgressSimulation();
};
