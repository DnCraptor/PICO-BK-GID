﻿#pragma once
#include "BaseFile.h"

class CWinFile : public CBaseFile
{
	public:
		enum class FILE_CHARSET {UNDEFINE, KOI8, CP866, CP1251, UTF8, UTF16LE};

	protected:
		FILE_CHARSET    m_nManualCharset;
		FILE_CHARSET    AnalyseCharset(); // анализ кодировки.
		wchar_t         ReadUTF8Char();
		std::wstring    GetFormatName(FILE_CHARSET fchr);

		/*
		   Используется алгоритм автоматического определения кодировки текста (ALT, WIN, KOI)
		   Описание алгоритма: http://ivr.webzone.ru/articles/defcod_2/
		   (c) Иван Рощин, Москва, 2004.
		*/

		static const uint8_t    table_2s[128];
		/* =========================================================================
		Вспомогательная функция alt2num.
		Вход: a - код русской буквы в кодировке ALT.
		Выход: порядковый номер этой буквы (0-31).
		========================================================================= */
		int alt2num(int a)
		{
			if (a >= 0xE0)
			{
				a -= 0x30;
			}

			return (a & 31);
		}
		/* =========================================================================
		Вспомогательная функция koi2num.
		Вход: a - код русской буквы в кодировке KOI.
		Выход: порядковый номер этой буквы (0-31).
		========================================================================= */

		int koi2num(int a)
		{
			static const uint8_t t[32] = { 30, 0, 1, 22, 4, 5, 20, 3, 21, 8, 9, 10, 11, 12, 13, 14, 15, 31,
			                               16, 17, 18, 19, 6, 2, 28, 27, 7, 24, 29, 25, 23, 26
			                             };
			return (t[a & 31]);
		}

		/* =========================================================================
		Вспомогательная функция work_2s - обработка двухбуквенного сочетания.
		Вход:  с1 - порядковый номер первой буквы (0-31),
		c2 - порядковый номер второй буквы (0-31),
		check - надо ли проверять, встречалось ли сочетание раньше
		(1 - да, 0 - нет),
		buf - адрес массива с информацией о встреченных сочетаниях.
		Выход: 0 - указанное сочетание уже встречалось раньше,
		1 - сочетание не встречалось раньше и является допустимым,
		2 - сочетание не встречалось раньше и является недопустимым.
		========================================================================= */
		int             work_2s(int c1, int c2, int check, uint8_t buf[128]);

		/* =========================================================================
		Вспомогательная функция def_code - определение кодировки текста.
		Вход:
		n - количество различных сочетаний русских букв (1-255), которого
		достаточно для определения кодировки.
		Выход: определённая кодировка
		========================================================================= */
		FILE_CHARSET    def_code(int n = 128);
		int             get_char();
		size_t          m_nLen;

	public:
		CWinFile(const fs::path &strName, FILE_CHARSET nManualCharSet = FILE_CHARSET::UNDEFINE);
		virtual ~CWinFile() override;

		virtual bool    Open(bool bWrite = false) override; // открытие файла, имя которого задано в конструкторе
		virtual wchar_t ReadChar() override;
		virtual bool    WriteChar(wchar_t ch) override;

};

