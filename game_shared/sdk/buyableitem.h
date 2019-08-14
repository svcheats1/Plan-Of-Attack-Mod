#ifndef _IBUYABLEITEM_H_
#define _IBUYABLEITEM_H_

/**
* Enum defining the item types
* WHEN CHANGING THIS, UPDATE SDK_WEAPON_PARSE.CPP!!!!!
**/
enum BI_Type
{
	BI_TYPE_NONE = -1,
	BI_TYPE_PISTOL,
	BI_TYPE_SHOTGUN,
	BI_TYPE_SMG,
	BI_TYPE_RIFLE,
	BI_TYPE_EXPLOSIVES,
	BI_TYPE_COUNT,
};

enum BI_Team
{
	BI_TEAM_BOTH = 0,
	BI_TEAM_A,
	BI_TEAM_B,
	BI_TEAM_COUNT,
};

/**
* Class definition for a buyable item interface
*
* @NOTE: BI stands for BuyableItem...go figure
**/
class IBuyableItem
{
public:
	virtual void BI_Init(void) { }	// implement this function if you need to load something or whatever
	virtual const char *GetClassType(void);
	virtual BI_Type GetType(void) const = 0; // returns the type of the item
	virtual int GetPosition(void) const = 0; // returns the slot we want this item to fall into in the buy menu
	virtual const char *GetItemName(void) const = 0; // returns the name of the item
	virtual int GetPrice(void) const = 0; // returns the price of the item
	virtual BI_Team GetBuyableTeam(void) const = 0;	// returns which team can buy this item

	// Comparison function to check positioning
	static int Compare(IBuyableItem * const *ppLeft, IBuyableItem * const *ppRight)
	{
		return (*ppLeft)->GetPosition() - (*ppRight)->GetPosition();
	}

private:
	CBaseEntity *m_pEntity;
};

/**
* Determines the class name of this entity
*
* @return const char*
**/
inline const char *IBuyableItem::GetClassType(void)
{
	CBaseEntity *pEntity = dynamic_cast<CBaseEntity *>(this);
	Assert(pEntity);

	// did we get it?
	if(pEntity)
		return pEntity->GetClassname();
	else
		return "";
}

#endif