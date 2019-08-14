//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "usermessages.h"
#include "shake.h"
#include "voice_gamemgr.h"

void RegisterUserMessages()
{
	usermessages->Register( "Geiger", 1 );		// geiger info data
	usermessages->Register( "Train", 1 );		// train control data
	usermessages->Register( "HudText", -1 );	
	usermessages->Register( "SayText", -1 );	
	usermessages->Register( "TextMsg", -1 );
	usermessages->Register( "HudMsg", -1 );
	usermessages->Register( "ResetHUD", 1 );	// called every respawn
	usermessages->Register( "GameTitle", 0 );	// show game title
	usermessages->Register( "ItemPickup", -1 );	// for item history on screen
	usermessages->Register( "ShowMenu", -1 );	// show hud menu
	usermessages->Register( "Shake", 13 );		// shake view
	usermessages->Register( "Fade", 10 );	// fade HUD in/out
	usermessages->Register( "VGUIMenu", -1 );	// Show VGUI menu
	usermessages->Register( "CloseCaption", 3 ); // Show a caption (by string id number)(duration in 10th of a second)

	usermessages->Register( "SendAudio", -1 );	// play radion command

	usermessages->Register( "VoiceMask", VOICE_MAX_PLAYERS_DW*4 * 2 + 1 );
	usermessages->Register( "RequestState", 0 );

	usermessages->Register( "BarTime", -1 );	// For the C4 progress bar.
	usermessages->Register( "Damage", -1 );		// for HUD damage indicators
	usermessages->Register( "RadioText", -1 );		// for HUD damage indicators
	usermessages->Register( "HintText", -1 );	// Displays hint text display
	
	//usermessages->Register( "ReloadEffect", 2 );			// a player reloading..
	usermessages->Register( "PlayerAnimEvent", -1 );	// jumping, firing, reload, etc.

	usermessages->Register( "AmmoDenied", 2 );
	usermessages->Register( "UpdateRadar", -1 );

	// Used to send a sample HUD message
	usermessages->Register( "GameMessage", -1 );

	// Server is demanding we display the changeteam command
	usermessages->Register( "ChangeTeam", 0 );

	// Server is demanding we display the changeskill menu
	usermessages->Register( "ChangeSkill", 0 );

	// show the buy menu
	usermessages->Register( "BuyMenu", 0 );

	// freezes the client
	usermessages->Register( "SetFreezeState", 1 );

	// syncs the timer with the client
	usermessages->Register("SyncRoundTimer", 4);

	// show the objectives menu
	usermessages->Register("ChooseObjective", 4);

	// sends information about the objectives
	usermessages->Register("ObjectiveStatus", -1);

	// inform the team menu
	usermessages->Register("TeamDifference", 0);

	// generic message in the center of the hud
	usermessages->Register("HudDisplayMsg", -1);
	usermessages->Register("ResetHudDisplayMsg", 0);

	// message to update the buy time
	usermessages->Register("BuyTime", 4);

	// send the client strategy info
	usermessages->Register("ChooseStrategy", 5);

	usermessages->Register("StrategyMsg", -1);
	usermessages->Register("StrategyMsgBlock", -1);

	// message to allow a player to punish a teammate
	usermessages->Register("AllowPunish", -1);
}

