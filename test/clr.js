var assert = require('assert');
var clr = require('../lib/clr');

describe('clr', function () {
  describe('#init', function () {
    it('should success', function () {
      var ns = clr.init({ global: false });

      assert(ns);
    });
  });

  describe('namespaces', function () {
    it('should have nested namespaces', function () {
      var ns = clr.init({ global: false });

      assert(ns.System.IO);
      assert(ns.System.IO.Ports);
    });

    it('should have classes', function () {
      var ns = clr.init({ global: false });

      assert(ns.System.Console);
      assert(ns.System.IO.Stream);
    });
  });

  describe('classes', function () {
    it('should work as constructor', function () {
      var ns = clr.init({ global: false });

      var dt = new ns.System.DateTime();
      assert(dt);
      assert(clr.isCLRObject(dt));
    });

    // TODO: static event

    it('#static field getter should work', function () {
      var ns = clr.init({ global: false });

      var empty = ns.System.String.Empty;
      assert.strictEqual(empty, '');
    });

    // TODO: static field setter

    it('#static property getter should work', function () {
      var ns = clr.init({ global: false });

      var now = ns.System.DateTime.Now;
      assert(now);
      assert(clr.isCLRObject(now));
    });

    // TODO: static property setter

    it('#static method should work', function () {
      var ns = clr.init({ global: false });

      var r = ns.System.String.Format('Hello, {0}!', 'world');
      assert.strictEqual(r, 'Hello, world!');
    });
    
    it('#instance property getter should work', function () {
      var ns = clr.init({ global: false });

      var dt = new ns.System.DateTime(1970, 1, 1);
      assert.strictEqual(dt.Year, 1970);
    });

    it('#instance property setter should work', function () {
      var ns = clr.init({ global: false });

      var ex = new ns.System.Exception();
      ex.Source = 'here';

      assert.strictEqual(ex.Source, 'here');
    });

    it('#instance method should work', function () {
      var ns = clr.init({ global: false });

      var dt = new ns.System.DateTime(1970, 1, 1);

      assert.strictEqual(dt.ToString(), '1970/01/01 0:00:00');
    });
  });

  describe('enums', function () {
    it('should work', function () {
      var ns = clr.init({ global: false });

      var obj = ns.System.TypeCode.Boolean;
      assert(typeof obj === 'object');
      assert(clr.isCLRObject(obj));
    });
  });

  describe("functions", function () {
    it("varargs with 0 item should work.", function () {
      var ns = clr.init({ global: false });

      var str = ns.System.String.Join(",");
      assert.equal(str, "");
    });

    it("varargs with 1 item should work.", function () {
      var ns = clr.init({ global: false });

      var str = ns.System.String.Join(",", "1");
      assert.equal(str, "1");
    });

    it("varargs with 2 item should work.", function () {
      var ns = clr.init({ global: false });

      var str = ns.System.String.Join(",", "1", "2");
      assert.equal(str, "12");
    });

    it("varargs with array items should work.", function () {
      var ns = clr.init({ global: false });

      var str = ns.System.String.Join(",", ["1", "2", "3"]);
      assert.equal(str, "123");
    });
  });

  describe("undefined", function () {
    it("undefined as parameter should be treated as null.", function () {
      var ns = clr.init({ global: false });

      try {
        ns.System.Math.Abs(undefined);
        assert.fail();
      } catch (ex) {
        assert.equal("MissingMethodException", ex.name);
      }
    });
  });
  
  it('async callback should work', function (done) {
    var ns = clr.init({ global: false });

    var called = 0;
    var t = new ns.System.Threading.Tasks.Task(function () { called++; });
    t.Start();

    setTimeout(function () {
      assert.strictEqual(called, 1);
      done();
    }, 100);
  });
  
  it('async callback should really work', function (done) {
    var ns = clr.init({ global: false });
    
    var called = 0;
    for (var i = 0; i < 200; i++) {
      var t = new ns.System.Threading.Tasks.Task(function () { called++; });
      t.Start();
    }
    
    setTimeout(function () {
      assert.strictEqual(called, 200);
      done();
    }, 1000);
  });
});
