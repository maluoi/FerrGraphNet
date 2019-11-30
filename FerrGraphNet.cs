
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace FerrGraphNet
{
    public class GraphLibrary<T,G,N,E>
    {
        enum Active {
            None,
            Graph,
            Node,
            Edge
        }

        Dictionary<string, Graph<G,N,E>> graphs = new Dictionary<string, Graph<G,N,E>>();
        public GraphData<T> data = new GraphData<T>();

        public GraphLibrary() 
        {
        }
        public Graph<G,N,E> Add(string graphId)
        {
            Graph<G,N,E> result = new Graph<G,N,E>(graphId);
            graphs[graphId] = result;
            return result;
        }
        public Graph<G,N,E> Get(string graphId) => graphs[graphId];
        public void Delete(string graphId) => graphs.Remove(graphId);
        

        public string Save()
        {
            StringBuilder result = new StringBuilder();
            data.Save(result, 0);
            result.AppendLine();

            foreach(var graph in graphs.Values)
                graph.Save(result);
            return result.ToString();
        }
        public void SaveFile(string fileName)
        {
            StreamWriter writer =  new StreamWriter(fileName);
            writer.Write(Save());
            writer.Close();
        }

        public static GraphLibrary<T,G,N,E> Load(string fileData)
        {
            GraphLibrary<T,G,N,E> library = new GraphLibrary<T, G, N, E>();
            Active active = Active.None;

            Graph<G,N,E> graph = null;
            int    nodeId  = -1;
            int    edgeId  = -1;
            int    currPos = 0;
            string line    = GraphString.NextLine(fileData, ref currPos)?.Trim();
            while (line != null)
            {
                if (line.Length == 0)
                    continue;

                if (line[0] == '-')
                {
                    if (line.Length < 3 || (line[2] != ' ' && line[2] != '\t'))
                        continue;

                    string args = line.Substring(3);

                    switch (line[1])
                    {
                        case 'g':
                            library.Add(args);
                            graph  = library.Get(args);
                            active = Active.Graph;
                            break;
                        case 'n':
                            nodeId = graph.AddNodeId(args);
                            active = Active.Node;
                            break;
                        case 'e':
                            string[] nodes = args.Split(',');
                            edgeId = graph.AddEdgeId(
                                nodes[0].Trim(), 
                                nodes[1].Trim());
                            active = Active.Edge;
                            break;
                    }
                }
                else if (line[0] == '#') { }
                else
                {
                    int    sep = line.IndexOf(' ');
                    string key = line.Substring(0, sep);
                    string val = line.Substring(sep+1);

                    val = GraphString.Unescape(val);
                    switch(active)
                    {
                        case Active.Graph: graph  .data [key]              = val; break;
                        case Active.Node:  graph  .nodes[nodeId].data[key] = val; break;
                        case Active.Edge:  graph  .edges[edgeId].data[key] = val; break;
                        case Active.None:  library.data [key]              = val; break;
                    }
                }

                line = GraphString.NextLine(fileData, ref currPos)?.Trim();
            }

            return library;
        }
        public static GraphLibrary<T,G,N,E> LoadFile(string fileName)
        {
            StreamReader reader = new StreamReader(fileName);
            string data = reader.ReadToEnd();
            reader.Close();
            return Load(data);
        }

        public override string ToString()
        {
            return string.Format("Library - {0} graphs", graphs.Count);
        }
    }

    public class Graph<T,N,E>
    {
        internal List<Node<N>> nodes = new List<Node<N>>();
        internal List<Edge<E>> edges = new List<Edge<E>>();
        public   GraphData<T>  data  = new GraphData<T>();

        public string Id { get; private set;}

        public Graph(string id)
        {
            Id = id;
        }

        public Node<N> AddNode(string id)
            => nodes[AddNodeId(id)];
        public int AddNodeId(string id)
        {
            nodes.Add(new Node<N>(id));
            return nodes.Count - 1;
        }
        
        public int FindNode(string id)
            => nodes.FindIndex(n=>n.Id == id);

        public Edge<E> AddEdge(Node<N> start, Node<N> end)
            => edges[AddEdgeId(FindNode(start.Id), FindNode(end.Id))];
        public Edge<E> AddEdge(string startId, string endId)
            => edges[AddEdgeId(FindNode(startId), FindNode(endId))];
        public Edge<E> AddEdge(int start, int end)
            => edges[AddEdgeId(start, end)];
        public int AddEdgeId(string startId, string endId)
            => AddEdgeId(FindNode(startId), FindNode(endId));
        public int AddEdgeId(int start, int end)
        {
            edges.Add(new Edge<E>(start, end));
            nodes[start].outEdges.Add(edges.Count - 1);
            nodes[end].inEdges.Add(edges.Count - 1);
            return edges.Count - 1;
        }

        public string Save()
        {
            StringBuilder result = new StringBuilder();
            Save(result);
            return result.ToString();
        }
        internal void Save(StringBuilder result)
        {
            result.Append("-g ");
            result.Append(Id);
            result.AppendLine();

            data.Save(result, 1);
            result.AppendLine();

            foreach(var n in nodes) n.Save(result);
            foreach(var e in edges) e.Save(result, (id)=>nodes[id].Id);
        }
        public override string ToString()
        {
            return string.Format("Graph {0} - N:{1} E:{2}", Id, nodes.Count, edges.Count);
        }
    }

    public class Node<T>
    {
        internal List<int>    inEdges;
        internal List<int>    outEdges;
        public   GraphData<T> data;

        public float x,y,z;

        public string Id { get; private set; }

        public Node(string id) {
            inEdges  = new List<int>();
            outEdges = new List<int>();
            data     = new GraphData<T>();
            x=y=z=0;
            Id = id;
        }

        internal void Save(StringBuilder result)
        {
            result.Append("-n ");
            result.Append(Id);
            result.AppendLine();
            data.Save(result, 1);
        }
        public override string ToString()
        {
            return string.Format("Node {0}", Id);
        }
    }

    public class Edge<T>
    {
        int                 start;
        int                 end;
        public GraphData<T> data;

        public Edge(int start, int end)
        {
            this.start = start;
            this.end   = end;
            this.data  = new GraphData<T>();
        }

        internal void Save(StringBuilder result, Func<int, string> idToName)
        {
            result.Append("-e ");
            result.Append(idToName(start));
            result.Append(", ");
            result.Append(idToName(end));
            result.AppendLine();
            data.Save(result, 1);
        }
        public override string ToString()
        {
            return string.Format("Edge - {0} -> {1}", start, end);
        }
    }

    public class GraphData<T>
    {
        Dictionary<string, string> pairs = new Dictionary<string, string>();
        public T parsed;

        public string this[string key] {
            get { return pairs[key]; }
            set { pairs[key] = value; }
        }

        public GraphData<T> Add(string key, string val)
        {
            pairs[key] = val;
            return this;
        }

        internal void Save(StringBuilder result, int tabCount)
        {
            string tabs = "";
            for (int t = 0; t < tabCount; t++) tabs += '\t';

            foreach(var pair in pairs)
            {
                result.Append(tabs);
                result.Append(pair.Key);
                result.Append(' ');
                if (pair.Value.Contains('\n'))
                {
                    result.Append('"');
                    result.Append(GraphString.Escape(pair.Value));
                    result.Append('"');
                }
                else result.Append(GraphString.Escape(pair.Value));
                result.AppendLine();
            }
        }
        public override string ToString()
        {
            return string.Format("Data keys - {0}", string.Join(',', pairs.Keys));
        }
    }

    static class GraphString
    {
        public static string Escape(string text)
        {
            string result = "";
            for (int i = 0; i < text.Length; i++)
            {
                if (text[i] == '"')
                    result += "\\'";
                else if (text[i] == '\\')
                    result += "\\\\";
                else
                    result += text[i];
            }
            return result;
        }
        public static string Unescape(string text)
        {
            string result = "";
            text = text.Replace("\"", "");
            for (int i = 0; i < text.Length; i++)
            {
                if (text[i] == '\\' && i + 1 < text.Length)
                {
                    if (text[i + 1] == '\'')
                        result += '"';
                    else
                        result += text[i + 1];
                    i += 1;
                }
                else
                    result += text[i];
            }
            return result;
        }
        public static string NextLine(string data, ref int curr)
        {
            int start = curr;
            bool q = false;
            while (curr < data.Length && (q || (data[curr] != '\n' && data[curr] != '\r')))
            {
                if (data[curr] == '"') q = !q;
                curr++;
            }
            int end = curr;
            while (curr < data.Length && (data[curr] == '\n' || data[curr] == '\r'))
                curr += 1;
            return start == end ? null : data[start..end];
        }
    }
}