#pragma once
#include "LabelManager.h"
#include "ObjParams.h"

namespace ObjManger
{
	bool MakeObj(const fs::path &strName, const std::wstring &strExt, const bool bTbl = false);
	bool LoadObj(const fs::path &strName);
	bool FindBlock(OBJBlockHeader &hdr, OBJTags tag);
	bool SaveIDBlk();
	bool ReadIDBlk(OBJIDBlock &data);
	bool SaveLabelTable(CLabelTable *lbltbl, OBJTags nTag);
	bool LoadLabelTable(CLabelTable *lbltbl, OBJBlockHeader &hdr);
	bool SaveRefsTable(OBJTags nTag);
	bool LoadRefsTable(OBJBlockHeader &hdr);
	bool SaveData();
	bool ReadData(const uint32_t nLen);

	bool SaveScripts();
	bool ReadScripts(const uint32_t nLen);
}
/*
Объектный файл.
имеет теговую структуру.
состоит из блоков:
1. Блок глобальных меток
2. Блок локальных меток (если есть)
3. Блок ссылок на метки.
4. Блок идентификатор
5. Блок скриптов
*/


