#include "node-clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Dynamic;
using namespace System::Linq;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
using namespace System::Text;
using namespace System::Text::RegularExpressions;


/*
 * String conversion
 */

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


/*
 * Value conversion
 */

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
	if (CLRObject::IsCLRObject(value))
	{
		return ChangeType(CLRObject::Unwrap(value), type, match);
	}

	// null value
	if (value.IsEmpty() || value->IsNull() || value->IsUndefined())
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
		else if (type->IsAssignableFrom(System::Char::typeid))
		{
			auto str = ToCLRString(value);
			if (str->Length == 1)
			{
				match = IMPLICIT_CONVERSION;
				return str[0];
			}
			else if (str->Length == 0)
			{
				match = EXPLICIT_CONVERSION;
				return static_cast<System::Char>(0);
			}
			else
			{
				match = EXPLICIT_CONVERSION;
				return str[0];
			}
		}
		else if (System::Enum::typeid->IsAssignableFrom(type) &&
			type != System::Enum::typeid)
		{
			try
			{
				match = IMPLICIT_CONVERSION;
				return System::Enum::Parse(type, ToCLRString(value));
			}
			catch (System::Exception^)
			{
			}
		}
	}
	else if (value->IsFunction())
	{
		auto func = Handle<Function>::Cast(value);
		if (type->IsAssignableFrom(System::MulticastDelegate::typeid))
		{
			match = EXACT;
			return V8Delegate::CreateDelegate(func);
		}
		else if (System::Delegate::typeid->IsAssignableFrom(type))
		{
			match = EXACT;
			return V8Delegate::CreateDelegate(func, type);
		}
		else if (type->IsAssignableFrom(System::Type::typeid) &&
			CLRObject::IsCLRConstructor(func))
		{
			match = EXACT;
			return System::Type::GetType(ToCLRString(CLRObject::TypeOf(value)), true);
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
	else if (node::Buffer::HasInstance(value))
	{
		if (type->IsAssignableFrom(array<System::Byte>::typeid))
		{
			auto length = node::Buffer::Length(value);
			auto data = node::Buffer::Data(value);

			auto arr = gcnew array<System::Byte>((int)length);
			Marshal::Copy((System::IntPtr)data, arr, 0, (int)length);

			match = EXACT;
			return arr;
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
		// from\to
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
	if (value == nullptr)
	{
		return Local<Value>::New(Null());
	}

	switch (System::Type::GetTypeCode(value->GetType()))
	{
	case System::TypeCode::Boolean:
		return Boolean::New((System::Boolean)value);

	case System::TypeCode::SByte:
	case System::TypeCode::Byte:
	case System::TypeCode::Int16:
	case System::TypeCode::UInt16:
	case System::TypeCode::Int32:
	case System::TypeCode::UInt32:
	case System::TypeCode::Int64:
	case System::TypeCode::UInt64:
	case System::TypeCode::Single:
	case System::TypeCode::Double:
	// case System::TypeCode::Decimal:
		return Number::New(System::Convert::ToDouble(value));

	case System::TypeCode::Char:
		return ToV8String(value->ToString());

	case System::TypeCode::String:
		return ToV8String((System::String^)value);

	default:
		return CLRObject::Wrap(value);
	}
}


/*
 * Exception conversion
 */

System::Exception^ ToCLRException(Handle<Value> ex)
{
	if (ex.IsEmpty() ||
		(!ex->IsString() && !ex->IsObject()))
	{
		return gcnew V8InvocationException();
	}
	else if (ex->IsString())
	{
		return gcnew V8InvocationException(ToCLRString(ex));
	}
	else
	{
		auto obj = Handle<Object>::Cast(ex);
		return gcnew V8InvocationException(
			ToCLRString(obj->Get(String::NewSymbol("name"))),
			ToCLRString(obj->Get(String::NewSymbol("message"))),
			ToCLRString(obj->Get(String::NewSymbol("stack"))));
	}
}

Local<Value> ToV8Error(System::Exception^ ex)
{
	auto err = Local<Object>::Cast(Exception::Error(ToV8String(ex->Message)));
	
	auto name = ex->GetType()->Name;
	if (ex->Data["name"] != nullptr)
	{
		name = ex->Data["name"]->ToString();
	}
	err->Set(
		String::NewSymbol("name"),
		ToV8String(name));

	auto stack = gcnew StringBuilder();
	stack->AppendFormat("{0}: {1}", name, ex->Message);
	if (ex->Data["stack"] != nullptr)
	{
		IEnumerable<System::String^>^ lines;
		lines = Regex::Split(ex->Data["stack"]->ToString(), "\r?\n");
		lines = Enumerable::Skip(lines, 1);
		for each (System::String^ line in lines)
		{
			stack->AppendLine();
			stack->Append(line);
		}			
	}
	stack->AppendLine();
	stack->Append(ex->StackTrace);
	{
		IEnumerable<System::String^>^ lines;
		lines = Regex::Split(ToCLRString(err->Get(String::NewSymbol("stack"))), "\r?\n");
		lines = Enumerable::Skip(lines, 1);
		for each (System::String^ line in lines)
		{
			stack->AppendLine();
			stack->Append(line);
		}
	}
	err->Set(String::NewSymbol("stack"), ToV8String(stack->ToString()));

	return err;
}