// LoadImgDlg.cpp: файл реализации
//

#include "pch.h"
#include "resource.h"
#include "BKParseImage.h"
#include "LoadImgDlg.h"
#include "Screen_Sizes.h"
#include "BKMessageBox.h"

// CLoadImgDlg
#pragma warning(disable:4996)

IMPLEMENT_DYNAMIC(CLoadImgDlg, CFileDialog)

CLoadImgDlg::CLoadImgDlg(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
                         DWORD dwFlags, LPCTSTR lpszFilter, CWnd *pParentWnd, DWORD dwSize, BOOL bVistaStyle) :
	CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dwSize, bVistaStyle)
{
	m_ofn.hwndOwner = pParentWnd->GetSafeHwnd();
	m_ofn.Flags |= OFN_EXPLORER | OFN_ENABLEHOOK | OFN_ENABLETEMPLATE;
	m_ofn.lpTemplateName = MAKEINTRESOURCE(IDD_LOAD_IMAGE);
	m_ofn.lpfnHook = OFNHookProc;
	m_ofn.lCustData = reinterpret_cast<LPARAM>(this);
}

CLoadImgDlg::~CLoadImgDlg()
    = default;


BEGIN_MESSAGE_MAP(CLoadImgDlg, CFileDialog)
END_MESSAGE_MAP()



// обработчики сообщений CLoadImgDlg
UINT_PTR CALLBACK CLoadImgDlg::OFNHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
	static CRect rectHD;
	// !!! под Win8 хук работает, но параметр ему не передаётся, поэтому обойдёмся статическими объектами

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
				CLoadImgDlg::DrawPreview(hdlg, strPath);
			}

			break;
		}
	}

	return 0;
}

void CLoadImgDlg::DrawPreview(HWND hdlg, const CString &strPath)
{
	CWnd *pDlg = CWnd::FromHandle(hdlg);
	CString strFormatName, strSystemFlag;
	GetImgFormat(strPath, strFormatName, strSystemFlag);
	pDlg->SetDlgItemText(IDC_EDIT_LI_SYSTEM, strFormatName);
	pDlg->SetDlgItemText(IDC_EDIT_LI_BOOTABLE, strSystemFlag);
}

void CLoadImgDlg::GetImgFormat(const CString &strImageName, CString &strFormatName, CString &strSystemFlag)
{
	CBKParseImage parser;
	fs::path name = strImageName.GetString();

	if (fs::exists(name))
	{
		if (fs::is_directory(name))
		{
			strFormatName = CString(MAKEINTRESOURCE(IDS_DISK_FORMAT_DIR));
		}
		else
		{
			PARSE_RESULT res = parser.ParseImage(name, 0);
			strSystemFlag = _T("");

			if (res.bImageBootable)
			{
				strSystemFlag = CString(MAKEINTRESOURCE(IDS_DISK_SYSTEM));
			}

			strFormatName = CString(CBKParseImage::GetOSName(res.imageOSType).c_str());

			// для возможностей локализации переопределим сообщения об Ошибке и неопознанном формате
			if (res.imageOSType == IMAGE_TYPE::ERROR_NOIMAGE)
			{
				strFormatName = CString(MAKEINTRESOURCE(IDS_DISK_FORMAT_ERROR));
			}
			else if (res.imageOSType == IMAGE_TYPE::UNKNOWN)
			{
				strFormatName = CString(MAKEINTRESOURCE(IDS_DISK_FORMAT_UNKNOWN));
				// теперь проверим на образ винчестера
				IMGFormat hddFormat;

				if (AnalyseImage(name, &hddFormat))
				{
					if (hddFormat.bSamara)
					{
						strFormatName = CString(MAKEINTRESOURCE(IDS_DISK_FORMAT_SAMARA));
					}
					else
					{
						strFormatName = CString(MAKEINTRESOURCE(IDS_DISK_FORMAT_ALTPRO));
					}
				}
			}
		}
	}
	else
	{
		strFormatName = CString(MAKEINTRESOURCE(IDS_DISK_NOACCESS));
	}
}

bool CLoadImgDlg::AnalyseImage(const fs::path &fname, IMGFormat *pFormat)
{
	FILE *f = nullptr;

	if ((f = _tfopen(fname.c_str(), _T("rb"))) != nullptr)
	{
		bool bRet = HDIStuff::CheckFormat(f, pFormat);
		fclose(f);
		return bRet;
	}

	return false;
}

