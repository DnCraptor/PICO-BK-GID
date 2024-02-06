#pragma once

struct workMode
{
	wchar_t     chInM;  // формат входного файла
	wchar_t     chOutM; // формат выходного файла
	int         nTabWidth;
	bool        bRecursive;
	fs::path    strInFileName;
	fs::path    strOutFileName;
	workMode() : chInM(L'A'), chOutM(L'A'), nTabWidth(4), bRecursive(false) {}
};

namespace ITCPC
{
	void        Usage();
	void        Process(const fs::path &file);
	bool        CheckInputParams();
	std::wstring GetFormatName(const wchar_t ch);
	void        MakeOutName();
}
