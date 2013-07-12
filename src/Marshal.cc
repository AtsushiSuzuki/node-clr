#include "node-clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Dynamic;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;


System::String^ ToCLRString(Handle<Value> value)
{
	return gcnew System::String((const wchar_t *)(*String::Value(value)));
}

Local<String> ToV8String(System::String^ value)
{
	pin_ptr<const wchar_t> ptr = PtrToStringChars(value);
	return String::New((const uint16_t*)ptr);
}

Local<String> ToV8Symbol(System::String ^value)
{
	auto ptr = (const char *)(Marshal::StringToHGlobalAnsi(value)).ToPointer();
	auto result = String::NewSymbol(ptr);
	Marshal::FreeHGlobal(System::IntPtr((void*)ptr));
	return result;
}

System::Object^ ToCLRValue(
	v8::Handle<v8::Value> value)
{
	return ChangeType(value, System::Object::typeid);
}

System::Object^ ChangeType(
	Handle<Value> value,
	System::Type^ type)
{
	int match;
	auto result = ChangeType(
		value,
		type,
		match);

	if (INCOMPATIBLE < match)
	{
		return result;
	}
	else
	{
		throw gcnew System::InvalidCastException();
	}
}

System::Object^ ChangeType(
	Handle<Value> value,
	System::Type^ type,
	int& match)
{
	// unwrap value
	if (CLRObject::IsWrapped(value))
	{
		return ChangeType(CLRObject::Unwrap(value), type, match);
	}

	// null value
	if (value->IsNull() || value->IsUndefined())
	{
		if (!type->IsValueType)
		{
			match = EXACT;
			return nullptr;
		}
		else if (type->IsGenericType &&
			type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
		{
			match = EXACT;
			return nullptr;
		}
	}
	
	// unwarp nullable
	if (type->IsGenericType &&
		type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
	{
		type = type->GetGenericArguments()[0];
	}

	// convert value
	if (value->IsBoolean())
	{
		if (type->IsAssignableFrom(System::Boolean::typeid))
		{
			match = EXACT;
			return value->BooleanValue();
		}
	}
	else if (value->IsInt32())
	{
		if (type->IsAssignableFrom(System::Int32::typeid) ||
			type == System::Int64::typeid ||
			type == System::Single::typeid ||
			type == System::Double::typeid ||
			type == System::Decimal::typeid)
		{
			match = EXACT;
			return value->Int32Value();
		}
		else if (type == System::SByte::typeid ||
			type == System::Byte::typeid ||
			type == System::Int16::typeid ||
			type == System::UInt16::typeid ||
			type == System::UInt32::typeid ||
			type == System::UInt64::typeid ||
			type == System::Char::typeid)
		{

			try
			{
				match = IMPLICIT_CONVERSION;
				return System::Convert::ChangeType(value->Int32Value(), type);
			}
			catch (System::OverflowException^)
			{
				return ChangeType(value->Int32Value(), type, match);
			}
		}
	}
	else if (value->IsUint32())
	{
		if (type->IsAssignableFrom(System::UInt32::typeid) ||
			type == System::Int64::typeid ||
			type == System::UInt64::typeid ||
			type == System::Single::typeid ||
			type == System::Double::typeid ||
			type == System::Decimal::typeid)
		{
			match = EXACT;
			return value->Uint32Value();
		}
		else if (type == System::SByte::typeid ||
			type == System::Byte::typeid ||
			type == System::Int16::typeid ||
			type == System::UInt16::typeid ||
			type == System::Int32::typeid ||
			type == System::Char::typeid)
		{
			try
			{
				match = IMPLICIT_CONVERSION;
				return value->Uint32Value();
			}
			catch (System::OverflowException^)
			{
				return ChangeType(value->Uint32Value(), type, match);
			}
		}
	}
	else if (value->IsNumber() || value->IsNumberObject())
	{
		if (type->IsAssignableFrom(System::Double::typeid))
		{
			match = EXACT;
			return value->NumberValue();
		}
		else if (type == System::SByte::typeid ||
			type == System::Byte::typeid ||
			type == System::Int16::typeid ||
			type == System::UInt16::typeid ||
			type == System::Int32::typeid ||
			type == System::UInt32::typeid ||
			type == System::Int64::typeid ||
			type == System::UInt64::typeid ||
			type == System::Char::typeid ||
			type == System::Single::typeid ||
			type == System::Decimal::typeid)
		{
			try
			{
				match = IMPLICIT_CONVERSION;
				return value->NumberValue();
			}
			catch (System::OverflowException^)
			{
				return ChangeType(value->NumberValue(), type, match);
			}
		}
	}
	else if (value->IsString())
	{
		if (type->IsAssignableFrom(System::String::typeid))
		{
			match = EXACT;
			return ToCLRString(value);
		}
	}
	else if (value->IsFunction())
	{
		if (type->IsAssignableFrom(System::MulticastDelegate::typeid))
		{
			match = EXACT;
			return V8Delegate::CreateDelegate(Handle<Function>::Cast(value));
		}
		else if (System::Delegate::typeid->IsAssignableFrom(type))
		{
			match = EXACT;
			return V8Delegate::CreateDelegate(Handle<Function>::Cast(value), type);
		}
	}
	else if (value->IsArray())
	{
		// TODO: handle cyclic reference

		auto elementType = System::Object::typeid;
		if (type->IsArray && type->HasElementType)
		{
			elementType = type->GetElementType();
		}
		else if (type->IsGenericType)
		{
			auto d = type->GetGenericTypeDefinition();
			if (d == IEnumerable<int>::typeid->GetGenericTypeDefinition() ||
				d == ICollection<int>::typeid->GetGenericTypeDefinition() ||
				d == IList<int>::typeid->GetGenericTypeDefinition() ||
				d == IReadOnlyCollection<int>::typeid->GetGenericTypeDefinition() ||
				d == IReadOnlyList<int>::typeid->GetGenericTypeDefinition())
			{
				elementType = type->GetGenericArguments()[0];
			}
		}

		auto from = Handle<Array>::Cast(value);
		auto to = System::Array::CreateInstance(elementType, from->Length());
		for (int i = 0; i < (int)from->Length(); i++)
		{
			int m;
			to->SetValue(
				ChangeType(
					from->Get(i),
					elementType,
					m),
				i);
			match = System::Math::Min(m, match);
		}

		if (type->IsAssignableFrom(to->GetType()))
		{
			return to;
		}
	}
	else
	{
		// TODO: handle DataContractAttribute
		// TODO: handle cyclic reference

		auto from = Handle<Object>::Cast(value);
		auto to = (IDictionary<System::String^, System::Object^>^)(gcnew ExpandoObject());
		
		auto names = from->GetOwnPropertyNames();
		for (int i = 0; i < (int)names->Length(); i++)
		{
			auto name = names->Get(i);

			int m;
			to[ToCLRString(name)] =
				ChangeType(
					from->Get(name),
					System::Object::typeid,
					m);

			match = System::Math::Min(m, match);
		}

		if (type->IsAssignableFrom(ExpandoObject::typeid))
		{
			return to;
		}
	}

	match = INCOMPATIBLE;
	return nullptr;
}

System::Object^ ChangeType(
	System::Object^ value,
	System::Type^ type,
	int& match)
{
	// null value
	if (value == nullptr)
	{
		if (!type->IsValueType)
		{
			match = EXACT;
			return nullptr;
		}
		else if (type->IsGenericType &&
			type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
		{
			match = EXACT;
			return nullptr;
		}
	}
	
	// unwarp nullable
	if (type->IsGenericType &&
		type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
	{
		type = type->GetGenericArguments()[0];
	}
	
	auto valueType = (value != nullptr) ? value->GetType() : System::Object::typeid;
	
	// primitives
	auto from = System::Type::GetTypeCode(valueType);
	auto to = System::Type::GetTypeCode(type);
	const int conversionTable[19][19] = {
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Empty
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Object
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DBNull
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // Boolean
		{ 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0 }, // Char
		{ 0, 0, 0, 0, 1, 3, 1, 2, 1, 2, 1, 2, 1, 2, 2, 2, 0, 0, 0 }, // SByte
		{ 0, 0, 0, 0, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0 }, // Byte
		{ 0, 0, 0, 0, 1, 1, 1, 3, 1, 2, 1, 2, 1, 2, 2, 2, 0, 0, 0 }, // Int16
		{ 0, 0, 0, 0, 1, 1, 1, 1, 3, 2, 2, 2, 2, 2, 2, 2, 0, 0, 0 }, // UInt16
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 3, 1, 2, 1, 2, 2, 2, 0, 0, 0 }, // Int32
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 2, 2, 0, 0, 0 }, // UInt32
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 3, 1, 2, 2, 2, 0, 0, 0 }, // Int64
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 2, 2, 0, 0, 0 }, // UInt64
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 2, 1, 0, 0, 0 }, // Single
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 0, 0, 0 }, // Double
		{ 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 0, 0, 0 }, // Decimal
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 }, // DateTime
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
		{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },  // String
	};

	match = conversionTable[(int)from][(int)to];
	if (INCOMPATIBLE < match)
	{
		switch (to)
		{
		case System::TypeCode::Char:
			return static_cast<System::Char>(value);
		case System::TypeCode::SByte:
			return static_cast<System::SByte>(value);
		case System::TypeCode::Byte:
			return static_cast<System::Byte>(value);
		case System::TypeCode::Int16:
			return static_cast<System::Int16>(value);
		case System::TypeCode::UInt16:
			return static_cast<System::UInt16>(value);
		case System::TypeCode::Int32:
			return static_cast<System::Int32>(value);
		case System::TypeCode::UInt32:
			return static_cast<System::UInt32>(value);
		case System::TypeCode::Int64:
			return static_cast<System::Int64>(value);
		case System::TypeCode::UInt64:
			return static_cast<System::UInt64>(value);
		case System::TypeCode::Single:
			return static_cast<System::Single>(value);
		case System::TypeCode::Double:
			return static_cast<System::Double>(value);
		case System::TypeCode::Decimal:
			return static_cast<System::Decimal>(value);
		}
	}

	if (type->IsAssignableFrom(valueType))
	{
		match = EXACT;
		return value;
	}
	else
	{
		match = INCOMPATIBLE;
		return nullptr;
	}
}

Handle<Value> ToV8Value(System::Object^ value)
{
	// primitives
	if (value == nullptr)
	{
		return Local<Value>::New(Null());
	}
	else if (dynamic_cast<System::Boolean^>(value) != nullptr)
	{
		return Boolean::New((System::Boolean)value);
	}
	else if (dynamic_cast<System::SByte^>(value) != nullptr ||
		dynamic_cast<System::Byte^>(value) != nullptr ||
		dynamic_cast<System::Int16^>(value) != nullptr ||
		dynamic_cast<System::UInt16^>(value) != nullptr ||
		dynamic_cast<System::Int32^>(value) != nullptr ||
		dynamic_cast<System::UInt32^>(value) != nullptr ||
		dynamic_cast<System::Int64^>(value) != nullptr ||
		dynamic_cast<System::UInt64^>(value) != nullptr ||
		dynamic_cast<System::Single^>(value) != nullptr ||
		dynamic_cast<System::Double^>(value) != nullptr ||
		dynamic_cast<System::Decimal^>(value) != nullptr)
	{
		return Number::New(System::Convert::ToDouble(value));
	}
	else if (dynamic_cast<System::String^>(value) != nullptr)
	{
		return ToV8String((System::String^)value);
	}
	// objects
	else
	{
		return CLRObject::Wrap(value);
	}
}

System::Exception^ ToCLRException(Handle<Value> ex)
{
	// TODO:
	return gcnew System::Exception(
		ToCLRString(
		ex->ToObject()->Get(String::NewSymbol("message"))));
}

Local<Value> ToV8Exception(System::Exception^ ex)
{
	// TODO:
	return Exception::Error(ToV8String(ex->ToString()));
}