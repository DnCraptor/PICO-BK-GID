#include "pch.h"
#include "BKImgFile.h"
#include <cwchar>

#pragma warning(disable:4996)

constexpr auto BLOCK_SIZE = 512;

CBKImgFile::CBKImgFile()
	: m_o(false)
	, m_nCylinders(83)
	, m_nHeads(2)
	, m_nSectors(10)
{
}

CBKImgFile::CBKImgFile(const fs::path &strName, const bool bWrite)
	: m_o(false)
	, m_nCylinders(83)
	, m_nHeads(2)
	, m_nSectors(10)
{
	Open(strName, bWrite);
}


CBKImgFile::~CBKImgFile()
{
	Close();
}

bool CBKImgFile::Open(const fs::path &pathName, const bool bWrite)
{
	if (m_o) {
		Close();
	}
	std::string strName = pathName.string();
#ifdef FD_RAW
	bool bNeedFDRaw = false;
	bool bFloppy = false; // флаг, true - обращение к реальному флопику, false - к образу
	bool bFDRaw = false;
	DWORD dwRet;
	// сперва проверим, что за имя входного файла
	if (strName.length() >= 4)
	{
		std::string st = strName.substr(0, 4);
		if (st == "\\\\.\\")   // если начинается с этого
		{
			bFloppy = true;     // то обращаемся к реальному дисководу
			st = strName.substr(4, 5);

			if (st == L"fdraw") // а если ещё и нужен fdraw
			{
				// то проверим, установлен ли драйвер
				bNeedFDRaw = true;
				HANDLE hFD = CreateFile(L"\\\\.\\fdrawcmd", GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

				if (hFD != INVALID_HANDLE_VALUE) // да, что-то установлено
				{
					DWORD dwVersion = 0;
					DeviceIoControl(hFD, IOCTL_FDRAWCMD_GET_VERSION, nullptr, 0, &dwVersion, sizeof(dwVersion), &dwRet, nullptr);
					CloseHandle(hFD);

					if (dwVersion && (HIWORD(dwVersion) == HIWORD(FDRAWCMD_VERSION)))
					{
						bFDRaw = true;
						// если dwVersion == 0, то "fdrawcmd.sys не установлен, смотрите: http://simonowen.com/fdrawcmd/\n";
						// если версия не совпадает, то тоже плохо ("Установленный fdrawcmd.sys не совместим с этой программой.\n");
					}
				}
			}
		}
	}
	if (bFloppy)
	{
		if (bNeedFDRaw) // если нужен fdraw
		{
			if (bFDRaw) // и он установлен
			{
				// открываем реальное устройство
				m_h = CreateFile(pathName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

				if (m_h != INVALID_HANDLE_VALUE) // если оно открылось, т.е. физически лоступно
				{
					// устанавливаем параметры доступа
					int DISK_DATARATE = FD_RATE_250K;          // 2 is 250 kbit/sec
					DeviceIoControl(m_h, IOCTL_FD_SET_DATA_RATE, &DISK_DATARATE, sizeof(DISK_DATARATE), nullptr, 0, &dwRet, nullptr);
					DeviceIoControl(m_h, IOCTL_FD_RESET, nullptr, 0, nullptr, 0, &dwRet, nullptr);
					bRet = true;
				}
			}
			// если не установлен то ничего.
		}
		else
		{
			// если fdraw не нужен, то просто открываем как есть
			// открываем реальное устройство
			m_h = CreateFile(pathName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

			if (m_h != INVALID_HANDLE_VALUE) // если оно открылось, т.е. физически доступно
			{
				bRet = true;
			}
		}
	}
	else
	{
#endif
		// открываем образ
		BYTE mode = bWrite ? FA_READ | FA_OPEN_APPEND : FA_READ;
		FRESULT r = f_open(&m_f, pathName.c_str(), mode);
		if (r == FR_OK)	{
			m_o = true;
		}
#ifdef FD_RAW
	}
#endif
	if (m_o) {
		m_strName = pathName;
	}
	return m_o;
}

void CBKImgFile::Close()
{
	if (m_o)
	{
		f_close(&m_f);
		m_o = false;
	}
}

void CBKImgFile::SetGeometry(const uint8_t c, const uint8_t h, const uint8_t s)
{
	if (c != 0xff)
	{
		m_nCylinders = c;
	}
	if (h != 0xff)
	{
		m_nHeads = h;
	}
	if (s != 0xff)
	{
		m_nSectors = s;
	}
}

CBKImgFile::CHS CBKImgFile::ConvertLBA(const UINT lba) const
{
	CHS ret;
	// превратить смещение в байтах в позицию в c:h:s;
	// поскольку формат строго фиксирован: c=80 h=2 s=10, а перемещения предполагаются только по границам секторов
	UINT t = static_cast<UINT>(m_nHeads) * static_cast<UINT>(m_nSectors);
	ret.c = lba / t;
	t = lba % t;
	ret.h = static_cast<uint8_t>(t / static_cast<UINT>(m_nSectors));
	ret.s = static_cast<uint8_t>(t % static_cast<UINT>(m_nSectors)) + 1;
	return ret;
}

UINT CBKImgFile::ConvertCHS(const CHS chs) const
{
	return ConvertCHS(chs.c, chs.h, chs.s);
}

UINT CBKImgFile::ConvertCHS(const uint8_t c, const uint8_t h, const uint8_t s) const
{
	UINT lba = (static_cast<UINT>(c) * static_cast<UINT>(m_nHeads) + static_cast<UINT>(h)) * static_cast<UINT>(m_nSectors) + static_cast<UINT>(s) - 1;
	return lba;
}

bool CBKImgFile::ReadCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors)
{
	if (m_o)
	{
		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;
		if (FR_OK == f_lseek(&m_f, lba))
		{
			lba = numSectors * BLOCK_SIZE;
			UINT br;
			return (FR_OK == f_read(&m_f, buffer, lba, &br));
		}
		return false;
	}
	return false;
}

bool CBKImgFile::WriteCHS(void *buffer, const uint8_t cyl, const uint8_t head, const uint8_t sector, const UINT numSectors)
{
	if (m_o)
	{
		UINT lba = ConvertCHS(cyl, head, sector) * BLOCK_SIZE;
		if (FR_OK == f_lseek(&m_f, lba))
		{
			lba = numSectors * BLOCK_SIZE;
			UINT br;
			return (FR_OK == f_read(&m_f, buffer, lba, &br));
		}
		return false;
	}
	return false;
}

bool CBKImgFile::ReadLBA(void *buffer, const UINT lba, const UINT numSectors)
{
	if (m_o)
	{
		if (FR_OK == f_lseek(&m_f, lba))
		{
			UINT size = numSectors * BLOCK_SIZE;
			UINT br;
			return (FR_OK == f_read(&m_f, buffer, lba, &br));
		}
		return false;
	}
	return false;
}

bool CBKImgFile::WriteLBA(void *buffer, const UINT lba, const UINT numSectors)
{
	if (m_o)
	{
		if (FR_OK == f_lseek(&m_f, lba))
		{
			UINT size = numSectors * BLOCK_SIZE;
			UINT br;
			return (FR_OK == f_read(&m_f, buffer, lba, &br));
		}
		return false;
	}
	return false;
}

long CBKImgFile::GetFileSize() const
{
	if (m_o)
	{
		return f_size(&m_f);
	}
	return -1;
}

bool CBKImgFile::SeekTo00()
{
	if (m_o)
	{
		return (FR_OK == f_lseek(&m_f, 0));
	}
	return false;
}

bool CBKImgFile::IsFileOpen() const
{
	return m_o;
}
