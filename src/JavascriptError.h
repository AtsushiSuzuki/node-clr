#ifndef V8EXCEPTION_H_
#define V8EXCEPTION_H_

#include "node-clr.h"


[System::Serializable]
public ref class JavascriptError : public System::Exception
{
public:
	JavascriptError();

	JavascriptError(
		System::String^ message);

	JavascriptError(
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
	JavascriptError(
		System::Runtime::Serialization::SerializationInfo^ info,
		System::Runtime::Serialization::StreamingContext context);
};

#endif