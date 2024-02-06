#pragma once
#include "BaseFile.h"

constexpr auto INPUT_SYMBOL_NULL = 1;
constexpr auto INPUT_SYMBOL_N = 2;
constexpr auto INPUT_SYMBOL_R = 4;
constexpr auto INPUT_SYMBOL_EOLN = (INPUT_SYMBOL_R | INPUT_SYMBOL_N | INPUT_SYMBOL_NULL);
constexpr auto INPUT_FLAG_ABSATZ = 8;

class CBKFile : public CBaseFile
{

	public:
		CBKFile(const fs::path &strName);
		virtual ~CBKFile() override;

		virtual bool    Open(bool bWrite) override; // открытие файла, имя которого задано в конструкторе
		virtual wchar_t ReadChar() override;
		virtual bool    WriteChar(wchar_t ch) override;
};

