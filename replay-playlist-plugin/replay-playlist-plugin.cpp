#include "replay-playlist-plugin.h"
#include "bakkesmod/wrappers/Engine/GameEngineWrapper.h"
#include "bakkesmod/wrappers/GameEvent/GameEventWrapper.h"
#include "bakkesmod/wrappers/GameEvent/ServerWrapper.h"
#include "bakkesmod/wrappers/Replay/ReplayManagerWrapper.h"
#include "utils/io.h" // For BakkesMod's logging

namespace fs = std::filesystem;

BAKKESMOD_PLUGIN(ReplayPlaylistPlugin, "Replay Playlist Plugin", "1.0.0", PLUGINTYPE_REPLAYSPEAKER)

// --- Helper Functions ---
std::string GetPlaylistKeyName(int playlistId) {
    // This function would ideally convert playlist ID to a string key.
    // For BakkesMod, playlist IDs are integers. We need to map them to our CVar keys.
    // This is a simplified example. You'll need to get the actual playlist ID from the game.
    switch (playlistId) {
        case 0:  return "Private"; // OnlinePlaylist_Private
        case 1:  return "Casual";  // OnlinePlaylist_Casual (was Unranked)
        case 10: return "Ranked_SoloDuel"; // OnlinePlaylist_RankedSoloDuel
        case 11: return "Ranked_Doubles";  // OnlinePlaylist_RankedDoubles
        case 13: return "Ranked_Standard"; // OnlinePlaylist_RankedStandard
        case 27: return "Tournament";    // OnlinePlaylist_Tournament
        // Add other relevant playlist IDs and their names
        default: return "Unknown_" + std::to_string(playlistId);
    }
}


// --- BakkesModPlugin Overrides ---
void ReplayPlaylistPlugin::onLoad()
{
    // Initialize console logging
    // console = std::make_shared<ConsoleWrapper>(gameWrapper->GetGUIManager().GetCVarManager());
    // For newer BM, use cvarManager directly for logging if needed, or use BM_LOG
    BM_LOG(INFO, "ReplayPlaylistPlugin loaded.");

    // Setup plugin data directory path
    // BakkesMod::Utils::GetBakkesModFolder() is usually the way, then append /data/YourPluginName
    // For this example, assuming gameWrapper->GetDataFolder() gives BakkesMod/data/
    pluginDataDir = gameWrapper->GetDataFolder() / "ReplayPlaylistPlugin";
    autosavedReplaysDir = pluginDataDir / "AutosavedReplays";

    CreatePluginDataDirectory();
    RegisterCVars();
    LoadConfig(); // Load CVar values from config file (BakkesMod handles this automatically for CVars)

    // Hook into game events
    // The specific event string might need to be verified from BakkesMod documentation or by dumping events.
    gameWrapper->HookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded",
                           [this](const std::string& eventName) { EventMatchEnded(eventName); });

    // Example of how to get current playlist (you'd do this in EventMatchEnded)
    // if (gameWrapper->IsInOnlineGame()) {
    //     ServerWrapper server = gameWrapper->GetCurrentGameState();
    //     if (server.IsValid()) {
    //         int playlistId = server.GetPlaylist().GetPlaylistId();
    //         BM_LOG(INFO, "Current Playlist ID: {}", playlistId);
    //     }
    // }

    BM_LOG(INFO, "ReplayPlaylistPlugin fully initialized.");
}

void ReplayPlaylistPlugin::onUnload()
{
    BM_LOG(INFO, "ReplayPlaylistPlugin unloading...");

    // Unhook game events
    // It's good practice to unhook events, though BakkesMod might handle some of this
    // automatically on plugin unload.
    if (gameWrapper) { // Ensure gameWrapper is still valid
        gameWrapper->UnhookEvent("Function TAGame.GameEvent_Soccar_TA.EventMatchEnded");
        BM_LOG(INFO, "Unhooked Function TAGame.GameEvent_Soccar_TA.EventMatchEnded.");
    }

    // CVars are automatically unregistered by BakkesMod's CVarManager when the plugin unloads
    // if they were registered with cvarManager->registerCvar.
    // Shared pointers (like cvarValuePtr in PlaylistTypeSetting) will automatically clean up.
    // Other dynamically allocated resources not managed by RAII/smart pointers would be cleaned here.
    // For this plugin, specific additional cleanup beyond event unhooking is minimal with the current structure.

    BM_LOG(INFO, "ReplayPlaylistPlugin unloaded successfully.");
}

// --- Plugin Functionality ---
void ReplayPlaylistPlugin::CreatePluginDataDirectory()
{
    if (!fs::exists(pluginDataDir)) {
        if (fs::create_directories(pluginDataDir)) {
            BM_LOG(INFO, "Plugin data directory created: {}", pluginDataDir.string());
        } else {
            BM_LOG(ERROR, "Failed to create plugin data directory: {}", pluginDataDir.string());
            // Handle error: maybe disable plugin functionality or notify user
            return;
        }
    }
    if (!fs::exists(autosavedReplaysDir)) {
        if (fs::create_directories(autosavedReplaysDir)) {
            BM_LOG(INFO, "AutosavedReplays directory created: {}", autosavedReplaysDir.string());
        } else {
            BM_LOG(ERROR, "Failed to create AutosavedReplays directory: {}", autosavedReplaysDir.string());
            // Handle error
        }
    }
}

void ReplayPlaylistPlugin::RegisterCVars()
{
    BM_LOG(INFO, "Registering CVars...");

    playlistSettings = {
        {"Private",         PlaylistTypeSetting("Private", "Private Match", 5)},
        {"Casual",          PlaylistTypeSetting("Casual", "Casual", 5)},
        {"Ranked_SoloDuel", PlaylistTypeSetting("Ranked_SoloDuel", "Ranked Solo Duel", 10)},
        {"Ranked_Doubles",  PlaylistTypeSetting("Ranked_Doubles", "Ranked Doubles", 10)},
        {"Ranked_Standard", PlaylistTypeSetting("Ranked_Standard", "Ranked Standard", 10)},
        {"Tournament",      PlaylistTypeSetting("Tournament", "Tournament", 10)}
        // Add any other playlist types you want to manage
    };

    for (auto& pair : playlistSettings) {
        PlaylistTypeSetting& setting = pair.second;
        // cvarmanager is inherited from BakkesModPlugin
        setting.cvarValuePtr = cvarManager->registerCvar(
            setting.cvarName,
            std::to_string(setting.defaultValue), // Default value as string
            "Max replays to autosave for " + setting.displayName + " playlist.",
            true, // Save to config
            true, 0, // Has min
            true, 100 // Has max (e.g., 100 replays max)
        );
        // If the CVar already existed, its value will be loaded.
        // We can also add a notifier if the CVar changes.
        // setting.cvarValuePtr->addOnValueChanged([this, key = pair.first](std::string oldValue, CVarWrapper cvar) {
        //    BM_LOG(INFO, "CVar {} changed from {} to {}", key, oldValue, cvar.getStringValue());
        // });
        BM_LOG(INFO, "Registered CVar: {} (Default: {})", setting.cvarName, setting.defaultValue);
    }
}

void ReplayPlaylistPlugin::LoadConfig()
{
    // For CVars registered with cvarManager->registerCvar and "saveToCfg = true",
    // BakkesMod automatically loads their values from the plugin's CFG file
    // (e.g., BakkesMod/cfg/ReplayPlaylistPlugin.cfg) when the plugin loads.
    // So, there's often no explicit "load" code needed here for the CVars themselves.
    // Their std::shared_ptr<int> (or string, bool) will hold the loaded value.

    // If you had other custom configuration not stored in CVars, you'd load it here.
    BM_LOG(INFO, "Configuration (CVars) loaded by BakkesMod.");

    // Example: Log current CVar values after they've been loaded/initialized
    for (const auto& pair : playlistSettings) {
        const PlaylistTypeSetting& setting = pair.second;
        if (setting.cvarValuePtr) {
            BM_LOG(INFO, "CVar {}: Current value = {}", setting.cvarName, *setting.cvarValuePtr);
        }
    }
}

#include <iomanip> // For std::put_time
#include <sstream> // For std::ostringstream
#include <chrono>  // For time functions

void ReplayPlaylistPlugin::EventMatchEnded(const std::string& eventName)
{
    BM_LOG(INFO, "[EventMatchEnded] Triggered: {}", eventName);

    // Check context (avoid saving during replays, or if not in a relevant game mode)
    // gameWrapper->IsInOnlineGame() also returns true for private matches and tournaments.
    // gameWrapper->IsInGame() is a more general check.
    if (gameWrapper->IsInReplay()) {
        BM_LOG(INFO, "[EventMatchEnded] Currently in a replay. No autosave.");
        return;
    }
    if (!gameWrapper->IsInOnlineGame()) { // Includes private, casual, ranked, tournament
        BM_LOG(INFO, "[EventMatchEnded] Not in an online game (private, casual, ranked, tournament). No autosave.");
        return;
    }

    ServerWrapper server = gameWrapper->GetCurrentGameState();
    if (!server.IsValid()) {
        BM_LOG(WARNING, "[EventMatchEnded] Server wrapper is invalid. Cannot determine playlist.");
        return;
    }

    // 1. Get Playlist Information
    int playlistId = server.GetPlaylist().GetPlaylistId();
    std::string playlistKey = GetPlaylistKeyName(playlistId);
    std::string playlistDisplayName = server.GetPlaylist().GetPlaylistName().toString(); // For logging
    BM_LOG(INFO, "[EventMatchEnded] Match ended in playlist: '{}' (ID: {}, Key: {})", playlistDisplayName, playlistId, playlistKey);

    int maxReplaysForThisPlaylist = GetMaxReplaysForPlaylist(playlistKey);
    if (maxReplaysForThisPlaylist <= 0) {
        BM_LOG(INFO, "[EventMatchEnded] Autosave disabled for playlist type '{}' (max replays = {}).", playlistKey, maxReplaysForThisPlaylist);
        return;
    }
    BM_LOG(INFO, "[EventMatchEnded] Max replays for this playlist type ('{}') is {}.", playlistKey, maxReplaysForThisPlaylist);

    // 2. Get Replay Folder & Find Most Recent Replay
    // Standard Rocket League replay folder: <Documents>/My Games/Rocket League/TAGame/Demos/
    // BakkesMod's gameWrapper->GetRLDataDir() usually points to <Documents>/My Games/Rocket League/TAGame/
    fs::path rlReplayDir = gameWrapper->GetRLDataDir() / "Demos";
    BM_LOG(INFO, "[EventMatchEnded] Rocket League replay directory: {}", rlReplayDir.string());

    if (!fs::exists(rlReplayDir) || !fs::is_directory(rlReplayDir)) {
        BM_LOG(ERROR, "[EventMatchEnded] Rocket League replay directory does not exist or is not a directory.");
        return;
    }

    fs::path mostRecentReplayPath;
    auto latestTime = fs::file_time_type::min();
    bool foundReplay = false;

    // This event might fire *just before* the replay file is finalized by the game.
    // A small delay might be needed, or a hook to a later event if this proves unreliable.
    // For now, we proceed without a delay.
    // Consider if game is still writing the replay (though MatchEnded should be late enough).
    // Rocket League might also take a moment to name the replay file with the match GUID.
    // For simplicity, we look for the most recent .replay file.
    // A more robust solution might involve waiting for ReplayWrapper::IsReplayWritten() or similar.
    // gameWrapper->GetReplayManager().SaveCurrentReplay() // This is to *initiate* a save, not get path.

    try {
        for (const auto& entry : fs::directory_iterator(rlReplayDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".replay") {
                auto writeTime = entry.last_write_time();
                // Check if it's recent enough (e.g., created in the last few minutes)
                // This helps avoid picking up very old replays if something goes wrong.
                auto now = std::chrono::file_clock::now();
                auto age = now - writeTime;
                if (age < std::chrono::minutes(5)) { // Only consider replays written in the last 5 minutes
                    if (writeTime > latestTime) {
                        latestTime = writeTime;
                        mostRecentReplayPath = entry.path();
                        foundReplay = true;
                    }
                }
            }
        }
    } catch (const fs::filesystem_error& e) {
        BM_LOG(ERROR, "[EventMatchEnded] Filesystem error while searching for replays: {}", e.what());
        return;
    }


    if (!foundReplay) {
        BM_LOG(WARNING, "[EventMatchEnded] No recent .replay file found in {}. Autosave might be too fast, or no replay was saved by the game.", rlReplayDir.string());
        // Consider adding a cvar for a small delay here and retrying once.
        return;
    }
    BM_LOG(INFO, "[EventMatchEnded] Most recent replay found: {}", mostRecentReplayPath.string());

    // 3. Create Autosave Subfolder for the current playlist type
    fs::path playlistAutosavePath = autosavedReplaysDir / playlistKey;
    try {
        if (!fs::exists(playlistAutosavePath)) {
            BM_LOG(INFO, "[EventMatchEnded] Creating directory: {}", playlistAutosavePath.string());
            if (!fs::create_directories(playlistAutosavePath)) {
                BM_LOG(ERROR, "[EventMatchEnded] Failed to create playlist autosave directory: {}. Check permissions.", playlistAutosavePath.string());
                return;
            }
        }
    } catch (const fs::filesystem_error& e) {
        BM_LOG(ERROR, "[EventMatchEnded] Filesystem error creating directory {}: {}", playlistAutosavePath.string(), e.what());
        return;
    }

    // 4. Copy and Rename Replay
    std::string timestampStr;
    try {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        std::tm buf{};
        #ifdef _WIN32
                localtime_s(&buf, &in_time_t);
        #else
                localtime_r(&in_time_t, &buf); // POSIX
        #endif
        std::ostringstream ss;
        ss << std::put_time(&buf, "%Y%m%d_%H%M%S");
        timestampStr = ss.str();
    } catch (const std::exception& e) {
        BM_LOG(ERROR, "[EventMatchEnded] Failed to create timestamp: {}", e.what());
        timestampStr = "timestamp_error"; // Fallback timestamp
    }

    // Sanitize original filename a bit, or use a generic name.
    // Using playlistKey and timestamp is good for sorting and identification.
    // std::string newFilename = timestampStr + "_" + mostRecentReplayPath.filename().string();
    std::string newFilename = timestampStr + "_" + playlistKey + ".replay"; // e.g., 20231027_153000_Ranked_Doubles.replay
    fs::path newReplayPath = playlistAutosavePath / newFilename;

    BM_LOG(INFO, "[EventMatchEnded] Attempting to copy '{}' to '{}'", mostRecentReplayPath.string(), newReplayPath.string());
    try {
        // Check if a file with this exact timestamp already exists (highly unlikely but good practice)
        if (fs::exists(newReplayPath)) {
            BM_LOG(WARNING, "[EventMatchEnded] Target file {} already exists. Appending unique part.", newReplayPath.string());
            newReplayPath = playlistAutosavePath / (timestampStr + "_" + playlistKey + "_1.replay"); // Simple uniqueness
        }
        fs::copy_file(mostRecentReplayPath, newReplayPath, fs::copy_options::overwrite_existing); // Or fail if exists
        BM_LOG(INFO, "[EventMatchEnded] Replay copied successfully to {}", newReplayPath.string());
    } catch (const fs::filesystem_error& e) {
        BM_LOG(ERROR, "[EventMatchEnded] Failed to copy replay from {} to {}: {}", mostRecentReplayPath.string(), newReplayPath.string(), e.what());
        return; // Don't proceed to delete old replays if copy failed
    }

    // 5. Enforce Replay Limits
    BM_LOG(INFO, "[EventMatchEnded] Enforcing replay limit for directory: {}", playlistAutosavePath.string());
    std::vector<fs::path> existingReplaysInSubfolder;
    try {
        for (const auto& entry : fs::directory_iterator(playlistAutosavePath)) {
            if (entry.is_regular_file() && entry.path().extension() == ".replay") {
                existingReplaysInSubfolder.push_back(entry.path());
            }
        }
    } catch (const fs::filesystem_error& e) {
        BM_LOG(ERROR, "[EventMatchEnded] Filesystem error listing replays in {}: {}", playlistAutosavePath.string(), e.what());
        return;
    }

    // Sort by filename (timestamp YYYYMMDD_HHMMSS ensures oldest come first)
    std::sort(existingReplaysInSubfolder.begin(), existingReplaysInSubfolder.end(),
        [](const fs::path& a, const fs::path& b) {
            return a.filename().string() < b.filename().string();
        });

    BM_LOG(INFO, "[EventMatchEnded] Found {} replays in {}. Max allowed: {}.", existingReplaysInSubfolder.size(), playlistAutosavePath.string(), maxReplaysForThisPlaylist);

    if (existingReplaysInSubfolder.size() > static_cast<size_t>(maxReplaysForThisPlaylist)) {
        int numToDelete = existingReplaysInSubfolder.size() - maxReplaysForThisPlaylist;
        BM_LOG(INFO, "[EventMatchEnded] Exceeds limit. Deleting {} oldest replays.", numToDelete);
        for (int i = 0; i < numToDelete; ++i) {
            try {
                BM_LOG(INFO, "[EventMatchEnded] Deleting: {}", existingReplaysInSubfolder[i].string());
                fs::remove(existingReplaysInSubfolder[i]);
            } catch (const fs::filesystem_error& e) {
                BM_LOG(ERROR, "[EventMatchEnded] Failed to delete old replay {}: {}", existingReplaysInSubfolder[i].string(), e.what());
                // Continue trying to delete others if one fails
            }
        }
    } else {
        BM_LOG(INFO, "[EventMatchEnded] Replay count ({}) is within or equal to the limit ({}). No deletions needed.", existingReplaysInSubfolder.size(), maxReplaysForThisPlaylist);
    }

    BM_LOG(INFO, "[EventMatchEnded] Autosave process completed for playlist: {}", playlistKey);
}

int ReplayPlaylistPlugin::GetMaxReplaysForPlaylist(const std::string& playlistKeyName)
{
    auto it = playlistSettings.find(playlistKeyName);
    if (it != playlistSettings.end()) {
        if (it->second.cvarValuePtr) {
            return *(it->second.cvarValuePtr);
        } else {
            BM_LOG(WARNING, "CVar pointer for {} is null. Returning default: {}", playlistKeyName, it->second.defaultValue);
            return it->second.defaultValue; // Fallback, though should not happen if registered
        }
    }
    BM_LOG(WARNING, "No CVar setting found for playlist key: {}. Returning 0 (no autosave).", playlistKeyName);
    return 0; // Default to 0 if playlist type is unknown or not configured
}


// --- PluginSettingsWindow Overrides ---
// This is for the settings UI in BakkesMod (F2 -> Plugins -> Replay Playlist Plugin)
void ReplayPlaylistPlugin::RenderSettings()
{
    ImGui::TextUnformatted("Configure the maximum number of replays to autosave for each playlist type.");
    ImGui::Spacing();

    for (auto& pair : playlistSettings) {
        PlaylistTypeSetting& setting = pair.second;
        if (setting.cvarValuePtr) {
            // Use cvarManager->renderCvar(setting.cvarName); for simpler rendering
            // Or, for more control:
            int currentValue = *setting.cvarValuePtr;
            if (ImGui::SliderInt(setting.displayName.c_str(), &currentValue, 0, 50)) { // Max 50 for example
                // cvarManager->executeCommand("rpp_max_replays_Casual " + std::to_string(currentValue));
                // This directly sets the CVar, which then should update *setting.cvarValuePtr via notifier if connected,
                // or we can set it directly:
                *setting.cvarValuePtr = currentValue;
                // Important: BakkesMod saves CVars automatically when they are changed if they were registered with saveToCfg=true.
                // You might need to explicitly tell the cvar to save if you change it programmatically like this
                // without going through executeCommand or if the CVar object itself isn't updated.
                // CVarWrapper c = cvarManager->getCvar(setting.cvarName);
                // if(c.IsInitialized()) c.setValue(currentValue);

                // To ensure the CVar system knows about the change immediately for saving,
                // it's often best to use cvarManager->executeCommand or CVarWrapper::setValue if available
                // For shared_ptr<int> CVars from registerCvar, modifying the int directly
                // should be fine as BakkesMod's CVar system periodically checks and saves them.
                // However, for immediate effect or if issues arise:
                // CVarWrapper cvar = cvarManager->getCvar(setting.cvarName);
                // if (cvar.IsInitialized()) { cvar.setValue(currentValue); }

            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("CVar: %s\nDefault: %d\n\nControls the maximum number of replays automatically saved for the %s playlist. Set to 0 to disable autosave for this playlist type.", setting.cvarName.c_str(), setting.defaultValue, setting.displayName.c_str());
            }
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("Reset All to Defaults")) {
        for (auto& pair : playlistSettings) {
            PlaylistTypeSetting& setting = pair.second;
            if (setting.cvarValuePtr) {
                *setting.cvarValuePtr = setting.defaultValue;
                // CVarWrapper cvar = cvarManager->getCvar(setting.cvarName);
                // if (cvar.IsInitialized()) { cvar.setValue(setting.defaultValue); }
            }
        }
        BM_LOG(INFO, "All playlist replay limits reset to defaults via UI.");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Resets all replay limits above to their original default values.");
    }


    ImGui::Spacing();
    if (ImGui::Button("Manual Test: Log Current Playlist Info")) {
        if (gameWrapper->IsInOnlineGame()) {
            ServerWrapper server = gameWrapper->GetCurrentGameState();
            if (server.IsValid()) {
                int playlistId = server.GetPlaylist().GetPlaylistId();
                std::string playlistName = server.GetPlaylist().GetPlaylistName().toString();
                BM_LOG(INFO, "Manual Test - Current Playlist: Name='{}', ID={}, KeyName='{}'", playlistName, playlistId, GetPlaylistKeyName(playlistId));
                cvarManager->executeCommand("echo \"Current Playlist ID: " + std::to_string(playlistId) + ", Name: " + GetPlaylistKeyName(playlistId) + "\"");

            } else {
                BM_LOG(INFO, "Manual Test - Not in a valid server state.");
                 cvarManager->executeCommand("echo \"Manual Test - Not in a valid server state.\"");
            }
        } else {
            BM_LOG(INFO, "Manual Test - Not in an online game.");
            cvarManager->executeCommand("echo \"Manual Test - Not in an online game.\"");
        }
    }
}

std::string ReplayPlaylistPlugin::GetPluginName()
{
// Name that appears in the F2 plugin list / settings window title
}

void ReplayPlaylistPlugin::SetImGuiContext(uintptr_t ctx)
{
    ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(ctx));
}
