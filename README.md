This plugin is based on the bakkesmod plugin
AutoSavePrivateReplay[(plugin)](https://bakkesplugins.com/plugins/view/133)[(github)](https://github.com/ubelhj/AutoSavePrivateReplay),
originally developed by JerryTheBee. It contains the current list of game modes
to choose from and doesn't fill the console with erroneous messages. After four
years, it needed an update.

When this plugin loads, it attempts to remove the older plugin
`AutoSavePrivateReplay`, because this plugin and that one are incompatible. Both
plugins utilize the same mechanisms and cannot coexist.

## How does it work?
There's a bakkesmod variable called `ranked_autosavereplay_all`, that's apart of
the ranked tab, and dictates saving replays automatically when you're allowed to
do so in a match in any gamemode.  This plugin basically checks, while you're in
a game, if you're in a game mode that you've selected to save the replays for.
If you are, the `ranked_autosavereplay_all` variable is turned on, therefore
automatically saving the replay through bakkesmod itself.  If you aren't, that
variable is turned off, meaning that no replay will be saved.

This check is performed after the kickoff for players and spectators.

Your selected playlists are saved through bakkesmod cvars between opening and
closing the game.

When the game is closed (or the plugin is unloaded), `ranked_autosavereplay_all`
is turned off.

WARNING:  
If you want to have bakkesmod save ALL replays, through
`ranked_autosavereplay_all` selected in the Ranked tab, you need to **DISABLE**
this plugin.
 
## If there's something wrong with the plugin
Please submit an issue ticket at
https://github.com/mgavin/AutosaveReplays/issues !

## If you would rather have checkboxes instead of selectable names (or would rather some part of the interface look different)
Please submit an issue ticket at
https://github.com/mgavin/AutosaveReplays/issues !

## If you would have a suggestion, like "Can you modify the replay name or something?"
Please submit an issue ticket at
https://github.com/mgavin/AutosaveReplays/issues !

Replay details, as shown in the game menu, would be as far as I would be willing
to look into. Actually modifying replay file contents, in a grand manner, is
outside the scope (and my interest) in this plugin.

---

## Here's the original text from the plugin's repo README.md:

Automatically saves replays from selected playlists

Enable by pressing f2 to open your bakkesmod plugins and selecting each of your
preferred playlists. Useful for ranked leagues where you need to provide replay
proof of games or just to save all of your hardcore chaos matches for replay
analysis.

Note offline playlists aren't working properly right now, will be fixed when I
have some time.

Note old version used "ranked_autosavereplay_private_only 1" for private
matches. Now uses "ranked_autosavereplay_privatematch 1".
