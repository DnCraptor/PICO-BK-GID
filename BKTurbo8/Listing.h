#pragma once

enum class ListType : int
{
	LT_BLANKLINE,
	LT_COMMENTARY,  // комментарий
	LT_ASSIGN,      // присваивание
	LT_ASSIGNREG,   // присваивание синонима имени регистра
	LT_LABEL,       // метка
	LT_INSTRUCTION, // обычная ассемблерная инструкция
	LT_PSC_ADDR,    // псевдокоманда ADDR
	LT_PSC_LA,      // псевдокоманда LA
	LT_PSC_PRINT,   // псевдокоманда PRINT
	LT_PSC_BLKW,    // псевдокоманда BLKW
	LT_PSC_BLKB,    // псевдокоманда BKLB
	LT_PSC_WORD,    // псевдокоманда WORD
	LT_PSC_BYTE,    // псевдокоманда BYTE
	LT_PSC_END,     // псевдокоманда END
	LT_PSC_EVEN,    // псевдокоманда EVEN
	LT_PSC_RAD50,   // псевдокоманда RAD50
	LT_PSC_ASCII,   // псевдокоманда ASCII,ASCIIZ
	LT_PSC_FLT2,    // псевдокоманда FLT2
	LT_PSC_FLT4,    // псевдокоманда FLT4
};

namespace ListingManager
{
	struct ListingCmdType
	{
		int nPC;        // адрес с которого начинаются бинарные данные
		int nEndAdr;    // адрес на котором кончаются бинарные данные
		ListType LT;    // тип команды в строке
	};

	struct ListingLine
	{
		int nLineNum;                       // номер строки
		std::wstring line;                  // сама строка
		std::vector<std::wstring> errors;   // список ошибок.
		std::vector <ListingCmdType> vCMD;  // список команд в строке

		ListingLine() : nLineNum(0)
		{}
	};

	void PrepareLine(const int cp);
	void AddPrepareLine(const int cp, const ListType lt);
	void AddPrepareLine2(const int len);
	void OutLineNum(FILE *of, const int nOut, const int num);
	void OutAddress(FILE *of, const int nOutLN, const bool bOut, const int num, const int addr);
	void OutWords(FILE *of, const int nOutLN, const bool bOutAddr, const bool bOut, const int num, int &bgn_addr, const int end_addr);
	void OutBytes(FILE *of, const int nOutLN, const bool bOutAddr, const bool bOut, const int num, int &bgn_addr, const int end_addr);
	void OutString(FILE *of, std::wstring *str = nullptr);
	void OutLineString(FILE *of, const int nCnt, std::wstring *str = nullptr);
	void MakeListing(const fs::path &strName, const fs::path &strExt);
}

extern std::vector <ListingManager::ListingLine> g_Listing;


/*
Генерация листинга делается в 2 этапа
1. при первом проходе собирается информация о строках, и их содержимом.

2. если ошибок нет и мы делаем окончательную компоновку, генерируется
листинг, данные берутся из массива скомпиленных данных и выводятся
в отформатированном виде в зависимости от типа команды в строке.

*/
