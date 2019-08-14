#ifndef _TUTORIALMENU_H_
#define _TUTORIALMENU_H_

#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/HTML.h>
#include "hilitebutton.h"

#include <cl_dll/iviewport.h>

#define TUTORIAL_URL_RES "resource/TutorialURLs.res"

/**
* Class declaration for the tutorial menu
**/
class CTutorialMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE(CTutorialMenu, vgui::Frame);

public:
	CTutorialMenu(IViewPort *pViewPort);
	virtual ~CTutorialMenu();

	virtual const char *GetName(void) { return PANEL_TUTORIAL; }
	virtual void SetData(KeyValues *pData);
	virtual void Reset(void);
	virtual void Update(void);
	virtual bool NeedsUpdate(void) { return false; }
	virtual bool HasInputElements(void) { return true; }
	virtual void ShowPanel(bool bShow);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void SetData(const char *szCommand);

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel(void) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible(void) { return BaseClass::IsVisible(); }
  	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent( parent ); }

protected:	
	// helpers
	void MakeMessageVisible(vgui::HTML *pMessage, bool bVisible);
	void ShowURL(KeyValues *pPage, vgui::HTML *pMessage);
	void LoadNextHTMLMessage(void);
	void LoadPrevHTMLMessage(void);
	KeyValues *FindPageBefore(KeyValues *pPage);

	// vgui overrides
	virtual void OnCommand(const char *command);
	virtual vgui::Panel *CreateControlByName(const char *controlName);

	IViewPort	*m_pViewPort;
	char		m_szExitCommand[255];
	KeyValues	*m_pPages;
	KeyValues	*m_pCurrentPage;

	vgui::HTML		*m_pHTMLMessagePrev;
	vgui::HTML		*m_pHTMLMessageCurr;
	vgui::HTML		*m_pHTMLMessageNext;
	vgui::Label		*m_pTitleLabel;
	CHiliteTextButton *m_pNextButton;
	CHiliteTextButton *m_pPrevButton;
};

#endif