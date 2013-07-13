#include "node-clr.h"

using namespace v8;


Handle<Object> CLRObject::Wrap(Handle<Object> obj, System::Object^ value)
{
	auto wrapper = new CLRObject(value);
	wrapper->node::ObjectWrap::Wrap(obj);
	
	obj->Set(
		String::NewSymbol("_clrType"),
		(value != nullptr)
			? ToV8String(value->GetType()->AssemblyQualifiedName)
			: ToV8String(System::Object::typeid->AssemblyQualifiedName),
		(PropertyAttribute)(ReadOnly | DontEnum | DontDelete));

	return obj;
}

Handle<Object> CLRObject::Wrap(System::Object^ value)
{
	auto tpl = ObjectTemplate::New();
	tpl->SetInternalFieldCount(1);

	auto obj = tpl->NewInstance();
	return Wrap(obj, value);
}

bool CLRObject::IsWrapped(Handle<Value> obj)
{
	return obj->IsObject() &&
		obj->ToObject()->Has(String::NewSymbol("_clrType"));
}

System::Object^ CLRObject::Unwrap(Handle<Value> obj)
{
	if (!IsWrapped(obj))
	{
		throw gcnew System::ArgumentException("argument \"obj\" is not CLR-wrapped object");
	}

	auto wrapper = node::ObjectWrap::Unwrap<CLRObject>(obj->ToObject());
	return wrapper->value;
}

Local<Function> CLRObject::CreateConstructor(Handle<String> typeName, Handle<Function> initializer)
{
	auto type = CLRGetType(ToCLRString(typeName));

	auto data = Object::New();
	data->Set(String::NewSymbol("type"), ToV8String(type->AssemblyQualifiedName));
	data->Set(String::NewSymbol("initializer"), initializer);

	auto tpl = FunctionTemplate::New(New, data);
	tpl->SetClassName(ToV8String(type->Name));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	return tpl->GetFunction();
}

Handle<Value> CLRObject::New(const Arguments& args)
{
	HandleScope scope;

	if (!args.IsConstructCall())
	{
		ThrowException(Exception::Error(String::New("Illegal invocation")));
		return scope.Close(Undefined());
	}

	auto typeName = args.Data()->ToObject()->Get(String::NewSymbol("type"));
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

	auto initializer = args.Data()->ToObject()->Get(String::NewSymbol("initializer"));
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
	: value(value)
{
}

CLRObject::~CLRObject()
{
}