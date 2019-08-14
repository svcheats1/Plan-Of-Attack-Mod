#include "cbase.h"
#include "hud_strategymsg.h"
#include "worldmap.h"
#include <vgui/ilocalize.h>

DECLARE_HUDELEMENT(HudStrategyMsg);
DECLARE_HUD_MESSAGE(HudStrategyMsg, ChooseStrategy);
DECLARE_HUD_MESSAGE(HudStrategyMsg, StrategyMsg);
DECLARE_HUD_MESSAGE(HudStrategyMsg, StrategyMsgBlock);

#define BYTE_TO_BOOL(byte) (byte == 0 ? false : true)
#define STRATEGY_DISPLAY_TIME 6.0

#define ATTACK_MSG vgui::localize()->Find("#Round_Attack")
#define ATTACK_BASE_MSG vgui::localize()->Find("#Round_Attack_Base")
#define DEFEND_MSG vgui::localize()->Find("#Round_Defend")
#define DEFEND_SUBLABEL vgui::localize()->Find("#Round_Defend_SubLabel")

#define ATTACK_SOUND "Radio.ObjectiveAttack"
#define DEFEND_SOUND "Radio.ObjectiveDefend"

#define STRATEGY_BASE_XPOS (ScreenWidth() - XRES(180) - XRES(5))
#define STRATEGY_BASE_YPOS YRES(170)
#define STRATEGY_BASE_WIDE	XRES(180)
#define STRATEGY_BASE_TALL	YRES(215)
#define STRATEGY_SUBLABEL_HEIGHT YRES(32)


HudStrategyMsg::HudStrategyMsg(const char *szName)
	: BaseClass(NULL, "HudStrategyMsg"), CHudElement(szName)
{
	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// hide when this stuff happens
	SetHiddenBits(HIDEHUD_PLAYERDEAD);

	// create the main label
	m_pLabel = new WrapLabel(this, "HudStrategyMsgLabel");
	m_pLabel->SetEnabled(true);
	m_pLabel->SetProportional(true);

	// create our sub label
	m_pSubLabel = new WrapLabel(this, "HudStrategyMsgSubLabel");
	m_pSubLabel->SetEnabled(true);
	m_pSubLabel->SetProportional(true);

	// create the label for the strategy name (underneath the map)
	m_pStrategyNameLabel = new Label(this, "HudStrategyMsgNameLabel", "");
	m_pStrategyNameLabel->SetEnabled(true);
	m_pStrategyNameLabel->SetProportional(true);

	// make us hidden
	m_fDisplayEndTime = 0;
	SetVisible(false);

	// set default size
	SetBounds(STRATEGY_BASE_XPOS, STRATEGY_BASE_YPOS, STRATEGY_BASE_WIDE, STRATEGY_BASE_TALL);

	// no map yet
	m_pMap = NULL;

	// listen for round resets
	gameeventmanager->AddListener(this, "round_ended", false);
	gameeventmanager->AddListener(this, "game_newmap", false);
}

HudStrategyMsg::~HudStrategyMsg()
{
	if (m_pMap)
		delete m_pMap;
}

void HudStrategyMsg::Init(void)
{
	// hook the messages
	HOOK_HUD_MESSAGE(HudStrategyMsg, StrategyMsgBlock);
	HOOK_HUD_MESSAGE(HudStrategyMsg, StrategyMsg);
	HOOK_HUD_MESSAGE(HudStrategyMsg, ChooseStrategy);

	// tim does this in displaymsg
	CHudElement::Reset();
	SetVisible(false);
}

void HudStrategyMsg::LevelInit(void)
{
	// tim does this in displaymsg
	SetVisible(false);
	CHudElement::LevelInit();
}

// called when a round_ended event comes in
void HudStrategyMsg::RoundReset(void)
{
	// no map after round reset
	if (m_pMap)
		delete m_pMap;
	m_pMap = NULL;

	// tell the worldmap to clear out the strategy
	CWorldMap *pWorldMap = (CWorldMap*)gViewPortInterface->FindPanelByName(PANEL_WORLDMAP);
	if (pWorldMap)
		pWorldMap->LoadFromKeyValues(NULL);

	m_fDisplayEndTime = 0.0;
}

// this is the message that tells us to choose a strategy
void HudStrategyMsg::MsgFunc_ChooseStrategy(bf_read &msg)
{
	// are we on offense?
	bool bOffensive = BYTE_TO_BOOL(msg.ReadByte());
	// time to display the panel for
	float fTime = msg.ReadFloat();

	// find the panel, pass in the data, and display it.
	IViewPortPanel *pPanel = gViewPortInterface->FindPanelByName(PANEL_STRATEGY);
	if (pPanel) {
		CStrategyMenu *menu = (CStrategyMenu*)pPanel;
		menu->SetTimer(fTime);
		menu->SetOffensive(bOffensive);
        menu->ShowPanel(true);
	}
}

// this message tells us to display the HUD message
// this message must also include the information that used to be in the attack tip
void HudStrategyMsg::MsgFunc_StrategyMsg(bf_read &msg)
{
	// are we attacking?
	m_bAttacking = BYTE_TO_BOOL(msg.ReadByte());
	// read which objective(s) to attack/defend.
	if (m_bAttacking) {
		// see if we're going to get a number
		m_bIsBase = BYTE_TO_BOOL(msg.ReadByte());

		// get obj name and number
		msg.ReadString(m_szObjectiveName, sizeof(m_szObjectiveName));
		if (!m_bIsBase)
			msg.ReadString(m_szObjectives, sizeof(m_szObjectives));
	}
	else {
		// get list of obj to defend
		msg.ReadString(m_szObjectives, sizeof(m_szObjectives));
	}

	// was this strategy transmitted?
	m_bTransmit = BYTE_TO_BOOL(msg.ReadByte());

	// read in the strategy data
	m_iCommandsToGo = msg.ReadShort();

	// reset the buffer
	m_sBuffer.Purge();
}

/**
* Displays the map
*
* @return void
**/
void HudStrategyMsg::DisplayMap(void)
{
	char *pData;

	// init a map pointer and grap the worldmap
	CStrategyMap *pMap = NULL;
	CWorldMap *pWorldMap = (CWorldMap*)gViewPortInterface->FindPanelByName(PANEL_WORLDMAP);

	// create enough room for everything in the buffer
	pData = new char[m_sBuffer.TellPut() + 1];
	memset(pData, 0, m_sBuffer.TellPut() + 1);
	m_sBuffer.Get(pData, m_sBuffer.TellPut());

	// if we're not transmitting, then load from the file
	if(!m_bTransmit)
	{
		// where the strategies are
		char filePath[256];
		sprintf(filePath, "strategies/%s/%s", g_pGameRules->MapName(), pData);

		// try to load it from file
		KeyValues *keyValues = new KeyValues("strategy");
		if (keyValues->LoadFromFile(vgui::filesystem(), filePath, "GAME")) {
			// get the definition key
			KeyValues *definition = keyValues->FindKey("definition");
			// found it?
			if (definition) {
				// create the map
				pMap = new CStrategyMap(this);
				// load it from the KVs
				if (!pMap->LoadFromKeyValues(definition)) {
					// failed, delete the map
					delete pMap;
					pMap = NULL;
				}
				// success, tell the worldmap
				else if (pWorldMap)
					pWorldMap->LoadFromKeyValues(definition);
			}
		}
		
		// delete our KVs
		keyValues->deleteThis();
	}
	else
	{
		// if it was transmitted, than load it from a buffer
		KeyValues *kV = new KeyValues("definition");
		if (kV->LoadFromBuffer("definition", pData)) {
			// success, create the map
			pMap = new CStrategyMap(this);
			// load it from the key values
			if (!pMap->LoadFromKeyValues(kV)) {
				// failed, trash the map
				delete pMap;
				pMap = NULL;
			}
			// success, tell the worldmap
			else if (pWorldMap)
				pWorldMap->LoadFromKeyValues(kV);
		}

		// delete the KVs
		kV->deleteThis();
	}

	// kill the buffer
	delete [] pData;

	// delete the current map if we have one
	if (m_pMap)
		delete m_pMap;

	// use the map we just got (may be null)
	m_pMap = pMap;

	// if we didn't get a map, then don't do the init stuff below
	if (!m_pMap)
		return;

	// set when we should stop displaying
	m_fDisplayEndTime = gpGlobals->curtime + STRATEGY_DISPLAY_TIME;

	// if we're attacking, using the normal size
	if (m_bAttacking)
		SetBounds(STRATEGY_BASE_XPOS, STRATEGY_BASE_YPOS, STRATEGY_BASE_WIDE, STRATEGY_BASE_TALL);
	// if we're defending, use the larger size to include the defend sublabel
	else
		SetBounds(
			STRATEGY_BASE_XPOS, 
			STRATEGY_BASE_YPOS - STRATEGY_SUBLABEL_HEIGHT,
			STRATEGY_BASE_WIDE,
			STRATEGY_BASE_TALL + STRATEGY_SUBLABEL_HEIGHT);

	// set the map properties
    m_pMap->SetParent(this);
	m_pMap->SetBgColor(Color(0, 0, 0, 0));
	m_pMap->SetVisible(true);

	// set the size/pos to be as big as the width of the panel, and push it to the
	// bottom of the panel.
	m_pMap->SetBounds(
		XRES(5),
		GetTall() - GetWide() - YRES(10),
		GetWide() - XRES(10),
		GetWide() - XRES(10));
	m_pMap->SetMapSize(GetWide() - XRES(10), GetWide() - XRES(10));

	// put the strategy name under the map
	m_pStrategyNameLabel->SetPos(DISPLAY_MSG_SUBLABEL1_X_POS, GetTall() - YRES(15));

	// set up the content of all our labels
	SetMessages();

	// play the appropriate attack/defend sound
	EmitSound();

	// OK, ready to go: visible!
	SetVisible(true);
}

/**
* Handles an individual block of the strategy message
*
* @param bf_read &msg The message to handle
* @return void
**/
void HudStrategyMsg::MsgFunc_StrategyMsgBlock(bf_read &msg)
{
	char szData[255];

	// read in the next hunk of data
	if(!msg.ReadString(szData, sizeof(szData)))
	{
		Assert(0);
		return;
	}

	// add it the buffer
	m_sBuffer.Put(szData, Q_strlen(szData));

	// knock off one command and see if there are any left
	--m_iCommandsToGo;
	if(!m_iCommandsToGo)
		DisplayMap();
}

void HudStrategyMsg::SetMessages()
{
	// buffers! silly wchar_t's...
	wchar_t buf1[255];
	wchar_t buf2[64];
	wchar_t buf3[16];
	wchar_t *wszConverted;
	// get a copy of this, so we get intellisense, as well as readability, etc
	ILocalize *local = vgui::localize();

	// are we attacking?
	if (m_bAttacking) {
		// main label is center/top aligned
		m_pLabel->SetContentAlignment(vgui::Label::Alignment::a_north);

		// attacking base?
		if (m_bIsBase)
		{
			// see if we have a localized version for the name
			wszConverted = vgui::localize()->Find(m_szObjectiveName);

			// did we get it?
			if(!wszConverted)
			{
				// convert to unicode and stick the name in the attack message
				vgui::localize()->ConvertANSIToUnicode(m_szObjectiveName, buf2, sizeof(buf2));
				swprintf(buf1, ATTACK_BASE_MSG, buf2);
			}
			else
				// stick the name in the attack message
				swprintf(buf1, ATTACK_BASE_MSG, wszConverted);

			// set the text using the complete string
			m_pLabel->SetText(buf1);
		}
		else {
			// otherwise we need to put the name and the number, eg. "(III)" in
			local->ConvertANSIToUnicode(m_szObjectiveName, buf2, sizeof(buf2));
			local->ConvertANSIToUnicode(m_szObjectives, buf3, sizeof(buf3));
			swprintf(buf1, ATTACK_MSG, buf2, buf3);
			m_pLabel->SetText(buf1);
		}

		// set proper visibility - no sublabel for attacking
		m_pLabel->SetVisible(true);
		m_pSubLabel->SetVisible(false);
	}
	else {
		// defend label is NW aligned
		m_pLabel->SetContentAlignment(vgui::Label::Alignment::a_northwest);

		// put which objectives to defend into the label, eg. "(I,II,III)"
		local->ConvertANSIToUnicode(m_szObjectives, buf2, sizeof(buf2));
		swprintf(buf1, DEFEND_MSG, buf2);
		m_pLabel->SetText(buf1);

		// sublabel is constant.
		m_pSubLabel->SetText(DEFEND_SUBLABEL);

		// set both visible
		m_pLabel->SetVisible(true);
		m_pSubLabel->SetVisible(true);
	}

	// get the strategy name from the map
	if (m_pMap)
		m_pStrategyNameLabel->SetText(m_pMap->GetName());
}

// sends out the attack or defend sound
void HudStrategyMsg::EmitSound()
{
	EmitSound_t sSound;

	CBasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();
	if(pPlayer)
	{
		// setup the sound
		sSound.m_nChannel = CHAN_STATIC;
		sSound.m_flVolume = VOL_NORM;
		sSound.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_STATIC);
		sSound.m_pSoundName = (m_bAttacking ? ATTACK_SOUND : DEFEND_SOUND);

		// send it out
		CSingleUserRecipientFilter filter(pPlayer);
		filter.MakeReliable();
		pPlayer->EmitSound(filter, pPlayer->entindex(), sSound);
	}
}

void HudStrategyMsg::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	// m_pScheme = pScheme;

	// apply the scheme
	BaseClass::ApplySchemeSettings(pScheme);

	// position, size, font
	m_pLabel->SetFont(pScheme->GetFont("Trebuchet16", IsProportional()));
	m_pLabel->SetSize(GetWide() - (DISPLAY_MSG_X_POS * 2), YRES(20));
	m_pLabel->SetPos(DISPLAY_MSG_X_POS, DISPLAY_MSG_Y_POS);

	// pos and size
	m_pSubLabel->SetSize(GetWide() - (DISPLAY_MSG_SUBLABEL1_X_POS * 2), STRATEGY_SUBLABEL_HEIGHT);
	m_pSubLabel->SetPos(DISPLAY_MSG_SUBLABEL1_X_POS, DISPLAY_MSG_SUBLABEL1_Y_POS);

	// pos, size, alignment
	m_pStrategyNameLabel->SetContentAlignment(vgui::Label::Alignment::a_north);
	m_pStrategyNameLabel->SetSize(GetWide() - (DISPLAY_MSG_SUBLABEL1_X_POS * 2), YRES(15));
	m_pStrategyNameLabel->SetPos(DISPLAY_MSG_SUBLABEL1_X_POS, GetTall() - YRES(15));

	// we want a background
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
	SetBgColor(pScheme->GetColor("TransparentLightBlack", Color(0, 0, 0, 0)));
}


void HudStrategyMsg::FireGameEvent(IGameEvent *pEvent)
{
	// call the roundreset function
	RoundReset();
}

// should draw override
bool HudStrategyMsg::ShouldDraw()
{
	// this checked the HUD flags
	if (!CHudElement::ShouldDraw())
		return false;

	// are we supossed to be displayed?
	if (m_fDisplayEndTime > gpGlobals->curtime) {
		// are we behind panels?
		if (BehindPanels()) {
			// extend the display time and hide us
			m_fDisplayEndTime = gpGlobals->curtime + STRATEGY_DISPLAY_TIME;
			SetVisible(false);
		}
		// make sure we're visible otherwise
		else
			SetVisible(true);
	}

	// return our visibility status
	return IsVisible();
}


/**
* Determines if we are behind important panels
*
* @return bool
**/
bool HudStrategyMsg::BehindPanels(void)
{
	IViewPortPanel *pPanel;
	const char *szPanels[6] = {PANEL_BUY, PANEL_STRATEGY, PANEL_OBJECTIVES, PANEL_TEAM, PANEL_CLASS, PANEL_SCOREBOARD};

	// flip through them
	for(int i = 0; i < 6; ++i)
	{
		// pull the panel and check if it is visible
		pPanel = gViewPortInterface->FindPanelByName(szPanels[i]);
		if(pPanel && pPanel->IsVisible())
			return true;
	}

	return false;
}

// paint override
void HudStrategyMsg::Paint()
{
	// only paint if we're not visible...
	if (!IsVisible())
		return;

	// if we're out of time, then disappear.
	if (m_fDisplayEndTime < gpGlobals->curtime) {
		SetVisible(false);
		return;
	}

	// down
	BaseClass::Paint();
}