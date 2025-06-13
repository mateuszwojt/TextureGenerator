// Copyright Mateusz Wojt. All Rights Reserved.

#include "TextureGeneratorCommands.h"

#define LOCTEXT_NAMESPACE "FTextureGeneratorModule"

void FTextureGeneratorCommands::RegisterCommands()
{
    UI_COMMAND(
        OpenPluginWindow,
        "Stability AI Texture Generator",
        "Open the Stability AI Texture Generator window",
        EUserInterfaceActionType::Button,
        FInputChord()
    );
}

#undef LOCTEXT_NAMESPACE
