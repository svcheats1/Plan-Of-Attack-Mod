#ifndef _SPAWNPOINT_H_
#define _SPAWNPOINT_H_

/**
* Spawn point class definition
**/
class CSpawnPoint : public CPointEntity
{
public:
	DECLARE_CLASS(CSpawnPoint, CPointEntity);

	DECLARE_DATADESC();

	// accessors
	virtual bool ClassMatches(const char *szClassOrWildcard);
	virtual bool ClassMatches(string_t szNameStr);
	virtual bool EnemiesNearby(CTeam *pTeam);
};

/**
* Base suppression spawn point
**/
class CSuppressionSpawnPoint : public CSpawnPoint
{
public:
	DECLARE_CLASS(CSuppressionSpawnPoint, CSpawnPoint);

	// constructor
	CSuppressionSpawnPoint()
		: m_bOccupied(false)
	{
		// ?
	}

	// accessors
	bool IsOccupied(void) { return m_bOccupied; }
	void SetOccupied(bool bOccupied) { m_bOccupied = bOccupied; }

private:
	bool m_bOccupied;
};

/**
* Class declaration for a coalition suppression start point
* We basically just need to know which objective the point is tied to
**/
class CSuppressionStartCoalition : public CSuppressionSpawnPoint
{
public:
	DECLARE_CLASS(CSuppressionStartCoalition, CSuppressionSpawnPoint);

	DECLARE_DATADESC();

	/**
	* Constructor
	**/
	CSuppressionStartCoalition()
	{
		// default to zero
		m_iObjective = 0;
	}

	// accessors
	int GetObjectiveID(void) { return m_iObjective; }
	
	/**
	* Pulls the info for the starting point from the key values
	*
	* @param const char *szKeyName Name of the variable
	* @param const char *szValue Value of the variable
	* @return bool True on success
	*/
	virtual bool KeyValue(const char *szKeyName, const char *szValue)
	{
		// copy any data we need
		if (FStrEq(szKeyName, "Objective"))
			m_iObjective = atoi(szValue);
		else
			// jump down
			return BaseClass::KeyValue( szKeyName, szValue );

		return true;
	}

private:
	int m_iObjective;
};

/**
* Class declaration for a american suppression start point
* Establishes groups for the objectives
**/
class CSuppressionStartAmerican : public CSuppressionSpawnPoint
{
public:
	DECLARE_CLASS(CSuppressionStartAmerican, CSuppressionSpawnPoint);

	DECLARE_DATADESC();

	/**
	* Constructor
	**/
	CSuppressionStartAmerican()
	{
		// default to zero
		m_iSpawnGroup = 0;
	}

	// accessors
	int GetSpawnGroup(void) { return m_iSpawnGroup; }
	
	/**
	* Pulls the info for the starting point from the key values
	*
	* @param const char *szKeyName Name of the variable
	* @param const char *szValue Value of the variable
	* @return bool True on success
	*/
	virtual bool KeyValue(const char *szKeyName, const char *szValue)
	{
		// copy any data we need
		if (FStrEq(szKeyName, "SpawnGroup"))
			m_iSpawnGroup = atoi(szValue);
		else
			// jump down
			return BaseClass::KeyValue( szKeyName, szValue );

		return true;
	}

private:
	int m_iSpawnGroup;
};


#endif