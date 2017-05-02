let clr_test = require("../build/Debug/clr_test.node");


describe("native tests", () => {
  for (let fn in clr_test) {
    it(fn, () => {
      clr_test[fn]();
    });
  }
});
