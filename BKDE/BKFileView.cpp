#include "pch.h"
#include "resource.h"
#include "BKImage.h"
#include "SprWnd.h"
#include "StringUtil.h"

// тут будут функции для просмотра файлов
#pragma warning(disable:4996)
static void PutTchar(FILE *f, wchar_t tch, int &counter)
{
	fwrite(&tch, 1, sizeof(tch), f);
	counter++;
	fflush(f);
}

// вход: pBuff - указатель на буфер.
//      pos - текущая позиция в буфере
static uint8_t GetByte(uint8_t *pBuff, int &pos)
{
	return *(pBuff + (pos++));
}

// вход: pBuff - указатель на буфер.
//      pos - текущая позиция в буфере
static uint16_t GetWord(uint8_t *pBuff, int &pos)
{
	uint16_t w = GetByte(pBuff, pos) | (GetByte(pBuff, pos) << 8);
	return w;
}

constexpr auto TAB_WIDTH = 8;

bool CBKImage::ViewFile(BKDirDataItem *fr)
{
	bool bRet = false;

	// если это не файл - выходим ничего не делая
	if (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LOGDISK))
	{
		return true;
	}

	fs::path strFName = imgUtil::SetSafeName(fr->strName);
	std::wstring strExt = strUtil::strToUpper(strFName.extension().wstring()); // узнаем расширение
	strFName = fs::temp_directory_path() / strFName;
	FILE *pFView;

	if ((pFView = _tfopen(strFName.c_str(), L"wb")))
	{
		const int len = fr->nSize;
		int pos = 0;
		auto uFBuffer = std::vector<uint8_t>(m_pFloppyImage->EvenSizeByBlock(len));
		const auto pFBuffer = uFBuffer.data();

		if (pFBuffer)
		{
			if (m_pFloppyImage->ReadFile(fr, pFBuffer))
			{
				uint8_t b;
				int counter = 0;
				wchar_t tch = 0xfeff; // BOM

				if (strExt == L".VXT" || strExt == L".LST")
				{
					PutTchar(pFView, tch, counter); // запишем сперва BOM

					if (GetWord(pFBuffer, pos) == 0x0c40)
					{
						// это мы пропускаем всякие шрифты и т.п.
						counter = GetWord(pFBuffer, pos);
					}
					else
					{
						counter = 0;
					}

					// имитируем перемещение на нужную позицию относительно начала файла
					int tabs = 0;
					pos = counter;
					counter = 0; // счётчик символов в строке

					while (pos < len)
					{
						b = GetByte(pFBuffer, pos);

						if (pos >= len) // доп.проверка на всякий случай
						{
							break;
						}

						if (b == 0 || b == 10) // конец строки
						{
							PutTchar(pFView, L'\r', counter);
							counter = -1;
							tch = L'\n';
						}
						else if (b < 9) // обычно это просто количество пробелов, но бывает и количество табуляторов.
						{
							// если на входе tabs == 0, то b - это кол-во пробелов.
							// tabs != 0 будет в единственном случае, когда сюда попадаем повторно сразу же,
							// т.е. два подряд символа с кодом меньше 9.
							// в этом случае второй символ - уже количество табуляций
							// третий и последующие подряд - снова кол-во табуляций, хотя скорее всего это невозможно,
							// т.к. ширина таб == 8, то 8 табуляторов - уже вся БКшная строка.
							if (tabs == 0)
							{
								tabs++;

								for (int i = 0; i < b; ++i)
								{
									PutTchar(pFView, L' ', counter);
								}
							}
							else
							{
								for (int i = 0; i < b; ++i)
								{
									for (int j = 0; j < 8; ++j) // для vxt - строго на 8 пробелов. это не совсем табуляция
									{
										PutTchar(pFView, L' ', counter);
									}
								}
							}

							continue;
						}
						else if (b == 9) // табуляция, по 8 символов на табулятор
						{
							int nNextTabPos = (counter / TAB_WIDTH + 1) * TAB_WIDTH;

							do
							{
								PutTchar(pFView, L' ', counter);
							}
							while (counter < nNextTabPos);

							tabs = 0;
							continue;
						}
						else if (b < 32)
						{
							tabs = 0;
							continue; // эти коды в vxt используются для визуального форматирования, типа жирный текст, курсив и т.п.
						}
						else
						{
							tch = imgUtil::BKToWIDEChar(b, m_pFloppyImage->m_pKoi8tbl);
						}

						tabs = 0;
						PutTchar(pFView, tch, counter);
					}
				}
				else
				{
					bool bFlg = false;
					PutTchar(pFView, tch, counter); // запишем сперва BOM

					while (pos < len)
					{
						b = GetByte(pFBuffer, pos);
						int repeater = 1;

						if (b == 0 || b == 10) // перевод строки
						{
							PutTchar(pFView, L'\r', counter);
							tch = L'\n';
							counter = -1;
						}
						else if (b == 9) // табуляция
						{
							int nNextTabPos = (counter / TAB_WIDTH + 1) * TAB_WIDTH;

							do
							{
								PutTchar(pFView, L' ', counter);
							}
							while (counter < nNextTabPos);

							continue;
						}
						else if (b < 020) // это количество пробелов, или табуляций
						{
							if (bFlg) // если число второй раз подряд - это уже число табуляций
							{
								for (int i = 0; i < b; ++i)
								{
									int nNextTabPos = (counter / TAB_WIDTH + 1) * TAB_WIDTH;

									do
									{
										PutTchar(pFView, L' ', counter);
									}
									while (counter < nNextTabPos);
								}

								bFlg = false;
							}
							else // если число один раз - это кол-во пробелов
							{
								for (int i = 0; i < b; ++i)
								{
									PutTchar(pFView, L' ', counter);
								}

								bFlg = true;
							}

							continue;
						}
						else if (b < 040) // это разные спец коды типа жирный, курсив, подчёркивание и т.п.
						{
							continue;
						}
						else
						{
							if (0200 < b && b < 0240) // то это количество следующих за кодом повторяющихся символов
							{
								repeater = b & 037;
								b = GetByte(pFBuffer, pos);

								// какая-то непонятная фигня. если после счётчика повторений коды 0..37, то они мапятся на 40..77
								if (b < 0140)
								{
									b += 040;
								} // это редкий диалект. обычно эта штука может вносить артефакты в текст
							}

							tch = imgUtil::BKToWIDEChar(b, m_pFloppyImage->m_pKoi8tbl);
						}

						for (int i = 0; i < repeater; ++i)
						{
							PutTchar(pFView, tch, counter);
						}

						bFlg = false;
					}
				}

				bRet = true;
			}
			else
			{
				AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(m_pFloppyImage->GetErrorNumber()));
			}
		}
		else
		{
			AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(IMAGE_ERROR::NOT_ENOUGHT_MEMORY));
		}

		fclose(pFView);
	}

	if (bRet)
	{
		// запускаем блокнот с этим файлом в качестве параметра
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));
		/*
		*если запускать текстовый файл через explorer, т.е. пытаться открыть прогой,
		которая с ассоциирована с *.txt, то процесс explorer успешно завершается передачей файла
		например блокноту, и прежде чем блокнот запустится, файл удаляется.
		Можно не удалять файлы во временной папке, но тогда перед повторным просмотром
		надо искать файл и удалять его, если он там есть, в общем всячески усложнять алгоритм
		К тому же, если расширение уже с чем-то в винде ассоциировано или наоборот, ни с чем не
		ассоциировано, то просмотреть файл будет затруднительно.
		Вариант с принудительным запуском блокнота самый оптимальный.
		*/
		std::wstring strCmdLine = L"notepad \"" + strFName.wstring() + L"\"";

		if (CreateProcess(nullptr,      // No module name (use command line)
		                  LPWSTR(strCmdLine.c_str()), // Command line
		                  nullptr,                    // Process handle not inheritable
		                  nullptr,                    // Thread handle not inheritable
		                  FALSE,                      // Set handle inheritance to FALSE
		                  0,                          // No creation flags
		                  nullptr,                    // Use parent's environment block
		                  nullptr,                    // Use parent's starting directory
		                  &si,                        // Pointer to STARTUPINFO structure
		                  &pi))                       // Pointer to PROCESS_INFORMATION structure
		{
// 			MSG msg;
// 			BOOL bRet;
// 			// Wait until child process exits.
// 			// если делать так, то почему-то клики мышкой пропадают.
// 			while (WAIT_OBJECT_0 != WaitForSingleObject(pi.hProcess, 100))
// 			{
// 				if (bRet = GetMessage(&msg, AfxGetMainWnd()->GetSafeHwnd(), 0, 0))
// 				{
// 					if (bRet != -1)
// 					{
// 						TranslateMessage(&msg);
// 						DispatchMessage(&msg);
// 					}
// 				}
// 			}
			Sleep(2000); // если делать так, то нет гарантии, что не случатся тормоза и успеем открыть файл в блокноте
			// Close process and thread handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	CFile::Remove(strFName.c_str());
	return bRet;
}

bool CBKImage::ViewFileRT11(BKDirDataItem *fr)
{
	bool bRet = false;

	// если это не файл - выходим ничего не делая
	if (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LOGDISK))
	{
		return true;
	}

	fs::path strFName = imgUtil::SetSafeName(fr->strName);
	std::wstring strExt = strUtil::strToUpper(strFName.extension().wstring()); // узнаем расширение
	strFName = fs::temp_directory_path() / strFName;
	enum class KoiType : int
	{
		KOI7N0 = 0,
		KOI7N1,
		KOI7N2,
		KOI8
	};
	KoiType nMode = KoiType::KOI7N2; // режим. 0 - КОИ7Н0, 1 - КОИ7Н1, 2 - КОИ7Н2, -1 - КОИ8
	FILE *pFView;

	if ((pFView = _tfopen(strFName.c_str(), L"wb")))
	{
		const int len = fr->nSize;
		auto uFBuffer = std::vector<uint8_t>(m_pFloppyImage->EvenSizeByBlock(len));
		const auto pFBuffer = uFBuffer.data();

		if (pFBuffer)
		{
			if (m_pFloppyImage->ReadFile(fr, pFBuffer))
			{
				uint8_t b;
				int counter = 0;
				int ccnt = 0; // счётчик управляющих кодов SI SO
				int cntRL = 0; // счётчик кодов 0300..0337 строчные русские
				int cntRH = 0; // счётчик кодов 0340..0377 заглавные русские
				int pos = 0;

				// сперва проверим - кои7 или кои8
				while (pos < len)
				{
					b = GetByte(pFBuffer, pos);

					if (b > 127)
					{
						counter++; // посчитаем коды больше 127

						if (b >= 0340)
						{
							cntRH++; // из них заглавные
						}
						else if (b >= 0300)
						{
							cntRL++; //из них строчные
						}
					}
					else if (b == 016 || b == 017)
					{
						ccnt++;
					}
				}

				if (counter)
				{
					// если коды >= 128 есть, сколько-то
					if (counter - (cntRL + cntRH) < (len / 20))
					{
						nMode = KoiType::KOI8; // то это вероятно кои8
					}
				}

				// если кодов >= 128 нету, это вероятно кои7н2. Или это ascii 128 чисто английский текст
				pos = 0; // перемещаемся к началу файла
				counter = 0;
				wchar_t tch = 0xfeff; // BOM
				PutTchar(pFView, tch, counter); // запишем сперва BOM

				while (pos < len)
				{
					b = GetByte(pFBuffer, pos);

					if (b == 0 || b == 10) // перевод строки
					{
						PutTchar(pFView, _T('\r'), counter);
						tch = _T('\n');
					}
					else if (b == 9) // табуляция
					{
						tch = _T('\t');
					}
					else if (b < 32) // это прочие управляющие символы
					{
						if (b == 016) // переключение в кои7н1 (РУС)
						{
							if (nMode == KoiType::KOI7N1)
							{
								nMode = KoiType::KOI7N2;
							}
							else
							{
								nMode = KoiType::KOI7N1;
							}

							// некоторым файлам нужно обязательно Н2,
							// а некоторым - обязательно Н1
							// автоматически это не определяется, может нужно опцию специальноую заводить
							//nMode = KoiType::KOI7N2;
						}
						else if (b == 017) // переключение в кои7н0 (ЛАТ)
						{
							nMode = KoiType::KOI7N0;
						}

//                      else if (b!= 015)
//                      {
//                          ASSERT(FALSE);
//                      }
						continue;
						// остальные коды, в том числе и \r - 015 игнорируем
					}
					else
					{
						switch (nMode)
						{
							case KoiType::KOI8:
								if (b == 044)
								{
									tch = 0x00a4; // исключение - знак $ в знак солнышко
								}
								else
								{
									// tch = BKToUNICODE_Byte(b, koi8tbl_RFC1489); // это для ДВК, УКНЦ и т.п.
									tch = imgUtil::BKToWIDEChar(b, imgUtil::koi8tbl11M); // это для БК.
								}

								break;

							case KoiType::KOI7N0: //просто аскии латиница
								if (b == 044)
								{
									tch = 0x00a4; // исключение - знак $ в знак солнышко
								}
								else
								{
									tch = wchar_t(b);
								}

								break;

							case KoiType::KOI7N1: // кириллица.коды 100 - 137 маленькие буквы, 140 - 177 - большие буквы
								if (b == 044)
								{
									tch = 0x00a4;
								}
								else if (b >= 0100 && b < 0200)
								{
									tch = imgUtil::koi8tbl11M[b];
								}
								else
								{
									tch = wchar_t(b);
								}

								break;

							case KoiType::KOI7N2: // смесь. коды 100-137 большие латинские, 140-177 - большие русские буквы
								if (b == 044)
								{
									tch = 0x00a4;
								}
								else if (b >= 0140 && b < 0200)
								{
									tch = imgUtil::koi8tbl11M[b]; // будем предполагать по умолчанию, что это кои7н2
									// tch = TCHAR(b); // будем предполагать по умолчанию, что это не кои7н2, а исключительно английский текст
								}
								else
								{
									tch = wchar_t(b);
								}

								break;
						}
					}

					PutTchar(pFView, tch, counter);
				}

				bRet = true;
			}
			else
			{
				AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(m_pFloppyImage->GetErrorNumber()));
			}
		}
		else
		{
			AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(IMAGE_ERROR::NOT_ENOUGHT_MEMORY));
		}

		fclose(pFView);
	}

	if (bRet)
	{
		// запускаем блокнот с этим файлом в качестве параметра
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		ZeroMemory(&pi, sizeof(pi));
		/*
		*если запускать текстовый файл через explorer, т.е. пытаться открыть прогой,
		которая сассоциирована с *.txt, то процесс explorer успешно завершается передачей файла
		например блокноту, и прежде чем блокнот запустится, файл удаляется.
		Можно не удалять файлы во временной папке, но тогда перед повторным просмотром
		надо искать файл и удалять его, если он там есть, в общем всячески усложнять алгоритм
		*/
		std::wstring strCmdLine = L"notepad \"" + strFName.wstring() + L"\"";

		if (CreateProcess(nullptr,          // No module name (use command line)
		                  LPWSTR(strCmdLine.c_str()), // Command line
		                  nullptr,          // Process handle not inheritable
		                  nullptr,          // Thread handle not inheritable
		                  false,            // Set handle inheritance to false
		                  0,                // No creation flags
		                  nullptr,          // Use parent's environment block
		                  nullptr,          // Use parent's starting directory
		                  &si,              // Pointer to STARTUPINFO structure
		                  &pi))             // Pointer to PROCESS_INFORMATION structure
		{
// 			MSG msg;
// 			BOOL bRet;
// 			// Wait until child process exits.
// 			while (WAIT_OBJECT_0 != WaitForSingleObject(pi.hProcess, 50))
// 			{
// 				if (bRet = GetMessage(&msg, AfxGetMainWnd()->GetSafeHwnd(), 0, 0))
// 				{
// 					if (bRet != -1)
// 					{
// 						TranslateMessage(&msg);
// 						DispatchMessage(&msg);
// 					}
// 				}
// 			}
			Sleep(2000);
			// Close process and thread handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
	}

	CFile::Remove(strFName.c_str());
	return bRet;
}

bool CBKImage::ViewFileAsSprite(BKDirDataItem *fr)
{
	bool bRet = false;

	// если это не файл - выходим ничего не делая
	if (fr->nAttr & (FR_ATTR::DIR | FR_ATTR::LOGDISK))
	{
		return true;
	}

	const int len = fr->nSize;

	if (len > 0)
	{
		auto uFBuffer = std::make_unique<uint8_t[]>(m_pFloppyImage->EvenSizeByBlock(len));

		if (uFBuffer)
		{
			if (m_pFloppyImage->ReadFile(fr, uFBuffer.get()))
			{
				auto pSprWnd = new CSprWnd(std::move(uFBuffer), len); // обязательно создавать динамически.

				if (pSprWnd)
				{
					WNDCLASS wc;
					memset(&wc, 0, sizeof(wc));
					wc.style = CS_HREDRAW | CS_VREDRAW;
					wc.hInstance = AfxGetInstanceHandle();
					wc.lpfnWndProc = ::DefWindowProc;
					wc.lpszClassName = L"BKSpriteViewClass";
					wc.hbrBackground = (HBRUSH)::GetStockObject(GRAY_BRUSH);
					AfxRegisterClass(&wc);

					if (pSprWnd->Create(
					            wc.lpszClassName,           // Имя класса виндовс
					            (std::wstring(L"SpriteView: ") + fr->strName.wstring()).c_str(),   // имя окна
					            WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME, // window styles
					            CRect(0, 0, 200, 200),   // размер окна
					            AfxGetMainWnd(),       // родитель окна
					            nullptr,
					            WS_EX_TOOLWINDOW | WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW // extended window styles
					        ))
					{
						pSprWnd->ShowWindow(SW_SHOW);
					}

					bRet = true;
				}
				else
				{
					AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(IMAGE_ERROR::NOT_ENOUGHT_MEMORY));
				}
			}
			else
			{
				AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(m_pFloppyImage->GetErrorNumber()));
			}
		}
		else
		{
			AfxGetMainWnd()->SendMessage(WM_SEND_ERRORNUM, WPARAM(0), LPARAM(IMAGE_ERROR::NOT_ENOUGHT_MEMORY));
		}
	}

	return bRet;
}

