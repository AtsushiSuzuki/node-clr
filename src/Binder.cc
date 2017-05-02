#include "clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Reflection;


System::Object^ ::Binder::Construct(
	System::Type^ type,
	const Nan::FunctionCallbackInfo<Value>& args)
{
	auto ctors = type->GetConstructors(BindingFlags::Public | BindingFlags::Static);
	auto arr = gcnew array<MethodBase^>(ctors->Length);
	for (int i = 0; i < ctors->Length; i++)
	{
		arr[i] = ctors[i];
	}

	return Invoke(nullptr, arr, args);
}

System::Type^ ::Binder::GetNestedType(
	System::Type^ type,
	System::String^ name)
{
	return type->GetNestedType(name);
}

System::Object^ ::Binder::GetField(
	System::Type^ type,
	System::Object^ target,
	System::String^ name)
{
	auto fi = type->GetField(name);
	return fi->GetValue(target);
}

void ::Binder::SetField(
	System::Type^ type,
	System::Object^ target,
	System::String^ name,
	Local<Value> value)
{
	auto fi = type->GetField(name);
	fi->SetValue(target, ToCLRValue(value, fi->FieldType));
}

System::Object^ ::Binder::GetProperty(
	System::Type^ type,
	System::Object^ target,
	System::String^ name)
{
	auto pi = type->GetProperty(name);
	return pi->GetValue(target);
}

void ::Binder::SetProperty(
	System::Type^ type,
	System::Object^ target,
	System::String^ name,
	Local<Value> value)
{
	auto pi = type->GetProperty(name);
	pi->SetValue(target, ToCLRValue(value, pi->PropertyType));
}

System::Object^ ::Binder::InvokeMethod(
	System::Type^ type,
	System::Object^ object,
	System::String^ methodName,
	const Nan::FunctionCallbackInfo<Value>& args)
{
	auto methods = type->GetMethods(
		((object != nullptr) ? BindingFlags::Instance : BindingFlags::Static) | BindingFlags::Public | BindingFlags::FlattenHierarchy);
	auto arr = gcnew array<MethodBase^>(methods->Length);
	for (int i = 0; i < methods->Length; i++)
	{
		arr[i] = methods[i];
	}

	return Invoke(object, arr, args);
}

int ::Binder::CanChangeType(
	Local<Value> value,
	System::Type^ type)
{
	// null value
	if (value.IsEmpty() || value->IsNull() || value->IsUndefined())
	{
		if (!type->IsValueType)
		{
			return EXACT;
		}
		else if (type->IsGenericType &&
			type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
		{
			return EXACT;
		}
		return INCOMPATIBLE;
	}

	// unwarp nullable
	if (type->IsGenericType &&
		type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
	{
		type = type->GetGenericArguments()[0];
	}

	// primitives
	if (value->IsBoolean() || value->IsBooleanObject())
	{
		return (type->IsAssignableFrom(bool::typeid)) ? EXACT : INCOMPATIBLE;
	}
	if (value->IsNumber() || value->IsNumberObject())
	{
		switch (System::Type::GetTypeCode(type))
		{
		case System::TypeCode::Char:
		case System::TypeCode::SByte:
		case System::TypeCode::Byte:
		case System::TypeCode::Int16:
		case System::TypeCode::UInt16:
		case System::TypeCode::Int32:
		case System::TypeCode::UInt32:
		case System::TypeCode::Int64:
		case System::TypeCode::UInt64:
		case System::TypeCode::Single:
			return EXPLICIT_CONVERSION;
		case System::TypeCode::Double:
			return EXACT;
		case System::TypeCode::Decimal:
			return IMPLICIT_CONVERSION;
		default:
			return (type->IsAssignableFrom(double::typeid)) ? EXACT : INCOMPATIBLE;
		}
	}
	if (value->IsString() || value->IsStringObject())
	{
		switch (System::Type::GetTypeCode(type))
		{
		case System::TypeCode::Char:
			return EXPLICIT_CONVERSION;
		case System::TypeCode::String:
			return EXACT;
		default:
			if (type->IsEnum)
			{
				return EXPLICIT_CONVERSION;
			}
			return (type->IsAssignableFrom(System::String::typeid)) ? EXACT : INCOMPATIBLE;
		}
	}

	// objects
	if (CLRObject::IsCLRObject(value))
	{
		auto object = CLRObject::Unwrap(value)->object_;
		return (type->IsAssignableFrom(object->GetType())) ? EXACT : INCOMPATIBLE;
	}
	if (value->IsFunction())
	{
		throw gcnew System::NotImplementedException();
	}
	if (value->IsArray())
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

		if (type->IsAssignableFrom(elementType->MakeArrayType()))
		{
			auto arr = Local<Array>::Cast(value); // Nan method?
			int match = EXACT;
			for (int i = 0; i < (int)arr->Length(); i++)
			{
				match = System::Math::Min(match, CanChangeType(arr->Get(i), elementType));
			}
			return match;
		}
		else
		{
			return INCOMPATIBLE;
		}
	}
	if (node::Buffer::HasInstance(value))
	{
		return (type->IsAssignableFrom(array<System::Byte>::typeid)) ? EXACT : INCOMPATIBLE;
	}
	if (value->IsObject())
	{
		// TODO: handle cyclic reference

		auto keyType = System::String::typeid;
		auto elementType = System::Object::typeid;
		if (type->IsGenericType)
		{
			auto d = type->GetGenericTypeDefinition();
			if (d == IDictionary<int, int>::typeid->GetGenericTypeDefinition() ||
				d == Dictionary<int, int>::typeid->GetGenericTypeDefinition())
			{
				auto typeArgs = type->GetGenericArguments();
				if (typeArgs[0] == System::Object::typeid ||
					typeArgs[0] == System::String::typeid)
				{
					keyType = typeArgs[0];
					elementType = typeArgs[1];
				}
			}
		}

		int match = EXACT;
		auto obj = Nan::To<Object>(value).ToLocalChecked();
		auto names = obj->GetPropertyNames();
		for (int i = 0; i < (int)names->Length(); i++)
		{
			auto name = names->Get(i);
			match = System::Math::Min(match, CanChangeType(obj->Get(name), elementType));
		}

		auto dictionaryType = Dictionary<int, int>::typeid
			->GetGenericTypeDefinition()
			->MakeGenericType(keyType, elementType);
		return (type->IsAssignableFrom(dictionaryType)) ? match : INCOMPATIBLE;
	}

	return INCOMPATIBLE;
}

template <typename T>
static T numeric_cast(double v)
{
	if (double::IsNaN(v))
	{
		throw gcnew System::InvalidCastException(System::String::Format(
			"Cannot convert Nan to {0}", T::typeid));
	}
	if (double::IsInfinity(v))
	{
		throw gcnew System::InvalidCastException(System::String::Format(
			"Cannot convert Inf to {0}", T::typeid));
	}
	if (v < (double)T::MinValue || (double)T::MaxValue < v)
	{
		throw gcnew System::OverflowException();
	}
	return static_cast<T>(v);
}

template <>
static System::Single numeric_cast<System::Single>(double v)
{
	using T = System::Single;

	if (double::IsNaN(v))
	{
		return T::NaN;
	}
	if (double::IsPositiveInfinity(v))
	{
		return T::PositiveInfinity;
	}
	if (double::IsNegativeInfinity(v))
	{
		return T::NegativeInfinity;
	}
	if (v < (double)T::MinValue || (double)T::MaxValue < v)
	{
		throw gcnew System::OverflowException();
	}
	return static_cast<T>(v);
}

System::Object^ ::Binder::ChangeType(
	Local<Value> value,
	System::Type^ type)
{
	// null value
	if (value.IsEmpty() || value->IsNull() || value->IsUndefined())
	{
		return nullptr;
	}

	// unwarp nullable
	if (type->IsGenericType &&
		type->GetGenericTypeDefinition() == System::Nullable<int>::typeid->GetGenericTypeDefinition())
	{
		type = type->GetGenericArguments()[0];
	}

	// primitives
	if (value->IsBoolean() || value->IsBooleanObject())
	{
		return value->BooleanValue();
	}
	if (value->IsNumber() || value->IsNumberObject())
	{
		switch (System::Type::GetTypeCode(type))
		{
		case System::TypeCode::Char:
			return numeric_cast<System::Char>(value->NumberValue());
		case System::TypeCode::SByte:
			return numeric_cast<System::SByte>(value->NumberValue());
		case System::TypeCode::Byte:
			return numeric_cast<System::Byte>(value->NumberValue());
		case System::TypeCode::Int16:
			return numeric_cast<System::Int16>(value->NumberValue());
		case System::TypeCode::UInt16:
			return numeric_cast<System::UInt16>(value->NumberValue());
		case System::TypeCode::Int32:
			return numeric_cast<System::Int32>(value->NumberValue());
		case System::TypeCode::UInt32:
			return numeric_cast<System::UInt32>(value->NumberValue());
		case System::TypeCode::Int64:
			return numeric_cast<System::Int64>(value->NumberValue());
		case System::TypeCode::UInt64:
			return numeric_cast<System::UInt64>(value->NumberValue());
		case System::TypeCode::Single:
			return numeric_cast<System::Single>(value->NumberValue());
		case System::TypeCode::Double:
			return value->NumberValue();
		case System::TypeCode::Decimal:
			return static_cast<System::Decimal>(value->NumberValue());
		default:
			return value->NumberValue();
		}
	}
	if (value->IsString() || value->IsStringObject())
	{
		auto str = ToCLRString(value->ToString());
		switch (System::Type::GetTypeCode(type))
		{
		case System::TypeCode::Char:
			if (str->Length == 1)
			{
				return str[0];
			}
			else
			{
				throw gcnew System::InvalidCastException("Cannot convert string to char; string must consits of 1 code point");
			}
		case System::TypeCode::String:
			return str;
		default:
			if (type->IsEnum)
			{
				if (!System::Enum::IsDefined(type, str))
				{
					throw gcnew System::InvalidCastException(System::String::Format(
						"Cannot convert string \"{0}\" to enum {1}",
						str,
						type));
				}
				return System::Enum::Parse(type, str);
			}
			return str;
		}
	}

	// objects
	if (CLRObject::IsCLRObject(value))
	{
		return CLRObject::Unwrap(value)->object_;
	}
	if (value->IsFunction())
	{
		throw gcnew System::NotImplementedException();
	}
	if (value->IsArray())
	{
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

		auto arr = Local<Array>::Cast(value);
		auto result = System::Array::CreateInstance(elementType, arr->Length());
		for (int i = 0; i < (int)arr->Length(); i++)
		{
			result->SetValue(ChangeType(arr->Get(i), elementType), i);
		}
		return result;
	}
	if (node::Buffer::HasInstance(value))
	{
		auto len = node::Buffer::Length(value);
		auto data = node::Buffer::Data(value);

		auto arr = gcnew array<System::Byte>((int)len);
		System::Runtime::InteropServices::Marshal::Copy((System::IntPtr)data, arr, 0, (int)len);

		return arr;
	}
	if (value->IsObject())
	{
		// TODO: handle cyclic reference

		auto keyType = System::String::typeid;
		auto elementType = System::Object::typeid;
		if (type->IsGenericType)
		{
			auto d = type->GetGenericTypeDefinition();
			if (d == IDictionary<int, int>::typeid->GetGenericTypeDefinition() ||
				d == Dictionary<int, int>::typeid->GetGenericTypeDefinition())
			{
				auto typeArgs = type->GetGenericArguments();
				if (typeArgs[0] == System::Object::typeid ||
					typeArgs[0] == System::String::typeid)
				{
					keyType = typeArgs[0];
					elementType = typeArgs[1];
				}
			}
		}

		auto dictionaryType = Dictionary<int, int>::typeid
			->GetGenericTypeDefinition()
			->MakeGenericType(keyType, elementType);
		auto dict = (System::Collections::IDictionary^)System::Activator::CreateInstance(dictionaryType);

		auto obj = Nan::To<Object>(value).ToLocalChecked();
		auto names = obj->GetPropertyNames();
		for (int i = 0; i < (int)names->Length(); i++)
		{
			auto name = names->Get(i);
			dict[ToCLRString(name)] = ChangeType(obj->Get(i), elementType);
		}

		return dict;
	}

	throw gcnew System::InvalidCastException(System::String::Format(
		"Cannot convert {0} to {1}",
		ToCLRString(value->ToDetailString()),
		type));
}

System::Object^ ::Binder::Invoke(
	System::Object^ object,
	array<MethodBase^>^ methods,
	const Nan::FunctionCallbackInfo<Value>& args)
{
	throw gcnew System::NotImplementedException();
}
