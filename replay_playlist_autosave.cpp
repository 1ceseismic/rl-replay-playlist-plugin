#include "replay_playlist_autosave.h"

#include <algorithm>
#include <cctype>
#include <limits>

#include <Windows.h>

#include <shellapi.h>
#include <stringapiset.h>
#include <winnls.h>

#include "imgui.h"
#include "imgui_internal.h"

#include "bm_helper.h"
#include "HookedEvents.h"

constexpr auto                      plugin_version = "3.0.1";
std::shared_ptr<CVarManagerWrapper> _globalCVarManager;

BAKKESMOD_PLUGIN(replay_playlist_autosave, "Automatically saves replays of selected playlists", plugin_version, NULL);
// PLUGINTYPE_FREEPLAY)

void replay_playlist_autosave::onLoad() {
      HookedEvents::gameWrapper = gameWrapper;
      _globalCVarManager        = cvarManager;

      cvs = std::make_unique<std::map<PlaylistId, CVarWrapper>>();
      init_cvars();
      init_hooked_events();
}

void replay_playlist_autosave::init_cvars() {
      cvarManager->registerCvar("ranked_autosavereplay_playlists", "0", "save replays from specified playlists")
            .addOnValueChanged([this](std::string, CVarWrapper cvar) { enable_plugin(cvar.getBoolValue()); });

      for (auto playlistPair : bm_helper::playlist_ids_str_spaced) {
            if (no_replay_playlists.contains(playlistPair.first)) {
                  continue;
            }

            std::string gamemode_str;
            std::copy_if(
                  begin(playlistPair.second),
                  end(playlistPair.second),
                  std::back_inserter(gamemode_str),
                  [](const char c) { return c != ' '; });  // take spaces out of names
            std::transform(cbegin(gamemode_str), cend(gamemode_str), begin(gamemode_str), [](unsigned char c) {
                  return std::tolower(c);
            });  // convert names to lowercase

            cvs->emplace(std::make_pair(
                  playlistPair.first,
                  cvarManager->registerCvar(
                        "ranked_autosavereplay_" + gamemode_str,
                        "0",
                        "save replays from " + gamemode_str,
                        false)));

            cvs->at(playlistPair.first).addOnValueChanged([this, playlistPair](std::string, CVarWrapper cvar) {
                  cvs->at(playlistPair.first)         = cvar;
                  enabledPlaylist[playlistPair.first] = cvar.getBoolValue();
            });
      }
}

void replay_playlist_autosave::init_hooked_events() {
      HookedEvents::AddHookedEvent("Function Engine.GameInfo.PreExit", [this](std::string eventName) {
            // ASSURED CLEANUP
            onUnload();
      });
}

void replay_playlist_autosave::find_and_remove_incompatible_plugins() {
      PluginManagerWrapper pmw            = gameWrapper->GetPluginManager();
      auto                 loaded_plugins = pmw.GetLoadedPlugins();
      for (const auto & plugin : *loaded_plugins) {
            if (plugin->_filename.compare("autosaveprivatereplay") == 0) {
                  LOG("Removing incompatible plugin AutoSavePrivateReplay");
                  cvarManager->executeCommand("plugin unload AutoSavePrivateReplay", 0);
                  cvarManager->executeCommand("writeplugins", 0);
                  try {
                        std::filesystem::remove(
                              gameWrapper->GetBakkesModPath().append("plugins/AutoSavePrivateReplay.dll"));
                        std::filesystem::remove(
                              gameWrapper->GetBakkesModPath().append("plugins/settings/AutoSavePrivateReplay.set"));
                  } catch (const std::exception & e) {
                        LOG("ERROR REMOVING AutoSavePrivateReplay.dll AND SETTINGS FILE! error: {}", e.what());
                  }
                  break;
            }
      }
}

void replay_playlist_autosave::enable_plugin(bool enabled) {
      if (enabled) {
            // hooks on a starting game
            HookedEvents::AddHookedEvent(
                  // the more I think about it, the more I think it's fine...
                  "Function GameEvent_Soccar_TA.Active.StartRound",  // this triggers after the countdown...
                  std::bind(&replay_playlist_autosave::start_game, this),
                  true);
      } else {
            HookedEvents::RemoveHook("Function GameEvent_Soccar_TA.Active.StartRound");
      }
}

void replay_playlist_autosave::start_game() {
      ServerWrapper sw = gameWrapper->GetCurrentGameState();
      if (sw) {
            GameSettingPlaylistWrapper gspw = sw.GetPlaylist();
            if (gspw) {
                  PlaylistId playid = static_cast<PlaylistId>(gspw.GetPlaylistId());
                  if (!bm_helper::playlist_ids_str.contains(playid)) {
                      LOG("Playlist ID {} not found!");
                     
                        return;
                  }

                  CVarWrapper cv = cvarManager->getCvar("ranked_autosavereplay_all");
                  cv.setValue(enabledPlaylist[playid] ? 1 : 0);
            }
      }
}

/// <summary>
/// https://mastodon.gamedev.place/@dougbinks/99009293355650878
/// </summary>
static inline void AddUnderline(ImColor col_) {
      ImVec2 min = ImGui::GetItemRectMin();
      ImVec2 max = ImGui::GetItemRectMax();
      min.y      = max.y;
      ImGui::GetWindowDrawList()->AddLine(min, max, col_, 1.0f);
}

/// <summary>
/// taken from https://gist.github.com/dougbinks/ef0962ef6ebe2cadae76c4e9f0586c69
/// "hyperlink urls"
/// </summary>
static inline void TextURL(const char * name_, const char * URL_, uint8_t SameLineBefore_, uint8_t SameLineAfter_) {
      if (1 == SameLineBefore_) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      }
      ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(0, 165, 255, 255));
      ImGui::Text("%s", name_);
      ImGui::PopStyleColor();
      if (ImGui::IsItemHovered()) {
            if (ImGui::IsMouseClicked(0)) {
                  // What if the URL length is greater than int but less than size_t?
                  // well then the program should crash, but this is fine.
                  const int nchar =
                        std::clamp(static_cast<int>(std::strlen(URL_)), 0, (std::numeric_limits<int>::max)() - 1);
                  wchar_t * URL = new wchar_t[nchar + 1];
                  wmemset(URL, 0, nchar + 1);
                  MultiByteToWideChar(CP_UTF8, 0, URL_, nchar, URL, nchar);
                  ShellExecuteW(NULL, L"open", URL, NULL, NULL, SW_SHOWNORMAL);

                  delete[] URL;
            }
            AddUnderline(ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered]);
            ImGui::SetTooltip("  Open in browser\n%s", URL_);
      } else {
            AddUnderline(ImGui::GetStyle().Colors[ImGuiCol_Button]);
      }
      if (1 == SameLineAfter_) {
            ImGui::SameLine(0.0f, ImGui::GetStyle().ItemInnerSpacing.x);
      }
}

void replay_playlist_autosave::SetImGuiContext(uintptr_t ctx) {
      ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext *>(ctx));
}

std::string replay_playlist_autosave::GetPluginName() {
      return "Autosave Replays";
}

void replay_playlist_autosave::RenderSettings() {
      CVarWrapper enabled_cv     = cvarManager->getCvar("ranked_autosavereplay_playlists");
      bool        plugin_enabled = enabled_cv.getBoolValue();
      if (ImGui::Checkbox("Enable Plugin", &plugin_enabled)) {
            enabled_cv.setValue(plugin_enabled);
      };
      ImGui::NewLine();
      ImGui::Separator();
      ImGui::NewLine();
      ImGui::TextUnformatted("Click a playlist to enable automatic saving of its replay.");
      ImGui::NewLine();
      ImGui::PushStyleColor(
            ImGuiCol_Header,
            ImVec4(0.17f, 0.51f, 0.16f, 0.7f));  // the color the selectable becomes when selected.
      ImGui::BeginColumns(
            "playlistselectables",
            static_cast<int>(std::size(SHOWN_PLAYLIST_CATEGORIES)),
          ImGuiOldColumnFlags_NoResize);
      size_t mxlines = 0;
      for (const std::string & header : SHOWN_PLAYLIST_CATEGORIES) {
            ImGui::TextUnformatted(header.c_str());
            AddUnderline(col_white);
            ImGui::NextColumn();
            mxlines = (std::max)(mxlines, bm_helper::playlist_categories[header].size());
      }
      for (int line = 0; line < mxlines; ++line) {
            for (const std::string & category : SHOWN_PLAYLIST_CATEGORIES) {
                  if (line < bm_helper::playlist_categories[category].size()) {
                        PlaylistId playid = bm_helper::playlist_categories[category][line];
                        if (!no_replay_playlists.contains(playid)) {
                              if (ImGui::Selectable(
                                        std::format(
                                              "[{:c}] {}",
                                              enabledPlaylist[playid] ? 'X' : ' ',
                                              bm_helper::playlist_ids_str_spaced[playid])
                                              .c_str(),
                                        &enabledPlaylist[playid])) {
                                    cvs->at(playid).setValue(enabledPlaylist[playid]);
                              }
                        }
                  }
                  ImGui::NextColumn();
            }
      }
      ImGui::EndColumns();
      ImGui::PopStyleColor();

      ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.0f, 0.2f, 0.8f, 1.0f));
      if (ImGui::CollapsingHeader("INFORMATION")) {
            // SOME TEXT EXPLAINING SOME QUESTIONS
            static const float INDENT_OFFSET = 40.0f;

            // Question 1
            ImGui::Indent(INDENT_OFFSET);

            ImGui::TextUnformatted("HOW DOES THIS WORK?");
            AddUnderline(col_white);

            ImGui::Unindent(INDENT_OFFSET);

            ImGui::TextUnformatted(
                  "There's a bakkesmod variable called `ranked_autosavereplay_all`, that's apart of\n"
                  "the ranked tab, (\"Automatically save all replays\" checkbox), and dictates saving\n"
                  "replays automatically when you're allowed to do so in a match in any gamemode.\n"
                  "This plugin basically checks, while you're in a game, if you're in a game mode that\n"
                  "you've selected to save the replays for.\n"
                  "If you are, the `ranked_autosavereplay_all` variable is turned on, therefore\n"
                  "automatically saving the replay through bakkesmod itself.  If you aren't, that\n"
                  "variable is turned off, meaning that no replay will be saved. Basically giving you\n"
                  "control over which gamemodes actually have their replays saved.\n\n"

                  "This check is performed after the kickoff for players and spectators.\n\n"

                  "Your selected playlists are saved through bakkesmod cvars between opening and\n"
                  "closing the game.\n\n"

                  "When the game is closed (or the plugin is unloaded), `ranked_autosavereplay_all`\n"
                  "is turned off.\n");

            ImGui::TextUnformatted("WARNING:  ");
            AddUnderline(col_white);
            ImGui::TextUnformatted(
                  "If you want to have bakkesmod save ALL replays, through\n"
                  "`ranked_autosavereplay_all` selected in the Ranked tab, you need to **DISABLE**\n"
                  "this plugin.");

            ImGui::NewLine();

            // Question 2
            ImGui::Indent(INDENT_OFFSET);

            ImGui::TextUnformatted("WHAT IF I CRASH OR HAVE A PROBLEM?");
            AddUnderline(col_white);
            ImGui::TextUnformatted("WHAT IF I HAVE A SUGGESTION?");
            AddUnderline(col_white);

            ImGui::Unindent(INDENT_OFFSET);

            ImGui::TextUnformatted("Raise an issue on the github page: ");
            TextURL("HERE", "https://github.com/mgavin/AutosaveReplays/issues", true, false);

            ImGui::NewLine();
      }
      ImGui::PopStyleColor();
}

void replay_playlist_autosave::onUnload() {
      // make sure to turn this off just in case for next time.
      CVarWrapper cv = cvarManager->getCvar("ranked_autosavereplay_all");
      if (cv) {
            cv.setValue(0);
      }
}
