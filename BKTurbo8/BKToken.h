#pragma once
#include "ObjParams.h"

class CBKToken
{
		std::wstring name;
		size_t hash;
		size_t calcHash()
		{
			return calcHash(name);
		}

	public:
		CBKToken(): hash(0) {}

		CBKToken(const std::wstring &strName) : name(strName)
		{
			hash = calcHash();
		}

		virtual ~CBKToken() = default;

		void clear()
		{
			name.clear();
			hash = 0;
		}

		void setName(const std::wstring &strName)
		{
			name = strName;
			hash = calcHash();
		}

		const std::wstring &getName()const
		{
			return name;
		}

		const size_t &getHash() const
		{
			return hash;
		}

		size_t calcHash(const std::wstring &str)
		{
			std::hash<std::wstring> hash_fn;
			size_t hash = hash_fn(str);
			return hash;
		}

		CBKToken &operator=(const CBKToken &t)
		    = default;

		CBKToken &operator=(const CBKToken *t)
		{
			this->name = t->name;
			this->hash = t->hash;
			return *this;
		}

		void Store(FILE *f) const
		{
			OBJTags tag = OBJTags::OBJ_Token;
			fwrite(&tag, 1, sizeof(tag), f);
			auto len = static_cast<uint32_t>(name.length());
			fwrite(&len, 1, sizeof(len), f);

			if (len > 0)
			{
				fwrite(name.c_str(), 1, len * sizeof(wchar_t), f);
			}
		}

		bool Load(FILE *f)
		{
			OBJTags tag;
			fread(&tag, 1, sizeof(tag), f);

			if (tag == OBJTags::OBJ_Token)
			{
				uint32_t len;
				fread(&len, 1, sizeof(uint32_t), f);

				if (static_cast<int>(len) > 0)
				{
					auto lname = std::make_unique<wchar_t[]>(static_cast<size_t>(len) + 1);

					if (lname)
					{
						fread(lname.get(), 1, len * sizeof(wchar_t), f);
						lname[len] = 0;
						name = std::wstring(lname.get());
						hash = calcHash();
						return true;
					}

					return false;
				}

				name.clear();
				hash = calcHash();
				return true;
			}

			return false;
		}
};

