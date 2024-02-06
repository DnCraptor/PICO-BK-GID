// AYVolPan.cpp: файл реализации
//
#ifdef UI
#include "pch.h"
#include "resource.h"
#include "BKAYVolPan.h"


// Диалоговое окно CAYVolPan

IMPLEMENT_DYNAMIC(CBKAYVolPan, CBaseDialog)

CBKAYVolPan::CBKAYVolPan(CWnd *pParent /*=nullptr*/)
	: CBaseDialog(IDD_AYVOLPAN_DLG, pParent)
{
}

CBKAYVolPan::~CBKAYVolPan()
    = default;

void CBKAYVolPan::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_AYPANA, m_AY[AY1].chan[CHAN_A].pan.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYPANB, m_AY[AY1].chan[CHAN_B].pan.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYPANC, m_AY[AY1].chan[CHAN_C].pan.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYVOLA, m_AY[AY1].chan[CHAN_A].vol.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYVOLB, m_AY[AY1].chan[CHAN_B].vol.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYVOLC, m_AY[AY1].chan[CHAN_C].vol.ctrl);
	DDX_Text(pDX, IDC_STATIC_AYVOLA,  m_AY[AY1].chan[CHAN_A].vol.nVol);
	DDX_Text(pDX, IDC_STATIC_AYVOLB,  m_AY[AY1].chan[CHAN_B].vol.nVol);
	DDX_Text(pDX, IDC_STATIC_AYVOLC,  m_AY[AY1].chan[CHAN_C].vol.nVol);
	DDX_Text(pDX, IDC_STATIC_AYPANAL, m_AY[AY1].chan[CHAN_A].pan.nPanL);
	DDX_Text(pDX, IDC_STATIC_AYPANBL, m_AY[AY1].chan[CHAN_B].pan.nPanL);
	DDX_Text(pDX, IDC_STATIC_AYPANCL, m_AY[AY1].chan[CHAN_C].pan.nPanL);
	DDX_Text(pDX, IDC_STATIC_AYPANAR, m_AY[AY1].chan[CHAN_A].pan.nPanR);
	DDX_Text(pDX, IDC_STATIC_AYPANBR, m_AY[AY1].chan[CHAN_B].pan.nPanR);
	DDX_Text(pDX, IDC_STATIC_AYPANCR, m_AY[AY1].chan[CHAN_C].pan.nPanR);
	DDX_Control(pDX, IDC_SLIDER_AYPANA2, m_AY[AY2].chan[CHAN_A].pan.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYPANB2, m_AY[AY2].chan[CHAN_B].pan.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYPANC2, m_AY[AY2].chan[CHAN_C].pan.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYVOLA2, m_AY[AY2].chan[CHAN_A].vol.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYVOLB2, m_AY[AY2].chan[CHAN_B].vol.ctrl);
	DDX_Control(pDX, IDC_SLIDER_AYVOLC2, m_AY[AY2].chan[CHAN_C].vol.ctrl);
	DDX_Text(pDX, IDC_STATIC_AYVOLA2,  m_AY[AY2].chan[CHAN_A].vol.nVol);
	DDX_Text(pDX, IDC_STATIC_AYVOLB2,  m_AY[AY2].chan[CHAN_B].vol.nVol);
	DDX_Text(pDX, IDC_STATIC_AYVOLC2,  m_AY[AY2].chan[CHAN_C].vol.nVol);
	DDX_Text(pDX, IDC_STATIC_AYPANAL2, m_AY[AY2].chan[CHAN_A].pan.nPanL);
	DDX_Text(pDX, IDC_STATIC_AYPANBL2, m_AY[AY2].chan[CHAN_B].pan.nPanL);
	DDX_Text(pDX, IDC_STATIC_AYPANCL2, m_AY[AY2].chan[CHAN_C].pan.nPanL);
	DDX_Text(pDX, IDC_STATIC_AYPANAR2, m_AY[AY2].chan[CHAN_A].pan.nPanR);
	DDX_Text(pDX, IDC_STATIC_AYPANBR2, m_AY[AY2].chan[CHAN_B].pan.nPanR);
	DDX_Text(pDX, IDC_STATIC_AYPANCR2, m_AY[AY2].chan[CHAN_C].pan.nPanR);
}


BEGIN_MESSAGE_MAP(CBKAYVolPan, CBaseDialog)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_BUTTON_AY1PAN_REVERT, &CBKAYVolPan::OnBnClickedButtonAy1panRevert)
	ON_BN_CLICKED(IDC_BUTTON_AY1PAN_DEFAULT, &CBKAYVolPan::OnBnClickedButtonAy1panDefault)
	ON_BN_CLICKED(IDC_BUTTON_AY1VOL_REVERT, &CBKAYVolPan::OnBnClickedButtonAy1volRevert)
	ON_BN_CLICKED(IDC_BUTTON_AY1VOL_DEFAULT, &CBKAYVolPan::OnBnClickedButtonAy1volDefault)
	ON_BN_CLICKED(IDC_BUTTON_AY2PAN_REVERT, &CBKAYVolPan::OnBnClickedButtonAy2panRevert)
	ON_BN_CLICKED(IDC_BUTTON_AY2PAN_DEFAULT, &CBKAYVolPan::OnBnClickedButtonAy2panDefault)
	ON_BN_CLICKED(IDC_BUTTON_AY2VOL_REVERT, &CBKAYVolPan::OnBnClickedButtonAy2volRevert)
	ON_BN_CLICKED(IDC_BUTTON_AY2VOL_DEFAULT, &CBKAYVolPan::OnBnClickedButtonAy2volDefault)
END_MESSAGE_MAP()


// Обработчики сообщений CAYVolPan

// константы числа 100%
constexpr double VOL_VALUE_D = 100.0;

constexpr auto AY_VOLPAN_SLIDER_SCALE = 100; // масштаб слайдеров
constexpr auto AY_VOLPAN_SLIDER_TICK_STEP = 10; // шаг насечек слайдеров


BOOL CBKAYVolPan::OnInitDialog()
{
	CBaseDialog::OnInitDialog();
	static int nIDs[AY_NUMS] = { IDC_NAME_AY, IDC_NAME_AY2 };

	for (int ay = 0; ay < AY_NUMS; ++ay)
	{
		//******************************************************************************************
		// Model Chip AY
		CString aystr = CString(MAKEINTRESOURCE(g_EnabledSoundChipModels[g_Config.m_nSoundChipModel[ay]].nIDstrModel));
		CString _str;
		_str.Format(L"AY/YM %d ( model %s )", ay + 1, aystr.GetString());
		SetDlgItemText(nIDs[ay], _str.GetString());
		CConfig::AYVolPan_s s = g_Config.getVolPan(ay);
		m_AY[ay].orig = m_AY[ay].curr = s;

		for (int c = 0; c < AY_CHANS; ++c)
		{
			AYChan &ac = m_AY[ay].chan[c];
			ac.pan.nPanL = s.nPL[c];
			ac.pan.nPanR = AY_PAN_BASE - ac.pan.nPanL;
			ac.pan.ctrl.SetRange(0, AY_VOLPAN_SLIDER_SCALE, FALSE);
			ac.pan.ctrl.SetPos(AY_VOLPAN_SLIDER_SCALE * ac.pan.nPanR / AY_PAN_BASE);
			ac.vol.nVol = int(s.V[c] * VOL_VALUE_D);
			ac.vol.ctrl.SetRange(0, AY_VOLPAN_SLIDER_SCALE, FALSE);
			ac.vol.ctrl.SetPos(static_cast<int>(AY_VOLPAN_SLIDER_SCALE * (AY_VOL_BASE - s.V[c])));
		}
	}

	for (int t = 0; t < AY_VOLPAN_SLIDER_SCALE; t += AY_VOLPAN_SLIDER_TICK_STEP)
	{
		for (auto &i : m_AY)
		{
			for (auto &c : i.chan)
			{
				c.pan.ctrl.SetTic(t);
				c.vol.ctrl.SetTic(t);
			}
		}
	}

	UpdateData(FALSE);
	return TRUE;  // return TRUE unless you set the focus to a control
	// Исключение: страница свойств OCX должна возвращать значение FALSE
}


void CBKAYVolPan::Save()
{
	g_Config.setVolPan(AY1, m_AY[AY1].curr);
	g_Config.setVolPan(AY2, m_AY[AY2].curr);
	UpdateData(FALSE);
}

void CBKAYVolPan::OnOK()
{
	Save();
	CBaseDialog::OnOK();
}


void CBKAYVolPan::OnCancel()
{
	// при отмене, возвращаем всё как было
	g_Config.setVolPan(AY1, m_AY[AY1].orig);
	g_Config.setVolPan(AY2, m_AY[AY2].orig);
	CBaseDialog::OnCancel();
}


void CBKAYVolPan::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	auto pSlider = reinterpret_cast<CSliderCtrl *>(pScrollBar);

	for (auto &ay : m_AY)
	{
		for (int c = 0; c < AY_CHANS; ++c)
		{
			AYChan &ac = ay.chan[c];

			if (pSlider == &ac.pan.ctrl)
			{
				ay.curr.nPR[c] = ac.pan.nPanR = AY_PAN_BASE * ac.pan.ctrl.GetPos() / AY_VOLPAN_SLIDER_SCALE;
				ay.curr.nPL[c] = ac.pan.nPanL = AY_PAN_BASE - ac.pan.nPanR;
				Save();
				break;
			}
		}
	}

	CBaseDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CBKAYVolPan::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pScrollBar)
{
	auto pSlider = reinterpret_cast<CSliderCtrl *>(pScrollBar);

	for (auto &ay : m_AY)
	{
		for (int c = 0; c < AY_CHANS; ++c)
		{
			AYChan &ac = ay.chan[c];

			if (pSlider == &ac.vol.ctrl)
			{
				ay.curr.V[c] = AY_VOL_BASE - double(ac.vol.ctrl.GetPos()) / double(AY_VOLPAN_SLIDER_SCALE);
				ac.vol.nVol = int(ay.curr.V[c] * VOL_VALUE_D);
				Save();
				break;
			}
		}
	}

	CBaseDialog::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CBKAYVolPan::SetPanSlider(int nAY, CConfig::AYVolPan_s *s)
{
	for (int c = 0; c < AY_CHANS; ++c)
	{
		slPan &pan = m_AY[nAY].chan[c].pan;
		pan.nPanL = m_AY[nAY].curr.nPL[c] = s->nPL[c];
		pan.nPanR = m_AY[nAY].curr.nPR[c] = s->nPR[c];
		pan.ctrl.SetPos(AY_VOLPAN_SLIDER_SCALE * pan.nPanR / AY_PAN_BASE);
	}

	Save();
}


void CBKAYVolPan::SetVolSlider(int nAY, CConfig::AYVolPan_s *s)
{
	for (int c = 0; c < AY_CHANS; ++c)
	{
		slVol &vol = m_AY[nAY].chan[c].vol;
		m_AY[nAY].curr.V[c] = s->V[c];
		vol.nVol = int(s->V[c] * VOL_VALUE_D);
		vol.ctrl.SetPos(static_cast<int>(AY_VOLPAN_SLIDER_SCALE * (AY_VOL_BASE - s->V[c])));
	}

	Save();
}

// Установка слайдеров и значений на текущие.
// т.е. если мы сидим и играемся со слайдерами, и вдруг нам не понравилось - жмём кнопку "Вернуть"
// и отменяем все позиции, устанавливаем на те, что были изначально.
// если текущие == по умолчанию, то эффект равен действию кнопки установки по умолчанию.

void CBKAYVolPan::OnBnClickedButtonAy1panRevert()
{
	SetPanSlider(AY1, &m_AY[AY1].orig);
}

// Установка слайдеров и значений по умолчанию, на предустановленные в конфиге.

void CBKAYVolPan::OnBnClickedButtonAy1panDefault()
{
	CConfig::AYVolPan_s def = g_Config.getVolPan(-1); // получаем структуру значений по умолчанию.
	SetPanSlider(AY1, &def);
}


void CBKAYVolPan::OnBnClickedButtonAy1volRevert()
{
	SetVolSlider(AY1, &m_AY[AY1].orig);
}


void CBKAYVolPan::OnBnClickedButtonAy1volDefault()
{
	CConfig::AYVolPan_s def = g_Config.getVolPan(-1); // получаем структуру значений по умолчанию.
	SetVolSlider(AY1, &def);
}


void CBKAYVolPan::OnBnClickedButtonAy2panRevert()
{
	SetPanSlider(AY2, &m_AY[AY2].orig);
}


void CBKAYVolPan::OnBnClickedButtonAy2panDefault()
{
	CConfig::AYVolPan_s def = g_Config.getVolPan(-1); // получаем структуру значений по умолчанию.
	SetPanSlider(AY2, &def);
}


void CBKAYVolPan::OnBnClickedButtonAy2volRevert()
{
	SetVolSlider(AY2, &m_AY[AY2].orig);
}


void CBKAYVolPan::OnBnClickedButtonAy2volDefault()
{
	CConfig::AYVolPan_s def = g_Config.getVolPan(-1); // получаем структуру значений по умолчанию.
	SetVolSlider(AY2, &def);
}
#endif