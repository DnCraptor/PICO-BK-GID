#include "pch.h"
#include "Listing.h"
#include "Globals.h"
#include "Reader.h"
#include "LabelManager.h"

std::vector <ListingManager::ListingLine> g_Listing;
#pragma warning(disable:4996)

int g_offsetpc = g_Globals.GetStartAddress();

// начальные приготовления
void ListingManager::PrepareLine(const int cp)
{
	ListingLine ll;
	ll.nLineNum = g_pReader->getLineNum();
	ll.line = g_pReader->getCurString();
	const ListingCmdType lct
	{
		cp,                     // nPC
		0,                      // nEndAdr
		ListType::LT_BLANKLINE  // LT
	};
	ll.vCMD.push_back(lct);
	g_Listing.push_back(ll);
}

// добавляем инфу о текущей инструкции
void ListingManager::AddPrepareLine(const int cp, const ListType lt)
{
	const auto pll = g_Listing.end() - 1;
	const auto plct = pll->vCMD.end() - 1;

	if (plct->LT == ListType::LT_BLANKLINE)   // если последняя запись - пустая строка, то она и первая в общем-то
	{
		plct->nPC = cp;
		plct->LT = lt;  // то меняем её
	}
	else if (plct->LT == ListType::LT_LABEL)  // если последняя запись - метка, то тоже заменяем её, если эта запись остаётся,
	{
		// то это была единственная метка в строке.
		if (lt != ListType::LT_COMMENTARY)
		{
			plct->nPC = cp;
			plct->LT = lt;  // то меняем её, если это не комментарий
		}
	}
	else if (lt != ListType::LT_COMMENTARY) // если нет - то добавим новую, если это не комментарий.
	{
		const ListingCmdType lct
		{
			cp,     // nPC
			0,      // nEndAdr
			lt      // LT
		};
		pll->vCMD.push_back(lct);
	}
}

// добавляем инфу о текущей инструкции
void ListingManager::AddPrepareLine2(const int len)
{
	const auto pll = g_Listing.end() - 1;
	const auto plct = pll->vCMD.end() - 1;
	plct->nEndAdr = len & 0xffff;
}

/*
Листинг состоит из 4х полей
1 - номер строки
2 - адрес команды
3 - бинарные данные
4 - текстовая строка
*/


// выводим номер строки
void ListingManager::OutLineNum(FILE *of, const int nOut, const int num)
{
	if (nOut == 0)
	{
		fwprintf(of, L"% 5d ", num);
	}
	else
	{
		fwprintf(of, L"      ");
	}
}
// выводим адрес
void ListingManager::OutAddress(FILE *of, const int nOutLN, const bool bOut, const int num, const int addr)
{
	OutLineNum(of, nOutLN, num);

	if (bOut)
	{
		fwprintf(of, L"%07o: ", ((addr - BASE_ADDRESS + g_offsetpc) & 0xffff));
	}
	else
	{
		fwprintf(of, L"         ");
	}
}
// выводим бинарные данные пословно
// Вход: bgn_addr - начальный адрес данных
//      end_addr - конечный адрес данных
void ListingManager::OutWords(FILE *of, const int nOutLN, const bool bOutAddr, const bool bOut, const int num, int &bgn_addr, const int end_addr)
{
	OutAddress(of, nOutLN, bOutAddr, num, bgn_addr);
	int npc = bgn_addr / 2;
	// самое простое. может быть от 1 до 3 слов данных
	const int nlen = (bOut) ? (end_addr - bgn_addr) / 2 : 0;
	const int nmax = 3 - nlen;

	for (int i = 0; i < nlen; ++i)
	{
		fwprintf(of, L"%07o ", g_Memory.w[npc++]);
	}

	for (int i = 0; i < nmax; ++i)
	{
		fwprintf(of, L"        ");
	}

	fwprintf(of, L" ");
	bgn_addr = npc * 2;
}
// выводим бинарные данные побайтово
// Вход: bgn_addr - начальный адрес данных
//      end_addr - конечный адрес данных
void ListingManager::OutBytes(FILE *of, const int nOutLN, const bool bOutAddr, const bool bOut, const int num, int &bgn_addr, const int end_addr)
{
	OutAddress(of, nOutLN, bOutAddr, num, bgn_addr);
	int npc = bgn_addr;
	// самое простое. может быть от 1 до 5 байтов данных
	const int nlen = (bOut) ? (end_addr - bgn_addr) : 0;
	const int nmax = 5 - nlen;

	for (int i = 0; i < nlen; ++i)
	{
		fwprintf(of, L"%04o ", g_Memory.b[npc++]);
	}

	for (int i = 0; i < nmax; ++i)
	{
		fwprintf(of, L"     ");
	}

	bgn_addr = npc;
}

void ListingManager::OutString(FILE *of, std::wstring *str /*= nullptr*/)
{
	if (str)
	{
		size_t nLen = str->length();

		if (nLen)
		{
			auto pBuff = std::make_unique <uint8_t[]>(nLen + 1);

			if (pBuff)
			{
				UNICODEtoBK(*str, pBuff.get(), nLen, false);
				pBuff[nLen] = 0;
				fprintf(of, "%s", pBuff.get());
				pBuff.reset();
			}
		}
	}
}

void ListingManager::OutLineString(FILE *of, const int nCnt, std::wstring *str)
{
	if (nCnt == 0)
	{
		OutString(of, str);
		return;
	}

	fwprintf(of, L"\n");
}


void ListingManager::MakeListing(const fs::path &strName, const fs::path &strExt)
{
	g_offsetpc = g_Globals.GetStartAddress();
	// сделаем новое имя файла
	fs::path outName = strName;
	outName.replace_extension(strExt);
	FILE *of = _wfopen(outName.c_str(), L"wt");

	if (of == nullptr)
	{
		wprintf(L"Ошибка создания файла %s\n", outName.c_str());
	}

	for (auto &ll : g_Listing)
	{
		int nCount = 0;

// перед строкой выведем ошибки, если есть
		for (auto &err : ll.errors)
		{
			OutString(of, &err);
		}

		for (auto &lct : ll.vCMD)
		{
			int adr = lct.nPC;

			// теперь, в зависимости от типа команды вывести содержимое
			switch (lct.LT)
			{
				case ListType::LT_BLANKLINE:
					OutLineNum(of, nCount++, ll.nLineNum);
					fwprintf(of, L"\n");
					break;

				case ListType::LT_COMMENTARY:
					adr = 0;
					OutWords(of, nCount, false, false, ll.nLineNum, adr, 0); // выводим только номер строки
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_ASSIGN:
					OutAddress(of, nCount, false, ll.nLineNum, 0); // здесь не выводим адрес
					{
						int v = (lct.nEndAdr >= 0) ? g_labelGlobalDefs.GetElement(lct.nEndAdr).getValue() : 0;
						fwprintf(of, L"%07o                  ", (v & 0xffff)); // словные данные выводим так
					}
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_ASSIGNREG:
					OutAddress(of, nCount, false, ll.nLineNum, 0); // здесь не выводим адрес
					{
						fwprintf(of, L"%07o                  ", (lct.nEndAdr & 0xffff)); // словные данные выводим так
					}
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_LABEL:
					OutWords(of, nCount, true, false, ll.nLineNum, adr, lct.nEndAdr); // выводим адрес и данные
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_INSTRUCTION:
					OutWords(of, nCount, true, true, ll.nLineNum, adr, lct.nEndAdr); // выводим адрес и данные
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_PSC_ADDR:
					OutWords(of, nCount, true, true, ll.nLineNum, adr, lct.nPC + 2);
					OutLineString(of, nCount++, &ll.line);
					OutWords(of, nCount, true, true, ll.nLineNum, adr, lct.nEndAdr);
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_PSC_LA:
					g_offsetpc = g_Globals.GetStartAddress(); // поменяем адрес инструкций в листинге
					OutWords(of, nCount, true, false, ll.nLineNum, adr, 0);
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_PSC_BLKW:
				case ListType::LT_PSC_BLKB:
				case ListType::LT_PSC_END:
					OutWords(of, nCount, true, false, ll.nLineNum, adr, 0);
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_PSC_PRINT:
					OutWords(of, nCount, true, true, ll.nLineNum, adr, lct.nPC + 4);
					OutLineString(of, nCount++, &ll.line);
					OutWords(of, nCount, true, true, ll.nLineNum, adr, lct.nPC + 6);
					OutLineString(of, nCount++, &ll.line);
					OutWords(of, nCount, true, true, ll.nLineNum, adr, lct.nEndAdr);
					OutLineString(of, nCount++, &ll.line);
					break;

				case ListType::LT_PSC_FLT2:
				case ListType::LT_PSC_FLT4:
				case ListType::LT_PSC_WORD:
				case ListType::LT_PSC_RAD50:
				{
					// выводить по три слова в строку
					const int len = (lct.nEndAdr - adr) / 2;
					int cnt = len / 3;

					if (len % 3)
					{
						cnt++;
					}

					for (int i = 0; i < cnt; ++i)
					{
						const int endadr = (adr + 6 < lct.nEndAdr) ? adr + 6 : lct.nEndAdr;
						OutWords(of, nCount, true, true, ll.nLineNum, adr, endadr); // выводим адрес и данные
						OutLineString(of, nCount++, &ll.line);
					}
				}
				break;

				case ListType::LT_PSC_BYTE:
				case ListType::LT_PSC_ASCII:
					// выводить по 5 байтов в строку
				{
					int len = (lct.nEndAdr - adr);
					int cnt = len / 5;

					if (len % 5)
					{
						cnt++;
					}

					for (int i = 0; i < cnt; ++i)
					{
						const int endadr = (adr + 5 < lct.nEndAdr) ? adr + 5 : lct.nEndAdr;
						OutBytes(of, nCount, true, true, ll.nLineNum, adr, endadr); // выводим адрес и данные
						OutLineString(of, nCount++, &ll.line);
					}
				}
				break;

				case ListType::LT_PSC_EVEN:
				{
					const bool bOut = ((lct.nEndAdr - lct.nPC) != 0);
					OutBytes(of, nCount, true, bOut, ll.nLineNum, adr, lct.nEndAdr);
					OutLineString(of, nCount++, &ll.line);
				}
				break;

				default:
				{
					fwprintf(of, L"LST:: !!! Unworked case !!!\n");
				}
			}
		}
	}

	fclose(of);
}


