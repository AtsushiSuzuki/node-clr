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
	v8::Handle<v8::Value> value,
	System::Type^ type)
{
	if (value->IsNull() || value->IsUndefined())
	{
		return nullptr;
	}
	else if (value->IsBoolean())
	{
		return value->BooleanValue();
	}
	else if (value->IsInt32())
	{
		return value->Int32Value();
	}
	else if (value->IsUint32())
	{
		return value->Uint32Value();
	}
	else if (value->IsNumber())
	{
		return value->NumberValue();
	}
	else if (value->IsString())
	{
		return ToCLRString(value);
	}
	else if (value->IsFunction())
	{
		auto func = gcnew V8Function(Handle<Function>::Cast(value));
		if (type != nullptr && System::Delegate::typeid->IsAssignableFrom(type))
		{
			return func->CreateDelegate(type);
		}
		else
		{
			return func->CreateDelegate();
		}
	}
	else if (value->IsArray())
	{
		// TODO: handle cyclic reference
		System::Type^ elementType = nullptr;
		if (type != nullptr && type->IsArray && type->HasElementType)
		{
			elementType = type->GetElementType();
		}
		else if (type != nullptr &&
			type->IsAssignableFrom(System::Array::typeid) &&
			type->IsGenericType)
		{
			elementType = type->GetGenericArguments()[0];
		}
		
		auto from = Handle<Array>::Cast(value);
		auto to = gcnew array<System::Object^>(from->Length());
		for (unsigned int i = 0; i < from->Length(); i++)
		{
			to[i] = ToCLRValue(from->Get(i), elementType);
		}
		
		if (elementType != nullptr && elementType != System::Object::typeid)
		{
			auto typedArray = System::Array::CreateInstance(elementType, from->Length());
			try
			{
				System::Array::Copy(to, typedArray, to->Length);
				return typedArray;
			}
			catch (System::InvalidCastException^)
			{
			}
		}

		return to;
	}
	else if (value->IsObject())
	{
		// TODO: handle DataContractAttribute
		// TODO: handle cyclic reference
		auto from = Handle<Object>::Cast(value);
		IDictionary<System::String^, System::Object^>^ to = gcnew ExpandoObject();
		auto names = from->GetOwnPropertyNames();
		for (unsigned int i = 0; i < names->Length(); i++)
		{
			auto name = names->Get(i);
			to[ToCLRString(name)] = ToCLRValue(from->Get(name), nullptr);
		}
		return to;
	}
	else if (value->IsExternal())
	{
		return (System::IntPtr)(External::Unwrap(value));
	}

	throw gcnew System::NotImplementedException();
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

array<System::Object^>^ ToCLRArguments(
	Handle<Array> args,
	array<ParameterInfo^>^ params)
{
	auto arr = gcnew array<System::Object^>(args->Length());
	for (int i = 0; i < (int)args->Length(); i++)
	{
		arr[i] = ToCLRValue(
			args->Get(Number::New(i)),
			(params != nullptr && i < params->Length)
				? params[i]->ParameterType
				: nullptr);
	}
	return arr;
}

std::vector<Handle<Value> > ToV8Arguments(array<System::Object^>^ args)
{
	std::vector<Handle<Value> > v;

	for (int i = 0; i < args->Length; i++)
	{
		v.push_back(ToV8Value(args[i]));
	}

	return v;
}

Local<Value> ToV8Exception(System::Exception^ ex)
{
	return Exception::Error(ToV8String(ex->ToString()));
}