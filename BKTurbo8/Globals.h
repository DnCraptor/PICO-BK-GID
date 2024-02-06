#pragma once

#include "Reader.h"
#include "GlobalStructs.h"

constexpr auto BASE_ADDRESS = 0;
constexpr auto DEFAULT_START_ADDRESS = 01000;
constexpr auto HIGH_BOUND = 0177000;

// тут будут храниться всякие глобальные параметры и флаги

enum class OPERAND_TYPE
{
	SRC,
	DST
};

enum class LINKING_MODE
{
	CL,
	CO,
	FINAL
};

// флаги типа арифметической операции
constexpr uint32_t ARIPHM_NOBRACKETS    = (1 << 0); // без скобок, только вычисления
constexpr uint32_t ARIPHM_NOLOCLABEL    = (1 << 1); // без локальных меток
constexpr uint32_t ARIPHM_NOUNDLABEL    = (1 << 2); // без неопределённых меток
constexpr uint32_t ARIPHM_INBRANCH      = (1 << 3); // в командах ветвления (влияет на обработку точки)

// флаги-опции
constexpr uint32_t BKT_GLBL_VERBOSITY   = (1 << 0);
constexpr uint32_t BKT_GLBL_INSCRIPT    = (1 << 1); // флаг, что обрабатываем скрипт.
constexpr uint32_t BKT_GLBL_INSTRING    = (1 << 2); // флаг, что находимся внутри строки "" для ascii, asciz и rad50
constexpr uint32_t BKT_GLBL_ENDREACHED  = (1 << 3); // флаг, что встретилась псевдокоманда .end
constexpr uint32_t BKT_GLBL_STOPONERROR = (1 << 4); // флаг остановки компиляции при первой встреченной ошибке
constexpr uint32_t BKT_GLBL_STEPINCLUDE = (1 << 5); // флаг, который говорит, что мы вошли в новую инклуду и надо начать цикл сначала
constexpr uint32_t BKT_GLBL_ININCLUDE   = (1 << 6); // флаг, который говорит, что инклуда обрабатывается.
constexpr uint32_t BKT_GLBL_SETENABL    = (1 << 7); // флаг команды ENABL, трактующей выполнять адресацию 67 как 37

class GlobalParams
{
		fs::path                m_strAppPath;   // домашний путь
		CReader::FILE_CHARSET   m_Charset;      // текущая кодировка исходника
		std::vector <std::wstring> m_vAddIncludes; // вектор, куда помещаются все добавленные инклуды
		std::vector <CReader *> m_vReader;      // вектор, куда помещаются реадеры при обработке инклуд

		OPERAND_TYPE            m_nOperandType; // тип обрабатываемого операнда
		LINKING_MODE            m_nLinkMode;    // режим компоновки меток: -1 = CL, 1 = CO, 0 = окончательная компоновка после .end
		uint32_t                m_nAriphmType;  // тип арифметического выражения, где оно встречается: -1 = константа, 1 - word или ассемблерная команда, 0 - все остальное

		uint32_t                m_nFlags;       // флаги-опции
		int                     m_nLinkAddr;    // адрес, с которого начинается линковка очередного объектного файла

		int                     m_nStartAddress; // смещение адреса начала трансляции проги относительно 01000 == по-умолчанию
		int                     m_nPC;          // текущий PC при компиляции, измеряется в байтах.
		int                     m_nProgramLength; // длина компилируемой программы
		int                     m_nError;       // флаг возникновения ошибки, и одновременно счётчик.

		int                     m_nLinkObjLen;  // размер прилинковываемого объектного файла
	public:
		GlobalParams()
			: m_Charset(CReader::FILE_CHARSET::UNDEFINE)
			, m_nFlags(0)
			, m_nLinkObjLen(0)
		{
			InitGlobalParameters();
		}

		void                InitGlobalParameters();
		void                ReInitGlobalParameters();

		void                SetBasePath(const fs::path &path);
		const fs::path     &GetBasePath() const;

		void                SetCharset(const CReader::FILE_CHARSET chs);
		CReader::FILE_CHARSET GetCharset() const;

		void                SetVerbose(const bool b);
		bool                isVerbose() const;

		void                SetInScript(const bool b);
		bool                isInScript() const;

		void                SetInString(const bool b);
		bool                isInString() const;

		void                SetENDReached(const bool b);
		bool                isENDReached() const;

		void                SetStopOnError(const bool b);
		bool                isStopOnError() const;

		void                SetLinkerAddr(const int n);
		int                 GetLinkerAddr() const;

		void                SetStepInclude(const bool b);
		bool                isStepInclude() const;

		void                SetInInclude(const bool b);
		bool                isInInclude() const;

		void                SetEnabl(const bool b);
		bool                isEnabl() const;

		void                SetOperandType(const OPERAND_TYPE ot);
		OPERAND_TYPE        GetOperandType() const;

		void                SetLinkMode(const LINKING_MODE lm);
		LINKING_MODE        GetLinkMode() const;

		void                SetAriphmType(const uint32_t at);
		uint32_t            GetAriphmType() const;

		void                SetPC(const int pc);
		int                 GetPC() const;

		void                SetProgramLength(const int l);
		int                 GetProgramLength() const;

		void                SetLinkObjLength(const int l);
		void                GrowLinkObjLength();

		void                SetError(const int n);
		int                 GetError() const;

		void                SetStartAddress(const int addr);
		int                 GetStartAddress() const;
		int                 GetRealAddress(const int addr) const;
		int                 CorrectOffset(const int addr) const;
		bool                FindReader(const fs::path &strFileName);
		void                PushReader(CReader *r);
		CReader            *PopReader();
};

union UMemory
{
	uint16_t w[32768];
	uint8_t  b[65536];
};

extern GlobalParams g_Globals;
extern UMemory g_Memory;
extern CReader *g_pReader;
extern std::vector <Registers_t> g_RegNames;

