// BKSettPage_3.cpp: файл реализации
//
#ifdef UI
#include "pch.h"

#include "BKSettPage_3.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// Диалоговое окно BKSettDlg_3

IMPLEMENT_DYNAMIC(BKSettDlg_3, CBaseDialog)

BKSettDlg_3::BKSettDlg_3(CWnd *pParent /*=nullptr*/)
	: CBaseDialog(IDD_SETTINGS_3, pParent)
	, m_bCurrOrigScreenshotsSize(FALSE)
	, m_bCurrBigButtons(FALSE)
	, m_bCurrExclusiveOpenImages(FALSE)
	, m_b2ndAyEnable(FALSE)
	, m_nCurrScreenshotNumber(0)
	, m_nDateInsteadOfScreenshotNumber(FALSE)
	, m_strCurrFFMPEGLine(_T(""))
	, m_nCurrSSRPos(0)
	, m_nCurrSndChipModelPos{0, 0}
	, m_nCurr2AYWorkMode(0)
	, m_nCurrScreenRenderType(0)
	, m_nCurrOscRenderType(0)
	, m_strCurrSoundChipFrequency(_T(""))
{}

BKSettDlg_3 ::~BKSettDlg_3()
    = default;


BEGIN_MESSAGE_MAP(BKSettDlg_3, CBaseDialog)
	ON_BN_CLICKED(IDC_BUTTON_SETT_FFMPEG_DEFAULT, &BKSettDlg_3::OnBnClickedButtonFfmpegDefault)
	ON_MESSAGE(WM_SETT_SENDTOTAB, &BKSettDlg_3::OnSendToTab)
	ON_BN_CLICKED(IDC_CHECK_SETT_SNDCHIPMODEL2, &BKSettDlg_3::OnBnClickedCheckSettSndchipmodel2)
	ON_BN_CLICKED(IDC_CHECK_SETT_DATEINSTEADOFNUMBER, &BKSettDlg_3::OnBnClickedCheckDateInsteadOfScreenshotNumber)
END_MESSAGE_MAP()

void BKSettDlg_3::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_SETT_ORIGINSCREENSIZE, m_bCurrOrigScreenshotsSize);
	DDX_Check(pDX, IDC_CHECK_SETT_BIGBUTTONS, m_bCurrBigButtons);
	DDX_Check(pDX, IDC_CHECK_SETT_EXCLUSIVEOPENIMG, m_bCurrExclusiveOpenImages);
	DDX_Text(pDX, IDC_EDIT_SETT_SCREENSHOTNUMBER, m_nCurrScreenshotNumber);
	DDX_Check(pDX, IDC_CHECK_SETT_DATEINSTEADOFNUMBER, m_nDateInsteadOfScreenshotNumber);
	DDX_Text(pDX, IDC_EDIT_SETT_FFMPEG, m_strCurrFFMPEGLine);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_SOUNDSR, m_nCurrSSRPos);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_SNDCHIPMODEL, m_nCurrSndChipModelPos[AY1]);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_SNDCHIPMODEL2, m_nCurrSndChipModelPos[AY2]);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_2AYMODE, m_nCurr2AYWorkMode);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_SCRRENDER, m_nCurrScreenRenderType);
	DDX_CBIndex(pDX, IDC_COMBO_SETT_OSCRENDER, m_nCurrOscRenderType);
	DDX_CBString(pDX, IDC_COMBO_SETT_SNDCHIPFREQ, m_strCurrSoundChipFrequency);
	DDX_Check(pDX, IDC_CHECK_SETT_SNDCHIPMODEL2, m_b2ndAyEnable);
}

// Обработчики сообщений BKSettDlg_3
BOOL BKSettDlg_3::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	auto ScreenRenderType = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_SCRRENDER));
	ScreenRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_OGL)).GetString());
	ScreenRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_D2D)).GetString());
	ScreenRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_DIB)).GetString());
	ScreenRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_D3D)).GetString());
	m_nCurrScreenRenderType = static_cast<int>(g_Config.m_nScreenRenderType);
	auto OscRenderType = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_OSCRENDER));
	OscRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_OGL)).GetString());
	OscRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_D2D)).GetString());
	OscRenderType->AddString(CString(MAKEINTRESOURCE(IDS_RENDER_GDI)).GetString());
	m_nCurrOscRenderType = static_cast<int>(g_Config.m_nOscRenderType);
	auto SoundSampleRate = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_SOUNDSR));
	int nPos = 0;

	for (const auto &i : g_EnabledSSR)
	{
		SoundSampleRate->AddString(Global::IntToString(i).GetBuffer());  // Заберём из массива список для выпадашки

		if (g_Config.m_nSoundSampleRate == i) // Переведём значение в номер позиции в выпадашке
		{
			m_nCurrSSRPos = nPos;
		}

		nPos++;
	}

	static int nChipComboIDs[AY_NUMS] = { IDC_COMBO_SETT_SNDCHIPMODEL, IDC_COMBO_SETT_SNDCHIPMODEL2 };

	for (int ay = AY1; ay < AY_NUMS; ++ay)
	{
		auto SoundChipModel = static_cast<CComboBox *>(GetDlgItem(nChipComboIDs[ay]));
		nPos = 0;

		for (const auto &m : g_EnabledSoundChipModels)
		{
			SoundChipModel->AddString(CString(MAKEINTRESOURCE(m.nIDstrModel)).GetString());

			if (g_Config.m_nSoundChipModel[ay] == m.nModelNum) // Переведём значение в номер позиции в выпадашке
			{
				m_nCurrSndChipModelPos[ay] = nPos;
			}

			nPos++;
		}
	}

	auto MultiSoundChipWorkMode = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_2AYMODE));
	nPos = 0;

	for (const auto &m : g_MultiSoundChipWorkModes)
	{
		MultiSoundChipWorkMode->AddString(CString(MAKEINTRESOURCE(m.nIDstrMode)).GetString());

		if (g_Config.m_n2AYWorkMode == m.nMode) // Переведём значение в номер позиции в выпадашке
		{
			m_nCurr2AYWorkMode = nPos;
		}

		nPos++;
	}

	// Частота сопроцессора/ов
	auto SoundChipFreq = static_cast<CComboBox *>(GetDlgItem(IDC_COMBO_SETT_SNDCHIPFREQ));
	bool bInList = false;

	for (const auto &f : g_SoundChipFrequences)
	{
		CString str = CString(MAKEINTRESOURCE(f.nIDstrFreq));
		SoundChipFreq->AddString(str.GetString());

		if (g_Config.m_nSoundChipFrequency == f.nFreq) // Переведём значение в номер позиции в выпадашке
		{
			m_strCurrSoundChipFrequency = str;
			bInList = true;
		}
	}

	if (!bInList)
	{
		m_strCurrSoundChipFrequency = Global::IntToString(g_Config.m_nSoundChipFrequency);
	}

	// второй AY
	m_b2ndAyEnable = g_Config.m_b2ndAY8910;
	// Screenshots sizes
	m_bCurrOrigScreenshotsSize = g_Config.m_bOrigScreenshotSize;
	// Screenshots number
	m_nCurrScreenshotNumber = g_Config.m_nScreenshotNumber;
	// Date Instead Of Screenshot Number
	m_nDateInsteadOfScreenshotNumber = g_Config.m_nDateInsteadOfScreenshotNumber;
	// Big buttons for Instrumental Panel
	m_bCurrBigButtons = g_Config.m_bBigButtons;
	// Как открывать образы
	m_bCurrExclusiveOpenImages = g_Config.m_bExclusiveOpenImages;
	// FFMPEG Line
	m_strCurrFFMPEGLine = g_Config.m_strFFMPEGLine;
	// Check FFMPEG
	bool m_bFoundFFMPEG = Global::CheckFFMPEG();
	GetDlgItem(IDC_EDIT_SETT_FFMPEG)->EnableWindow(m_bFoundFFMPEG ? TRUE : FALSE);
	UpdateData(FALSE);
	OnBnClickedCheckSettSndchipmodel2();
	OnBnClickedCheckDateInsteadOfScreenshotNumber();
	return TRUE;
}


LRESULT BKSettDlg_3::OnSendToTab(WPARAM, LPARAM)
{
	return LRESULT(Save());
}

DWORD BKSettDlg_3::Save()
{
	DWORD nChanges = NO_CHANGES;
	UpdateData(TRUE);
	// получим новый тип рендера экрана
	auto nNewRender = static_cast<CONF_SCREEN_RENDER>(m_nCurrScreenRenderType);

	if (g_Config.m_nScreenRenderType != nNewRender)  // Если что-то поменялось
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_nScreenRenderType = nNewRender;
	}

	// получим новый тип рендера осциллографа
	auto nNewOSC = static_cast<CONF_OSCILLOSCOPE_RENDER>(m_nCurrOscRenderType);

	if (g_Config.m_nOscRenderType != nNewOSC)        // Если что-то поменялось
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_nOscRenderType = nNewOSC;
	}

	g_Config.CheckRenders(); // нужно, т.к. есть зависимость от ОС, в которой работает эмулятор

	// получим новую частоту дискретизации звука
	if (g_Config.m_nSoundSampleRate != g_EnabledSSR[m_nCurrSSRPos])  // Если что-то поменялось
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_nSoundSampleRate = g_EnabledSSR[m_nCurrSSRPos];
	}

	// получим новый режим работы двух AY
	if (g_Config.m_n2AYWorkMode != g_MultiSoundChipWorkModes[m_nCurr2AYWorkMode].nMode)  // Если что-то поменялось
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_n2AYWorkMode = g_MultiSoundChipWorkModes[m_nCurr2AYWorkMode].nMode;
	}

	// получим новую модель сопра #1 и #2
	for (int ay = AY1; ay < AY_NUMS; ++ay)
	{
		if (g_Config.m_nSoundChipModel[ay] != g_EnabledSoundChipModels[m_nCurrSndChipModelPos[ay]].nModelNum)  // Если что-то поменялось
		{
			nChanges |= CHANGES_NEEDREBOOT;
			g_Config.m_nSoundChipModel[ay] = g_EnabledSoundChipModels[m_nCurrSndChipModelPos[ay]].nModelNum;
		}
	}

	// получим новую частоту его работы
	int freq = _tcstoul(m_strCurrSoundChipFrequency, nullptr, 10);

	if (g_Config.m_nSoundChipFrequency != freq)  // Если что-то поменялось
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_nSoundChipFrequency = freq;
	}

	g_Config.CheckSndChipFreq();

	// Сохраним разрешение второго AY
	if (!!m_b2ndAyEnable != g_Config.m_b2ndAY8910)
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_b2ndAY8910 = !!m_b2ndAyEnable;
	}

	// Save Screenshots sizes
	if (!!m_bCurrOrigScreenshotsSize != g_Config.m_bOrigScreenshotSize)
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_bOrigScreenshotSize = !!m_bCurrOrigScreenshotsSize;
	}

	// Save Big buttons for Instrumental Panel
	if (!!m_bCurrBigButtons != g_Config.m_bBigButtons)
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_bBigButtons = !!m_bCurrBigButtons;
	}

	// сохраним способ открывания образов
	if (!!m_bCurrExclusiveOpenImages != g_Config.m_bExclusiveOpenImages)
	{
		nChanges |= CHANGES_NEEDREBOOT;
		g_Config.m_bExclusiveOpenImages = !!m_bCurrExclusiveOpenImages;
	}

	// Save Screenshots number
	if (m_nCurrScreenshotNumber != g_Config.m_nScreenshotNumber)
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_nScreenshotNumber = m_nCurrScreenshotNumber;
	}

	// Save Date Instead Of Screenshot Number
	if (!!m_nDateInsteadOfScreenshotNumber != g_Config.m_nDateInsteadOfScreenshotNumber)
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_nDateInsteadOfScreenshotNumber = m_nDateInsteadOfScreenshotNumber;
	}

	// Save FFMPEG Line
	if (m_strCurrFFMPEGLine != g_Config.m_strFFMPEGLine)
	{
		nChanges |= CHANGES_NOREBOOT;
		g_Config.m_strFFMPEGLine = m_strCurrFFMPEGLine;
	}

	return nChanges;
}

void BKSettDlg_3::OnBnClickedButtonFfmpegDefault()
{
	SetDlgItemText(IDC_EDIT_SETT_FFMPEG, DEFAULT_FFMPEG_CMDLINE);
}



void BKSettDlg_3::OnBnClickedCheckSettSndchipmodel2()
{
	UpdateData(TRUE);
	GetDlgItem(IDC_COMBO_SETT_SNDCHIPMODEL2)->EnableWindow(m_b2ndAyEnable);
	GetDlgItem(IDC_COMBO_SETT_2AYMODE)->EnableWindow(m_b2ndAyEnable);
	auto p = GetDlgItem(IDC_STATIC_SETT_2NDAY);
	p->ShowWindow(FALSE);
	p->EnableWindow(m_b2ndAyEnable);
	p->ShowWindow(TRUE);
	p = GetDlgItem(IDC_STATIC_SETT_2NDAY_MODE);
	p->ShowWindow(FALSE);
	p->EnableWindow(m_b2ndAyEnable);
	p->ShowWindow(TRUE);
}

void BKSettDlg_3::OnBnClickedCheckDateInsteadOfScreenshotNumber()
{
	UpdateData(TRUE);
	GetDlgItem(IDC_EDIT_SETT_SCREENSHOTNUMBER)->EnableWindow(!m_nDateInsteadOfScreenshotNumber);
	auto p = GetDlgItem(IDC_TEXT_SETT_SCREENSHOTNUMBER);
	p->ShowWindow(FALSE);
	p->EnableWindow(!m_nDateInsteadOfScreenshotNumber);
	p->ShowWindow(TRUE);
}

void BKSettDlg_3::OnOK()
{
	// блокируем клавишу Enter
}


void BKSettDlg_3::OnCancel()
{
	// блокируем клавишу Esc
}

#endif