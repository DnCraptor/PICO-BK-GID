/////////////////////////////////////////////////////////////////////////////
//
// ShellBrowser.cpp: implementation of the CShellBrowser class.
//
#ifdef UI
#include "pch.h"
#include "BrowseForFolder.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
//
// Construction/Destruction
//

CBrowseForFolder::CBrowseForFolder(const HWND hParent /*= nullptr*/, const LPITEMIDLIST pidl /*= nullptr*/, const int nTitleID /*= 0*/)
{
	m_hwnd = nullptr;
	SetOwner(hParent);
	SetRoot(pidl);
	SetTitle(nTitleID);
	m_bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_SHAREABLE | BIF_USENEWUI | BIF_UAHINT;
	m_bi.lpfn = BrowseCallbackProc;
	m_bi.lParam = reinterpret_cast<LPARAM>(this);
	m_bi.pszDisplayName = m_szSelected;
}

CBrowseForFolder::CBrowseForFolder(const HWND hParent, const LPITEMIDLIST pidl, const CString &strCurrFolder, const CString &strTitle)
{
	m_hwnd = nullptr;
	SetOwner(hParent);
	SetRoot(pidl);
	SetTitle(strTitle);
	m_bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE | BIF_SHAREABLE | BIF_USENEWUI | BIF_UAHINT;
	m_bi.lpfn = BrowseCallbackProc;
	m_bi.lParam = reinterpret_cast<LPARAM>(this);
	m_bi.pszDisplayName = m_szSelected;
	m_strCurrFolder = strCurrFolder;
}

CBrowseForFolder::~CBrowseForFolder()
    = default;

/////////////////////////////////////////////////////////////////////////////
//
// Implementation
//

void CBrowseForFolder::SetOwner(const HWND hwndOwner)
{
	if (m_hwnd != nullptr)
	{
		return;
	}

	m_bi.hwndOwner = hwndOwner;
}

void CBrowseForFolder::SetRoot(const LPITEMIDLIST pidl)
{
	if (m_hwnd != nullptr)
	{
		return;
	}

	m_bi.pidlRoot = pidl;
}

CString CBrowseForFolder::GetTitle() const
{
	return m_bi.lpszTitle;
}

void CBrowseForFolder::SetTitle(const CString &strTitle)
{
	if (m_hwnd != nullptr)
	{
		return;
	}

	size_t len = static_cast<size_t>(strTitle.GetLength()) + 1;
	m_pchTitle = std::make_unique<TCHAR[]>(len);

	if (m_pchTitle)
	{
		_tcscpy_s(m_pchTitle.get(), len, strTitle);
		m_bi.lpszTitle = m_pchTitle.get();
	}
}

bool CBrowseForFolder::SetTitle(const int nTitle)
{
	if (nTitle <= 0)
	{
		return false;
	}

	CString strTitle;

	if (!strTitle.LoadString(static_cast<size_t>(nTitle)))
	{
		return false;
	}

	SetTitle(strTitle);
	return true;
}

void CBrowseForFolder::SetFlags(const UINT ulFlags)
{
	if (m_hwnd != nullptr)
	{
		return;
	}

	m_bi.ulFlags = ulFlags;
}

CString CBrowseForFolder::GetSelectedFolder() const
{
	return m_szSelected;
}

bool CBrowseForFolder::SelectFolder()
{
	bool bRet = false;
	LPITEMIDLIST pidl;

	if ((pidl = ::SHBrowseForFolder(&m_bi)) != nullptr)
	{
		m_strPath.Empty();

		if (::SHGetPathFromIDList(pidl, m_szSelected))
		{
			bRet = true;
			m_strPath = CString(m_szSelected);
		}

		LPMALLOC pMalloc;

		// Retrieve a pointer to the shell's IMalloc interface
		if (SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			// free the PIDL that SHBrowseForFolder returned to us.
			pMalloc->Free(pidl);
			// release the shell's IMalloc interface
			(void)pMalloc->Release();
		}
	}

	m_hwnd = nullptr;
	return bRet;
}

void CBrowseForFolder::OnInit(CWnd *pDlg) const
{
	SetSelection(m_strCurrFolder);
}

void CBrowseForFolder::OnSelChanged(const LPITEMIDLIST pidl) const
{
	(void)pidl;
}

void CBrowseForFolder::EnableOK(const BOOL bEnable) const
{
	if (m_hwnd == nullptr)
	{
		return;
	}

	(void)SendMessage(m_hwnd, BFFM_ENABLEOK, static_cast<WPARAM>(bEnable), 0);
}

void CBrowseForFolder::SetSelection(const LPITEMIDLIST pidl) const
{
	if (m_hwnd == nullptr)
	{
		return;
	}

	(void)SendMessage(m_hwnd, BFFM_SETSELECTION, FALSE, reinterpret_cast<LPARAM>(pidl));
}

void CBrowseForFolder::SetSelection(const CString &strPath) const
{
	if (m_hwnd == nullptr)
	{
		return;
	}

	(void)SendMessage(m_hwnd, BFFM_SETSELECTION, TRUE, reinterpret_cast<LPARAM>(LPCTSTR(strPath)));
}

void CBrowseForFolder::SetStatusText(const CString &strText) const
{
	if (m_hwnd == nullptr)
	{
		return;
	}

	(void)SendMessage(m_hwnd, BFFM_SETSTATUSTEXT, 0, reinterpret_cast<LPARAM>(LPCTSTR(strText)));
}

int CALLBACK CBrowseForFolder::BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
	TCHAR szPath[_MAX_PATH];
	auto pbff = reinterpret_cast<CBrowseForFolder *>(lpData);
	pbff->m_hwnd = hwnd;

	if (uMsg == BFFM_INITIALIZED)
	{
		::SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
		pbff->OnInit(CWnd::FromHandle(hwnd));
	}
	else if (uMsg == BFFM_SELCHANGED)
	{
		::SHGetPathFromIDList(LPITEMIDLIST(lParam), szPath);
		::SendMessage(hwnd, BFFM_SETSTATUSTEXT, 0, LPARAM(szPath));
		pbff->OnSelChanged(reinterpret_cast<LPITEMIDLIST>(lParam));
	}

	return 0;
}
#endif