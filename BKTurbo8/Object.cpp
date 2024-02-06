#include "pch.h"
#include "Object.h"
#include "Globals.h"
#include "ScriptAsm.h"
#include "BKToken.h"

#pragma warning(disable:4996)

FILE *objFile = nullptr;

bool ObjManger::MakeObj(const fs::path &strName, const std::wstring &strExt, const bool bTbl)
{
	// сделаем новое имя файла
	fs::path outName = strName;
	outName.replace_extension(strExt);
	objFile = _wfopen(outName.c_str(), L"wb");

	if (objFile == nullptr)
	{
		wprintf(L"Ошибка создания файла %s\n", outName.c_str());
		return false;
	}

	SaveIDBlk();

	if (bTbl) // когда создаём таблицу меток, сохраняем только глобальные метки
	{
		//!!! когда создаём таблицу меток, надо модифицировать таблицу. все метки превратить в определения и сделать им реальные адреса
		CLabelTable tblLabelTable;
		auto sz = g_labelGlobalDefs.getSize();

		if (sz)
		{
			for (size_t n = 0; n < sz; ++n)
			{
				Label lbl(g_labelGlobalDefs.GetElement(n));

				if ((lbl.getType() & LBL_DEFINITE_MASK) == LBL_GLOBAL)
				{
					int value = lbl.getValue();
					value = g_Globals.GetRealAddress(value);
					lbl.setValue(value);
					lbl.setType(LBL_DEFINE);
				}

				tblLabelTable.AddLabel(lbl);
			}

			SaveLabelTable(&tblLabelTable, OBJTags::OBJ_GLB);
			tblLabelTable.Clear();
		}
	}
	else
	{
		SaveData();
		SaveLabelTable(&g_labelGlobalDefs, OBJTags::OBJ_GLB);
		SaveLabelTable(&g_labelLocalDefs, OBJTags::OBJ_LOC);
		SaveRefsTable(OBJTags::OBJ_REF);
		SaveScripts();
	}

	return true;
}

OBJIDBlock g_loadID;
bool ObjManger::LoadObj(const fs::path &strName)
{
	objFile = _wfopen(strName.c_str(), L"rb");

	if (objFile)
	{
		if (ReadIDBlk(g_loadID))
		{
			if (g_loadID.nVersion == OBJVERSION)
			{
				if (g_Globals.GetLinkerAddr() == BASE_ADDRESS)
				{
					g_Globals.SetStartAddress(int(g_loadID.nStartAddress) & 0xffff); // стартовый адрес берём только у первого модуля не нулевой длины.
				}

				bool bRes = true;
				OBJBlockHeader hdr;

				if (FindBlock(hdr, OBJTags::OBJ_GLB))
				{
					bRes = bRes && LoadLabelTable(&g_labelGlobalDefs, hdr);
				}

				if (FindBlock(hdr, OBJTags::OBJ_LOC))
				{
					bRes = bRes && LoadLabelTable(&g_labelLocalDefs, hdr);
				}

				if (FindBlock(hdr, OBJTags::OBJ_REF))
				{
					bRes = bRes && LoadRefsTable(hdr);
				}

				if (FindBlock(hdr, OBJTags::OBJ_SCRIPT))
				{
					bRes = bRes && ReadScripts(hdr.nEntryCount);
				}

				// данные подчитывать после меток, т.к. g_GlobalParameters.nProgramLength тут корректируется
				if (FindBlock(hdr, OBJTags::OBJ_DATA))
				{
					bRes = bRes && ReadData(hdr.nBlkLen);
				}

				return bRes;
			}

			// выведем предупреждение
			wprintf(L"LINK:: Не поддерживаемая версия объектного файла.\n");
		}

		fclose(objFile);
	}

	return false;
}



bool ObjManger::SaveIDBlk()
{
	//long fp = ftell(objFile);
	const OBJBlockHeader hdr
	{
		OBJTags::OBJ_HDRTAG, // nHdrTag
		OBJTags::OBJ_ID,    // nBlockTag
		sizeof(OBJIDBlock), // nBlkLen
		1,                  // nEntryCount
		0                   // nCheckSum
	};
	const OBJIDBlock data
	{
		OBJVERSION, // nVersion
		static_cast<int16_t>(g_Globals.GetStartAddress()), // nStartAddress - адрес запуска
		static_cast<int16_t>(g_Globals.GetLinkMode())   // nMode - режим компоновки
	};
	fwrite(&hdr, 1, sizeof(hdr), objFile);
	fwrite(&data, 1, sizeof(data), objFile);
	return true;
}

bool ObjManger::FindBlock(OBJBlockHeader &hdr, OBJTags tag)
{
	fseek(objFile, 0, SEEK_SET);

	while (true)
	{
		fread(&hdr, 1, sizeof(hdr), objFile);

		if (feof(objFile))
		{
			break;
		}

		if (hdr.nHdrTag == OBJTags::OBJ_HDRTAG)
		{
			if (hdr.nBlockTag == tag)
			{
				return true;
			}

			if (fseek(objFile, hdr.nBlkLen, SEEK_CUR))
			{
				break;
			}
		}
	}

	return false;
}

bool ObjManger::ReadIDBlk(OBJIDBlock &data)
{
	OBJBlockHeader hdr;

	if (FindBlock(hdr, OBJTags::OBJ_ID))
	{
		if (sizeof(data) == fread(&data, 1, sizeof(data), objFile))
		{
			return true;
		}
	}

	// выведем предупреждение
	wprintf(L"LINK:: Не найден блок заголовка объектного файла.\n");
	return false;
}


bool ObjManger::SaveData()
{
	const int dataLen = g_Globals.GetProgramLength();
	const OBJBlockHeader hdr
	{
		OBJTags::OBJ_HDRTAG, // nHdrTag
		OBJTags::OBJ_DATA,  // nBlockTag
		static_cast<uint32_t>(dataLen), // nBlkLen
		1,                  // nEntryCount
		0                   // nCheckSum
	};
	fwrite(&hdr, 1, sizeof(hdr), objFile);

	if (dataLen)
	{
		fwrite(&g_Memory.b[BASE_ADDRESS], 1, dataLen, objFile);
	}

	return true;
}

bool ObjManger::ReadData(const uint32_t nLen)
{
	if (nLen)
	{
		if (BASE_ADDRESS + g_Globals.GetLinkerAddr() + nLen < HIGH_BOUND)
		{
			fread(&g_Memory.b[BASE_ADDRESS + g_Globals.GetLinkerAddr()], 1, nLen, objFile);
			g_Globals.SetLinkObjLength(nLen);
			return true;
		}

		wprintf(L"Критическая ошибка:: Достигнут предел свободной памяти\n");
		return false;
	}

	return true;
}

bool ObjManger::SaveLabelTable(CLabelTable *lbltbl, OBJTags nTag)
{
	const size_t sz = lbltbl->getSize();

	if (sz > 0)
	{
		const long fp = ftell(objFile);
		OBJBlockHeader hdr
		{
			OBJTags::OBJ_HDRTAG,        // nHdrTag
			nTag,                       // nBlockTag
			0,                          // nBlkLen
			static_cast<uint32_t>(sz),  // nEntryCount
			0                           // nCheckSum
		};
		fwrite(&hdr, 1, sizeof(hdr), objFile);

		for (size_t i = 0; i < sz; ++i)
		{
			Label &l = lbltbl->GetElement(i);
			l.Store(objFile);
		}

		// как посчитать контрольную сумму пока неясно.
		const long efp = ftell(objFile);
		hdr.nBlkLen = efp - fp - static_cast<long>(sizeof(hdr));
		fseek(objFile, fp, SEEK_SET);
		fwrite(&hdr, 1, sizeof(hdr), objFile); // скорректируем размер блока
		fseek(objFile, efp, SEEK_SET);
	}

	return true;
}

bool ObjManger::LoadLabelTable(CLabelTable *lbltbl, OBJBlockHeader &hdr)
{
	// предполагается, что мы прочитали заголовок блока, и знаем нужную таблицу
	// и указатель стоит на начале массива
	uint32_t num = 0; // буфер, куда читаем данные
	const uint32_t nLen = hdr.nEntryCount;

	for (uint32_t i = 0; i < nLen; ++i)
	{
		Label l;

		if (l.Load(objFile))
		{
			// здесь надо корректировать адреса меток
			int value = l.getValue();
			const uint32_t type = l.getType();

			if (hdr.nBlockTag == OBJTags::OBJ_REF) // в ссылках на метки корректируем вообще всё
			{
				// для ссылок на метки надо ко всем прибавить g_GlobalParameters.nProgramLength - BASE_ADDRESS
				value += g_Globals.GetLinkerAddr() - BASE_ADDRESS;
			}
			else
			{
				// в таблицах меток надо делать коррекцию в зависимости от режима компоновки
				if (g_loadID.nMode == static_cast<uint16_t>(LINKING_MODE::CL)) // какой режим компоновки?
				{
					// режим CL, тут надо корректировать всё, кроме определений
					if ((type & LBL_DEFINITE_MASK) == LBL_GLOBAL || (type & LBL_DEFINITE_MASK) == LBL_LOCAL)
					{
						value += g_Globals.GetLinkerAddr() - BASE_ADDRESS;
					}

					// константы надо оставлять нетронутыми
				}
				else
				{
					// режим CO, тут надо корректировать только ссылки на метки

					// если таблица меток - то их адреса надо по-особому корректировать
					if ((type & LBL_DEFINITE_MASK) == LBL_GLOBAL || (type & LBL_DEFINITE_MASK) == LBL_LOCAL)
					{
						value += (g_loadID.nStartAddress - g_Globals.GetStartAddress());
					}

					// константы надо оставлять нетронутыми
				}
			}

			value &= 0xffff; // не давать переполнения 16 разрядов.
			l.setValue(value);

			if (!lbltbl->AddLabel(l))
			{
				// добавим немного интеллекту
				const int n = lbltbl->SearchLabel(l.getPToken()); // найдём уже существующую метку

				if (lbltbl->GetValue(n) != value && (type & LBL_DEFINITE_MASK) != LBL_WEAKDEFINE)  // если значение её не совпадает со значением новой метки
				{
					// выведем предупреждение
					wprintf(L"LINK:: Метка '%s' уже существует. Повторная метка не добавлена.\nИсполняемый модуль может быть с ошибкой.\n", l.getToken().getName().c_str());
				}
			}
		}
		else
		{
			return false;
		}
	}

	return true;
}


bool ObjManger::SaveRefsTable(OBJTags nTag)
{
	const size_t sz = g_RefsTable.getSize();

	if (sz > 0)
	{
		const long fp = ftell(objFile);
		OBJBlockHeader hdr
		{
			OBJTags::OBJ_HDRTAG,        // nHdrTag
			nTag,                       // nBlockTag
			0,                          // nBlkLen
			static_cast<uint32_t>(sz),  // nEntryCount
			0                           // nCheckSum
		};
		fwrite(&hdr, 1, sizeof(hdr), objFile);

		for (size_t i = 0; i < sz; ++i)
		{
			LabelRef &l = g_RefsTable.GetElement(i);
			l.Store(objFile);
		}

		// как посчитать контрольную сумму пока неясно.
		const long efp = ftell(objFile);
		hdr.nBlkLen = efp - fp - static_cast<long>(sizeof(hdr));
		fseek(objFile, fp, SEEK_SET);
		fwrite(&hdr, 1, sizeof(hdr), objFile); // скорректируем размер блока
		fseek(objFile, efp, SEEK_SET);
	}

	return true;
}

bool ObjManger::LoadRefsTable(OBJBlockHeader &hdr)
{
	// предполагается, что мы прочитали заголовок блока, и знаем нужную таблицу
	// и указатель стоит на начале массива
	uint32_t num = 0; // буфер, куда читаем данные
	const uint32_t nLen = hdr.nEntryCount;

	for (uint32_t i = 0; i < nLen; ++i)
	{
		LabelRef l;

		if (l.Load(objFile))
		{
			// здесь надо корректировать адреса меток
			int value = l.getAddress();
			const uint32_t type = l.getType();
			// в ссылках на метки корректируем вообще всё
			// для ссылок на метки надо ко всем прибавить g_GlobalParameters.nProgramLength - BASE_ADDRESS
			value += g_Globals.GetLinkerAddr() - BASE_ADDRESS;
			l.setAddress(value);
			g_RefsTable.AddRefs(l);
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool ObjManger::SaveScripts()
{
	if (!g_ScriptDefs.empty())
	{
		OBJBlockHeader hdr
		{
			OBJTags::OBJ_HDRTAG, // nHdrTag
			OBJTags::OBJ_SCRIPT, // nBlockTag
			0,                  // nBlkLen
			static_cast<uint32_t>(g_ScriptDefs.size()), // nEntryCount
			0                   // nCheckSum
		};
		const long fp = ftell(objFile);
		fwrite(&hdr, 1, sizeof(hdr), objFile);
		g_ScriptAsm.Store(objFile);
		const long efp = ftell(objFile);
		hdr.nBlkLen = efp - fp - static_cast<long>(sizeof(hdr));
		fseek(objFile, fp, SEEK_SET);
		fwrite(&hdr, 1, sizeof(hdr), objFile); // скорректируем размер блока
		fseek(objFile, efp, SEEK_SET);
	}

	return true;
}

bool ObjManger::ReadScripts(const uint32_t nLen)
{
	return g_ScriptAsm.Load(nLen, objFile);
}

