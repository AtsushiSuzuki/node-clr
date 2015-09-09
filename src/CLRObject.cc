#include "node-clr.h"

using namespace v8;


Nan::Persistent<v8::ObjectTemplate> CLRObject::objectTemplate_;

void CLRObject::Init()
{
	auto tmpl = Nan::New<ObjectTemplate>();
	tmpl->SetInternalFieldCount(1);
	objectTemplate_.Reset(tmpl);
}

bool CLRObject::IsCLRObject(Handle<Value> value)
{
	if (!value.IsEmpty() && value->IsObject() && !value->IsFunction())
	{
		auto type = Handle<Object>::Cast(value)->GetHiddenValue(Nan::New<String>("clr::type").ToLocalChecked());
		return !type.IsEmpty();
	}
	else
	{
		return false;
	}
}

Handle<Value> CLRObject::GetType(Handle<Value> value)
{
	return Handle<Object>::Cast(value)->GetHiddenValue(Nan::New<String>("clr::type").ToLocalChecked());
}

bool CLRObject::IsCLRConstructor(Handle<Value> value)
{
	if (!value.IsEmpty() && value->IsFunction())
	{
		auto type = value->ToObject()->GetHiddenValue(Nan::New<String>("clr::type").ToLocalChecked());
		return !type.IsEmpty();
	}
	else
	{
		return false;
	}
}

Handle<Value> CLRObject::TypeOf(Handle<Value> value)
{
	return Handle<Object>::Cast(value)->GetHiddenValue(Nan::New<String>("clr::type").ToLocalChecked());
}

Handle<Object> CLRObject::Wrap(Handle<Object> obj, System::Object^ value)
{
	auto wrapper = new CLRObject(value);
	wrapper->node::ObjectWrap::Wrap(obj);
	
	auto name = (value != nullptr)
		? ToV8String(value->GetType()->AssemblyQualifiedName)
		: ToV8String(System::Object::typeid->AssemblyQualifiedName);

	obj->SetHiddenValue(
		Nan::New<String>("clr::type").ToLocalChecked(),
		name);

	return obj;
}

Handle<Object> CLRObject::Wrap(System::Object^ value)
{
	auto tmpl = Nan::New<ObjectTemplate>(objectTemplate_);
	auto obj = tmpl->NewInstance();
	return Wrap(obj, value);
}

System::Object^ CLRObject::Unwrap(Handle<Value> obj)
{
	if (!IsCLRObject(obj))
	{
		throw gcnew System::ArgumentException("argument \"obj\" is not CLR-wrapped object");
	}

	auto wrapper = node::ObjectWrap::Unwrap<CLRObject>(obj->ToObject());
	return wrapper->value_;
}

Local<Function> CLRObject::CreateConstructor(Handle<String> typeName, Handle<Function> initializer)
{
	auto type = System::Type::GetType(ToCLRString(typeName), true);

	auto tpl = Nan::New<FunctionTemplate>(New);
	tpl->SetClassName(ToV8String(type->Name));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	auto ctor = tpl->GetFunction();
	ctor->SetHiddenValue(Nan::New<String>("clr::type").ToLocalChecked(), ToV8String(type->AssemblyQualifiedName));
	ctor->SetHiddenValue(Nan::New<String>("clr::initializer").ToLocalChecked(), initializer);

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
	auto typeName = ctor->GetHiddenValue(Nan::New<String>("clr::type").ToLocalChecked());

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

	auto initializer = ctor->GetHiddenValue(Nan::New<String>("clr::initializer").ToLocalChecked());
	if (!initializer.IsEmpty())
	{
		std::vector<Handle<Value> > params;
		for (int i = 0; i < info.Length(); i++)
		{
			params.push_back(info[i]);
		}
		Local<Function>::Cast(initializer)->Call(info.This(), info.Length(), (0 < params.size()) ? &(params[0]) : nullptr);
	}
}

CLRObject::CLRObject(System::Object^ value)
	: value_(value)
{
}

CLRObject::~CLRObject()
{
}