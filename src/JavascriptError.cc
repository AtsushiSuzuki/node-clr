#include "node-clr.h"

using namespace v8;
using namespace System::Runtime::Serialization;
using namespace System::Text;

JavascriptError::JavascriptError()
	: System::Exception("Exception thrown from Javascript function")
{
	this->Name = nullptr;
	this->Stack = nullptr;
}

JavascriptError::JavascriptError(
	System::String^ message)
	: System::Exception(message)
{
	this->Name = nullptr;
	this->Stack = nullptr;
}

JavascriptError::JavascriptError(
	System::String^ name,
	System::String^ message,
	System::String^ stack)
	: System::Exception(message)
{
	this->Name = name;
	this->Stack = stack;
}

JavascriptError::JavascriptError(
	SerializationInfo^ info,
	StreamingContext context)
	: System::Exception(info, context)
{
}

System::String^ JavascriptError::Name::get()
{
	return (System::String^)this->Data["Name"];
}

void JavascriptError::Name::set(System::String^ value)
{
	this->Data["Name"] = value;
}

System::String^ JavascriptError::Stack::get()
{
	return (System::String^)this->Data["Stack"];
}

void JavascriptError::Stack::set(System::String^ value)
{
	this->Data["Stack"] = value;
}

System::String^ JavascriptError::StackTrace::get()
{
	auto sb = gcnew StringBuilder();

	if (this->Stack != nullptr)
	{
		sb->Append(this->Stack);
		sb->AppendLine();
	}
	sb->Append(this->System::Exception::StackTrace);

	return sb->ToString();
}

System::String^ JavascriptError::ToString()
{
	auto sb = gcnew StringBuilder();

	sb->Append(this->GetType()->Name);
	sb->Append(": ");
	if (this->Name != nullptr)
	{
		sb->Append(this->Name);
		sb->Append(": ");
	}
	sb->Append(this->Message);

	sb->AppendLine();
	sb->Append(this->StackTrace);

	return sb->ToString();
}