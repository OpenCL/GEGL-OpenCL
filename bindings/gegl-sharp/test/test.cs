using System;
using Gegl;

public class Test {
    public static void Main (string [] args)
    {
        Global.Init();

        Node graph = new Node();

        Node load       = graph.CreateChild("load");
        load.SetProperty("path", "../../../docs/gallery/data/car-stack.jpg");

        Node scale      = graph.CreateChild("scale");
        scale.SetProperty("x", 0.50);
        scale.SetProperty("y", 0.50);

        Node rotate      = graph.CreateChild("rotate");
        rotate.SetProperty("degrees", 45.0);

        Node brightness_contrast = graph.CreateChild("brightness-contrast");
        brightness_contrast.SetProperty("contrast", 4.0);
        brightness_contrast.SetProperty("brightness", -2.0);

        Node save       = graph.CreateChild("png-save");
        save.SetProperty("path", "sample-out.png");

        load.Link(scale);
        scale.Link(rotate);
        rotate.Link(brightness_contrast);
        brightness_contrast.Link(save);

        Console.WriteLine("XML: {0}", Gegl.Global.ToXml(graph, "./"));

        save.Process();

        //Node from_xml = Global.ParseXml("", "/");

        Global.Exit();
        Console.WriteLine("Finished");
    }
}
