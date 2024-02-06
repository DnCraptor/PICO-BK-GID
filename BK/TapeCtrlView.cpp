// CTapeCtrlView.cpp: файл реализации
//
#ifdef UI
#include "pch.h"
#include "resource.h"
#include "TapeCtrlView.h"
#include "Config.h"
#include "Tape.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

// CTapeCtrlView

IMPLEMENT_DYNAMIC(CTapeCtrlView, CDocPaneDlgViewBase)

CTapeCtrlView::CTapeCtrlView()
	: m_pTape(nullptr)
	, m_bPlay(false)
{
}

CTapeCtrlView::~CTapeCtrlView()
    = default;

void CTapeCtrlView::DoDataExchange(CDataExchange *pDX)
{
	CDocPaneDlgViewBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_TC_RECORD, m_btnRecord);
	DDX_Control(pDX, IDC_BUTTON_TC_PLAY, m_btnPlay);
	DDX_Control(pDX, IDC_BUTTON_TC_STOP, m_btnStop);
}

BEGIN_MESSAGE_MAP(CTapeCtrlView, CDocPaneDlgViewBase)
	ON_WM_TIMER()
	ON_WM_DESTROY()
	ON_MESSAGE(WM_INITDIALOG, &CTapeCtrlView::HandleInitDialog)
	ON_COMMAND(IDC_CHECK_TC_AUTOBEGIN, &CTapeCtrlView::OnBnClickedTcAutobegin)
	ON_COMMAND(IDC_CHECK_TC_AUTOEND, &CTapeCtrlView::OnBnClickedTcAutoend)
	ON_COMMAND(IDC_CHECK_TC_RECORD, &CTapeCtrlView::OnTcRecord)
	ON_COMMAND(IDC_BUTTON_TC_STOP, &CTapeCtrlView::OnTcStop)
	ON_COMMAND(IDC_BUTTON_TC_PLAY, &CTapeCtrlView::OnTcPlay)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_TC_RECORD, &CTapeCtrlView::OnUpdateTcRecord)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_TC_STOP, &CTapeCtrlView::OnUpdateTcStop)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_TC_PLAY, &CTapeCtrlView::OnUpdateTcPlay)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_TC_AUTOBEGIN, &CTapeCtrlView::OnUpdateTcAutobegin)
	ON_UPDATE_COMMAND_UI(IDC_CHECK_TC_AUTOEND, &CTapeCtrlView::OnUpdateTcAutoend)
END_MESSAGE_MAP()


// обработчики сообщений CTapeCtrlView

LRESULT CTapeCtrlView::HandleInitDialog(WPARAM wp, LPARAM lp)
{
	CDocPaneDlgViewBase::HandleInitDialog(wp, lp);
	m_iconRecordActive = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TCTRL_RECORD));
	m_iconRecordPassive = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TCTRL_RECORD_DARK));
	m_iconRecordStop = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TCTRL_STOP));
	m_iconRecordStart = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TCTRL_PLAY));
	m_iconRecordPause = ::LoadIcon(::AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TCTRL_PAUSE));
	m_btnRecord.SetIcon(m_iconRecordPassive);
	m_btnPlay.SetIcon(m_iconRecordStart); m_bPlay = false;
	m_btnStop.SetIcon(m_iconRecordStop);
	SetTimer(BKTIMER_TAPECTRL, 300, nullptr);
// это хак, чтобы применялся цвет к тексту при изменении визуального стиля.
// но при этом перестают применяться визуальные стили вообще к этим контролам.
//  Всё надо вручную делать. Пока не будем применять.
//  ::SetWindowTheme(GetDlgItem(IDC_CHECK_TC_AUTOBEGIN)->GetSafeHwnd(), _T(""), _T(""));
//  ::SetWindowTheme(GetDlgItem(IDC_CHECK_TC_AUTOEND)->GetSafeHwnd(), _T(""), _T(""));
	return TRUE;
}

void CTapeCtrlView::StartPlayTape()
{
	OnTcPlay();
}

void CTapeCtrlView::InitParams(CTape *pTape)
{
	m_pTape = pTape;
}


void CTapeCtrlView::OnTcRecord()
{
	m_pTape->SetWaveLoaded(false);

	if (IsDlgButtonChecked(IDC_CHECK_TC_RECORD))
	{
		m_btnRecord.SetIcon(m_iconRecordActive);
		m_pTape->StartRecord(g_Config.m_bTapeAutoBeginDetection, g_Config.m_bTapeAutoEndDetection);
	}
	else
	{
		m_btnRecord.SetIcon(m_iconRecordPassive);
		m_pTape->StopRecord();
		ShowSaveDialog();
	}

	GetParent()->SetFocus();
}


void CTapeCtrlView::OnTcStop()
{
	if (m_pTape->IsWaveLoaded() && m_pTape->IsPlaying())
	{
		m_pTape->StopPlay();
		m_pTape->ResetPlayWavePos();
		m_btnPlay.SetIcon(m_iconRecordStart); m_bPlay = false;
	}

	if (m_btnRecord.GetCheck())
	{
		m_btnRecord.SetCheck(FALSE);
	}

	if (m_pTape->IsRecording())
	{
		m_btnRecord.SetIcon(m_iconRecordPassive);
		m_pTape->StopRecord();
		ShowSaveDialog();
	}
}


void CTapeCtrlView::OnTcPlay()
{
	if (m_pTape->IsWaveLoaded())
	{
		if (m_pTape->IsPlaying())
		{
			m_pTape->StopPlay();
			m_btnPlay.SetIcon(m_iconRecordStart); m_bPlay = false;
		}
		else
		{
			m_pTape->StartPlay();
			m_btnPlay.SetIcon(m_iconRecordPause); m_bPlay = true;
		}
	}
}

void CTapeCtrlView::OnUpdateTcRecord(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(g_Config.m_bEmulateSaveTape ? FALSE : TRUE);
}

void CTapeCtrlView::OnUpdateTcStop(CCmdUI *pCmdUI)
{
	bool b = g_Config.m_bEmulateLoadTape && g_Config.m_bEmulateSaveTape;
	pCmdUI->Enable(b ? FALSE : TRUE);
}

void CTapeCtrlView::OnUpdateTcPlay(CCmdUI *pCmdUI)
{
	bool b = g_Config.m_bEmulateLoadTape && g_Config.m_bEmulateSaveTape;
	bool b1 = m_pTape ? m_pTape->IsWaveLoaded() : false;
	pCmdUI->Enable(!b && b1 ? TRUE : FALSE);
}

void CTapeCtrlView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == BKTIMER_TAPECTRL)
	{
		if (m_pTape)
		{
			if (m_btnRecord.GetCheck() && !m_pTape->IsRecording())
			{
				m_btnRecord.SetCheck(FALSE);
				OnTcRecord();
			}

			if (m_bPlay && !m_pTape->IsWaveLoaded())
			{
				m_btnPlay.SetIcon(m_iconRecordStart); m_bPlay = false;
				m_btnPlay.EnableWindow(FALSE);
			}
		}
	}
	else
	{
		CPaneDialog::OnTimer(nIDEvent);
	}
}


void CTapeCtrlView::ShowSaveDialog()
{
	CString strFilterTape(MAKEINTRESOURCE(IDS_FILEFILTER_TAPE_SAVE));
	CString strWaveExt(MAKEINTRESOURCE(IDS_FILEEXT_WAVE));
	CString strTapeExt(MAKEINTRESOURCE(IDS_FILEEXT_TAPE));
	CString strBinExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
	CFileDialog dlg(FALSE, strTapeExt, nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
	                strFilterTape, this);
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strTapePath.c_str();

	if (dlg.DoModal() == IDOK)
	{
		fs::path strFilePath = dlg.GetPathName().GetString();
		CString strExt = strFilePath.extension().c_str();

		if (!strExt.CollateNoCase(strWaveExt))
		{
			m_pTape->SaveWaveFile(strFilePath);
		}
		else if (!strExt.CollateNoCase(strBinExt))
		{
			TAPE_FILE_INFO tfi;
			memset(&tfi, -1, sizeof(TAPE_FILE_INFO));
			m_pTape->GetWaveFile(&tfi);
			m_pTape->SaveBinFile(strFilePath, &tfi);
		}
		else if (!strExt.CollateNoCase(strTapeExt))
		{
			m_pTape->SaveMSFFile(strFilePath);
		}

		if (strFilePath.has_parent_path())
		{
			g_Config.m_strTapePath = strFilePath.parent_path();
		}
	}
}


void CTapeCtrlView::OnDestroy()
{
	if (m_iconRecordActive)
	{
		DestroyIcon(m_iconRecordActive);
	}

	if (m_iconRecordPassive)
	{
		DestroyIcon(m_iconRecordPassive);
	}

	if (m_iconRecordStop)
	{
		DestroyIcon(m_iconRecordStop);
	}

	if (m_iconRecordStart)
	{
		DestroyIcon(m_iconRecordStart);
	}

	if (m_iconRecordPause)
	{
		DestroyIcon(m_iconRecordPause);
	}

	CPaneDialog::OnDestroy();
}


void CTapeCtrlView::OnBnClickedTcAutobegin()
{
	g_Config.m_bTapeAutoBeginDetection = !g_Config.m_bTapeAutoBeginDetection;
}

void CTapeCtrlView::OnBnClickedTcAutoend()
{
	g_Config.m_bTapeAutoEndDetection = !g_Config.m_bTapeAutoEndDetection;
}

void CTapeCtrlView::OnUpdateTcAutobegin(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bTapeAutoBeginDetection);
}

void CTapeCtrlView::OnUpdateTcAutoend(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(g_Config.m_bTapeAutoEndDetection);
}
#endif