#include "node-clr.h"

using namespace v8;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Text::RegularExpressions;

/*
 * main module
 */
class CLR
{
	// clr.import(assemblyName | assemblyPath)
	//   load specified assembly into current process.
	//   - assemblyName: partial name of assembly, ie) "System.Data"
	//   - assemblyPath: .NET EXE/DLL file path relative from cwd
	static Handle<Value> Import(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 1 || !args[0]->IsString())
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		auto name = ToCLRString(args[0]);
		Assembly^ assembly;
		try
		{
			if (File::Exists(name))
			{
				assembly = Assembly::LoadFrom(name);
			}
			else
			{
#pragma warning(push)
#pragma warning(disable:4947)
				assembly = Assembly::LoadWithPartialName(name);
#pragma warning(pop)
			}
		}
		catch (System::Exception^ ex)
		{
			ThrowException(ToV8Error(ex));
			return scope.Close(Undefined());
		}

		if (assembly == nullptr)
		{
			ThrowException(Exception::Error(String::New("Assembly not found")));
			return scope.Close(Undefined());
		}

		return scope.Close(Undefined());
	}
	

	// clr.getAssemblies() : assemblyNames
	//   lists all assembly names in current process
	//   - assemblyNames: array of assembly name string
	static Handle<Value> GetAssemblies(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 0)
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		auto arr = Array::New();
		auto index = 0;
		for each (auto assembly in System::AppDomain::CurrentDomain->GetAssemblies())
		{
			if (assembly == Assembly::GetExecutingAssembly())
			{
				continue;
			}

			arr->Set(Number::New(index++), ToV8String(assembly->FullName));
		}

		return scope.Close(arr);
	}
	

	// clr.getTypes() : typeNames
	//   lists all non-nested type name (Assembly-Qualified-Name) in current process
	//   - typeNames: array of type name string
	static Handle<Value> GetTypes(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 0)
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		auto arr = Array::New();
		auto index = 0;
		for each (auto assembly in System::AppDomain::CurrentDomain->GetAssemblies())
		{
			// exclude current assembly ("clr.node")
			if (assembly == System::Reflection::Assembly::GetExecutingAssembly())
			{
				continue;
			}

			for each (auto type in assembly->GetTypes())
			{
				// exclude non-public types
				if (!type->IsPublic)
				{
					continue;
				}
				// exclude compiler generated types
				if (type->IsSpecialName)
				{
					continue;
				}

				arr->Set(Number::New(index++), ToV8String(type->AssemblyQualifiedName));
			}
		}

		return scope.Close(arr);
	}
	

	// clr.createConstructor(typeName, initializer) : constructor
	//   create new constructor function from given typeName,
	//   - typeName: type name of constructor
	//   - initializer: an function which is invoked in constructor function
	//   - constructor: an constructor function to invoke CLR type constructor, returning CLR wrapped function
	static Handle<Value> CreateConstructor(const Arguments& args)
	{
		HandleScope scope;

		if ((args.Length() != 1 && args.Length() != 2) ||
			!args[0]->IsString() ||
			!args[1]->IsFunction())
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		Handle<Value> result;
		try
		{
			result = CLRObject::CreateConstructor(
				Handle<String>::Cast(args[0]),
				Handle<Function>::Cast(args[1]));
		}
		catch (System::Exception^ ex)
		{
			ThrowException(ToV8Error(ex));
			return scope.Close(Undefined());
		}

		return scope.Close(result);
	}
	

	// clr.getMembers(typeName, CLRObject) : members
	//   list up type's static or instance members
	//   - typeName: type name string
	//   - CLRObject: CLR object instance or null
	//   - members: array of object that contains member information
	//   - members[i].memberType: 'event' | 'field' | 'method' | 'property' | 'nestedType'
	//   - members[i].name: member name
	//   - members[i].accessibility: array of string which denotes member's accessibility, 'get' | 'set'
	//   - members[i].fullName: CLR type's full name for nestedType
	static Handle<Value> GetMembers(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 2 ||
			!args[0]->IsString())
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		auto type = System::Type::GetType(ToCLRString(args[0]));
		auto isStatic = !args[1]->BooleanValue();
		
		auto obj = Object::New();
		auto members = type->GetMembers(
			BindingFlags::Public |
			((isStatic) ? BindingFlags::Static : BindingFlags::Instance));
		for each (auto member in members)
		{
			auto ei = dynamic_cast<EventInfo^>(member);
			if (ei != nullptr &&
				!ei->IsSpecialName &&
				!obj->Has(ToV8Symbol(member->Name)))
			{
				// events
				auto desc = Object::New();
				desc->Set(String::NewSymbol("name"), ToV8String(member->Name));
				desc->Set(String::NewSymbol("type"), String::New("event"));
				obj->Set(ToV8Symbol(member->Name), desc);
			}

			auto fi = dynamic_cast<FieldInfo^>(member);
			if (fi != nullptr &&
				!fi->IsSpecialName &&
				!obj->Has(ToV8Symbol(member->Name)))
			{
				// fields
				auto desc = Object::New();
				desc->Set(String::NewSymbol("name"), ToV8String(member->Name));
				desc->Set(String::NewSymbol("type"), String::New("field"));
				auto access = Array::New();
				int index = 0;
				access->Set(Number::New(index++), String::New("get"));
				if (!fi->IsInitOnly)
				{
					access->Set(Number::New(index++), String::New("set"));
				}
				desc->Set(String::NewSymbol("access"), access);
				obj->Set(ToV8Symbol(member->Name), desc);
			}

			auto mi = dynamic_cast<MethodInfo^>(member);
			if (mi != nullptr &&
				!mi->IsSpecialName &&
				!obj->Has(ToV8Symbol(member->Name)))
			{
				// methods
				auto desc = Object::New();
				desc->Set(String::NewSymbol("name"), ToV8String(member->Name));
				desc->Set(String::NewSymbol("type"), String::New("method"));
				obj->Set(ToV8Symbol(member->Name), desc);
			}

			auto pi = dynamic_cast<PropertyInfo^>(member);
			if (pi != nullptr &&
				!pi->IsSpecialName)
			{
				// properties
				auto desc = (obj->Has(ToV8Symbol(member->Name)))
					? Local<Object>::Cast(obj->Get(ToV8Symbol(member->Name)))
					: Object::New();
				desc->Set(String::NewSymbol("name"), ToV8String(member->Name));
				desc->Set(String::NewSymbol("type"), String::New("property"));
				
				auto access = (obj->Has(String::NewSymbol("access")))
					? Local<Array>::Cast(obj->Get(String::NewSymbol("access")))
					: Array::New();
				auto canGet = pi->CanRead;
				auto canSet = pi->CanWrite;
				for (int i = 0; i < (int)access->Length(); i++)
				{
					if (access->Get(Number::New(i))->StrictEquals(String::New("get")))
					{
						canGet = true;
					}
					if (access->Get(Number::New(i))->StrictEquals(String::New("set")))
					{
						canSet = true;
					}
				}
				int index = 0;
				if (canGet)
				{
					access->Set(Number::New(index++), String::New("get"));
				}
				if (canSet)
				{
					access->Set(Number::New(index++), String::New("set"));
				}
				desc->Set(String::NewSymbol("access"), access);
				
				obj->Set(ToV8Symbol(member->Name), desc);
			}

			auto ti = dynamic_cast<System::Type^>(member);
			if (ti != nullptr &&
				!ti->IsSpecialName &&
				!obj->Has(ToV8Symbol(member->Name)))
			{
				// nested typess
				auto desc = Object::New();
				desc->Set(String::NewSymbol("name"), ToV8String(member->Name));
				desc->Set(String::NewSymbol("type"), String::New("nestedType"));
				desc->Set(String::NewSymbol("fullName"), ToV8String(ti->AssemblyQualifiedName));
				obj->Set(ToV8Symbol(member->Name), desc);
			}
		}

		return scope.Close(obj);
	}

	
	// clr.invokeMethod(typeName, methodName, CLRObject, arguments) : returnValue
	//   invoke static or instance method
	//   - typeName: type name string, for static members
	//   - methodName: method name
	//   - CLRObject: CLR object instance, for instance members
	//   - arguments: array of method arguments
	//   - returnValue: return value of method, v8 primitive or CLR wrapped object
	static Handle<Value> InvokeMethod(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 4 ||
			!args[0]->IsString() ||
			!args[1]->IsString() ||
			(!CLRObject::IsCLRObject(args[2]) && args[2]->BooleanValue() != false) ||
			!args[3]->IsArray())
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		Handle<Value> result;
		try
		{
			result = CLRBinder::InvokeMethod(
				args[0],
				args[1],
				args[2],
				args[3]);
		}
		catch (System::Exception^ ex)
		{
			ThrowException(ToV8Error(ex));
			return scope.Close(Undefined());
		}

		return scope.Close(result);
	}
	

	// clr.getField(typeName, fieldName, CLRObject) : returnValue
	//   invoke field getter
	//   - typeName: type name string, for static members
	//   - fieldName: field name
	//   - CLRObject: CLR object instance, for instance members
	//   - returnValue: field value, v8 primitive or CLR wrapped object
	static Handle<Value> GetField(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 3 ||
			!args[0]->IsString() ||
			!args[1]->IsString() ||
			(!CLRObject::IsCLRObject(args[2]) && args[2]->BooleanValue() != false))
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		Handle<Value> result;
		try
		{
			result = CLRBinder::GetField(
				args[0],
				args[1],
				args[2]);
		}
		catch (System::Exception^ ex)
		{
			ThrowException(ToV8Error(ex));
			return scope.Close(Undefined());
		}

		return scope.Close(result);
	}
	

	// clr.setField(typeName | CLRObject, fieldName, value)
	//   invoke field setter
	//   - typeName: type name string, for static members
	//   - CLRObject: CLR object instance, for instance members
	//   - fieldName: field name
	//   - value: field value, v8 primitive or CLR wrapped object
	static Handle<Value> SetField(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 4 ||
			!args[0]->IsString() ||
			!args[1]->IsString() ||
			(!CLRObject::IsCLRObject(args[2]) && args[2]->BooleanValue() != false) ||
			!args[3].IsEmpty())
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		try
		{
			CLRBinder::SetField(
				args[0],
				args[1],
				args[2],
				args[3]);
		}
		catch (System::Exception^ ex)
		{
			ThrowException(ToV8Error(ex));
			return scope.Close(Undefined());
		}

		return scope.Close(Undefined());
	}
	

	// clr.isCLRObject(obj) : boolean
	//   returns if specified object is CLR wrapped object
	//   - obj: CLR wrapped object or any javascript value
	static Handle<Value> IsCLRObject(const Arguments& args)
	{
		HandleScope scope;

		if (args.Length() != 1 ||
			args[0].IsEmpty())
		{
			ThrowException(Exception::TypeError(String::New("Arguments does not match it's parameter list")));
			return scope.Close(Undefined());
		}

		return scope.Close(Boolean::New(CLRObject::IsCLRObject(args[0])));
	}
	
	// resolve assemblies which is loaded by reflection
	static Assembly^ ResolveAssembly(System::Object^ sender, System::ResolveEventArgs^ ea)
	{
		for each (auto assembly in System::AppDomain::CurrentDomain->GetAssemblies())
		{
			if (assembly->FullName == ea->Name)
			{
				return assembly;
			}
		}

		return nullptr;
	}

public:
	static void Init(Handle<Object> exports)
	{
		CLRObject::Init();

		exports->Set(String::NewSymbol("import"), FunctionTemplate::New(Import)->GetFunction());
		exports->Set(String::NewSymbol("getAssemblies"), FunctionTemplate::New(GetAssemblies)->GetFunction());
		exports->Set(String::NewSymbol("getTypes"), FunctionTemplate::New(GetTypes)->GetFunction());
		exports->Set(String::NewSymbol("createConstructor"), FunctionTemplate::New(CreateConstructor)->GetFunction());
		exports->Set(String::NewSymbol("getMembers"), FunctionTemplate::New(GetMembers)->GetFunction());
		exports->Set(String::NewSymbol("invokeMethod"), FunctionTemplate::New(InvokeMethod)->GetFunction());
		exports->Set(String::NewSymbol("getField"), FunctionTemplate::New(GetField)->GetFunction());
		exports->Set(String::NewSymbol("setField"), FunctionTemplate::New(SetField)->GetFunction());
		exports->Set(String::NewSymbol("isCLRObject"), FunctionTemplate::New(IsCLRObject)->GetFunction());

		System::AppDomain::CurrentDomain->AssemblyResolve += gcnew System::ResolveEventHandler(
			&CLR::ResolveAssembly);
	}
};

NODE_MODULE(clr, CLR::Init);
