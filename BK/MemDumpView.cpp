#ifdef UI
#include "pch.h"

#include "MemDumpView.h"
#include "BKMessageBox.h"
#include "Board.h"
#include "Config.h"
#include "Debugger.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// CMemDumpView

DISPLAY_MODE_PARAM CMemDumpView::m_dmArray[DISPLAY_MODES_NUM] =
{
	{ DUMP_DISPLAY_MODE::DD_WORD_VIEW, IDS_MEMDUMP_WORD },
	{ DUMP_DISPLAY_MODE::DD_BYTE_VIEW, IDS_MEMDUMP_BYTE }
};


IMPLEMENT_DYNAMIC(CMemDumpView, CDocPaneDlgViewBase)

CMemDumpView::CMemDumpView()
	: m_pDebugger(nullptr)
	, m_nCurrentDDM(0)
	, m_pV(nullptr)
	, m_strDumpAddr(_T("000000"))
	, m_pDmpEAPtr(nullptr)
{
}

CMemDumpView::~CMemDumpView()
    = default;

void CMemDumpView::DoDataExchange(CDataExchange *pDX)
{
	CDocPaneDlgViewBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MD_MEMORY, m_ListMemory);
	DDX_Control(pDX, IDC_EDIT_MD_DUMPADDR, m_EditAddr);
	DDX_Text(pDX, IDC_EDIT_MD_DUMPADDR, m_strDumpAddr);
	DDX_Control(pDX, IDC_COMBO_MD_WINDOWS, m_cbWndList);
	DDX_Control(pDX, IDC_COMBO_MD_WINDADDR, m_cbWndAddr);
}

BEGIN_MESSAGE_MAP(CMemDumpView, CDocPaneDlgViewBase)
	ON_WM_SETFOCUS()
	ON_MESSAGE(WM_INITDIALOG, &CMemDumpView::HandleInitDialog)
	ON_MESSAGE(WM_DBG_TOP_ADDRESS_UPDATE, &CMemDumpView::OnDumpAddressChange)
	ON_COMMAND(IDC_BUTTON_MD_DUMPMODE, &CMemDumpView::OnBnClickedButtonDumpMode)
	ON_COMMAND(IDC_BUTTON_MD_SAVE, &CMemDumpView::OnBnClickedButtonSaveDump)
	ON_COMMAND(IDC_BUTTON_MD_LOAD, &CMemDumpView::OnBnClickedButtonLoadDump)
	ON_CBN_SELCHANGE(IDC_COMBO_MD_WINDOWS, &CMemDumpView::OnCbnSelchangeComboMdWindows)
	ON_CBN_SELCHANGE(IDC_COMBO_MD_WINDADDR, &CMemDumpView::OnCbnSelchangeComboMdWindaddr)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_MD_DUMPMODE, &CMemDumpView::OnUpdateButton)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_MD_SAVE, &CMemDumpView::OnUpdateButton)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_MD_LOAD, &CMemDumpView::OnUpdateButton)

END_MESSAGE_MAP()

void CMemDumpView::AttachDebugger(CDebugger *pDbgr)
{
	m_pDebugger = pDbgr;
}

/////////////////////////////////////////////////////////////////////////////
// обработчики сообщений CMemDumpView

LRESULT CMemDumpView::HandleInitDialog(WPARAM wp, LPARAM lp)
{
	CDocPaneDlgViewBase::HandleInitDialog(wp, lp);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_MD_DUMPADDR), IDS_TOOLTIP_EDIT_MD_DUMPADDR);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_MD_DUMPMODE), IDS_TOOLTIP_BUTTON_MD_DUMPMODE);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_MD_SAVE), IDS_TOOLTIP_BUTTON_MD_SAVE);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_MD_LOAD), IDS_TOOLTIP_BUTTON_MD_LOAD);
	m_ToolTip.AddTool(GetDlgItem(IDC_COMBO_MD_WINDOWS), IDS_TOOLTIP_COMBO_MD_WINDOWS);
	m_ToolTip.AddTool(GetDlgItem(IDC_COMBO_MD_WINDADDR), IDS_TOOLTIP_COMBO_MD_WINDADDR);
	m_EditAddr.SetLimitText(6);
	SetDlgItemText(IDC_BUTTON_MD_DUMPMODE, CString(MAKEINTRESOURCE(m_dmArray[m_nCurrentDDM].strID))); // на кнопке нарисуем начальное значение
	m_ListMemory.SetFont(&m_hFont, FALSE);
	m_ListMemory.Init(m_dmArray[m_nCurrentDDM].mode); // создадим только колонки
	m_cbWndAddr.AddString(_T("000000"));
	m_cbWndAddr.AddString(_T("040000"));
	m_cbWndAddr.AddString(_T("100000"));
	m_cbWndAddr.AddString(_T("140000"));
	m_cbWndAddr.SetCurSel(0);
	m_ListMemory.SetAddress(m_ListMemory.m_nDmpWndAddr);
	return TRUE;  // возврат значения TRUE, если фокус не передан элементу управления
}

void CMemDumpView::OnSetFocus(CWnd *pOldWnd)
{
	m_ListMemory.SetFocus();
}

void CMemDumpView::DisplayMemDump()
{
	if (IsVisible())
	{
		m_ListMemory.RefreshItems();
	}
}

void CMemDumpView::SetAddress(uint16_t addr)
{
	m_ListMemory.SetAddress(addr);
	m_strDumpAddr = Global::WordToOctString(addr);
	UpdateData(FALSE);
}

void CMemDumpView::SetDumpWindows(VWinParam *pV, int nPageListPos, int nAddrListPos)
{
	m_pV = pV;
	int nCount = m_cbWndList.GetCount();

	if (nCount > 0)
	{
		for (int i = nCount - 1; i >= 0; --i)
		{
			m_cbWndList.DeleteString(i);
		}
	}

	m_cbWndList.AddString(_T("АП БК 64Кб"));

	if (!pV->empty())
	{
		for (auto &n : *pV)
		{
			m_cbWndList.AddString(n.strName);
		}

		m_cbWndList.EnableWindow(TRUE);
	}
	else
	{
		m_cbWndList.EnableWindow(FALSE);
	}

	m_cbWndAddr.SetCurSel(nAddrListPos);
	m_cbWndList.SetCurSel(nPageListPos);
	// обновим данные
	OnCbnSelchangeComboMdWindows();
}


int CMemDumpView::GetDumpPageListPos()
{
	return m_cbWndList.GetCurSel();
}

int CMemDumpView::GetDumpAddrListPos()
{
	return m_cbWndAddr.GetCurSel();
}

void CMemDumpView::OnCbnSelchangeComboMdWindows()
{
	int n = m_cbWndList.GetCurSel();

	if (n <= 0)
	{
		m_ListMemory.m_nDmpWndAddr = 0;
		m_ListMemory.m_nDmpWndLen = 65536;
		m_pDmpEAPtr = nullptr;
		m_cbWndAddr.EnableWindow(FALSE);
	}
	else
	{
		--n;

		if (m_pV->at(n).nDmpWndAddr != -1)
		{
			m_cbWndAddr.EnableWindow(FALSE);
			m_ListMemory.m_nDmpWndAddr = m_pV->at(n).nDmpWndAddr;
		}
		else
		{
			m_cbWndAddr.EnableWindow(TRUE);
			m_ListMemory.m_nDmpWndAddr = m_cbWndAddr.GetCurSel() * 040000;
		}

		m_ListMemory.m_nDmpWndLen = m_pV->at(n).nDmpWndLen;
		m_pDmpEAPtr = m_pV->at(n).pDmpEAPtr;
	}

	RefreshDump(m_pDmpEAPtr); //нужно, чтоб передать данные списку
}


void CMemDumpView::OnCbnSelchangeComboMdWindaddr()
{
	m_ListMemory.m_nDmpWndAddr = m_cbWndAddr.GetCurSel() * 040000;
	RefreshDump(m_pDmpEAPtr); //нужно, чтоб передать данные списку
}

LRESULT CMemDumpView::OnDumpAddressChange(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);
	uint16_t addr = Global::OctStringToWord(m_strDumpAddr);
	// обновим значение, чтобы оно было всегда 6 значным
	m_strDumpAddr = Global::WordToOctString(addr);
	UpdateData(FALSE);
	m_ListMemory.SetAddress(addr);
	m_ListMemory.RefreshItems(addr, true);
	return S_OK;
}


void CMemDumpView::OnBnClickedButtonDumpMode()
{
	if (++m_nCurrentDDM >= DISPLAY_MODES_NUM)
	{
		m_nCurrentDDM = 0;
	}

	SetDlgItemText(IDC_BUTTON_MD_DUMPMODE, CString(MAKEINTRESOURCE(m_dmArray[m_nCurrentDDM].strID)));
	m_ListMemory.SetDisplayMode(m_dmArray[m_nCurrentDDM].mode);
}

void CMemDumpView::RefreshDump(uint8_t *pDmpEAPtr)
{
	m_pDmpEAPtr = pDmpEAPtr;
	m_ListMemory.m_nDmpWndEndAddr = m_ListMemory.m_nDmpWndAddr + m_ListMemory.m_nDmpWndLen;
	m_ListMemory.SetAddress(m_ListMemory.m_nDmpWndAddr);
	m_ListMemory.SetDisplayMode(m_dmArray[m_nCurrentDDM].mode); // перерисуем всё
}

uint16_t CMemDumpView::GetDebugMemDumpWord(uint16_t dumpAddress)
{
	if (m_pDebugger)
	{
		if (m_pDmpEAPtr)
		{
			return *(reinterpret_cast<uint16_t *>(m_pDmpEAPtr + (dumpAddress - m_ListMemory.m_nDmpWndAddr)));
		}

		return m_pDebugger->GetDebugMemDumpWord(dumpAddress);
	}

	return 0;
}

uint8_t CMemDumpView::GetDebugMemDumpByte(uint16_t dumpAddress)
{
	if (m_pDebugger)
	{
		if (m_pDmpEAPtr)
		{
			return *(m_pDmpEAPtr + (dumpAddress - m_ListMemory.m_nDmpWndAddr));
		}

		return m_pDebugger->GetDebugMemDumpByte(dumpAddress);
	}

	return 0;
}

void CMemDumpView::SetDebugMemDumpWord(uint16_t dumpAddress, uint16_t value)
{
	if (m_pDebugger)
	{
		if (m_pDmpEAPtr)
		{
			*(reinterpret_cast<uint16_t *>(m_pDmpEAPtr + (dumpAddress - m_ListMemory.m_nDmpWndAddr))) = value;
		}
		else
		{
			m_pDebugger->SetDebugMemDump(dumpAddress, value);
		}
	}
}

void CMemDumpView::SetDebugMemDumpByte(uint16_t dumpAddress, uint8_t value)
{
	if (m_pDebugger)
	{
		if (m_pDmpEAPtr)
		{
			*(m_pDmpEAPtr + (dumpAddress - m_ListMemory.m_nDmpWndAddr)) = value;
		}
		else
		{
			m_pDebugger->SetDebugMemDump(dumpAddress, value);
		}
	}
}

#pragma warning(disable:4996)

void CMemDumpView::OnBnClickedButtonSaveDump()
{
	SvDmpParam sBinParam;
	GetDlgItem(IDC_BUTTON_MD_SAVE)->GetWindowRect(&sBinParam.rect);
	auto SvDmpDlg = std::make_unique<CSvDmpParamDlg>(&sBinParam);

	if (SvDmpDlg)
	{
		if (SvDmpDlg->DoModal() == IDOK)
		{
			int nAddr = sBinParam.nBgnAddr;
			int nLen = sBinParam.nLength;

			if (nAddr + nLen >= m_ListMemory.m_nDmpWndLen)
			{
				nLen = m_ListMemory.m_nDmpWndLen - nAddr;
			}

			if (nLen > 0)
			{
				CString strDefExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
				CString strFilter(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
				CFileDialog dlg(FALSE, strDefExt, nullptr,
				                OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
				                strFilter, this);
				dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str(); // диалог всегда будем начинать с директории для bin файлов

				if (dlg.DoModal() == IDOK)
				{
					auto buf = std::vector<uint8_t>(nLen);

					if (buf.data())
					{
						fs::path str = dlg.GetPathName().GetString();

						if (m_pDebugger)
						{
							auto pBoard = m_pDebugger->GetBoard();

							if (pBoard)
							{
								pBoard->StopCPU();
								//pBoard->SetMemPages(sBinParam.nPage0, sBinParam.nPage1);
							}
						}

						for (int addr = nAddr, i = 0; i < nLen; ++i)
						{
							buf[i] = GetDebugMemDumpByte(addr++);
						}

						Global::SaveBinFile(buf.data(), nAddr, nLen, str);

						if (m_pDebugger)
						{
							auto pBoard = m_pDebugger->GetBoard();

							if (pBoard)
							{
								//pBoard->RestoreMemPages();
								pBoard->RunCPU();
							}
						}
					}
					else
					{
						g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
					}
				}
			}
		}
	}
}


void CMemDumpView::OnBnClickedButtonLoadDump()
{
	if (!!(::GetAsyncKeyState(VK_SHIFT) & 0x8000))
	{
		SvDmpParam sBinParam;
		GetDlgItem(IDC_BUTTON_MD_LOAD)->GetWindowRect(&sBinParam.rect);
		auto LdDmpDlg = std::make_unique<CLdDmpParamDlg>(&sBinParam);

		if (LdDmpDlg)
		{
			if (LdDmpDlg->DoModal() == IDOK)
			{
				// если БК11(М) - подключить нужные страницы
				if (m_pDebugger)
				{
					auto pBoard = m_pDebugger->GetBoard();

					if (pBoard)
					{
						pBoard->StopCPU();
						pBoard->SetMemPages(sBinParam.nPage0, sBinParam.nPage1);
					}
				}

				LoadFunc(sBinParam.nBgnAddr);

				// если БК11(М) - отключить нужные страницы и вернуть как было
				if (m_pDebugger)
				{
					auto pBoard = m_pDebugger->GetBoard();

					if (pBoard)
					{
						pBoard->RestoreMemPages();
						pBoard->RunCPU();
					}
				}
			}
		}
	}
	else
	{
		LoadFunc();
	}
}

void CMemDumpView::OnUpdateButton(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}

void CMemDumpView::LoadFunc(uint16_t nLoadAddr)
{
	CString strDefExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
	CString strFilter(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
	CFileDialog dlg(TRUE, strDefExt, nullptr,
	                OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilter, this);
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str(); // диалог всегда будем начинать с директории для bin файлов

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = dlg.GetPathName().GetString();
		uint16_t nAddr, nLen;
		std::unique_ptr<uint8_t[]> buf;

		if (Global::LoadBinFile(buf, nAddr, nLen, str, false))
		{
			// !!! Косяк - невозможно загружать принудительно с нулевого адреса.
			// Тут бы очень помогли отрицательные числа, но их ввод в поле адреса не предусмотрен,
			// как быть, пока идей нет.
			if (nLoadAddr)
			{
				nAddr = nLoadAddr;
			}

			if (int(nAddr) + int(nLen) >= m_ListMemory.m_nDmpWndLen)
			{
				nLen = m_ListMemory.m_nDmpWndLen - nAddr;
			}

			for (int i = 0; i < nLen; ++i)
			{
				SetDebugMemDumpByte(nAddr++, buf[i]);
			}
		}
	}
}


IMPLEMENT_DYNAMIC(CSvDmpParamDlg, CBaseDialog)

CSvDmpParamDlg::CSvDmpParamDlg(SvDmpParam *pParam, CWnd *pParent /*= nullptr*/)
	: CBaseDialog(CSvDmpParamDlg::IDD, pParent)
	, m_pParam(pParam)
	, m_nBgnAddr(0)
	, m_nEndAddr(040000)
	, m_nLength(m_nEndAddr - m_nBgnAddr)
{
	m_strBgnAddr = Global::WordToOctString(m_nBgnAddr);
	m_strEndAddr = Global::WordToOctString(m_nEndAddr);
	m_strLength = Global::WordToOctString(m_nLength);
}

CSvDmpParamDlg::~CSvDmpParamDlg()
    = default;

BEGIN_MESSAGE_MAP(CSvDmpParamDlg, CBaseDialog)
	ON_MESSAGE(WM_DBG_TOP_ADDRESS_UPDATE, &CSvDmpParamDlg::OnEditChange)
	ON_EN_KILLFOCUS(IDC_EDIT_SVDMP_BGNADDR, &CSvDmpParamDlg::OnEnKillfocusEditSvdmpBgnaddr)
	ON_EN_KILLFOCUS(IDC_EDIT_SVDMP_ENDADDR, &CSvDmpParamDlg::OnEnKillfocusEditSvdmpEndaddr)
	ON_EN_KILLFOCUS(IDC_EDIT_SVDMP_LENGTH, &CSvDmpParamDlg::OnEnKillfocusEditSvdmpLength)
END_MESSAGE_MAP()

void CSvDmpParamDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_SVDMP_BGNADDR, m_EditBgnAddr);
	DDX_Control(pDX, IDC_EDIT_SVDMP_ENDADDR, m_EditEndAddr);
	DDX_Control(pDX, IDC_EDIT_SVDMP_LENGTH, m_EditLength);
	DDX_Text(pDX, IDC_EDIT_SVDMP_BGNADDR, m_strBgnAddr);
	DDX_Text(pDX, IDC_EDIT_SVDMP_ENDADDR, m_strEndAddr);
	DDX_Text(pDX, IDC_EDIT_SVDMP_LENGTH, m_strLength);
}

BOOL CSvDmpParamDlg::OnInitDialog()
{
	CRect r2;
	GetWindowRect(r2);
	CRect r;
	r.left = m_pParam->rect.left;
	r.top = m_pParam->rect.top + m_pParam->rect.Height();
	r.right = r.left + r2.Width();
	r.bottom = r.top + r2.Height();
//  r2.OffsetRect(m_pParam->rect.left - r2.left, m_pParam->rect.top + m_pParam->rect.Height() - r2.top);
	MoveWindow(r, FALSE);
	CBaseDialog::OnInitDialog();
	m_EditBgnAddr.SetLimitText(6);
	m_EditEndAddr.SetLimitText(6);
	m_EditLength.SetLimitText(6);
	UpdateData(FALSE);
	return TRUE;
}

void CSvDmpParamDlg::OnOK()
{
	if (m_pParam)
	{
		m_pParam->nBgnAddr = m_nBgnAddr;
		m_pParam->nLength = m_nLength;
	}

	CBaseDialog::OnOK();
}


LRESULT CSvDmpParamDlg::OnEditChange(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);

	switch (wParam)
	{
		case IDC_EDIT_SVDMP_BGNADDR:
			m_nBgnAddr = Global::OctStringToWord(m_strBgnAddr);

			if (m_nBgnAddr > 65536)
			{
				m_nBgnAddr = 0;
			}

			if (m_nBgnAddr > m_nEndAddr)
			{
				std::swap(m_nBgnAddr, m_nEndAddr);
			}

			m_nLength = m_nEndAddr - m_nBgnAddr;
			break;

		case IDC_EDIT_SVDMP_ENDADDR:
			m_nEndAddr = Global::OctStringToWord(m_strEndAddr);

			if (m_nEndAddr > 65536)
			{
				m_nEndAddr = 0;
			}

			if (m_nBgnAddr > m_nEndAddr)
			{
				std::swap(m_nBgnAddr, m_nEndAddr);
			}

			m_nLength = m_nEndAddr - m_nBgnAddr;
			break;

		case IDC_EDIT_SVDMP_LENGTH:
			m_nLength = Global::OctStringToWord(m_strLength);

			if (m_nLength > 65536)
			{
				m_nLength = 65536;
			}

			m_nEndAddr = m_nBgnAddr + m_nLength;

			if (m_nEndAddr > 65536)
			{
				m_nEndAddr = 65536;
				m_nLength = m_nEndAddr - m_nBgnAddr;
			}

			break;

		default:
			return LRESULT();
	}

	m_strBgnAddr = Global::WordToOctString(m_nBgnAddr);
	m_strEndAddr = Global::WordToOctString(m_nEndAddr);
	m_strLength = Global::WordToOctString(m_nLength);
	UpdateData(FALSE);
	return LRESULT();
}


void CSvDmpParamDlg::OnEnKillfocusEditSvdmpBgnaddr()
{
	OnEditChange(IDC_EDIT_SVDMP_BGNADDR, 0);
}


void CSvDmpParamDlg::OnEnKillfocusEditSvdmpEndaddr()
{
	OnEditChange(IDC_EDIT_SVDMP_ENDADDR, 0);
}


void CSvDmpParamDlg::OnEnKillfocusEditSvdmpLength()
{
	OnEditChange(IDC_EDIT_SVDMP_LENGTH, 0);
}


IMPLEMENT_DYNAMIC(CLdDmpParamDlg, CBaseDialog)

CLdDmpParamDlg::CLdDmpParamDlg(SvDmpParam *pParam /*= nullptr*/, CWnd *pParent /*= nullptr*/)
	: CBaseDialog(CLdDmpParamDlg::IDD, pParent)
	, m_pParam(pParam)
	, m_nBgnAddr(0)
	, m_nPg0(5)
	, m_nPg1(1)
{
	m_strBgnAddr = Global::WordToOctString(m_nBgnAddr);
}

CLdDmpParamDlg::~CLdDmpParamDlg()
    = default;

BEGIN_MESSAGE_MAP(CLdDmpParamDlg, CBaseDialog)
	ON_MESSAGE(WM_DBG_TOP_ADDRESS_UPDATE, &CLdDmpParamDlg::OnEditChange)
	ON_EN_KILLFOCUS(IDC_EDIT_LDDMP_ADDR, &CLdDmpParamDlg::OnEnKillfocusEditLddmpAddr)
END_MESSAGE_MAP()

void CLdDmpParamDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_LDDMP_ADDR, m_EditBgnAddr);
	DDX_Text(pDX, IDC_EDIT_LDDMP_ADDR, m_strBgnAddr);
	DDX_Control(pDX, IDC_EDIT_LDDMP_PG0, m_EditPG0); // IDC_EDIT_LDDMP_PG0 - номер страницы 1 ( на адрес 040000 )
	DDX_Control(pDX, IDC_EDIT_LDDMP_PG1, m_EditPG1);// IDC_EDIT_LDDMP_PG1 - страница 2 ( на адрес 100000 )
	DDX_Text(pDX, IDC_EDIT_LDDMP_PG0, m_nPg0);
	DDX_Text(pDX, IDC_EDIT_LDDMP_PG1, m_nPg1);
}

BOOL CLdDmpParamDlg::OnInitDialog()
{
	CRect r2;
	GetWindowRect(r2);
	CRect r;
	r.left = m_pParam->rect.left;
	r.top = m_pParam->rect.top + m_pParam->rect.Height();
	r.right = r.left + r2.Width();
	r.bottom = r.top + r2.Height();
	//  r2.OffsetRect(m_pParam->rect.left - r2.left, m_pParam->rect.top + m_pParam->rect.Height() - r2.top);
	MoveWindow(r, FALSE);
	CBaseDialog::OnInitDialog();
	m_EditBgnAddr.SetLimitText(6);
	m_EditPG0.SetLimitText(1);
	m_EditPG1.SetLimitText(1);

	// Если сейчас не БК11М и её конфигурации, засерим контролы выбора страниц.
	// В других местах - не юзаем результаты в контролах и не сохраняем
	if (!(g_Config.isBK11() || g_Config.isBK11M()))
	{
		GetDlgItem(IDC_EDIT_LDDMP_PG0)->EnableWindow(FALSE);
		GetDlgItem(IDC_EDIT_LDDMP_PG1)->EnableWindow(FALSE);
		GetDlgItem(IDC_TEXT_LDDMP_PG0)->EnableWindow(FALSE);
		GetDlgItem(IDC_TEXT_LDDMP_PG1)->EnableWindow(FALSE);
	}

	UpdateData(FALSE);
	return TRUE;
}

void CLdDmpParamDlg::OnOK()
{
	UpdateData(TRUE);

	if (m_pParam)
	{
		m_nBgnAddr = Global::OctStringToWord(m_strBgnAddr);
		m_pParam->nBgnAddr = m_nBgnAddr;
		m_pParam->nPage0 = m_nPg0;
		m_pParam->nPage1 = m_nPg1;
	}

	CBaseDialog::OnOK();
}


LRESULT CLdDmpParamDlg::OnEditChange(WPARAM wParam, LPARAM lParam)
{
	UpdateData(TRUE);

	switch (wParam)
	{
		case IDC_EDIT_LDDMP_ADDR:
			m_nBgnAddr = Global::OctStringToWord(m_strBgnAddr);

			if (m_nBgnAddr > 65536)
			{
				m_nBgnAddr = 0;
			}

			break;

		default:
			return LRESULT();
	}

	m_strBgnAddr = Global::WordToOctString(m_nBgnAddr);
	UpdateData(FALSE);
	return LRESULT();
}

void CLdDmpParamDlg::OnEnKillfocusEditLddmpAddr()
{
	OnEditChange(IDC_EDIT_LDDMP_ADDR, 0);
}

#endif