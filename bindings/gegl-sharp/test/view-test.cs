using Gtk;
using System;
using Gegl;

public class ViewTest {
    public static void Main (string [] args)
    {
        Gegl.Global.Init();

        Node gegl = Gegl.Global.ParseXml(
            @"<gegl>
               <over>
                 <gaussian-blur std_dev_y='0' name='blur'/>
                 <shift x='20' y='170' name='shift'/>
                 <text string='gegl-sharp' size='120' color='rgb(0.5,0.5,1.0)'/>
               </over>
               <FractalExplorer xmin='0.2' ymin='0' xmax='0.5' ymax='0.45'
                                width='400' height='400'/>
            </gegl>", ""
        );

        Gtk.Application.Init ();
        Gtk.Window window = new Gtk.Window("Gegl# View Test");
        window.DeleteEvent += OnDelete;

        Gegl.View view = new Gegl.View();
        view.Node = gegl;
        window.SetDefaultSize(400, 400);

        window.Add(view);
        window.ShowAll();

        GLib.Timeout.Add(100, delegate {
            view.Scale *= 1.01;
            view.X += 2;
            view.Y += 2;

            return true;
        });

        Gtk.Application.Run();

        Console.WriteLine("Finished");
    }

    static void OnDelete (object o, DeleteEventArgs e)
    {
        Gtk.Application.Quit();
        Gegl.Global.Exit();
    }   
}
