var util = require('util');
var _ = require('underscore');
var clr = require('bindings')('clr.node');

function parseType(name) {
  var m = /^((?:\w+\.)*)(.*), ([^,]+, [^,]+, [^,]+, [^,]+)$/.exec(name);
  if (m) {
    return {
      assemblyQualifiedName: m[0],
      fullName: m[1] + m[2],
      namespace: m[1].slice(0, -1),
      name: m[2],
      keyword: m[2].split('`')[0],
      assembly: m[3]
    };
  } else {
    throw Error('cannot parse string as type name');
  }
}

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

  // define namespace factory
  var types = clr.getTypes();
  var namespaces = _.chain(types)
    .map(function (type) {
      return parseType(type);
    })
    .groupBy(function (type) {
      return type.namespace;
    })
    .value();

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
      .sortBy(_.identity)
      .unique(true)
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
      .sortBy(function (type) {
        return type.name;
      })
      .sortBy(function (type) {
        return type.keyword;
      })
      .unique(true, function (type) {
        return type.keyword;
      })
      .each(function (type) {
        Object.defineProperty(ns, type.keyword, {
          configurable: true,
          enumerable: true,
          get: function () {
            return constructorFactory(type.assemblyQualifiedName);
          }
        });
      });

    return ns;
  });

  // define constructor factory
  var constructorFactory = _.memoize(function (type) {
    var ctor = clr.createConstructor(type, function () { imbue(this); });
    
    Object.defineProperty(ctor, 'of', {
      configurable: true,
      enumerable: false,
      value: function () {
        var name = parseType(type);
        var args = _.map(arguments, function (item) {
          return (clr.isCLRConstructor(item))
            ? clr.typeOf(item)
            : item;
        });
        return constructorFactory(
          util.format(
            '%s`%d[%s], %s',
            name.fullName.split('`')[0],
            args.length,
            args.map(function (arg) {
              return util.format('[%s]', arg);
            }).join(','),
            name.assembly));
      }
    });

    // define static members
    _.chain(clr.getMembers(type, null))
      .sortBy(function (member) { return member.name; })
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
              clr.invokeMethod(type, 'add_' + member.name, null, [makeCallback(callback)]);
            },
            remove: function (callback) {
              // TODO: does not work
              clr.invokeMethod(type, 'remove_' + member.name, null, [makeCallback(callback)]);
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
          if (!member.indexed) {
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
          } else if (member.name == 'Item') {
            if (_.contains(member.access, 'get')) {
              ctor.get = function () {
                return imbue(clr.invokeMethod(type, 'get_' + member.name, null, _.toArray(arguments)));
              };
            }
            if (_.contains(member.access, 'set')) {
              ctor.set = function () {
                clr.invokeMethod(type, 'set_', member.name, null, _.toArray(arguments));
              };
            }
          } else {
            ctor[member.name] = {};
            if (_.contains(member.access, 'get')) {
              ctor[member.name].get = function () {
                return imbue(clr.invokeMethod(type, 'get_' + member.name, null, _.toArray(arguments)));
              };
            }
            if (_.contains(member.access, 'set')) {
              ctor[member.name].set = function () {
                return clr.invokeMethod(type, 'set_' + member.name, null, _.toArray(arguments));
              };
            }
          }
        } else if (member.type === 'method') {
          ctor[member.name] = function () {
            return imbue(clr.invokeMethod(type, member.name, null, _.toArray(arguments)));
          };
        }
      });

    // define instance members
    _.chain(clr.getMembers(type, true))
      .sortBy(function (member) { return member.name; })
      .each(function (member) {
        if (member.type === 'event') {
          Object.defineProperty(ctor.prototype, member.name, {
            configurable: true,
            enumerable: true,
            get: function () {
              var self = this;
              return {
                add: function (callback) {
                  clr.invokeMethod(type, 'add_' + member.name, self, [makeCallback(callback)]);
                },
                remove: function (callback) {
                  // TODO: does not work
                  clr.invokeMethod(type, 'remove_' + member.name, self, [makeCallback(callback)]);
                }
              };
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
          if (!member.indexed) {
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
          } else if (member.name == 'Item') {
            if (_.contains(member.access, 'get')) {
              ctor.prototype.get = function () {
                return imbue(clr.invokeMethod(type, 'get_' + member.name, this, _.toArray(arguments)));
              };
            }
            if (_.contains(member.access, 'set')) {
              ctor.prototype.set = function () {
                clr.invokeMethod(type, 'set_' + member.name, this, _.toArray(arguments));
              };
            }
          } else {
            Object.defineProperty(ctor.prototype, member.name, {
              configurable: true,
              enumerable: true,
              get: function () {
                var self = this;
                var obj = {};
                if (_.contains(member.access, 'get')) {
                  obj.get = function () {
                    return imbue(clr.invokeMethod(type, 'get_' + member.name, self, _.toArray(arguments)));
                  };
                }
                if (_.contains(member.access, 'set')) {
                  obj.set = function () {
                    return clr.invokeMethod(type, 'set_' + member.name, self, _.toArray(arguments));
                  };
                }
              }
            });
          }
        } else if (member.type === 'method') {
          ctor.prototype[member.name] = function () {
            return imbue(clr.invokeMethod(type, member.name, this, _.toArray(arguments)));
          };
        }
      });

    Object.defineProperty(ctor.prototype, 'toString', {
      configurable: true,
      enumerable: false,
      value: function () {
        return '[CLRObject: ' + parseType(clr.getType(this)).fullName + ']';
      }
    });
    // custom inspector function
    // TODO: colorize?
    Object.defineProperty(ctor.prototype, 'inspect', {
      configurable: true,
      enumerable: false,
      value: function (recurseTimes) {
        var str = util.inspect(this, { customInspect: false, depth: recurseTimes });
        var base = this.toString();
        if (str === '{}') {
          return base;
        } else {
          return '{ ' + base + '\n ' + str.substr(1);
        }
      }
    });

    return ctor;
  });

  // set __proto__ to plain CLR wrapped object
  function imbue(obj) {
    if (clr.isCLRObject(obj)) {
      var ctor = constructorFactory(clr.getType(obj));
      obj.__proto__ = ctor.prototype;
    }
    return obj;
  }

  function makeCallback(fn) {
    return function () {
      fn.apply(null, _.map(arguments, function (v) { return imbue(v); }));
    };
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
