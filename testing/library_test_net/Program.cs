using System;

namespace FerrGraphNet
{
    class Program
    {
        static void Example1()
        {
            var graph = new Graph<object, object, object>("NewGraph");
            graph.data.Add("desc", "Key value pairs can even contain\nnew lines and \"quotes\" without any problems!");

            graph.AddNode("Start");
            graph.AddNode("Middle");
            graph.AddNode("End")
                .data.Add("cost", "100");

            graph.AddEdge("Start", "Middle");
            graph.AddEdge("Start", "End")
                .data.Add("distance", "5.1");

            Console.WriteLine(graph.Save());
        }
        static void Example2()
        {
            var graph = new Graph<object, object, object>("MakeThing");
            graph.data.Add("Data", "This graph was procedurally generated");

            string[] words  = new string[] { "Value", "1.337", "Use a microphone!", "\n1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1", "Here's some \"text\" with some quotes in it, ouch!", "And a single \" quote in the middle."  };
            Random   r      = new Random();
            int      totalN = 0;
            for (int iter = 0; iter < 3; iter++)
            {
                int count = r.Next(4,10);
                for (int i = 0; i < count; i++)
                {
                    var node = graph.AddNode("Node"+totalN);
                    while (r.Next(2)==0)
                        node.data.Add("Key", words[r.Next(words.Length)]);
                    totalN += 1;
                }

                count = r.Next(2,6);
                for (int i = 0; i < count; i++)
                {
                    int start = r.Next(0, graph.nodes.Count);
                    int end   = start;
                    while (end == start) end = r.Next(0, graph.nodes.Count);
                    var edge = graph.AddEdge(start, end);
                    while (r.Next(2) == 0)
                        edge.data.Add("Key", words[r.Next(words.Length)]);
                }
            }

            Console.WriteLine(graph.Save());
        }

        static void Main()
        {
            var lib = GraphLibrary<object, object, object, object>.LoadFile("../../../../../demo.fgn");

            Console.WriteLine(lib.Save());

            Example1();
            Example2();
        }
    }
}
