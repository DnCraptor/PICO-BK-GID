// TapeManagerDlg.cpp : implementation file
//
#ifdef UI
#include "pch.h"
#include "resource.h"
#include "TapeManagerDlg.h"
#include "Config.h"
#include "Screen_Sizes.h"
#include "BKMessageBox.h"
#include "BrowseForFolder.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTapeManagerDlg dialog

IMPLEMENT_DYNAMIC(CTapeManagerDlg, CBaseDialog)

CTapeManagerDlg::CTapeManagerDlg(CWnd *pParent /*=nullptr*/)
	: CBaseDialog(CTapeManagerDlg::IDD, pParent)
	, m_pCurrCaptureBuffer(nullptr)
	, m_hWaveIn(nullptr)
	, m_bStatusExit(false)
	, m_bCaptureMode(false)
	, m_nCaptureLength(0)
	, m_nCaptureOffset(0)
	, m_bRun(false)
	, m_bCanceled(false)
	, m_strCurrentFolder(g_Config.m_strTapePath.c_str()) // начинаем обзор всегда с этого места. ВСЕГДА.
	, m_nSilenceLength(DEFAULT_SOUND_SAMPLE_RATE / 10)
	, m_nTapePos(0)
	, m_nTapeCount(0)
{
}

CTapeManagerDlg::~CTapeManagerDlg()
{
	m_imgList.DeleteImageList();
}

void CTapeManagerDlg::DoDataExchange(CDataExchange *pDX)
{
	CBaseDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_TM_FILES, m_list);
	DDX_Control(pDX, IDC_COMBO_TM_SAVEAS, m_comboSaveAs);
	DDX_Control(pDX, IDC_COMBO_TM_DEVICES, m_comboDevices);
	DDX_Text(pDX, IDC_EDIT_TM_FOLDER, m_strCurrentFolder);
}

BEGIN_MESSAGE_MAP(CTapeManagerDlg, CBaseDialog)
	ON_MESSAGE(MM_WIM_DATA, &CTapeManagerDlg::OnWaveInProc)
	ON_MESSAGE(WM_BUFFER_READY, &CTapeManagerDlg::OnBufferReady)
	ON_MESSAGE(WM_INFO_READY, &CTapeManagerDlg::OnInfoReady)
	ON_NOTIFY(LVN_GETDISPINFO, IDC_LIST_TM_FILES, &CTapeManagerDlg::OnGetdispinfoTmList)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_TM_FILES, &CTapeManagerDlg::OnRclickTmList)
	ON_BN_CLICKED(IDC_BUTTON_TM_LOAD, &CTapeManagerDlg::OnTmLoad)
	ON_BN_CLICKED(IDC_BUTTON_TM_BROWSE, &CTapeManagerDlg::OnTmBrowse)
	ON_BN_CLICKED(IDC_BUTTON_TM_REMOVE, &CTapeManagerDlg::OnTmRemove)
	ON_CBN_SELENDOK(IDC_COMBO_TM_SAVEAS, &CTapeManagerDlg::OnSelendokTmSaveAs)
	ON_CBN_SELCHANGE(IDC_COMBO_TM_DEVICES, &CTapeManagerDlg::OnCbnSelchangeTmDevices)
	ON_UPDATE_COMMAND_UI(IDC_BUTTON_TM_REMOVE, &CTapeManagerDlg::OnUpdateTmRemove)
	ON_UPDATE_COMMAND_UI(IDC_COMBO_TM_SAVEAS, &CTapeManagerDlg::OnUpdateSelendokTmSaveAs)
	ON_COMMAND(ID_TM_SELECTALL, &CTapeManagerDlg::OnTmSelectall)
	ON_COMMAND(ID_TM_UNSELECTALL, &CTapeManagerDlg::OnTmUnselectall)
	ON_WM_DESTROY()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTapeManagerDlg message handlers


BOOL CTapeManagerDlg::OnInitDialog()
{
	CBaseDialog::OnInitDialog();

	if (FillDevices() == 0)
	{
		m_comboDevices.EnableWindow(FALSE);
		CString strItem(MAKEINTRESOURCE(IDS_TAPEMNGR_CAPTURE_ERROR));
		m_comboDevices.InsertString(0, strItem);
	}
	else
	{
		if (OpenDevice())   // всегда открываем устройство ввода по умолчанию.
		{
			m_bRun = true;
			waveInStart(m_hWaveIn);
		}
	}

	m_bStatusExit = false;
	// Start status event thread
	m_thrStatus = std::thread(&CTapeManagerDlg::StatusThread, this);

	if (m_thrStatus.joinable())
	{
		m_thrStatus.detach();
	}

	InitListCtrl();
	InitComboCtrl();
	SetFolder(m_strCurrentFolder);
	FileListChanged(m_list.GetItemCount()); // разрешим/запретим некоторые кнопки
	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CTapeManagerDlg::InitListCtrl()
{
	int nPixelW = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSX);
	int nPixelH = ::GetDeviceCaps(GetDC()->GetSafeHdc(), LOGPIXELSY);
	m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT);
	m_list.InsertColumn(TMLIST::COL_TYPE, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_TYPE)), LVCFMT_LEFT, ::MulDiv(20, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_NAME, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_NAME)), LVCFMT_LEFT, ::MulDiv(100, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_ADDR, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_ADDR)), LVCFMT_RIGHT, ::MulDiv(60, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_LEN,  CString(MAKEINTRESOURCE(IDS_TAPEMNGR_LENGTH)), LVCFMT_RIGHT, ::MulDiv(60, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_TIME, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_TIME)), LVCFMT_RIGHT, ::MulDiv(40, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_WAVELEN, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_WAVELENGTH)), LVCFMT_RIGHT, ::MulDiv(80, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_CRC,  CString(MAKEINTRESOURCE(IDS_TAPEMNGR_CRC)), LVCFMT_LEFT, ::MulDiv(40, nPixelW, DEFAULT_DPIX));
	m_list.InsertColumn(TMLIST::COL_PATH, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_PATH)), LVCFMT_LEFT, ::MulDiv(300, nPixelW, DEFAULT_DPIX));
	// m_list.SetColumnWidth (COLUMN_PATH, LVSCW_AUTOSIZE_USEHEADER);
	m_imgList.Create(::MulDiv(16, nPixelW, DEFAULT_DPIX), ::MulDiv(16, nPixelH, DEFAULT_DPIY), ILC_COLOR, 3, 1);
	m_imgList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME)));
	m_imgList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_WAVE)));
	m_imgList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TAPE)));
	m_imgList.Add(LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TAPE)));
	m_list.SetImageList(&m_imgList, LVSIL_SMALL);
}


void CTapeManagerDlg::InitComboCtrl()
{
	m_comboSaveAs.InsertString(0, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_SAVE_AS)));
	m_comboSaveAs.SetItemData(0, TMCOMBO::NONE);
	CString strItem = CString(MAKEINTRESOURCE(IDS_FILEFILTER_TAP_ONLY));
	strItem = strItem.Left(strItem.Find(_T('|')));
	m_comboSaveAs.InsertString(1, strItem);
	m_comboSaveAs.SetItemData(1, TMCOMBO::SAVE_AS_MSF);
	strItem = CString(MAKEINTRESOURCE(IDS_FILEFILTER_WAV));
	strItem = strItem.Left(strItem.Find(_T('|')));
	m_comboSaveAs.InsertString(2, strItem);
	m_comboSaveAs.SetItemData(2, TMCOMBO::SAVE_AS_WAV);
	strItem = CString(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
	strItem = strItem.Left(strItem.Find(_T('|')));
	m_comboSaveAs.InsertString(3, strItem);
	m_comboSaveAs.SetItemData(3, TMCOMBO::SAVE_AS_BIN);
	CDC *pDC = m_comboSaveAs.GetDC();
	CFont *pFont = m_comboSaveAs.GetFont();
	CFont *pOldFont = pDC->SelectObject(pFont);
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	int width = 0;
	int nCount = m_comboSaveAs.GetCount();

	for (int i = 0; i < nCount; ++i)
	{
		m_comboSaveAs.GetLBText(i, strItem);
		CSize sz = pDC->GetTextExtent(strItem);
		sz.cx += tm.tmAveCharWidth;

		if (sz.cx > width)
		{
			width = sz.cx;
		}
	}

	pDC->SelectObject(pOldFont);
	m_comboSaveAs.ReleaseDC(pDC);
	width += /*::GetSystemMetrics(SM_CXVSCROLL) +*/ 2 * ::GetSystemMetrics(SM_CXEDGE);
	m_comboSaveAs.SetDroppedWidth(width);
	m_comboSaveAs.SetCurSel(0);
	m_comboSaveAs.EnableWindow(FALSE);
	UpdateData(FALSE);
}


void CTapeManagerDlg::SetFolder(const CString &strFolder)
{
	m_strCurrentFolder = strFolder;
	UpdateData(FALSE);
}



void CTapeManagerDlg::ClearBuffers()
{
	const size_t nBufferLength = GetBufferSampleLength() * SAMPLE_IO_BLOCKALIGN;

	for (auto &pos : m_arCaptureBuffers)
	{
		memset(pos.get(), 0, nBufferLength);
	}
}


LRESULT CTapeManagerDlg::OnBufferReady(WPARAM, LPARAM)
{
	const size_t nSampleLen = GetBufferSampleLength();
	DebugDraw(m_pCurrCaptureBuffer, nSampleLen);

	if (!m_bCaptureMode)
	{
		// Выделим память под все буферы
		const size_t lenInSamples = nSampleLen * CAPTURE_BUFFERS_NUM; // размер в сэмплах для всех буферов
		m_TapeReader.AllocWaveBuffer(lenInSamples);
		// Скопируем все захваченные буферы в парсер кассеты
		SAMPLE_INT *pTapeBuffer = m_TapeReader.GetWaveBuffer();
		const size_t nBufLen = nSampleLen * SAMPLE_INT_BLOCKALIGN; // длина одного буфера в байтах
		const size_t nBufInt = nSampleLen * BUFFER_CHANNELS; // приращение к указателю

		for (auto &pos : m_arCaptureBuffers)
		{
			memcpy(pTapeBuffer, pos.get(), nBufLen);
			pTapeBuffer += nBufInt;
		}

		// Попробуем найти заголовок
		TAPE_FILE_INFO tfi{};
		m_TapeReader.GetWaveFile(&tfi, true);

		// TRACE ("tfi.start_tuning = %i\n", tfi.start_tuning);
		// TRACE ("tfi.synchro_header = %i\n", tfi.synchro_header);
		if (tfi.synchro_data != -1)
		{
			// Если заголовок найден - подготовимся к захвату данных
			m_nCaptureOffset = tfi.start_tuning;
			m_nCaptureLength = 0;
			CString strName = Global::BKToUNICODE(tfi.name, 16);
			strName.Trim();
			CString strNameFormat;
			strNameFormat.Format(IDS_TAPEMNGR_NAME_FORMAT, strName, tfi.address, tfi.length);
			OutString(strNameFormat);
			// Открываем временный файл для захвата
			m_strCaptureFileName = GetTmpFilePath();

			if (m_captureFile.Open(m_strCaptureFileName.c_str(), CFile::modeCreate | CFile::modeWrite))
			{
				m_bCaptureMode = true;
				WriteBuffer(m_TapeReader.GetWaveBuffer(), lenInSamples);
			}
		}
	}
	else
	{
		// Продолжаем захват данных
		WriteBuffer(m_pCurrCaptureBuffer, int(nSampleLen));
	}

	return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// на входе размер буфера в сэмплах
bool CTapeManagerDlg::WriteBuffer(SAMPLE_INT *pBuff, size_t nSizeInSamples)
{
	if (m_captureFile.m_hFile == CFile::hFileNull)
	{
		return false;
	}

	m_nCaptureLength += nSizeInSamples;

	if (m_nCaptureLength < m_nCaptureOffset)
	{
		return true;
	}

	const size_t t = m_nCaptureLength - m_nCaptureOffset;
	const size_t nOffs = max(0, (nSizeInSamples - t));
	const size_t nSizeToWrite = min(nSizeInSamples, t);
	SAMPLE_INT *pOffs = pBuff + size_t(nOffs) * BUFFER_CHANNELS;
	m_captureFile.Write(pOffs, nSizeToWrite * SAMPLE_INT_BLOCKALIGN);

	if (IsCaptureEnd(pOffs, nSizeToWrite))
	{
		m_captureFile.Close();
		m_bCaptureMode = false;
		m_strCaptureFileNameList.AddTail(m_strCaptureFileName);
		CString strOut(MAKEINTRESOURCE(IDS_TAPEMNGR_END_OF_FILE));
		OutString(strOut);
		auto pTapeUnit = new CTapeUnit;

		if (pTapeUnit)
		{
			pTapeUnit->SetTmpFile(m_strCaptureFileName);
			InsertNewUnit(pTapeUnit);
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		}

		FileListChanged(m_list.GetItemCount());
		RetrieveInfo(reinterpret_cast<CTapeUnit *>(m_list.GetItemData(m_nTapePos)));
	}

	return true;
}


bool CTapeManagerDlg::IsCaptureEnd(SAMPLE_INT *pBuff, size_t nSizeInSamples) const
{
	double nAverageSum = 0.0;
	int nSilenceLength = 0;
	constexpr double SILENCE_THRESHOLD = 128.0 / FLOAT_BASE;

	for (size_t n = 0; n < nSizeInSamples; ++n)
	{
		const SAMPLE_INT nAverage = (n > 0) ? (nAverageSum / n) : 0;
		const SAMPLE_INT sample = ((*pBuff++) + (*pBuff++)) / 2;
		nAverageSum += sample;
		const SAMPLE_INT ds = sample - nAverage;

		if (-SILENCE_THRESHOLD <= ds && ds <= SILENCE_THRESHOLD)
		{
			nSilenceLength++;
		}
		else
		{
			nSilenceLength = 0;
		}
	}

	// TRACE ("nAverage = %i\n", nAverageSum / nSize);
	// TRACE ("Silence length = %i\n", nSilenceLength);
	if (nSilenceLength >= m_nSilenceLength)
	{
		return true;
	}

	return false;
}

void CTapeManagerDlg::OutString(const CString &str)
{
	static bool bFirstStr = true;
	CString strOut;

	if (bFirstStr)
	{
		bFirstStr = false;
		strOut = str;
	}
	else
	{
		strOut = _T("\r\n") + str;
	}

	const INT_PTR nTextLimit = SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_GETLIMITTEXT, 0, 0); //получим длину текста едит контрола
	const INT_PTR nStrLen = strOut.GetLength(); // длина новой добавляемой строки
	const LRESULT hndl = SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_GETHANDLE, 0, 0); //получим хендл буфера текста едит контрола

	if (hndl)
	{
		// получим указатель на буфер текста едит контрола правильным образом
		auto buf = reinterpret_cast<TCHAR *>(LocalLock(HLOCAL(hndl)));

		if (buf)
		{
			INT_PTR nTxtLen = lstrlen(buf); // размер текста в буфере

			// если новая добавляемая строка уже не влазит
			if (nTxtLen + nStrLen >= nTextLimit)
			{
				const INT_PTR nOversize = nTxtLen + nStrLen - nTextLimit;
				//надо удалять строки из начала, до тех пор, пока не начнёт влазить
				INT_PTR nPos = 0;
				TCHAR *p = buf;

				//найдём, сколько строк надо удалить
				while (nPos <= nOversize)
				{
					TCHAR *pf = _tcschr(p, _T('\n'));

					if (pf) //если нашлось
					{
						pf++; //передвинем за найденный символ, чтобы и его включать
						nPos += (pf - p);
						p = pf;
					}
					else
					{
						//если не нашлось, то удалить надо всё
						nPos = nTxtLen;
						break;
					}
				}

				SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_SETSEL, 0, nPos);
				SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_REPLACESEL, FALSE, NULL); //или заменить на пустую строку
				nTxtLen -= nPos; //после этого у нас строка в буфере должна уменьшиться на столько
			}

			SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_SETSEL, nTxtLen, nTxtLen);
			SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(strOut.GetBuffer()));
			SendDlgItemMessage(IDC_EDIT_TM_OUT, EM_SCROLLCARET, 0, 0);
			//GetDlgItem(IDC_EDIT_TM_OUT)->Invalidate(FALSE);
		}

		LocalUnlock(HLOCAL(hndl));
	}
}


/////////////////////////////////////////////////////////////////////////////
// на входе указатель на буфер данных. внутри 16 бит стерео
void CTapeManagerDlg::DebugDraw(SAMPLE_INT *pBuffer, size_t nSizeInSamples)
{
// #ifdef _DEBUG
	CWnd *pMonitor = GetDlgItem(IDC_PICTURE_TM_MONITOR);

	if (pMonitor)
	{
		CClientDC dc(this);
		CRect rcMonitor;
		pMonitor->GetWindowRect(&rcMonitor);
		ScreenToClient(&rcMonitor);
		rcMonitor.DeflateRect(1, 1);
		int x_offs = rcMonitor.left;
		const int y_offs = rcMonitor.top;
		const int width = rcMonitor.Width();
		const int height = rcMonitor.Height();
		const int height_half = height / 2;
		const int height_4 = height / 4;
		dc.PatBlt(x_offs, y_offs, width, height, WHITENESS);
		const int yL = y_offs + height_4;
		const int yR = y_offs + height_4 * 3;

		for (int p = 0; p < width; ++p)
		{
			size_t pos = p * nSizeInSamples / width;
			dc.MoveTo(x_offs, yL);  // ставим точку
			double dyL = pBuffer[pos++];   // dyL == -1 .. 1
			dc.LineTo(x_offs, yL - int(height_4 * dyL));  // рисуем линию от той точки до этой
			dc.MoveTo(x_offs, yR);  // ставим точку
			double dyR = pBuffer[pos];     // dyR == -1 .. 1
			dc.LineTo(x_offs, yR - int(height_4 * dyR));  // рисуем линию от той точки до этой
			x_offs++;
		}
	}

// #endif
}


fs::path CTapeManagerDlg::GetTmpFilePath() const
{
	TCHAR pTmpFilePath[_MAX_PATH];
	GetTempFileName(g_Config.m_strTapePath.c_str(), _T("raw"), 0, pTmpFilePath);
	return {pTmpFilePath};
}


void CTapeManagerDlg::RetrieveInfo(CTapeUnit *pUnit)
{
	// Set new unit for process
	if (pUnit)
	{
		{
			std::lock_guard<std::mutex> lk(m_mutModifyStatus);
			m_queueStat.push(pUnit);
		}
		m_cvStatusNew.notify_one();
	}
}


void CTapeManagerDlg::StatusThread()
{
	std::lock_guard<std::mutex> lk(m_mutThr);

	for (;;)
	{
		std::unique_lock<std::mutex> lk(m_mutModifyStatus);
		m_cvStatusNew.wait(lk, [&]()
		{
			return !m_queueStat.empty();
		});

		// всё блин красиво и по новомодному, но как блин сделать новомодный выход из потока
		// по событию - нигде не рассказывается.
		// если событие выхода
		if (m_bStatusExit)
		{
			m_queueStat.pop();
			break;    // прерываем цикл
		}

		TRACE("Tread CS IN\n");
		CTapeUnit *pTapeUnit = m_queueStat.front(); m_queueStat.pop();
		lk.unlock();

		if (pTapeUnit)
		{
			bool bSkip = (pTapeUnit->GetWaveLength() != -1);

			// Получаем информацию о нём
			if (!bSkip)
			{
				TRACE("Tread CS RetrieveInfo IN\n");
				pTapeUnit->RetrieveInfo();
				TRACE("Tread CS RetrieveInfo OUT\n");
			}

			TRACE("Tread PostMessage IN\n");
			// pTapeUnit - указатель на существующий объект, так что можно
			PostMessage(WM_INFO_READY, WPARAM(bSkip), reinterpret_cast<LPARAM>(pTapeUnit));
			TRACE("Tread PostMessage OUT\n");
		}

		TRACE("Tread CS OUT\n");
	}
}


LRESULT CTapeManagerDlg::OnInfoReady(WPARAM wParam, LPARAM lParam)
{
	bool bSkip = !!wParam;

	if (!bSkip)
	{
		auto pTapeUnit = reinterpret_cast<CTapeUnit *>(lParam);
		m_list.Invalidate(FALSE);
		std::lock_guard<std::mutex> lk(m_mutModifyStatus);

		if (!pTapeUnit->GetCRC())
		{
			CString strError;
			strError.Format(IDS_TAPEMNGR_CRC_ERROR, pTapeUnit->GetName());
			OutString(strError);
		}
	}

	if (++m_nTapePos < m_nTapeCount)
	{
		TRACE("RetriveInfo CS IN\n");
		RetrieveInfo(reinterpret_cast<CTapeUnit *>(m_list.GetItemData(m_nTapePos)));
		TRACE("RetriveInfo CS OUT\n");
	}

	return S_OK;
}


void CTapeManagerDlg::FileListChanged(int nTapeCount)
{
	m_nTapePos = 0;
	m_nTapeCount = nTapeCount;
	m_comboSaveAs.EnableWindow(nTapeCount > 0);
	GetDlgItem(IDC_BUTTON_TM_REMOVE)->EnableWindow(nTapeCount > 0);
}


void CTapeManagerDlg::InsertNewUnit(CTapeUnit *pUnit)
{
	const int nItem = m_list.GetItemCount();

	for (int n = TMLIST::COL_TYPE; n <= TMLIST::COL_PATH; n++)
	{
		LVITEM lvi;
		memset(&lvi, 0, sizeof(LVITEM));
		lvi.mask = LVIF_IMAGE | LVIF_TEXT | LVIF_PARAM;
		lvi.iItem = nItem;
		lvi.iSubItem = n;
		lvi.pszText = LPSTR_TEXTCALLBACK;
		lvi.iImage = static_cast<int>(pUnit->GetType());
		lvi.lParam = reinterpret_cast<LPARAM >(pUnit);
		m_list.InsertItem(&lvi);
	}
}


void CTapeManagerDlg::OnTmLoad()
{
	CString strFilterManager(MAKEINTRESOURCE(IDS_FILEFILTER_TAPE_LOAD));
	CFileDialog dlg(TRUE, nullptr, nullptr, OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER, strFilterManager);
	const int MaxFile = 2562;
	auto pc = std::vector<TCHAR>(MaxFile);

	if (pc.data())
	{
		ZeroMemory(pc.data(), MaxFile);
		dlg.GetOFN().nMaxFile = MaxFile;
		dlg.GetOFN().lpstrFile = pc.data();
		dlg.GetOFN().lpstrInitialDir = m_strCurrentFolder.GetString();

		if (dlg.DoModal() == IDOK)
		{
			fs::path strFilePath = dlg.GetPathName().GetString();
			POSITION pos = dlg.GetStartPosition();
			int i = 0; // счётчик добавленных файлов, не используется.

			do
			{
				strFilePath = dlg.GetNextPathName(pos).GetString();
				// Create tape unit from file
				auto pTapeUnit = new CTapeUnit;

				if (pTapeUnit)
				{
					if (pTapeUnit->SetFile(strFilePath))
					{
						InsertNewUnit(pTapeUnit);
					}

					i++;
				}
				else
				{
					g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
				}
			}
			while (pos);

			FileListChanged(m_list.GetItemCount());
			RetrieveInfo(reinterpret_cast<CTapeUnit *>(m_list.GetItemData(m_nTapePos)));
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}
}


void CTapeManagerDlg::OnTmBrowse()
{
	UpdateData(TRUE);
	CString s(MAKEINTRESOURCE(IDS_TAPEMANAGER_BROWSESELECT));
	CBrowseForFolder browse(GetSafeHwnd(), nullptr, m_strCurrentFolder, s);

	if (!browse.SelectFolder())
	{
		return;
	}

	SetFolder(browse.GetSelectedFolder());
}


void CTapeManagerDlg::UnInit()
{
	if (!m_bCanceled)
	{
		m_bCanceled = true;

		if (m_bRun)
		{
			waveInStop(m_hWaveIn);
			m_bRun = false;
		}

		CloseDevice();
		// Wait while status thread finishing
		{
			std::lock_guard<std::mutex> lk(m_mutModifyStatus);
			m_bStatusExit = true; // выставляем событие
			m_queueStat.push(nullptr); // флаг не ложного срабатывания
		}
		m_cvStatusNew.notify_one();  // пробуждаем поток
		{
			// скобки - область видимости блокировки.
			// подождём, пока мутекс не освободится,
			// а освободится он, когда поток завершится
			std::lock_guard<std::mutex> lk(m_mutThr);
		}
		DeleteUnits(false);
		POSITION pos = m_strCaptureFileNameList.GetHeadPosition();

		while (pos)
		{
			fs::path strFileName = m_strCaptureFileNameList.GetNext(pos);
			DeleteFile(strFileName.c_str());
		}
	}
}
// делаем разинициализацию во всех возможных случаях закрытия окна:
// при нажатии на крестик, при нажатии на кнопку "Закрыть" и вообще.
// но только один раз
void CTapeManagerDlg::OnDestroy()
{
	UnInit();
	CBaseDialog::OnDestroy();
}

void CTapeManagerDlg::OnClose()
{
	UnInit();
	CBaseDialog::OnClose();
}

void CTapeManagerDlg::OnCancel()
{
	UnInit();
	CBaseDialog::OnCancel();
}


void CTapeManagerDlg::OnUpdateTmRemove(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount());
}


void CTapeManagerDlg::OnTmRemove()
{
	DeleteUnits(true);
	FileListChanged(m_list.GetItemCount());
}


void CTapeManagerDlg::OnUpdateSelendokTmSaveAs(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_list.GetSelectedCount());
}


void CTapeManagerDlg::OnSelendokTmSaveAs()
{
	UpdateData(TRUE);
	int nItem = m_comboSaveAs.GetCurSel();
	CTapeUnit::TAPETYPE type = CTapeUnit::TAPETYPE::TYPE_NONE;

	if (nItem >= 0)
	{
		DWORD_PTR nSaveAs = m_comboSaveAs.GetItemData(nItem);

		switch (nSaveAs)
		{
			case TMCOMBO::SAVE_AS_BIN:
				type = CTapeUnit::TAPETYPE::TYPE_BIN;
				break;

			case TMCOMBO::SAVE_AS_WAV:
				type = CTapeUnit::TAPETYPE::TYPE_WAV;
				break;

			case TMCOMBO::SAVE_AS_MSF:
				type = CTapeUnit::TAPETYPE::TYPE_MSF;
				break;
		}
	}

	m_comboSaveAs.SetCurSel(0);
	m_comboSaveAs.ShowDropDown(FALSE);
	UpdateData(FALSE);
	SaveUnitsAs(type);
}


void CTapeManagerDlg::OnGetdispinfoTmList(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO *>(pNMHDR);

	if (pDispInfo->item.mask & LVIF_TEXT)
	{
		// pDispInfo->item.mask |= LVIF_DI_SETITEM;
		auto pTapeUnit = reinterpret_cast<CTapeUnit *>(pDispInfo->item.lParam);
		CString strUnknown = CString(MAKEINTRESOURCE(IDS_TAPEMNGR_UNKNOWN));

		switch (pDispInfo->item.iSubItem)
		{
			case TMLIST::COL_NAME:
			{
				CString strName = pTapeUnit->GetName();

				if (strName.IsEmpty())
				{
					strName = strUnknown;
				}

				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strName.GetString());
			}
			break;

			case TMLIST::COL_ADDR:
			{
				CString strAddress;

				if (pTapeUnit->GetAddress() == CTapeUnit::UNKNOWN)
				{
					strAddress = strUnknown;
				}
				else
				{
					Global::WordToOctString(pTapeUnit->GetAddress(), strAddress);
				}

				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strAddress.GetString());
			}
			break;

			case TMLIST::COL_LEN:
			{
				CString strLength;

				if (pTapeUnit->GetLength() == CTapeUnit::UNKNOWN)
				{
					strLength = strUnknown;
				}
				else
				{
					Global::WordToOctString(pTapeUnit->GetLength(), strLength);
				}

				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strLength.GetString());
			}
			break;

			case TMLIST::COL_TIME:
			{
				CString strTime = (pTapeUnit->GetTime() == CTapeUnit::UNKNOWN) ? strUnknown : Global::MsTimeToTimeString(pTapeUnit->GetTime());
				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strTime.GetString());
			}
			break;

			case TMLIST::COL_WAVELEN:
			{
				CString strLength = (pTapeUnit->GetWaveLength() == CTapeUnit::UNKNOWN) ? strUnknown : Global::IntToFileLengthString(pTapeUnit->GetWaveLength());
				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strLength.GetString());
			}
			break;

			case TMLIST::COL_CRC:
			{
				bool bCRC = pTapeUnit->GetCRC();
				CString strCRC = bCRC ? CString(MAKEINTRESOURCE(IDS_TAPEMNGR_CRC_OK)) : CString(MAKEINTRESOURCE(IDS_TAPEMNGR_CRC_FAIL));
				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strCRC.GetString());
			}
			break;

			case TMLIST::COL_PATH:
			{
				fs::path strPath = pTapeUnit->GetPath();
				_tcscpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strPath.c_str());
			}
			break;
		}
	}

	*pResult = S_OK;
}


void CTapeManagerDlg::DeleteUnits(bool bSelected)
{
	CArray<int, int> items;

	if (bSelected)
	{
		POSITION pos = m_list.GetFirstSelectedItemPosition();

		while (pos)
		{
			items.Add(m_list.GetNextSelectedItem(pos));
		}
	}
	else
	{
		const int nItems = m_list.GetItemCount();

		for (int n = 0; n < nItems; ++n)
		{
			items.Add(n);
		}
	}

	for (INT_PTR n = items.GetSize() - 1; n >= 0; n--)
	{
		const int nItem = items[n];
		auto pTapeUnit = reinterpret_cast<CTapeUnit *>(m_list.GetItemData(nItem));

		if (pTapeUnit)
		{
			delete pTapeUnit;
		}

		m_list.DeleteItem(nItem);
	}
}


void CTapeManagerDlg::SaveUnitsAs(CTapeUnit::TAPETYPE type)
{
	if (type == CTapeUnit::TAPETYPE::TYPE_NONE
	        || type == CTapeUnit::TAPETYPE::TYPE_TMP)
	{
		return;
	}

	UpdateData(TRUE);
	CString strSaveMessage = CString(MAKEINTRESOURCE(IDS_TAPEMNGR_SAVE_MESSAGE));
	OutString(strSaveMessage);
	POSITION pos = m_list.GetFirstSelectedItemPosition();

	while (pos)
	{
		const int nItem = m_list.GetNextSelectedItem(pos);
		auto pTapeUnit = reinterpret_cast<CTapeUnit *>(m_list.GetItemData(nItem));

		if (pTapeUnit->GetCRC())
		{
			if (pTapeUnit->SaveAs(m_strCurrentFolder.GetString(), type))
			{
				strSaveMessage.Format(IDS_TAPEMNGR_SAVE_SUCCEED, pTapeUnit->GetName());
			}
			else
			{
				strSaveMessage.Format(IDS_TAPEMNGR_SAVE_ERROR, pTapeUnit->GetName());
			}
		}
		else
		{
			strSaveMessage.Format(IDS_TAPEMNGR_BADCRC_SAVE, pTapeUnit->GetName());
		}

		OutString(strSaveMessage);
	}
}


void CTapeManagerDlg::OnRclickTmList(NMHDR *pNMHDR, LRESULT *pResult)
{
	CMenu menu;
	menu.LoadMenu(IDR_TAPEMANAGER_MENU);
	CMenu *pSubMenu = menu.GetSubMenu(0);
	CPoint point;
	GetCursorPos(&point);
	pSubMenu->TrackPopupMenu(TPM_RIGHTBUTTON, point.x, point.y, this);
	*pResult = S_OK;
}


void CTapeManagerDlg::OnTmSelectall()
{
	const int nCount = m_list.GetItemCount();

	for (int n = 0; n < nCount; ++n)
	{
		m_list.SetItemState(n, LVIS_SELECTED, LVIS_SELECTED);
	}
}


void CTapeManagerDlg::OnTmUnselectall()
{
	m_list.SetItemState(-1, ~LVIS_SELECTED, LVIS_SELECTED);
}

// сперва найдём и заполним комбобокс устройствами захвата
// выход: 0 - нет ни одного устройства
UINT CTapeManagerDlg::FillDevices()
{
	m_comboDevices.ResetContent();
	UINT nDevices = waveInGetNumDevs();

	if (nDevices)
	{
		m_comboDevices.InsertString(0, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_INPUT_DEVS)));
		m_comboDevices.SetItemData(0, WAVE_MAPPER);

		for (UINT i = 0; i < nDevices; ++i)
		{
			WAVEINCAPS caps;
			ZeroMemory(&caps, sizeof(WAVEINCAPS));
			MMRESULT mRes = waveInGetDevCaps(i, &caps, sizeof(WAVEINCAPS));

			if ((mRes == S_OK) && (WAVE_FORMAT_44S16 == (caps.dwFormats & WAVE_FORMAT_44S16)))
			{
				m_comboDevices.InsertString(i + 1, caps.szPname);
				m_comboDevices.SetItemData(i + 1, i);
			}
		}
	}
	else
	{
		m_comboDevices.InsertString(0, CString(MAKEINTRESOURCE(IDS_TAPEMNGR_NO_INPUTDEVS)));
		m_comboDevices.SetItemData(0, WAVE_MAPPER);
	}

	if (m_comboDevices.GetCount())
	{
		m_comboDevices.SetCurSel(0);
	}

	UpdateData(FALSE);
	return nDevices;
}

bool CTapeManagerDlg::OpenDevice()
{
	UpdateData(TRUE);
	ZeroMemory(&m_wfx, sizeof(WAVEFORMATEX));
	// m_wfx.cbSize            = 0; // extra size. sizeof (WAVEFORMATEX);
	m_wfx.wFormatTag        = WAVE_FORMAT_PCM;
	m_wfx.nSamplesPerSec    = DEFAULT_SOUND_SAMPLE_RATE;
	m_wfx.wBitsPerSample    = SAMPLE_IO_BPS;
	m_wfx.nChannels         = BUFFER_CHANNELS;
	m_wfx.nBlockAlign       = (m_wfx.wBitsPerSample >> 3) * m_wfx.nChannels;
	m_wfx.nAvgBytesPerSec   = m_wfx.nSamplesPerSec * m_wfx.nBlockAlign;
	auto uDevID = static_cast<UINT>(m_comboDevices.GetItemData(m_comboDevices.GetCurSel()));
	// В WIN10 обязательно должен быть включен доступ к микрофону.
	// Параметры->Конфиденциальность->Микрофон
	// "Доступ к микрофону для этого устройства включен"
	// И приложениям нужно разрешать доступ к микрофону,
	// иначе нифига не работает. Вот ни в какую не открывается устройство WaveIn
	MMRESULT mRes = waveInOpen(&m_hWaveIn, uDevID, &m_wfx,
	                           reinterpret_cast<DWORD_PTR>(GetSafeHwnd()),
	                           0,
	                           CALLBACK_WINDOW);

	if (mRes == MMSYSERR_NOERROR)
	{
		PrepareBuffers();
		return true;
	}

	return false;
}

LRESULT CTapeManagerDlg::OnWaveInProc(WPARAM wParam, LPARAM lParam)
{
	ProcessHeader(reinterpret_cast<WAVEHDR *>(lParam));
	return S_OK;
}


void CTapeManagerDlg::ProcessHeader(WAVEHDR *pHdr)
{
	TRACE("WaveHDR user %d\n", pHdr->dwUser);

	if (WHDR_DONE == (WHDR_DONE & pHdr->dwFlags))
	{
		CSingleLock lock(&m_csModifyBuffer, TRUE);
		std::unique_ptr<SAMPLE_INT[]> pTailBuff = std::move(m_arCaptureBuffers.front());
		m_arCaptureBuffers.pop_front();
		SAMPLE_INT *pIntBuf = pTailBuff.get();
		auto pIOBuf = reinterpret_cast<SAMPLE_IO *>(pHdr->lpData);
		const int nRecodedSample = pHdr->dwBytesRecorded / m_wfx.nBlockAlign;

		for (int i = 0; i < nRecodedSample; ++i)
		{
			*pIntBuf++ = SAMPLE_INT(*pIOBuf++) / FLOAT_BASE; // левый
			*pIntBuf++ = SAMPLE_INT(*pIOBuf++) / FLOAT_BASE; // правый
		}

		if (pHdr->dwBytesRecorded < m_wfx.nAvgBytesPerSec)
		{
			size_t nSilence = (static_cast<size_t>(m_wfx.nAvgBytesPerSec) - pHdr->dwBytesRecorded) / m_wfx.nBlockAlign;
			memset(pIntBuf, 0, nSilence * SAMPLE_IO_BLOCKALIGN);
		}

		m_arCaptureBuffers.push_back(std::move(pTailBuff));
		m_pCurrCaptureBuffer = m_arCaptureBuffers.back().get();
		lock.Unlock();
		MMRESULT mRes = waveInPrepareHeader(m_hWaveIn, pHdr, sizeof(WAVEHDR));
		mRes = waveInAddBuffer(m_hWaveIn, pHdr, sizeof(WAVEHDR));
		SendMessage(WM_BUFFER_READY);
	}
}

void CTapeManagerDlg::CloseDevice()
{
	if (m_hWaveIn)
	{
		UnPrepareBuffers();
		MMRESULT mRes = waveInClose(m_hWaveIn);
		m_hWaveIn = nullptr;
	}
}


void CTapeManagerDlg::PrepareBuffers()
{
	for (int i = 0; i < CAPTURE_BUFFERS_NUM; ++i)
	{
		ZeroMemory(&m_WHDR[i], sizeof(WAVEHDR));
		m_WHDR[i].dwBufferLength = m_wfx.nAvgBytesPerSec; // длина буфера в байтах
		m_WHDR[i].lpData = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, m_WHDR[i].dwBufferLength);
		m_WHDR[i].dwUser = i;
		MMRESULT mRes = waveInPrepareHeader(m_hWaveIn, &m_WHDR[i], sizeof(WAVEHDR));
		mRes = waveInAddBuffer(m_hWaveIn, &m_WHDR[i], sizeof(WAVEHDR));
	}

	InitBuffers();
}

void CTapeManagerDlg::UnPrepareBuffers()
{
	if (m_hWaveIn)
	{
		MMRESULT mRes = waveInStop(m_hWaveIn);

		for (auto &i : m_WHDR)
		{
			if (i.lpData)
			{
				mRes = waveInUnprepareHeader(m_hWaveIn, &i, sizeof(WAVEHDR));
				HeapFree(GetProcessHeap(), 0, i.lpData);
				ZeroMemory(&i, sizeof(WAVEHDR));
			}
		}
	}

	RemoveBuffers();
}

void CTapeManagerDlg::InitBuffers()
{
	RemoveBuffers();
	const size_t nBufferLength = GetBufferSampleLength() * SAMPLE_IO_BLOCKALIGN;

	for (int n = 0; n < CAPTURE_BUFFERS_NUM; ++n)
	{
		auto pBuff = std::make_unique<SAMPLE_INT[]>(GetBufferSampleLength() * BUFFER_CHANNELS);

		if (pBuff)
		{
			memset(pBuff.get(), 0, nBufferLength);
			m_arCaptureBuffers.emplace_back(std::move(pBuff));
		}
		else
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
			break;
		}
	}
}

void CTapeManagerDlg::RemoveBuffers()
{
	// delete all buffers memory
	m_arCaptureBuffers.clear();
}



void CTapeManagerDlg::OnCbnSelchangeTmDevices()
{
	if (m_bRun)
	{
		waveInStop(m_hWaveIn);
		m_bRun = false;
	}

	CloseDevice();

	if (OpenDevice())
	{
		m_bRun = true;
		waveInStart(m_hWaveIn);
	}
}


#endif