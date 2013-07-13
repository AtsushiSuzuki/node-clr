(function () {
  var util = require('util');
  var _ = require('underscore');
  var clr = require('../build/Release/clr');

  /** module initializer */
  function init(options) {
    // options
    options = _.extend({
      assemblies: ['mscorlib', 'System', 'System.Core'],
      global: true
    }, options);

    // load assemblies
    _.each(options.assemblies, function (item) {
      clr.import(item);
    });

    // define constructor factory
    var types = clr.getTypes();
    var constructorFactory = _.memoize(function (type) {
      var ctor = clr.createConstructor(type, function () { imbue(this); });

      // define static members
      _.chain(clr.getMembers(type, null))
        .sortBy(function (item) { return item.name; })
        .each(function (member) {
          if (member.type === 'nestedType') {
            Object.defineProperty(ctor, member.name, {
              configurable: true,
              enumerable: true,
              get: function () {
                return constructorFactory(member.fullName);
              }
            });
          } else if (member.type === 'event') {
            ctor[member.name] = {
              add: function (callback) {
                clr.invokeMethod(type, 'add_' + member.name, null, [callback]);
              },
              remove: function (callback) {
                // TODO: does not work
                clr.invokeMethod(type, 'remove_' + member.name, null, [callback]);
              }
            };
          } else if (member.type === 'field') {
            Object.defineProperty(ctor, member.name, {
              configurable: true,
              enumerable: true,
              get: (_.contains(member.access, 'get'))
                ? function () { return imbue(clr.getField(type, member.name, null)); }
                : undefined,
              set: (_.contains(member.access, 'set'))
                ? function (value) { clr.setField(type, member.name, null, value); }
                : undefined
            });
          } else if (member.type === 'property') {
            Object.defineProperty(ctor, member.name, {
              configurable: true,
              enumerable: true,
              get: (_.contains(member.access, 'get'))
                ? function () { return imbue(clr.invokeMethod(type, 'get_' + member.name, null, [])); }
                : undefined,
              set: (_.contains(member.access, 'set'))
                ? function (value) { clr.invokeMethod(type, 'set_' + member.name, null, [value]); }
                : undefined
            });
          } else if (member.type === 'method') {
            ctor[member.name] = function () {
              return imbue(clr.invokeMethod(type, member.name, null, _.toArray(arguments)));
            };
          }
        });

      // define instance members
      _.chain(clr.getMembers(type, true))
        .sortBy(function (item) { return item.name; })
        .each(function (member) {
          if (member.type === 'event') {
            Object.defineProperty(ctor.prototype, member.name, {
              configurable: true,
              enumerable: true,
              get: function () {
                var self = this;
                return {
                  add: function (callback) {
                    clr.invokeMethod(type, 'add_' + member.name, self, [callback]);
                  },
                  remove: function (callback) {
                    // TODO: does not work
                    clr.invokeMethod(type, 'remove_' + member.name, self, [callback]);
                  }
                }
              }
            });
          } else if (member.type === 'field') {
            Object.defineProperty(ctor.prototype, member.name, {
              configurable: true,
              enumerable: true,
              get: (_.contains(member.access, 'get'))
                ? function () { return imbue(clr.getField(type, member.name, this)); }
                : undefined,
              set: (_.contains(member.access, 'set'))
                ? function (value) { clr.setField(type, member.name, this, value); }
                : undefined
            });
          } else if (member.type === 'property') {
            Object.defineProperty(ctor.prototype, member.name, {
              configurable: true,
              enumerable: true,
              get: (_.contains(member.access, 'get'))
                ? function () { return imbue(clr.invokeMethod(type, 'get_' + member.name, this, [])); }
                : undefined,
              set: (_.contains(member.access, 'set'))
                ? function (value) { clr.invokeMethod(type, 'set_' + member.name, this, [value]); }
                : undefined
            });
          } else if (member.type === 'method') {
            ctor.prototype[member.name] = function () {
              return imbue(clr.invokeMethod(type, member.name, this, _.toArray(arguments)));
            };
          }
        });

      ctor.prototype.toString = function () {
        return '[CLRObject: ' + this._clrType.split(',')[0] + ']';
      }
      // custom inspector function
      // TODO: colorize?
      ctor.prototype.inspect = function (recurseTimes) {
        var str = util.inspect(this, { customInspect: false, depth: recurseTimes });
        var base = this.toString();
        if (str === '{}') {
          return base;
        } else {
          return '{ ' + base + '\n ' + str.substr(1);
        }
      };

      return ctor;
    });

    // define namespace factory
    var namespaces = _.groupBy(types, function (type) {
      return /^(.*)\.[^\.]*$/.exec(type.split(',')[0])[1];
    });
    var namespaceFactory = _.memoize(function (name) {
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
          configurable: true,
          enumerable: true,
          get: function () {
            return namespaceFactory(item);
          }
        });
      });

      // define types
      _.chain(namespaces[name])
        .sortBy()
        .each(function (item) {
          Object.defineProperty(ns, item.split(',')[0].slice(prefix.length), {
            configurable: true,
            enumerable: true,
            get: function () {
              return constructorFactory(item);
            }
          });
        });

      return ns;
    });

    function imbue(obj) {
      var type = obj && obj._clrType;
      if (type) {
        var ctor = constructorFactory(type);
        obj.__proto__ = ctor.prototype;
      }

      return obj;
    }

    // create global namespace
    var ns = namespaceFactory('');
    if (options.global) {
      _.extend(global, ns);
    }
    return ns;
  }

  module.exports = init;
  module.exports.init = init;
  _.extend(module.exports, clr);
})();
