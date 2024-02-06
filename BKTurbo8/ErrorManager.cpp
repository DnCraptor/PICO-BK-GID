#include "pch.h"
#include "ErrorManager.h"
#include "Globals.h"
#include "Listing.h"

#pragma warning(disable:4996)

static const ErrorManager::BKTError_t g_Errors[] =
{
	{ ERRNUM::E_101, L"Псевдокоманда .LA должна быть первой в тексте." },
	{ ERRNUM::E_102, L"Ошибка длины или направления перехода в команде SOB." },
	{ ERRNUM::E_103, L"Недопустимый символ в строке / Синтаксическая ошибка." },
	{ ERRNUM::E_104, L"Ошибка или отсутствие числового аргумента." },
	{ ERRNUM::E_105, L"Неправильная псевдокоманда." },
	{ ERRNUM::E_106, L"Неправильная ассемблерная инструкция." },
	{ ERRNUM::E_107, L"Отсутствует символ после \'." },
	{ ERRNUM::E_108, L"Отсутствуют или недостаточно символов после \"." },
	{ ERRNUM::E_109, L"Ошибка длины перехода по оператору ветвления." },
	{ ERRNUM::E_110, L"Ошибка аргумента MARK." },
	{ ERRNUM::E_111, L"Ошибка в имени регистра." },
	{ ERRNUM::E_112, L"Ошибка в псевдокоманде." },
	{ ERRNUM::E_113, L"Метка уже определена ранее." },
	{ ERRNUM::E_114, L"Аргумент .BLKB слишком велик." },
	{ ERRNUM::E_115, L"Аргумент .BLKW слишком велик." },
	{ ERRNUM::E_116, L"Аргумент .ORG слишком велик." },
	{ ERRNUM::E_117, L"Нечётный адрес команды." },
	{ ERRNUM::E_118, L"Отсутствует .END" },
	{ ERRNUM::E_119, L"Неопределённая метка в непосредственном выражении." },
	{ ERRNUM::E_120, L"Отсутствует переход у команды SOB." },
	{ ERRNUM::E_121, L"Ошибка аргумента TRAP." },
	{ ERRNUM::E_122, L"Ошибка аргумента EMT." },
	{ ERRNUM::E_123, L"Ошибка или неверный метод адресации." },
	{ ERRNUM::E_124, L"Отсутствует второй операнд." },
	{ ERRNUM::E_125, L"Переполнение байтового аргумента." },
	{ ERRNUM::E_126, L"Переполнение словного аргумента." },
	{ ERRNUM::E_127, L"Неожиданный конец строкового аргумента." },
	{ ERRNUM::E_128, L"Ошибка в числе с плавающей точкой." },
	{ ERRNUM::E_129, L"Невозможно открыть файл include." },
	{ ERRNUM::E_130, L"Ошибка в аргументах псевдокоманды." },
	{ ERRNUM::E_131, L"Ошибка в числовом аргументе." },
	{ ERRNUM::E_132, L"Ошибка в имени регистра FPU." },
	{ ERRNUM::E_133, L".ENDS без .SCRIPT." },
	{ ERRNUM::E_134, L"Неожиданный конец строки." },
};

int ErrorManager::IsError()
{
	return g_Globals.GetError();
}

/*
вывод сообщения об ошибке.
n - номер ошибки
boutstr true - вывести строку, в которой случилась ошибка,
*/
wchar_t errbuf[65536] = { 0 };
wchar_t errbuf2[65536] = { 0 };
void ErrorManager::OutError(ERRNUM n, bool bOutStr, int nAddr)
{
	FILE *errf = _wfopen(L"_errors.txt", L"a");
	/*
	Тут в общем такая ситуация:
	nAddr введён только для того, чтобы правильно отслеживать ошибку длины перехода
	по ветвлению вперёд.
	Во всех случаях nAddr == -1 == значение по умолчанию.
	и только в одном единственном случае возникновения ошибки перехода по ветвлению вперёд
	nAddr - это адрес команды, которая вызвала ошибку 110

	!!! Если возникнет ещё какая нибудь такая ситуация, придётся всё переделывать.

	*/
	auto &error = g_Errors[n - ERRNUM::E_101];

	// при линковке листинг не создаётся, и тут получается всё весьма хреново.
	if (g_Listing.empty())
	{
		if (nAddr == -1)
		{
			nAddr = g_Globals.GetPC();
		}

		// и приятный бонус. теперь и тут всё правильно отображается для ошибки 110 по переходу вперёд.
		_swprintf(errbuf2, L"Error %3d: %s\n", error.num, error.str.c_str());
		_swprintf(errbuf, L"Line none (Addr: %07o)", g_Globals.GetRealAddress(nAddr));
		wprintf(L"%s - %s", errbuf, errbuf2);

		if (errf)
		{
			fwprintf(errf, L"%s :: %s - %s", g_pReader->GetFileName().c_str(), errbuf, errbuf2);
		}

		if (errf)
		{
			fclose(errf);
		}

		g_Globals.SetError(g_Globals.GetError() + 1);
		return;
	}

	auto pll = g_Listing.end() - 1;

	if (nAddr == -1)
	{
		nAddr = g_Globals.GetPC();
	}
	else if (n == E_109)
	{
		// нужно теперь найти, в какое место вставить текст ошибки в листинге
		bool bEx = false;

		for (auto ll = g_Listing.begin(); ll != g_Listing.end(); ++ll)
		{
			for (auto &lct : ll->vCMD)
			{
				if (lct.nPC == nAddr && lct.LT == ListType::LT_INSTRUCTION)
				{
					pll = ll;
					bEx = true;
					break;
				}
			}

			if (bEx)
			{
				break;
			}
		}
	}

	// и приятный бонус. теперь и тут всё правильно отображается для ошибки 110 по переходу вперёд.
	_swprintf(errbuf2, L"Error %3d: %s\n", error.num, error.str.c_str());
	_swprintf(errbuf, L"Line %d (Addr: %07o)", pll->nLineNum, g_Globals.GetRealAddress(nAddr));
	pll->errors.emplace_back(std::wstring(errbuf2)); // добавляем в листинг ошибку
	wprintf(L"%s - %s", errbuf, errbuf2);

	if (errf)
	{
		fwprintf(errf, L"%s :: %s - %s", g_pReader->GetFileName().c_str(), errbuf, errbuf2);
	}

#if (DEBUG_LABEL_MANAGER)
	// /-----------------------debug---------------------------
	fwprintf(dbgF, L"%s", errbuf);
	// /-----------------------debug---------------------------
#endif

	// если нужно - ещё и строку покажем, где ошибка
	if (bOutStr)
	{
		wprintf(L"%s\n", pll->line.c_str());

		if (errf)
		{
			fwprintf(errf, L"%s\n", pll->line.c_str());
		}

#if (DEBUG_LABEL_MANAGER)
		// /-----------------------debug---------------------------
		fwprintf(dbgF, L"%s\n", pll->line.c_str());
		// /-----------------------debug---------------------------
#endif
	}

	if (errf)
	{
		fclose(errf);
	}

	g_Globals.SetError(g_Globals.GetError() + 1);
}

