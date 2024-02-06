#include "pch.h"
#include "Globals.h"

GlobalParams g_Globals;

UMemory g_Memory;

CReader *g_pReader = nullptr; // текущий обрабатываемый файл

std::vector <Registers_t> g_RegNames =
{
	{0, {CBKToken(L"R0"), CBKToken(L"%0")} },
	{1, {CBKToken(L"R1"), CBKToken(L"%1")} },
	{2, {CBKToken(L"R2"), CBKToken(L"%2")} },
	{3, {CBKToken(L"R3"), CBKToken(L"%3")} },
	{4, {CBKToken(L"R4"), CBKToken(L"%4")} },
	{5, {CBKToken(L"R5"), CBKToken(L"%5")} },
	{6, {CBKToken(L"R6"), CBKToken(L"%6"), CBKToken(L"SP")} }, //список должен быть таким, для скриптов берётся только первое имя из списка
	{7, {CBKToken(L"R7"), CBKToken(L"%7"), CBKToken(L"PC")} },
	// это я задумал ввести поддержку присвоения именам регистров пользовательских значений,
	// но что-то пока непонятно, как сделать обращения к регистрам через %n и зачем вообще это надо делать
};

// инициализация всех параметров
void GlobalParams::InitGlobalParameters()
{
	m_nFlags = 0;
	m_nStartAddress = DEFAULT_START_ADDRESS;
	ReInitGlobalParameters();
}
// переинициализация. некоторых параметров.
void GlobalParams::ReInitGlobalParameters()
{
	m_nFlags &= ~(BKT_GLBL_INSCRIPT | BKT_GLBL_INSTRING | BKT_GLBL_ENDREACHED | BKT_GLBL_STOPONERROR);
	m_nLinkAddr = BASE_ADDRESS;
	m_nOperandType = OPERAND_TYPE::SRC;
	m_nLinkMode = LINKING_MODE::FINAL;
	m_nAriphmType = 0;
	m_nPC = BASE_ADDRESS;
	m_nProgramLength = 0;
	m_nError = 0;
	memset(&(g_Memory.b), 0, 65536);
}

void GlobalParams::SetBasePath(const fs::path &path)
{
	m_strAppPath = path;
}

const fs::path &GlobalParams::GetBasePath() const
{
	return m_strAppPath;
}

void GlobalParams::SetCharset(const CReader::FILE_CHARSET chs)
{
	m_Charset = chs;
}

CReader::FILE_CHARSET GlobalParams::GetCharset() const
{
	return m_Charset;
}


#define SET_FLAG(b, f) if(b){m_nFlags |= (f);}else{m_nFlags &= ~(f);}
#define GET_FLAG(f) !!(m_nFlags & (f));

void GlobalParams::SetVerbose(const bool b)
{
	SET_FLAG(b, BKT_GLBL_VERBOSITY);
}

bool GlobalParams::isVerbose() const
{
	return GET_FLAG(BKT_GLBL_VERBOSITY);
}

void GlobalParams::SetInScript(const bool b)
{
	SET_FLAG(b, BKT_GLBL_INSCRIPT);
}

bool GlobalParams::isInScript() const
{
	return GET_FLAG(BKT_GLBL_INSCRIPT);
}

void GlobalParams::SetInString(const bool b)
{
	SET_FLAG(b, BKT_GLBL_INSTRING);
}

bool GlobalParams::isInString() const
{
	return GET_FLAG(BKT_GLBL_INSTRING);
}

void GlobalParams::SetENDReached(const bool b)
{
	SET_FLAG(b, BKT_GLBL_ENDREACHED);
}

bool GlobalParams::isENDReached() const
{
	return GET_FLAG(BKT_GLBL_ENDREACHED);
}

void GlobalParams::SetStopOnError(const bool b)
{
	SET_FLAG(b, BKT_GLBL_STOPONERROR);
}

bool GlobalParams::isStopOnError() const
{
	return GET_FLAG(BKT_GLBL_STOPONERROR);
}

void GlobalParams::SetLinkerAddr(const int n)
{
	m_nLinkAddr = n;
}

int GlobalParams::GetLinkerAddr() const
{
	return m_nLinkAddr;
}

void GlobalParams::SetStepInclude(const bool b)
{
	SET_FLAG(b, BKT_GLBL_STEPINCLUDE);
}

bool GlobalParams::isStepInclude() const
{
	return GET_FLAG(BKT_GLBL_STEPINCLUDE);
}

void GlobalParams::SetInInclude(const bool b)
{
	SET_FLAG(b, BKT_GLBL_ININCLUDE);
}

bool GlobalParams::isInInclude() const
{
	return GET_FLAG(BKT_GLBL_ININCLUDE);
}

void GlobalParams::SetEnabl(const bool b)
{
	SET_FLAG(b, BKT_GLBL_SETENABL);
}

bool GlobalParams::isEnabl() const
{
	return GET_FLAG(BKT_GLBL_SETENABL);
}

void GlobalParams::SetOperandType(const OPERAND_TYPE ot)
{
	m_nOperandType = ot;
}

OPERAND_TYPE GlobalParams::GetOperandType() const
{
	return m_nOperandType;
}

void GlobalParams::SetLinkMode(const LINKING_MODE lm)
{
	m_nLinkMode = lm;
}

LINKING_MODE GlobalParams::GetLinkMode() const
{
	return m_nLinkMode;
}

void GlobalParams::SetAriphmType(const uint32_t at)
{
	m_nAriphmType = at;
}

uint32_t GlobalParams::GetAriphmType() const
{
	return m_nAriphmType;
}

void GlobalParams::SetPC(const int pc)
{
	m_nPC = pc;
}

int GlobalParams::GetPC() const
{
	return m_nPC;
}

void GlobalParams::SetProgramLength(const int l)
{
	m_nProgramLength = l;
}

int GlobalParams::GetProgramLength() const
{
	return m_nProgramLength;
}

void GlobalParams::SetLinkObjLength(const int l)
{
	m_nLinkObjLen = l;
}

void GlobalParams::GrowLinkObjLength()
{
	m_nProgramLength += m_nLinkObjLen;
	m_nLinkAddr += m_nLinkObjLen;
	m_nLinkObjLen = 0;
}

void GlobalParams::SetError(const int n)
{
	m_nError = n;
}

int GlobalParams::GetError() const
{
	return m_nError;
}

void GlobalParams::SetStartAddress(const int addr)
{
	m_nStartAddress = addr;
}
int GlobalParams::GetStartAddress() const
{
	return m_nStartAddress & 0xffff;
}

int GlobalParams::GetRealAddress(const int addr) const
{
	return (addr - BASE_ADDRESS + m_nStartAddress) & 0xffff;
}
int GlobalParams::CorrectOffset(const int addr) const
{
	return (addr - BASE_ADDRESS - m_nStartAddress) & 0xffff;
}

bool GlobalParams::FindReader(const fs::path &strFileName)
{
	// поищем, вдруг уже добавляли эту инклуду
	for (auto &rdr : m_vReader)
	{
		if (rdr && rdr->GetFileName() == strFileName)
		{
			return true; // добавляли
		}
	}

	// теперь поищем среди всех ранее включавшихся
	for (auto &s : m_vAddIncludes)
	{
		if (s == strFileName)
		{
			return true; // уже включали
		}
	}

	// нет, такое ещё точно не добавляли
	m_vAddIncludes.push_back(strFileName);
	return false;
}

void GlobalParams::PushReader(CReader *r)
{
	m_vReader.emplace_back(r);

	if (r)
	{
		SET_FLAG(true, BKT_GLBL_STEPINCLUDE | BKT_GLBL_ININCLUDE);
	}
}

CReader *GlobalParams::PopReader()
{
	CReader *ret = nullptr;

	if (!m_vReader.empty()) // если в векторе что-то есть
	{
		ret = m_vReader.back(); // достаём
		m_vReader.pop_back();

		if (ret)
		{
			SET_FLAG(false, BKT_GLBL_ENDREACHED);
		}
	}

	return ret;
}
