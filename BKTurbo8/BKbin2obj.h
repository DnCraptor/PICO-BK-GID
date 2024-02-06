#pragma once

#define GDIPVER 0x0110
#include <atlimage.h>

namespace BKBin2Obj
{
	void Usage();
	void ConvertBin();
	void ConvertImage();
	void convertBitmap1(uint8_t *destp, CImage *img, bool t);
	void convertBitmap2(uint8_t *destp, CImage *img, bool t);
}
