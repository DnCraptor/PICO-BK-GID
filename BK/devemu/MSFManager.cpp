// MSFManager.cpp: implementation of the CMSFManager class.
//
#include "pch.h"
#include "MSFManager.h"
#include "BKMessageBox.h"

#ifdef MSF

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#define new DEBUG_NEW
#endif

/////////////////////////////////////////////////////////////////////////////
// Construction/Destruction
CMSFManager::CMSFManager()
	: m_bOpenForLoad(true)
	, m_header{ MSF_STATE_ID, MSF_VERSION_CURRENT, static_cast<DWORD>(CONF_BKMODEL::BK_0010_01) }
{
}

CMSFManager::~CMSFManager()
{
	ReleaseFile();
}


bool CMSFManager::ReleaseFile()
{
	if (m_fileMSF.m_o) // если файл был открыт
	{
		m_fileMSF.Close(); // закрываем
		m_blocks.RemoveAll();
		return true;
	}

	return false;
}

bool CMSFManager::Seek(int nOffset, UINT nFrom)
{
	try
	{
		m_fileMSF.Seek(nOffset, nFrom);
	}
	catch (CFileException &e)
	{
		return false;
	}

	return true;
}

// проверка msf файла
bool CMSFManager::CheckFile(const fs::path &strPath, bool bSilent)
{
	bool bRet = true;
	ReleaseFile();
	DWORD flag = CFile::modeRead;

	if (m_fileMSF.Open(strPath.c_str(), flag))
	{
		int nFilePos = 0;

		if (m_fileMSF.Read(&m_header, sizeof(m_header)) == sizeof(m_header))
		{
			nFilePos += sizeof(m_header);

			if (m_header.type == MSF_STATE_ID)
			{
				if (m_header.version > MSF_VERSION_CURRENT)
				{
					if (!bSilent)
					{
						g_BKMsgBox.Show(IDS_BK_ERROR_MSFVERSIONMISMATCH, MB_OK);
					}

					bRet = false;
				}
				else
				{
					if (m_header.version < MSF_VERSION_MINIMAL)
					{
						if (!bSilent)
						{
							g_BKMsgBox.Show(IDS_BK_ERROR_MSFOLDVERSION, MB_OK);
						}
					}

					MSF_BLOCK_INFO bi{};

					while (m_fileMSF.Read(&bi.header, sizeof(MSF_BLOCK_HEADER)) == sizeof(MSF_BLOCK_HEADER))
					{
						bi.offset = nFilePos;
						m_blocks.Add(bi);

						if (!Seek(bi.header.length - sizeof(MSF_BLOCK_HEADER), CFile::SeekPosition::current))
						{
							if (!bSilent)
							{
								g_BKMsgBox.Show(IDS_BK_ERROR_MSFCORRUPTMSF, MB_OK);
							}

							bRet = false;
							break;
						}

						nFilePos += bi.header.length;
					}
				}
			}
			else
			{
				if (!bSilent)
				{
					g_BKMsgBox.Show(IDS_BK_ERROR_MSFNOMSFFILE, MB_OK);
				}

				bRet = false;
			}
		}
		else
		{
			if (!bSilent)
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_MSFREADHEADERRROR, MB_OK);
			}

			bRet = false;
		}

		ReleaseFile();
	}
	else
	{
		if (!bSilent)
		{
			g_BKMsgBox.Show(IDS_BK_ERROR_MSFOPENFILEERROR, MB_OK);
		}

		bRet = false;
	}

	return bRet;
}

bool CMSFManager::OpenFile(const fs::path &strPath, bool bLoad, bool bSilent)
{
	bool bRet = true;
	ReleaseFile();
	DWORD flag = CFile::modeRead;

	if (!bLoad)
	{
		flag |= CFile::modeCreate | CFile::modeWrite;
	}

	if (m_fileMSF.Open(strPath.c_str(), flag))
	{
		m_bOpenForLoad = bLoad;

		if (m_bOpenForLoad)
		{
			if (m_fileMSF.Read(&m_header, sizeof(m_header)) == sizeof(m_header))
			{
				int nFilePos = sizeof(m_header);

				if (m_header.type == MSF_STATE_ID)
				{
					MSF_BLOCK_INFO bi{};

					while (m_fileMSF.Read(&bi.header, sizeof(MSF_BLOCK_HEADER)) == sizeof(MSF_BLOCK_HEADER))
					{
						bi.offset = nFilePos;
						m_blocks.Add(bi);

						if (!Seek(bi.header.length - sizeof(MSF_BLOCK_HEADER), CFile::SeekPosition::current))
						{
							if (!bSilent)
							{
								g_BKMsgBox.Show(IDS_BK_ERROR_MSFCORRUPTMSF, MB_OK);
							}

							bRet = false;
							break;
						}

						nFilePos += bi.header.length;
					}
				}
				else
				{
					if (!bSilent)
					{
						g_BKMsgBox.Show(IDS_BK_ERROR_MSFNOMSFFILE, MB_OK);
					}

					bRet = false;
				}
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_MSFREADHEADERRROR, MB_OK);
				bRet = false;
			}
		}
		else
		{
			try
			{
				m_fileMSF.Write(&m_header, sizeof(m_header));
			}
			catch (CFileException &e)
			{
				if (!bSilent)
				{
					g_BKMsgBox.Show(IDS_BK_ERROR_MSFWRITEHEADERRROR, MB_OK);
				}

				bRet = false;
			}
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MSFOPENFILEERROR, MB_OK);
		bRet = false;
	}

	if (!bRet)
	{
		ReleaseFile();
	}

	return bRet;
}


bool CMSFManager::FindBlock(DWORD blockType, MSF_BLOCK_INFO *pBi)
{
	for (int n = 0; n < m_blocks.GetSize(); ++n)
	{
		MSF_BLOCK_INFO bi = m_blocks[n];

		if (bi.header.type == blockType)
		{
			*pBi = bi;
			return true;
		}
	}

	return false;
}


bool CMSFManager::GetBlockHeader(MSF_BLOCK_INFO *pBi)
{
	bool bRet = false;

	if (Seek(pBi->offset, CFile::SeekPosition::begin))
	{
		if (Seek(sizeof(MSF_BLOCK_HEADER), CFile::SeekPosition::current))
		{
			bRet = true;
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MSFSEEKERROR, MB_OK);
	}

	return bRet;
}


bool CMSFManager::GetBlockData(uint8_t *pBuff, DWORD length)
{
	if (m_fileMSF.Read(pBuff, length) != length)
	{
		return false;
	}

	return true;
}


bool CMSFManager::SetBlockHeader(MSF_BLOCK_INFO *pBi)
{
	try
	{
		m_fileMSF.Write(&pBi->header, sizeof(MSF_BLOCK_HEADER));
	}
	catch (CFileException &e)
	{
		return false;
	}

	return true;
}


bool CMSFManager::SetBlockData(uint8_t *pBuff, DWORD length)
{
	try
	{
		m_fileMSF.Write(pBuff, length);
	}
	catch (CFileException &e)
	{
		return false;
	}

	return true;
}

bool CMSFManager::GetBlockPreview(HBITMAP *hScreenshot, uint8_t **pBits)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;
#ifdef UI
	if (FindBlock(MSF_BLOCKTYPE_PREVIEW, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			BITMAPINFOHEADER bmih{};

			if (GetBlockData(reinterpret_cast<uint8_t *>(&bmih), sizeof(bmih)))
			{
				*pBits = nullptr;

				if ((*hScreenshot = ::CreateDIBSection(nullptr, (BITMAPINFO *)&bmih, DIB_RGB_COLORS, (void **)pBits, nullptr, 0)))
				{
					if (GetBlockData(*pBits, bmih.biSizeImage))
					{
						bRet = true;
					}
					else
					{
						str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
						g_BKMsgBox.Show(str, MB_OK);
					}
				}
				else
				{
					g_BKMsgBox.Show(IDS_BK_ERROR_MSFPREVIEWBLK_DIB, MB_OK);
				}
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
		g_BKMsgBox.Show(str, MB_OK);
	}
#endif
	return bRet;
}

bool CMSFManager::SetBlockPreview(HBITMAP hScreenshot)
{
	bool bRet = false;
#ifdef UI
	DIBSECTION ds{};
	CString str;

	if (::GetObject(hScreenshot, sizeof(ds), &ds))
	{
		MSF_BLOCK_INFO bi{ 0,
			{
				MSF_BLOCKTYPE_PREVIEW,
				sizeof(MSF_BLOCK_HEADER) + sizeof(ds.dsBmih) + ds.dsBmih.biSizeImage
			}
		};

		if (SetBlockHeader(&bi))
		{
			if (SetBlockData(reinterpret_cast<uint8_t *>(&ds.dsBmih), sizeof(ds.dsBmih)))
			{
				ASSERT(ds.dsBm.bmBits);

				if (SetBlockData(reinterpret_cast<uint8_t *>(ds.dsBm.bmBits), ds.dsBmih.biSizeImage))
				{
					bRet = true;
				}
				else
				{
					str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
					g_BKMsgBox.Show(str, MB_OK);
				}
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKPREVIEW)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_MSFPREVIEWBLK_OBJ);
	}
#endif
	return bRet;
}

constexpr auto nBase10MemSize = 0200000;

bool CMSFManager::GetBlockBaseMemory(PSRAM *pMemory)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_BASEMEMORY, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (CheckBlockSize(&bi, nBase10MemSize))
			{
				bRet = GetBlockData(pMemory, nBase10MemSize);
			}

			if (!bRet)
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockBaseMemory(PSRAM *pMemory)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{
			MSF_BLOCKTYPE_BASEMEMORY,
			sizeof(MSF_BLOCK_HEADER) + nBase10MemSize
		}
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(pMemory, nBase10MemSize))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

constexpr auto nBase11MemSize = 0700000;

bool CMSFManager::GetBlockBaseMemory11M(PSRAM *pMemory)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_BASEMEMORY11M, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (CheckBlockSize(&bi, nBase11MemSize))
			{
				bRet = GetBlockData(pMemory, nBase11MemSize);
			}

			if (!bRet)
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM11)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM11)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM11)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockBaseMemory11M(PSRAM *pMemory)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{0,
		{
			MSF_BLOCKTYPE_BASEMEMORY11M,
			sizeof(MSF_BLOCK_HEADER) + nBase11MemSize
		} // 0700000 == 128кб ОЗУ + 80кб ПЗУ, в том числе FDD + 16кб ОЗУ FDD
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(pMemory, nBase11MemSize))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM11)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKBASEMEM11)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

constexpr auto nSMK512MemSize = (512 - 16) * 1024;

bool CMSFManager::GetBlockMemorySMK512(PSRAM *pMemory)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_MEMORY_SMK512, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (CheckBlockSize(&bi, nSMK512MemSize))
			{
				bRet = GetBlockData(pMemory, nSMK512MemSize);
			}

			if (!bRet)
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSMK512MEM)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSMK512MEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSMK512MEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockMemorySMK512(PSRAM *pMemory)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{
			MSF_BLOCKTYPE_MEMORY_SMK512,
			sizeof(MSF_BLOCK_HEADER) + nSMK512MemSize
		}
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(pMemory, nSMK512MemSize))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSMK512MEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSMK512MEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}


bool CMSFManager::GetBlockCPURegisters(MSF_CPU_REGISTERS *pCPURegisters)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_CPU_REGISTERS, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (!CheckBlockSize(&bi, sizeof(MSF_CPU_REGISTERS)))
			{
				return false;
			}

			if (GetBlockData(reinterpret_cast<uint8_t *>(pCPURegisters), sizeof(MSF_CPU_REGISTERS)))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCPUREGS)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCPUREGS)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCPUREGS)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockCPURegisters(MSF_CPU_REGISTERS *pCPURegisters)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{0,
		{
			MSF_BLOCKTYPE_CPU_REGISTERS,
			sizeof(MSF_BLOCK_HEADER) + sizeof(MSF_CPU_REGISTERS)
		}
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(reinterpret_cast<uint8_t *>(pCPURegisters), sizeof(MSF_CPU_REGISTERS)))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCPUREGS)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCPUREGS)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}


bool CMSFManager::GetBlockPortRegs(MSF_PORT_REGS *pPortRegs)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (m_header.version < MSF_VERSION_MINIMAL)
	{
		return bRet;
	}

	if (FindBlock(MSF_BLOCKTYPE_PORT_REGS, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (!CheckBlockSize(&bi, sizeof(MSF_PORT_REGS)))
			{
				return false;
			}

			if (GetBlockData(reinterpret_cast<uint8_t *>(pPortRegs), sizeof(MSF_PORT_REGS)))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSYSREGS)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSYSREGS)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSYSREGS)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}


bool CMSFManager::SetBlockPortRegs(MSF_PORT_REGS *pPortRegs)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{0,
		{
			MSF_BLOCKTYPE_PORT_REGS,
			sizeof(MSF_BLOCK_HEADER) + sizeof(MSF_PORT_REGS)
		}
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(reinterpret_cast<uint8_t *>(pPortRegs), sizeof(MSF_PORT_REGS)))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSYSREGS)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKSYSREGS)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

constexpr auto nExt32MemSize = 4 * 8192;

bool CMSFManager::GetBlockExt32Memory(DWORD *pnPage, uint8_t *pMemoryExt)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_10EXT32, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (CheckBlockSize(&bi, sizeof(DWORD) + nExt32MemSize))
			{
				bRet = GetBlockData(reinterpret_cast<uint8_t *>(&pnPage), sizeof(DWORD));

				if (bRet)
				{
					bRet = GetBlockData(pMemoryExt, nExt32MemSize);
				}
			}

			if (!bRet)
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT32MEM)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT32MEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT32MEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockExt32Memory(DWORD nPage, uint8_t *pMemoryExt)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{0,
		{
			MSF_BLOCKTYPE_10EXT32,
			sizeof(MSF_BLOCK_HEADER) + sizeof(DWORD) + nExt32MemSize
		}
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(reinterpret_cast<uint8_t *>(&nPage), sizeof(DWORD)))
		{
			if (SetBlockData(pMemoryExt, nExt32MemSize))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT32MEM)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT32MEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT32MEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

constexpr auto nExt16MemSize = 3 * 8192;

bool CMSFManager::GetBlockExt16Memory(uint8_t *pMemoryExt)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_10EXT16, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (CheckBlockSize(&bi, nExt16MemSize))
			{
				bRet = GetBlockData(pMemoryExt, nExt16MemSize);
			}

			if (!bRet)
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT16MEM)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT16MEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT16MEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockExt16Memory(uint8_t *pMemoryExt)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{
			MSF_BLOCKTYPE_10EXT16,
			sizeof(MSF_BLOCK_HEADER) + nExt16MemSize
		} // сохраняем 16кб озу + 8 кб пзу контроллера
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(pMemoryExt, nExt16MemSize))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT16MEM)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKEXT16MEM)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::GetBlockTape(uint8_t *pPackedWave, DWORD *pnBitsLength, DWORD *pnPackLength)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_WAVE, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			DWORD nPackLength = bi.header.length - sizeof(MSF_BLOCK_HEADER) - sizeof(DWORD);

			if (GetBlockData(reinterpret_cast<uint8_t *>(pnBitsLength), sizeof(DWORD)))
			{
				if (!pPackedWave)
				{
					*pnPackLength = nPackLength;
					return true;
				}

				if (GetBlockData(reinterpret_cast<uint8_t *>(pPackedWave), nPackLength))
				{
					bRet = true;
				}
				else
				{
					str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
					g_BKMsgBox.Show(str, MB_OK);
				}
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockTape(uint8_t *pPackedWave, DWORD nBitsLength, DWORD nPackLength)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{ MSF_BLOCKTYPE_WAVE, sizeof(MSF_BLOCK_HEADER) + sizeof(DWORD) + nPackLength }
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(reinterpret_cast<uint8_t *>(&nBitsLength), sizeof(DWORD)))
		{
			if (SetBlockData(reinterpret_cast<uint8_t *>(pPackedWave), nPackLength))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKTAPE)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}


bool CMSFManager::GetBlockMemMap(BKMEMBank_t *MBType, ConfBKModel_t *CBKMType)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_MEMMAP, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			if (CheckBlockSize(&bi, sizeof(BKMEMBank_t) * 16 + sizeof(ConfBKModel_t)))
			{
				bRet = GetBlockData(reinterpret_cast<uint8_t *>(MBType), sizeof(BKMEMBank_t) * 16);

				if (bRet)
				{
					bRet = GetBlockData(reinterpret_cast<uint8_t *>(CBKMType), sizeof(ConfBKModel_t));
				}
			}

			if (!bRet)
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKMEMMAPMGR)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKMEMMAPMGR)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		// в старых версиях этого блока не было,
		// поэтому старые версии будут не совместимы с новой. и ничего тут не поделать
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKMEMMAPMGR)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockMemMap(BKMEMBank_t *MBType, ConfBKModel_t *CBKMType)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{
			MSF_BLOCKTYPE_MEMMAP,
			sizeof(MSF_BLOCK_HEADER) + sizeof(BKMEMBank_t) * 16 + sizeof(ConfBKModel_t)
		}
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(reinterpret_cast<uint8_t *>(MBType), sizeof(BKMEMBank_t) * 16))
		{
			if (SetBlockData(reinterpret_cast<uint8_t *>(CBKMType), sizeof(ConfBKModel_t)))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKMEMMAPMGR)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKMEMMAPMGR)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKMEMMAPMGR)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}



bool CMSFManager::GetBlockConfig()
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (FindBlock(MSF_BLOCKTYPE_CONFIG, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			DWORD nBlockLength = bi.header.length - sizeof(MSF_BLOCK_HEADER);
			auto pBlock = std::vector<uint8_t>(nBlockLength);

			if (pBlock.data())
			{
				if (GetBlockData(pBlock.data(), nBlockLength))
				{
					g_Config.LoadConfig_FromMemory(pBlock.data(), nBlockLength);
					bRet = true;
				}
				else
				{
					str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCONFIG)));
					g_BKMsgBox.Show(str, MB_OK);
				}
			}
			else
			{
				g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCONFIG)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCONFIG)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

#define _MAX_PATH   2 // max. length of full pathname 260
bool CMSFManager::SetBlockConfig()
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{MSF_BLOCKTYPE_CONFIG, sizeof(MSF_BLOCK_HEADER)}  // + длина блока. длина переменная
	};
	UINT nSize = _MAX_PATH * 1000;
	auto pBlock = std::vector<uint8_t>(nSize); // размер будет достаточен наверняка

	if (pBlock.data())
	{
		ZeroMemory(pBlock.data(), nSize);
		auto len = static_cast<DWORD>(g_Config.SaveConfig_ToMemory(pBlock.data(), nSize));
		bi.header.length += len;

		if (SetBlockHeader(&bi))
		{
			if (SetBlockData(pBlock.data(), len))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCONFIG)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKCONFIG)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		g_BKMsgBox.Show(IDS_BK_ERROR_NOTENMEMR);
	}

	return bRet;
}

// проверка размера блока, перед его чтением. На тот случай, если я опять наизменял конфигурацию
// вход: bi - указатель на блок заголовка
//      size - sizeof структуры
// выход: true - всё нормально
//      false - размер не совпадает.
bool CMSFManager::CheckBlockSize(MSF_BLOCK_INFO *pbi, DWORD size)
{
	return (pbi->header.length == sizeof(MSF_BLOCK_HEADER) + size);
}

bool CMSFManager::GetBlockFrameData(MSF_FRAMEDATA *FDBlock)
{
	MSF_BLOCK_INFO bi{};
	CString str;
	bool bRet = false;

	if (m_header.version < MSF_VERSION_MINIMAL)
	{
		return bRet;
	}

	if (FindBlock(MSF_BLOCKTYPE_FRAMEDATA, &bi))
	{
		if (GetBlockHeader(&bi))
		{
			// если размер не совпадает, блок не читаем. Но и сообщение об ошибке не выдаём
			if (!CheckBlockSize(&bi, sizeof(MSF_FRAMEDATA)))
			{
				return false;
			}

			if (GetBlockData(reinterpret_cast<uint8_t *>(FDBlock), sizeof(MSF_FRAMEDATA)))
			{
				bRet = true;
			}
			else
			{
				str.Format(IDS_BK_ERROR_DTABLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKFRAMEDATA)));
				g_BKMsgBox.Show(str, MB_OK);
			}
		}
		else
		{
			str.Format(IDS_BK_ERROR_HDRBLKRDERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKFRAMEDATA)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_BLKNOTFOUND, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKFRAMEDATA)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}

bool CMSFManager::SetBlockFrameData(MSF_FRAMEDATA *FDBlock)
{
	CString str;
	bool bRet = false;
	MSF_BLOCK_INFO bi{ 0,
		{ MSF_BLOCKTYPE_FRAMEDATA, sizeof(MSF_BLOCK_HEADER) + sizeof(MSF_FRAMEDATA) }
	};

	if (SetBlockHeader(&bi))
	{
		if (SetBlockData(reinterpret_cast<uint8_t *>(FDBlock), sizeof(MSF_FRAMEDATA)))
		{
			bRet = true;
		}
		else
		{
			str.Format(IDS_BK_ERROR_DTABLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKFRAMEDATA)));
			g_BKMsgBox.Show(str, MB_OK);
		}
	}
	else
	{
		str.Format(IDS_BK_ERROR_HDRBLKWRERR, CString(MAKEINTRESOURCE(IDS_BK_ERROR_BLKFRAMEDATA)));
		g_BKMsgBox.Show(str, MB_OK);
	}

	return bRet;
}
#endif
