# node-clr : .NET Framework API Binding for Node.js

## Usage:
	# npm install clr
	# node
	
	> require('clr').init();
	> System.Console.WriteLine('Hello, {0}!', 'world');
	'Hello, world!'

	> var now = new System.DateTime(2013, 7, 1);
	> now.ToString();
	'2013/07/01 0:00:00'

	> require('clr').init({ assemblies: [ 'System.Windows.Forms' ] });
	> with (System.Windows.Forms) {
	>   var f = new Form();
	>   
	>   var p = new FlowLayoutPanel();
	>   f.Controls.Add(p);
	>   
	>   var t = new TextBox();
	>   t.Text = 'world';
	>   p.Controls.Add(t);
	>   
	>   var b = new Button();
	>   b.Text = 'Greet';
	>   b.Click.add(function (thiz, ea) {
	>     console.log('clicked');
	>     MessageBox.Show('Hello, ' + t.Text + '!');
	>   });
	>   p.Controls.Add(b);
	> 
	>   Application.Run(f);
	> }
	(running WinForm application)


## Prerequisites:

This library is built and tested on following environment:

- Node.js v6.3.1
- .NET Framework 4.5
- Visual Studio 2015
- [Node.js native module build environment](https://github.com/TooTallNate/node-gyp)


## clr.init([options])

Initialize CLR rutime with given options. Returns global `namespace`.

- `options` {Object}
	- `assemblies` {Array} - An array of assembly name (partial name, full name or absolute path to .dll/.exe).
	  Defaults to `['mscorlib', 'System', 'System.Core']`.
	- `global` {boolean} - if `true`, CLR global namespace objects are injected into javascript global object.
	  Defaults to `true`.


## CLR namespaces

- {Object}

CLR namespace objects contain nested namespaces or types.


## CLR types

- {Function}

CLR type functions work as constructor for corresponding CLR types.
The constructor returns wrapped CLR object.

	var now = new System.DateTime(2013, 7, 1);

The code above invokes CLR constructor [`DateTime (Int32, Int32, Int32)`](http://msdn.microsoft.com/ja-jp/library/xcfzdy4x.aspx)
and returns {Object} that wraps `DateTime` instance.


CLR type also contains static members.

	var now = System.DateTime.Now;

The code above invokes CLR static property getter [`DateTime.Now`](http://msdn.microsoft.com/ja-jp/library/system.datetime.now.aspx).


### type.of([type])

Bind generic parameters to type and returns bound CLR type.

	var list = new (System.Collections.Generic.List.of(System.Int32))();


## CLR objects

- {Object}

Javascript object that wraps CLR instance, which contains instance members.

	var now = System.DateTime.Now;
	now.ToString();
	
	> '2013/07/01 0:00:00'


## CLR methods

- {Function}

CLR methods can be invoked as Javascript function. Arguments and return value are marshalled as conventions below.

	System.Console.WriteLine('Hello, {0}!', 'world');
	
	> 'Hello, world!'

## CLR properties/fields

- {Getter/Setter}

CLR properties/fields are exposed as object's getter or setter function.

	var now = System.DateTime.Now;


## CLR indexed properties

### obj.get([index])

Get object's default indexed property

	var a = 

### obj.set([index], value)


## CLR events

- {Object}

CLR events can be hooked by `add` and `remove` function.


### event.add(handler)

Add javascript event handler to specified event.

- `handler` {Function}


### event.remove(handler)

Remove javascript event handler from event.

** This isn't working right now **

- `handler` {Function}


## Marshaling:

V8 => CLR:

- `null` or `undefined` => `null`
- `boolean` => `System.Boolean`
- `nubmer` => Any numeric type or `System.Double`
- `string` => `System.String` or `System.Char` or Enums
- `Function` => Any delegate type or `System.Func<System.Object[], System.Object>`
- `Array` => Any type of array or `System.Object[]`
- `Buffer` => `System.Byte[]`
- `object` => `System.Dynamic.ExpandoObject`

CLR => V8:

- `null` => `null`
- `System.Boolean` => `boolean`
- Any numberic type excepts System.Decimal => `number`
- `System.String` or `System.Char` => `string`
- Any other types => CLR wrapped object


## Threading:

You can use .NET threads. All Javascript callback functions are invoked in main event loop.

	var t = new System.Threading.Thread(function () {
	  console.log('Hello, world!');
	});
	t.Start();
	
	> 'Hello, world!' // will be invoked asynchronously, but in main thread


## TODO:
- Unit test
- Better marshaling
  - `Object` => class with `DataContractAttribute`
  - handle cyclic reference
- New Event api, resembles to EventEmitter
- Prototype chain which reflects CLR inheritance
- Generic method
- cast
- valueOf (explicit conversion from wrapped CLR object to javascript types)
- Async invocation
- Compiler API
- Enum equality
