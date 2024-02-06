#include "pch.h"
#include "bk2ibm.h"

void ITCPC::BK2IBM(CBaseFile *pInFile, CBaseFile *pOutFile)
{
	if (pInFile == nullptr || pOutFile == nullptr)
	{
		wprintf(L"Недостаточно памяти.\n");
		return;
	}

	int counter = 0, tabs = 0;

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

	if (pInFile->ReadWord() == 0x0c40)
	{
		// это мы пропускаем всякие шрифты и доп.модули
		counter = pInFile->ReadWord();
	}

	pInFile->SetPos(counter);
	counter = 0; // счётчик символов в строке

	while (!pInFile->isEOF())
	{
		wchar_t tch = pInFile->ReadChar();

		if (pInFile->isEOF())
		{
			break;
		}

		if (tch == 0 || tch == 10) // конец строки
		{
			pOutFile->WriteChar(L'\n');
			counter = 0;
			tabs = 0;
			continue;
		}

		if (tch < 9) // обычно это просто количество пробелов, но бывает и количество табуляторов.
		{
			// если на входе tabs == 0, то tch - это кол-во пробелов.
			// tabs != 0 будет в единственном случае, когда сюда попадаем повторно сразу же,
			// т.е. два подряд символа с кодом меньше 9.
			// в этом случае второй символ - уже количество табуляций
			// третий и последующие подряд - снова кол-во табуляций, хотя скорее всего это невозможно,
			// т.к. ширина таб == 8, то 8 табуляторов - уже вся БКшная строка.
			if (tabs == 0)
			{
				tabs++;

				for (wchar_t i = 0; i < tch; ++i)
				{
					pOutFile->WriteChar(L' '); counter++;
				}
			}
			else
			{
				for (wchar_t i = 0; i < tch; ++i)
				{
					for (uint8_t j = 0; j < 8; ++j) // для vxt - строго на 8 пробелов. это не совсем табуляция
					{
						pOutFile->WriteChar(L' '); counter++;
					}
				}
			}

			continue;
		}

		if (tch == 9) // табуляция, по 8 символов на табулятор
		{
			int nNextTabPos = (counter / pInFile->getTabWidth() + 1) * pInFile->getTabWidth();

			do
			{
				pOutFile->WriteChar(L' '); counter++;
			}
			while (counter < nNextTabPos);

			tabs = 0;
			continue;
		}

		if (tch < 32)
		{
			tabs = 0;
			continue; // эти коды в vxt используются для визуального форматирования, типа жирный текст, курсив и т.п.
		}

		tabs = 0;
		pOutFile->WriteChar(tch); counter++;
	}
}

