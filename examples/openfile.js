require("../lib/clr").init({assemblies: ["System.Windows.Forms"]});

with (System.Windows.Forms) {
  console.log("select file");

  var ofd = new OpenFileDialog();
  if (ofd.ShowDialog().Equals(DialogResult.OK)) {
    console.log(ofd.FileName);
  } else {
    console.log("cancelled");
  }
}
