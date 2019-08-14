#ifndef _STRATEGYMAP_H_
#define _STRATEGYMAP_H_

#include <vgui_controls/Frame.h>
#include <cl_dll/iviewport.h>
#include "map.h"
#include "hilitebutton.h"
#include <KeyValues.h>
#include "strategyarrow.h"

// A strategy map is a map with arrows on it
class CStrategyMap : public CMap
{
	DECLARE_CLASS_SIMPLE(CStrategyMap, CMap);

public:
	CStrategyMap(vgui::Panel *pParent);
	~CStrategyMap();

	virtual bool LoadFromKeyValues(KeyValues *data);

	virtual void Paint();
	virtual void SetMapSize(int wide, int tall);
	virtual void MapReset();
	virtual void OnMouseReleased(vgui::MouseCode code);
	virtual void OnCursorEntered();
	virtual void OnCursorExited();

	// accessors & mutators
	virtual const char* GetName() { return m_szName; }
	const char* GetFilename(void) { return m_szFilename; }
	void SetFilename(const char* sz) { Q_strncpy(m_szFilename, sz, sizeof(m_szFilename)); }
	void SetKeyValues(KeyValues *pData);
	KeyValues *GetKeyValues(bool bCreate = false);
	bool ShouldTransmit(void) { return m_bShouldTransmit; }
	void SetShouldTransmit(bool bShouldTransmit) { m_bShouldTransmit = bShouldTransmit; }
	void SetZ(float z);

protected:
	// helpers
	virtual void AddArrow(CStrategicArrow *pArrow) { m_aArrows.AddToTail(pArrow); }
	virtual void AddArrowPoint(CStrategicArrow *pArrow, int iX, int iY, int iWidth);
	CStrategicArrow *GetArrow(int iIndex) { return (m_aArrows.IsValidIndex(iIndex) ? m_aArrows[iIndex] : NULL); }
	CUtlVector<CStrategicArrow *> *GetArrows(void) { return &m_aArrows; }

private:
	CUtlVector<CStrategicArrow*> m_aArrows;
	char m_szFilename[128], m_szName[64];
	bool m_bShouldTransmit;
	KeyValues *m_pKeyValues;


};

#endif