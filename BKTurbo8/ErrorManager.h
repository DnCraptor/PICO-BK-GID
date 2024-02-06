#pragma once

enum ERRNUM : int
{
	E_101 = 101,
	E_102,
	E_103,
	E_104,
	E_105,
	E_106,
	E_107,
	E_108,
	E_109,
	E_110,
	E_111,
	E_112,
	E_113,
	E_114,
	E_115,
	E_116,
	E_117,
	E_118,
	E_119,
	E_120,
	E_121,
	E_122,
	E_123,
	E_124,
	E_125,
	E_126,
	E_127,
	E_128,
	E_129,
	E_130,
	E_131,
	E_132,
	E_133,
	E_134,
};

namespace ErrorManager
{
	struct BKTError_t
	{
		ERRNUM num;
		std::wstring str;
	};

	void OutError(ERRNUM n, bool bOutStr = true, int nAddr = -1);
	int IsError();
}
