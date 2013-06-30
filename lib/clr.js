module.exports = function (assemblies) {
  var clr = require('../build/Release/clr');
  var _ = require('underscore');
  
  _.each(assemblies || [ 'mscorlib.dll', 'System.dll', 'System.Core.dll' ], function (item) {
    clr.import(item);
  });

  var types = clr.types();
  var type = _.memoize(function (name) {
    // TODO: map class inheritance to prototype chain
    var constructor = clr.constructor(name, function () { imbue(this); });

    _.each(clr.methods(name, null), function (item) {
      Object.defineProperty(constructor, item, {
        enumerable: true,
        value: function () {
          return imbue(clr.invoke(name, null, item, _.toArray(arguments)));
        }
      });
    });

    _.each(clr.properties(name, null), function (item) {
      Object.defineProperty(constructor, item, {
        enumerable: true,
        get: function () {
          return imbue(clr.get(name, null, item, []));
        },
        set: function (value) {
          clr.set(name, null, item, [value]);
        }
      });
    });

    _.each(clr.methods(name, true), function (item) {
      Object.defineProperty(constructor.prototype, item, {
        enumerable: true,
        value: function () {
          return imbue(clr.invoke(name, this, item, _.toArray(arguments)));
        }
      });
    });

    _.each(clr.properties(name, true), function (item) {
      Object.defineProperty(constructor.prototype, item, {
        enumerable: true,
        get: function () {
          return imbue(clr.get(name, this, item, []));
        },
        set: function (value) {
          clr.set(name, this, item, [value]);
        }
      });
    });

    return constructor;
  });

  var namespaces = _.groupBy(types, function (type) {
    return /^(.*)\.[^\.]*$/.exec(type)[1];
  });
  var namespace = _.memoize(function (name) {
    var ns = {};
    var prefix = (name !== '') ? name + '.' : name;

    _.chain(namespaces)
      .keys()
      .filter(function (item) {
        return (item.indexOf(prefix) === 0) && /^[^\.]+$/.test(item.slice(prefix.length));
      })
      .sortBy(function (item) { return item; })
      .each(function (item) {
        Object.defineProperty(ns, item.slice(prefix.length), {
          enumerable: true,
          get: function () {
            return namespace(item);
          }
        });
      });

    _.chain(namespaces[name])
      .sortBy(function (item) { return item; })
      .each(function (item) {
        Object.defineProperty(ns, item.slice(prefix.length), {
          enumerable: true,
          get: function () {
            return type(item);
          }
        });
      });

    return ns;
  });

  function imbue(obj) {
    var name = obj && obj.__clr_type__;
    if (name) {
      var c = type(name);
      obj.__proto__ = c.prototype;

      if (process.env.NODE_ENV !== 'production') {
        // make repl friendly
        _.each(Object.getOwnPropertyNames(c.prototype), function (prop) {
          Object.defineProperty(obj, prop, Object.getOwnPropertyDescriptor(c.prototype, prop));
        });
      }
    }

    return obj;
  };

  return namespace('');
};
