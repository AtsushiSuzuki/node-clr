require('../lib/clr').init({ assemblies: [ 'System.Windows.Forms' ] });
with (System.Windows.Forms) {
  var f = new Form();

  var t = new TextBox();
  t.Text = 'world';
  f.Controls.Add(t);
  
  var b = new Button();
  b.Text = 'Greet';
  b.Click.add(function (thiz, ea) {
    console.log('clicked');
    MessageBox.Show('Hello, ' + t.Text + '!');
  });
  b.Top = 20;
  f.Controls.Add(b);

  Application.Run(f);
}