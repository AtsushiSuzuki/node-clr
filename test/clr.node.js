var assert = require('assert');
var _ = require('underscore');
var clr = require('bindings')('clr.node');

describe('clr.node', function () {

  describe('#import()', function () {
    it('should success with partial name', function () {
      clr.import('System.Xml');
    });

    it('should fail with unknown assembly name', function () {
      assert.throws(function () {
        clr.import('asdf');
      });
    });
  });

  describe('#getAssemblies()', function () {
    it('should success', function () {
      var assemblies = clr.getAssemblies();

      assert(0 < assemblies.length);
    });
  });

  describe('#getTypes()', function () {
    it('should success', function () {
      var types = clr.getTypes();

      assert(0 < types.length);
    });

    it('should not contains nested type', function () {
      var types = clr.getTypes();

      assert(_.every(types, function (item) { return item.indexOf('+') === -1; }));
    });
  });
  
  describe('#createConstructor()', function () {
    it('should create constructor of System.Object', function () {
      var ctor = clr.createConstructor('System.Object', function () { });

      assert(typeof ctor === 'function');
    });

    describe('#System.Object.ctor', function () {
      it('should create System.Object instance', function () {
        var ctor = clr.createConstructor('System.Object', function () { });
        var obj = new ctor();
      });
    });

    describe('#System.DateTime.ctor', function () {
      it('should create System.DateTime instance', function () {
        var ctor = clr.createConstructor('System.DateTime', function () { });
        var obj = new ctor();
      });

      it('should work with parameters', function () {
        var ctor = clr.createConstructor('System.DateTime', function () { });
        var obj = new ctor(2013, 7, 7);
      });
    });
  });

  describe('#getMembers()', function () {
    it('should retrieve static members', function () {
      var members = clr.getMembers('System.Object', null);

      assert(0 < _.size(members))
    });

    it('should retrieve instance members', function () {
      var ctor = clr.createConstructor('System.Object', function () { });
      var obj = new ctor();
      var members = clr.getMembers('System.Object', obj);

      assert(0 < _.size(members));
    });
    
    it('should retrieve events', function () {
      var obj = clr.invokeMethod('System.AppDomain', 'get_CurrentDomain', null, []);
      var members = clr.getMembers('System.AppDomain', obj);
      
      assert(_.some(members, function (item) {
        return item.name === 'UnhandledException' &&
          item.type === 'event';
      }));
    });

    it('should retrieve fields', function () {
      var members = clr.getMembers('System.String', null);

      assert(_.some(members, function (item) {
        return item.name === 'Empty' &&
          item.type === 'field' &&
          item.access.length === 1 &&
          item.access[0] === 'get';
      }));
    });
    
    it('should retrieve methods', function () {
      var members = clr.getMembers('System.String', null);

      assert(_.some(members, function (item) {
        return item.name === 'Format' &&
          item.type === 'method';
      }));
    });
    
    it('should retrieve properties', function () {
      var members = clr.getMembers('System.AppDomain', null);

      assert(_.some(members, function (item) {
        return item.name === 'CurrentDomain' &&
          item.type === 'property' &&
          item.access.length === 1 &&
          item.access[0] === 'get';
      }));
    });

    /*
    it('should retrieve nested types', function () {
      assert.fail('Not implemented yet');
    });
     */

    /*
    it('should merge property accesses', function () {
      assert.fail('Not implemented yet');
    });
     */
  });

  describe('#invokeMethod()', function () {
    it('should work with static method', function () {
      var r = clr.invokeMethod('System.String', 'Concat', null, ['Hello, ', 'world!']);

      assert.strictEqual(r, 'Hello, world!');
    });
    
    it('should work with instance method', function () {
      var ctor = clr.createConstructor('System.DateTime', function () { });
      var d = new ctor(1970, 1, 1);
      var r = clr.invokeMethod('System.DateTime', 'ToString', d, []);

      assert.strictEqual('1970/01/01 0:00:00', r);
    });

    it('should work as property getter', function () {
      var d = clr.invokeMethod('System.DateTime', 'get_Now', null, []);
      var hour = clr.invokeMethod('System.DateTime', 'get_Hour', d, []);

      assert.strictEqual(new Date().getHours(), hour);
    });

    it('should work as property setter', function () {
      var ctor = clr.createConstructor('System.Exception', function () { });
      var ex = new ctor();
      clr.invokeMethod('System.Exception', 'set_Source', ex, ['Hello']);
      var r = clr.invokeMethod('System.Exception', 'get_Source', ex, []);

      assert.strictEqual('Hello', r);
    });

    // TODO: events
    // TODO: binding - function, array, varargs, etc...
  });
});
