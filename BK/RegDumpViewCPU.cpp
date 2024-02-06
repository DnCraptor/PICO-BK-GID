#include "pch.h"
#ifdef UI
#include "RegDumpViewCPU.h"
#include "Board.h"
#include "Screen_Sizes.h"
#include "Debugger.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif


CUSTOMVIEW_MODE_PARAM CRegDumpViewCPU::m_cvArray[CUSTOMVIEW_REGS_NUM] =
{
	{ CV_DEC_VIEW, IDS_REGDUMPCPU_DEC },
	{ CV_HEX_VIEW, IDS_REGDUMPCPU_HEX }
};

constexpr auto COLUMN_WIDTH = 60;
constexpr auto COLUMN_WIDTH_PRT = 78;
constexpr auto COLUMN_WIDTH_REG = 38;
constexpr auto COLUMN_WIDTH_TXT = 70;

const UINT CRegDumpViewCPU::m_pListCpuIDs[LISTCPU_L::CPU_L_NUM] =
{
	IDS_MEMORY_R0, IDS_MEMORY_R1, IDS_MEMORY_R2, IDS_MEMORY_R3,
	IDS_MEMORY_R4, IDS_MEMORY_R5, IDS_MEMORY_SP, IDS_MEMORY_PC,
	IDS_MEMORY_PSW
};

const UINT CRegDumpViewCPU::m_pListSysIDs[2][LISTSYS_L::SYS_L_NUM] =
{
	{
		IDS_MEMORY_177660, IDS_MEMORY_177662IN, IDS_MEMORY_177662OUT, IDS_MEMORY_177664,
		IDS_MEMORY_177714IN, IDS_MEMORY_177714OUT, IDS_MEMORY_177716IN, IDS_MEMORY_177716OUT_TAP,
		IDS_MEMORY_177716OUT_MEM
	},
	{
		IDS_MEMORY_177700, IDS_MEMORY_177702, IDS_MEMORY_177704, IDS_MEMORY_177706,
		IDS_MEMORY_177710, IDS_MEMORY_177712, 0, 0, 0
	}
};

const UINT CRegDumpViewCPU::m_pListSysRegs[2][LISTSYS_L::SYS_L_NUM] =
{
	{
		SYS_PORT_177660, SYS_PORT_177662_IN, SYS_PORT_177662_OUT, SYS_PORT_177664,
		SYS_PORT_177714_IN, SYS_PORT_177714_OUT, SYS_PORT_177716_IN, SYS_PORT_177716_OUT_TAP,
		SYS_PORT_177716_OUT_MEM
	},
	{
		SYS_PORT_177700, SYS_PORT_177702, SYS_PORT_177704, SYS_PORT_177706,
		SYS_PORT_177710, SYS_PORT_177712, 0, 0, 0
	}
};


const UINT CRegDumpViewCPU::m_pListAltProIDs[LISTALTPRO_L::AP_L_NUM] =
{
	IDS_ALTPRO_MODE, IDS_ALTPRO_CODE
};



/////////////////////////////////////////////////////////////////////////////
// CRegDumpViewCPU
IMPLEMENT_DYNAMIC(CRegDumpViewCPU, CDocPaneDlgViewBase)

CRegDumpViewCPU::CRegDumpViewCPU()
	: m_pDebugger(nullptr)
	, m_nCurrCPUFreq(0)
	, m_nCurrentCVM(CV_DEC_VIEW)
{
}

CRegDumpViewCPU::~CRegDumpViewCPU()
    = default;

void CRegDumpViewCPU::DoDataExchange(CDataExchange *pDX)
{
	CDocPaneDlgViewBase::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_RDC_CPUREGS, m_listCPU);
	DDX_Control(pDX, IDC_LIST_RDC_SYSREGS, m_listSys);
	DDX_Control(pDX, IDC_LIST_RDC_ALTPRO_DATA, m_listAltPro);
	DDX_Text(pDX, IDC_EDIT_RDC_CPUFREQ, m_nCurrCPUFreq);
	DDX_Control(pDX, IDC_SPIN_RDC_CPUFREQ, m_spinCPUFreq);
}

BEGIN_MESSAGE_MAP(CRegDumpViewCPU, CDocPaneDlgViewBase)
	ON_MESSAGE(WM_INITDIALOG, &CRegDumpViewCPU::HandleInitDialog)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RDC_CPUREGS, &CRegDumpViewCPU::OnLvnItemchangedMdCpuRegisters)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RDC_SYSREGS, &CRegDumpViewCPU::OnLvnItemchangedMdSysRegisters)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_RDC_ALTPRO_DATA, &CRegDumpViewCPU::OnLvnItemchangedMdAltProData)
	ON_EN_CHANGE(IDC_EDIT_RDC_CPUFREQ, &CRegDumpViewCPU::OnEnChangeEditDbg1Cpufreq)
	ON_COMMAND(IDC_BUTTON_RDC_CPUFREQ_DEC, &CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqDec)
	ON_COMMAND(IDC_BUTTON_RDC_CPUFREQ_INC, &CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqInc)
	ON_COMMAND(IDC_BUTTON_RDC_CPUFREQ_BASESET, &CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqBaseset)
	ON_COMMAND(IDC_BUTTON_RDC_CPUFREQ_MAXSET, &CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqMaxset)
	ON_COMMAND(IDC_BUTTON_RDC_CVMODE, &CRegDumpViewCPU::OnBnClickedRegdumpCvmode)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_RDC_CPUFREQ_DEC, &CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqDec)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_RDC_CPUFREQ_INC, &CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqInc)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_RDC_CPUFREQ_BASESET, &CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqBaseset)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_RDC_CPUFREQ_MAXSET, &CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqMaxset)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_RDC_CVMODE, &CRegDumpViewCPU::OnUpdateRegdumpCvmode)

END_MESSAGE_MAP()

// Вход: list - список, grp - элемент "Группа"
// height - количество строк в списке
// Вход-Выход:  ptLT - координата верхней левой точки списка, если 0 - то берутся текущие
//              maxGW - максимальная ширина группы
void CRegDumpViewCPU::ResizeList(CMultiEditListCtrl *list, CWnd *grp, int height, CPoint &ptLT, int &maxGW)
{
	CRect listRect, grpRect, itemRect;
	list->GetWindowRect(&listRect);     // текущие размеры списка
	ScreenToClient(&listRect);

	if (ptLT.x)
	{
		listRect.left = ptLT.x;
	}

	if (ptLT.y)
	{
		listRect.top = ptLT.y;
	}

	grp->GetWindowRect(&grpRect);   // текущие размеры группы
	ScreenToClient(&grpRect);
	list->GetItemRect(0, itemRect, LVIR_BOUNDS); // получим размеры элемента строки списка
	// и на их основе расчитаем новые размеры списка, чтобы был точно по размерам элементов
	int cx = itemRect.Width() + ::GetSystemMetrics(SM_CXDLGFRAME);
	int cy = itemRect.Height() * height + ::GetSystemMetrics(SM_CYDLGFRAME);
	list->SetWindowPos(nullptr, listRect.left, listRect.top, cx, cy, SWP_SHOWWINDOW); // изменяем размеры списка
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	int padX = ::MulDiv(4, nPixelW, DEFAULT_DPIX); // отступ группы от списка слева и справа
	int padY = ::MulDiv(5, nPixelH, DEFAULT_DPIY); // отступ группы от списка сверху и снизу
	grpRect.left = listRect.left - padX;
	grpRect.top = listRect.top - ::GetSystemMetrics(SM_CYCAPTION) / 2 - padY;
	grpRect.right = listRect.left + cx + padX;
	grpRect.bottom = listRect.top + cy + padY;
	grp->SetWindowPos(nullptr, grpRect.left, grpRect.top, grpRect.Width(), grpRect.Height(), SWP_SHOWWINDOW); // изменяем размеры группы

	if (maxGW < grpRect.Width())
	{
		maxGW = grpRect.Width();
	}

	ptLT.x = listRect.left;
	ptLT.y = grpRect.bottom + ::GetSystemMetrics(SM_CYCAPTION) / 2 + padY;
}


void CRegDumpViewCPU::SetGrpWidth(CWnd *grp, int width)
{
	CRect grpRect;
	grp->GetWindowRect(&grpRect);   // текущие размеры группы
	ScreenToClient(&grpRect);
	grp->SetWindowPos(nullptr, grpRect.left, grpRect.top, width, grpRect.Height(), SWP_SHOWWINDOW); // изменяем размеры группы
}

void CRegDumpViewCPU::AttachDebugger(CDebugger *pDbgr)
{
	m_pDebugger = pDbgr;
}


/////////////////////////////////////////////////////////////////////////////
// обработчики сообщений CRegDumpViewCPU
LRESULT CRegDumpViewCPU::HandleInitDialog(WPARAM wp, LPARAM lp)
{
	CDocPaneDlgViewBase::HandleInitDialog(wp, lp);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_DEC), ID_CPU_SLOWDOWN);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_INC), ID_CPU_ACCELERATE);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_BASESET), ID_CPU_NORMALSPEED);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_RDC_CVMODE), IDS_TOOLTIP_BUTTON_RDC_CVMODE);
	m_ToolTip.AddTool(GetDlgItem(IDC_EDIT_RDC_CPUFREQ), IDS_TOOLTIP_EDIT_RDC_CPUFREQ);
	m_ToolTip.AddTool(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_MAXSET), ID_CPU_MAXSPEED);
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	// Подготовим ленту с иконками и имаджлист
	CBitmap m_bmparr;
	m_bmparr.LoadBitmap(IDR_MAINFRAME_256);
	CImageList m_imagelist;
	m_imagelist.Create(16, 15, ILC_COLORDDB | ILC_MASK, 0, 0);
	// Наложим ленту на имаджлист, учитывая фон/прозрачность (в нашем случае какой-то серый)
	m_imagelist.Add(&m_bmparr, RGB(192, 192, 192));
	// Вытаскиваем на кнопки по одной иконке из имаджлиста
	(reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_DEC)))->SetIcon(m_imagelist.ExtractIcon(7));
	(reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_INC)))->SetIcon(m_imagelist.ExtractIcon(5));
	(reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_BASESET)))->SetIcon(m_imagelist.ExtractIcon(8));
	(reinterpret_cast<CButton *>(GetDlgItem(IDC_BUTTON_RDC_CPUFREQ_MAXSET)))->SetIcon(m_imagelist.ExtractIcon(6));
	// Уничтожим ненужное более
	m_imagelist.DeleteImageList();
	m_bmparr.DeleteObject();
	// Создадим список регистров CPU
	// 3 колонки, 1-я - имя регистра, 2-я - его значение, 3-я - кастомное значение
	m_listCPU.SetFont(&m_hFont, FALSE);
	m_listCPU.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	m_listCPU.AcceptDigits(false); // принимаем не только цифры
	m_listCPU.InsertColumn(LISTCPU_C::COL_NAME, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH_REG, nPixelW, DEFAULT_DPIX));
	m_listCPU.InsertColumn(LISTCPU_C::COL_VALUE, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	m_listCPU.InsertColumn(LISTCPU_C::COL_CUSTOM, LPSTR_TEXTCALLBACK, LVCFMT_RIGHT, ::MulDiv(COLUMN_WIDTH_TXT, nPixelW, DEFAULT_DPIX));

	for (int i = LISTCPU_L::LINE_R0; i <= LISTCPU_L::LINE_PSW; ++i)
	{
		m_listCPU.InsertItem(i, CString(MAKEINTRESOURCE(m_pListCpuIDs[i])));
		m_listCPU.SetItem(i, LISTCPU_C::COL_VALUE, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
		m_listCPU.SetItem(i, LISTCPU_C::COL_CUSTOM, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
	}

	m_listCPU.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LISTCPU_C::COL_NAME);
	m_listCPU.EnableColumnEdit(false, LISTCPU_C::COL_NAME);
	// Создадим список портов и системных регистров
	// 4 колонки, 1-я и 3-я - адрес порта/регистра, 2-я и 4-я - их значения
	m_listSys.SetFont(&m_hFont, FALSE);
	m_listSys.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	m_listSys.InsertColumn(LISTSYS_C::COL_NAME1, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH_PRT, nPixelW, DEFAULT_DPIX));
	m_listSys.InsertColumn(LISTSYS_C::COL_VALUE1, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	m_listSys.InsertColumn(LISTSYS_C::COL_NAME2, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));
	m_listSys.InsertColumn(LISTSYS_C::COL_VALUE2, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));

	for (int i = LISTSYS_L::LINE_REG177660; i <= LISTSYS_L::LINE_REG177716OUT_MEM; ++i)
	{
		m_listSys.InsertItem(i, CString(MAKEINTRESOURCE(m_pListSysIDs[0][i])));
		m_listSys.SetItem(i, LISTSYS_C::COL_VALUE1, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
		CString strHeader;

		if (m_pListSysIDs[1][i])
		{
			strHeader = CString(MAKEINTRESOURCE(m_pListSysIDs[1][i]));
		}
		else
		{
			strHeader.Empty();
		}

		m_listSys.SetItem(i, LISTSYS_C::COL_NAME2, LVIF_TEXT, strHeader, 0, 0, 0, 0);
		m_listSys.SetItem(i, LISTSYS_C::COL_VALUE2, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
	}

	m_listSys.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LISTSYS_C::COL_NAME1);
	m_listSys.EnableColumnEdit(false, LISTSYS_C::COL_NAME1);
	m_listSys.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LISTSYS_C::COL_NAME2);
	m_listSys.EnableColumnEdit(false, LISTSYS_C::COL_NAME2);

	// ещё надо запретить редактировать свободные ячейки в 3-м столбце
	for (int i = LISTSYS_L2::LINE_REG177712 + 1; i <= LISTSYS_L::LINE_REG177716OUT_MEM; ++i)
	{
		m_listSys.EnableEdit(false, i, LISTSYS_C::COL_VALUE2);
	}

	// создание списка режимов AltPro
	m_listAltPro.SetFont(&m_hFont, FALSE);
	m_listAltPro.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER | LVS_EX_SNAPTOGRID);
	m_listAltPro.InsertColumn(LISTALTPRO_C::COL_NAMEA, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH_PRT, nPixelW, DEFAULT_DPIX));
	m_listAltPro.InsertColumn(LISTALTPRO_C::COL_VALUEA, LPSTR_TEXTCALLBACK, LVCFMT_LEFT, ::MulDiv(COLUMN_WIDTH, nPixelW, DEFAULT_DPIX));

	for (int i = LISTALTPRO_L::LINE_MODE; i <= LISTALTPRO_L::LINE_CODE; ++i)
	{
		m_listAltPro.InsertItem(i, CString(MAKEINTRESOURCE(m_pListAltProIDs[i])));
		m_listAltPro.SetItem(i, LISTALTPRO_C::COL_VALUEA, LVIF_TEXT, LPSTR_TEXTCALLBACK, 0, 0, 0, 0);
	}

	m_listAltPro.SetColumnBkColor(::GetSysColor(COLOR_BTNFACE), LISTALTPRO_C::COL_NAMEA);
	m_listAltPro.EnableColumnEdit(false, LISTALTPRO_C::COL_NAMEA);
	int nMaxGrpWidth = 0;
	CPoint ptLT{ 0, 0 };
	// Оказалось, что в Windows XP и Windows 7 пользовательский интерфейс выглядит по разному,
	// и размеры списков различаются. В одной винде больше, в другой - меньше. Поэтому диалоги
	// со списками выглядят криво или там, или там, нельзя сделать, чтобы везде было одинаково красиво.
	// Из за этого их приходится динамически подгонять по размерам одного элемента списка.
	// Ну и заодно и расставлять в окне диалога красиво.
	ResizeList(&m_listCPU, GetDlgItem(IDC_STATIC_RDC_CPUREGTITLE), LISTCPU_L::LINE_PSW + 1, ptLT, nMaxGrpWidth);
	ResizeList(&m_listSys, GetDlgItem(IDC_STATIC_RDC_SYSREGTITLE), LISTSYS_L::LINE_REG177716OUT_MEM + 1, ptLT, nMaxGrpWidth);
	ResizeList(&m_listAltPro, GetDlgItem(IDC_STATIC_RDC_SMKREGTITLE), LISTALTPRO_L::LINE_CODE + 1, ptLT, nMaxGrpWidth);
	// теперь сделаем группы все одной ширины
	SetGrpWidth(GetDlgItem(IDC_STATIC_RDC_SMKREGTITLE), nMaxGrpWidth);
	SetGrpWidth(GetDlgItem(IDC_STATIC_RDC_SYSREGTITLE), nMaxGrpWidth);
	SetGrpWidth(GetDlgItem(IDC_STATIC_RDC_CPUREGTITLE), nMaxGrpWidth);
	// и пододвинем кнопку на место
	CRect rect, btnRect;
	m_listCPU.GetWindowRect(&rect);     // текущие размеры списка
	ScreenToClient(&rect);
	GetDlgItem(IDC_BUTTON_RDC_CVMODE)->GetWindowRect(btnRect);
	rect.left = rect.right + ::MulDiv(4, nPixelW, DEFAULT_DPIX);
	GetDlgItem(IDC_BUTTON_RDC_CVMODE)->SetWindowPos(nullptr, rect.left, rect.top, btnRect.Width(), btnRect.Height(), SWP_SHOWWINDOW);
	SetDlgItemText(IDC_BUTTON_RDC_CVMODE, CString(MAKEINTRESOURCE(m_cvArray[m_nCurrentCVM].strID)));
	//и под конец изменим размеры самого окна
	GetWindowRect(&rect);
	ScreenToClient(&rect);
	rect.bottom = ptLT.y - ::GetSystemMetrics(SM_CYCAPTION) / 2;
	rect.right = ptLT.x + nMaxGrpWidth;
	m_sizeDefault = rect.Size();
	return TRUE; // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

CString CRegDumpViewCPU::FormatPSW(uint16_t value)
{
	//                   012345678
	CString strPSW = _T("--------");
	TCHAR *pStr = strPSW.GetBuffer();

	if (value & (1 << static_cast<int>(PSW_BIT::HALT)))
	{
		pStr[0] = _T('H');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::MASKI)))
	{
		pStr[1] = _T('M');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::P)))
	{
		pStr[2] = _T('P');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::T)))
	{
		pStr[3] = _T('T');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::N)))
	{
		pStr[4] = _T('N');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::Z)))
	{
		pStr[5] = _T('Z');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::V)))
	{
		pStr[6] = _T('V');
	}

	if (value & (1 << static_cast<int>(PSW_BIT::C)))
	{
		pStr[7] = _T('C');
	}

	return strPSW;
}


uint16_t CRegDumpViewCPU::GetPSW(CString &strPSW)
{
	uint16_t psw = 0;
	strPSW.Trim();
	strPSW.MakeUpper();
	int nStrLen = strPSW.GetLength();
	const TCHAR *pStr = strPSW.GetString();

	// все символы могут быть любыми, но воспринимаются только те, что нужны.
	// причём порядок следования не важен, количество символов не важно
	for (int i = 0; i < nStrLen; ++i)
	{
		switch (pStr[i])
		{
			case _T('C'):
				psw |= (1 << static_cast<int>(PSW_BIT::C));
				break;

			case _T('V'):
				psw |= (1 << static_cast<int>(PSW_BIT::V));
				break;

			case _T('Z'):
				psw |= (1 << static_cast<int>(PSW_BIT::Z));
				break;

			case _T('N'):
				psw |= (1 << static_cast<int>(PSW_BIT::N));
				break;

			case _T('T'):
				psw |= (1 << static_cast<int>(PSW_BIT::T));
				break;

			case _T('P'):
				psw |= (1 << static_cast<int>(PSW_BIT::P));
				break;

			case _T('M'):
				psw |= (1 << static_cast<int>(PSW_BIT::MASKI));
				break;

			case _T('H'):
				psw |= (1 << static_cast<int>(PSW_BIT::HALT));
				break;
		}
	}

	return psw;
}

void CRegDumpViewCPU::DisplayPortRegs()
{
	if (m_pDebugger)
	{
		for (int i = LISTSYS_L::LINE_REG177660; i <= LISTSYS_L::LINE_REG177716OUT_MEM; ++i)
		{
			m_listSys.SetItemWithModified(m_pDebugger->GetPortValue(m_pListSysRegs[0][i]), i, LISTSYS_C::COL_VALUE1);
		}

		for (int i = LISTSYS_L2::LINE_REG177700; i <= LISTSYS_L2::LINE_REG177712; ++i)
		{
			m_listSys.SetItemWithModified(m_pDebugger->GetPortValue(m_pListSysRegs[1][i]), i, LISTSYS_C::COL_VALUE2);
		}
	}
}

void CRegDumpViewCPU::DisplayRegisters()
{
	if (m_pDebugger)
	{
		for (int i = LISTCPU_L::LINE_R0; i <= LISTCPU_L::LINE_PC; ++i)
		{
			uint16_t val = m_pDebugger->GetRegister(static_cast<CCPU::REGISTER>(i));
			// выводим первую колонку
			m_listCPU.SetItemWithModified(val, i, LISTCPU_C::COL_VALUE);
			// вторую колонку
			CString str = WordToCustom(val);
			m_listCPU.SetItemWithModifiedASCII(str, i, LISTCPU_C::COL_CUSTOM);
		}

		m_listCPU.SetItemWithModified(m_pDebugger->GetRegister(static_cast<CCPU::REGISTER>(LISTCPU_L::LINE_PSW)), LISTCPU_L::LINE_PSW, LISTCPU_C::COL_VALUE);
		SetPSW(m_pDebugger->GetBoard()->GetPSW());
	}
}

void CRegDumpViewCPU::SetPSW(uint16_t value)
{
	CString str = FormatPSW(value);
	m_listCPU.SetItemWithModifiedASCII(str, LISTCPU_L::LINE_PSW, LISTCPU_C::COL_CUSTOM);
}

void CRegDumpViewCPU::DisplayAltProData()
{
	if (m_pDebugger)
	{
		m_listAltPro.SetItemWithModified(m_pDebugger->GetAltProData(LISTALTPRO_L::LINE_MODE), LISTALTPRO_L::LINE_MODE, LISTALTPRO_C::COL_VALUEA);
		m_listAltPro.SetItemWithModified(m_pDebugger->GetAltProData(LISTALTPRO_L::LINE_CODE), LISTALTPRO_L::LINE_CODE, LISTALTPRO_C::COL_VALUEA);
	}
}


void CRegDumpViewCPU::OnLvnItemchangedMdCpuRegisters(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = S_OK;
	int nSubItem = pNMListView->iSubItem;
	int pos = pNMListView->iItem;
	auto reg = static_cast<CCPU::REGISTER>(pos);
	ASSERT(reg <= CCPU::REGISTER::PSW);
	uint16_t value;

	if (nSubItem == LISTCPU_C::COL_VALUE)
	{
		auto pStr = reinterpret_cast<CString *>(pNMListView->lParam);
		value = Global::OctStringToWord(*pStr);
	}
	else if (nSubItem == LISTCPU_C::COL_CUSTOM)
	{
		auto pStr = reinterpret_cast<CString *>(pNMListView->lParam);
		value = (reg == CCPU::REGISTER::PSW) ? GetPSW(*pStr) : CustomToWord(*pStr);
	}
	else
	{
		return; // nSubItem может быть и другое значение
	}

	if (m_pDebugger)
	{
		m_pDebugger->SetDebugRegs(reg, value);
	}

	if (reg == CCPU::REGISTER::PSW)
	{
		m_listCPU.SetItemWithModified(value, LISTCPU_L::LINE_PSW, LISTCPU_C::COL_VALUE);
		SetPSW(value);
	}
	else
	{
		m_listCPU.SetItemWithModified(value, pos, LISTCPU_C::COL_VALUE);
		CString str = WordToCustom(value);
		m_listCPU.SetItemWithModifiedASCII(str, pos, LISTCPU_C::COL_CUSTOM);
	}
}

CString CRegDumpViewCPU::WordToCustom(uint16_t value)
{
	CString str;

	switch (m_nCurrentCVM)
	{
		case CV_DEC_VIEW:
			str.Format(_T("%05d"), value);
			break;

		case CV_HEX_VIEW:
			str.Format(_T("0x%04x"), value);
			break;
	}

	return str;
}

uint16_t CRegDumpViewCPU::CustomToWord(const CString &str)
{
	uint16_t res = 0;

	switch (m_nCurrentCVM)
	{
		case CV_DEC_VIEW:
			res = _ttoi(str);
			break;

		case CV_HEX_VIEW:
			res = uint16_t(_tcstol(str, nullptr, 16));
			break;
	}

	return res;
}

void CRegDumpViewCPU::OnLvnItemchangedMdAltProData(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nSubItem = pNMListView->iSubItem;

	if (nSubItem == LISTALTPRO_L::LINE_CODE)
	{
		int pos = pNMListView->iItem;
		auto pStr = reinterpret_cast<CString *>(pNMListView->lParam);
		uint16_t value = Global::OctStringToWord(*pStr);

		if (m_pDebugger)
		{
			m_pDebugger->SetDebugAltProData(pos, value);
		}

		m_listAltPro.SetItemWithModified(value, pos, nSubItem);
		// Тут надо принудительно обновить содержимое дампера
		GetParentFrame()->PostMessage(WM_MEMDUMP_NEED_UPDATE);
	}

	*pResult = S_OK;
}


void CRegDumpViewCPU::OnLvnItemchangedMdSysRegisters(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = S_OK;
	int pos = -1;
	int nSubItem = pNMListView->iSubItem;
	int nitem = pNMListView->iItem;

	switch (nSubItem)
	{
		case LISTSYS_C::COL_VALUE1:
			ASSERT(nitem <= LISTSYS_L::LINE_REG177716OUT_MEM);
			pos = m_pListSysRegs[0][nitem];
			break;

		case LISTSYS_C::COL_VALUE2:
			ASSERT(nitem <= LISTSYS_L2::LINE_REG177712);
			pos = m_pListSysRegs[1][nitem];
			break;

		default:
			return;
	}

	auto pStr = reinterpret_cast<CString *>(pNMListView->lParam);
	uint16_t value = Global::OctStringToWord(*pStr);

	if (m_pDebugger)
	{
		m_pDebugger->SetDebugPorts(pos, value);
	}

	m_listSys.SetItemWithModified(value, nitem, nSubItem);
}


void CRegDumpViewCPU::OnEnChangeEditDbg1Cpufreq()
{
	UpdateData(TRUE);
	CMotherBoard *pBoard = m_pDebugger->GetBoard();
	ASSERT(pBoard);

	if (pBoard)
	{
		pBoard->SetCPUFreq(m_nCurrCPUFreq);
	}

	GetParentFrame()->PostMessage(WM_SETCPUFREQ);
}

void CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqDec()
{
	GetParentFrame()->PostMessage(WM_COMMAND, ID_CPU_SLOWDOWN);
}

void CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqInc()
{
	GetParentFrame()->PostMessage(WM_COMMAND, ID_CPU_ACCELERATE);
}

void CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqBaseset()
{
	GetParentFrame()->PostMessage(WM_COMMAND, ID_CPU_NORMALSPEED);
}

void CRegDumpViewCPU::OnBnClickedButtonDbg1CpufreqMaxset()
{
	GetParentFrame()->PostMessage(WM_COMMAND, ID_CPU_MAXSPEED);
}

void CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqDec(CCmdUI *pCmdUI)
{
	CMotherBoard *pBoard = m_pDebugger->GetBoard();
	bool bOn = (pBoard) ? pBoard->CanSlowDown() : false;
	pCmdUI->Enable(bOn);
}

void CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqInc(CCmdUI *pCmdUI)
{
	CMotherBoard *pBoard = m_pDebugger->GetBoard();
	bool bOn = (pBoard) ? pBoard->CanAccelerate() : false;
	pCmdUI->Enable(bOn);
}

void CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqMaxset(CCmdUI *pCmdUI)
{
	bool bChk = (g_Config.m_nCPUTempFreq >= 0);
	pCmdUI->SetCheck(bChk);
}

void CRegDumpViewCPU::OnUpdateButtonDbg1CpufreqBaseset(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}

void CRegDumpViewCPU::OnUpdateRegdumpCvmode(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(true);
}

// отобразим текущую частоту
void CRegDumpViewCPU::SetFreqParam()
{
	CMotherBoard *pBoard = m_pDebugger->GetBoard();
	ASSERT(pBoard);

	if (pBoard)
	{
		m_nCurrCPUFreq = pBoard->GetCPUFreq();
		m_spinCPUFreq.SetRange32(pBoard->GetLowBound(), pBoard->GetHighBound());
		m_spinCPUFreq.SetPos32(m_nCurrCPUFreq);
		m_spinCPUFreq.SetBuddy(GetDlgItem(IDC_EDIT_RDC_CPUFREQ));
	}

	UpdateData(FALSE);
}

void CRegDumpViewCPU::UpdateFreq()
{
	CMotherBoard *pBoard = m_pDebugger->GetBoard();
	ASSERT(pBoard);

	if (pBoard)
	{
		m_nCurrCPUFreq = pBoard->GetCPUFreq();
	}

	UpdateData(FALSE);
}

void CRegDumpViewCPU::DisplayRegDump()
{
	if (IsWindowVisible())
	{
		DisplayRegisters();
		DisplayPortRegs();
		DisplayAltProData();
		// выводим на экран данные FDD
	}
}

void CRegDumpViewCPU::OnBnClickedRegdumpCvmode()
{
	if (++m_nCurrentCVM >= CUSTOMVIEW_REGS_NUM)
	{
		m_nCurrentCVM = 0;
	}

	SetDlgItemText(IDC_BUTTON_RDC_CVMODE, CString(MAKEINTRESOURCE(m_cvArray[m_nCurrentCVM].strID)));
	CMotherBoard *pBoard = m_pDebugger->GetBoard();
	ASSERT(pBoard);

	// если мы в отладочном останове, то
	if (pBoard && pBoard->IsCPUBreaked())
	{
		DisplayRegisters(); // надо вручную обновить регистры
	}
}

#endif