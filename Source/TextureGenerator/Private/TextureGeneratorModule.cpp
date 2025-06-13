// Copyright Mateusz Wojt. All Rights Reserved.

#include "TextureGeneratorModule.h"
#include "TextureGeneratorStyle.h"
#include "TextureGeneratorCommands.h"
#include "TextureGeneratorSettings.h"
#include "Widgets/STextureGeneratorWidget.h"
#include "Misc/MessageDialog.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "ToolMenu.h"
#include "ToolMenuSection.h"
#include "ToolMenuMisc.h"
#include "Framework/Docking/TabManager.h"
#include "Framework/Docking/WorkspaceItem.h"
#include "ISettingsModule.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

DEFINE_LOG_CATEGORY(LogTextureGenerator);

static const FName TextureGeneratorTabName("TextureGenerator");

#define LOCTEXT_NAMESPACE "FTextureGeneratorModule"

void FTextureGeneratorModule::StartupModule()
{
    // Register styles
    FTextureGeneratorStyle::Initialize();
    FTextureGeneratorStyle::ReloadTextures();

    // Register commands
    FTextureGeneratorCommands::Register();

    // Register plugin settings
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->RegisterSettings("Project", "Plugins", "Stability AI Texture Generator",
            LOCTEXT("TextureGeneratorSettingsName", "Stability AI Texture Generator"),
            LOCTEXT("TextureGeneratorSettingsDescription", "Configure Stability AI Texture Generation Plugin"),
            GetMutableDefault<UTextureGeneratorSettings>()
        );
    }
    
    // Register menu
    PluginCommands = MakeShareable(new FUICommandList);
    PluginCommands->MapAction(
        FTextureGeneratorCommands::Get().OpenPluginWindow,
        FExecuteAction::CreateRaw(this, &FTextureGeneratorModule::PluginButtonClicked),
        FCanExecuteAction());

    UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FTextureGeneratorModule::RegisterMenus));
    
    // Register tab spawner
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner(
        TextureGeneratorTabName,
        FOnSpawnTab::CreateRaw(this, &FTextureGeneratorModule::OnSpawnPluginTab))
        .SetDisplayName(LOCTEXT("TextureGeneratorTabTitle", "Stability AI Texture Generator"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
}

void FTextureGeneratorModule::ShutdownModule()
{
    // Unregister settings
    if (ISettingsModule* SettingsModule = FModuleManager::GetModulePtr<ISettingsModule>("Settings"))
    {
        SettingsModule->UnregisterSettings("Project", "Plugins", "Stability AI Texture Generator");
    }
    
    // Unregister tab spawner
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(TextureGeneratorTabName);
    
    // Unregister menu
    UToolMenus::UnRegisterStartupCallback(this);
    UToolMenus::UnregisterOwner(this);
    
    // Unregister commands
    FTextureGeneratorStyle::Shutdown();
    FTextureGeneratorCommands::Unregister();
}

void FTextureGeneratorModule::PluginButtonClicked()
{
    FGlobalTabmanager::Get()->TryInvokeTab(TextureGeneratorTabName);
}

void FTextureGeneratorModule::RegisterMenus()
{
    // Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
    FToolMenuOwnerScoped OwnerScoped(this);
    
    // Add entry to the Window menu
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    {
        FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");
        Section.AddMenuEntryWithCommandList(FTextureGeneratorCommands::Get().OpenPluginWindow, PluginCommands);
    }
    
    // Add toolbar button
    UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.PlayToolBar");
    {
        FToolMenuSection& Section = ToolbarMenu->FindOrAddSection("PluginTools");
        {
            FToolMenuEntry& Entry = Section.AddEntry(FToolMenuEntry::InitToolBarButton(
                FTextureGeneratorCommands::Get().OpenPluginWindow,
                TAttribute<FText>(),
                TAttribute<FText>(),
                FSlateIcon(FTextureGeneratorStyle::GetStyleSetName(), "TextureGenerator.PluginAction")
            ));
            Entry.SetCommandList(PluginCommands);
        }
    }
}

TSharedRef<SDockTab> FTextureGeneratorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab)
        .TabRole(ETabRole::NomadTab)
        [
            SNew(STextureGeneratorWidget)
        ];
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTextureGeneratorModule, TextureGenerator)
