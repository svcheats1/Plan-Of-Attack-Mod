#include "cbase.h"
#include "mouseoverpanelbutton.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

MouseOverPanelButton::MouseOverPanelButton(vgui::Panel *parent, const char *panelName, const char *szText)
	: Button(parent, panelName, szText)
{
	// create the panel
	m_pPanel = new vgui::EditablePanel( parent, NULL );
	m_pPanel->SetVisible( false );

	SetResName(GetName());
}

MouseOverPanelButton::MouseOverPanelButton(vgui::Panel *parent, const char *panelName, vgui::Panel *templatePanel ) :
				Button( parent, panelName, "MouseOverPanelButton")
{
	m_pPanel = new vgui::EditablePanel( parent, NULL );
	m_pPanel->SetVisible( false );

	// copy size&pos from template panel
	int x,y,wide,tall;
	templatePanel->GetBounds( x, y, wide, tall );
	m_pPanel->SetBounds( x, y, wide, tall );

	SetResName(GetName());
}

void MouseOverPanelButton::SetResName(const char *szName)
{
	// set the name
	Q_strncpy(m_szResName, szName, sizeof(m_szResName));
}

void MouseOverPanelButton::ShowPage()
{
	if( m_pPanel )
	{
		m_pPanel->SetVisible( true );
		m_pPanel->MoveToFront();
		//g_lastPanel = m_pPanel;
	}
}

void MouseOverPanelButton::HidePage()
{
	if ( m_pPanel )
	{
		m_pPanel->SetVisible( false );
		//g_lastPanel = m_pPanel;
	}
}

/**
* Figures out which resource file we need to load for this button
*
* @return const char *
**/
const char *MouseOverPanelButton::GetPanelRes(void)
{
	static char classPanel[ _MAX_PATH ];
	Q_snprintf( classPanel, sizeof( classPanel ), "resource/ui/mouseoverpanels/%s.res", m_szResName);

	if(!vgui::filesystem()->FileExists(classPanel) && 
		vgui::filesystem()->FileExists("resource/ui/mouseoverpanels/default.res"))
	{
		Q_snprintf ( classPanel, sizeof( classPanel ), "resource/ui/mouseoverpanels/default.res" );
	}
	else if(!vgui::filesystem()->FileExists(classPanel))
	{
		return NULL;
	}

	return classPanel;
}

void MouseOverPanelButton::ApplySettings( KeyValues *resourceData ) 
{
	const char *szName;

	// did we get anything?
	if(resourceData)
		BaseClass::ApplySettings( resourceData );

	// name, position etc of button is set, now load matching
	// resource file for associated info panel:
	szName = GetPanelRes();
	Assert(szName);
	if(szName)
		m_pPanel->LoadControlSettings(szName);
}

void MouseOverPanelButton::OnCursorExited(void)
{
	BaseClass::OnCursorExited();

	if ( m_pPanel && IsEnabled() )
		HidePage();
}

void MouseOverPanelButton::OnCursorEntered() 
{
	BaseClass::OnCursorEntered();

	if ( m_pPanel && IsEnabled() )
	{
		/*if ( g_lastPanel )
		{
			g_lastPanel->SetVisible( false );
		}*/
		ShowPage();
	}
}