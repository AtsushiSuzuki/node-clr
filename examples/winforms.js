require('../lib/clr').init({ assemblies: [ 'System.Windows.Forms' ] });
with (System.Windows.Forms) {
  var f = new Form();
  
  var p = new FlowLayoutPanel();
  f.Controls.Add(p);
  
  var t = new TextBox();
  t.Text = 'world';
  p.Controls.Add(t);
  
  var b = new Button();
  b.Text = 'Greet';
  b.Click.add(function (thiz, ea) {
    console.log('clicked');
    MessageBox.Show('Hello, ' + t.Text + '!');
  });
  p.Controls.Add(b);

  Application.Run(f);
}