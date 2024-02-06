// BKbin2obj.cpp: определяет точку входа для консольного приложения.
//

#include "pch.h"
#include <cwctype>
#include <clocale>
#include "getopt.h"
#include "BKbin2obj.h"
#include "Globals.h"
#include "LabelManager.h"
#include "Object.h"
#include "StringUtil.h"

// #pragma comment (lib,"Gdiplus.lib")
// 
// // Update Manifest
// // cf: http://blogs.msdn.com/b/oldnewthing/archive/2007/05/31/2995284.aspx
// //
// // We use features from GDI+ v1.1 which was new as of Windows Vista. There is no redistributable for Windows XP.
// // This adds information to the .exe manifest to force the GDI+ 1.1 version of gdiplus.dll to be loaded.
// // Without this, Windows will load the GDI+ 1.0 version of gdiplus.dll and the application will fail to launch with missing entry points.
// #ifdef _WIN64
// 	#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.GdiPlus' version='1.1.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
// #else
// 	#pragma comment(linker, "\"/manifestdependency:type='Win32' name='Microsoft.Windows.GdiPlus' version='1.1.0.0' processorArchitecture='X86' publicKeyToken='6595b64144ccf1df' language='*'\"")
// #endif

#pragma warning(disable:4996)

namespace BKBin2Obj
{
	std::wstring m_strType, m_strLabel, m_strFinalLabel;
	fs::path m_strInFileName, m_strOutFileName;
	/*
	опции:
	bColor - чб / цветная картинка
	bTransparency - прозрачность
	*/
	bool m_bColor = false, m_bTransparency = false, m_bEven = false;
	int m_nEvenBound = 0;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[])
{
	setlocale(LC_ALL, "Russian");
	g_Globals.InitGlobalParameters();
	static struct option long_options[] =
	{
		{ L"help",         ARG_NONE, nullptr, L'?' },
		{ L"source",       ARG_REQ,  nullptr, L's' },
		{ L"even",         ARG_OPT,  nullptr, L'e' },
		{ L"label",        ARG_REQ,  nullptr, L'l' },
		{ L"final",        ARG_REQ,  nullptr, L'f' },
		{ L"color",        ARG_NONE, nullptr, L'c' },
		{ L"transparency", ARG_NONE, nullptr, L't' },
		{ nullptr,         ARG_NULL, nullptr, ARG_NULL }
	};
	static wchar_t optstring[] = L"?s:e::l:f:ct";
	bool bShowHelp = false;
	int option_index = 0;
	int c;

	while ((c = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1)
	{
		// Handle options
		c = tolower(c);

		switch (c)
		{
			case L'?':
				bShowHelp = true;
				break;

			case L's':
				if (optarg)
				{
					BKBin2Obj::m_strType = strUtil::strToUpper(std::wstring(optarg));
					BKBin2Obj::m_strType = BKBin2Obj::m_strType.substr(0, 3);
				}

				break;

			case L'e':
				BKBin2Obj::m_bEven = true;

				if (optarg)
				{
					const int nTmp = _tcstol(optarg, nullptr, 8) & 0xffff;

					if (0 <= nTmp && nTmp <= 0100000)
					{
						BKBin2Obj::m_nEvenBound = nTmp;
					}
				}

				break;

			case L'l':
				BKBin2Obj::m_bEven = true;

				if (optarg)
				{
					BKBin2Obj::m_strLabel = strUtil::strToUpper(std::wstring(optarg));
				}

				break;

			case L'f':
				BKBin2Obj::m_bEven = true;

				if (optarg)
				{
					BKBin2Obj::m_strFinalLabel = strUtil::strToUpper(std::wstring(optarg));
				}

				break;

			case L'c':
				BKBin2Obj::m_bColor = true;
				break;

			case L't':
				BKBin2Obj::m_bTransparency = true;
				break;
		}

		if (bShowHelp)
		{
			BKBin2Obj::Usage();
			return 0;
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 1)
	{
		wprintf(L"Ошибка в командной строке: не задано имя входного файла.\n\n");
		BKBin2Obj::Usage();
	}
	else
	{
		BKBin2Obj::m_strInFileName = fs::path(*argv);
		argc--;
		argv++;

		if (argc >= 1)
		{
			BKBin2Obj::m_strOutFileName = fs::path(*argv);
		}
		else
		{
			BKBin2Obj::m_strOutFileName = BKBin2Obj::m_strInFileName;
		}

		if (BKBin2Obj::m_strLabel.empty())
		{
			// если имя метки не было задано
			// сделаем имя метки по умолчанию
			BKBin2Obj::m_strLabel = BKBin2Obj::m_strInFileName.stem().wstring();
		}

		g_Globals.SetLinkMode(LINKING_MODE::CL);

		if (BKBin2Obj::m_strType == L"BIN")
		{
			BKBin2Obj::ConvertBin();
		}
		else if (BKBin2Obj::m_strType == L"IMG")
		{
			BKBin2Obj::ConvertImage();
		}
	}

	return 0;
}

void BKBin2Obj::ConvertImage()
{
	CImage img;

	if (SUCCEEDED(img.Load(m_strInFileName.c_str())))
	{
// 		Gdiplus::Bitmap *bitmap = Gdiplus::Bitmap::FromHBITMAP(HBITMAP(img), nullptr);
// 
// 		Gdiplus::PixelFormat pf = PixelFormat4bppIndexed;
// 		Gdiplus::DitherType dt = Gdiplus::DitherType::DitherTypeNone;
// 		Gdiplus::PaletteType pt = Gdiplus::PaletteType::PaletteTypeOptimal;
// 
// 		bitmap->ConvertFormat(pf, dt, pt, nullptr, 0);
// 		// чё то чересчур сложно и непонятно. ну его пока нафиг
// 		delete bitmap;

		const int w = img.GetWidth();
		const int h = img.GetHeight();
		// посчитаем размер БКшного ресурса при таких размерах картинки.
		int size = 0;

		if (m_bColor)
		{
			// для цветной картинки.
			// 1 пиксел - 2 бита
			size = ((w + 3) / 4) * h * (m_bTransparency ? 2 : 1);
		}
		else
		{
			// для чёрно-белой картинки.
			// 1 пиксел - 1 бит
			size = ((w + 7) / 8) * h * (m_bTransparency ? 2 : 1);
		}

		if (size < 65536)
		{
			if (size >= 32768)
			{
				wprintf(L"Предупреждение! Размер ресурса больше половины доступной памяти БК.\n");
			}

			// конвертируем
			if (m_bColor)
			{
				convertBitmap2(&g_Memory.b[BASE_ADDRESS], &img, m_bTransparency);
			}
			else
			{
				convertBitmap1(&g_Memory.b[BASE_ADDRESS], &img, m_bTransparency);
			}

			CBKToken token(m_strLabel); // добавляем метку
			g_labelGlobalDefs.AddLabel(&token, BASE_ADDRESS, LBL_GLOBAL);

			if (m_bEven)
			{
				if (m_nEvenBound)
				{
					// выравнивание по произвольной границе степени 2
					size = size ? ((size - 1) | (m_nEvenBound - 1)) + 1 : 0;
				}
				else
				{
					// выравниваем по чётному адресу
					size++;
					size &= ~1;
				}
			}

			g_Globals.SetProgramLength(size);
			ObjManger::MakeObj(m_strOutFileName, std::wstring(L".obj"));
		}
		else
		{
			wprintf(L"Размер ресурса слишком велик.\n");
		}

		img.Destroy();
	}
	else
	{
		wprintf(L"Ошибка загрузки файла изображения %s\n", m_strInFileName.c_str());
	}
}


// преобразовываем обычный массив бинарных данных
void BKBin2Obj::ConvertBin()
{
	FILE *inf = _wfopen(m_strInFileName.c_str(), L"rb");

	if (inf)
	{
		// узнаем размер файла
		long size = fs::file_size(m_strInFileName);

		if (size < 65536)
		{
			if (size >= 32768)
			{
				wprintf(L"Предупреждение! Размер ресурса больше половины доступной памяти БК.\n");
			}

			fread(&g_Memory.b[BASE_ADDRESS], 1, size, inf); // читаем файл
			CBKToken token(m_strLabel); // добавляем метку
			g_labelGlobalDefs.AddLabel(&token, BASE_ADDRESS, LBL_GLOBAL);

			if (m_bEven)
			{
				if (m_nEvenBound)
				{
					// выравнивание по произвольной границе степени 2
					size = size ? ((size - 1) | (m_nEvenBound - 1)) + 1 : 0;
				}
				else
				{
					// выравниваем по чётному адресу
					size++;
					size &= ~1;
				}
			}

			if (!m_strFinalLabel.empty())
			{
				CBKToken finalToken(m_strFinalLabel); // добавляем метку
				g_labelGlobalDefs.AddLabel(&finalToken, BASE_ADDRESS + size, LBL_GLOBAL);
			}

			g_Globals.SetProgramLength(size);
			ObjManger::MakeObj(m_strOutFileName, std::wstring(L".obj"));
		}
		else
		{
			wprintf(L"Размер ресурса слишком велик.\n");
		}

		fclose(inf);
	}
	else
	{
		wprintf(L"Ошибка открытия файла %s\n", m_strInFileName.c_str());
	}
}

// функции из pdp11asm vinxru
// это цветная картинка
void BKBin2Obj::convertBitmap2(uint8_t *destp, CImage *img, bool t)
{
	const int w = img->GetWidth();
	const int h = img->GetHeight();
	const int destbpl = (w + 3) / 4 * (t ? 2 : 1);

	for (int y = 0; y < h; ++y)
	{
		const size_t basep = static_cast<size_t>(y) * destbpl;
		for (int x = 0; x < w; ++x)
		{
			COLORREF сolor = img->GetPixel(x, y);

			const bool tr = (сolor == 0xFF00FF); // это у нас прозначный цвет?

			// теперь определим, какой цвет у пикселя
			if ((сolor & 0xff0000) && !(сolor & 0x00f8f8)) // если в синем канале что-то есть, а в остальных почти нету
			{
				сolor = 1; // то это синий цвет
			}
			else if ((сolor & 0x00ff00) && !(сolor & 0xf800f8)) // если в зелёном канале что-то есть, а в остальных почти нету
			{
				сolor = 2; // то это зелёный цвет
			}
			else if ((сolor & 0x0000ff) && !(сolor & 0xf8f800)) // если в красном канале что-то есть, а в остальных почти нету
			{
				сolor = 3; // то это красный цвет
			}
			else
			{
				// все остальные комбинации - чёрный цвет. нефиг тут.
				сolor = 0; // чёрный
			}

			const uint8_t cbit = сolor << ((x & 3) * 2);
			const uint8_t bit = 3 << ((x & 3) * 2);
			const size_t x4 = x / 4;
			const size_t x40 = x4 & ~1;

			if (t)
			{
				if (!tr) // если не прозрачный цвет, то формируем точку
				{
					if (w >= 16)
					{
						destp[basep + x4 + x40 + 2] |= cbit;
						destp[basep + x4 + x40] |= bit;
					}
					else
					{
						destp[basep + x4 * 2 + 1] |= cbit;
						destp[basep + x4 * 2] |= bit;
					}
				}
				// если цвет прозрачный - то ничего не делаем.
			}
			else
			{
				destp[basep + x4] |= cbit;
			}
		}
	}
}

// -----------------------------------------------------------------------------
// это чёрно белая картинка
void BKBin2Obj::convertBitmap1(uint8_t *destp, CImage *img, bool t)
{
	const int w = img->GetWidth();
	const int h = img->GetHeight();
	const size_t destbpl = ((static_cast<size_t>(w) + 7) / 8) * (t ? 2 : 1);

	for (int y = 0; y < h; ++y)
	{
		const size_t basep = static_cast<size_t>(y) * destbpl;
		for (int x = 0; x < w; ++x)
		{
			const uint8_t bit = 1 << (x & 7);
			const size_t x8 = x / 8;
			const size_t x80 = x8 & ~1;
			COLORREF c = img->GetPixel(x, y);
			const bool tr = (c == 0xFF00FF);

			if (c == 0)
			{
				c = 0;
			}
			else
			{
				c = 1;
			}

			if (t)
			{
				if (!tr)
				{
					if (w >= 16)
					{
						if (c)
						{
							destp[basep + x8 + x80 + 2] |= bit;
						}

						destp[basep + x8 + x80] |= bit;
					}
					else
					{
						if (c)
						{
							destp[basep + x8 * 2 + 1] |= bit;
						}

						destp[basep + x8 * 2] |= bit;
					}
				}
			}
			else
			{
				if (c)
				{
					destp[basep + x8] |= c << (x & 7);
				}
			}
		}
	}
}

void BKBin2Obj::Usage()
{
	wprintf(L"Конвертер бинарных объектов в объектные модули кросс ассемблера Turbo8.\n" \
	        L"(с) 2014-2023 gid\n\n" \
	        L"Использование:\n" \
	        L"BKbin2obj -? (--help)\n" \
	        L"  Вывод этой справки.\n\n" \
	        L"BKbin2obj [-s<type>][-c][-t][-e[0bound]][-l<label_name>][-f<label_name>] <input_file_name> [output_file_name]\n" \
	        L"  -s<source> (--source <source>) - тип входного объекта.\n" \
	        L"    Возможные типы:\n" \
	        L"    bin - просто бинарный массив;\n" \
	        L"    img - картинка в формате BMP, GIF, JPEG, PNG и TIFF.\n\n" \
	        L"  -e[0bound] (--even[=0bound]) - выравнивание массива данных по границе блока.\n" \
	        L"    Если параметр не задан - делается выравнивание по границе слова.\n" \
	        L"    Параметр - число в восьмеричном виде. Предполагается, что число - степень\n" \
	        L"    двойки. Если задать произвольное число, результат будет совсем не тем, что\n" \
	        L"    ожидался.\n\n" \
	        L"  -l<label_name> (--label <label_name>) - задать имя метки. Если имя метки не\n" \
	        L"    задано, оно формируется из имени входного файла.\n\n" \
	        L"  -f<label_name> (--final <label_name>) - задать имя финальной метки в конце.\n" \
	        L"    Если задан ключ -e, то метка ставится в конце выравнивания.\n\n" \
	        L"  Ключи, действующие только при выборе типа img:\n" \
	        L"  -c (--color) - обрабатывать картинку как цветное изображение, иначе - чёрно-\n" \
	        L"    белое.\n\n" \
	        L"  -t (--transparency) - использование прозрачности.\n" \
	        L"    Алгоритмы преобразования взяты из проекта pdp11asm vinxru, один к одному,\n" \
	        L"    я даже не разбирался как они работают.\n\n" \
	        L"  input_file_name - входной файл.\n" \
	        L"  output_file_name - необязательное имя выходного файла, если нужно задать\n" \
	        L"  объектному файлу имя, отличное от входного.\n");
}

/*
конвертор всяких бинарных объектов в объектные файлы, чтобы их прилинковывать к
ассемблерным объектникам.
*/

/*
Конвертирование картинок.
1. Загружаем картинку используя класс CImage
1.1 Проверяем, BPP картинки совпадает с целевым BPP?
1.2 Если да и если CImage::IsDIBSection, то можно извлекать индексы цветов прямо из CImage
1.3 Тут надо будет проверить может ли вообще эта фигня работать.
2. Создаём объект Bitmap и передаём туда картинку (если это возможно.)
3. В соответствии с выбранным целевым BPP, конвертируем методом Bitmap::ConvertFormat
4. Достаём попиксельно пиксели из битмапа и формируем массив данных.

*/
