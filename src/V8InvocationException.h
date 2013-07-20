#ifndef V8EXCEPTION_H_
#define V8EXCEPTION_H_

#include "node-clr.h"


// Exception that holds Javascript Error information
[System::Serializable]
public ref class V8InvocationException : public System::Exception
{
public:
	V8InvocationException();

	V8InvocationException(
		System::String^ message);

	V8InvocationException(
		System::String^ name,
		System::String^ message,
		System::String^ stack);


	property System::String^ Name
	{
		System::String^ get();
		private: void set(System::String^ value);
	}

	property System::String^ Stack
	{
		System::String^ get();
		private: void set(System::String^ value);
	}

	virtual property System::String^ StackTrace
	{
		System::String^ get() override;
	}

	virtual System::String^ ToString() override;

protected:
	V8InvocationException(
		System::Runtime::Serialization::SerializationInfo^ info,
		System::Runtime::Serialization::StreamingContext context);
};

#endif