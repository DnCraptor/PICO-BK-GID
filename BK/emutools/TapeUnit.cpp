// TapeUnit.cpp: implementation of the CTapeUnit class.
//


#include "pch.h"
#include "resource.h"
#include "TapeUnit.h"

#include "Config.h"
#include "Tape.h"
#include "MSFManager.h"
#include "BKMessageBox.h"

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction


CTapeUnit::CTapeUnit() :
	m_nType(TAPETYPE::TYPE_NONE),
	m_nAddress(UNKNOWN),
	m_nLength(UNKNOWN),
	m_nMSTime(UNKNOWN),
	m_nWaveLength(UNKNOWN),
	m_bCRC(false)
{
}

CTapeUnit::CTapeUnit(CTapeUnit &tapeUnit)
{
	m_nType = tapeUnit.m_nType;
	m_nAddress = tapeUnit.m_nAddress;
	m_nLength = tapeUnit.m_nLength;
	m_nMSTime = tapeUnit.m_nMSTime;
	m_nWaveLength = tapeUnit.m_nWaveLength;
	m_bCRC = tapeUnit.m_bCRC;
	m_strName = tapeUnit.m_strName;
	m_strPath = tapeUnit.m_strPath;
}

CTapeUnit::~CTapeUnit()
    = default;


CTapeUnit &CTapeUnit::operator= (CTapeUnit &tapeUnit)
    = default;


bool CTapeUnit::SetFile(const fs::path &strPath)
{
	bool bRet = false;

	// получим информацию о файле - таким образом проверяется существует он или нет
	if (fs::exists(strPath)) // если true - то файл существует.
	{
		// теперь определим тип файла
		m_strName.Empty();
		m_bCRC = false;

		if (IsFileWave(strPath))
		{
			// это PCM Wave
			m_nType = TAPETYPE::TYPE_WAV;
			bRet = true;
		}
		else if (IsFileMSF(strPath))
		{
			// это MSF, т.е. tap файл
			m_nType = TAPETYPE::TYPE_MSF;
			bRet = true;
		}
		else if (IsFileBin(strPath))
		{
			// это BIN, обычный БК бин
			m_nType = TAPETYPE::TYPE_BIN;
			m_bCRC = true;
			bRet = true;
		}

		if (bRet) // если формат определили
		{
			m_nAddress = UNKNOWN;
			m_nLength = UNKNOWN;
			m_nMSTime = UNKNOWN;
			m_nWaveLength = UNKNOWN;
			m_strPath = strPath;
		}
	}

	return bRet;
}


bool CTapeUnit::SetTmpFile(const fs::path &strPath)
{
	// получим информацию о файле
	if (fs::exists(strPath)) // если файл существует.
	{
		// то зададим тип файла
		m_nType = TAPETYPE::TYPE_TMP;     // файл типа TMP, для захвата Wave
		m_nAddress = UNKNOWN;
		m_nLength = UNKNOWN;
		m_nMSTime = UNKNOWN;
		m_nWaveLength = UNKNOWN;
		m_bCRC = false;
		m_strName.Empty();
		m_strPath = strPath;
		return true;
	}

	return false; // файла нет - делать нечего
}


bool CTapeUnit::IsFileWave(const fs::path &strPath) const
{
	CFile waveFile;

	if (!waveFile.Open(strPath.c_str(), CFile::modeRead))
	{
		return false;
	}

	WaveHeader waveHeader{};

	if (waveFile.Read(&waveHeader, sizeof(WaveHeader)) != sizeof(WaveHeader))
	{
		return false;
	}

	if (waveHeader.riffTag != RIFF_TAG || waveHeader.fmtTag != FMT_TAG)
	{
		return false;
	}

	WAVEFORMATEX wfx;

	if (waveFile.Read(&wfx, waveHeader.fmtSize) != waveHeader.fmtSize)
	{
		return false;
	}

	wfx.cbSize = 0;
//  if (wfx.nAvgBytesPerSec != 44100 || wfx.nBlockAlign != 1
//          || wfx.nChannels != 1 || wfx.nSamplesPerSec != 44100
//          || wfx.wBitsPerSample != 8 || wfx.wFormatTag != WAVE_FORMAT_PCM)
//  {
//      return false;
//  }
	DataHeader dataHeader{};

	for (;;)
	{
		if (waveFile.Read(&dataHeader, sizeof(DataHeader)) != sizeof(DataHeader))
		{
			return false;
		}

		if (dataHeader.dataTag == DATA_TAG)
		{
			break;
		}

		waveFile.Seek(dataHeader.dataSize, CFile::SeekPosition::current);
	}

	return true;
}


bool CTapeUnit::IsFileMSF(const fs::path &strPath) const
{
	CMSFManager msf;

	if (!msf.CheckFile(strPath, true))
	{
		return false;
	}

	if (!msf.OpenFile(strPath, true))
	{
		return false;
	}

	MSF_BLOCK_INFO bi{};

	if (!msf.FindBlock(MSF_BLOCKTYPE_WAVE, &bi))
	{
		return false;
	}

	return true;
}


bool CTapeUnit::IsFileBin(const fs::path &strPath) const
{
	if (fs::exists(strPath))
	{
		CFile binFile;

		if (binFile.Open(strPath.c_str(), CFile::modeRead))
		{
			uint16_t length;

			// читаем первое слово - это адрес, его надо пропустить
			if (binFile.Read(&length, sizeof(length)) != sizeof(length))
			{
				return false;
			}

			// читаем второе слово - это длина
			if (binFile.Read(&length, sizeof(length)) != sizeof(length))
			{
				return false;
			}

			auto fsize = fs::file_size(strPath);

			if (fsize < 65536)
			{
				if (length != fsize - 4) // если не простой бин
				{
					if (length != fsize - 6) // если не усложнённый
					{
						if (length != fsize - 22) // и не сложный
						{
							return false;
						}
					}
				}

				return true;
			}
		}
	}

	return false;
}


bool CTapeUnit::RetrieveInfo()
{
	bool bRet = false;
	auto pTape = std::make_unique<CTape>();

	if (pTape)
	{
		TAPE_FILE_INFO tfi{};

		switch (m_nType)
		{
			case TAPETYPE::TYPE_BIN:
				// Open binary file
				bRet = pTape->LoadBinFile(m_strPath, &tfi);
				break;

			case TAPETYPE::TYPE_WAV:
				// Open wave file
				bRet = pTape->LoadWaveFile(m_strPath);
				break;

			case TAPETYPE::TYPE_MSF:
				// Open MSF file
				bRet = pTape->LoadMSFFile(m_strPath);
				break;

			case TAPETYPE::TYPE_TMP:
				// Open TMP file
				bRet = pTape->LoadTmpFile(m_strPath);
				break;
		}

		if (bRet)
		{
			pTape->GetWaveFile(&tfi, false);

			if (tfi.marker2 == -1) // файл повреждён как-то
			{
				bRet = false; // у файла нет конца
			}
			else
			{
				// да - получим полную информацию о нём
				m_nAddress = tfi.address;
				m_nLength = tfi.length;
				m_nWaveLength = pTape->GetWaveLength();
				m_nMSTime = int(float(m_nWaveLength) * 1000.0 / float(pTape->GetWorkingSSR()));
				m_bCRC = (tfi.crc == pTape->CalcCRC(&tfi));
				m_strName = Global::BKToUNICODE(tfi.name, 16);
				m_strName.Trim();
			}
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return bRet;
}


bool CTapeUnit::SaveAs(const fs::path &strPath, TAPETYPE type)
{
	Global::SetSafeName(m_strName);
	fs::path strName1 = m_strName.GetString();
	fs::path strNewPath = strPath / strName1;
	fs::path strName = strName1.stem();
	CString strExt = strName1.extension().c_str();
	CString strTypeExt;
	TAPE_FILE_INFO tfi{};
	bool bRet = true;

	// Добавим расширение для нового типа
	switch (type)
	{
		case TAPETYPE::TYPE_BIN:
		{
			// Добавим расширение .bin
			strTypeExt = CString(MAKEINTRESOURCE(IDS_FILEEXT_BINARY));
			strNewPath = strPath / strName;

			if (strExt.IsEmpty() || strExt.CollateNoCase(strTypeExt))
			{
				strNewPath += strTypeExt.GetString();
			}
		}
		break;

		case TAPETYPE::TYPE_WAV:
		{
			// Добавим расширение .wav
			strTypeExt = CString(MAKEINTRESOURCE(IDS_FILEEXT_WAVE));
			strNewPath = strPath / strName;
			strNewPath += strTypeExt.GetString();
		}
		break;

		case TAPETYPE::TYPE_MSF:
		{
			// Добавим расширение .tap
			strTypeExt = CString(MAKEINTRESOURCE(IDS_FILEEXT_TAPE));
			strNewPath = strPath / strName;
			strNewPath += strTypeExt.GetString();
		}
		break;
	}

	// Проверим, а не существует ли уже такой файл?
	int nNum = 0;

	while (fs::exists(strNewPath) && nNum < 10000)
	{
		// сделаем новое имя
		nNum++;
		CString strNameN = CString(strName.c_str()) + _T("(") + Global::IntToString(nNum) + _T(")");
		strNewPath = strPath / strNameN.GetString();
		strNewPath += strTypeExt;
	} // будем проверять имена с цифрой до тех пор, пока не найдём

	if (nNum >= 10000) // если вышли по такому условию, то явно что-то не так
	{
		// значит тупо перепишем файл с основным именем.
		strNewPath = strPath / strName;
		strNewPath += strTypeExt;
	}

	auto pTape = std::make_unique<CTape>();

	if (pTape)
	{
		switch (m_nType)
		{
			case TAPETYPE::TYPE_BIN: // Если текущий тип файла - bin
				if (type == TAPETYPE::TYPE_BIN)
				{
					// если и новый тоже bin - просто копируем файл
					bRet = !!(CopyFile(m_strPath.c_str(), strNewPath.c_str(), FALSE));
				}
				else
				{
					// сконвертируем bin в wave
					bRet = pTape->LoadBinFile(m_strPath, &tfi);
				}

				break;

			case TAPETYPE::TYPE_WAV: // Если текущий тип файла - wav
				if (type == TAPETYPE::TYPE_WAV)
				{
					// если и новый тоже wav - просто копируем файл
					bRet = !!(CopyFile(m_strPath.c_str(), strNewPath.c_str(), FALSE));
				}
				else
				{
					// загрузим wav файл
					bRet = pTape->LoadWaveFile(m_strPath);
				}

				break;

			case TAPETYPE::TYPE_MSF: // Если текущий тип файла - tap
				if (type == TAPETYPE::TYPE_MSF)
				{
					// если и новый тоже tap - просто копируем файл
					bRet = !!(CopyFile(m_strPath.c_str(), strNewPath.c_str(), FALSE));
				}
				else
				{
					// сконвертируем tap в wave
					bRet = pTape->LoadMSFFile(m_strPath);
				}

				break;

			case TAPETYPE::TYPE_TMP: // если текущий файл - временный
				bRet = pTape->LoadTmpFile(m_strPath);
				break;

			default: // Какой-то неизвестный тип.
				ASSERT(false);
				bRet = false;
		}

		if (bRet)
		{
			// конвертируем в заданный формат
			switch (type)
			{
				case TAPETYPE::TYPE_BIN:
					// сохраним загруженный/сконвертированный wav как bin
					bRet = pTape->GetWaveFile(&tfi, false);

					if (bRet)
					{
						bRet = pTape->SaveBinFile(strNewPath, &tfi);
					}

					break;

				case TAPETYPE::TYPE_WAV:
					// сохраним загруженный/сконвертированный wav
					bRet = pTape->SaveWaveFile(strNewPath);
					break;

				case TAPETYPE::TYPE_MSF:
					// сохраним загруженный/сконвертированный wav как tap
					bRet = pTape->SaveMSFFile(strNewPath);
					break;
			}
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR, MB_OK);
	}

	return bRet;
}
