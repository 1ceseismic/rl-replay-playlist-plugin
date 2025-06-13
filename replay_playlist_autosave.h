#pragma once
#define WIN32_LEAN_AND_MEAN

#include <set>
#include <string>

#include "bakkesmod/plugin/bakkesmodplugin.h"
#include "bakkesmod/plugin/pluginsettingswindow.h"
#include "imgui.h"

#include "bm_helper.h"

class replay_playlist_autosave : public BakkesMod::Plugin::BakkesModPlugin, public BakkesMod::Plugin::PluginSettingsWindow {
private:
      const ImColor col_white = ImColor {
            ImVec4 {1.0f, 1.0f, 1.0f, 1.0f}
      };
      const std::vector<std::string> SHOWN_PLAYLIST_CATEGORIES =
            {"Casual", "Competitive", "Tournament", "Offline", "Private Match"};
      const std::set<PlaylistId> no_replay_playlists = {
            PlaylistId::Unknown,
            PlaylistId::Casual,
            PlaylistId::Training,
            PlaylistId::Workshop,
            PlaylistId::UGCTrainingEditor,
            PlaylistId::UGCTraining,

      };

      std::unique_ptr<std::map<PlaylistId, CVarWrapper>>
                                 cvs;  // saved cvars so I don't have to look them up over and over
      std::map<PlaylistId, bool> enabledPlaylist = [this] {
            std::map<PlaylistId, bool> tmp;
            for (const auto & x : bm_helper::playlist_ids_str) {
                  if (no_replay_playlists.contains(x.first)) {
                        continue;
                  }
                  tmp[x.first] = false;
            }
            return tmp;
      }();

      void init_cvars();
      void init_hooked_events();
      void find_and_remove_incompatible_plugins();

public:
      virtual void onLoad() override;
      virtual void onUnload() override;

      void        SetImGuiContext(uintptr_t ctx) override;
      std::string GetPluginName() override;
      void        RenderSettings() override;

      void enable_plugin(bool enabled);

      void start_game();
};
