// Board_10_FDD.cpp: implementation of the CMotherBoard_10_FDD class.
//


#include "pch.h"
#include "Board_10_FDD.h"
#include "BKMessageBox.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CMotherBoard_10_FDD::CMotherBoard_10_FDD()
	: m_nFDDCatchAddr(-1)
	, m_nFDDExitCatchAddr(-1)
{
	m_nBKPortsIOArea = BK_PORTSIO_AREA;
	m_bBasicOn = false;

	if (!FillWndVectorPtr(0260000))
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
		throw;
	}
}

CMotherBoard_10_FDD::~CMotherBoard_10_FDD()
    = default;

MSF_CONF CMotherBoard_10_FDD::GetConfiguration()
{
	return MSF_CONF::BK1001_FDD;
}


bool CMotherBoard_10_FDD::FillWndVectorPtr(int nMemSize)
{
	// Инициализация модуля памяти, 64 кб + 16 кб доп. +8кб доп пзу
	if (CMotherBoard::FillWndVectorPtr(nMemSize))
	{
		InitMemoryValues(nMemSize);
	///	ZeroMemory(GetMainMemory() + 0200000, nMemSize - 0200000); // доп память обнулим
		return true;
	}

	return false;
}




void CMotherBoard_10_FDD::SetFDDType(BK_DEV_MPI model, bool bInit)
{
	m_fdd.SetFDDType(model);

	if (bInit)
	{
		switch (model)
		{
			case BK_DEV_MPI::STD_FDD:
			case BK_DEV_MPI::SAMARA:
				m_fdd.init_A16M_10(&m_ConfBKModel, ALTPRO_A16M_STD10_MODE);
				break;

			case BK_DEV_MPI::A16M:
			case BK_DEV_MPI::SMK512:
				m_fdd.init_A16M_10(&m_ConfBKModel, ALTPRO_A16M_START_MODE);
				break;
		}
	}

	switch (model)
	{
		case BK_DEV_MPI::A16M:
	///		m_vWindows.push_back({ _T("A16M:0"), 0100000, 040000, GetAddMemory() });
			break;

		case BK_DEV_MPI::SMK512:

			// пересоздадим массив памяти. добавим ещё 512кб
			if (FillWndVectorPtr(02220000))
			{
				for (int i = 0; i < 16; ++i) //заполним список окон
				{
			///		CString str;
			///		str.Format(_T("SMK:%02d (%s)"), i, g_arStrSMKPgNums[i].GetString());
			///		m_vWindows.push_back({ str, 0100000, 0100000, GetAddMemory() + i * 0100000});
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

BK_DEV_MPI CMotherBoard_10_FDD::GetFDDType()
{
	return m_fdd.GetFDDType();
}
/***
uint8_t *CMotherBoard_10_FDD::GetAddMemory() const
{
	return GetMainMemory() + 0220000;
}
**/
void CMotherBoard_10_FDD::OnReset()
{
	CMotherBoard::OnReset();
	m_fdd.Reset();
	// в эмуляторе флоповода примонтируем образы, прописанные в ини
	m_fdd.AttachDrives();

	if (m_fdd.GetFDDType() == BK_DEV_MPI::STD_FDD)
	{
		SetWordIndirect(0120000, 0); // хак, чтобы не зацикливаться в вечном стопе
	}
}

/*
Запись данных в системные регистры БК0010
    вход: num - адрес регистра (177660, 177716 и т.п.)
          pDst - адрес в массиве, куда сохраняется новое значение
          src - записываемое значение.
          bByteOperation - флаг операции true - байтовая, false - словная
*/
bool CMotherBoard_10_FDD::SetSystemRegister(uint16_t addr, uint16_t src, UINT dwFlags)
{
	if (CMotherBoard::SetSystemRegister(addr, src, dwFlags))
	{
		return true;
	}

	bool bRet = true;

	switch (addr & 0177776)
	{
		case 0177130:

			// обрабатываем только при словной операции или байтовой, но с младшим байтом
			// хотя я подозреваю, что здесь как обычно, невозможно обратиться к регистру по нечётному адресу
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

					// сохраняем копию
					for (int i = 012, j = 0; i <= 017; ++i, ++j)
					{
						m_MemMapBasic[j].bReadable = m_MemoryMap[i].bReadable;
						m_MemMapBasic[j].bWritable = m_MemoryMap[i].bWritable;
						m_MemMapBasic[j].nBank = m_MemoryMap[i].nBank;
						m_MemMapBasic[j].nOffset = m_MemoryMap[i].nOffset;
					}
				}
			}

			// тут надо добавить манипуляцию ПЗУ бейска но только для А16М
			if ((m_fdd.GetFDDType() == BK_DEV_MPI::A16M) && (GetAltProExtCode() & 010))
			{
				if (!m_bBasicOn)
				{
					m_bBasicOn = true;

					// фокус покус. подключаем бейсик. и похер, че там было
					for (int i = 012; i <= 017; ++i)
					{
						m_MemoryMap[i].bReadable = true;
						m_MemoryMap[i].bWritable = false;
						m_MemoryMap[i].nBank = i;
						m_MemoryMap[i].nOffset = i << 12;
					}
				}
			}
			else
			{
				if (m_bBasicOn) // чтобы зазря циклы не гонять, сделаем флаг.
				{
					for (int i = 012, j = 0; i <= 017; ++i, ++j)
					{
						m_MemoryMap[i].bReadable = m_MemMapBasic[j].bReadable;
						m_MemoryMap[i].bWritable = m_MemMapBasic[j].bWritable;
						m_MemoryMap[i].nBank = m_MemMapBasic[j].nBank;
						m_MemoryMap[i].nOffset = m_MemMapBasic[j].nOffset;
					}

					m_bBasicOn = false;
				}
			}

		// и при этом это же передаётся в контроллер, т.е. по сути оно сперва передаётся в контроллер, а потом происходят вышеописанные действия
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

bool CMotherBoard_10_FDD::GetSystemRegister(uint16_t addr, void *pDst, UINT dwFlags)
{
	if (CMotherBoard::GetSystemRegister(addr, pDst, dwFlags))
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

bool CMotherBoard_10_FDD::InitMemoryModules()
{
	bool bRes = CMotherBoard::InitMemoryModules();

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
				RomNameIndex = IDS_INI_FDR;
				break;
		}

		// Load FDD driver
		bRes = LoadRomModule(RomNameIndex, A16M_ROM_10);
		// по адресу 160000 - ПЗУ дисковода
		m_MemoryMap[016].bReadable = bRes;
		m_MemoryMap[016].bWritable = false;
		m_MemoryMap[016].nBank = A16M_ROM_10;
		m_MemoryMap[016].nOffset = A16M_ROM_10 << 12;

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

void CMotherBoard_10_FDD::MemoryManager()
{
	CMotherBoard::MemoryManager();

	switch (m_fdd.GetFDDType())
	{
		case BK_DEV_MPI::SMK512:
			// а потом переустановим в соответствии с настройками SMK
			m_fdd.SMK512_MemManager_10(m_MemoryMap, &m_ConfBKModel);
			break;

		case BK_DEV_MPI::A16M:
		case BK_DEV_MPI::STD_FDD:
		case BK_DEV_MPI::SAMARA:
			// а потом переустановим в соответствии с настройками А16М
			m_fdd.A16M_MemManager_10(m_MemoryMap, &m_ConfBKModel);
			break;
	}
}



bool CMotherBoard_10_FDD::Interception()
{
	if (CMotherBoard::Interception())
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
				uint16_t m = GetAltProMode();

				if (m == 0 || m == 040)
				{
					return false;
				}

				break;
			}

			case BK_DEV_MPI::SMK512:
			{
				uint16_t m = GetAltProMode();

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
 0000000 - 64кб - основной массив памяти 32кб ОЗУ + 32кб ПЗУ или чего-либо в зависимости от конфигурации
 0200000 - 8кб ПЗУ прошивки контроллера
 0220000 - 16кб ОЗУ А16М
 0260000 - конец.
 для смк512 карта памяти выглядит так
 0000000 - 64кб - основной массив памяти 32кб ОЗУ + 32кб ПЗУ или чего-либо в зависимости от конфигурации
 0200000 - 8кб ПЗУ прошивки контроллера
 0220000 - 512кб ОЗУ СМК
 участок 0200000-0260000 сохраняется методом SetBlockExt16Memory
 участок 0260000-02220000 - сохраняется методом SetBlockMemorySMK512, тут сохраняются не 512 кб, как можно
 было ожидать, а 512кб - 16 кб
*/

bool CMotherBoard_10_FDD::RestoreMemory(CMSFManager &msf)
{
	if (CMotherBoard::RestoreMemory(msf))
	{ /***
		if (msf.IsLoad())
		{
			if (!msf.GetBlockExt16Memory(GetMainMemory() + 0200000))
			{
				return false;
			}

			if (m_fdd.GetFDDType() == BK_DEV_MPI::SMK512)
			{
				if (!msf.GetBlockMemorySMK512(GetMainMemory() + 0260000))
				{
					return false;
				}
			}
		}
		else
		{
			if (!msf.SetBlockExt16Memory(GetMainMemory() + 0200000))
			{
				return false;
			}

			if (m_fdd.GetFDDType() == BK_DEV_MPI::SMK512)
			{
				if (!msf.SetBlockMemorySMK512(GetMainMemory() + 0260000))
				{
					return false;
				}
			}
		}
*/
		return true;
	}

	return false;
}
