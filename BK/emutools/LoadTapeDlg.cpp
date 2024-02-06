// LoadTapeDlg.cpp : implementation file
//

#include "pch.h"
#include "resource.h"
#include "LoadTapeDlg.h"
#include "Config.h"
#include "Screen_Sizes.h"
#include "Tape.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadTapeDlg

IMPLEMENT_DYNAMIC(CLoadTapeDlg, CFileDialog)

CLoadTapeDlg::CLoadTapeDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
                           DWORD dwFlags, LPCTSTR lpszFilter, CWnd *pParentWnd, DWORD dwSize, BOOL bVistaStyle) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
{
	m_ofn.hwndOwner = pParentWnd->GetSafeHwnd();
	m_ofn.Flags |= OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
	m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_LOAD_TAPE);
	m_ofn.lpfnHook = OFNHookProc;
	m_ofn.lCustData = reinterpret_cast<LPARAM>(this);
}


BEGIN_MESSAGE_MAP(CLoadTapeDlg, CFileDialog)
END_MESSAGE_MAP()



UINT_PTR CALLBACK CLoadTapeDlg::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	static CRect rectHD;

	switch (uiMsg)
	{
		case WM_INITDIALOG:
		{
			::GetWindowRect(hdlg, &rectHD);
			break;
		}

		case WM_CHILDACTIVATE:
		{
			CWnd *pDlg = CWnd::FromHandle(hdlg);
			int nPixelW = ::GetDeviceCaps(pDlg->GetDC()->GetSafeHdc(), LOGPIXELSX);
			int nPixelH = ::GetDeviceCaps(pDlg->GetDC()->GetSafeHdc(), LOGPIXELSY);
			HWND hParent = ::GetParent(hdlg);
			CRect rectFD;
			BOOL bRes = ::GetWindowRect(hParent, &rectFD);
			CRect rectHD1;
			bRes = ::GetWindowRect(hdlg, &rectHD1);
			// увеличим окно в ширину
			bRes = ::MoveWindow(hParent, rectFD.left, rectFD.top, rectFD.Width() + rectHD.Width() + ::MulDiv(10, nPixelW, DEFAULT_DPIX), rectFD.Height() - rectHD.Height(), FALSE);
			// переместим превьюшку на своё новое место
			bRes = ::MoveWindow(hdlg, rectFD.Width() - ::MulDiv(15, nPixelW, DEFAULT_DPIX), -::MulDiv(390, nPixelH, DEFAULT_DPIY), rectHD.Width(), rectHD1.Height(), FALSE);
			break;
		}

		case WM_NOTIFY:
		{
			auto pNm = reinterpret_cast<LPNMHDR>(lParam);

			if (pNm->code == CDN_SELCHANGE)
			{
				CString strPath;
				HWND hParent = ::GetParent(hdlg);
				::SendMessage(hParent, CDM_GETFILEPATH, 1024, reinterpret_cast<LPARAM>(strPath.GetBufferSetLength(1024)));
				strPath.ReleaseBuffer();
				CLoadTapeDlg::DrawPreview(hdlg, strPath.GetString());
			}

			break;
		}
	}

	return 0;
}


void CLoadTapeDlg::ClearItems(CWnd *pDlg)
{
	pDlg->SetDlgItemText(IDC_EDIT_LT_NAME, _T(""));
	pDlg->SetDlgItemText(IDC_EDIT_LT_ADDRESS, _T(""));
	pDlg->SetDlgItemText(IDC_EDIT_LT_LENGTH, _T(""));
	pDlg->SetDlgItemText(IDC_EDIT_LT_TIME, _T(""));
}

void CLoadTapeDlg::DrawPreview(HWND hdlg, const fs::path &strPath)
{
	static CTape tape;
	CWnd *pDlg = CWnd::FromHandle(hdlg);

	if (fs::exists(strPath))
	{
		if (fs::is_directory(strPath))
		{
			pDlg->SetDlgItemText(IDC_EDIT_LT_TYPE, _T("Директория"));
			ClearItems(pDlg);
			return;
		}

		CString strType;
		bool bUnk = false;
		TAPE_FILE_INFO tfi;
		memset(&tfi, -1, sizeof(TAPE_FILE_INFO));

		if (tape.LoadWaveFile(strPath))
		{
			strType = CString(MAKEINTRESOURCE(IDS_LOADTAPE_TYPE_WAVE));
		}
		else if (tape.LoadMSFFile(strPath, true))
		{
			strType = CString(MAKEINTRESOURCE(IDS_LOADTAPE_TYPE_MSF));
		}
		else if (tape.LoadBinFile(strPath, &tfi))
		{
			strType = CString(MAKEINTRESOURCE(IDS_LOADTAPE_TYPE_BIN));
		}
		else
		{
			strType = CString(MAKEINTRESOURCE(IDS_LOADTAPE_TYPE_UNKNOWN));
			bUnk = true;
		}

		// Set preview controls
		pDlg->SetDlgItemText(IDC_EDIT_LT_TYPE, strType);

		if (bUnk)
		{
			ClearItems(pDlg);
			return;
		}

		// Get current wave file information
		tape.GetWaveFile(&tfi, true);
		CString strError(MAKEINTRESOURCE(IDS_LOADTAPE_ERRORVALUE));

		// Set file name
		if (tfi.name[0] == 255 && tfi.name[1] == 255)
		{
			pDlg->SetDlgItemText(IDC_EDIT_LT_NAME, strError);
		}
		else
		{
			pDlg->SetDlgItemText(IDC_EDIT_LT_NAME, Global::BKToUNICODE(tfi.name, 16));
		}

		CString strWord;

		// Set file address
		if (tfi.address == 65535)
		{
			strWord = strError;
		}
		else
		{
			Global::WordToOctString(tfi.address, strWord);
		}

		pDlg->SetDlgItemText(IDC_EDIT_LT_ADDRESS, strWord);

		// Set file length
		if (tfi.length == 65535)
		{
			strWord = strError;
		}
		else
		{
			Global::WordToOctString(tfi.length, strWord);  // длина в байтах, а не длина ваве
		}

		pDlg->SetDlgItemText(IDC_EDIT_LT_LENGTH, strWord);
		auto timeLen = int(float(tape.GetWaveLength()) * 1000.0 / float(tape.GetWorkingSSR()));
		CString strTime = Global::MsTimeToTimeString(timeLen);
		pDlg->SetDlgItemText(IDC_EDIT_LT_TIME, strTime);
	}
	else
	{
		pDlg->SetDlgItemText(IDC_EDIT_LT_TYPE, CString(MAKEINTRESOURCE(IDS_DISK_NOACCESS)));
		ClearItems(pDlg);
	}
}
