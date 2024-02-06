// BKSettPage_1.cpp: файл реализации
//
#ifdef UI
#include "pch.h"

#include "BKSettPage_1.h"

#include "LoadImgDlg.h"
#include "BKMessageBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Диалоговое окно BKSettDlg_1
IMPLEMENT_DYNAMIC(BKSettDlg_1, CBaseDialog)

BKSettDlg_1::BKSettDlg_1(CWnd *pParent /*=nullptr*/)
	: CBaseDialog(IDD_SETTINGS_1, pParent)
	, m_nCurrCPUFrequency(0)
{}

BKSettDlg_1::~BKSettDlg_1()
    = default;

BEGIN_MESSAGE_MAP(BKSettDlg_1, CBaseDialog)
	ON_BN_CLICKED(IDC_BUTTON_SETT_SELHDD0, &BKSettDlg_1::OnBnClickedSelect0)
	ON_BN_CLICKED(IDC_BUTTON_SETT_SELHDD1, &BKSettDlg_1::OnBnClickedSelect1)
	ON_BN_CLICKED(IDC_BUTTON_SETT_EXCHGHDD, &BKSettDlg_1::OnBnClickedExchange)
	ON_BN_CLICKED(IDC_BUTTON_SETT_CLEARHDD0, &BKSettDlg_1::OnBnClickedButtonClearHdd0)
	ON_BN_CLICKED(IDC_BUTTON_SETT_CLEARHDD1, &BKSettDlg_1::OnBnClickedButtonClearHdd1)
	ON_BN_CLICKED(IDC_BUTTON_SETT_CLEARFDDA, &BKSettDlg_1::OnBnClickedButtonClearFddA)
	ON_BN_CLICKED(IDC_BUTTON_SETT_CLEARFDDB, &BKSettDlg_1::OnBnClickedButtonClearFddB)
	ON_BN_CLICKED(IDC_BUTTON_SETT_CLEARFDDC, &BKSettDlg_1::OnBnClickedButtonClearFddC)
	ON_BN_CLICKED(IDC_BUTTON_SETT_CLEARFDDD, &BKSettDlg_1::OnBnClickedButtonClearFddD)
	ON_BN_CLICKED(IDC_BUTTON_SETT_SELFDDA, &BKSettDlg_1::OnBnClickedButtonSelectFddA)
	ON_BN_CLICKED(IDC_BUTTON_SETT_SELFDDB, &BKSettDlg_1::OnBnClickedButtonSelectFddB)
	ON_BN_CLICKED(IDC_BUTTON_SETT_SELFDDC, &BKSettDlg_1::OnBnClickedButtonSelectFddC)
	ON_BN_CLICKED(IDC_BUTTON_SETT_SELFDDD, &BKSettDlg_1::OnBnClickedButtonSelectFddD)
	ON_BN_CLICKED(IDC_BUTTON_SETT_NEWFDDA, &BKSettDlg_1::OnBnClickedButtonSettNewFddA)
	ON_BN_CLICKED(IDC_BUTTON_SETT_NEWFDDB, &BKSettDlg_1::OnBnClickedButtonSettNewFddB)
	ON_BN_CLICKED(IDC_BUTTON_SETT_NEWFDDC, &BKSettDlg_1::OnBnClickedButtonSettNewFddC)
	ON_BN_CLICKED(IDC_BUTTON_SETT_NEWFDDD, &BKSettDlg_1::OnBnClickedButtonSettNewFddD)
	ON_MESSAGE(WM_SETT_SENDTOTAB, &BKSettDlg_1::OnSendToTab)
END_MESSAGE_MAP()

void BKSettDlg_1::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_SETT_DUMPADDR, m_arStrCurrDumpAddr[0]);
	DDX_Text(pDX, IDC_EDIT_SETT_DISASMADDR, m_strCurrDisasmAddr);
	DDX_Text(pDX, IDC_EDIT_SETT_CPUSTARTADDR, m_strCurrCPURunAddr);
	DDX_Text(pDX, IDC_EDIT_SETT_CPUFREQ, m_nCurrCPUFrequency);
}

// Обработчики сообщений BKSettDlg_1
BOOL BKSettDlg_1::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	// HDD Images
	GetFileImage(HDD_MODE::MASTER);
	GetFileImage(HDD_MODE::SLAVE);
	// FDD Images
	GetFileImage(FDD_DRIVE::A);
	GetFileImage(FDD_DRIVE::B);
	GetFileImage(FDD_DRIVE::C);
	GetFileImage(FDD_DRIVE::D);
	reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SETT_CPUSTARTADDR))->SetLimitText(6);
	reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SETT_DISASMADDR))->SetLimitText(6);
	reinterpret_cast<CEdit *>(GetDlgItem(IDC_EDIT_SETT_DUMPADDR))->SetLimitText(6);
	// Адрес запуска процессора
	m_strCurrCPURunAddr = Global::WordToOctString(g_Config.m_nCPURunAddr);
	// Частота работы процессора
	m_nCurrCPUFrequency = g_Config.m_nCPUFrequency;

	// Dump Adr
	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		m_arStrCurrDumpAddr[i] = Global::WordToOctString(g_Config.m_arDumper[i].nAddr);
	}

	// Disasm Adr
	m_strCurrDisasmAddr = Global::WordToOctString(g_Config.m_nDisasmAddr);
	UpdateData(FALSE);
	return TRUE;
}



void BKSettDlg_1::OnBnClickedSelect0()
{
	LoadFileImage(HDD_MODE::MASTER);
}

void BKSettDlg_1::OnBnClickedSelect1()
{
	LoadFileImage(HDD_MODE::SLAVE);
}


void BKSettDlg_1::OnBnClickedButtonSelectFddA()
{
	LoadFileImage(FDD_DRIVE::A);
}

void BKSettDlg_1::OnBnClickedButtonSelectFddB()
{
	LoadFileImage(FDD_DRIVE::B);
}

void BKSettDlg_1::OnBnClickedButtonSelectFddC()
{
	LoadFileImage(FDD_DRIVE::C);
}

void BKSettDlg_1::OnBnClickedButtonSelectFddD()
{
	LoadFileImage(FDD_DRIVE::D);
}


void BKSettDlg_1::OnBnClickedButtonClearHdd0()
{
	ClearImgPath(HDD_MODE::MASTER);
}

void BKSettDlg_1::OnBnClickedButtonClearHdd1()
{
	ClearImgPath(HDD_MODE::SLAVE);
}


void BKSettDlg_1::OnBnClickedButtonClearFddA()
{
	ClearImgPath(FDD_DRIVE::A);
}

void BKSettDlg_1::OnBnClickedButtonClearFddB()
{
	ClearImgPath(FDD_DRIVE::B);
}

void BKSettDlg_1::OnBnClickedButtonClearFddC()
{
	ClearImgPath(FDD_DRIVE::C);
}

void BKSettDlg_1::OnBnClickedButtonClearFddD()
{
	ClearImgPath(FDD_DRIVE::D);
}


// Тут мы ловим сообщение главного окна, мол "Сохраняйся, сваливаем"
// если было изменение - возвращается не 0, если не было - 0
LRESULT BKSettDlg_1::OnSendToTab(WPARAM, LPARAM)
{
	return LRESULT(Save());
}

void BKSettDlg_1::OnBnClickedExchange()
{
	DrvNmInfo &masterHDD = m_aHDD[static_cast<int>(HDD_MODE::MASTER)];
	DrvNmInfo &slaveHDD = m_aHDD[static_cast<int>(HDD_MODE::SLAVE)];
	std::swap(masterHDD, slaveHDD);
	std::swap(masterHDD.nDlgID, slaveHDD.nDlgID); // а это назад надо
	masterHDD.Operation = slaveHDD.Operation = DrvNmSetOPerate_t::OP_EXCHANGE;
	CString strHDDImgPath = Global::isEmptyUnit(masterHDD.New) ? masterHDD.Old.c_str() : masterHDD.New.c_str();
	SetDlgItemText(masterHDD.nDlgID, strHDDImgPath);
	strHDDImgPath = Global::isEmptyUnit(slaveHDD.New) ? slaveHDD.Old.c_str() : slaveHDD.New.c_str();
	SetDlgItemText(slaveHDD.nDlgID, strHDDImgPath);
}


DWORD BKSettDlg_1::Save()
{
	UpdateData(TRUE);
	DWORD nChanges = NO_CHANGES;
	// Сохраним остальные исправленные или нет параметры
	nChanges |= SaveDrvImgName(FDD_DRIVE::A);
	nChanges |= SaveDrvImgName(FDD_DRIVE::B);
	nChanges |= SaveDrvImgName(FDD_DRIVE::C);
	nChanges |= SaveDrvImgName(FDD_DRIVE::D);
	nChanges |= SaveDrvImgName(HDD_MODE::MASTER);
	nChanges |= SaveDrvImgName(HDD_MODE::SLAVE);
	// Адрес запуска процессора
	UINT c = Global::OctStringToWord(m_strCurrCPURunAddr);

	if (g_Config.m_nCPURunAddr != c)
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_nCPURunAddr = c;
	}

	// Частота работы процессора
	if (g_Config.m_nCPUFrequency != m_nCurrCPUFrequency)
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_nCPUFrequency = m_nCurrCPUFrequency;
	}

	// Dump Adr
	for (int i = 0; i < NUMBER_VIEWS_MEM_DUMP; ++i)
	{
		c = Global::OctStringToWord(m_arStrCurrDumpAddr[i]);

		if (g_Config.m_arDumper[i].nAddr != c)
		{
			nChanges |= CHANGES_NOREBOOT;
			g_Config.m_arDumper[i].nAddr = c;
		}
	}

	// Disasm Adr
	c = Global::OctStringToWord(m_strCurrDisasmAddr);

	if (g_Config.m_nDisasmAddr != c)
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_nDisasmAddr = c;
	}

	return nChanges;
}

void BKSettDlg_1::OnOK()
{
	// блокируем клавишу Enter
}


void BKSettDlg_1::OnCancel()
{
	// блокируем клавишу Esc
}

void BKSettDlg_1::LoadFileImage(const HDD_MODE eDrive)
{
	const CString strFilterIMG(MAKEINTRESOURCE(IDS_FILEFILTER_BKHDDIMG));
	const int nDrive = g_Config.GetDriveNum(eDrive);
	LoadFileImage_1(strFilterIMG, m_aHDD[nDrive]);
}

void BKSettDlg_1::LoadFileImage(const FDD_DRIVE eDrive)
{
	const CString strFilterIMG(MAKEINTRESOURCE(IDS_FILEFILTER_BKIMG));
	const int nDrive = g_Config.GetDriveNum(eDrive);
	LoadFileImage_1(strFilterIMG, m_aFDD[nDrive]);
}

void BKSettDlg_1::LoadFileImage_1(const CString &strFilterIMG, DrvNmInfo &di)
{
	CLoadImgDlg dlg(true, nullptr, nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterIMG, nullptr);
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strIMGPath.c_str(); // диалог всегда будем начинать с домашней директории образов.

	if (dlg.DoModal() == IDOK)
	{
		di.New = g_Config.GetShortDriveImgName(dlg.GetPathName().GetString());
		di.Operation = DrvNmSetOPerate_t::OP_REPLACE;
		SetDlgItemText(di.nDlgID, di.New.c_str());
	}
}


void BKSettDlg_1::ClearImgPath(const HDD_MODE eDrive)
{
	const int nDrive = g_Config.GetDriveNum(eDrive);
	ClearImgPath_1(m_aHDD[nDrive]);
}

void BKSettDlg_1::ClearImgPath(const FDD_DRIVE eDrive)
{
	const int nDrive = g_Config.GetDriveNum(eDrive);
	ClearImgPath_1(m_aFDD[nDrive]);
}

void BKSettDlg_1::ClearImgPath_1(DrvNmInfo &di)
{
	di.New = g_strEmptyUnit.GetString();
	di.Operation = DrvNmSetOPerate_t::OP_REPLACE;
	SetDlgItemText(di.nDlgID, di.New.c_str());
}


void BKSettDlg_1::GetFileImage(const HDD_MODE eDrive)
{
	const fs::path strImgName = g_Config.GetShortDriveImgName(eDrive);
	const int nDrive = g_Config.GetDriveNum(eDrive);
	RenewFileImage_1(strImgName, m_aHDD[nDrive]);
}

void BKSettDlg_1::GetFileImage(const FDD_DRIVE eDrive)
{
	const fs::path strImgName = g_Config.GetShortDriveImgName(eDrive);
	const int nDrive = g_Config.GetDriveNum(eDrive);
	RenewFileImage_1(strImgName, m_aFDD[nDrive]);
}


void BKSettDlg_1::RenewFileImage_1(const fs::path &strImgName, DrvNmInfo &di)
{
	di.Old = strImgName;
	di.Operation = DrvNmSetOPerate_t::OP_NONE;
	SetDlgItemText(di.nDlgID, strImgName.c_str());
}

// возвращет: CHANGES_NEEDREBOOT - если было изменение,
//            NO_CHANGES - если не было
DWORD BKSettDlg_1::SaveDrvImgName(const HDD_MODE eDrive)
{
	DWORD nRet = NO_CHANGES;
	const int nDrive = g_Config.GetDriveNum(eDrive);
	DrvNmInfo &di = m_aHDD[nDrive];

	if (di.Operation != DrvNmSetOPerate_t::OP_NONE)
	{
		fs::path strImgPath;

		if (di.Operation == DrvNmSetOPerate_t::OP_EXCHANGE)    // Обмен
		{
			strImgPath = !Global::isEmptyUnit(di.New) ? di.New : di.Old;
		}
		else if (di.Operation == DrvNmSetOPerate_t::OP_REPLACE)    // заменили файл
		{
			strImgPath = di.New;
		}

		g_Config.SetDriveImgName(eDrive, strImgPath);
		di.Operation = DrvNmSetOPerate_t::OP_NONE;
		nRet |= CHANGES_NEEDREBOOT;
	}

	return nRet;
}

DWORD BKSettDlg_1::SaveDrvImgName(const FDD_DRIVE eDrive)
{
	DWORD nRet = NO_CHANGES;
	const int nDrive = g_Config.GetDriveNum(eDrive);
	DrvNmInfo &di = m_aFDD[nDrive];

	if (di.Operation != DrvNmSetOPerate_t::OP_NONE)
	{
		fs::path strImgPath;

		if (di.Operation == DrvNmSetOPerate_t::OP_EXCHANGE)    // Обмен
		{
			strImgPath = !Global::isEmptyUnit(di.New) ? di.New : di.Old;
		}
		else if (di.Operation == DrvNmSetOPerate_t::OP_REPLACE)    // заменили файл
		{
			strImgPath = di.New;
		}

		g_Config.SetDriveImgName(eDrive, strImgPath);
		di.Operation = DrvNmSetOPerate_t::OP_NONE;
		nRet |= CHANGES_NEEDREBOOT;
	}

	return nRet;
}


#pragma warning(disable:4996)
void BKSettDlg_1::MakeNewEmptyImage(const FDD_DRIVE eDrive)
{
	const CString strFilterIMG(MAKEINTRESOURCE(IDS_FILEFILTER_BKIMG));
	CFileDialog dlg(FALSE, _T(".img"), nullptr,
					OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
					strFilterIMG, nullptr);
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strIMGPath.c_str(); // диалог всегда будем начинать с домашней директории образов.
	dlg.GetOFN().lpstrTitle = _T("Создать пустой файл образа...");

	if (dlg.DoModal() == IDOK)
	{
		const fs::path newFile = dlg.GetPathName().GetString();
		FILE *f = _tfopen(newFile.c_str(), _T("wb"));
		if (f)
		{
			std::vector v(512, 0);
			for (int i = 0; i < (10 * 80 * 2); ++i)
			{
				fwrite(v.data(), 512, 1, f);
			}
			fclose(f);

			const int nDrive = g_Config.GetDriveNum(eDrive);
			DrvNmInfo &di = m_aFDD[nDrive];
			di.New = g_Config.GetShortDriveImgName(newFile);
			di.Operation = DrvNmSetOPerate_t::OP_REPLACE;
			SetDlgItemText(di.nDlgID, di.New.c_str());
		}
		else
		{
			g_BKMsgBox.Show(_T("Невозможно создать файл образа."), MB_OK|MB_ICONEXCLAMATION);
		}
	}
}

void BKSettDlg_1::OnBnClickedButtonSettNewFddA()
{
	MakeNewEmptyImage(FDD_DRIVE::A);
}


void BKSettDlg_1::OnBnClickedButtonSettNewFddB()
{
	MakeNewEmptyImage(FDD_DRIVE::B);
}


void BKSettDlg_1::OnBnClickedButtonSettNewFddC()
{
	MakeNewEmptyImage(FDD_DRIVE::C);
}


void BKSettDlg_1::OnBnClickedButtonSettNewFddD()
{
	MakeNewEmptyImage(FDD_DRIVE::D);
}
#endif