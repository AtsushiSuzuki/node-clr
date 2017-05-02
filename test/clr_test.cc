#include "../src/clr.h"

using namespace v8;
using namespace System::Collections::Generic;


class Assert
{
public:
	generic <typename T>
	static void Equal(T expected, T actual)
	{
		if (!System::Object::Equals(expected, actual))
		{
			throw gcnew System::Exception(System::String::Format(
				"Assertion failed: expected {0} ({1}), actual {2} ({3})",
				expected,
				(expected != nullptr) ? expected->GetType()->Name : "nullptr",
				actual,
				(actual != nullptr) ? actual->GetType()->Name : "nullptr"));
		}
	}

	generic <typename T>
	static void SequenceEqual(array<T>^ expected, array<T>^ actual)
	{
		if (expected->Length != actual->Length)
		{
			throw gcnew System::Exception(System::String::Format(
				"Assertion failed: expected->Length {0}, actual->Length {1}",
				expected->Length,
				actual->Length));
		}
		for (int i = 0; i < expected->Length; i++)
		{
			if (!System::Object::Equals(expected[i], actual[i]))
			{
				throw gcnew System::Exception(System::String::Format(
					"Assertion failed: expected[{0}] {1} ({2}), actual[{0}] {3} ({4})",
					i,
					expected[i],
					(expected[i] != nullptr) ? expected[i]->GetType()->Name : "nullptr",
					actual[i],
					(actual[i] != nullptr) ? actual[i]->GetType()->Name : "nullptr"));
			}
		}
	}

	generic <typename T>
	static T IsType(System::Object^ actual)
	{
		if (actual == nullptr)
		{
			throw gcnew System::ArgumentNullException("actual");
		}
		if (actual->GetType() != T::typeid)
		{
			throw gcnew System::Exception(System::String::Format(
				"Assertion failed: actual {0}({1}) is not {2}",
				actual,
				actual->GetType(),
				T::typeid));
		}
		return safe_cast<T>(actual);
	}
};

#define TEST(name, block) \
Nan::SetMethod(exports, name, [](const Nan::FunctionCallbackInfo<Value>& info) { \
	try \
	{ \
		block; \
	} \
	catch (System::Exception^ ex) \
	{ \
		Nan::ThrowError(ToV8Error(ex)); \
	} \
})

#define ASSERT(cond) \
	{ \
		if (!cond) { \
			throw gcnew System::Exception("Assertion failed: \"" + #cond + "\""); \
		} \
	}

#define ASSERT_THROW(block, exception) \
	try \
	{ \
		block; \
		throw gcnew System::Exception("Assertion failed: exception " + #exception + " expected"); \
	} \
	catch (exception) \
	{ \
	}

class CLRTest {
public:
    static void Init(Local<Object> exports)
    {
		TEST("Binder::ChangeType should convert `null` to `object`", {
			auto value = Nan::Null();
			auto type = System::Object::typeid;
			
			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(nullptr, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `null` to `int`", {
			auto value = Nan::Null();
			auto type = int::typeid;

			Assert::Equal<int>(INCOMPATIBLE, Binder::CanChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `null` to `string`", {
			auto value = Nan::Null();
			auto type = System::String::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(nullptr, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `null` to `int?`", {
			auto value = Nan::Null();
			auto type = System::Nullable<int>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(nullptr, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `undefined` to `object`", {
			auto value = Nan::Undefined();
			auto type = System::Object::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(nullptr, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `Number` to `double`", {
			auto value = Nan::New<Number>(42);
			auto type = double::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>((double)42, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `Number` to `double?`", {
			auto value = Nan::New<Number>(42);
			auto type = System::Nullable<double>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>((double)42, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `Number` to `int`", {
			auto value = Nan::New<Number>(42.1); // lossy
			auto type = int::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(42, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should fail to convert `number` to `int16` when overflow occurs", {
			auto value = Nan::New<Number>(40000);
			auto type = System::Int16::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			ASSERT_THROW(Binder::ChangeType(value, type), System::OverflowException^);
		});

		TEST("Binder::ChangeType should convert `Number` to `decimal`", {
			auto value = Nan::New<Number>(42);
			auto type = System::Decimal::typeid;

			Assert::Equal<int>(IMPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>((System::Decimal)42, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `Number` to `object`", {
			auto value = Nan::New<Number>(42);
			auto type = System::Object::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>((double)42, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `Nan` to `double`", {
			auto value = Nan::New<Number>(double::NaN);
			auto type = double::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			ASSERT(double::IsNaN((double)Binder::ChangeType(value, type)));
		});

		TEST("Binder::ChangeType should fail to convert `Nan` to `int`", {
			auto value = Nan::New<Number>(double::NaN);
			auto type = int::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			ASSERT_THROW(Binder::ChangeType(value, type), System::InvalidCastException^);
		});

		TEST("Binder::ChangeType should convert `String` to `string`", {
			auto value = Nan::New<String>("hello, world!").ToLocalChecked();
			auto type = System::String::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>("hello, world!", Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `String` to `char`", {
			auto value = Nan::New<String>("x").ToLocalChecked();
			auto type = System::Char::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(System::Char('x'), Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should fail to convert `String` to `char` when string is not with length 1", {
			auto value = Nan::New<String>("hello, world!").ToLocalChecked();
			auto type = System::Char::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			ASSERT_THROW(Binder::ChangeType(value, type), System::InvalidCastException^);
		});

		TEST("Binder::ChangeType should convert `String` to `object`", {
			auto value = Nan::New<String>("hello, world!").ToLocalChecked();
			auto type = System::Object::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>("hello, world!", Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `String` to enum", {
			auto value = Nan::New<String>("Double").ToLocalChecked();
			auto type = System::TypeCode::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(System::TypeCode::Double, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should fail to convert `String` to enum", {
			auto value = Nan::New<String>("hello, world!").ToLocalChecked();
			auto type = System::TypeCode::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			ASSERT_THROW(Binder::ChangeType(value, type), System::InvalidCastException^);
		});

		TEST("Binder::ChangeType should convert CLR object to CLR type", {
			auto object = System::DateTime::Now;
			auto value = (new CLRObject(object))->ToV8Value();
			auto type = System::DateTime::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(object, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert CLR object to `object`", {
			auto object = System::DateTime::Now;
			auto value = (new CLRObject(object))->ToV8Value();
			auto type = System::Object::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::Equal<System::Object^>(object, Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `[]` to `object[]`", {
			auto value = Nan::New<Array>();
			auto type = array<System::Object^>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::IsType<array<System::Object^>^>(Binder::ChangeType(value, type));
			Assert::SequenceEqual<System::Object^>(gcnew array<System::Object^>(0) {}, (array<System::Object^>^)Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `[]` to `int[]`", {
			auto value = Nan::New<Array>();
			auto type = array<int>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::IsType<array<int>^>(Binder::ChangeType(value, type));
			Assert::SequenceEqual<int>(gcnew array<int>(0) {}, (array<int>^)Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `[1]` to `object[]`", {
			auto value = Nan::New<Array>();
			value->Set(0, Nan::New<Number>(1));
			auto type = array<System::Object^>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::IsType<array<System::Object^>^>(Binder::ChangeType(value, type));
			Assert::SequenceEqual<System::Object^>(gcnew array<System::Object^>(1) { (double)1 }, (array<System::Object^>^)Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `[1]` to `double[]`", {
			auto value = Nan::New<Array>();
			value->Set(0, Nan::New<Number>(1));
			auto type = array<double>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			Assert::IsType<array<double>^>(Binder::ChangeType(value, type));
			Assert::SequenceEqual<double>(gcnew array<double>(1) { (double)1 }, (array<double>^)Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `[1]` to `int[]`", {
			auto value = Nan::New<Array>();
			value->Set(0, Nan::New<Number>(1));
			auto type = array<int>::typeid;

			Assert::Equal<int>(EXPLICIT_CONVERSION, Binder::CanChangeType(value, type));
			Assert::IsType<array<int>^>(Binder::ChangeType(value, type));
			Assert::SequenceEqual<int>(gcnew array<int>(1) { 1 }, (array<int>^)Binder::ChangeType(value, type));
		});

		TEST("Binder::ChangeType should convert `Buffer` to `byte[]`", {
			auto value = node::Buffer::New(Nan::GetCurrentContext()->GetIsolate(), 10).ToLocalChecked();
			for (int i = 0; i < 10; i++)
			{
				value->Set(i, Nan::New<Number>(i));
			}
			auto type = array<byte>::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			auto arr = Assert::IsType<array<byte>^>(Binder::ChangeType(value, type));
			Assert::SequenceEqual<byte>(
				gcnew array<byte>(10) { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, },
				arr);
		});

		TEST("Binder::ChangeType should convert `Object` to `object`", {
			auto value = Nan::New<Object>();
			auto type = System::Object::typeid;

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			auto dict = (Assert::IsType<Dictionary<System::String^, System::Object^>^>(Binder::ChangeType(value, type)));
			Assert::Equal<int>(0, dict->Count);
		});

		TEST("Binder::ChangeType should convert `Object` to `Dictionary<string, object>`", {
			auto value = Nan::New<Object>();
			auto type = (Dictionary<System::String^, System::Object^>::typeid);

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			auto dict = (Assert::IsType<Dictionary<System::String^, System::Object^>^>(Binder::ChangeType(value, type)));
			Assert::Equal<int>(0, dict->Count);
		});

		TEST("Binder::ChangeType should convert `Object` to `Dictionary<string, int>`", {
			auto value = Nan::New<Object>();
			auto type = (Dictionary<System::String^, int>::typeid);

			Assert::Equal<int>(EXACT, Binder::CanChangeType(value, type));
			auto dict = (Assert::IsType<Dictionary<System::String^, int>^>(Binder::ChangeType(value, type)));
			Assert::Equal<int>(0, dict->Count);
		});
    }
};

#pragma managed(push, off)
NODE_MODULE(clr_test, CLRTest::Init);
#pragma managed(pop)
