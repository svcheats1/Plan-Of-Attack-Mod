#ifndef _STRATEGYMANAGER_H_
#define _STRATEGYMANAGER_H_

#define GetStrategyManager CStrategyManager::GetInstance

class CStrategyManager
{
public:
	static CStrategyManager* GetInstance(void);
	static void Term(void);

	// send the current strategy to one player
	void SendStrategy(CBasePlayer *pPlayer);
	// send the current strategy to a whole team
	void SendStrategy(int iTeam);

	// one should be set for each team.
	void SetStrategyPicker(CBasePlayer *pPlayer);
	void ProcessChooseStrategy(CBasePlayer *pPlayer, const char *szStr, int iCommands, bool bIsBlock);
	void SendChooseStrategy(int iTeam);
	void RoundReset();
	void ChooseTimeExpired(int iTeam);
	bool HasStrategy(int iTeam) { return (m_aStrategies[iTeam].m_szCommands != NULL); }
	void PlayerLeft(CBasePlayer *pPlayer);

private:
	CStrategyManager();
	~CStrategyManager();

	// send the strategy by any filter, need a team number too
	void SendStrategy(IRecipientFilter &filter, int iTeam);

	struct STeamStrategy
	{
		char **m_szCommands;
		int m_iCommands;
		bool m_bSentStrategy;
	};

	CBasePlayer* m_pPickers[3];
	STeamStrategy m_aStrategies[3];

	// singleton
	static CStrategyManager *s_pInstance;
};

#endif