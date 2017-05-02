#include "clr.h"

using namespace v8;
using namespace System::Collections::Generic;
using namespace System::Text;
using namespace System::Text::RegularExpressions;


/*
 * String conversion
 */

System::String^ ToCLRString(Local<Value> value)
{
	return gcnew System::String((const wchar_t *)(*String::Value(value)));
}

Local<String> ToV8String(System::String^ value)
{
	pin_ptr<const wchar_t> ptr = PtrToStringChars(value);
	return Nan::New<String>((const uint16_t*)ptr).ToLocalChecked();
}


/* 
 * Value conversion
 */

Local<Value> ToV8Value(System::Object^ object)
{
	if (object != nullptr)
	{
		return (new CLRObject(object))->ToV8Value();
	}
	else
	{
		return Nan::Null();
	}
}

System::Object^ ToCLRValue(Local<Value> value)
{
	return Binder::ChangeType(value, System::Object::typeid);
}

System::Object^ ToCLRValue(Local<Value> value, System::Type^ type)
{
	if (Binder::CanChangeType(value, type) <= INCOMPATIBLE)
	{
		throw gcnew System::InvalidCastException(System::String::Format(
			"Cannot convert {0} to {1}",
			ToCLRString(value->ToDetailString()),
			type));
	}

	return Binder::ChangeType(value, type);
}


/*
 * Exception conversions
 */

Local<Value> ToV8Error(System::Exception^ ex)
{
	auto err = Local<Object>::Cast(Exception::Error(ToV8String(ex->Message)));
	
	auto name = ex->GetType()->Name;
	if (ex->Data["name"] != nullptr)
	{
		name = ex->Data["name"]->ToString();
	}
	err->Set(
		Nan::New<String>("name").ToLocalChecked(),
		ToV8String(name));

	auto stack = gcnew StringBuilder();
	stack->AppendFormat("{0}: {1}", name, ex->Message);
	if (ex->Data["stack"] != nullptr)
	{
		auto lines = Regex::Split(ex->Data["stack"]->ToString(), "\r?\n");
		for (int i = 1; i < lines->Length; i++)
		{
			stack->AppendLine();
			stack->Append(lines[i]);
		}
	}
	stack->AppendLine();
	stack->Append(ex->StackTrace);
	{
		auto lines = Regex::Split(ToCLRString(err->Get(Nan::New<String>("stack").ToLocalChecked())), "\r?\n");
		for (int i = 1; i < lines->Length; i++)
		{
			stack->AppendLine();
			stack->Append(lines[i]);
		}
	}
	err->Set(Nan::New<String>("stack").ToLocalChecked(), ToV8String(stack->ToString()));

	return err;
}
