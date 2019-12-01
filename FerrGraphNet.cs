
using System;
using System.Collections.Generic;
using System.IO;
using System.Text;

namespace FerrGraphNet
{
	public enum NodeLoc
	{
		In,
		Out,
		Any
	}

	interface IGraphLookup<N,E>
	{
		public Node<N,E> GetNode(int index);
		public Edge<E> GetEdge(int index);
	}

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

    public class Graph<T,N,E> : IGraphLookup<N,E>
    {
        internal List<Node<N,E>> nodes = new List<Node<N,E>>();
        internal List<Edge<E>>   edges = new List<Edge<E>>();
        public   GraphData<T>    data  = new GraphData<T>();

        public IEnumerable<Node<N,E>> Nodes { get => nodes; }
        public IEnumerable<Edge<E>>   Edges { get => edges; }
		public int NodeCount { get => nodes.Count; }
        public int EdgeCount { get => edges.Count; }
        public Node<N,E> GetNode(int index) => nodes[index];
        public Edge<E>   GetEdge(int index) => edges[index];

        public string Id { get; private set;}

        public Graph(string id)
        {
            Id = id;
        }

		public Node<N,E> AddNode(string id, N data = default(N))
            => nodes[AddNodeId(id, data)];
        public int AddNodeId(string id, N data = default(N))
        {
            nodes.Add(new Node<N,E>(this, id));
            nodes[nodes.Count-1].data.parsed = data;
            return nodes.Count - 1;
        }
        
        public int FindNode(string id)
            => nodes.FindIndex(n=>n.Id == id);

        public Edge<E> AddEdge(Node<N,E> start, Node<N,E> end, E data = default(E))
            => edges[AddEdgeId(FindNode(start.Id), FindNode(end.Id), data)];
        public Edge<E> AddEdge(string startId, string endId, E data = default(E))
            => edges[AddEdgeId(FindNode(startId), FindNode(endId), data)];
        public Edge<E> AddEdge(int start, int end, E data = default(E))
            => edges[AddEdgeId(start, end)];
        public int AddEdgeId(string startId, string endId, E data = default(E))
            => AddEdgeId(FindNode(startId), FindNode(endId), data);
        public int AddEdgeId(int start, int end, E data = default(E))
        {
            edges.Add(new Edge<E>(start, end));
            edges[edges.Count - 1].data.parsed = data;
            nodes[start].outEdges.Add(edges.Count - 1);
            nodes[end].inEdges.Add(edges.Count - 1);
            return edges.Count - 1;
        }


        /// <summary>Travels up the tree, and find all the roots (nodes with no inputs) attached
        /// to the provided nodes.</summary>
        /// <param name="ofNodeIds">Ids of the nodes you want to find the roots of.</param>
        /// <returns>A list of node ids containing all the roots (nodes with no inputs) that are 
        /// connected to the provided list of nodes.</returns>
        public List<int> FindRoots(List<int> ofNodeIds)
        {
            List<int> curr = new List<int>(ofNodeIds);
            List<int> result = new List<int>();
            while (curr.Count > 0)
            {
                int id = curr[curr.Count - 1];
                var n  = nodes[id];
                curr.RemoveAt(curr.Count-1);

                if (n.inEdges.Count == 0)
                    result.Add(id);
                else
                    for (int i = 0; i < n.inEdges.Count; i++) { 
						var e = edges[n.inEdges[i]];
                        if (!curr.Contains(e.Start))
                            curr.Add(e.Start);
					}
            }
            return result;
        }

        public List<int> FindConnected(List<int> toNodeIds)
        {
            List<int> search = new List<int>(toNodeIds);

            for (int index = 0; index < search.Count; index++)
            {
                var n = nodes[search[index]];
                for (int i = 0; i < n.inEdges.Count; i++) { 
					var e = edges[n.inEdges[i]];
                    if (!search.Contains(e.Start))
                        search.Add(e.Start);
				}
                for (int i = 0; i < n.outEdges.Count; i++) {
					var e = edges[n.outEdges[i]];
					if (!search.Contains(e.End))
                        search.Add(e.End);
				}
            }
            return search;
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

    public class Node<N, E>
    {
		#region Fields and Properties

		IGraphLookup<N,E> graphLookup;

        internal List<int>    inEdges;
        internal List<int>    outEdges;
        public   GraphData<N> data;

        public float x, y, z;

        public string Id { get; private set; }

		#endregion

        internal Node(IGraphLookup<N, E> graphLookup, string id) {
			this.graphLookup = graphLookup;
            inEdges  = new List<int>();
            outEdges = new List<int>();
            data     = new GraphData<N>();
            x=y=z=0;
            Id = id;
        }

		#region Enumeration/Iteration

		public IEnumerable<Edge<E>> Edges(NodeLoc loc) { switch(loc) { 
			case NodeLoc.In:  for (int i = 0; i < inEdges.Count;  i++) yield return graphLookup.GetEdge(inEdges[i]); break;
			case NodeLoc.Out: for (int i = 0; i < outEdges.Count; i++) yield return graphLookup.GetEdge(outEdges[i]); break;
			case NodeLoc.Any: 
					for (int i = 0; i < inEdges.Count;  i++) yield return graphLookup.GetEdge(inEdges[i]);
					for (int i = 0; i < outEdges.Count; i++) yield return graphLookup.GetEdge(outEdges[i]); break;
		} }
		public IEnumerable<Node<N,E>> Nodes(NodeLoc loc) { switch(loc) { 
			case NodeLoc.In:  for (int i = 0; i < inEdges.Count;  i++) yield return graphLookup.GetNode(graphLookup.GetEdge(inEdges [i]).Start); break;
			case NodeLoc.Out: for (int i = 0; i < outEdges.Count; i++) yield return graphLookup.GetNode(graphLookup.GetEdge(outEdges[i]).End  ); break;
			case NodeLoc.Any:
				for (int i = 0; i < inEdges.Count;  i++) yield return graphLookup.GetNode(graphLookup.GetEdge(inEdges [i]).Start);
				for (int i = 0; i < outEdges.Count; i++) yield return graphLookup.GetNode(graphLookup.GetEdge(outEdges[i]).End  ); break;
		} }
		public int       OutCount => outEdges.Count;
		public int       InCount  => inEdges.Count;
		public Node<N,E> OutNode (int index) => graphLookup.GetNode(graphLookup.GetEdge(outEdges[index]).End);
		public Node<N,E> InNode  (int index) => graphLookup.GetNode(graphLookup.GetEdge(inEdges[index]).Start);
		public Edge<E>   InEdge  (int index) => graphLookup.GetEdge(inEdges[index]);
		public Edge<E>   OutEdge (int index) => graphLookup.GetEdge(outEdges[index]);

		#endregion

		#region String and Saving

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
		
		#endregion
    }

    public class Edge<T>
    {
        int                 start;
        int                 end;
        public GraphData<T> data;

        public int Start { get=>start; }
        public int End { get=>end; }

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
        public IEnumerable<KeyValuePair<string, string>> Enumerable { get=>pairs; }

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