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
	static NAN_METHOD(Import)
	{
		Nan::HandleScope scope;

		if (info.Length() != 1 || !info[0]->IsString())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		auto name = ToCLRString(info[0]);
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
			Nan::ThrowError(ToV8Error(ex));
			return;
		}

		if (assembly == nullptr)
		{
			Nan::ThrowError("Assembly not found");
			return;
		}
	}
	

	// clr.getAssemblies() : assemblyNames
	//   lists all assembly names in current process
	//   - assemblyNames: array of assembly name string
	static NAN_METHOD(GetAssemblies)
	{
		Nan::HandleScope scope;

		if (info.Length() != 0)
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		auto arr = Nan::New<Array>();
		auto index = 0;
		for each (auto assembly in System::AppDomain::CurrentDomain->GetAssemblies())
		{
			if (assembly == Assembly::GetExecutingAssembly())
			{
				continue;
			}

			Nan::Set(arr, Nan::New<Number>(index++), ToV8String(assembly->FullName));
		}

		info.GetReturnValue().Set(arr);
	}
	

	// clr.getTypes() : typeNames
	//   lists all non-nested type name (Assembly-Qualified-Name) in current process
	//   - typeNames: array of type name string
	static NAN_METHOD(GetTypes)
	{
		Nan::HandleScope scope;

		if (info.Length() != 0)
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		auto arr = Nan::New<Array>();
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

				Nan::Set(arr, Nan::New<Number>(index++), ToV8String(type->AssemblyQualifiedName));
			}
		}

		info.GetReturnValue().Set(arr);
	}
	

	// clr.createConstructor(typeName, initializer) : constructor
	//   create new constructor function from given typeName,
	//   - typeName: type name of constructor
	//   - initializer: an function which is invoked in constructor function
	//   - constructor: an constructor function to invoke CLR type constructor, returning CLR wrapped function
	static NAN_METHOD(CreateConstructor)
	{
		Nan::HandleScope scope;

		if ((info.Length() != 1 && info.Length() != 2) ||
			!info[0]->IsString() ||
			!info[1]->IsFunction())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		Local<Value> result;
		try
		{
			result = CLRObject::CreateConstructor(
				Local<String>::Cast(info[0]),
				Local<Function>::Cast(info[1]));
		}
		catch (System::Exception^ ex)
		{
			Nan::ThrowError(ToV8Error(ex));
			return;
		}

		info.GetReturnValue().Set(result);
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
	static NAN_METHOD(GetMembers)
	{
		Nan::HandleScope scope;

		if (info.Length() != 2 ||
			!info[0]->IsString())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		auto type = System::Type::GetType(ToCLRString(info[0]), true);
		auto isStatic = !Nan::To<bool>(info[1]).ToChecked();
		
		auto obj = Nan::New<Object>();
		auto members = type->GetMembers(
			BindingFlags::Public |
			((isStatic) ? BindingFlags::Static : BindingFlags::Instance));
		for each (auto member in members)
		{
			auto ei = dynamic_cast<EventInfo^>(member);
			if (ei != nullptr &&
				!ei->IsSpecialName &&
				!Nan::Has(obj, ToV8Symbol(member->Name)).ToChecked())
			{
				// events
				auto desc = Nan::New<Object>();
				Nan::Set(desc, Nan::New<String>("name").ToLocalChecked(), ToV8String(member->Name));
				Nan::Set(desc, Nan::New<String>("type").ToLocalChecked(), Nan::New<String>("event").ToLocalChecked());
				Nan::Set(obj, ToV8Symbol(member->Name), desc);
			}

			auto fi = dynamic_cast<FieldInfo^>(member);
			if (fi != nullptr &&
				!fi->IsSpecialName &&
				!Nan::Has(obj, ToV8Symbol(member->Name)).ToChecked())
			{
				// fields
				auto desc = Nan::New<Object>();
				Nan::Set(desc, Nan::New<String>("name").ToLocalChecked(), ToV8String(member->Name));
				Nan::Set(desc, Nan::New<String>("type").ToLocalChecked(), Nan::New<String>("field").ToLocalChecked());
				auto access = Nan::New<Array>();
				int index = 0;
				Nan::Set(access, Nan::New<Number>(index++), Nan::New<String>("get").ToLocalChecked());
				if (!fi->IsInitOnly)
				{
					Nan::Set(access, Nan::New<Number>(index++), Nan::New<String>("set").ToLocalChecked());
				}
				Nan::Set(desc, Nan::New<String>("access").ToLocalChecked(), access);
				Nan::Set(obj, ToV8Symbol(member->Name), desc);
			}

			auto mi = dynamic_cast<MethodInfo^>(member);
			if (mi != nullptr &&
				!mi->IsSpecialName &&
				!Nan::Has(obj, ToV8Symbol(member->Name)).ToChecked())
			{
				// methods
				auto desc = Nan::New<Object>();
				Nan::Set(desc, Nan::New<String>("name").ToLocalChecked(), ToV8String(member->Name));
				Nan::Set(desc, Nan::New<String>("type").ToLocalChecked(), Nan::New<String>("method").ToLocalChecked());
				Nan::Set(obj, ToV8Symbol(member->Name), desc);
			}

			auto pi = dynamic_cast<PropertyInfo^>(member);
			if (pi != nullptr &&
				!pi->IsSpecialName)
			{
				// properties
				auto desc = (Nan::Has(obj, ToV8Symbol(member->Name)).ToChecked())
					? Local<Object>::Cast(Nan::Get(obj, ToV8Symbol(member->Name)).ToLocalChecked())
					: Nan::New<Object>();
				Nan::Set(desc, Nan::New<String>("name").ToLocalChecked(), ToV8String(member->Name));
				Nan::Set(desc, Nan::New<String>("type").ToLocalChecked(), Nan::New<String>("property").ToLocalChecked());
				
				auto access = (Nan::Has(obj, Nan::New<String>("access").ToLocalChecked()).ToChecked())
					? Local<Array>::Cast(Nan::Get(obj, Nan::New<String>("access").ToLocalChecked()).ToLocalChecked())
					: Nan::New<Array>();
				auto canGet = pi->CanRead;
				auto canSet = pi->CanWrite;
				for (int i = 0; i < (int)access->Length(); i++)
				{
					if (Nan::Get(access, Nan::New<Number>(i)).ToLocalChecked()->StrictEquals(Nan::New<String>("get").ToLocalChecked()))
					{
						canGet = true;
					}
					if (Nan::Get(access, Nan::New<Number>(i)).ToLocalChecked()->StrictEquals(Nan::New<String>("set").ToLocalChecked()))
					{
						canSet = true;
					}
				}
				int index = 0;
				if (canGet)
				{
					Nan::Set(access, Nan::New<Number>(index++), Nan::New<String>("get").ToLocalChecked());
				}
				if (canSet)
				{
					Nan::Set(access, Nan::New<Number>(index++), Nan::New<String>("set").ToLocalChecked());
				}
				Nan::Set(desc, Nan::New<String>("access").ToLocalChecked(), access);

				Nan::Set(desc, Nan::New<String>("indexed").ToLocalChecked(), Nan::New<Boolean>(0 < pi->GetIndexParameters()->Length));
				
				Nan::Set(obj, ToV8Symbol(member->Name), desc);
			}

			auto ti = dynamic_cast<System::Type^>(member);
			if (ti != nullptr &&
				!ti->IsSpecialName &&
				!Nan::Has(obj, ToV8Symbol(member->Name)).ToChecked())
			{
				// nested typess
				auto desc = Nan::New<Object>();
				Nan::Set(desc, Nan::New<String>("name").ToLocalChecked(), ToV8String(member->Name));
				Nan::Set(desc, Nan::New<String>("type").ToLocalChecked(), Nan::New<String>("nestedType").ToLocalChecked());
				Nan::Set(desc, Nan::New<String>("fullName").ToLocalChecked(), ToV8String(ti->AssemblyQualifiedName));
				Nan::Set(obj, ToV8Symbol(member->Name), desc);
			}
		}

		info.GetReturnValue().Set(obj);
	}

	
	// clr.invokeMethod(typeName, methodName, CLRObject, arguments) : returnValue
	//   invoke static or instance method
	//   - typeName: type name string, for static members
	//   - methodName: method name
	//   - CLRObject: CLR object instance, for instance members
	//   - arguments: array of method arguments
	//   - returnValue: return value of method, v8 primitive or CLR wrapped object
	static NAN_METHOD(InvokeMethod)
	{
		Nan::HandleScope scope;

		if (info.Length() != 4 ||
			!info[0]->IsString() ||
			!info[1]->IsString() ||
			(!CLRObject::IsCLRObject(info[2]) && Nan::To<bool>(info[2]).ToChecked() != false) ||
			!info[3]->IsArray())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		Local<Value> result;
		try
		{
			result = CLRBinder::InvokeMethod(
				info[0],
				info[1],
				info[2],
				info[3]);
		}
		catch (System::Exception^ ex)
		{
			Nan::ThrowError(ToV8Error(ex));
			return;
		}

		info.GetReturnValue().Set(result);
	}
	

	// clr.getField(typeName, fieldName, CLRObject) : returnValue
	//   invoke field getter
	//   - typeName: type name string, for static members
	//   - fieldName: field name
	//   - CLRObject: CLR object instance, for instance members
	//   - returnValue: field value, v8 primitive or CLR wrapped object
	static NAN_METHOD(GetField)
	{
		Nan::HandleScope scope;

		if (info.Length() != 3 ||
			!info[0]->IsString() ||
			!info[1]->IsString() ||
			(!CLRObject::IsCLRObject(info[2]) && Nan::To<bool>(info[2]).ToChecked() != false))
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		Local<Value> result;
		try
		{
			result = CLRBinder::GetField(
				info[0],
				info[1],
				info[2]);
		}
		catch (System::Exception^ ex)
		{
			Nan::ThrowError(ToV8Error(ex));
			return;
		}

		info.GetReturnValue().Set(result);
	}
	

	// clr.setField(typeName | CLRObject, fieldName, value)
	//   invoke field setter
	//   - typeName: type name string, for static members
	//   - CLRObject: CLR object instance, for instance members
	//   - fieldName: field name
	//   - value: field value, v8 primitive or CLR wrapped object
	static NAN_METHOD(SetField)
	{
		Nan::HandleScope scope;

		if (info.Length() != 4 ||
			!info[0]->IsString() ||
			!info[1]->IsString() ||
			(!CLRObject::IsCLRObject(info[2]) && Nan::To<bool>(info[2]).ToChecked() != false) ||
			!info[3].IsEmpty())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		try
		{
			CLRBinder::SetField(
				info[0],
				info[1],
				info[2],
				info[3]);
		}
		catch (System::Exception^ ex)
		{
			Nan::ThrowError(ToV8Error(ex));
			return;
		}
	}
	

	// clr.isCLRObject(obj) : boolean
	//   returns if specified object is CLR wrapped object
	//   - obj: CLR wrapped object or any javascript value
	static NAN_METHOD(IsCLRObject)
	{
		Nan::HandleScope scope;

		if (info.Length() != 1 ||
			info[0].IsEmpty())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		info.GetReturnValue().Set(Nan::New<Boolean>(CLRObject::IsCLRObject(info[0])));
	}

	static NAN_METHOD(GetType)
	{
		Nan::HandleScope scope;

		if (info.Length() != 1 ||
			!CLRObject::IsCLRObject(info[0]))
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		info.GetReturnValue().Set(CLRObject::GetType(info[0]));
	}

	static NAN_METHOD(IsCLRConstructor)
	{
		Nan::HandleScope scope;

		if (info.Length() != 1 ||
			info[0].IsEmpty())
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}
		
		info.GetReturnValue().Set(CLRObject::TypeOf(info[0]));
	}
	
	static NAN_METHOD(TypeOf)
	{
		Nan::HandleScope scope;

		if (info.Length() != 1 ||
			!CLRObject::IsCLRConstructor(info[0]))
		{
			Nan::ThrowTypeError("Arguments does not match it's parameter list");
			return;
		}

		info.GetReturnValue().Set(CLRObject::TypeOf(info[0]));
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
	static void Init(Local<Object> exports)
	{
		CLRObject::Init();

		Nan::Set(exports, Nan::New<String>("import").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(Import)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("getAssemblies").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(GetAssemblies)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("getTypes").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(GetTypes)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("createConstructor").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(CreateConstructor)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("getMembers").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(GetMembers)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("invokeMethod").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(InvokeMethod)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("getField").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(GetField)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("setField").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(SetField)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("isCLRObject").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(IsCLRObject)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("getType").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(GetType)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("isCLRConstructor").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(IsCLRConstructor)).ToLocalChecked());
		Nan::Set(exports, Nan::New<String>("typeOf").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(TypeOf)).ToLocalChecked());

		System::AppDomain::CurrentDomain->AssemblyResolve += gcnew System::ResolveEventHandler(
			&CLR::ResolveAssembly);
	}
};

#pragma managed(push, off)
NODE_MODULE(clr, CLR::Init);
#pragma managed(pop)
