#include "cbase.h"
#include "hud_skill.h"
#include <vgui/isurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

/**
* Constructor
**/
HudSkill::HudSkill(const char *szName)
	: CHudElement(szName), m_bShouldDraw(false), BGPanel(NULL, "HudSkill")
{
	// set the parent
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent(pParent);

	SetHiddenBits( 0 );

	// we're visible
	SetVisible(true);
}

/**
* Applies the new scheme settings
*
* @param IScheme *pScheme The new scheme
* @return void
**/
void HudSkill::ApplySchemeSettings(IScheme *pScheme)
{
	// cache the scheme
	m_pScheme = pScheme;

	// move down
	BaseClass::ApplySchemeSettings(pScheme);

	// set the background
	//SetBgColor(m_pScheme->GetColor("Menu.BgColor", Color(255, 255, 255, 0)));

	// setup the font to use
	m_pLabel->SetFont(m_pScheme->GetFont("Trebuchet14", true));

	// don't draw the background
	SetPaintBackgroundEnabled(false);
}

/**
* Determines whether or not we should draw the hudskill element
*
* @return bool
**/
bool HudSkill::ShouldDraw(void)
{
	// grab the skill
	C_BasePlayer *pPlayer = C_SDKPlayer::GetLocalPlayer();

	if (!pPlayer || pPlayer->GetTeamNumber() <= TEAM_SPECTATOR)
		return false;

	return m_bShouldDraw && CHudElement::ShouldDraw();
}

/**
* Initializes the element
*
* @return void
**/
void HudSkill::Init(void)
{
	// reset
	CHudElement::Reset();

	// create the label
	m_pLabel = new Label(this, "", NO_SKILL_STR);

	// set the position
	m_pLabel->SetPos(SKILLCLASS_LABEL_XPOS, SKILLCLASS_LABEL_YPOS);
	m_pLabel->SetSize(SKILLCLASS_LABEL_WIDTH, SKILLCLASS_LABEL_HEIGHT);
	m_pLabel->SetContentAlignment(Label::a_northwest);
	m_pLabel->SetProportional(true);
}

/**
* Paints the texture and current skill level
*
* @return void
**/
void HudSkill::Paint(void)
{
	CSkillClass *pSkill;
	C_SDKPlayer *pPlayer;
	float fXP;
	Color sColor;

	// grab the skill
	pPlayer = dynamic_cast<C_SDKPlayer *>(C_SDKPlayer::GetLocalPlayer());
	if(!pPlayer)
		return;

	pSkill = pPlayer->GetSkillClass(true);
	if(!pSkill)
		return;

	// get the xp and check bounds
	fXP = pPlayer->GetXP();
	if(fXP > 0 && fXP <= MAX_XP)
	{
		// pull the color
		sColor = m_pScheme->GetColor("OrangeDim", Color(255, 0, 0, 255));

		// draw the level
		surface()->DrawSetColor(sColor);
		surface()->DrawFilledRect(SKILLCLASS_BAR_STARTX, 
									SKILLCLASS_BAR_STARTY, 
									SKILLCLASS_BAR_STARTX + (SKILLCLASS_BAR_WIDTH * (fXP / MAX_XP)),
									SKILLCLASS_BAR_STARTY + SKILLCLASS_BAR_HEIGHT);
	}

	// set the color
	m_pLabel->SetFgColor(m_pScheme->GetColor("OrangeDim", Color(255, 255, 255, 0)));

	// draw the background over the bar
	BaseClass::Paint();
}

/**
* Takes care of logic that occurs when the local player's skill information is updated
*
* @return void
**/
void HudSkill::LocalSkillUpdated(void)
{
	C_SDKPlayer *pPlayer;

	// get the current player
	pPlayer = dynamic_cast<C_SDKPlayer *>(C_SDKPlayer::GetLocalPlayer());

	// do we have a player? a skill class?
	if(!pPlayer || !pPlayer->GetSkillClass(true) || pPlayer->GetSkillClass(true)->GetClassIndex() == NONE_CLASS_INDEX)
	{
		// don't show the element
		m_bShouldDraw = false;
		return;
	}
	else
	{
		wchar_t *wszUni;
		CSkillClass *pSkill;
		char szStr[256];

		// grab the skill
		pSkill = pPlayer->GetSkillClass(true);

		// set the string
		if(pSkill->GetLevelBoost() != 0)
			Q_snprintf(szStr, sizeof(szStr), 
						(pSkill->GetLevelBoost() > 0 ? "Level %d %s (+%d)" : "Level %d %s (%d)"), 
						pSkill->GetSkillLevel(true) + 1, 
						pSkill->GetClassNameShort(), 
						pSkill->GetLevelBoost());
		else
			Q_snprintf(szStr, sizeof(szStr), "Level %d %s", pSkill->GetSkillLevel(true) + 1, pSkill->GetClassNameShort());

		// set the text after figuring out the unicode equivalent
		wszUni = vgui::localize()->Find(szStr);
		if (!wszUni)
		{
			// create some room and convert to ansi
			wszUni = new wchar_t[256];
			vgui::localize()->ConvertANSIToUnicode(szStr, wszUni, 256);
			m_pLabel->SetText(wszUni);
			delete [] wszUni;
		}
		else
			// just set the text
			m_pLabel->SetText(wszUni);

		// draw the element
		m_bShouldDraw = true;
	}
}

/**
* Futzes with the texture name to add a prefix based on the clients team
*
* @param const char *szName Name of the texture
* @param char *szStr The string to fill in
* @param int iSize The amount of buffer space we have
* @return void
**/
void HudSkill::GetBackgroundTextureName(const char *szName, char *szStr, int iSize)
{
	// do we have a player yet?
	if(CSDKPlayer::GetLocalPlayer())
	{
		// red or blue?
		if(CSDKPlayer::GetLocalPlayer()->GetTeamNumber() == TEAM_B)
			Q_snprintf(szStr, iSize, "vgui/red/%s", szName);
		else
			Q_snprintf(szStr, iSize, "vgui/blue/%s", szName);
	}
}

/**
* Handles case where the screen size changes
*
* @param int iOldWidth
* @param int iOldHeight
* @return void
**/
void HudSkill::OnScreenSizeChanged(int iOldWidth, int iOldHeight)
{
	// jump down
	BaseClass::OnScreenSizeChanged(iOldWidth, iOldHeight);

	// reset all of our size info
	if(m_pLabel)
	{
		m_pLabel->SetPos(SKILLCLASS_LABEL_XPOS, SKILLCLASS_LABEL_YPOS);
		m_pLabel->SetSize(SKILLCLASS_LABEL_WIDTH, SKILLCLASS_LABEL_HEIGHT);
	}
}