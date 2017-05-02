#include "clr.h"

using namespace v8;


CLRObject::CLRObject(System::Object^ value)
    : object_(value)
{
	if (value == nullptr)
	{
		throw gcnew System::ArgumentNullException("value");
	}
}

CLRObject::~CLRObject()
{
}

Local<Object> CLRObject::ToV8Value()
{
	auto tmpl = Nan::New<ObjectTemplate>();
	tmpl->SetInternalFieldCount(1);

	auto value = Nan::NewInstance(tmpl).ToLocalChecked();
	Nan::SetPrivate(value, Nan::New<String>("clr").ToLocalChecked(), Nan::New<String>("object").ToLocalChecked());

	auto ctor = CLRType::GetInstance(object_->GetType())->ToV8Value();
	auto proto = Nan::Get(ctor, Nan::New<String>("prototype").ToLocalChecked()).ToLocalChecked();
	Nan::SetPrototype(value, proto);

	this->Wrap(value);

	return value;
}

bool CLRObject::IsCLRObject(Local<Value> value)
{
	if (value.IsEmpty() || !value->IsObject())
	{
		return false;
	}
	auto obj = Nan::To<Object>(value).ToLocalChecked();
	auto kind = Nan::GetPrivate(obj, Nan::New<String>("clr").ToLocalChecked());
	return !kind.IsEmpty() && kind.ToLocalChecked()->Equals(Nan::New<String>("object").ToLocalChecked());
}

CLRObject* CLRObject::Unwrap(Local<Value> value)
{
	return Nan::ObjectWrap::Unwrap<CLRObject>(Nan::To<Object>(value).ToLocalChecked());
}

