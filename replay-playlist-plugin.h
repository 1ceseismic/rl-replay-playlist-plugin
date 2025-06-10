#pragma once

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginwindow.h"
#include "bakkesmod/plugin/PluginSettingsWindow.h"

#include <string>
#include <vector>
#include <filesystem>
#include <map>

// Forward declaration
class ReplayPlaylistPlugin;

// Struct to hold information about each playlist type and its CVar
struct PlaylistTypeSetting {
    std::string cvarName;
    std::string displayName;
    int defaultValue;
    std::shared_ptr<int> cvarValuePtr; // Pointer to the CVar's value

    PlaylistTypeSetting(std::string name, std::string display, int defVal)
        : cvarName("rpp_max_replays_" + name), displayName(display), defaultValue(defVal) {}
};

class ReplayPlaylistPlugin : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow
{
public:
    // BakkesModPlugin
    void onLoad() override;
    void onUnload() override;

    // PluginSettingsWindow
    void RenderSettings() override;
    std::string GetPluginName() override;
    void SetImGuiContext(uintptr_t ctx) override;

private:
    // Methods
    void CreatePluginDataDirectory();
    void RegisterCVars();
    void LoadConfig();
    void EventMatchEnded(const std::string& eventName); // Hook for match ended

    // Member variables
    std::filesystem::path pluginDataDir;
    std::filesystem::path autosavedReplaysDir;

    // Map to store playlist type settings and their CVars
    std::map<std::string, PlaylistTypeSetting> playlistSettings;

    // Helper to easily get CVar values
    int GetMaxReplaysForPlaylist(const std::string& playlistKeyName);
};
