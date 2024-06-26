#include "StringConverter.hpp"

#include <cstdio>
#include <cstdlib>

#include "Registry.hpp"


namespace Fyrion
{
	usize Converters::U32ToString(char* buffer, usize bufferSize, u32 value)
	{
		return snprintf(buffer, bufferSize, "%u", value);
	}

	usize Converters::U64ToString(char* buffer, usize bufferSize, u64 value)
	{
		return snprintf(buffer, bufferSize, "%llu", value);
	}

	usize Converters::I32ToString(char* buffer, usize bufferSize, i32 value)
	{
		return snprintf(buffer, bufferSize, "%d", value);
	}

	usize Converters::I64ToString(char* buffer, usize bufferSize, i64 value)
	{
		return snprintf(buffer, bufferSize, "%lld", value);
	}

	u32 Converters::U32FromString(const char* str)
	{
		return strtoul(str, nullptr, 0);
	}

	i32 Converters::I32FromString(const char* str)
	{
		return strtol(str, nullptr, 0);
	}

	u64 Converters::U64FromString(const char* str)
	{
		return strtoull(str, nullptr, 0);
	}

	i64 Converters::I64FromString(const char* str)
	{
		return strtoll(str, nullptr, 0);
	}

	usize Converters::F32ToString(char* buffer, usize bufferSize, f32 value)
	{
		return snprintf(buffer, bufferSize, "%f", value);
	}

	usize Converters::F64ToString(char* buffer, usize bufferSize, f64 value)
	{
		return snprintf(buffer, bufferSize, "%lf", value);
	}

	f32 Converters::F32FromString(const char* str)
	{
		return strtof(str, nullptr);
	}

	f64 Converters::F64FromString(const char* str)
	{
		return strtod(str, nullptr);
	}

	usize Converters::EnumStringSize(TypeID enumid, i64 code)
	{
		if (TypeHandler* typeHandler = Registry::FindTypeById(enumid))
		{
			if (ValueHandler* value = typeHandler->FindValueByCode(code))
			{
				return value->GetDesc().Size();
			}
		}
		return 0;
	}


	usize Converters::EnumToString(TypeID enumid, i64 code, char* buffer, usize pos)
	{
		if (TypeHandler* typeHandler = Registry::FindTypeById(enumid))
		{
			if (ValueHandler* value = typeHandler->FindValueByCode(code))
			{
				StringView desc = value->GetDesc();
				MemCopy(buffer + pos, desc.CStr(), desc.Size());
				return desc.Size();
			}
		}
		return 0;
	}

	i64 Converters::EnumFromString(TypeID enumid, const char* desc, usize size)
	{
		if (TypeHandler* typeHandler = Registry::FindTypeById(enumid))
		{
			if (ValueHandler* value = typeHandler->FindValueByName(StringView{desc, size}))
			{
				return value->GetCode();
			}
		}
		return 0;
	}
}

