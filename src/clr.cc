#include "clr.h"

using namespace v8;


class CLR {
public:
    static void Init(Local<Object> exports)
    {
		auto type = CLRType::GetInstance(System::Guid::typeid);
        Nan::Set(exports,
                 Nan::New("Guid").ToLocalChecked(),
                 type->ToV8Value());
		Nan::SetMethod(exports,
			"GetType",
			GetTypeCallback);
    }

	static void GetTypeCallback(const Nan::FunctionCallbackInfo<Value>& info)
	{
		if (info.Length() != 1 || !info[0]->IsString())
		{
			return Nan::ThrowTypeError("Invalid arguments");
		}

		auto name = ToCLRString(Nan::To<String>(info[0]).ToLocalChecked());
		auto type = System::Type::GetType(name);
		info.GetReturnValue().Set(CLRType::GetInstance(type)->ToV8Value());
	}
};

#pragma managed(push, off)
NODE_MODULE(clr, CLR::Init);
#pragma managed(pop)
