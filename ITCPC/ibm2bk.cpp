#include "pch.h"
#include "ibm2bk.h"
/*
void ITCPC::IBM2BK(CBaseFile *pInFile, CBaseFile *pOutFile)
{
    if (pInFile == nullptr || pOutFile == nullptr)
    {
        wprintf(L"Недостаточно памяти.\n");
        return;
    }

    unsigned int dwFlags = 0, t;
    int nCharsCount = 0;
    wchar_t Sym;

    if (!pInFile->Open(false))
    {
        wprintf(L"Ошибка! %s не открывается.\n", pInFile->getFileName().c_str());
        return;
    }

    if (!pOutFile->Open(true))
    {
        wprintf(L"Ошибка! %s не открывается.\n", pOutFile->getFileName().c_str());
        return;
    }

    while (!pInFile->isEOF())
    {
        Sym = pInFile->ReadChar();

        if (pInFile->isEOF())
        {
            break;
        }

        switch (Sym)
        {
            case 0: t = INPUT_SYMBOL_NULL;
                break;

            case 10: t = INPUT_SYMBOL_N;
                break;

            case 13: t = INPUT_SYMBOL_R;
                break;

            default:
            {
                dwFlags = 0;

                if (Sym == 9)
                {
                    int nNextTabPos = (nCharsCount / pInFile->getTabWidth() + 1) * pInFile->getTabWidth();

                    do
                    {
                        pOutFile->WriteChar(L' '); nCharsCount++;
                    }
                    while (nCharsCount < nNextTabPos);
                }
                else
                {
                    pOutFile->WriteChar(Sym); nCharsCount++;
                }

                continue;
            }
        }

        if ((dwFlags & INPUT_SYMBOL_EOLN) == 0)
        {
            dwFlags |= t;
        }
        else
        {
            if ((dwFlags & t) == 0)
            {
                dwFlags = 0;
                continue;
            }
        }

        pOutFile->WriteChar(0);
        nCharsCount = 0;
    }
}
*/

// вариант с упаковкой пробелов.
void ITCPC::IBM2BK(CBaseFile *pInFile, CBaseFile *pOutFile)
{
	if (pInFile == nullptr || pOutFile == nullptr)
	{
		wprintf(L"Недостаточно памяти.\n");
		return;
	}

	unsigned int dwFlags = 0, t;
	int nCharsCount = 0;

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

	int nWSCounter = 0; // счётчик непрерывной последовательности пробелов
	int tab = pOutFile->getTabWidth();

	while (!pInFile->isEOF())
	{
		wchar_t Sym = pInFile->ReadChar();

		if (pInFile->isEOF())
		{
			break;
		}

		switch (Sym)
		{
			case 0: t = INPUT_SYMBOL_NULL;
				break;

			case 10: t = INPUT_SYMBOL_N;
				break;

			case 13: t = INPUT_SYMBOL_R;
				break;

			default:
			{
				dwFlags = 0;

				if (Sym == L' ')
				{
					nWSCounter++; // считаем идущие подряд пробелы
				}
				else if (Sym == 9) // если встретили табуляцию
				{
					// преобразуем её в пробелы и посчитаем, сколько вышло
					int nNextTabPos = (nCharsCount / pInFile->getTabWidth() + 1) * pInFile->getTabWidth();

					while (nCharsCount < nNextTabPos)
					{
						nWSCounter++;
						nCharsCount++;
					}

					continue;
				}
				else // если кончились пробелы
				{
					if (nWSCounter) // если сколько-то насчитали
					{
						// упаковываем пробелы
						if (nWSCounter == 1)    // если пробел всего 1, то нечего городить
						{
							pOutFile->WriteByte(' '); // выведем всего 1 пробел.
						}
						else if (nCharsCount % tab == 0) // если стоим на позиции табулятора
						{
							do // будем ставить табуляторы, пока не достигнем нужной позиции
							{
								pOutFile->WriteByte(9); // выведем код табуляции.
								nWSCounter -= tab;
							}
							while (nWSCounter > 0);
						}
						else
						{
							// тут всякие сложные случаи.
							// можно бы с ориентацией на табуляции сделать, но нафиг
							// а вообще, как-то не получается по-нормальному сделать
							// 8 - ширина табулятора, а код 8 это ещё и стрелка влево.
							// лучше его не использовать
							do
							{
								uint8_t ncnt = (nWSCounter > 7) ? 7 : nWSCounter;
								nWSCounter -= 7;
								pOutFile->WriteByte(ncnt);
							}
							while (nWSCounter > 0);
						}

						nWSCounter = 0;
					}

					// всё остальное отправляем как есть.
					pOutFile->WriteChar(Sym);
				}

				nCharsCount++;
				continue;
			}
		}

		if ((dwFlags & INPUT_SYMBOL_EOLN) == 0)
		{
			dwFlags |= t;
		}
		else
		{
			if ((dwFlags & t) == 0)
			{
				dwFlags = 0;
				continue;
			}
		}

		pOutFile->WriteChar(0);
		nCharsCount = 0;
	}
}

