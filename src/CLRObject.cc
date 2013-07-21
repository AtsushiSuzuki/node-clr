#include "node-clr.h"

using namespace v8;


Persistent<ObjectTemplate> CLRObject::objectTemplate_;

void CLRObject::Init()
{
	objectTemplate_ = Persistent<ObjectTemplate>::New(ObjectTemplate::New());
	objectTemplate_->SetInternalFieldCount(1);
}

bool CLRObject::IsCLRObject(Handle<Value> value)
{
	if (!value.IsEmpty() && value->IsObject() && !value->IsFunction())
	{
		auto type = value->ToObject()->GetHiddenValue(String::NewSymbol("clr::type"));
		return !type.IsEmpty();
	}
}

bool CLRObject::IsCLRConstructor(Handle<Value> value)
{
	if (!value.IsEmpty() && value->IsFunction())
	{
		auto type = value->ToObject()->GetHiddenValue(String::NewSymbol("clr::type"));
		return !type.IsEmpty();
	}
}

Handle<Object> CLRObject::Wrap(Handle<Object> obj, System::Object^ value)
{
	auto wrapper = new CLRObject(value);
	wrapper->node::ObjectWrap::Wrap(obj);
	
	auto name = (value != nullptr)
		? ToV8String(value->GetType()->AssemblyQualifiedName)
		: ToV8String(System::Object::typeid->AssemblyQualifiedName);

	obj->SetHiddenValue(
		String::NewSymbol("clr::type"),
		name);
	obj->Set(
		String::NewSymbol("clr::type"),
		name,
		(PropertyAttribute)(ReadOnly | DontEnum | DontDelete));

	return obj;
}

Handle<Object> CLRObject::Wrap(System::Object^ value)
{
	auto obj = objectTemplate_->NewInstance();
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
	auto type = System::Type::GetType(ToCLRString(typeName));

	auto tpl = FunctionTemplate::New(New);
	tpl->SetClassName(ToV8String(type->Name));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	auto ctor = tpl->GetFunction();
	ctor->SetHiddenValue(String::NewSymbol("clr::type"), ToV8String(type->AssemblyQualifiedName));
	ctor->SetHiddenValue(String::NewSymbol("clr::initializer"), initializer);

	return ctor;
}

Handle<Value> CLRObject::New(const Arguments& args)
{
	HandleScope scope;

	if (!args.IsConstructCall())
	{
		ThrowException(Exception::Error(String::New("Illegal invocation")));
		return scope.Close(Undefined());
	}

	auto ctor = args.Callee();
	auto typeName = ctor->GetHiddenValue(String::NewSymbol("clr::type"));
	System::Object^ value;
	try
	{
		value = CLRBinder::InvokeConstructor(typeName, args);
	}
	catch (System::Exception^ ex)
	{
		ThrowException(ToV8Error(ex));
		return scope.Close(Undefined());
	}
	
	Wrap(args.This(), value);

	auto initializer = ctor->GetHiddenValue(String::NewSymbol("clr::initializer"));
	if (!initializer.IsEmpty())
	{
		std::vector<Handle<Value> > params;
		for (int i = 0; i < args.Length(); i++)
		{
			params.push_back(args[i]);
		}
		Local<Function>::Cast(initializer)->Call(args.This(), args.Length(), (0 < params.size()) ? &(params[0]) : nullptr);
	}

	return scope.Close(Undefined());
}

CLRObject::CLRObject(System::Object^ value)
	: value_(value)
{
}

CLRObject::~CLRObject()
{
}