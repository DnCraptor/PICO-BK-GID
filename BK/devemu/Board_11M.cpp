﻿// Board_11M.cpp: implementation of the CMotherBoard_11M class.
//


#include "pch.h"
#include "Board_11M.h"
#include "BKMessageBox.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CMotherBoard_11M::CMotherBoard_11M(BK_DEV_MPI model)
	: CMotherBoard(model)
{
	m_nStartAddr = 0140000;
	m_nBKPortsIOArea = BK_PURE_PORTSIO_AREA;
	m_fdd.init_A16M_11M(&m_ConfBKModel, ALTPRO_A16M_STD11_MODE);
	SetCPUBaseFreq(CPU_SPEED_BK11); // частота задаётся этой константой

	if (!FillWndVectorPtr(0700000))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		throw;
	}

	m_reg177716out_mem = 0;
}

CMotherBoard_11M::~CMotherBoard_11M()
    = default;

MSF_CONF CMotherBoard_11M::GetConfiguration()
{
	return MSF_CONF::BK11M;
}

bool CMotherBoard_11M::FillWndVectorPtr(int nMemSize)
{
	if (CMotherBoard::FillWndVectorPtr(nMemSize))
	{/***
		ZeroMemory(GetMainMemory() + 0640000, nMemSize - 0640000); // доп память обнулим
		CString str; //заполним список окон

		for (int i = 0; i < 8; ++i)
		{
			str.Format(_T("БК11:%d"), i);
			int addr = (i == 0) ? 0 : -1;
			m_vWindows.push_back({ str, addr, 040000, GetMainMemory() + i * 040000 });
		}
**/
		return true;
	}

	return false;
}


void CMotherBoard_11M::StopInterrupt()
{
	// нажали на кнопку стоп
	if (!(m_reg177716out_tap & 010000)) // если кнопка СТОП не заблокирована
	{
		m_cpu.SetIRQ1();
	}
}

void CMotherBoard_11M::UnStopInterrupt()
{
	// отжали кнопку стоп
	if (!(m_reg177716out_tap & 010000)) // если кнопка СТОП не заблокирована
	{
		m_cpu.UnsetIRQ1();
	}
}


int CMotherBoard_11M::GetScreenPage() const
{
	return ((m_reg177662out & 0100000) ? 6 : 5);
}

void CMotherBoard_11M::OnReset()
{
	CMotherBoard::OnReset();
	ChangePalette(); // эта функция используется и при начальной инициализации, и при выполнении
	// команды RESET, что не совсем правильно. при RESET регистр палитр не затрагивается.
}


void CMotherBoard_11M::Set177716RegMem(uint16_t w)
{
	constexpr uint16_t mask = 077433;
	m_reg177716out_mem = (m_reg177716out_mem & ~mask) | (w & mask);
	MemoryManager();
}

void CMotherBoard_11M::Set177716RegTap(uint16_t w)
{
	constexpr uint16_t mask = 010344;
	m_reg177716out_tap = (m_reg177716out_tap & ~mask) | (w & mask);
	m_pSpeaker->SetData(m_reg177716out_tap);
}

// вход: pg0 - номер страницы в окно 0, если -1, то не устанавливать
//       pg1 - номер страницы в окно 1, если -1, то не устанавливать
void CMotherBoard_11M::SetMemPages(int pg0, int pg1)
{
	if (pg0 >= 0)
	{
		uint32_t nBnk = (pg0 & 7) << 2;

		for (int i = 04, j = 0; i <= 07; ++i, ++j)
		{
			m_MemoryMap[i].nBank = nBnk + j;
			m_MemoryMap[i].nOffset = m_MemoryMap[i].nBank << 12;
			m_MemoryMap[i].nTimingCorrection = RAM_TIMING_CORR_VALUE_D;
		}
	}

	if (pg1 >= 0)
	{
		uint32_t nBnk = (pg1 & 7) << 2;

		for (int i = 010, j = 0; i <= 013; ++i, ++j)
		{
			m_MemoryMap[i].bReadable = true;
			m_MemoryMap[i].bWritable = true;
			m_MemoryMap[i].nBank = nBnk + j;
			m_MemoryMap[i].nOffset = m_MemoryMap[i].nBank << 12;
			m_MemoryMap[i].nTimingCorrection = RAM_TIMING_CORR_VALUE_D;
		}
	}
}

void CMotherBoard_11M::RestoreMemPages()
{
	MemoryManager();
}

/*
Запись данных в системные регистры БК0011М
    вход: num - адрес регистра (177660, 177716 и т.п.)
          src - записываемое значение.
          bByteOperation - флаг операции true - байтовая, false - словная
*/
bool CMotherBoard_11M::SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags)
{
	switch (addr & 0177776)
	{
		case 0177660:
			/*
			177660
			Регистр состояния клавиатуры.
			(0100)бит 6 -- маскирование прерывание от клавиатуры,
			    "0" -- разрешено прерывание по вектору 060 либо 0274.
			    Прерывание вызывается при появлении "1" в бите 7. Начальное состояние: "0".
			    Доступен по чтению и по записи
			(0200)бит 7 -- готовность: "1" -- в регистре данных клавиатуры (177662) готов код нажатой клавиши.
			    Устанавливается при нажатии на клавишу, сбрасывается при чтении регистра данных клавиатуры.
			    Начальное состояние: "1".
			    Доступен только по чтению.
			другие биты: "0". Доступны только по чтению.
			*/
		{
			const uint16_t mask = dwFlags & GSSR_INTERNAL ? 0300 : 0100;

			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377;

				if (addr & 1)
				{
					src = (src << 8);
				}
			}

			// сбрасываем используемые биты и устанавливаем их значения из слова, которое записываем.
			// остальные биты - которые не используются - остаются нетронутыми.
			m_reg177660 = (m_reg177660 & ~mask) | (src & mask);
		}

		return true;

		case 0177662:
		{
			constexpr uint16_t mask = 0147400;

			/*
			177662
			Регистр данных клавиатуры.
			    биты 0-6: код клавиши. Доступ только по чтению.
			биты 08-11: палитра
			(040000)бит 14: разрешение прерывания по (внешнему) таймеру (50 Гц),
			    "0" -- прерывание разрешено,
			    "1" -- таймер отключён.
			    Доступен только по записи.
			(0100000)бит 15: переключение буферов экрана 0 - №5, 1 - №6
			*/
			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377; // работаем с младшим байтом

				if (addr & 1)
				{
					src <<= 8; // работаем со старшим байтом
				}
			}

			// сбрасываем используемые биты и устанавливаем их значения из слова, которое записываем.
			// остальные биты - которые не используются - остаются нетронутыми.
			m_reg177662out = (m_reg177662out & ~mask) | (src & mask);
		}

		ChangePalette();
		return true;

		case 0177664:
			/*
			177664
			Регистр скроллинга. Доступен по записи и чтению.
			    биты 0-7: смещение скроллинга, строк. Начальное значение -- 0330.
			(01000)бит 9: сокращённый режим экрана, "0" -- сокращённый (1/4 экрана, старшие адреса),
			    "1" -- полный экран 256 строк.
			*/
		{
			constexpr uint16_t mask = 01377;

			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377;
				/*
				оказалось, что в этот регистр не работает байтовая запись.
				чтение байта работает, но записывается всегда слово.
				а при байтовой операции, старший байт просто теряется.
				*/
			}

			m_reg177664 = (m_reg177664 & ~mask) | (src & mask);
		}

		return true;

		case 0177714:

			/*177714
			    Регистр параллельного программируемого порта ввода вывода - два регистра, входной по чтению и выходной по записи.
			    из выходного нельзя ничего прочитать т.к., читается оно из входного,
			    во входной невозможно ничего записать, т.к. записывается оно в выходной.
			*/
			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377; // работаем с младшим байтом

				if (addr & 1)
				{
					src <<= 8; // работаем со старшим байтом
				}
			}

			if (m_pAYSnd)
			{
				if (dwFlags & GSSR_BYTEOP)
				{
					m_pAYSnd->write_data(LOBYTE(src));
				}
				else
				{
					m_pAYSnd->write_address(src);
				}
			}

			if (m_pCovox)
			{
				m_pCovox->SetData(src);
			}

			if (m_pMenestrel)
			{
				m_pMenestrel->SetData(src);
			}

			m_reg177714out = src;

			if (g_Config.m_bICLBlock) // если включён блок нагрузок
			{
				// данные записанные в выходной порт передаются во входной
				m_reg177714in = src;
			}
			else if (g_Config.m_bMouseMars)
			{
				m_pParent->GetScreen()->SetMouseStrobe(src);
				m_reg177714in = m_pParent->GetScreen()->GetMouseStatus();
			}

			return true;

		case 0177716:

			/*
			177716
			Системный регистр. Внешний регистр 1 (ВР1, SEL1) процессора ВМ1, регистр начального пуска.
			как и 177714 состоит из двух регистров, раздельных по чтению и по записи
			По чтению:
			(004)бит 2: признак записи в системный регистр. Устанавливается в "1" при
			    любой записи в регистр, сбрасывается в "0" по окончании операции чтения из регистра.
			(040)бит 5: данные с магнитофона, "0" - логический 0, "1" - логическая 1
			(100)бит 6: индикатор нажатия клавиши, установлен в "0" если нажата клавиша клавиатуры, "1" если нет нажатых клавиш.
			(200)бит 7: константа "1", указывающая на отсутствие в системе команд расширенной арифметики
			    биты 8-15: адрес начального пуска, 140000 (БК-0011), младший байт при этом игнорируется
			    остальные биты не используются, "0".
			По записи:
			    бит 11==0
			(004)бит 2: данные на магнитофон. Начальное состояние "0".
			(040)бит 5: данные на магнитофон. Начальное состояние "0".
			(100)бит 6: данные на магнитофон и на пьезодинамик. Начальное состояние "0".
			(200)бит 7: включение двигателя магнитофона, "1" -- стоп, "0" -- пуск. Начальное состояние "1".
			(010000)бит 12: разрешение прерывания по клавише СТОП, 0 - разрешено, 1 - запрещено
			    биты 0,1,3,4 не используются, "0".
			    бит 11==1
			(001)бит 0: подключение страницы №8 пзу в окно №1
			(002)бит 1: подключение страницы №9 пзу в окно №1
			(010)бит 3: подключение страницы №10 пзу в окно №1
			(020)бит 4: подключение страницы №11 пзу в окно №1
			    биты 8-10: номер страницы озу в окно №1
			    биты 12-14: номер страницы озу в окно №0
			*/
			if (dwFlags & GSSR_BYTEOP)
			{
				src &= 0377; // работаем с младшим байтом

				if (addr & 1)
				{
					src <<= 8; // работаем со старшим байтом
				}
			}

			if (src & 04000)
			{
				Set177716RegMem(src);
			}
			else
			{
				Set177716RegTap(src);
			}

			// В БК 2й разряд SEL1 фиксирует любую запись в этот регистр, взводя триггер D9.1 на неограниченное время, сбрасывается который любым чтением этого регистра.
			if (!(dwFlags & GSSR_INTERNAL))
			{
				m_reg177716in |= 4;
			}

			return true;
	}

	return false;
}

bool CMotherBoard_11M::LoadRomModule11(int iniRomNameIndex, int bank)
{
	CString strName = g_Config.GetRomModuleName(iniRomNameIndex);
	// здесь делается жёсткая зависимость от номера банка. При смене структуры данных тут тоже всё надо будет переделывать.
	const int n = (bank - BRD_11_BASIC2_BNK) / 2; // номер бита в битовой маске

	if (Global::isEmptyUnit(strName)) // если там пусто
	{
		m_ConfBKModel.nROMPresent &= ~(1 << n); // сбрасываем бит
		return true; // Там ПЗУ не предусмотрено, но это не ошибка
	}

	m_ConfBKModel.nROMPresent |= (1 << n); // устанавливаем бит бит
	fs::path strPath = g_Config.m_strROMPath / strName.GetString();
	CFile file;

	if (file.Open(strPath.c_str(), CFile::modeRead))
	{
		auto len = static_cast<UINT>(file.GetLength());

		if (len > 020000) // размер ПЗУ не должен быть больше 8кб
		{
			len = 020000;
		}

		UINT readed = file.Read(&m_psram, static_cast<size_t>(bank) << 12, len);
		file.Close();

		if (readed == len)
		{
			return true;
		}
	}
	else
	{
		CString strError(MAKEINTRESOURCE(IDS_ERROR_CANTOPENFILE));
		g_BKMsgBox.Show(strError + _T('\'') + CString(strPath.c_str()) + _T('\''), MB_OK | MB_ICONSTOP);
	}

	m_ConfBKModel.nROMPresent &= ~(1 << n); // сбрасываем бит, потому что не смогли правильно прочитать ПЗУ
	return false;
}

bool CMotherBoard_11M::InitMemoryModules()
{
	m_ConfBKModel.nROMPresent = 0;
	LoadRomModule11(IDS_INI_BK11M_RE2_328_BASIC2, BRD_11_BASIC2_BNK);
	LoadRomModule11(IDS_INI_BK11M_RE2_329_BASIC3, BRD_11_BASIC3_BNK);
	LoadRomModule11(IDS_INI_BK11M_RE2_327_BASIC1, BRD_11_BASIC1_BNK);
	LoadRomModule11(IDS_INI_BK11M_RE2_325_EXT, BRD_11_EXT_BNK);
	bool bBos = LoadRomModule(IDS_INI_BK11M_RE2_324_BOS, BRD_11_BOS_BNK);
	// Подгружаем опциональные страницы 012 и 013.
	LoadRomModule11(IDS_INI_BK11_RE_OPT_PG12_1, BRD_11_PG12_1_BNK);
	LoadRomModule11(IDS_INI_BK11_RE_OPT_PG12_2, BRD_11_PG12_2_BNK);
	LoadRomModule11(IDS_INI_BK11_RE_OPT_PG13_1, BRD_11_PG13_1_BNK);
	LoadRomModule11(IDS_INI_BK11_RE_OPT_PG13_2, BRD_11_PG13_2_BNK);

	// и проинициализируем карту памяти
	for (int i = 0; i < 020; ++i)
	{
		m_MemoryMap[i].bReadable = true;
		m_MemoryMap[i].bWritable = true;
		m_MemoryMap[i].nBank = i;
		m_MemoryMap[i].nOffset = i << 12;
		m_MemoryMap[i].nTimingCorrection = RAM_TIMING_CORR_VALUE_D;
	}

	// по адресу 140000 - основное ПЗУ БОС
	for (int i = 014, bnk = BRD_11_BOS_BNK; i <= 015; ++i, ++bnk)
	{
		m_MemoryMap[i].bReadable = bBos;
		m_MemoryMap[i].bWritable = false;
		m_MemoryMap[i].nBank = bnk;
		m_MemoryMap[i].nOffset = bnk << 12;
		m_MemoryMap[i].nTimingCorrection = ROM_TIMING_CORR_VALUE;
	}

	// по адресу 160000 - по умолчанию - модуль МСТД
	bool bTst = LoadRomModule(IDS_INI_BK11M_RE2_330_MSTD, BRD_11_TST_BNK);

	for (int i = 016, bnk = BRD_11_TST_BNK; i <= 017; ++i, ++bnk)
	{
		m_MemoryMap[i].bReadable = bTst;
		m_MemoryMap[i].bWritable = false;
		m_MemoryMap[i].nBank = bnk;
		m_MemoryMap[i].nOffset = bnk << 12;
		m_MemoryMap[i].nTimingCorrection = ROM_TIMING_CORR_VALUE;
	}

	MemoryManager();
	return true;
}

void CMotherBoard_11M::InitMemoryValues(int nMemSize) {
	uint16_t val = 0;
	int n = 8;
	int flag = 8;
	for (size_t i = 0; i < nMemSize / 2; ++i) {
		m_psram.set16(i << 1, val);
		if (--n <= 0) {
			n = 8;
			if (--flag > 0)	{
				val = ~val;
			} else {
				flag = 8;
			}
		}
	}
}

const int CMotherBoard_11M::m_arPageNums[8] = { 1, 5, 2, 3, 4, 7, 0, 6 }; // перекодировка БКшной кодировки номеров страниц в нормальную.
const int CMotherBoard_11M::m_arPageCodes[8] = { 6, 0, 2, 3, 4, 1, 7, 5 }; // перекодировка номеров страниц в коды.

void CMotherBoard_11M::MemoryManager()
{
	const uint16_t mem = m_reg177716out_mem;
	const int nRAMPageWnd0 = (mem >> 12) & 7; // номер страницы ОЗУ в окне 0
	const int nRAMPageWnd1 = (mem >> 8) & 7; // номер страницы ОЗУ в окне 1
	int nROMinWnd1 = 0; // номер страницы ПЗУ в окне 1, если 0 - нет там ПЗУ, там ОЗУ

	// стр 8 пзу
	if (mem & 1)
	{
		nROMinWnd1 = 8;
	}
	// стр 9 пзу
	else if (mem & 2)
	{
		nROMinWnd1 = 9;
	}
	// стр 10 пзу
	else if (mem & 010)
	{
		nROMinWnd1 = 10;
	}
	// стр 11 пзу
	else if (mem & 020)
	{
		nROMinWnd1 = 11;
	}

	// теперь надо в соответствии с этим поправить карту памяти
	// сперва разберёмся с окном 0, там может быть только ОЗУ, в том числе и экран
	uint32_t nBnk = m_arPageNums[nRAMPageWnd0] << 2;

	for (int i = 04, j = 0; i <= 07; ++i, ++j)
	{
		m_MemoryMap[i].nBank = nBnk + j;
		m_MemoryMap[i].nOffset = m_MemoryMap[i].nBank << 12;
		m_MemoryMap[i].nTimingCorrection = RAM_TIMING_CORR_VALUE_D;
	}

	// теперь разберёмся с окном 1, там может быть как пзу, так и ОЗУ, а так же может быть пусто, для страниц 10 и 11
	if (nROMinWnd1)
	{
		const int n = (nROMinWnd1 - 8) << 1;
		nBnk = nROMinWnd1 << 2;

		for (int i = 010, j = 0; i <= 013; ++i, ++j)
		{
			m_MemoryMap[i].bReadable = !!(m_ConfBKModel.nROMPresent & (1 << (n + (j >> 1))));
			m_MemoryMap[i].bWritable = false;
			m_MemoryMap[i].nBank = nBnk + j;
			m_MemoryMap[i].nOffset = m_MemoryMap[i].nBank << 12;
			m_MemoryMap[i].nTimingCorrection = ROM_TIMING_CORR_VALUE;
		}
	}
	else
	{
		nBnk = m_arPageNums[nRAMPageWnd1] << 2;

		for (int i = 010, j = 0; i <= 013; ++i, ++j)
		{
			m_MemoryMap[i].bReadable = true;
			m_MemoryMap[i].bWritable = true;
			m_MemoryMap[i].nBank = nBnk + j;
			m_MemoryMap[i].nOffset = m_MemoryMap[i].nBank << 12;
			m_MemoryMap[i].nTimingCorrection = RAM_TIMING_CORR_VALUE_D;
		}
	}
}

void CMotherBoard_11M::ChangePalette()
{
	m_pParent->GetScreen()->SetPalette(m_reg177662out >> 8);
}

bool CMotherBoard_11M::Interception()
{
	if (CMotherBoard::Interception())
	{
		return true;
	}

	switch (GetRON(CCPU::REGISTER::PC) & 0177776)
	{
		case 0155170:
			return EmulateSaveTape11();

		case 0155604:
			return EmulateLoadTape11();
	}

	return false;
}

bool CMotherBoard_11M::RestoreState(CMSFManager &msf, HBITMAP hScreenshot)
{
	if (RestorePreview(msf, hScreenshot))
	{
		if (RestoreConfig(msf))
		{
			if (RestoreRegisters(msf))
			{
				if (RestoreMemoryMap(msf))
				{
					if (RestoreMemory(msf))
					{
						if (msf.IsLoad())
						{
							ChangePalette();
						}

						return true;
					}
				}
			}
		}
	}

	return false;
}

bool CMotherBoard_11M::RestoreMemory(CMSFManager &msf)
{
	if (msf.IsLoad())
	{
		if (msf.GetBlockBaseMemory11M(GetMainMemory()))
		{
			return true;
		}
	}
	else
	{
		if (msf.SetBlockBaseMemory11M(GetMainMemory()))
		{
			return true;
		}
	}

	return false;
}


constexpr auto BK_NAMELENGTH = 16;   // Максимальная длина имени файла на БК - 16 байтов
constexpr auto BK_BMB11M = 042602;
constexpr auto BK_BMB10_ADDRESS = 2;
constexpr auto BK_BMB10_LENGTH = 4;
constexpr auto BK_BMB10_NAME = 6;
constexpr auto BK_BMB11_PAGES = BK_BMB10_NAME + BK_NAMELENGTH;
constexpr auto BK_BMB11_FOUND_ADDRESS = BK_BMB11_PAGES + 2;
constexpr auto BK_BMB11_FOUND_LENGTH = BK_BMB11_FOUND_ADDRESS + 2;
constexpr auto BK_BMB11_FOUND_NAME = BK_BMB11_FOUND_LENGTH + 2;
constexpr auto BK_BMB11M_ERRADDR = 042657;
constexpr auto BK_BMB11M_RNOFLAG = 042661;
constexpr auto BK_BMB11M_FICTFLAG = 042662;
constexpr auto BK_BMB11M_USRPG = 042666;
constexpr auto BK_BMB11M_CRCADDR = 042672;
constexpr auto BK_BMB11M_FICTADDR = 0164;

bool CMotherBoard_11M::EmulateLoadTape11()
{
	#ifdef EMULATE_TAPE
	if (g_Config.m_bEmulateLoadTape && ((GetWord(0154614) == 04767) && (GetWord(0154616) == 0165536)))
	{
		bool bFileSelect = false; // что делать после диалога выбора
		bool bCancelSelectFlag = false; // флаг для усложнения алгоритма
		bool bError = false;
		bool bIsDrop = false;
		CString strBinExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
		uint16_t fileAddr = 0;
		uint16_t abp = BK_BMB11M;
		// получим код для подключения страниц
		uint16_t nPageCode = GetWord(BK_BMB11M_USRPG);
		uint16_t nSysPageCode = 054002;
		fs::path strFileName;
		// Внутренняя загрузка на БК
		uint8_t bkName[BK_NAMELENGTH] = { 0 };  // Максимальная длина имени файла на БК - BK_NAMELENGTH байтов
		uint8_t bkFoundName[BK_NAMELENGTH] = { 0 };

		if (!m_pParent->isBinFileNameSet())
		{
			// если загружаем не через драг-н-дроп, то действуем как обычно
			fileAddr = GetWord(abp + BK_BMB10_ADDRESS);   // второе слово - адрес загрузки/сохранения

			// Подбираем 16 байтовое имя файла из блока параметров
			for (uint16_t c = 0; c < BK_NAMELENGTH; ++c)
			{
				bkName[c] = GetByte(abp + BK_BMB10_NAME + c);
			}

			strFileName = Global::BKToUNICODE(bkName, BK_NAMELENGTH).Trim().GetString(); // тут надо перекодировать  имя файла из кои8 в unicode

			if (!strFileName.empty()) // если имя файла не пустое
			{
				strFileName += strBinExt.GetString(); // добавляем стандартное расширение для бин файлов,
				// чтобы не рушить логику следующих проверок
			}
		}
		else
		{
			// если загружаем через драг-н-дроп, то берём имя оттуда
			strFileName = m_pParent->GetStrBinFileName();
			bIsDrop = true;
		}

		if (strFileName.empty())
		{
			// Если имя пустое - то покажем диалог выбора файла.
			bFileSelect = false;
l_SelectFile:
			// Запомним текущую директорию
			fs::path strCurDir = fs::current_path();
			fs::current_path(g_Config.m_strBinPath);
			CString strFilterBin(MAKEINTRESOURCE(IDS_FILEFILTER_BIN));
			CFileDialog dlg(TRUE, nullptr, nullptr,
			                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
			                strFilterBin, m_pParent->GetScreen()->GetBackgroundWindow());
			// Зададим начальной директорией директорию с Bin файлами
			dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str();

			if (dlg.DoModal() == IDCANCEL)
			{
				// Если нажали Отмену, установим ошибку во втором байте блока параметров
				bError = true; // случилась ошибка
			}
			else
			{
				strFileName = dlg.GetPathName().GetString(); // вот выбранный файл
				g_Config.m_strBinPath = strFileName.parent_path();
				// имя файла надо бы как-то поместить в 0352..0372 иначе некоторые глюки наблюдаются
				CString strFound = dlg.GetFileTitle();
				Global::UNICODEtoBK(strFound, bkFoundName, BK_NAMELENGTH, true); // вот из этого массива будем потом помещать

				if (bFileSelect)
				{
					// тут надо проверить тот ли файл нам подсовывают.
					CString strFindEx = Global::BKToUNICODE(bkName, BK_NAMELENGTH); // с расширением
					fs::path p = strFindEx.GetString();
					CString strFind = p.stem().c_str(); // без расширения

					if (!bIsDrop) // только если не дроп. там не с чем сравнивать
					{
						if (strFind.CollateNoCase(strFound) != 0 && strFindEx.CollateNoCase(strFound) != 0)
						{
							int result = g_BKMsgBox.Show(IDS_BK_ERROR_WRONGFILE, MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);

							if (result == IDYES)
							{
								goto l_SelectFile;
							}
							else
							{
								bError = true;
								bCancelSelectFlag = true; // не будем ничего делать, сразу выйдем.
							}
						}
					}
				}
			}

			// восстановим текущую директорию
			fs::current_path(strCurDir);
		}
		else    // Если имя не пустое
		{
			// If Saves Default flag is set loading from User directory
			// Else Load from Binary files directory
			fs::path strCurrentPath = g_Config.m_bSavesDefault ? g_Config.m_strSavesPath : g_Config.m_strBinPath;
			CString s = strFileName.c_str();
			Global::SetSafeName(s); // перекодируем небезопасные символы на безопасные
			strFileName = s.GetString();

			// сейчас узнаем, нужно ли нам добавлять расширение .bin, или наоборот, удалять
			if (CString(strFileName.extension().c_str()).CollateNoCase(strBinExt) == 0)
			{
				// у файла уже есть расширение бин
				if (!fs::exists(strCurrentPath / strFileName)) // если нету файла с расширением.
				{
					fs::path str = strFileName.stem(); // удаляем расширение

					if (fs::exists(strCurrentPath / str)) // если есть файл без расширения
					{
						strFileName = str; // оставим файл без расширения
					}
					else
					{
						// нет файла ни с расширением, ни без расширения
						SetByte(BK_BMB11M_ERRADDR, 1);
						bError = true;
					}
				}
			}
			else
			{
				// у файла нету расширения бин
				if (!fs::exists(strCurrentPath / strFileName)) // если нету файла без расширения
				{
					fs::path str = strFileName;
					str += strBinExt; // добавляем стандартное расширение для бин файлов.

					if (fs::exists(strCurrentPath / str))  // если есть файл с расширением
					{
						strFileName = str;
					}
					else
					{
						// нет файла ни с расширением, ни без расширения
						SetByte(BK_BMB11M_ERRADDR, 1);
						bError = true;
					}
				}
			}

			CString str = strFileName.stem().c_str();
			Global::UNICODEtoBK(str, bkFoundName, BK_NAMELENGTH, true);
			strFileName = strCurrentPath / strFileName;
		}

		if (!bCancelSelectFlag)
		{
			CFile file;
			uint16_t readAddr = 0;
			uint16_t readSize = 0;

			// Загрузим файл, если ошибок не было
			if (!bError && file.Open(strFileName.c_str(), CFile::modeRead))
			{
				file.Read(&readAddr, sizeof(readAddr));   // Первое слово в файле - адрес загрузки
				file.Read(&readSize, sizeof(readSize));   // Второе слово в файле - длина
				// сплошь и рядом встречаются .bin файлы. у которых во втором слове указана длина
				// меньше, чем длина файла - 4. Это другой формат бин, у которого в начале указывается
				// адрес, длина, имя файла[16], массив[длина], КС - контрольная сумма в конце
				uint16_t filesz = (file.GetLength() < 65536) ? static_cast<uint16_t>(file.GetLength()) : 65535;
				bool bIsCRC = false;

				if (readSize == filesz - 4)
				{
					bIsCRC = false;
				}
				else if (readSize == filesz - 6)
				{
					bIsCRC = true;
				}
				else if (readSize == filesz - 22)
				{
					bIsCRC = true;
					file.Read(bkFoundName, BK_NAMELENGTH); // прочитаем оригинальное имя файла
				}
				else
				{
					// всё равно загрузим. Пусть не бин
					file.Seek(0, CFile::SeekPosition::begin);
					readAddr = 0;
					readSize = filesz;
				}

				SetWord(abp + BK_BMB11_FOUND_ADDRESS, readAddr);
				SetWord(abp + BK_BMB11_FOUND_LENGTH, readSize);

				if (bkFoundName[0])
				{
					// копируем прочитанное имя файла
					for (uint16_t i = 0; i < BK_NAMELENGTH; ++i)
					{
						SetByte(abp + BK_BMB11_FOUND_NAME + i, bkFoundName[i]);
					}
				}
				else
				{
					// копируем прочитанное имя файла
					for (uint16_t i = 0; i < BK_NAMELENGTH; ++i)
					{
						SetByte(abp + BK_BMB11_FOUND_NAME + i, bkName[i]);
					}
				}

				if (fileAddr == 0)
				{
					fileAddr = readAddr;
				}

				bool bReadNameOnly = !!GetByte(BK_BMB11M_RNOFLAG); //прочитать только имя файла
				bool bFictive = !!GetByte(BK_BMB11M_FICTFLAG); //фиктивное чтение

				if (!bReadNameOnly)
				{
					SetWord(0177716, nPageCode); // подключаем нужные страницы ОЗУ
					DWORD cs = 0; // подсчитаем контрольную сумму

					// Загрузка по адресу fileAddr
					for (int i = 0; i < readSize; ++i)
					{
						uint8_t val;
						file.Read(&val, sizeof(val));

						if (bFictive)
						{
							SetByte(BK_BMB11M_FICTADDR, val);
						}
						else
						{
							SetByte(fileAddr++, val);
						}

						cs += uint16_t(val);

						if (cs & 0xffff0000)
						{
							cs++;
							cs &= 0xffff;
						}
					}

					SetWord(0177716, nSysPageCode); // подключаем системные страницы
					uint16_t crc;

					if (bIsCRC && file.Read(&crc, sizeof(crc)) == sizeof(uint16_t))
					{
						if (crc != LOWORD(cs))
						{
							SetByte(BK_BMB11M_ERRADDR, 2);
							cs = crc;
						}
					}

					// а иначе, мы не знаем какая должна быть КС. поэтому считаем, что файл априори верный
					file.Close();
					// Заполняем системные ячейки, как это делает emt 36
					uint16_t loadcrc = LOWORD(cs);
					SetWord(BK_BMB11M_CRCADDR, loadcrc); // сохраним контрольную сумму
				}

				SetRON(CCPU::REGISTER::PC, 0155646); // выходим туда.
			}
			else
			{
				// При ошибке покажем сообщение
				CString strError;
				strError.Format(IDS_CANT_OPEN_FILE_S, strFileName);
				int result = g_BKMsgBox.Show(strError, MB_ICONWARNING | MB_YESNOCANCEL | MB_DEFBUTTON2);

				switch (result)
				{
					case IDNO:
						// если не хотим останавливаться, то пойдём на диалог, и поищем файл в другом месте.
						bError = false;
						SetByte(BK_BMB11M_ERRADDR, 0);
						bFileSelect = true; // включим проверку на неподходящее имя.
						goto l_SelectFile;

					// если отмена - просто выходим с заданным кодом ошибки
					case IDYES:
						// если хотим остановиться - зададим останов.
						BreakCPU();
						[[fallthrough]];

					// если отмена - просто выходим с заданным кодом ошибки
					case IDCANCEL:
						SetByte(BK_BMB11M_ERRADDR, 4);
						break;
				}

				SetRON(CCPU::REGISTER::PC, 0154762); // выходим на обработку ошибок.
			}
		}
		else
		{
			SetRON(CCPU::REGISTER::PC, 0154762); // выходим на обработку ошибок.
		}

		// Refresh keyboard
		m_pParent->SendMessage(WM_RESET_KBD_MANAGER, 0); // и почистим индикацию управляющих клавиш в статусбаре
		return true; // сэмулировал
	}
    #endif
	return false;
}

bool CMotherBoard_11M::EmulateSaveTape11()
{
	#ifdef EMULATE_TAPE
	if (g_Config.m_bEmulateSaveTape && ((GetWord(0154614) == 04767) && (GetWord(0154616) == 0165536)))
	{
		bool bError = false;
		uint16_t abp = BK_BMB11M; // вот тут блок параметров
		// получим код для подключения страниц
		uint16_t nPageCode = GetWord(BK_BMB11M_USRPG);
		uint16_t nSysPageCode = 054002;
		// получим адрес блока параметров из R1 (BK emt 36)
		uint16_t fileAddr = GetWord(abp + BK_BMB10_ADDRESS);  // второе слово - адрес загрузки/сохранения
		uint16_t fileSize = GetWord(abp + BK_BMB10_LENGTH);  // третье слово - длина файла (для загрузки может быть 0)
		uint16_t cs = GetWord(BK_BMB11M_CRCADDR);// заберём подсчитанную КС

		if (fileSize)
		{
			uint8_t bkName[BK_NAMELENGTH] {};  // Максимальная длина имени файла на БК - 16 байтов

			// Подбираем 16 байтовое имя файла из блока параметров
			for (uint16_t c = 0; c < BK_NAMELENGTH; ++c)
			{
				bkName[c] = GetByte(abp + BK_BMB10_NAME + c);
			}

			fs::path strFileName = Global::BKToUNICODE(bkName, BK_NAMELENGTH).Trim().GetString(); // тут надо перекодировать  имя файла из кои8 в unicode

			// Если имя пустое
			if (strFileName.empty())
			{
				// Покажем диалог сохранения
				CFileDialog dlg(FALSE, nullptr, nullptr,
				                OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_EXPLORER,
				                nullptr, m_pParent->GetScreen()->GetBackgroundWindow());
				dlg.GetOFN().lpstrInitialDir = g_Config.m_strBinPath.c_str();

				if (dlg.DoModal() == IDOK)
				{
					// Получим имя
					strFileName = dlg.GetPathName().GetString();
					g_Config.m_strBinPath = strFileName.parent_path();
					CString s = strFileName.c_str();
					Global::UNICODEtoBK(s, bkName, BK_NAMELENGTH, true);
				}
				else
				{
					// Если отмена - установим флаг ошибки
					bError = true;
				}
			}
			else
			{
				CString strBinExt(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
				// Если имя не пустое
				CString s = strFileName.c_str();
				Global::SetSafeName(s); // перекодируем небезопасные символы на безопасные
				strFileName = s.GetString();
				strFileName = (g_Config.m_bSavesDefault ? g_Config.m_strSavesPath : g_Config.m_strBinPath) // подставляем соответствующий путь
				              / strFileName;
				strFileName += strBinExt; // добавляем стандартное расширение для бин файлов.
			}

			CFile file;

			// Save file array if no errors
			if (!bError && (file.Open(strFileName.c_str(), CFile::modeCreate | CFile::modeWrite)))
			{
				// Записываем заголовок бин файла
				file.Write(&fileAddr, sizeof(fileAddr)); // слово адреса
				file.Write(&fileSize, sizeof(fileSize)); // слово длины

				if (g_Config.m_bUseLongBinFormat)
				{
					file.Write(bkName, BK_NAMELENGTH); // имя файла
				}

				SetWord(0177716, nPageCode); // подключаем нужные страницы ОЗУ

				for (int i = 0; i < fileSize; ++i)
				{
					uint8_t val = GetByte(fileAddr++);
					file.Write(&val, sizeof(val));
				}

				SetWord(0177716, nSysPageCode); // подключаем системные страницы

				if (g_Config.m_bUseLongBinFormat)
				{
					file.Write(&cs, sizeof(uint16_t)); // контрольная сумма
				}

				file.Close();
			}
		}

		if (bError)
		{
			SetRON(CCPU::REGISTER::PC, 0154762); // выходим на обработку ошибок.
		}
		else
		{
			SetRON(CCPU::REGISTER::PC, 0155312); // выходим туда.
		}

		// Refresh keyboard
		m_pParent->SendMessage(WM_RESET_KBD_MANAGER, 0); // и почистим индикацию управляющих клавиш в статусбаре
		return true; // сэмулировали
	}
    #endif
	return false;
}
