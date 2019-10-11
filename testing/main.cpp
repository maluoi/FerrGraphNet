
#define FERR_GRAPHNET_IMPLEMENT
#include "../ferr_graphnet.h"
#include <time.h>

struct node_data_t {
	float slider;
	float position[3];
	const char *text;
};

void example1() {
	fgn_library_t lib = {};
	fgn_load_file(lib, "out_proc.fgn");

	fgn_lib_each(lib, [](fgn_graph_t &graph) {
		printf("Graph - %s\n", graph.id);
		fgn_graph_node_each(graph, [](fgn_node_t &node) {
			printf("%s: [in:%d, out:%d, keys:%d]\n", node.id, node.in_ct, node.out_ct, node.data.pair_ct);
		});
	});

	fgn_destroy(lib);
}
void example2() {
	fgn_graph_t graph = {};
	fgn_graph_set_id(graph, "NewGraph");
	fgn_data_add(graph.data, "desc", "Key value pairs can even contain\nnew lines and \"quotes\" without any problems!");

	fgn_graph_idx n1 = fgn_graph_node_add(graph, "Start");
	fgn_graph_idx n2 = fgn_graph_node_add(graph, "Middle");
	fgn_graph_idx n3 = fgn_graph_node_add(graph, "End");
	fgn_data_add(fgn_graph_node_get(graph, n2).data, "cost", "100");

	fgn_graph_idx e1 = fgn_graph_edge_add(graph, n1, n2);
	fgn_graph_idx e2 = fgn_graph_edge_add(graph, "Start", "End");
	fgn_data_add(fgn_graph_edge_get(graph, e2).data, "distance", "5.1");

	char *text_graph = fgn_save(graph);
	printf("Output file:\n%s", text_graph);
	free(text_graph);

	fgn_destroy(graph);
}
void example3() {
	fgn_parser_t node_parser = {};
	fgn_parser_create<node_data_t>(node_parser);
	fgn_parser_add(node_parser, "slider",   offsetof(node_data_t, slider),   fgn_parse_float,  fgn_write_float);
	fgn_parser_add(node_parser, "position", offsetof(node_data_t, position), fgn_parse_float3, fgn_write_float3);
	fgn_parser_add(node_parser, "text",     offsetof(node_data_t, text),     fgn_parse_string, fgn_write_string);

	fgn_library_t lib = {};
	fgn_load_file(lib, "out_proc.fgn");
	fgn_parse(lib, &node_parser);

	fgn_lib_each(lib, [](fgn_graph_t &graph) {
		for (int i = 0, ct = fgn_graph_node_count(graph); i < ct; i += 1) {
			node_data_t &data = fgn_graph_node_data<node_data_t>(graph, i);
			printf("<%g, %g, %g> - slider: %f - text: %s\n", 
				data.position[0], data.position[1], data.position[2], data.slider, data.text);
		}
	});

	fgn_destroy(lib);
	fgn_destroy(node_parser);
}

int main() {

	example1();
	example2();
	example3();

	// Create a parser for the node_data_t struct
	fgn_parser_t node_parser = {};
	fgn_parser_create<node_data_t>(node_parser);
	fgn_parser_add(node_parser, "slider",   offsetof(node_data_t, slider),   fgn_parse_float,  fgn_write_float);
	fgn_parser_add(node_parser, "position", offsetof(node_data_t, position), fgn_parse_float3, fgn_write_float3);
	fgn_parser_add(node_parser, "text",     offsetof(node_data_t, text),     fgn_parse_string, fgn_write_string);

	// load from file!
	fgn_library_t lib = {};
	fgn_load_file(lib, "out_proc.fgn");
	fgn_parse(lib, &node_parser);

	// Read some parsed data from the graph
	fgn_graph_t  *test_graph = fgn_lib_find(lib, "MakeThing");
	if (test_graph != nullptr) {
		node_data_t *data = fgn_graph_node_data<node_data_t>(*test_graph, "Node2");
		if (data != nullptr)
			printf("Slider %f\n", data->slider);
	}
	
	// Procedurally create a graph
	fgn_graph_t graph = {};
	fgn_graph_set_id(graph, "MakeThing");
	fgn_data_add(graph.data, "Data", "This graph was procedurally generated");
	fgn_data_add(graph.data, "Data", "Here's a multi-line\ntext added procedurally!");

	srand(time(nullptr));

	const char *words[] = {"Value", "1.337", "Use a microphone!", "\n1 0 0 0\n0 1 0 0\n0 0 1 0\n0 0 0 1", "Here's some \"text\" with some quotes in it, ouch!", "And a single \" quote in the middle."};
	int total_n = 0;
	for (size_t iter = 0; iter < 3; iter++) {
		// Add some nodes
		int ct = 4 + rand() % 6;
		for (int32_t i = 0; i < ct; i++) {
			char name[32];
			sprintf_s(name, "Node%d", total_n++);
			fgn_node_idx idx = fgn_graph_node_add(graph, name);
			while (rand() % 2 == 0) fgn_data_add(fgn_graph_node_get(graph, idx).data, "Key", words[rand()%_countof(words)]);
			if    (rand() % 8 == 0) fgn_graph_node_data<node_data_t>(graph, idx).text = "Multi-line \"da\nta\" in\na struct.";
		}

		// Add some edges
		int e_ct = 2 + rand() % 4;
		for (int32_t i = 0; i < e_ct; i++) {
			int start = rand() % graph.node_ct;
			int end   = rand() % graph.node_ct;
			while (end == start) end = rand() % ct;
			fgn_edge_idx id = fgn_graph_edge_add(graph, start, end);
			while (rand() % 2 == 0) fgn_data_add(fgn_graph_edge_get(graph, id).data, "Key", words[rand()%_countof(words)]);
			if    (rand() % 8 == 0) fgn_graph_edge_data<node_data_t>(graph, id).text = "Multi-line \"data\" in\na struct.";
		}

		// Randomly delete a few nodes and edges
		int del = rand() % ct * 0.2f;
		for (int32_t i = 0; i < del; i++) {
			fgn_graph_node_delete(graph, rand() % graph.node_ct);
		}
		del = rand() % e_ct * 0.2f;
		for (int32_t i = 0; i < del; i++) {
			fgn_graph_edge_delete(graph, rand() % graph.edge_ct);
		}
	}

	// Print out the graphs we've created
	printf("%s\n\n______________________________\n\n", fgn_save( lib, &node_parser, &node_parser ));
	printf("%s", fgn_save(graph, &node_parser, &node_parser));

	// And dump 'em to file
	fgn_save_file(graph, "out_proc.fgn", &node_parser, &node_parser);

	// 
	fgn_destroy(lib);
	fgn_destroy(graph);
	fgn_destroy(node_parser);
	return 0;
}