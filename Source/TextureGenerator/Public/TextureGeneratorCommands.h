// Copyright Mateusz Wojt. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "TextureGeneratorStyle.h"

class FTextureGeneratorCommands : public TCommands<FTextureGeneratorCommands>
{
public:
    FTextureGeneratorCommands()
        : TCommands<FTextureGeneratorCommands>(
            TEXT("TextureGenerator"),
            NSLOCTEXT("Contexts", "TextureGenerator", "Stability AI Texture Generator Plugin"),
            NAME_None,
            FTextureGeneratorStyle::GetStyleSetName())
    {
    }

    // TCommands<> interface
    virtual void RegisterCommands() override;

public:
    TSharedPtr<FUICommandInfo> OpenPluginWindow;
};
