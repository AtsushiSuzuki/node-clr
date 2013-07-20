#include "node-clr.h"

using namespace v8;
using namespace System::Runtime::Serialization;
using namespace System::Text;

V8InvocationException::V8InvocationException()
	: System::Exception("Exception thrown from Javascript function")
{
	this->Name = nullptr;
	this->Stack = nullptr;
}

V8InvocationException::V8InvocationException(
	System::String^ message)
	: System::Exception(message)
{
	this->Name = nullptr;
	this->Stack = nullptr;
}

V8InvocationException::V8InvocationException(
	System::String^ name,
	System::String^ message,
	System::String^ stack)
	: System::Exception(message)
{
	this->Name = name;
	this->Stack = stack;
}

V8InvocationException::V8InvocationException(
	SerializationInfo^ info,
	StreamingContext context)
	: System::Exception(info, context)
{
}

System::String^ V8InvocationException::Name::get()
{
	return (System::String^)this->Data["Name"];
}

void V8InvocationException::Name::set(System::String^ value)
{
	this->Data["Name"] = value;
}

System::String^ V8InvocationException::Stack::get()
{
	return (System::String^)this->Data["Stack"];
}

void V8InvocationException::Stack::set(System::String^ value)
{
	this->Data["Stack"] = value;
}

// merge Javascript stacktrace with .NET stacktrace
System::String^ V8InvocationException::StackTrace::get()
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

System::String^ V8InvocationException::ToString()
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