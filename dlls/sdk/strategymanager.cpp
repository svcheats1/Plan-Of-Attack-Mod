#include "cbase.h"
#include "strategymanager.h"
#include "objectivemanager.h"
#include "team.h"
#include "sdk_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define BOOL_TO_BYTE(b) (b ? 1 : 0)

// declaration of independence
CStrategyManager *CStrategyManager::s_pInstance = NULL;

CStrategyManager* CStrategyManager::GetInstance(void)
{
	if (!s_pInstance)	
		s_pInstance = new CStrategyManager();

	return s_pInstance;
}

void CStrategyManager::Term(void)
{
	delete s_pInstance;
}

CStrategyManager::CStrategyManager()
{
	memset(m_aStrategies, 0, sizeof(m_aStrategies));

	RoundReset();
}

CStrategyManager::~CStrategyManager()
{
	RoundReset();
}

// one should be set for each team.
void CStrategyManager::SetStrategyPicker(CBasePlayer *pPlayer)
{
	if (pPlayer)
		m_pPickers[pPlayer->GetTeamNumber()] = pPlayer;
}

// this should only be called by the defending team, as the offensive team should
// have it chained on the client-side by the chooseobjective panel.
void CStrategyManager::SendChooseStrategy(int iTeam)
{
	// failure cases
	if(!g_pGameRules || !((CSDKGameRules *)g_pGameRules)->GetCurrentRound() || 
		(iTeam != TEAM_A && iTeam != TEAM_B) || !m_pPickers[iTeam])
		return;

	// get the time remaining to choose
	float fTime;
	bool bOffensive = m_pPickers[iTeam]->GetTeam()->IsOffensive();
	if (bOffensive)
		fTime = ((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->GetRemainingOffensiveFreezeTime();
	else
		fTime = ((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->GetRemainingDefensiveChooseTime();

	// send it off
	CSingleUserRecipientFilter filter(m_pPickers[iTeam]);
	UserMessageBegin(filter, "ChooseStrategy");
		WRITE_BYTE(BOOL_TO_BYTE(bOffensive));
		WRITE_FLOAT(fTime);
	MessageEnd();
}

void CStrategyManager::SendStrategy(CBasePlayer *pPlayer)
{
	CSingleUserRecipientFilter filter(pPlayer);
	SendStrategy(filter, pPlayer->GetTeamNumber());
}

void CStrategyManager::SendStrategy(int iTeam)
{
	if (iTeam < TEAM_A || iTeam > TEAM_B)
		return;

	CTeamRecipientFilter filter(iTeam, true);
	SendStrategy(filter, iTeam);
}

void CStrategyManager::SendStrategy(IRecipientFilter &filter, int iTeam)
{
	// must have valid team, strategy, picker
    if (!m_aStrategies[iTeam].m_szCommands || !m_pPickers[iTeam] || m_aStrategies[iTeam].m_bSentStrategy)
		return;

	// shortcut pointer
	CBasePlayer *pPlayer = m_pPickers[iTeam];

	// make sure we have everything in case the player disconnects
	if(!pPlayer || !pPlayer->GetTeam() || (pPlayer->GetTeam()->IsOffensive() && !GET_OBJ_MGR()->GetCurrentObjective()))
		return;

	// try to get the data out of the stored strategy
	int iTransmit;
	char szData[256];
	if (sscanf(m_aStrategies[iTeam].m_szCommands[0], "%d %255s", &iTransmit, szData) < 2)
		return;

	m_aStrategies[iTeam].m_bSentStrategy = true;
	
	// send it off
	UserMessageBegin(filter, "StrategyMsg");

	if (pPlayer->GetTeam()->IsOffensive()) {
		CObjective *pObj = GET_OBJ_MGR()->GetCurrentObjective();
		// offensive
		WRITE_BYTE(BOOL_TO_BYTE(true));
		// base?
		WRITE_BYTE(BOOL_TO_BYTE(!pObj->IsRequiredToWin()));
		// the standard obj stuff
		((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->WriteObjectiveInfo(pObj);
	}
	else {
		// defensive
		WRITE_BYTE(BOOL_TO_BYTE(false));
		// defendable objectives
		WRITE_STRING(GET_OBJ_MGR()->GetDefendableObjectives(pPlayer->GetTeamNumber()));
	}

	// transmit?
	WRITE_BYTE(iTransmit);
	// number of commands to expect
	WRITE_SHORT(m_aStrategies[iTeam].m_iCommands);

	// done
	MessageEnd();

	// write each command
	for(int i = 0; i < m_aStrategies[iTeam].m_iCommands; ++i)
	{
		// split out the real data
		memset(szData, 0, sizeof(szData));
		if(iTransmit)
		{
			// data is everything after the second space
			// if there are more than 99 arrows we're fucked anyway
			if(m_aStrategies[iTeam].m_szCommands[i][3] == ' ')
				memcpy(szData, &(m_aStrategies[iTeam].m_szCommands[i][4]), Q_strlen(m_aStrategies[iTeam].m_szCommands[i]) - 4);
			else if(m_aStrategies[iTeam].m_szCommands[i][4] == ' ')
				memcpy(szData, &(m_aStrategies[iTeam].m_szCommands[i][5]), Q_strlen(m_aStrategies[iTeam].m_szCommands[i]) - 5);
		}
		else
		{
			if(sscanf(m_aStrategies[iTeam].m_szCommands[i], "%d %255s", &iTransmit, szData) < 2)
			{
				Assert(0);
				return;
			}
		}

		// write the next command
		UserMessageBegin(filter, "StrategyMsgBlock");
		WRITE_STRING(szData);
		MessageEnd();
	}
}

void CStrategyManager::ProcessChooseStrategy(CBasePlayer *pPlayer, const char *szStr, int iCommands, bool bIsBlock)
{
	// need a player
	if (!pPlayer)
		return;

	// team number
	int iTeam = pPlayer->GetTeamNumber();

	// not on a valid team
	if (iTeam < TEAM_A || iTeam > TEAM_B)
		return;

	// player is not the picker
	if (pPlayer != m_pPickers[iTeam])
		return;

	// strategy is already picked
	if (m_aStrategies[iTeam].m_szCommands != NULL && !bIsBlock)
		return;
	
	// avoid a crash.  this would occur if a message was skipped or they came out of order
	if(m_aStrategies[iTeam].m_szCommands == NULL && bIsBlock)
	{
		DevWarning("Received block with no space for commands!");
		return;
	}

	// check the remaining choose time left
	bool bOffensive = m_pPickers[iTeam]->GetTeam()->IsOffensive();
	if (bOffensive) {
		if (!((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->InOffensiveFreezeTime())
			return;
	}
	else {
		if (!((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->InDefensiveChooseTime())
			return;
	}

	// if we're not adding a block we need to create enough room for all the blocks
	if(!bIsBlock)
	{
		// create enough room for all the commands
		m_aStrategies[iTeam].m_szCommands = new char *[iCommands + 1];
		for(int i = 0; i < iCommands + 1; ++i)
		{
			m_aStrategies[iTeam].m_szCommands[i] = new char[256];
			memset(m_aStrategies[iTeam].m_szCommands[i], 0, 256);
		}

		// copy in the first command and set the total count
		Q_strncpy(m_aStrategies[iTeam].m_szCommands[0], szStr, 256);
		m_aStrategies[iTeam].m_iCommands = iCommands + 1;
	}
	else
	{
		// if we're a block then we can just copy the string in
		Q_strncpy(m_aStrategies[iTeam].m_szCommands[m_aStrategies[iTeam].m_iCommands - (iCommands + 1)], szStr, 256);
	}
	
	// JD: send the strategy immediately if freeze time is over
	if (!bOffensive && iCommands + 1 == m_aStrategies[iTeam].m_iCommands && 
		!((CSDKGameRules*)g_pGameRules)->GetCurrentRound()->InDefensiveFreezeTime())
	{
		SendStrategy(iTeam);
	}
}

void CStrategyManager::RoundReset()
{
	// clear out the data.
	for(int i = 0; i < 3; ++i) {
        m_pPickers[i] = NULL;

		if (m_aStrategies[i].m_szCommands)
		{
			for(int j = 0; j < m_aStrategies[i].m_iCommands; ++j)
				delete [] m_aStrategies[i].m_szCommands[j];
			delete [] m_aStrategies[i].m_szCommands;
		}

        m_aStrategies[i].m_szCommands = NULL;
		m_aStrategies[i].m_bSentStrategy = false;
	}
}


/**
* Determines the action to take when a player leaves the game
*
* @param CBasePlayer *pPlayer The player who left
* @return void
**/
void CStrategyManager::PlayerLeft(CBasePlayer *pPlayer)
{
	// are they a picker?
	for(int i = 0; i < 3; ++i)
	{
		// is it them?
		if(m_pPickers[i] == pPlayer)
			m_pPickers[i] = NULL;
	}
}