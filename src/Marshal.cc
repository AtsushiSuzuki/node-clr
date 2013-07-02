#include "node-clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Runtime::InteropServices;


System::String^ CLRString(Handle<Value> value)
{
	return gcnew System::String((const wchar_t *)(*String::Value(value)));
}

Local<String> V8String(System::String ^value)
{
	pin_ptr<const wchar_t> ptr = PtrToStringChars(value);
	return String::New((const uint16_t *)ptr);
}

Local<String> V8Symbol(System::String ^value)
{
	auto ptr = (const char *)(Marshal::StringToHGlobalAnsi(value)).ToPointer();
	auto result = String::NewSymbol(ptr);
	Marshal::FreeHGlobal(System::IntPtr((void*)ptr));
	return result;
}

System::Object^ CLRValue(Handle<Value> value)
{
	HandleScope scope;
	
	// primitives OR primitive-ish values
	if (value->IsNull() || value->IsUndefined())
	{
		return nullptr;
	}
	else if (value->IsBoolean() || value->IsBooleanObject())
	{
		return value->BooleanValue();
	}
	else if (value->IsNumber() || value->IsNumberObject())
	{
		if (value->IsInt32())
		{
			return value->Int32Value();
		}
		else
		{
			return value->NumberValue();
		}
	}
	else if (value->IsString() || value->IsStringObject())
	{
		return CLRString(value);
	}
	// objects
	else if (value->IsFunction())
	{
		auto wrapper = gcnew V8Callback(Handle<Function>::Cast(value));
		return gcnew System::Func<array<System::Object^>, System::Object^>(wrapper, &(V8Callback::Invoke));
	}
	else if (CLRObject::IsWrapped(value))
	{
		return CLRObject::Unwrap(value);
	}
	else if (value->IsArray())
	{
		// TODO: handle cyclic reference
		auto from = Handle<Array>::Cast(value);
		auto to = gcnew array<System::Object^>(from->Length());
		for (unsigned int i = 0; i < from->Length(); i++)
		{
			to[i] = CLRValue(from->Get(Number::New(i)));
		}
		return to;
	}
	else
	{
		// TODO: handle cyclic reference
		auto from = Handle<Object>::Cast(value);
		auto to = gcnew Dictionary<System::String^, System::Object^>();
		auto names = from->GetOwnPropertyNames();
		for (unsigned int i = 0; i < names->Length(); i++)
		{
			auto name = names->Get(i);
			to[CLRString(name)] = CLRValue(from->Get(name));
		}
		return to;
	}
}

Handle<Value> V8Value(System::Object^ value)
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
		return V8String((System::String^)value);
	}
	// objects
	else
	{
		return CLRObject::Wrap(value);
	}
	// TODO: map Task<T> to promise
}

array<System::Object^>^ CLRArguments(const Arguments &args)
{
	auto arr = gcnew array<System::Object^>(args.Length());
	for (int i = 0; i < args.Length(); i++)
	{
		arr[i] = (CLRValue(args[i]));
	}
	return arr;
}

array<System::Object^>^ CLRArguments(Handle<Array> args)
{
	auto arr = gcnew array<System::Object^>(args->Length());
	for (unsigned int i = 0; i < args->Length(); i++)
	{
		arr[i] = (CLRValue(args->Get(Number::New(i))));
	}
	return arr;
}

std::vector<Handle<Value> > V8Arguments(array<System::Object^>^ args)
{
	std::vector<Handle<Value> > v;

	for (int i = 0; i < args->Length; i++)
	{
		v.push_back(V8Value(args[i]));
	}

	return v;
}

Local<Value> V8Exception(System::Exception ^ex)
{
	return Exception::Error(V8String(ex->ToString()));
}
