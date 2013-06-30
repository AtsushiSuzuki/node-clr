module.exports = {
  'System.String.Format': function (test) {
    var System = require('../lib/clr')().System;
    
    test.strictEqual('Hello, world!', System.String.Format('Hello, {0}!', 'world'));
    test.done();
  }
};