#ifdef UI
#include "pch.h"

#include "BKSettingsDlg.h"
#include "BK.h"
#include "Config.h"
#include "Screen_Sizes.h"

#include "BKSettPage_1.h"
#include "BKSettPage_2.h"
#include "BKSettPage_3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNAMIC(CSettingsDlg, CBaseDialog)

CSettingsDlg::CSettingsDlg()
	: CBaseDialog(CSettingsDlg::IDD)
{
}

BEGIN_MESSAGE_MAP(CSettingsDlg, CBaseDialog)
END_MESSAGE_MAP()


void CSettingsDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_TAB_SETT, m_wndTabLoc);
}

BOOL CSettingsDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	SetDlgItemText(IDC_STATIC_EMULCOMPILED, theApp.m_strCompileVersion);
	CRect rectTab;
	m_wndTabLoc.GetWindowRect(&rectTab);
	ScreenToClient(&rectTab);
	m_ctrTab.Create(CMFCTabCtrl::STYLE_3D_ROUNDED, rectTab, this, IDC_TAB_SETT,
	                CMFCTabCtrl::LOCATION_TOP);
	m_ctrTab.AutoDestroyWindow(FALSE);  // нужно обязательно, иначе глючит при закрытии
	// потому что что-то надо ещё видимо настроить.
	m_ctrTab.EnableTabSwap(TRUE);
//  m_ctrTab.EnableAutoColor(TRUE);
	m_ctrTab.SetActiveTabBoldFont(TRUE);
	// Добавляем вкладки
	auto pPage1 = new BKSettDlg_1;
	AddTab(pPage1, BKSettDlg_1::IDD, IDS_SETT_TABTITLE0);
	auto pPage3 = new BKSettDlg_3;
	AddTab(pPage3, BKSettDlg_3::IDD, IDS_SETT_TABTITLE1);
	auto pPage2 = new BKSettDlg_2;
	AddTab(pPage2, BKSettDlg_2::IDD, IDS_SETT_TABTITLE2);
	// Model Board
	CONF_BKMODEL n = g_Config.GetBKModel(); //Number();
	CString str;
	str.LoadString(g_mstrConfigBKModelParameters[static_cast<int>(n)].nIDBKModelName);
	SetDlgItemText(IDC_NAME_SETT_BOARD, str.GetString());
	m_ctrTab.RecalcLayout();
	m_ctrTab.RedrawWindow();
	CenterWindow(GetParent());
	return TRUE;
}

void CSettingsDlg::AddTab(CBaseDialog *pDlg, UINT nDlgID, UINT nTitleID)
{
	m_vDlgs.push_back(pDlg);
	VERIFY(pDlg->Create(nDlgID, &m_ctrTab));
	m_ctrTab.AddTab(pDlg, nTitleID, (UINT) - 1, FALSE);
}


// выход: true - нужен рестарт
//      false - не нужен рестарт
DWORD CSettingsDlg::Save()
{
	// Сохраним всё что менялось во вкладках (в диалогах вкладок)
	DWORD nResult = NO_CHANGES;

	for (auto p : m_vDlgs)
	{
		if (p)
		{
			nResult |= DWORD(p->SendMessage(WM_SETT_SENDTOTAB));
			// если во вкладке что-то поменялось, которое требует перезапуска эмулятора,
			// то будет установлен флаг CHANGES_NEEDREBOOT, если поменялась опция, которая не требует перезапуска,
			// то будет установлен флаг CHANGES_NOREBOOT
		}
	}

	return nResult;
}

void CSettingsDlg::OnCancel()
{
	ReleaseTabs();
	CBaseDialog::EndDialog(NO_CHANGES);
}

void CSettingsDlg::OnOK()
{
	DWORD nRes = Save(); // Сохраним
	ReleaseTabs();

	if (nRes & CHANGES_NEEDREBOOT)
	{
		CBaseDialog::EndDialog(CHANGES_NEEDREBOOT);
	}
	else if (nRes & CHANGES_NOREBOOT)
	{
		CBaseDialog::EndDialog(CHANGES_NOREBOOT);
	}
	else
	{
		// если изменений не было, или они не требуют перезапуска, то
		// возвращается IDCANCEL, чтобы не делать перезапуск эмулятора
		CBaseDialog::EndDialog(NO_CHANGES);
	}
}


void CSettingsDlg::ReleaseTabs()
{
	m_ctrTab.CleanUp();

	for (auto &p : m_vDlgs)
	{
		if (p)
		{
			p->DestroyWindow();
			delete p;
			p = nullptr;
		}
	}

	m_vDlgs.clear();
}

#endif