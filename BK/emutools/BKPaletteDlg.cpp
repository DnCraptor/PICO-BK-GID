// BKPaletteDlg.cpp: файл реализации
//
#ifdef UI
#include "pch.h"
#include "BK.h"

#include "afxcolorbutton.h"

#include "BKPaletteDlg.h"
#include "ScreenColors_Shared.h"
#include "Ini.h"

// Диалоговое окно CBKPaletteDlg

IMPLEMENT_DYNAMIC(CBKPaletteDlg, CBaseDialog)

CBKPaletteDlg::CBKPaletteDlg(CWnd *pParent /*=nullptr*/)
	: CBaseDialog(CBKPaletteDlg::IDD, pParent)
{
}

CBKPaletteDlg::~CBKPaletteDlg()
    = default;

COLORREF CBKPaletteDlg::BGRtoColorref(DWORD bgr) const
{
	return RGB((bgr & 0xff0000) >> 16,
	           (bgr & 0x00ff00) >> 8,
	           (bgr & 0x0000ff));
}

DWORD CBKPaletteDlg::ColorrefToBGR(COLORREF col) const
{
	return 0xff000000
	       | GetRValue(col) << 16
	       | GetGValue(col) << 8
	       | GetBValue(col);
}

void CBKPaletteDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_PMBL, m_btnPalMonoBK);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_PMWH, m_btnPalMonoWH);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_PAC0, m_arPalMonoAdapt[0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_PAC1, m_arPalMonoAdapt[1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_PAC2, m_arPalMonoAdapt[2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_PAC3, m_arPalMonoAdapt[3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P0C0,  m_arPalColor[ 0][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P0C1,  m_arPalColor[ 0][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P0C2,  m_arPalColor[ 0][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P0C3,  m_arPalColor[ 0][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P1C0,  m_arPalColor[ 1][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P1C1,  m_arPalColor[ 1][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P1C2,  m_arPalColor[ 1][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P1C3,  m_arPalColor[ 1][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P2C0,  m_arPalColor[ 2][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P2C1,  m_arPalColor[ 2][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P2C2,  m_arPalColor[ 2][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P2C3,  m_arPalColor[ 2][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P3C0,  m_arPalColor[ 3][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P3C1,  m_arPalColor[ 3][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P3C2,  m_arPalColor[ 3][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P3C3,  m_arPalColor[ 3][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P4C0,  m_arPalColor[ 4][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P4C1,  m_arPalColor[ 4][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P4C2,  m_arPalColor[ 4][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P4C3,  m_arPalColor[ 4][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P5C0,  m_arPalColor[ 5][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P5C1,  m_arPalColor[ 5][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P5C2,  m_arPalColor[ 5][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P5C3,  m_arPalColor[ 5][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P6C0,  m_arPalColor[ 6][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P6C1,  m_arPalColor[ 6][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P6C2,  m_arPalColor[ 6][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P6C3,  m_arPalColor[ 6][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P7C0,  m_arPalColor[ 7][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P7C1,  m_arPalColor[ 7][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P7C2,  m_arPalColor[ 7][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P7C3,  m_arPalColor[ 7][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P8C0,  m_arPalColor[ 8][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P8C1,  m_arPalColor[ 8][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P8C2,  m_arPalColor[ 8][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P8C3,  m_arPalColor[ 8][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P9C0,  m_arPalColor[ 9][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P9C1,  m_arPalColor[ 9][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P9C2,  m_arPalColor[ 9][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P9C3,  m_arPalColor[ 9][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P10C0, m_arPalColor[10][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P10C1, m_arPalColor[10][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P10C2, m_arPalColor[10][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P10C3, m_arPalColor[10][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P11C0, m_arPalColor[11][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P11C1, m_arPalColor[11][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P11C2, m_arPalColor[11][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P11C3, m_arPalColor[11][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P12C0, m_arPalColor[12][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P12C1, m_arPalColor[12][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P12C2, m_arPalColor[12][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P12C3, m_arPalColor[12][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P13C0, m_arPalColor[13][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P13C1, m_arPalColor[13][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P13C2, m_arPalColor[13][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P13C3, m_arPalColor[13][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P14C0, m_arPalColor[14][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P14C1, m_arPalColor[14][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P14C2, m_arPalColor[14][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P14C3, m_arPalColor[14][3]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P15C0, m_arPalColor[15][0]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P15C1, m_arPalColor[15][1]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P15C2, m_arPalColor[15][2]);
	DDX_Control(pDX, IDC_MFCCOLORBUTTON_P15C3, m_arPalColor[15][3]);
	DDX_Text(pDX, IDC_STATIC_PRESET_FILENAME, g_Config.m_strPaletteFileName);
	DDX_Control(pDX, IDC_COMBO_PRESET_MONO, m_comboPresetsMono);
}


BEGIN_MESSAGE_MAP(CBKPaletteDlg, CBaseDialog)
	ON_BN_CLICKED(IDC_BUTTON_RESET_MONOPAL, &CBKPaletteDlg::OnBnClickedButtonResetMonopal)
	ON_BN_CLICKED(IDC_BUTTON_RESET_MONOADAPT_PAL, &CBKPaletteDlg::OnBnClickedButtonResetMonoadaptPal)
	ON_BN_CLICKED(IDC_BUTTON_RESET_COLORPAL, &CBKPaletteDlg::OnBnClickedButtonResetColorpal)
	ON_CBN_SELCHANGE(IDC_COMBO_PRESET_MONO, &CBKPaletteDlg::OnCbnSelchangePresetMono)
	ON_BN_CLICKED(IDC_BUTTON_SAVE_COLORPAL, &CBKPaletteDlg::OnBnClickedButtonSaveColorpal)
	ON_BN_CLICKED(IDC_BUTTON_LOAD_COLORPAL, &CBKPaletteDlg::OnBnClickedButtonLoadColorpal)
END_MESSAGE_MAP()


// Обработчики сообщений CBKPaletteDlg

static const UINT arnMonochromePalettePresetNameIDs[4] =
{
	IDS_PALETTE_BWPRESET0,
	IDS_PALETTE_BWPRESET1,
	IDS_PALETTE_BWPRESET2,
	IDS_PALETTE_BWPRESET3
};


BOOL CBKPaletteDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	CString strMore(MAKEINTRESOURCE(IDS_PALETTE_MORE));
	m_btnPalMonoBK.EnableOtherButton(strMore);
	m_btnPalMonoWH.EnableOtherButton(strMore);

	for (auto &PalMono : m_arPalMonoAdapt)
	{
		PalMono.EnableOtherButton(strMore);
	}

	for (auto &PalColor : m_arPalColor)
	{
		for (auto &Pal : PalColor)
		{
			Pal.EnableOtherButton(strMore);
		}
	}

	// Пресеты для монохромных режимов
	int nNum = 0;   // это номер текущего пресета.

	for (auto id : arnMonochromePalettePresetNameIDs)
	{
		m_comboPresetsMono.AddString(CString(MAKEINTRESOURCE(id)));    // Добавляем в список читабельное ИМЯ градации
	}

	UpdateFileNameCtrl();   // Обновим поле имени файла
	UpdateData(FALSE);
	GrabPalette();
	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}

// При открытии окна, хотелось бы видеть имя файла, который загружали/сохраняли ранее
void CBKPaletteDlg::UpdateFileNameCtrl(bool clear) const
{
	if (clear)
	{
		auto pW = GetDlgItem(IDC_STATIC_PRESET_FILENAME);
		pW->ShowWindow(FALSE);
		pW->SetWindowText(_T(""));
		pW->ShowWindow(TRUE);
	}
}

void CBKPaletteDlg::GrabPalette()
{
	UpdateData(TRUE);
	m_btnPalMonoBK.SetColor(BGRtoColorref(g_pMonochromePalette[0][0]));
	m_btnPalMonoWH.SetColor(BGRtoColorref(g_pMonochromePalette[0][1]));

	for (int i = 0; i < 4; ++i)
	{
		m_arPalMonoAdapt[i].SetColor(BGRtoColorref(g_pAdaptMonochromePalette[0][i]));
	}

	for (int j = 0; j < 16; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			m_arPalColor[j][i].SetColor(BGRtoColorref(g_pColorPalettes[j][i]));
		}
	}

	// Пресеты для монохромных режимов
	int nNum = 0;   // это номер текущего пресета.

	for (int i = 0; i < 4; ++i)
	{
		// сравниваем какой-нибудь цвет в пресете с цветом в наборе пресетов, можно сравнивать любой
		// я решил сравнивать второй цвет
		if (g_pAdaptMonochromePalette[0][2] == g_pAdaptMonochromePalette_std[i * 2][2])
		{
			nNum = i; // нашли
		}
	}

	m_comboPresetsMono.SetCurSel(nNum);
	UpdateData(FALSE);
}


void CBKPaletteDlg::OnBnClickedButtonResetMonopal()
{
	UpdateData(TRUE);
	m_btnPalMonoBK.SetColor(BGRtoColorref(g_pMonochromePalette_std[0][0]));
	m_btnPalMonoWH.SetColor(BGRtoColorref(g_pMonochromePalette_std[0][1]));
	UpdateData(FALSE);
}


void CBKPaletteDlg::OnBnClickedButtonResetMonoadaptPal()
{
	UpdateData(TRUE);

	for (int i = 0; i < 4; ++i)
	{
		m_arPalMonoAdapt[i].SetColor(BGRtoColorref(g_pAdaptMonochromePalette_std[0][i]));
	}

	UpdateData(FALSE);
}


void CBKPaletteDlg::OnBnClickedButtonResetColorpal()
{
	UpdateData(TRUE);

	for (int j = 0; j < 16; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			m_arPalColor[j][i].SetColor(BGRtoColorref(g_pColorPalettes_std[j][i]));
		}
	}

	g_Config.m_strPaletteFileName.Empty();
	UpdateFileNameCtrl(true);
	UpdateData(FALSE);
}


void CBKPaletteDlg::OnOK()
{
	FixPalette();
	CBaseDialog::OnOK();
}


void CBKPaletteDlg::FixPalette()
{
	UpdateData(TRUE);
	// сохраняем изменения
	g_pMonochromePalette[0][0] = ColorrefToBGR(m_btnPalMonoBK.GetColor());
	g_pMonochromePalette[0][1] = ColorrefToBGR(m_btnPalMonoWH.GetColor());
	g_pMonochromePalette[0][2] = g_pMonochromePalette[1][0] = g_pMonochromePalette[1][1] = g_pMonochromePalette[0][0];
	g_pMonochromePalette[0][3] = g_pMonochromePalette[1][2] = g_pMonochromePalette[1][3] = g_pMonochromePalette[0][1];

	for (int i = 0; i < 4; ++i)
	{
		g_pAdaptMonochromePalette[1][i] = g_pAdaptMonochromePalette[0][i] = ColorrefToBGR(m_arPalMonoAdapt[i].GetColor());
	}

	for (int j = 0; j < 16; ++j)
	{
		for (int i = 0; i < 4; ++i)
		{
			g_pColorPalettes[j][i] = ColorrefToBGR(m_arPalColor[j][i].GetColor());
		}
	}
}

void CBKPaletteDlg::OnCbnSelchangePresetMono()
{
	UpdateData(TRUE);
	int n = m_comboPresetsMono.GetCurSel() * 2;
	m_btnPalMonoBK.SetColor(BGRtoColorref(g_pAdaptMonochromePalette_std[n][0]));
	m_btnPalMonoWH.SetColor(BGRtoColorref(g_pAdaptMonochromePalette_std[n][3]));

	for (int i = 0; i < 4; ++i)
	{
		m_arPalMonoAdapt[i].SetColor(BGRtoColorref(g_pAdaptMonochromePalette_std[n][i]));
	}

	UpdateData(FALSE);
}

// Загрузка-Сохранение набора палитр
// TODO: перенести в ресурсы
static CString strDefExt = _T(".bkp");  // MAKEINTRESOURCE(IDS_FILEFILTER_BKHDDIMG);
static CString strFilterPAL = _T("Файл палитры БК (.bkp)|*.bkp|Все файлы (*.*)|*.*||");

void CBKPaletteDlg::OnBnClickedButtonSaveColorpal()
{
	CFileDialog dlg(FALSE, strDefExt, nullptr,
	                OFN_HIDEREADONLY | OFN_CREATEPROMPT | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterPAL, this);
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strSavesPath.c_str(); // диалог всегда будем начинать с домашней директории

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = { dlg.GetPathName().GetString() };
		g_Config.m_strPaletteFileName = { str.filename().c_str() };
		UpdateFileNameCtrl(true);
		UpdateData(FALSE);
		CIni iniFile;
		iniFile.SetIniFileName(str);
		FixPalette(); //при сохранении палитры, применяются изменения.
		// можно сделать без применения, будет сложнее
		// сохраним стандартную обычную ЧБ палитру
		CString strPal = ::ColorToStr(g_pMonochromePalette[0][0])
		                 + _T(',') + ::ColorToStr(g_pMonochromePalette[0][1]);
		iniFile.SetValueString(IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALBW, strPal);
		// сохраним стандартную адаптивную ЧБ палитру
		strPal = ::PaletteToStr(&g_pAdaptMonochromePalette[0][0]);
		iniFile.SetValueString(IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALADAPTBW, strPal);

		// сохраним цветные палитры
		for (int i = 0; i < 16; ++i)
		{
			strPal = ::PaletteToStr(&g_pColorPalettes[i][0]);
			iniFile.SetValueString(IDS_INI_SECTIONNAME_PALETTES, arColIni[i], strPal);
		}

		iniFile.FlushIni();
		iniFile.Clear();
	}
}

void CBKPaletteDlg::OnBnClickedButtonLoadColorpal()
{
	CFileDialog dlg(TRUE, nullptr, nullptr,
	                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR | OFN_EXPLORER,
	                strFilterPAL, this);
	dlg.GetOFN().lpstrInitialDir = g_Config.m_strSavesPath.c_str(); // диалог всегда будем начинать с домашней директории

	if (dlg.DoModal() == IDOK)
	{
		fs::path str = { dlg.GetPathName().GetString() };
		g_Config.m_strPaletteFileName = { str.filename().c_str() };
		UpdateFileNameCtrl(true);
		UpdateData(FALSE);
		CIni iniFile;
		iniFile.SetIniFileName(str);
		// создадим стандартную обычную ЧБ палитру
		CString strPal = ::ColorToStr(g_pMonochromePalette_std[0][0])
		                 + _T(',') + ::ColorToStr(g_pMonochromePalette_std[0][1]);
		// загрузим текущую обычную ЧБ палитру
		CString strPalVal = iniFile.GetValueString(IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALBW, strPal);
		::StrToPalette(strPalVal, &g_pMonochromePalette[0][0]);
		g_pMonochromePalette[0][2] = g_pMonochromePalette[1][0] = g_pMonochromePalette[1][1] = g_pMonochromePalette[0][0];
		g_pMonochromePalette[0][3] = g_pMonochromePalette[1][2] = g_pMonochromePalette[1][3] = g_pMonochromePalette[0][1];
		// создадим стандартную адаптивную ЧБ палитру
		strPal = ::PaletteToStr(&g_pAdaptMonochromePalette_std[0][0]);
		// загрузим текущую адаптивную ЧБ палитру
		strPalVal = iniFile.GetValueString(IDS_INI_SECTIONNAME_PALETTES, IDS_INI_PALADAPTBW, strPal);
		::StrToPalette(strPalVal, &g_pAdaptMonochromePalette[0][0]);
		g_pAdaptMonochromePalette[1][0] = g_pAdaptMonochromePalette[0][0];
		g_pAdaptMonochromePalette[1][1] = g_pAdaptMonochromePalette[0][1];
		g_pAdaptMonochromePalette[1][2] = g_pAdaptMonochromePalette[0][2];
		g_pAdaptMonochromePalette[1][3] = g_pAdaptMonochromePalette[0][3];

		// создадим цветные палитры
		for (int i = 0; i < 16; ++i)
		{
			strPal = ::PaletteToStr(&g_pColorPalettes_std[i][0]);
			strPalVal = iniFile.GetValueString(IDS_INI_SECTIONNAME_PALETTES, arColIni[i], strPal);
			::StrToPalette(strPalVal, &g_pColorPalettes[i][0]);
		}

		GrabPalette(); //при загрузке палитры, она сразу применяется.
		// Вот тут сделать без применения довольно сложно
	}
}

#endif