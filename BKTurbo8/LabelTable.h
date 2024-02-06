#pragma once
#include "BKToken.h"
#include "Parser2.h"

// определения меток.
constexpr uint32_t LBL_DEFINITE_MASK    = 0x0000000F; // Маска для выделения типа метки
constexpr uint32_t LBL_UNKNOWN          = 0x00000000; // значение для ошибочных ситуаций
constexpr uint32_t LBL_GLOBAL           = 0x00000001; // описание глобальной метки
constexpr uint32_t LBL_LOCAL            = 0x00000002; // описание локальной метки
constexpr uint32_t LBL_DEFINE           = 0x00000003; // описание константы (присваивания) с вычисленным значением
constexpr uint32_t LBL_WEAKDEFINE       = 0x00000004; // описание константы (присваивания) значение которой ещё не вычислено. там арифметическое выражение.

// определения, где встретилось арифметическое выражение
constexpr uint32_t ARL_DEFINITE_MASK    = 0x0000000F; // Маска для выделения типа метки
constexpr uint32_t ARL_DEFINE           = 0x00000001; // в присваивании (в константе)
constexpr uint32_t ARL_CMDARG           = 0x00000002; // в команде

constexpr uint32_t ARL_FLAGS_MASK       = 0x0000F000; // Маска для выделения флагов арифметического выражения
constexpr uint32_t ARL_LOCAL            = 0x00004000; // ссылка на локальную метку
constexpr uint32_t ARL_BYTEL            = 0x00008000; // флаг байтового результата, т.е. значение надо сохранять в байт
// определения ссылок на метки
constexpr uint32_t REFERENCE_TYPE_MASK  = 0x000000F0; // Маска для выделения типа метки
constexpr uint32_t ARL_BRANCH_LABEL     = 0x00000010; // метка в ветвлении
constexpr uint32_t ARL_OFFSET_LABEL     = 0x00000020; // смещение до адреса
constexpr uint32_t ARL_RELATIVE_LABEL   = 0x00000030; // Метка

class Label
{
	protected:
		uint32_t    type;       // тип - метка или определение, это нужно для релокации.
		// при линковке объектника меткам нужно перевычислять адреса, определениям - нет
		// LOCAL,GLOBAL,DEFINE
		int         value;      // значение метки или определения
		CBKToken    label;      // имя метки или определения

	public:
		Label() = default;
		Label(uint32_t lt, int lv, CBKToken *pt)
			: type(lt)
			, value(lv)
			, label(*pt) {}

		inline uint32_t getType() const
		{
			return type;
		}
		inline void setType(uint32_t t)
		{
			type = t & LBL_DEFINITE_MASK;
		}
		inline int getValue() const
		{
			return value;
		}
		inline void setValue(int v)
		{
			value = v;
		}

		inline const CBKToken &getToken()
		{
			return label;
		}

		inline CBKToken *getPToken()
		{
			return &label;
		}

		void Store(FILE *f)
		{
			OBJTags tag = OBJTags::OBJ_Label;
			fwrite(&tag, 1, sizeof(tag), f);
			fwrite(&type, 1, sizeof(type), f);
			fwrite(&value, 1, sizeof(value), f);
			label.Store(f);
		}

		bool Load(FILE *f)
		{
			OBJTags tag;
			fread(&tag, 1, sizeof(tag), f);

			if (tag == OBJTags::OBJ_Label)
			{
				fread(&type, 1, sizeof(type), f);
				fread(&value, 1, sizeof(value), f);

				if (label.Load(f))
				{
					return true;
				}
			}

			return false;
		}
};

// Класс для управления таблицей меток.
// необходимые функции - добавление, удаление, очистка таблицы, поиск и выдача результата.
class CLabelTable
{
#ifdef _DEBUG
		std::wstring            m_strDumpName;
#endif

	protected:
		std::vector <Label> LabelTable;

	public:
		CLabelTable();
		virtual ~CLabelTable();
#ifdef _DEBUG
		void                SetDumpName(const std::wstring &strDumpName);
		void                Dump();
#endif

		void                Clear();    // очистка массива меток
		inline size_t       getSize() const
		{
			return LabelTable.size();
		}

		bool                AddLabel(CBKToken *token, const int value, const uint32_t lt);
		bool                AddLabel(Label &lbl);
		CBKToken           *GetLabel(const size_t n);
		void                DeleteLabel(CBKToken *token);
		void                DeleteLabel(const size_t n);
		int                 SearchLabel(CBKToken *token);
		int                 GetValue(CBKToken *token);
		int                 GetValue(const size_t n);
		uint32_t            GetType(CBKToken *token);
		uint32_t            GetType(const size_t n);
		Label              &GetElement(const size_t n)
		{
			return LabelTable.at(n);
		}
};

class LabelRef
{
		uint32_t                type;   // тип арифметического выражения, выражение может быть в команде, тогда у него есть адрес
		// или может быть в определении, тогда у него нет адреса, но есть имя.
		int                     address;// адрес, где встретилось арифметическое выражение
		CBKToken                define; // имя определения, где встретилось арифметическое выражение
		// или имя локальной метки, для них нету арифметических выражений
		CRPNParser::RPNChain    chain;  // цепочка арифметического выражения.

	public:
		LabelRef() = default;
		LabelRef(uint32_t lt, int addr, CBKToken *pt, CRPNParser::RPNChain &rpn)
			: type(lt)
			, address(addr)
			, define{ pt ? * pt : CBKToken() }
			, chain(rpn)
		{}
		inline uint32_t getType() const
		{
			return type;
		}
		inline void setType(uint32_t t)
		{
			type = t;
		}
		inline int getAddress() const
		{
			return address;
		}
		inline void setAddress(int v)
		{
			address = v;
		}

		inline const CBKToken &getDefine()
		{
			return define;
		}

		inline CBKToken *getPDefine()
		{
			return &define;
		}

		inline CRPNParser::RPNChain &getRPN()
		{
			return chain;
		}

		inline CRPNParser::RPNChain *getPPRN()
		{
			return &chain;
		}

		void Store(FILE *f)
		{
			OBJTags tag = OBJTags::OBJ_Refs;
			fwrite(&tag, 1, sizeof(tag), f);
			fwrite(&type, 1, sizeof(type), f);
			fwrite(&address, 1, sizeof(address), f);
			define.Store(f);
			g_RPNParser.Store(f, chain);
		}

		bool Load(FILE *f)
		{
			OBJTags tag;
			fread(&tag, 1, sizeof(tag), f);

			if (tag == OBJTags::OBJ_Refs)
			{
				fread(&type, 1, sizeof(type), f);
				fread(&address, 1, sizeof(address), f);

				if (define.Load(f))
				{
					if (g_RPNParser.Load(f, chain))
					{
						return true;
					}
				}
			}

			return false;
		}
};


// Класс для управления таблицей ссылок на метки.
// в общем случае - это таблица ссылок на арифметические выражения, которые встречаются в коде
// в тривиальном случае арифметическое выражение состоит из одного имени метки.
class CRefsTable
{
#ifdef _DEBUG
		std::wstring            m_strDumpName;
#endif

	protected:
		std::vector <LabelRef> RefsTable;

	public:
		CRefsTable();
#ifdef _DEBUG
		void                SetDumpName(const std::wstring &strDumpName);
		void                Dump();
#endif
		virtual ~CRefsTable();

		void                Clear();    // очистка массива меток
		inline size_t       getSize() const
		{
			return RefsTable.size();
		}
		bool                AddRefs(CBKToken *token, const int addr, uint32_t lt);
		bool                AddRefs(LabelRef &lbl);

		uint32_t            GetType(const size_t n);
		void                DeleteElement(const size_t n);
		LabelRef           &GetElement(const size_t n)
		{
			return RefsTable.at(n);
		}

		void                LockUndefinedLocalLabels();
		void                FixLocalLabels(CBKToken *token, const int value);
};

