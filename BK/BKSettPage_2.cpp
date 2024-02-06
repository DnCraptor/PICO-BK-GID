// BKSettPage_2.cpp: файл реализации
//
#ifdef UI
#include "pch.h"

#include "BKSettPage_2.h"

#include "Config.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Bool_Items BKSettDlg_2::arOptionItems[OPTIONS_ARRAYSIZE] =
{
	// itemID,                          id,                                     value
	// ИД контрола,                     глобальная переменная,                  текущее значение
	  { IDC_CHECK_SETT_COLORMODE,       &g_Config.m_bColorMode,                 FALSE }
	, { IDC_CHECK_SETT_ADAPTBLKWHT,     &g_Config.m_bAdaptBWMode,               FALSE }
	, { IDC_CHECK_SETT_FULLSCREEN,      &g_Config.m_bFullscreenMode,            FALSE }
	, { IDC_CHECK_SETT_LUMINOFOREMODE,  &g_Config.m_bLuminoforeEmulMode,        FALSE }
	, { IDC_CHECK_SETT_SMOOTHING,       &g_Config.m_bSmoothing,                 FALSE }

	, { IDC_CHECK_SETT_EMUSPEAKER,      &g_Config.m_bSpeaker,                   FALSE }
	, { IDC_CHECK_SETT_FILTSPEAKER,     &g_Config.m_bSpeakerFilter,             FALSE }
	, { IDC_CHECK_SETT_SPEAKDCOFFS,     &g_Config.m_bSpeakerDCOffset,           FALSE }
	, { IDC_CHECK_SETT_EMUCOVOX,        &g_Config.m_bCovox,                     FALSE }
	, { IDC_CHECK_SETT_FILTCOVOX,       &g_Config.m_bCovoxFilter,               FALSE }
	, { IDC_CHECK_SETT_COVOXDCOFFS,     &g_Config.m_bCovoxDCOffset,             FALSE }
	, { IDC_CHECK_SETT_STEREOCOVOX,     &g_Config.m_bStereoCovox,               FALSE }
	, { IDC_CHECK_SETT_EMUMENESTREL,    &g_Config.m_bMenestrel,                 FALSE }
	, { IDC_CHECK_SETT_FILTMENESTREL,   &g_Config.m_bMenestrelFilter,           FALSE }
	, { IDC_CHECK_SETT_MENESDCOFFS,     &g_Config.m_bMenestrelDCOffset,         FALSE }
	, { IDC_CHECK_SETT_EMUAY,           &g_Config.m_bAY8910,                    FALSE }
	, { IDC_CHECK_SETT_FILTAY,          &g_Config.m_bAY8910Filter,              FALSE }
	, { IDC_CHECK_SETT_AY8910DCOFFS,    &g_Config.m_bAY8910DCOffset,            FALSE }

	, { IDC_CHECK_SETT_BKKEYBOARD,      &g_Config.m_bBKKeyboard,                FALSE }
	, { IDC_CHECK_SETT_JCUKENKBD,       &g_Config.m_bJCUKENKbd ,                FALSE }
	, { IDC_CHECK_SETT_NATIVERUSLATSWITCH, &g_Config.m_bNativeRusLatSwitch,     FALSE }
	, { IDC_CHECK_SETT_JOYSTICK,        &g_Config.m_bJoystick,                  FALSE }
	, { IDC_CHECK_SETT_EMUMOUSE,        &g_Config.m_bMouseMars,                 FALSE }
	, { IDC_CHECK_SETT_ICLBLOCK,        &g_Config.m_bICLBlock,                  FALSE }

	, { IDC_CHECK_SETT_EMULOADTAPE,     &g_Config.m_bEmulateLoadTape,           FALSE }
	, { IDC_CHECK_SETT_EMUSAVETAPE,     &g_Config.m_bEmulateSaveTape,           FALSE }
	, { IDC_CHECK_SETT_AUTOBGNTAPE,     &g_Config.m_bTapeAutoBeginDetection,    FALSE }
	, { IDC_CHECK_SETT_AUTOENDTAPE,     &g_Config.m_bTapeAutoEndDetection,      FALSE }
	, { IDC_CHECK_SETT_EMUFDDIO,        &g_Config.m_bEmulateFDDIO,              FALSE }
	, { IDC_CHECK_SETT_LONGBIN,         &g_Config.m_bUseLongBinFormat,          FALSE }

	, { IDC_CHECK_SETT_PAUSECPU,        &g_Config.m_bPauseCPUAfterStart,        FALSE }
	, { IDC_CHECK_SETT_ASKFORBREAK,     &g_Config.m_bAskForBreak,               FALSE }
	, { IDC_CHECK_SETT_SHOWPERFSTR,     &g_Config.m_bShowPerformance,           FALSE }
	, { IDC_CHECK_SETT_SAVESDEFAULT,    &g_Config.m_bSavesDefault,              FALSE }
};

enum OPT_IDX : int
{
	SETT_COLORMODE,
	SETT_ADAPTBLKWHT,
	SETT_FULLSCREEN,
	SETT_LUMINOFOREMODE,
	SETT_SMOOTHING,

	SETT_EMUSPEAKER,
	SETT_FILTSPEAKER,
	SETT_SPEAKDCOFFS,
	SETT_EMUCOVOX,
	SETT_FILTCOVOX,
	SETT_COVOXDCOFFS,
	SETT_STEREOCOVOX,
	SETT_EMUMENESTREL,
	SETT_FILTMENESTREL,
	SETT_MENESDCOFFS,
	SETT_EMUAY,
	SETT_FILTAY,
	SETT_AY8910DCOFFS,

	SETT_BKKEYBOARD,
	SETT_JCUKENKBD,
	SETT_NATIVERUSLATSWITCH,
	SETT_JOYSTICK,
	SETT_EMUMOUSE,
	SETT_ICLBLOCK,
	SETT_EMULOADTAPE,
	SETT_EMUSAVETAPE,
	SETT_AUTOBGNTAPE,
	SETT_AUTOENDTAPE,
	SETT_EMUFDDIO,
	SETT_LONGBIN,
	SETT_PAUSECPU,
	SETT_ASKFORBREAK,
	SETT_SHOWPERFSTR,
	SETT_SAVESDEFAULT
};


// Диалоговое окно BKSettDlg_2

IMPLEMENT_DYNAMIC(BKSettDlg_2, CBaseDialog)

BKSettDlg_2::BKSettDlg_2(CWnd *pParent /*=nullptr*/)
	: CBaseDialog(IDD_SETTINGS_2, pParent)
	, m_nVKBDVw(0)
	, m_nRegDumpIntrv(0)
{}

BKSettDlg_2 ::~BKSettDlg_2()
    = default;


BEGIN_MESSAGE_MAP(BKSettDlg_2, CBaseDialog)
	ON_MESSAGE(WM_SETT_SENDTOTAB, &BKSettDlg_2::OnSendToTab)
	ON_BN_CLICKED(IDC_CHECK_SETT_EMUCOVOX, &BKSettDlg_2::OnBnClickedCheckSettEmucovox)
	ON_BN_CLICKED(IDC_CHECK_SETT_EMUAY, &BKSettDlg_2::OnBnClickedCheckSettEmuay)
	ON_BN_CLICKED(IDC_CHECK_SETT_JOYSTICK, &BKSettDlg_2::OnBnClickedCheckSettJoystick)
	ON_BN_CLICKED(IDC_CHECK_SETT_EMUMOUSE, &BKSettDlg_2::OnBnClickedCheckSettEmumouse)
	ON_BN_CLICKED(IDC_CHECK_SETT_ICLBLOCK, &BKSettDlg_2::OnBnClickedCheckSettIclblock)
	ON_BN_CLICKED(IDC_CHECK_SETT_EMUMENESTREL, &BKSettDlg_2::OnBnClickedCheckSettEmumenestrel)
END_MESSAGE_MAP()

void BKSettDlg_2::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_SETT_COLORMODE,        arOptionItems[OPT_IDX::SETT_COLORMODE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_ADAPTBLKWHT,      arOptionItems[OPT_IDX::SETT_ADAPTBLKWHT].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_FULLSCREEN,       arOptionItems[OPT_IDX::SETT_FULLSCREEN].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_LUMINOFOREMODE,   arOptionItems[OPT_IDX::SETT_LUMINOFOREMODE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_SMOOTHING,        arOptionItems[OPT_IDX::SETT_SMOOTHING].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUSPEAKER,       arOptionItems[OPT_IDX::SETT_EMUSPEAKER].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_FILTSPEAKER,      arOptionItems[OPT_IDX::SETT_FILTSPEAKER].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_SPEAKDCOFFS,      arOptionItems[OPT_IDX::SETT_SPEAKDCOFFS].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUCOVOX,         arOptionItems[OPT_IDX::SETT_EMUCOVOX].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_FILTCOVOX,        arOptionItems[OPT_IDX::SETT_FILTCOVOX].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_COVOXDCOFFS,      arOptionItems[OPT_IDX::SETT_COVOXDCOFFS].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_STEREOCOVOX,      arOptionItems[OPT_IDX::SETT_STEREOCOVOX].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUMENESTREL,     arOptionItems[OPT_IDX::SETT_EMUMENESTREL].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_FILTMENESTREL,    arOptionItems[OPT_IDX::SETT_FILTMENESTREL].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_MENESDCOFFS,      arOptionItems[OPT_IDX::SETT_MENESDCOFFS].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUAY,            arOptionItems[OPT_IDX::SETT_EMUAY].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_FILTAY,           arOptionItems[OPT_IDX::SETT_FILTAY].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_AY8910DCOFFS,     arOptionItems[OPT_IDX::SETT_AY8910DCOFFS].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_BKKEYBOARD,       arOptionItems[OPT_IDX::SETT_BKKEYBOARD].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_JCUKENKBD,        arOptionItems[OPT_IDX::SETT_JCUKENKBD].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_NATIVERUSLATSWITCH, arOptionItems[OPT_IDX::SETT_NATIVERUSLATSWITCH].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_JOYSTICK,         arOptionItems[OPT_IDX::SETT_JOYSTICK].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUMOUSE,         arOptionItems[OPT_IDX::SETT_EMUMOUSE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_ICLBLOCK,         arOptionItems[OPT_IDX::SETT_ICLBLOCK].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMULOADTAPE,      arOptionItems[OPT_IDX::SETT_EMULOADTAPE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUSAVETAPE,      arOptionItems[OPT_IDX::SETT_EMUSAVETAPE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_AUTOBGNTAPE,      arOptionItems[OPT_IDX::SETT_AUTOBGNTAPE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_AUTOENDTAPE,      arOptionItems[OPT_IDX::SETT_AUTOENDTAPE].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_EMUFDDIO,         arOptionItems[OPT_IDX::SETT_EMUFDDIO].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_LONGBIN,          arOptionItems[OPT_IDX::SETT_LONGBIN].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_PAUSECPU,         arOptionItems[OPT_IDX::SETT_PAUSECPU].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_ASKFORBREAK,      arOptionItems[OPT_IDX::SETT_ASKFORBREAK].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_SHOWPERFSTR,      arOptionItems[OPT_IDX::SETT_SHOWPERFSTR].bValue);
	DDX_Check(pDX, IDC_CHECK_SETT_SAVESDEFAULT,     arOptionItems[OPT_IDX::SETT_SAVESDEFAULT].bValue);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_VIRTKBDVW, m_nVKBDVw);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_REGSDUMPINT, m_nRegDumpIntrv);
}

static const int RegDumpInt[11] =
{
	0, 1, 2, 3, 4, 5, 10, 15, 20, 25, 50
};

// Обработчики сообщений BKSettDlg_2
BOOL BKSettDlg_2::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	//читаем из конфига значения опций
	for (auto &item : arOptionItems)
	{
		item.bValue = *item.id;
	}

	auto ctrlVKBDVw = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_VIRTKBDVW));
	ctrlVKBDVw->AddString(CString(MAKEINTRESOURCE(IDS_SETT_KBDVIEW0)).GetString());
	ctrlVKBDVw->AddString(CString(MAKEINTRESOURCE(IDS_SETT_KBDVIEW1)).GetString());
	m_nVKBDVw = g_Config.m_nVKBDType;
	auto ctrlRegDumpInt = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_REGSDUMPINT));

	for (int i = 0; i < 11; ++i)
	{
		if (RegDumpInt[i] == g_Config.m_nRegistersDumpInterval)
		{
			m_nRegDumpIntrv = i;
		}

		CString str = Global::IntToString(RegDumpInt[i]);
		ctrlRegDumpInt->AddString(str);
	}

	UpdateData(FALSE);
	return TRUE;
}

// Тут мы ловим сообщение главного окна, мол "Сохраняйся, сваливаем"
LRESULT BKSettDlg_2::OnSendToTab(WPARAM, LPARAM)
{
	return LRESULT(Save());
}

DWORD BKSettDlg_2::Save()
{
	DWORD nChanges = NO_CHANGES;
	UpdateData(TRUE);

	for (auto &item : arOptionItems)
	{
		bool b = !!item.bValue;

		if (b != *item.id)
		{
			nChanges |= CHANGES_NOREBOOT;
			*item.id = b;
		}
	}

	if (g_Config.m_nVKBDType != m_nVKBDVw)  // Если что-то поменялось
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_nVKBDType = m_nVKBDVw;
	}

	if (g_Config.m_nRegistersDumpInterval != RegDumpInt[m_nRegDumpIntrv])
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_nRegistersDumpInterval = RegDumpInt[m_nRegDumpIntrv];
	}

	return nChanges;
}

void BKSettDlg_2::OnBnClickedCheckSettEmucovox()
{
	UpdateData(TRUE);

	if (arOptionItems[OPT_IDX::SETT_EMUCOVOX].bValue)
	{
		arOptionItems[OPT_IDX::SETT_EMUMENESTREL].bValue = FALSE;
		arOptionItems[OPT_IDX::SETT_EMUAY].bValue = FALSE;
	}

	UpdateData(FALSE);
}


void BKSettDlg_2::OnBnClickedCheckSettEmumenestrel()
{
	UpdateData(TRUE);

	if (arOptionItems[OPT_IDX::SETT_EMUMENESTREL].bValue)
	{
		arOptionItems[OPT_IDX::SETT_EMUCOVOX].bValue = FALSE;
		arOptionItems[OPT_IDX::SETT_EMUAY].bValue = FALSE;
	}

	UpdateData(FALSE);
}


void BKSettDlg_2::OnBnClickedCheckSettEmuay()
{
	UpdateData(TRUE);

	if (arOptionItems[OPT_IDX::SETT_EMUAY].bValue)
	{
		arOptionItems[OPT_IDX::SETT_EMUCOVOX].bValue = FALSE;
		arOptionItems[OPT_IDX::SETT_EMUMENESTREL].bValue = FALSE;
	}

	UpdateData(FALSE);
}

void BKSettDlg_2::OnBnClickedCheckSettJoystick()
{
	UpdateData(TRUE);

	if (arOptionItems[OPT_IDX::SETT_JOYSTICK].bValue)
	{
		arOptionItems[OPT_IDX::SETT_EMUMOUSE].bValue = FALSE;
		arOptionItems[OPT_IDX::SETT_ICLBLOCK].bValue = FALSE;
	}

	UpdateData(FALSE);
}


void BKSettDlg_2::OnBnClickedCheckSettEmumouse()
{
	UpdateData(TRUE);

	if (arOptionItems[OPT_IDX::SETT_EMUMOUSE].bValue)
	{
		arOptionItems[OPT_IDX::SETT_JOYSTICK].bValue = FALSE;
		arOptionItems[OPT_IDX::SETT_ICLBLOCK].bValue = FALSE;
	}

	UpdateData(FALSE);
}


void BKSettDlg_2::OnBnClickedCheckSettIclblock()
{
	UpdateData(TRUE);

	if (arOptionItems[OPT_IDX::SETT_ICLBLOCK].bValue)
	{
		arOptionItems[OPT_IDX::SETT_JOYSTICK].bValue = FALSE;
		arOptionItems[OPT_IDX::SETT_EMUMOUSE].bValue = FALSE;
	}

	UpdateData(FALSE);
}

void BKSettDlg_2::OnOK()
{
	// блокируем клавишу Enter
}


void BKSettDlg_2::OnCancel()
{
	// блокируем клавишу Esc
}

#endif