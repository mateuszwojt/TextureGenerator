// Copyright Mateusz Wojt. All Rights Reserved.

#include "Widgets/STextureGeneratorWidget.h"
#include "TextureGeneratorStyle.h"
#include "TextureGeneratorSettings.h"
#include "TextureGeneratorModule.h"
#include "Utils/TextureUtils.h"

#include "Async/Async.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "AssetThumbnail.h"
#include "Editor.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "Misc/Paths.h"
#include "Misc/MessageDialog.h"
#include "HAL/FileManager.h"
#include "Materials/Material.h"

#define LOCTEXT_NAMESPACE "TextureGenerator"

void STextureGeneratorWidget::Construct(const FArguments& InArgs)
{
    // Initialize thumbnail pool for the texture picker
    AssetThumbnailPool = MakeShareable(new FAssetThumbnailPool(24));
    
    // Initialize the Stability AI API client
    Client = MakeUnique<FStabilityAPIClient>();
    const FString APIKey = GetMutableDefault<UTextureGeneratorSettings>()->APIKey;
    if (APIKey.IsEmpty())
    {
        FMessageDialog::Open(EAppMsgType::Ok, FText::FromString("Stability API Key not set. Go to Project Settings -> Stability AI Image Generator -> and fill in the API key parameter."));
    }
    Client->SetAPIKey(APIKey);
    
    // Bind API callbacks
    Client->OnImageGenerated.BindRaw(this, &STextureGeneratorWidget::OnImageGenerated);
    Client->OnError.BindRaw(this, &STextureGeneratorWidget::OnGenerationError);

    // Initialize model selection options
    ModelOptions.Add(MakeShareable(new EImageGenerationModel(EImageGenerationModel::StableImageUltra)));
    ModelOptions.Add(MakeShareable(new EImageGenerationModel(EImageGenerationModel::StableImageCore)));
    ModelOptions.Add(MakeShareable(new EImageGenerationModel(EImageGenerationModel::StableDiffusion)));
    SelectedModelOption = ModelOptions[0];

    // Initialize style selection options
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::None)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Model3D)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::AnalogFilm)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Anime)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Cinematic)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::ComicBook)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::DigitalArt)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Enhance)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::FantasyArt)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Isometric)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::LineArt)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::LowPoly)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::ModelingCompound)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::NeonPunk)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Origami)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::Photographic)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::PixelArt)));
    StyleOptions.Add(MakeShareable(new EStylePreset(EStylePreset::TileTexture)));
    SelectedStyleOption = StyleOptions[0]; // Default to None

    // Initialize progress variables
    bInProgress = false;
    GenerationProgress = 0.0f;
    
    // Create the main container
    ChildSlot
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            CreateHeader()
        ]
        + SVerticalBox::Slot()
        .FillHeight(1.0f)
        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
        [
            SNew(SScrollBox)
            + SScrollBox::Slot()
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    CreatePromptSection()
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 16.0f, 0.0f, 0.0f)
                [
                    CreateImageSettingsSection()
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
        [
            CreateActionButtons()
        ]
    ];
}

TSharedRef<SWidget> STextureGeneratorWidget::CreateHeader()
{
    return 
        SNew(SBox)
        .HeightOverride(48)
        .Padding(FMargin(10.0f, 0.0f, 16.0f, 8.0f))
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("PluginTitle", "Stability AI Texture Generator"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
                ]
            ]
        ];
}

TSharedRef<SWidget> STextureGeneratorWidget::CreatePromptSection()
{
    return 
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("PromptLabel", "Prompt"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
        [
            SAssignNew(PromptTextBox, SEditableTextBox)
            .HintText(LOCTEXT("PromptHint", "Describe the image you want to generate..."))
            .SelectAllTextWhenFocused(true)
            .AllowContextMenu(true)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 8.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("NegativePromptLabel", "Negative Prompt (Optional)"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
        [
            SAssignNew(NegativePromptTextBox, SEditableTextBox)
            .HintText(LOCTEXT("NegativePromptHint", "What to avoid in the generated image..."))
            .AllowContextMenu(true)
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 12.0f, 0.0f, 0.0f)
        [
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ModelLabel", "AI Model"))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SAssignNew(ModelComboBox, SComboBox<TSharedPtr<EImageGenerationModel>>)
                .OptionsSource(&ModelOptions)
                .OnGenerateWidget(this, &STextureGeneratorWidget::MakeModelComboWidget)
                .OnSelectionChanged(this, &STextureGeneratorWidget::OnModelSelectionChanged)
                .InitiallySelectedItem(SelectedModelOption)
                [
                    SNew(STextBlock)
                    .Text(this, &STextureGeneratorWidget::GetModelComboText)
                ]
            ]
        ];
}

TSharedRef<SWidget> STextureGeneratorWidget::CreateImageSettingsSection()
{
    return 
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ImageSettingsLabel", "Image Settings"))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
        [
            SAssignNew(ReferenceTextureSelector, SObjectPropertyEntryBox)
            .AllowedClass(UTexture2D::StaticClass())
            .ObjectPath_Lambda([this]() -> FString
            {
                return SelectedReferenceTexture.IsValid() ? 
                    SelectedReferenceTexture->GetPathName() : 
                    FString();
            })
            .OnObjectChanged(this, &STextureGeneratorWidget::OnReferenceTextureChanged)
            .AllowClear(true)
            .DisplayUseSelected(true)
            .DisplayBrowse(true)
            .DisplayThumbnail(true)
            .ThumbnailPool(AssetThumbnailPool)
            .NewAssetFactories(TArray<UFactory*>())
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 4.0f, 0.0f, 0.0f)
        [
            SNew(STextBlock)
            .Text(LOCTEXT("ReferenceTextureHint", "Select a texture to use as reference for image-to-image generation"))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 12.0f, 0.0f, 0.0f)
        [
            // Strength slider container - only visible when reference texture is selected
            SAssignNew(StrengthContainer, SBox)
            .Visibility_Lambda([this]() -> EVisibility
            {
                return SelectedReferenceTexture.IsValid() ? EVisibility::Visible : EVisibility::Collapsed;
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("StrengthLabel", "Strength"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot()
                    .FillWidth(1.0f)
                    .VAlign(VAlign_Center)
                    [
                        SAssignNew(StrengthSlider, SSlider)
                        .Value_Lambda([this]() -> float
                        {
                            return Strength;
                        })
                        .OnValueChanged_Lambda([this](float NewValue)
                        {
                            Strength = NewValue;
                        })
                        .MinValue(0.0f)
                        .MaxValue(1.0f)
                        .StepSize(0.01f)
                        .SliderBarColor(FLinearColor(0.1f, 0.5f, 0.9f, 1.0f))
                        .SliderHandleColor(FLinearColor::White)
                    ]
                    + SHorizontalBox::Slot()
                    .AutoWidth()
                    .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                    .VAlign(VAlign_Center)
                    [
                        SNew(SBox)
                        .WidthOverride(50.0f)
                        [
                            SNew(STextBlock)
                            .Text_Lambda([this]() -> FText
                            {
                                return FText::AsNumber(FMath::RoundToInt(Strength * 100.0f));
                            })
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                            .Justification(ETextJustify::Center)
                        ]
                    ]
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 4.0f, 0.0f, 0.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("StrengthHint", "Controls how much the generated image follows the reference texture (0 = ignore reference, 100 = closely follow reference)"))
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                    .AutoWrapText(true)
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 12.0f, 0.0f, 0.0f)
        [
            // Seed input section
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SeedLabel", "Seed"))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(SeedEntryBox, SNumericEntryBox<int32>)
                    .Value_Lambda([this]() -> TOptional<int32>
                    {
                        return GenerationSeed >= 0 ? TOptional<int32>(GenerationSeed) : TOptional<int32>();
                    })
                    .OnValueChanged_Lambda([this](int32 NewValue)
                    {
                        GenerationSeed = NewValue;
                    })
                    .OnValueCommitted_Lambda([this](int32 NewValue, ETextCommit::Type CommitType)
                    {
                        GenerationSeed = NewValue;
                    })
                    .AllowSpin(true)
                    .MinValue(0)
                    .MaxValue(2147483647) // Max int32
                    .MinSliderValue(0)
                    .MaxSliderValue(1000000)
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("RandomSeedButton", "Random"))
                    .OnClicked_Lambda([this]() -> FReply
                    {
                        GenerationSeed = FMath::Rand();
                        return FReply::Handled();
                    })
                    .ToolTipText(LOCTEXT("RandomSeedTooltip", "Generate a random seed value"))
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("SeedHintText", "Seed controls randomness. Same seed with same settings produces identical results. Leave empty for random."))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                .AutoWrapText(true)
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 12.0f, 0.0f, 0.0f)
        [
            // Style preset section
            SNew(SVerticalBox)
            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("StyleLabel", "Style Preset"))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SAssignNew(StyleComboBox, SComboBox<TSharedPtr<EStylePreset>>)
                .OptionsSource(&StyleOptions)
                .OnGenerateWidget(this, &STextureGeneratorWidget::MakeStyleComboWidget)
                .OnSelectionChanged(this, &STextureGeneratorWidget::OnStyleSelectionChanged)
                .InitiallySelectedItem(SelectedStyleOption)
                [
                    SNew(STextBlock)
                    .Text(this, &STextureGeneratorWidget::GetStyleComboText)
                ]
            ]
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0.0f, 4.0f, 0.0f, 0.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("StyleHintText", "Choose a style preset to influence the visual style of the generated image"))
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
                .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                .AutoWrapText(true)
            ]
        ];
}

TSharedRef<SWidget> STextureGeneratorWidget::CreateActionButtons()
{
    return 
        SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        .Padding(0.0f, 0.0f, 0.0f, 8.0f)
        [
            // Progress bar - only visible when generating
            SAssignNew(ProgressBarContainer, SBox)
            .Visibility_Lambda([this]() -> EVisibility
            {
                return bInProgress ? EVisibility::Visible : EVisibility::Collapsed;
            })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0.0f, 0.0f, 0.0f, 4.0f)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]() -> FText
                    {
                        return FText::Format(LOCTEXT("GeneratingProgress", "Generating image... {0}%"), 
                            FText::AsNumber(FMath::RoundToInt(GenerationProgress * 100.0f)));
                    })
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                    .ColorAndOpacity(FSlateColor::UseSubduedForeground())
                ]
                + SVerticalBox::Slot()
                .AutoHeight()
                [
                    SAssignNew(ProgressBar, SProgressBar)
                    .Percent_Lambda([this]() -> TOptional<float>
                    {
                        return GenerationProgress;
                    })
                    .FillColorAndOpacity(FLinearColor(0.0f, 0.8f, 0.2f, 1.0f)) // Green progress bar
                ]
            ]
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .HAlign(HAlign_Right)
            .Padding(0.0f, 0.0f, 8.0f, 0.0f)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SAssignNew(GenerateButton, SButton)
                    .Text_Lambda([this]() -> FText
                    {
                        return bInProgress ? LOCTEXT("GeneratingButton", "Generating...") : LOCTEXT("GenerateButton", "Generate");
                    })
                    .OnClicked(this, &STextureGeneratorWidget::OnGenerateClicked)
                    .IsEnabled_Lambda([this]()
                    {
                        // Allow generating image only if some prompt has been entered and currently not processing another image
                        return !PromptTextBox->GetText().ToString().IsEmpty() && !bInProgress;
                    })
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .Padding(8.0f, 0.0f, 0.0f, 0.0f)
                [
                    SAssignNew(CancelButton, SButton)
                    .Text(LOCTEXT("CancelButton", "Cancel"))
                    .OnClicked(this, &STextureGeneratorWidget::OnCancelClicked)
                    .IsEnabled_Lambda([this]()
                    {
                        return bInProgress; // Only enable cancel when generation is in progress
                    })
                ]
            ]
        ];
}

TSharedRef<SWidget> STextureGeneratorWidget::MakeModelComboWidget(TSharedPtr<EImageGenerationModel> InOption)
{
    if (!InOption.IsValid())
    {
        return SNew(STextBlock).Text(LOCTEXT("InvalidModel", "Invalid Model"));
    }

    return SNew(SVerticalBox)
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(GetModelDisplayName(*InOption))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 9))
        ]
        + SVerticalBox::Slot()
        .AutoHeight()
        [
            SNew(STextBlock)
            .Text(GetModelDescription(*InOption))
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 8))
            .ColorAndOpacity(FSlateColor::UseSubduedForeground())
        ];
}

FText STextureGeneratorWidget::GetModelComboText() const
{
    if (SelectedModelOption.IsValid())
    {
        return GetModelDisplayName(*SelectedModelOption);
    }
    return LOCTEXT("SelectModel", "Select Model");
}

FText STextureGeneratorWidget::GetModelDisplayName(EImageGenerationModel Model) const
{
    switch (Model)
    {
    case EImageGenerationModel::StableImageUltra:
        return LOCTEXT("StableImageUltraName", "Stable Image Ultra");
    case EImageGenerationModel::StableImageCore:
        return LOCTEXT("StableImageCoreName", "Stable Image Core");
    case EImageGenerationModel::StableDiffusion:
        return LOCTEXT("StableDiffusionName", "Stable Diffusion");
    default:
        return LOCTEXT("UnknownModel", "Unknown Model");
    }
}

FText STextureGeneratorWidget::GetModelDescription(EImageGenerationModel Model) const
{
    switch (Model)
    {
    case EImageGenerationModel::StableImageUltra:
        return LOCTEXT("StableImageUltraDesc", "Most advanced model, highest image quality");
    case EImageGenerationModel::StableImageCore:
        return LOCTEXT("StableImageCoreDesc", "Best quality to speed ratio");
    case EImageGenerationModel::StableDiffusion:
        return LOCTEXT("StableDiffusion3Desc", "Base model");
    default:
        return LOCTEXT("UnknownModelDesc", "Unknown model type");
    }
}

void STextureGeneratorWidget::OnModelSelectionChanged(TSharedPtr<EImageGenerationModel> NewSelection,
    ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        SelectedModelOption = NewSelection;
    }
}

TSharedRef<SWidget> STextureGeneratorWidget::MakeStyleComboWidget(TSharedPtr<EStylePreset> InOption)
{
    if (!InOption.IsValid())
    {
        return SNew(STextBlock).Text(LOCTEXT("InvalidStyle", "Invalid Style"));
    }

    return SNew(STextBlock)
        .Text(GetStyleDisplayName(*InOption))
        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9));
}

FText STextureGeneratorWidget::GetStyleComboText() const
{
    if (SelectedStyleOption.IsValid())
    {
        return GetStyleDisplayName(*SelectedStyleOption);
    }
    return LOCTEXT("SelectStyle", "Select Style");
}

FText STextureGeneratorWidget::GetStyleDisplayName(EStylePreset Style) const
{
    switch (Style)
    {
    case EStylePreset::None:
        return LOCTEXT("StyleNone", "None");
    case EStylePreset::Model3D:
        return LOCTEXT("Style3DModel", "3D Model");
    case EStylePreset::AnalogFilm:
        return LOCTEXT("StyleAnalogFilm", "Analog Film");
    case EStylePreset::Anime:
        return LOCTEXT("StyleAnime", "Anime");
    case EStylePreset::Cinematic:
        return LOCTEXT("StyleCinematic", "Cinematic");
    case EStylePreset::ComicBook:
        return LOCTEXT("StyleComicBook", "Comic Book");
    case EStylePreset::DigitalArt:
        return LOCTEXT("StyleDigitalArt", "Digital Art");
    case EStylePreset::Enhance:
        return LOCTEXT("StyleEnhance", "Enhance");
    case EStylePreset::FantasyArt:
        return LOCTEXT("StyleFantasyArt", "Fantasy Art");
    case EStylePreset::Isometric:
        return LOCTEXT("StyleIsometric", "Isometric");
    case EStylePreset::LineArt:
        return LOCTEXT("StyleLineArt", "Line Art");
    case EStylePreset::LowPoly:
        return LOCTEXT("StyleLowPoly", "Low Poly");
    case EStylePreset::ModelingCompound:
        return LOCTEXT("StyleModelingCompound", "Modeling Compound");
    case EStylePreset::NeonPunk:
        return LOCTEXT("StyleNeonPunk", "Neon Punk");
    case EStylePreset::Origami:
        return LOCTEXT("StyleOrigami", "Origami");
    case EStylePreset::Photographic:
        return LOCTEXT("StylePhotographic", "Photographic");
    case EStylePreset::PixelArt:
        return LOCTEXT("StylePixelArt", "Pixel Art");
    case EStylePreset::TileTexture:
        return LOCTEXT("StyleTileTexture", "Tile Texture");
    default:
        return LOCTEXT("UnknownStyle", "Unknown Style");
    }
}

FString STextureGeneratorWidget::GetStyleAPIString(EStylePreset Style) const
{
    switch (Style)
    {
    case EStylePreset::None:
        return FString();
    case EStylePreset::Model3D:
        return TEXT("3d-model");
    case EStylePreset::AnalogFilm:
        return TEXT("analog-film");
    case EStylePreset::Anime:
        return TEXT("anime");
    case EStylePreset::Cinematic:
        return TEXT("cinematic");
    case EStylePreset::ComicBook:
        return TEXT("comic-book");
    case EStylePreset::DigitalArt:
        return TEXT("digital-art");
    case EStylePreset::Enhance:
        return TEXT("enhance");
    case EStylePreset::FantasyArt:
        return TEXT("fantasy-art");
    case EStylePreset::Isometric:
        return TEXT("isometric");
    case EStylePreset::LineArt:
        return TEXT("line-art");
    case EStylePreset::LowPoly:
        return TEXT("low-poly");
    case EStylePreset::ModelingCompound:
        return TEXT("modeling-compound");
    case EStylePreset::NeonPunk:
        return TEXT("neon-punk");
    case EStylePreset::Origami:
        return TEXT("origami");
    case EStylePreset::Photographic:
        return TEXT("photographic");
    case EStylePreset::PixelArt:
        return TEXT("pixel-art");
    case EStylePreset::TileTexture:
        return TEXT("tile-texture");
    default:
        return FString();
    }
}

void STextureGeneratorWidget::OnStyleSelectionChanged(TSharedPtr<EStylePreset> NewSelection,
    ESelectInfo::Type SelectInfo)
{
    if (NewSelection.IsValid())
    {
        SelectedStyleOption = NewSelection;
    }
}

void STextureGeneratorWidget::OnReferenceTextureChanged(const FAssetData& AssetData)
{
    if (AssetData.IsValid())
    {
        SelectedReferenceTexture = Cast<UTexture2D>(AssetData.GetAsset());
    }
    else
    {
        SelectedReferenceTexture.Reset();
    }
}

FReply STextureGeneratorWidget::OnGenerateClicked()
{
    FString PromptText = PromptTextBox->GetText().ToString();
    FString NegativePromptText = NegativePromptTextBox->GetText().ToString();
    
    if (PromptText.IsEmpty())
    {
        OnGenerationError(TEXT("Prompt text is empty. Please enter some text first to generate the texture."));
        return FReply::Handled();
    }

    // Get the style preset string for the API
    FString StylePreset;
    if (SelectedStyleOption.IsValid() && *SelectedStyleOption != EStylePreset::None)
    {
        StylePreset = GetStyleAPIString(*SelectedStyleOption);
    }

    int32 SeedValue = GenerationSeed;

    // Start progress tracking
    bInProgress = true;
    GenerationProgress = 0.0f;
    StartProgressSimulation();

    // Send request to the API - runs text-to-image by default.
    // If valid texture was passed, it attempts to run image-to-image workflow.
    Client->GenerateImage(
        PromptText,
        NegativePromptText,
        SelectedReferenceTexture.Get(),
        Strength,
        *SelectedModelOption,
        SeedValue,
        StylePreset);
    
    return FReply::Handled();
}

FReply STextureGeneratorWidget::OnCancelClicked()
{
    Client->CancelRequest();

    // Reset progress state
    bInProgress = false;
    GenerationProgress = 0.0f;

    // Show notification
    Async(EAsyncExecution::TaskGraphMainThread, []()
    {
        FNotificationInfo Info(FText::FromString("Texture generation cancelled"));
        Info.ExpireDuration = 5.0f;
        FSlateNotificationManager::Get().AddNotification(Info);
    });
    
    return FReply::Handled();
}

void STextureGeneratorWidget::OnImageGenerated(const TArray<uint8>& ImageData)
{
    // Complete progress
    GenerationProgress = 1.0f;
    StopProgressSimulation();
    
    // Save the generated image as texture asset
    FString PackageName;
    FString BaseName = FGuid::NewGuid().ToString().Left(8);
    UTexture2D* NewTexture = FTextureUtils::CreateTextureFromImageData(ImageData, BaseName, PackageName);
    if (!NewTexture)
    {
        OnGenerationError(TEXT("Creating texture from image data failed."));
        return;
    }

    // Create a basic material utilizing the generated texture
    UMaterial* NewMaterial = FTextureUtils::CreateMaterialForTexture(NewTexture, BaseName, PackageName);
    if (!NewMaterial)
    {
        OnGenerationError(TEXT("Creating material from texture failed."));
        return;
    }

    // Save packages
    TArray<UPackage*> PackagesToSave;
    PackagesToSave.Add(NewTexture->GetPackage());
    PackagesToSave.Add(NewMaterial->GetPackage());
    UEditorLoadingAndSavingUtils::SavePackages(PackagesToSave, true);

    // Show the newly created objects in the Content Browser
    TArray<UObject*> Objects;
    Objects.Add(NewTexture);
    Objects.Add(NewMaterial);
    GEditor->SyncBrowserToObjects(Objects);

    // Reset progress state
    bInProgress = false;
    GenerationProgress = 0.0f;

    // Show notification
    Async(EAsyncExecution::TaskGraphMainThread, []()
    {
        FNotificationInfo Info(FText::FromString("Texture generation finished!"));
        Info.ExpireDuration = 5.0f;
        Info.bUseSuccessFailIcons = true;
        Info.Image = FAppStyle::GetBrush("Icons.Success");
        FSlateNotificationManager::Get().AddNotification(Info);
    });
}

void STextureGeneratorWidget::OnGenerationError(const FString& ErrorMessage)
{
    // Reset progress state
    bInProgress = false;
    GenerationProgress = 0.0f;
    StopProgressSimulation();
    
    UE_LOG(LogTextureGenerator, Error, TEXT("%s"), *ErrorMessage);

    // Make user experience more bearable by showing notifications on error
    Async(EAsyncExecution::TaskGraphMainThread, [ErrorMessage]()
    {
        FNotificationInfo Info(FText::FromString(ErrorMessage));
        Info.ExpireDuration = 5.0f;
        Info.bUseSuccessFailIcons = true;
        Info.Image = FAppStyle::GetBrush("Icons.Error");
        FSlateNotificationManager::Get().AddNotification(Info);
    });
}

void STextureGeneratorWidget::StartProgressSimulation()
{
    // Since Stability AI doesn't provide real progress callbacks, we'll simulate progress
    ProgressSimulationStartTime = FDateTime::Now();
    
    // Start a timer that updates every 100ms
    if (!ProgressUpdateHandle.IsValid())
    {
        GEditor->GetTimerManager()->SetTimer(
            ProgressUpdateHandle,
            FTimerDelegate::CreateRaw(this, &STextureGeneratorWidget::UpdateProgressSimulation),
            0.1f,
            true);
    }
}

void STextureGeneratorWidget::StopProgressSimulation()
{
    if (ProgressUpdateHandle.IsValid())
    {
        GEditor->GetTimerManager()->ClearTimer(ProgressUpdateHandle);
        ProgressUpdateHandle.Invalidate();
    }
}

void STextureGeneratorWidget::UpdateProgressSimulation()
{
    if (!bInProgress)
    {
        StopProgressSimulation();
        return;
    }
    
    // Calculate elapsed time
    FTimespan ElapsedTime = FDateTime::Now() - ProgressSimulationStartTime;
    double ElapsedSeconds = ElapsedTime.GetTotalSeconds();
    
    // Simulate progress curve (starts fast, slows down)
    // Most images take 10-30 seconds, so we'll simulate accordingly
    float ExpectedDuration = 20.0f; // 20 seconds expected duration
    float RawProgress = FMath::Clamp(ElapsedSeconds / ExpectedDuration, 0.0f, 0.95f);
    
    // Apply easing curve to make it feel more natural
    GenerationProgress = FMath::Pow(RawProgress, 1.5f); // Ease-out curve
    
    // Cap at 95% until actual completion
    GenerationProgress = FMath::Min(GenerationProgress, 0.95f);
}
