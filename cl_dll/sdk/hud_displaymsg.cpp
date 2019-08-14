#include "cbase.h"
#include "hud_displaymsg.h"
#include "hud_macros.h"
#include "iclientmode.h"
#include <cl_dll/iviewport.h>

#include <KeyValues.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IVGui.h>
#include <vgui/ILocalize.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_HUDELEMENT(HudDisplayMsg);
DECLARE_HUD_MESSAGE(HudDisplayMsg, HudDisplayMsg);
DECLARE_HUD_MESSAGE(HudDisplayMsg, ResetHudDisplayMsg);

/**
* Convar stuff
**/
static ConVar cl_tips("cl_tips", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Show tips");

/**
* Default constructor
**/
HudDisplayMsg::HudDisplayMsg(const char *szName)
	: CHudElement(szName), vgui::Panel(NULL, "HudDisplayMsg")
{
	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	// don't display
	m_fEndDisplayTime = -1;
	m_iTimeLeft = m_iLastTimeLeft = -1;

	// we want to be proportional
	SetProportional(true);

	// create the main label
	m_pLabel = new WrapLabel(this, "HudDisplayMsgLabel");
	m_pLabel->SetEnabled(true);
	m_pLabel->SetProportional(true);

	// create our sub label
	m_pSubLabel1 = new WrapLabel(this, "HudDisplayMsgSubLabel1");
	m_pSubLabel1->SetEnabled(true);
	m_pSubLabel1->SetProportional(true);

	// create the second sub label
	m_pSubLabel2 = new WrapLabel(this, "HudDisplayMsgSubLabel2");
	m_pSubLabel2->SetEnabled(true);
	m_pSubLabel2->SetProportional(true);

	// hide everything
	ShowMsgs(false);

	// no queue yet
	m_bQHeadChanged = false;

	// load all the msgs
	LoadMsgs();
}

/**
* Destructor
**/
HudDisplayMsg::~HudDisplayMsg()
{
	// kill our key values
	m_pMsgs->deleteThis();
	m_pMsgCounts->deleteThis();
}

/**
* Loads all the display messages from the resource
*
* @return void
**/
void HudDisplayMsg::LoadMsgs(void)
{
	// load the messages
	m_pMsgs = new KeyValues("DisplayMessages");
	m_pMsgs->LoadFromFile(filesystem, DISPLAY_MSG_RES);

	// create some room for the counts
	m_pMsgCounts = new KeyValues("DisplayMessageCounts");
}

/**
* Initializes the element
*
* @return void
**/
void HudDisplayMsg::Init(void)
{
	// hook the message
	HOOK_HUD_MESSAGE(HudDisplayMsg, HudDisplayMsg);
	HOOK_HUD_MESSAGE(HudDisplayMsg, ResetHudDisplayMsg);
	CHudElement::Reset();
}

void HudDisplayMsg::LevelInit(void)
{
	// reset everything
	ResetData();
}

/**
* Resets all the display message info.
*
* @param bf_read &msg Junk
* @return void
**/
void HudDisplayMsg::MsgFunc_ResetHudDisplayMsg(bf_read &msg)
{
	ResetData();
}

/**
* Resets our data.  Don't call this from reset!
*
* @return void
**/
void HudDisplayMsg::ResetData(void)
{
	KeyValues *pSubLabel;

	// kill the time, etc
	m_fEndDisplayTime = -1;
	m_bQHeadChanged = false;
	m_iTimeLeft = m_iLastTimeLeft = -1;

	// kill the queue
	for(int i = 0; i < m_aQ.Count(); ++i)
	{
		// does it have a sublabel?
		pSubLabel = (KeyValues *)m_aQ[i]->GetPtr("sublabel1");
		if(pSubLabel)
			pSubLabel->deleteThis();
		pSubLabel = (KeyValues *)m_aQ[i]->GetPtr("sublabel2");
		if(pSubLabel)
			pSubLabel->deleteThis();

		// kill the current element
		m_aQ[i]->deleteThis();
	}
	m_aQ.Purge();

	// hide everything
	ShowMsgs(false);

}

/**
* Defines a handler for the EndOfRound message
*
* @param bf_read &msg The message to handle
* @return void
**/
void HudDisplayMsg::MsgFunc_HudDisplayMsg(bf_read &msg)
{
	KeyValues *pMsg, *pSubMsg, *pInterrupts;

	// pull the main label and add it to the queue
	pMsg = ExtractMsg(msg);
	if(!pMsg)
	{
		// if you get here, you're probably trying to display a message
		// that does not exist in the res file (check your spelling and case)
		Assert(0);
		return;
	}

	// did we need a sublabel?
	if((bool)msg.ReadByte())
	{
		// pull the label
		pSubMsg = ExtractMsg(msg);

		// add it to the current message
		if(pSubMsg)
			pMsg->SetPtr("sublabel1", pSubMsg);
	}

	// a second one?
	if((bool)msg.ReadByte())
	{
		// pull the label
		pSubMsg = ExtractMsg(msg);

		// add it on to this one
		if(pSubMsg)
			pMsg->SetPtr("sublabel2", pSubMsg);
	}

	// should we display it?
	if(!ShouldDisplayMsg(pMsg))
		return;

	// figure out where to put it in the queue
	// is it critical?
	if(pMsg->GetInt("critical"))
	{
		// let the current one know it's being bumped
		if(m_aQ.IsValidIndex(0))
			m_aQ[0]->SetInt("was_bumped", 1);

		// does the new message have any interrupts?
		pInterrupts = pMsg->FindKey("interrupts");
		if(pInterrupts)
			RemoveInterrupts(pInterrupts);
		
		// add to the head and let folks know
		m_aQ.AddToHead(pMsg);
		m_bQHeadChanged = true;
		m_iTimeLeft = m_iLastTimeLeft = -1;
		m_fEndDisplayTime = -1;
	}
	else
	{
		// did the head change?
		if(!m_aQ.Count())
		{
			// let everyone know and reset our times
			m_bQHeadChanged = true;
			m_iTimeLeft = m_iLastTimeLeft = -1;
			m_fEndDisplayTime = -1;
		}

		// add to the tail
		m_aQ.AddToTail(pMsg);
	}
}

/**
* Removes interrupted elements from the queue
*
* @param KeyValues *pInterrupts The interrupts for the new message
* @return void
**/
void HudDisplayMsg::RemoveInterrupts(KeyValues *pInterrupts)
{
	KeyValues *pInterrupt;
	const char *szName;

	// flip through all of them
	for(pInterrupt = pInterrupts->GetFirstSubKey(); pInterrupt; pInterrupt = pInterrupt->GetNextKey())
	{
		// pull the name and flip through all the messages to clear the old ones out
		szName = pInterrupt->GetString(NULL);
		for(int i = 0; i < m_aQ.Count();)
		{
			// check the name
			if(FStrEq(szName, m_aQ[i]->GetName()))
			{
				// pull it out of the list
				m_aQ.Remove(i);
			}
			// if we didn't match the name we can increment our counter
			else
				++i;
		}
	}
}

/**
* Gets the next dm to display from the msg.  Sends back a copy
* of the corresponding key values to the calling function
*
* @param bf_read &msg The message to copy the data from
* @return KeyValues * A copy of the correct set of key values
**/
KeyValues *HudDisplayMsg::ExtractMsg(bf_read &msg)
{
	wchar_t *wszMsg;
	char szName[256];
	KeyValues *pValues;

	// read in the name of the current display message
	msg.ReadString(szName, sizeof(szName));
	if(!Q_strlen(szName))
		return NULL;

	// see if we can find the proper set. then make a copy
	pValues = m_pMsgs->FindKey(szName);
	if(!pValues)
		return NULL;
	pValues = pValues->MakeCopy();

	// do we have a base time?
	if(pValues->GetInt("timer") && pValues->GetInt("usetimerbase"))
		pValues->SetInt("timerbase", msg.ReadShort());

	// figure out the message text
	wszMsg = GetMsgText(msg, pValues->GetString("message"));
	if(!wszMsg)
	{
		// clear out the values and bail
		pValues->deleteThis();
		return NULL;
	}

	// set our string
	pValues->SetWString("text", wszMsg);
	delete [] wszMsg;

	return pValues;
}

/**
* Determines the text to use for the current message
*
* @param bf_read &msg The message to pull replacements from
* @param const char *szStr The string to get the text for
* @return wchar_t *
**/
wchar_t *HudDisplayMsg::GetMsgText(bf_read &msg, const char *szStr)
{
	wchar_t *wszMsg, *wszMsgCopy;
	bool bDeleteMsg = false;

	// do we have a real name?
	if(!Q_strlen(szStr))
		return NULL;

	// see if we have a localized version for it
	wszMsg = vgui::localize()->Find(szStr);

	// didn't get one, so convert whatever we have to unicode
	if(!wszMsg)
	{
		// convert to unicode
		wszMsg = new wchar_t[256];
		bDeleteMsg = true;
		vgui::localize()->ConvertANSIToUnicode(szStr, wszMsg, sizeof(wchar_t) * 256);
	}

	// copy the message.  this prevents localization stuff from learning
	// about the real message.  if we don't do this we will never get our %s stuff again
	wszMsgCopy = new wchar_t[256];
	wcsncpy(wszMsgCopy, wszMsg, wcslen(wszMsg));
	wszMsgCopy[wcslen(wszMsg)] = 0;

	// perform the replacements
	PerformReplacements(wszMsgCopy, msg);

	// did we have to create some space?
	if(bDeleteMsg)
		delete [] wszMsg;

	return wszMsgCopy;
}

/**
* Replaces all the %s info with data from msg
*
* @param char *szStr The string to replace stuff in
* @param bf_read &msg The message to use to replace stuff
* @return void
**/
void HudDisplayMsg::PerformReplacements(wchar_t *szStr, bf_read &msg)
{
	wchar_t *wszConverted, *wszPtr, wszNextString[256];
	char szTemp[256];
	int iSubs, iEnd;
	bool bDeleteMsgConverted = false;

	// make any necessary replacements
	iSubs = (int)msg.ReadShort();
	for(int i = 0; i < iSubs; ++i)
	{
		// pull the string
		memset((void *)szTemp, 0, sizeof(szTemp));
		msg.ReadString(szTemp, sizeof(szTemp));

		// see if we have a localized version for the substitution
		wszConverted = vgui::localize()->Find(szTemp);

		// did we get it?
		if(!wszConverted)
		{
			// convert to unicode
			wszConverted = new wchar_t[256];
			vgui::localize()->ConvertANSIToUnicode(szTemp, wszConverted, sizeof(wchar_t) * 256);
			bDeleteMsgConverted = true;
		}

		// find the first %s
		wszPtr = wcsstr(szStr, L"%s");

		// search failed: bail!
		if (wszPtr == NULL) {
			// clear out the space
			if(bDeleteMsgConverted)
				delete [] wszConverted;
			continue;
		}

		// increment past the %s
		wszPtr += 2;
		// copy everything after it
		wcsncpy(wszNextString, wszPtr, wcslen(wszPtr));
		wszNextString[wcslen(wszPtr)] = 0;

		// stop the current string after the first %s
		wszPtr[0] = 0;

		// we need to terminate the new string right after the combined length of the two
		iEnd = wcslen(szStr) + wcslen(wszConverted) - 2;

		// insert the replacement
		_snwprintf(szStr, wcslen(szStr) + wcslen(wszConverted) - 2, szStr, wszConverted);

		// add on the remainder of the string from above and terminate
		wcscpy(&szStr[iEnd], wszNextString);
		szStr[iEnd + wcslen(wszNextString)] = 0;

		// clear out the space
		if(bDeleteMsgConverted)
			delete [] wszConverted;
	}
}

/**
* Paints the background
*
* @return void
**/
void HudDisplayMsg::PaintBackground(void)
{
	// jump down if we're not hidden
	if(!m_bHidden)
		BaseClass::PaintBackground();
}

/**
* Paints the label
*
* @return void
**/
void HudDisplayMsg::Paint(void)
{
	KeyValues *pValues;

	// are we still in the correct time
	if(!m_bQHeadChanged && gpGlobals->curtime >= m_fEndDisplayTime)
	{
		// bump the first item off the queue
		if(m_aQ.Count())
		{
			// pull the element, remove it, and destroy it
			pValues = m_aQ[0];
			m_aQ.Remove(0);
			pValues->deleteThis();

			// queue changed
			m_bQHeadChanged = true;
		}

		// if there's nothing left, reset
		if(!m_aQ.Count())
		{
			m_iTimeLeft = m_iLastTimeLeft = -1;
			m_fEndDisplayTime = -1;
		}

		// hide our stuff, it will come back on in a second...
		ShowMsgs(false);
	}

	// display the head
	DisplayHead();

	// add any timers
	if(IsVisible())
		AddTimers();

	// if we're not hidden jump down
	if(!m_bHidden)
		BaseClass::Paint();
}

/**
* Adds timers to the item add the head of the queue.  Any #timer# tags in any
* of the three labels will be replaced with the different between gpGlobals->curtime
* and m_fEndDisplayTime
*
* @return void
**/
void HudDisplayMsg::AddTimers(void)
{
	KeyValues *pSubMsg1, *pSubMsg2;
	wchar_t wszSeconds[64], wszInt[8];
	wchar_t *wszPos, *wszConverted;
	int iTimerBase, iTimerTime;

	// check that the time changed
	if(m_iLastTimeLeft == m_iTimeLeft || m_iTimeLeft < 0)
		return;

	// check that there is a head
	if(!m_aQ.IsValidIndex(0))
		return;

	// is the head message or any of its sub messages using timers?
	pSubMsg1 = (KeyValues *)m_aQ[0]->GetPtr("sublabel1");
	pSubMsg2 = (KeyValues *)m_aQ[0]->GetPtr("sublabel2");
	if(!m_aQ[0]->GetInt("timer") && 
		(!pSubMsg1 || !pSubMsg1->GetInt("timer")) && 
		(!pSubMsg2 || !pSubMsg2->GetInt("timer")))
	{
		return;
	}

	// figure out the timer time.  if we have a timerbase we want to subtract
	// off that. otherwise, just use the length the thing is supposed to display for
	iTimerBase = m_aQ[0]->GetInt("timerbase");
	iTimerTime = (iTimerBase ? iTimerBase : m_aQ[0]->GetInt("length", DEFAULT_TIMER_LENGTH)) + (m_iTimeLeft - m_aQ[0]->GetInt("length", DEFAULT_TIMER_LENGTH));

	// figure out the text to use
	wszConverted = vgui::localize()->Find(iTimerTime == 1 ? "Second" : "Seconds");
	if(!wszConverted)
	{
		// convert to unicode
		vgui::localize()->ConvertANSIToUnicode(iTimerTime == 1 ? "1 second" : "%s seconds", 
												wszSeconds, sizeof(wchar_t) * 64);
	}
	else
		wcsncpy(wszSeconds, wszConverted, 64);
	
	// do we need to replace some stuff?
	// subsitute the %s for iTimeLeft
	if(iTimerTime > 1)
	{
		// find the %s
		wszPos = wcsstr(wszSeconds, L"%s");
		if(!wszPos)
		{
			Assert(0);
			return;
		}

		// write out the string for time left
		_snwprintf(wszInt, sizeof(wchar_t) * 8, L"%d", iTimerTime);

		// move everything in our string along to make room for the int
		// then copy the string in
		wcsncpy(wszPos + wcslen(wszInt), wszPos + 2, (wszPos + wcslen(wszInt)) - wszSeconds);
		memcpy(wszPos, wszInt, sizeof(wchar_t) * wcslen(wszInt));
	}

	// subsitute the timer in each of them
	SubstituteTimer(m_pLabel, m_aQ[0]->GetWString("text"), wszSeconds);
	if(pSubMsg1)
		SubstituteTimer(m_pSubLabel1, pSubMsg1->GetWString("text"), wszSeconds);
	if(pSubMsg2)
		SubstituteTimer(m_pSubLabel2, pSubMsg2->GetWString("text"), wszSeconds);
}

/**
* Subsitutes the the given string in place of #timer# and sets the text of the 
* given label to the result
*
* @param vgui::Label *pLabel The label whose text to set
* @param const wchar_t *wszText The text to find #timer# in
* @param const wchar_t *wszSub The text to subsitute
* @return void
**/
void HudDisplayMsg::SubstituteTimer(vgui::Label *pLabel, const wchar_t *wszText, const wchar_t *wszSub)
{
	wchar_t wszTextCopy[256];
	wchar_t *wszPos;

	// start with the main label
	wszText = m_aQ[0]->GetWString("text");
	if(wszText && (wszPos = wcsstr(wszText, L"#timer#")) != NULL)
	{
		// copy the string
		wcsncpy(wszTextCopy, wszText, 256);
		wszPos = wcsstr(wszTextCopy, L"#timer#");

		// move everything in our string along to make room for the new string
		wcsncpy(wszPos + wcslen(wszSub), wszPos + 7, (wszPos + wcslen(wszSub)) - wszTextCopy);
		memcpy(wszPos, wszSub, sizeof(wchar_t) * wcslen(wszSub));
		wszPos += wcslen(wszSub) + 1;
		*wszPos = 0;

		// set the new text
		pLabel->SetText(wszTextCopy);
	}
}

/**
* Determines if we are behind important panels
*
* @return bool
**/
bool HudDisplayMsg::BehindPanels(void)
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

	// don't draw over the strategy msg, since its a glorified tip
	CHudElement *elem = gHUD.FindElement("HudStrategyMsg");
	if (elem && elem->ShouldDraw())
		return true;

	return false;
}

/**
* Displays the message at the head of the queue
*
* @return void
**/
void HudDisplayMsg::DisplayHead(void)
{
	KeyValues *pValues, *pSubValues1, *pSubValues2;
	const char *szSound;
	EmitSound_t sSound;
	CBasePlayer *pPlayer;
	const wchar_t *wszText;

	// figure out how much time is left
	if(!m_bHidden)
	{
		// record the previous time left so we're not processing stuff every second
		// and then bump down the time remaining
		m_iLastTimeLeft = m_iTimeLeft;
		m_iTimeLeft = (int)(m_fEndDisplayTime - gpGlobals->curtime) + 1;
		if(m_iTimeLeft <= 0)
			m_iTimeLeft = -1;
	}

	// are there other menus up?  extend the display time and make us invisible
	if(m_aQ.IsValidIndex(0) && BehindPanels())
	{
		// start the display all over again
		m_fEndDisplayTime = gpGlobals->curtime + (float)m_aQ[0]->GetInt("length", DEFAULT_TIMER_LENGTH);

		// hide us
		ShowMsgs(false);
		return;
	}

	// bail if the head didn't change
	if(!m_bQHeadChanged)
		return;
	m_bQHeadChanged = false;

	// pull the head
	if(!m_aQ.IsValidIndex(0))
		return;
	pValues = m_aQ[0];

	// set the text
	wszText = pValues->GetWString("text");
	if(!wszText || !wcslen(wszText))
	{
		Assert(0);
		return;
	}
	m_pLabel->SetText(wszText);

	// do we need sub label text?
	pSubValues1 = (KeyValues *)pValues->GetPtr("sublabel1");
	if(pSubValues1)
	{
		// pull the text and set it
		wszText = pSubValues1->GetWString("text");
		if(!wszText || !wcslen(wszText))
		{
			Assert(0);
			return;
		}
		m_pSubLabel1->SetText(wszText);
	}

	// do we need the second sub label text?
	pSubValues2 = (KeyValues *)pValues->GetPtr("sublabel2");
	if(pSubValues2)
	{
		// pull the text and set it
		wszText = pSubValues2->GetWString("text");
		if(!wszText || !wcslen(wszText))
		{
			Assert(0);
			return;
		}
		m_pSubLabel2->SetText(wszText);
	}

	// set the time
	m_fEndDisplayTime = gpGlobals->curtime + (float)pValues->GetInt("length", DEFAULT_TIMER_LENGTH);

	// do the positioning stuff
	PositionLabel(m_pLabel, pValues);
	if(pSubValues1)
		PositionLabel(m_pSubLabel1, pSubValues1);
	if(pSubValues2)
		PositionLabel(m_pSubLabel2, pSubValues2);

	// everything is visible
	m_bHidden = false;
	SetVisible(true);
	m_pLabel->SetVisible(true);
	if(pSubValues1)
		m_pSubLabel1->SetVisible(true);
	else
		m_pSubLabel1->SetVisible(false); // this is redundant but...
	if(pSubValues2)
		m_pSubLabel2->SetVisible(true);
	else
		m_pSubLabel2->SetVisible(false); // this is redundant but...

	// play a sound if we have one as long as we weren't bumped
	szSound = pValues->GetString("sound");
	pPlayer = CBasePlayer::GetLocalPlayer();
	if(Q_strlen(szSound) && pPlayer && !pValues->GetInt("was_bumped"))
	{
		// setup the sound
		sSound.m_nChannel = CHAN_STATIC;
		sSound.m_flVolume = VOL_NORM;
		sSound.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_STATIC);
		sSound.m_pSoundName = szSound;

		// send it out
		CSingleUserRecipientFilter filter(pPlayer);
		filter.MakeReliable();
		pPlayer->EmitSound(filter, pPlayer->entindex(), sSound);
	}
}

/**
* Positions the label according to the alignment tag in the key values
*
* @param vgui::Label *pLabel The label to position
* @param KeyValues *pValues The key values to use
* @return void
**/
void HudDisplayMsg::PositionLabel(vgui::Label *pLabel, KeyValues *pValues)
{
	const char *szPos;

	// see if we can find the string
	szPos = pValues->GetString("alignment", "northwest");

	// what are we doing?
	if(FStrEq("center", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_center);
	else if(FStrEq("east", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_east);
	else if(FStrEq("north", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_north);
	else if(FStrEq("northeast", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_northeast);
	else if(FStrEq("west", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_west);
	else if(FStrEq("south", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_south);
	else if(FStrEq("southeast", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_southeast);
	else if(FStrEq("southwest", szPos))
		pLabel->SetContentAlignment(vgui::Label::a_southwest);
	else
		pLabel->SetContentAlignment(vgui::Label::a_northwest);
}

/**
* Determines whether or not we should draw the hud cash element
*
* @return bool
**/
bool HudDisplayMsg::ShouldDraw(void)
{
	// is there anything in the queue?
	return m_aQ.Count() > 0;
}

/**
* Applies the scheme settings?
*
* @param IScheme *pScheme The scheme to use
* @return void
**/
void HudDisplayMsg::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// apply the scheme
	BaseClass::ApplySchemeSettings(pScheme);

	// set the font and whatnot
	m_pLabel->SetFont(m_pScheme->GetFont("Trebuchet16", IsProportional()));
	m_pSubLabel1->SetFont(m_pScheme->GetFont("Trebuchet12", IsProportional()));
	m_pSubLabel2->SetFont(m_pScheme->GetFont("Trebuchet12", IsProportional()));

	// position and size
	m_pLabel->SetSize(GetWide() - (DISPLAY_MSG_X_POS * 2), GetTall() - (DISPLAY_MSG_Y_POS * 2));
	m_pSubLabel1->SetSize(GetWide() - (DISPLAY_MSG_SUBLABEL1_X_POS * 2), GetTall() - DISPLAY_MSG_SUBLABEL1_Y_POS);
	m_pSubLabel2->SetSize(GetWide() - (DISPLAY_MSG_SUBLABEL2_X_POS * 2), GetTall() - DISPLAY_MSG_SUBLABEL2_Y_POS);
	m_pLabel->SetPos(DISPLAY_MSG_X_POS, DISPLAY_MSG_Y_POS);
	m_pSubLabel1->SetPos(DISPLAY_MSG_SUBLABEL1_X_POS, DISPLAY_MSG_SUBLABEL1_Y_POS);
	m_pSubLabel2->SetPos(DISPLAY_MSG_SUBLABEL2_X_POS, DISPLAY_MSG_SUBLABEL2_Y_POS);

	// we want a background
	SetPaintBackgroundEnabled(true);
	SetPaintBackgroundType(2);
	SetBgColor(m_pScheme->GetColor("TransparentLightBlack", Color(0, 0, 0, 0)));
}

/**
* Sets the visibility of the panel
*
* @param bool bShow
* @return void
**/
void HudDisplayMsg::ShowMsgs(bool bShow)
{
	// set everything to the correct state
	SetVisible(bShow);
	m_pLabel->SetVisible(bShow);
	m_pSubLabel1->SetVisible(bShow);
	m_pSubLabel2->SetVisible(bShow);

	// set the internal state
	m_bHidden = !bShow;
}

/**
* Determines if we should display a particular message
* NOTE - This method will keep track of the number of times a message is displayed
*		so be careful to call it once per received message
*
* @param KeyValues *pMsg The message to make a decision on
* @return bool
**/
bool HudDisplayMsg::ShouldDisplayMsg(KeyValues *pMsg)
{
	// are we displaying tips?
	if(pMsg->GetInt("type") == DMTYPE_TIP && !cl_tips.GetInt())
		return false;

	// are we at the maximum?
	if(AtMaxForMsg(pMsg))
		return false;

	return true;
}

/**
* Determines if we are the maximum for a particular type of message
*
* @param KeyValues *pMsg The message to check on
* @return bool
**/
bool HudDisplayMsg::AtMaxForMsg(KeyValues *pMsg)
{
	const char *szName;
	int iMax;
	KeyValues *pResult;

	// tip or regular? if it's a tip we will use a sublabel
	if(pMsg->GetInt("type") == DMTYPE_TIP && pMsg->GetPtr("sublabel1"))
		pMsg = (KeyValues *)pMsg->GetPtr("sublabel1");

	// pull the count and the name
	szName = pMsg->GetName();
	iMax = pMsg->GetInt("count");
	if(!iMax)
		return false;
	
	// see if we can find it
	pResult = m_pMsgCounts->FindKey(szName, true);
	if(pResult)
	{
		// is it over?
		if(pResult->GetInt("count") >= iMax)
			return true;

		// increment the count
		pResult->SetInt("count", pResult->GetInt("count") + 1);
	}

	return false;
}

/******************************************************************************************/
/** WrapLabel Implementation **************************************************************/
/******************************************************************************************/

/**
* Constructor
**/
WrapLabel::WrapLabel(vgui::Panel *pParent, const char *szName)
: BaseClass(pParent, szName, "")
{
	// ?
}

/**
* Sets the text for the label
*
* @param const wchar_t *wszStr The text to use
* @return void
**/
void WrapLabel::SetText(const wchar_t *wszStr)
{
	vgui::HFont sFont;
	wchar_t *wszStrNew;
	vgui::IScheme *pScheme;
	int i, j, k, iWide;

	// figure out the font we'll be using
	sFont = vgui::INVALID_FONT;
	if(GetFont() == vgui::INVALID_FONT)
	{
		// pull the scheme and get the default font
		pScheme = vgui::scheme()->GetIScheme(GetScheme());
		if(pScheme)
			sFont = pScheme->GetFont("Default", IsProportional());
	}
	else
		sFont = GetFont();

	// clear the new string
	wszStrNew = new wchar_t[wcslen(wszStr) + 1];
	memset(wszStrNew, 0, wcslen(wszStr) + 1);

	// add new lines when we get too fat
	// start iterating through our characters
	for(i = j = iWide = 0; i < (int)wcslen(wszStr); ++i)
	{
		// is it a newline? clear out the width
		if(wszStr[i] == '\n')
			iWide = 0;
		// add on the width of the character
		else
		{
			// figure out the width of the character
			iWide += vgui::surface()->GetCharacterWidth(sFont, wszStr[i]);

			// are we over?
			if(iWide >= (GetWide() - 5))
			{
				// backtrack to the last space in the new str
				for(k = 0; k < j; ++k)
				{
					// is it a space?
					if(wszStrNew[j - k] == ' ')
					{

						// add the carriage return
						wszStrNew[j - k] = '\n';

						// slide both indices back
						j -= k - 1;
						i -= k - 1;

						break;
					}
				}

				// couldn't find it?
				// stick the newline in right here
				if(k == j)
					wszStrNew[j++] = '\n';

				// start over
				iWide = 0;
			}
		}

		// set the character
		wszStrNew[j++] = wszStr[i];
	}
	wszStrNew[j] = 0;

	// set the text
	BaseClass::SetText(wszStrNew);
	delete [] wszStrNew;
}