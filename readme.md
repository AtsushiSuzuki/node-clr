# node-clr : Node.js binding for .NET Framework API

## example:
	> require('clr').init();
	> System.Console.WriteLine('Hello, {0}!', 'world');
	
	'Hello, world!'
	
	> var now = new System.DateTime(2013, 7, 1);
	> now.ToString();
	
	'2013/07/01 0:00:00'


## prerequisites:

This package is developed and tested on:

- node.js v0.10.12
- .NET Framework 4.5
- Visual Studio 2012


## usage:
- clr.init([options]) : Namespace
	- initialize CLR runtime with given assemblies and returns global namespace
- options.assemblies: Array [default: mscorlib, System, System.Core]
	- specify referenced assemblies
- options.global: bool [default: true]
	- inject CLR namespaces to global
- options.imbue: bool [default: true]
	- define each instance member on each instance, not on prototype

### class Namespace:
- namespace.{namespace name} : Namespace
	- get nested namespace
- namespace.{type name} : Type
	- get type

### class Type:
- new type(args...) : Any
	- invoke CLR constructor and returns wrapped CLR object or primitive
- type.{static method name}(args...) : Any
	- invoke static method and returns wrapped CLR object or primitive
- type.{static property or field name} : Any
	- invoke static property or field getter
- type.{static property or field name} = value
	- invoke static proprety or field setter

### class Object:
- object.{instance method name}(args...) : Any
	- invoke instance method and returns wrapped CLR object or primitive
- object.{property or field name} : Any
	- invoke instance property or field getter
- object.{static property or field name} = value
	- invoke instance proprety or field setter

## marshaling:
### V8 => CLR:
- null or undefined => null
- boolean or Boolean => System.Boolean
- number or Nubmer => System.Int32 or System.Double
- string or String => System.String
- function => System.Func<System.Object[], System.Object>
- array => System.Object[]
- object => System.Collections.Generic.Dictionary<System.String, System.Object>

### CLR => V8
- null or no return value => null
- System.Boolean => boolean
- System.SByte, System.Byte, System.Int16, System.UInt16, System.Int32, System.UInt32, System.Int64, System.UInt64, System.Single, System.Double, System.Decimal => number
- System.String => string
- System.Object => wrapped object