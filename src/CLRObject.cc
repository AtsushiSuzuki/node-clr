#include "node-clr.h"

using namespace v8;


Nan::Persistent<v8::ObjectTemplate> CLRObject::objectTemplate_;

void CLRObject::Init()
{
	auto tmpl = Nan::New<ObjectTemplate>();
	tmpl->SetInternalFieldCount(1);
	objectTemplate_.Reset(tmpl);
}

bool CLRObject::IsCLRObject(Local<Value> value)
{
	if (!value.IsEmpty() && value->IsObject() && !value->IsFunction())
	{
		return Nan::HasPrivate(value->ToObject(), Nan::New<String>("clr::type").ToLocalChecked()).ToChecked();
	}
	else
	{
		return false;
	}
}

Local<Value> CLRObject::GetType(Local<Value> value)
{
	return Nan::GetPrivate(Nan::To<Object>(value).ToLocalChecked(), Nan::New<String>("clr::type").ToLocalChecked()).ToLocalChecked();
}

bool CLRObject::IsCLRConstructor(Local<Value> value)
{
	if (!value.IsEmpty() && value->IsFunction())
	{
		auto type = Nan::GetPrivate(Nan::To<Object>(value).ToLocalChecked(), Nan::New<String>("clr::type").ToLocalChecked());
		return !type.IsEmpty();
	}
	else
	{
		return false;
	}
}

Local<Value> CLRObject::TypeOf(Local<Value> value)
{
	return Nan::GetPrivate(Nan::To<Object>(value).ToLocalChecked(), Nan::New<String>("clr::type").ToLocalChecked()).ToLocalChecked();
}

Local<Object> CLRObject::Wrap(Local<Object> obj, System::Object^ value)
{
	auto wrapper = new CLRObject(value);
	wrapper->node::ObjectWrap::Wrap(obj);
	
	auto name = (value != nullptr)
		? ToV8String(value->GetType()->AssemblyQualifiedName)
		: ToV8String(System::Object::typeid->AssemblyQualifiedName);

	Nan::SetPrivate(obj, Nan::New<String>("clr::type").ToLocalChecked(), name);

	return obj;
}

Local<Object> CLRObject::Wrap(System::Object^ value)
{
	auto tmpl = Nan::New<ObjectTemplate>(objectTemplate_);
	auto obj = tmpl->NewInstance();
	return Wrap(obj, value);
}

System::Object^ CLRObject::Unwrap(Local<Value> obj)
{
	if (!IsCLRObject(obj))
	{
		throw gcnew System::ArgumentException("argument \"obj\" is not CLR-wrapped object");
	}

	auto wrapper = node::ObjectWrap::Unwrap<CLRObject>(obj->ToObject());
	return wrapper->value_;
}

Local<Function> CLRObject::CreateConstructor(Local<String> typeName, Local<Function> initializer)
{
	auto type = System::Type::GetType(ToCLRString(typeName), true);

	auto tpl = Nan::New<FunctionTemplate>(New);
	tpl->SetClassName(ToV8String(type->Name));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	auto ctor = tpl->GetFunction();
	Nan::SetPrivate(ctor, Nan::New<String>("clr::type").ToLocalChecked(), ToV8String(type->AssemblyQualifiedName));
	Nan::SetPrivate(ctor, Nan::New<String>("clr::initializer").ToLocalChecked(), initializer);

	return ctor;
}

NAN_METHOD(CLRObject::New)
{
	Nan::HandleScope scope;

	if (!info.IsConstructCall())
	{
		return Nan::ThrowError("Illegal invocation");
	}

	auto ctor = info.Callee();
	auto typeName = Nan::GetPrivate(ctor, Nan::New<String>("clr::type").ToLocalChecked()).ToLocalChecked();

	auto arr = Nan::New<Array>();
	for (int i = 0; i < info.Length(); i++)
	{
		arr->Set(Nan::New<Number>(i), info[i]);
	}
	
	System::Object^ value;
	try
	{
		value = CLRBinder::InvokeConstructor(typeName, arr);
	}
	catch (System::Exception^ ex)
	{
		Nan::ThrowError(ToV8Error(ex));
		return;
	}
	
	Wrap(info.This(), value);

	auto initializer = Nan::GetPrivate(ctor, Nan::New<String>("clr::initializer").ToLocalChecked());
	if (!initializer.IsEmpty())
	{
		std::vector<Local<Value> > params;
		for (int i = 0; i < info.Length(); i++)
		{
			params.push_back(info[i]);
		}
		Local<Function>::Cast(initializer.ToLocalChecked())->Call(info.This(), info.Length(), (0 < params.size()) ? &(params[0]) : nullptr);
	}
}

CLRObject::CLRObject(System::Object^ value)
	: value_(value)
{
}

CLRObject::~CLRObject()
{
}