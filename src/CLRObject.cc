#include "node-clr.h"

using namespace v8;


Persistent<ObjectTemplate> CLRObject::objectTemplate_;

void CLRObject::Init()
{
	auto tmpl = NanNew<ObjectTemplate>();
	tmpl->SetInternalFieldCount(1);
	NanAssignPersistent(objectTemplate_, tmpl);
}

bool CLRObject::IsCLRObject(Handle<Value> value)
{
	if (!value.IsEmpty() && value->IsObject() && !value->IsFunction())
	{
		auto type = Handle<Object>::Cast(value)->GetHiddenValue(NanNew<String>("clr::type"));
		return !type.IsEmpty();
	}
	else
	{
		return false;
	}
}

Handle<Value> CLRObject::GetType(Handle<Value> value)
{
	return Handle<Object>::Cast(value)->GetHiddenValue(NanNew<String>("clr::type"));
}

bool CLRObject::IsCLRConstructor(Handle<Value> value)
{
	if (!value.IsEmpty() && value->IsFunction())
	{
		auto type = value->ToObject()->GetHiddenValue(NanNew<String>("clr::type"));
		return !type.IsEmpty();
	}
	else
	{
		return false;
	}
}

Handle<Value> CLRObject::TypeOf(Handle<Value> value)
{
	return Handle<Object>::Cast(value)->GetHiddenValue(NanNew<String>("clr::type"));
}

Handle<Object> CLRObject::Wrap(Handle<Object> obj, System::Object^ value)
{
	auto wrapper = new CLRObject(value);
	wrapper->node::ObjectWrap::Wrap(obj);
	
	auto name = (value != nullptr)
		? ToV8String(value->GetType()->AssemblyQualifiedName)
		: ToV8String(System::Object::typeid->AssemblyQualifiedName);

	obj->SetHiddenValue(
		NanNew<String>("clr::type"),
		name);

	return obj;
}

Handle<Object> CLRObject::Wrap(System::Object^ value)
{
	auto tmpl = NanNew<ObjectTemplate>(objectTemplate_);
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

	auto tpl = NanNew<FunctionTemplate>(New);
	tpl->SetClassName(ToV8String(type->Name));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	
	auto ctor = tpl->GetFunction();
	ctor->SetHiddenValue(NanNew<String>("clr::type"), ToV8String(type->AssemblyQualifiedName));
	ctor->SetHiddenValue(NanNew<String>("clr::initializer"), initializer);

	return ctor;
}

NAN_METHOD(CLRObject::New)
{
	NanScope();

	if (!args.IsConstructCall())
	{
		NanThrowError("Illegal invocation");
		NanReturnUndefined();
	}

	auto ctor = args.Callee();
	auto typeName = ctor->GetHiddenValue(NanNew<String>("clr::type"));

	auto arr = NanNew<Array>();
	for (int i = 0; i < args.Length(); i++)
	{
		arr->Set(NanNew<Number>(i), args[i]);
	}
	
	System::Object^ value;
	try
	{
		value = CLRBinder::InvokeConstructor(typeName, arr);
	}
	catch (System::Exception^ ex)
	{
		NanThrowError(ToV8Error(ex));
		NanReturnUndefined();
	}
	
	Wrap(args.This(), value);

	auto initializer = ctor->GetHiddenValue(NanNew<String>("clr::initializer"));
	if (!initializer.IsEmpty())
	{
		std::vector<Handle<Value> > params;
		for (int i = 0; i < args.Length(); i++)
		{
			params.push_back(args[i]);
		}
		Local<Function>::Cast(initializer)->Call(args.This(), args.Length(), (0 < params.size()) ? &(params[0]) : nullptr);
	}

	NanReturnUndefined();
}

CLRObject::CLRObject(System::Object^ value)
	: value_(value)
{
}

CLRObject::~CLRObject()
{
}