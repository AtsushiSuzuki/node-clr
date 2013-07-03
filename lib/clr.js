(function () {
  var _ = require('underscore');
  var clr = require('../build/Release/clr');

  /** module initializer */
  function init(options) {
    // options
    options = _.extend({
      assemblies: ['mscorlib', 'System', 'System.Core'],
      global: true,
      imbue: true
    }, options);

    // load assemblies
    _.each(options.assemblies, function (item) {
      clr.import(item);
    });
    
    // define constructor factory
    var types = clr.types();
    var constructorFactory = _.memoize(function (name) {
      // TODO: map class inheritance to prototype chain
      var constructor = clr.constructor(name, function () { imbue(this); });

      // define static methods
      _.each(clr.methods(name, null), function (item) {
        Object.defineProperty(constructor, item, {
          enumerable: true,
          value: function () {
            return imbue(clr.invoke(name, null, item, _.toArray(arguments)));
          }
        });
      });

      // define static properties
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

      // define instance methods
      _.each(clr.methods(name, true), function (item) {
        Object.defineProperty(constructor.prototype, item, {
          enumerable: true,
          value: function () {
            return imbue(clr.invoke(name, this, item, _.toArray(arguments)));
          }
        });
      });

      // define instance properties
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

    // define namespace factory
    var namespaces = _.groupBy(types, function (type) {
      return /^(.*)\.[^\.]*$/.exec(type.split(',')[0])[1];
    });
    var namespace = _.memoize(function (name) {
      var ns = {};
      var prefix = (name !== '') ? name + '.' : name;

      // define nested namespaces
      _.chain(namespaces)
        .keys()
        .filter(function (item) {
          return (item.indexOf(prefix) === 0) && (item !== prefix);
        })
        .map(function (item) {
          return prefix + item.slice(prefix.length).split('.')[0];
        })
        .sortBy()
        .uniq()
        .each(function (item) {
          Object.defineProperty(ns, item.slice(prefix.length), {
            enumerable: true,
            get: function () {
              return namespace(item);
            }
          });
        });

      // define types
      _.chain(namespaces[name])
        .sortBy(function (item) { return item; })
        .each(function (item) {
          Object.defineProperty(ns, item.split(',')[0].slice(prefix.length), {
            enumerable: true,
            get: function () {
              return constructorFactory(item);
            }
          });
        });

      return ns;
    });

    // imbue CLR wrapper
    function imbue(obj) {
      var name = obj && obj.__clr_type__;
      if (name) {
        var c = constructorFactory(name);
        obj.__proto__ = c.prototype;

        if (options.imbue) {
          // make repl friendly
          _.each(Object.getOwnPropertyNames(c.prototype), function (prop) {
            Object.defineProperty(obj, prop, Object.getOwnPropertyDescriptor(c.prototype, prop));
          });
        }
      }

      return obj;
    };

    // create global namespace
    var ns = namespace('');
    if (options.global) {
      _.extend(global, ns);
    }
    return ns;
  };

  module.exports = init;
  module.exports.init = init;
  _.extend(module.exports, clr);
})();