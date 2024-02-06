// LoadMemoryDlg.cpp : implementation file
//

#include "pch.h"

#include "resource.h"
#include "LoadMemoryDlg.h"
#include "MSFManager.h"
#include "Config.h"
#include "Screen_Sizes.h"
#include <vfw.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CLoadMemoryDlg

IMPLEMENT_DYNAMIC(CLoadMemoryDlg, CFileDialog)

CLoadMemoryDlg::CLoadMemoryDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
                               DWORD dwFlags, LPCTSTR lpszFilter, CWnd *pParentWnd, DWORD dwSize, BOOL bVistaStyle) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
{
	m_ofn.hwndOwner = pParentWnd->GetSafeHwnd();
	m_ofn.Flags |= OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE; // |OFN_ENABLESIZING;
	m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_LOAD_MEMORY);
	m_ofn.lpfnHook = OFNHookProc;
	m_ofn.lCustData = reinterpret_cast<LPARAM>(this);
}


BEGIN_MESSAGE_MAP(CLoadMemoryDlg, CFileDialog)
END_MESSAGE_MAP()


UINT_PTR CALLBACK CLoadMemoryDlg::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
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
				CLoadMemoryDlg::DrawPreview(hdlg, strPath.GetString());
			}

			break;
		}
	}

	return 0;
}


void CLoadMemoryDlg::ClearItems(CWnd *pDlg, const CString &strMsg)
{
	pDlg->GetDlgItem(IDC_PICTURE_LM_PREVIEW_BORDER)->ShowWindow(FALSE);
	pDlg->GetDlgItem(IDC_PICTURE_LM_PREVIEW_BORDER)->ShowWindow(TRUE);
	pDlg->SetDlgItemText(IDC_EDIT_LM_CONFIG, _T(""));
	pDlg->SetDlgItemText(IDC_EDIT_LM_VERSION, strMsg);
}

void CLoadMemoryDlg::DrawPreview(HWND hdlg, const fs::path &strPath)
{
	CWnd *pDlg = CWnd::FromHandle(hdlg);

	if (fs::exists(strPath))
	{
		if (fs::is_directory(strPath))
		{
			ClearItems(pDlg, _T("Директоря"));
		}
		else
		{
			auto pMgr = std::make_unique<CMSFManager>();

			if (pMgr)
			{
				if (pMgr->OpenFile(strPath, true, true))
				{
					DWORD vers = pMgr->GetVersion();

					if (vers > MSF_VERSION_CURRENT)
					{
						ClearItems(pDlg, _T("Неверная или неподдерживаемая версия MSF файла."));
					}
					else
					{
						if (vers < MSF_VERSION_CURRENT)
						{
							pDlg->SetDlgItemText(IDC_EDIT_LM_VERSION, _T("Устаревшая версия MSF файла."));
						}
						else
						{
							pDlg->SetDlgItemText(IDC_EDIT_LM_VERSION, _T("Актуальная версия MSF файла."));
						}

						// достанем номер конфигурации
						UINT nID = g_mstrConfigBKModelParameters[static_cast<DWORD>(pMgr->GetConfiguration())].nIDBKModelName;
						pDlg->SetDlgItemText(IDC_EDIT_LM_CONFIG, CString(MAKEINTRESOURCE(nID)));
						// теперь загрузим миниатюру скриншота экрана
						HBITMAP hBm = nullptr;
						uint8_t *pBits = nullptr;
						pMgr->GetBlockPreview(&hBm, &pBits);

						if (hBm)
						{
							CRect rcPreview;
							pDlg->GetDlgItem(IDC_PICTURE_LM_PREVIEW_BORDER)->GetWindowRect(&rcPreview);
							pDlg->ScreenToClient(rcPreview);
							rcPreview.DeflateRect(1, 1);
							HDRAWDIB hdd = ::DrawDibOpen();
							HDC hdc = ::GetDC(hdlg);
							DIBSECTION ds{};
							::GetObject(hBm, sizeof(ds), &ds);
							::DrawDibDraw(hdd, hdc,
							              rcPreview.left, rcPreview.top, rcPreview.Width(), rcPreview.Height(),
							              &ds.dsBmih, pBits,
							              0, 0, ds.dsBmih.biWidth, ds.dsBmih.biHeight,
							              DDF_BUFFER | DDF_HALFTONE);
							::ReleaseDC(hdlg, hdc);
							::DrawDibClose(hdd);
							::DeleteObject(hBm);
						}
					}
				}
				else
				{
					ClearItems(pDlg, _T("Ошибка открытия MSF файл."));
				}
			}
		}
	}
	else
	{
		ClearItems(pDlg, CString(MAKEINTRESOURCE(IDS_DISK_NOACCESS)));
	}
}
