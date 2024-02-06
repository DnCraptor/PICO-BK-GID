#include "pch.h"
#include "ibm2vxt.h"

const static wchar_t *cchStrEndEnum = L".!?:;";

// просмотр вперёд.
uint8_t ITCPC::VXT_Subr(CBaseFile *pInFile, unsigned int &dwFlags, int &nOtstup, TCHAR &wchPrSym)
{
	unsigned int t = 0;
	long pos = pInFile->GetPos();
	int nSpacesCount = 0;
	uint8_t chOut = 10;

	while (!pInFile->isEOF())
	{
		wchar_t wchSym = pInFile->ReadChar();

		if (pInFile->isEOF())
		{
			break;
		}

		if (wchSym == 0)
		{
			t = INPUT_SYMBOL_NULL;
		}
		else if (wchSym == 10)
		{
			t = INPUT_SYMBOL_N;
		}
		else if (wchSym == 13)
		{
			t = INPUT_SYMBOL_R;
		}
		else if (wchSym == 9)
		{
			int nNextTabPos = (nSpacesCount / pInFile->getTabWidth() + 1) * pInFile->getTabWidth();

			do
			{
				nSpacesCount++;
			}
			while (nSpacesCount < nNextTabPos);

			continue;
		}
		else if (wchSym == 32)
		{
			nSpacesCount++;
			continue;
		}
		else
		{
			if (nSpacesCount > nOtstup)
			{
				if (wcschr(cchStrEndEnum, wchPrSym) != nullptr)
				{
					break;
				}
			}

			chOut = 0;
			break;
		}

		if ((dwFlags & t) != 0)
		{
			break;
		}
	}

	pInFile->SetPos(pos);
	return chOut;
}

void ITCPC::IBM2VXT(CBaseFile *pInFile, CBaseFile *pOutFile)
{
	if (pInFile == nullptr || pOutFile == nullptr)
	{
		wprintf(L"Недостаточно памяти.\n");
		return;
	}

	int nTmpOtstup = 0;       // временный отступ.
	int nOtstup = 1000000;    // отступ в строке.
	int nCharsCount = 0;      // счётчик символов в строке для входного файла
	int nSpacesCount = 0;     // счётчик пробельных символов.
	unsigned int t;           // временные флаги.
	unsigned int dwFlags = 0; // флаги.
	wchar_t wchPrSym = 0;     // предыдущий символ
	wchar_t wchSym;           // текущий символ
	uint8_t chOut;            // выходной символ, как правило управляющий

	if (!pInFile->Open(false))
	{
		wprintf(L"Ошибка! '%s' не открывается.\n", pInFile->getFileName().c_str());
		return;
	}

	if (!pOutFile->Open(true))
	{
		wprintf(L"Ошибка! '%s' не открывается.\n", pOutFile->getFileName().c_str());
		return;
	}

	while (!pInFile->isEOF())
	{
		wchSym = pInFile->ReadChar(); // читаем очередной символ

		if (pInFile->isEOF())
		{
			break;
		}

		switch (wchSym) // проверка на конец строки или не конец строки
		{
			case 0: t = INPUT_SYMBOL_NULL;
				break;

			case 10: t = INPUT_SYMBOL_N;
				break;

			case 13: t = INPUT_SYMBOL_R;
				break;

			default:
			{
				// если не символы конца строки, то сбросим все флаги разных вариантов конца строки
				dwFlags &= ~INPUT_SYMBOL_EOLN;

				switch (wchSym)
				{
					case 9: // если табуляция
					{
						int nNextTabPos = (nCharsCount / pInFile->getTabWidth() + 1) * pInFile->getTabWidth();

						do  // забъём пробелами сколько нужно
						{
							nCharsCount++;  // увеличим счётчик символов в строке
							nSpacesCount++; // увеличим счётчик пробельных символов

							if (dwFlags & INPUT_FLAG_ABSATZ)
							{
								nTmpOtstup++;   // если нужно считаем новый отступ
							}
						}
						while (nCharsCount < nNextTabPos);

						break;
					}

					case 32:    // если пробел
						nCharsCount++;
						nSpacesCount++;

						if (dwFlags & INPUT_FLAG_ABSATZ)
						{
							nTmpOtstup++;   // если нужно считаем новый отступ
						}

						break;

					default:    // все непробельные символы
					{
						if (dwFlags & INPUT_FLAG_ABSATZ) // отступ считали?
						{
							dwFlags &= ~INPUT_FLAG_ABSATZ;  // больше не нужно

							if (nTmpOtstup < nOtstup)   // и если он меньше текущего
							{
								nOtstup = nTmpOtstup;   // новый отступ будет высчитанным.
							}
						}

						if (nSpacesCount) // если насчитали сколько-то пробелов
						{
							// кодируем пробелы
							while (nSpacesCount > 8)
							{
								chOut = nSpacesCount & 7;
								nSpacesCount >>= 3;

								if (chOut == 0)
								{
									chOut = 8;
									nSpacesCount--;
								}

								pOutFile->WriteByte(chOut);
							}

							chOut = nSpacesCount;
							pOutFile->WriteByte(chOut);
							nSpacesCount = 0;
						}

						wchPrSym = wchSym;
						pOutFile->WriteChar(wchSym); nCharsCount++;
					}
				}

				continue;
			}
		}

		if ((dwFlags & INPUT_SYMBOL_EOLN) == 0) // если ни одного флага конца строки нету
		{
			dwFlags |= t;   // установим тот, который получили
		}
		else
		{
			// если хоть один был
			if ((dwFlags & t) == 0) // и если не точно такой, что был в прошлый раз
			{
				dwFlags &= ~INPUT_SYMBOL_EOLN; // то сбросим флаги конца строки
				continue; // и пойдём читать следующий символ
			}
		}

		// обработка конца строки
		if (nCharsCount != 0)
		{
			chOut = VXT_Subr(pInFile, dwFlags, nOtstup, wchPrSym);
		}
		else
		{
			chOut = 10;
		}

		nSpacesCount = 0;
		nTmpOtstup = 0;
		dwFlags |= INPUT_FLAG_ABSATZ;
		pOutFile->WriteByte(chOut);
		nCharsCount = 0;
	}

	pInFile->Close();
	pOutFile->Close();
}

