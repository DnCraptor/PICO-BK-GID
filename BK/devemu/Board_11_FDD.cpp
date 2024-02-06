// Board_11_FDD.cpp: implementation of the CMotherBoard_11_FDD class.
//


#include "pch.h"
#include "Board_11_FDD.h"
#include "BKMessageBox.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CMotherBoard_11_FDD::CMotherBoard_11_FDD()
	: m_nFDDCatchAddr(-1)
	, m_nFDDExitCatchAddr(-1)
{
	m_nBKPortsIOArea = BK_PORTSIO_AREA;
}

CMotherBoard_11_FDD::~CMotherBoard_11_FDD()
    = default;

MSF_CONF CMotherBoard_11_FDD::GetConfiguration()
{
	return MSF_CONF::BK11_FDD;
}

void CMotherBoard_11_FDD::SetFDDType(BK_DEV_MPI model, bool bInit)
{
	m_fdd.SetFDDType(model);

	if (bInit)
	{
		switch (model)
		{
			case BK_DEV_MPI::STD_FDD:
			case BK_DEV_MPI::SAMARA:
				m_fdd.init_A16M_11M(&m_ConfBKModel, ALTPRO_A16M_STD11_MODE);
				break;

			case BK_DEV_MPI::A16M:
			case BK_DEV_MPI::SMK512:
				m_fdd.init_A16M_11M(&m_ConfBKModel, ALTPRO_A16M_START_MODE);
				break;
		}
	}

	switch (model)
	{
		case BK_DEV_MPI::A16M:
			m_vWindows.push_back({ _T("A16M:0"), 0100000, 040000, GetMainMemory() + 0220000 });
			break;

		case BK_DEV_MPI::SMK512:

			// пересоздадим массив памяти. добавим ещё 512кб
			if (FillWndVectorPtr(02640000))
			{
				for (int i = 0; i < 16; ++i) //заполним список окон
				{
					CString str;
					str.Format(_T("SMK:%02d (%s)"), i, g_arStrSMKPgNums[i].GetString());
					m_vWindows.push_back({ str, 0100000, 0100000, GetAddMemory() + i * 0100000 });
				}
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
				throw;
			}

			break;
	}
}

BK_DEV_MPI CMotherBoard_11_FDD::GetFDDType()
{
	return m_fdd.GetFDDType();
}

uint8_t *CMotherBoard_11_FDD::GetAddMemory() const
{
	return GetMainMemory() + 0640000;
}

void CMotherBoard_11_FDD::OnReset()
{
	CMotherBoard_11M::OnReset();
	m_fdd.Reset();
	// в эмуляторе флоповода примонтируем образы, прописанные в ини
	m_fdd.AttachDrives();
}

/*
Запись данных в системные регистры БК0011
    вход: num - адрес регистра (177660, 177716 и т.п.)
          pDst - адрес в массиве, куда сохраняется новое значение
          src - записываемое значение.
          bByteOperation - флаг операции true - байтовая, false - словная
*/
bool CMotherBoard_11_FDD::SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags)
{
	if (CMotherBoard_11::SetSystemRegister(addr, src, dwFlags))
	{
		return true;
	}

	bool bRet = true;

	switch (addr & 0177776)
	{
		case 0177130:

			// обрабатываем только при словной операции или байтовой, но с младшим байтом
			if ((addr & 1) == 0)
			{
				if (dwFlags & GSSR_BYTEOP) // если операция байтовая, то старший байт теряем
				{
					src &= 0377;
				}

				// тут надо добавить манипуляцию дозу
				if (m_fdd.Change_AltPro_Mode(&m_ConfBKModel, src))
				{
					MemoryManager();
				}
			}

		// тут не нужен break, т.к. надо выполнить передачу команды контроллеру FDD
		case 0177132:
			if (dwFlags & GSSR_BYTEOP)
			{
				m_fdd.SetByte(addr, LOBYTE(src));
			}
			else
			{
				m_fdd.SetWord(addr, src);
			}

			break;

		default:
		{
			// если у нас SMK512, то у него есть регистры HDD
			bRet = m_fdd.WriteHDDRegisters(addr, ~src);
		}
	}

	return bRet;
}

bool CMotherBoard_11_FDD::GetSystemRegister(uint16_t addr, void *pDst, UINT dwFlags)
{
	if (CMotherBoard_11::GetSystemRegister(addr, pDst, dwFlags))
	{
		return true;
	}

	auto pDst_W = reinterpret_cast<uint16_t *>(pDst);
	auto pDst_B = reinterpret_cast<uint8_t *>(pDst);

	if (dwFlags & GSSR_INTERNAL)
	{
		uint16_t w = 0;

		switch (addr & 0177776)
		{
			case 0177130:
				w = m_fdd.GetStateDebug();
				break;

			case 0177132:
				w = m_fdd.GetDataDebug();
				break;

			default:
				// если у нас SMK512, то у него есть регистры HDD
				bool bRet = m_fdd.ReadHDDRegistersInternal(addr, w);

				if (bRet)
				{
					if (dwFlags & GSSR_BYTEOP)
					{
						*pDst_B = LOBYTE(w);
					}
					else
					{
						*pDst_W = w;
					}
				}

				return bRet;
		}

		if (m_fdd.GetFDDType() != BK_DEV_MPI::STD_FDD && (GetAltProExtCode() & 4)) // бит 2 отключает эти порты по чтению
		{
			return false;
		}

		if (dwFlags & GSSR_BYTEOP)
		{
			*pDst_B = LOBYTE(w);
		}
		else
		{
			*pDst_W = w;
		}

		return true;
	}

	switch (addr & 0177776)
	{
		case 0177130:
		case 0177132:
			if (m_fdd.GetFDDType() != BK_DEV_MPI::STD_FDD && (GetAltProExtCode() & 4)) // бит 2 отключает эти порты по чтению
			{
				return false;
			}

			if (dwFlags & GSSR_BYTEOP)
			{
				m_fdd.GetByte(addr, pDst_B);
			}
			else
			{
				m_fdd.GetWord(addr, pDst_W);
			}

			return true;
	}

	// если у нас SMK512, то у него есть регистры HDD
	uint16_t w;
	bool bRet = m_fdd.ReadHDDRegisters(addr, w);

	if (bRet)
	{
		if (dwFlags & GSSR_BYTEOP)
		{
			*pDst_B = LOBYTE(w);
		}
		else
		{
			*pDst_W = w;
		}
	}

	return bRet;
}

bool CMotherBoard_11_FDD::InitMemoryModules()
{
	bool bRes = CMotherBoard_11::InitMemoryModules();

	if (bRes)
	{
		int RomNameIndex;

		switch (m_fdd.GetFDDType())
		{
			case BK_DEV_MPI::SAMARA:
				RomNameIndex = IDS_INI_FDR_SAMARA;
				break;

			case BK_DEV_MPI::SMK512:
				RomNameIndex = IDS_INI_FDR_SMK512;
				break;

			case BK_DEV_MPI::A16M:
				RomNameIndex = IDS_INI_FDR_A16M;
				break;

			case BK_DEV_MPI::STD_FDD:
			default:
				RomNameIndex = IDS_INI_FDR253;
				break;
		}

		// Load FDD driver
		bRes = LoadRomModule(RomNameIndex, A16M_ROM_11M);
		// по адресу 160000 - ПЗУ дисковода
		m_MemoryMap[016].bReadable = bRes;
		m_MemoryMap[016].bWritable = false;
		m_MemoryMap[016].nBank = A16M_ROM_11M;
		m_MemoryMap[016].nOffset = A16M_ROM_11M << 12;

		if (0167 == GetWordIndirect(0160016))
		{
			// 326 прошивка
			m_nFDDCatchAddr = 0160372;
			m_nFDDExitCatchAddr = 0161564;
		}
		else
		{
			// 253 прошивка, там нету перехода к эмулятору EIS/FIS
			m_nFDDCatchAddr = 0160422;
			m_nFDDExitCatchAddr = 0161540;
		}

		// дополнительная проверка на правильную прошивку.
		if (GetWordIndirect(m_nFDDCatchAddr)       == 0010663
		        && GetWordIndirect(m_nFDDCatchAddr + 2)   == 0000050
		        && GetWordIndirect(m_nFDDCatchAddr + 4)   == 0106763
		        && GetWordIndirect(m_nFDDCatchAddr + 6)   == 0000052
		        && GetWordIndirect(m_nFDDCatchAddr + 010) == 0012700
		        && GetWordIndirect(m_nFDDCatchAddr + 012) == 0000004
		        && GetWordIndirect(m_nFDDCatchAddr + 014) == 0011063
		        && GetWordIndirect(m_nFDDCatchAddr + 016) == 0000046
		        && GetWordIndirect(m_nFDDCatchAddr + 020) == 0010710
		        && GetWordIndirect(m_nFDDCatchAddr + 022) == 0062710
		   )
		{
			// всё ок
		}
		else
		{
			// нестандартная прошивка
			m_nFDDCatchAddr = 0177777;
			m_nFDDExitCatchAddr = 0177777;
		}
	}

	return true;
}

void CMotherBoard_11_FDD::MemoryManager()
{
	CMotherBoard_11::MemoryManager(); // сперва восстановим память в окне 1

	// по адресу 140000 - надо восстанавливать основное ПЗУ БОС
	for (int i = 014, bnk = BRD_11_BOS_BNK; i <= 015; ++i, ++bnk)
	{
		m_MemoryMap[i].bReadable = true;
		m_MemoryMap[i].bWritable = false;
		m_MemoryMap[i].nBank = bnk;
		m_MemoryMap[i].nOffset = bnk << 12;
		m_MemoryMap[i].nTimingCorrection = ROM_TIMING_CORR_VALUE;
	}

	switch (m_fdd.GetFDDType())
	{
		case BK_DEV_MPI::SMK512:
			// а потом переустановим в соответствии с настройками SMK
			m_fdd.SMK512_MemManager_11M(m_MemoryMap, &m_ConfBKModel);
			break;

		case BK_DEV_MPI::A16M:
			// а потом переустановим в соответствии с настройками А16М
			m_fdd.A16M_MemManager_11M(m_MemoryMap, &m_ConfBKModel);
			break;
	}
}


// тут возможно надо будет сделать перехват магнитофона
bool CMotherBoard_11_FDD::Interception()
{
	if (CMotherBoard_11::Interception())
	{
		return true;
	}

	if ((GetRON(CCPU::REGISTER::PC) & 0177776) == m_nFDDCatchAddr)
	{
		// для контроллеров альтпро тут нужно сделать исключение. если вместо ПЗУ - ОЗУ.
		switch (m_fdd.GetFDDType())
		{
			case BK_DEV_MPI::A16M:
			{
				const uint16_t m = GetAltProMode();

				if (m == 0 || m == 040)
				{
					return false;
				}

				break;
			}

			case BK_DEV_MPI::SMK512:
			{
				const uint16_t m = GetAltProMode();

				if (m == 0 || m == 040 || m == 20 || m == 120 || m == 0100)
				{
					return false;
				}

				break;
			}
		}

		/*
		эмуляция работы с дисководом. Если мы работаем стандартными методами,
		а если нестандартными - то у нас есть полная эмуляция работы с дисководом через порты.
		*/
		if (g_Config.m_bEmulateFDDIO)
		{
			m_fdd.EmulateFDD(this);
			SetRON(CCPU::REGISTER::PC, m_nFDDExitCatchAddr);
			return true;
		}
	}

	return false;
}

/*
 Тут вот какая ситуация.
 для а16м карта памяти выглядит так
 0000000 - 128кб - ОЗУ БК
 0400000 - 80кб ПЗУ БК, в том числе и ПЗУ прошивки контроллера FDD, А16М или СМК
 0640000 - 16кб ОЗУ А16М - интегрирован для упрощения алгоритмов в основную память БК
 0700000 - конец

 для смк512 карта памяти выглядит так
 0000000 - 128кб - ОЗУ БК
 0400000 - 80кб ПЗУ БК, в том числе и ПЗУ прошивки контроллера FDD, А16М или СМК
 0640000 - 512кб ОЗУ СМК
 участок 0000000-0700000 сохраняется методом SetBlockBaseMemory11M
 участок 0700000-02640000 - сохраняется методом SetBlockMemorySMK512, тут сохраняются не 512 кб, как можно
 было ожидать, а 512кб - 16 кб
*/

bool CMotherBoard_11_FDD::RestoreMemory(CMSFManager &msf)
{
	if (msf.IsLoad())
	{
		SetFDDType(g_Config.m_BKFDDModel, false);
	}

	if (CMotherBoard_11::RestoreMemory(msf))
	{
		if (m_fdd.GetFDDType() == BK_DEV_MPI::SMK512)
		{
			if (msf.IsLoad())
			{
				if (!msf.GetBlockMemorySMK512(GetMainMemory() + 0700000))
				{
					return false;
				}
			}
			else
			{
				if (!msf.SetBlockMemorySMK512(GetMainMemory() + 0700000))
				{
					return false;
				}
			}
		}

		return true;
	}

	return false;
}
