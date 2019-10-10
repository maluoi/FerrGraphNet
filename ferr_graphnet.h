
#ifndef FERR_GRAPH_NET_H
#define FERR_GRAPH_NET_H

#include <stdint.h>

///////////////////////////////////////////

typedef int32_t  fgn_graph_idx;
typedef int32_t  fgn_node_idx;
typedef int32_t  fgn_edge_idx;
typedef uint64_t fgn_hash_t;

// Core graph data types
struct fgn_library_t;
struct fgn_graph_t;
struct fgn_node_t;
struct fgn_edge_t;

// User-defined data storage and key/value pairs
struct fgn_data_t;
struct _fgn_pair_t;

// Parsing info for turning key/value pairs into structs
struct fgn_parser_t;
struct fgn_parse_state_t;
struct _fgn_parse_item_t;

///////////////////////////////////////////
/// Saving, Loading & Destroying        ///
///////////////////////////////////////////

struct fgn_data_t {
	_fgn_pair_t *pairs;
	int32_t      pair_ct;
	int32_t      pair_cap;
	void        *data;
};
struct _fgn_pair_t {
	fgn_hash_t key_hash;
	char      *key;
	char      *value;
};

struct fgn_library_t {
	fgn_graph_t *graphs;
	int32_t      graph_ct;
	int32_t      graph_cap;
	fgn_data_t   data;
};

struct fgn_graph_t {
	char       *id;
	fgn_hash_t  id_hash;

	fgn_node_t *nodes;
	int32_t     node_ct;
	int32_t     node_cap;
	fgn_edge_t *edges;
	int32_t     edge_ct;
	int32_t     edge_cap;
	fgn_data_t  data;
};

int32_t fgn_load     (fgn_library_t &lib, const char *filedata);
int32_t fgn_load_file(fgn_library_t &lib, const char *filename);
char   *fgn_save     (fgn_library_t &lib, const fgn_parser_t *parser_node = nullptr, const fgn_parser_t *parser_edge = nullptr, const fgn_parser_t *parser_graph = nullptr);
char   *fgn_save     (fgn_graph_t &graph, const fgn_parser_t *parser_node = nullptr, const fgn_parser_t *parser_edge = nullptr, const fgn_parser_t *parser_graph = nullptr);
int32_t fgn_save_file(fgn_library_t &lib, const char *filename, const fgn_parser_t *parser_node = nullptr, const fgn_parser_t *parser_edge = nullptr, const fgn_parser_t *parser_graph = nullptr);
int32_t fgn_save_file(fgn_graph_t &graph, const char *filename, const fgn_parser_t *parser_node = nullptr, const fgn_parser_t *parser_edge = nullptr, const fgn_parser_t *parser_graph = nullptr);
void    fgn_destroy  (fgn_library_t &lib);
void    fgn_destroy  (fgn_graph_t &graph);

///////////////////////////////////////////
/// Library functions                   ///
///////////////////////////////////////////

fgn_graph_idx       fgn_lib_add   (      fgn_library_t &lib, const char   *graph_id);
fgn_graph_idx       fgn_lib_findid(const fgn_library_t &lib, const char   *graph_id);
inline int32_t      fgn_lib_count (const fgn_library_t &lib)                          { return lib.graph_ct; }
inline fgn_graph_t &fgn_lib_get   (const fgn_library_t &lib, fgn_graph_idx graph_idx) { return lib.graphs[graph_idx]; }
inline fgn_graph_t *fgn_lib_find  (const fgn_library_t &lib, const char   *graph_id ) { fgn_graph_idx i = fgn_lib_findid(lib, graph_id); return i == -1 ? nullptr : &lib.graphs[i]; }

///////////////////////////////////////////
/// Graph manipulation, Nodes and Edges ///
///////////////////////////////////////////

struct fgn_node_t {
	char         *id;
	fgn_hash_t    id_hash;
	fgn_edge_idx *in_edges;
	int32_t       in_ct, in_cap;
	fgn_edge_idx *out_edges;
	int32_t       out_ct, out_cap;
	fgn_data_t    data;
};
struct fgn_edge_t {
	fgn_node_idx start;
	fgn_node_idx end;
	fgn_data_t   data;
};

void               fgn_graph_set_id     (fgn_graph_t &graph, const char *id);
fgn_node_idx       fgn_graph_node_add   (fgn_graph_t &graph, const char *id);
void               fgn_graph_node_setid (fgn_graph_t &graph, fgn_node_idx idx, const char *text_id);
fgn_node_idx       fgn_graph_node_findid(const fgn_graph_t &graph, const char *id);
void               fgn_graph_node_delete(fgn_graph_t &graph, fgn_node_idx node);
void               fgn_graph_node_delete(fgn_graph_t &graph, const char *node);
inline fgn_node_t *fgn_graph_node_find  (const fgn_graph_t &graph, const char *id)   { fgn_node_idx i = fgn_graph_node_findid(graph, id); return i == -1 ? nullptr : &graph.nodes[i]; }
inline int32_t     fgn_graph_node_count (const fgn_graph_t &graph)                   { return graph.node_ct; }
inline fgn_node_t &fgn_graph_node_get   (const fgn_graph_t &graph, fgn_node_idx idx) { return graph.nodes[idx]; }

fgn_edge_idx       fgn_graph_edge_add   (fgn_graph_t &graph, fgn_node_idx start, fgn_node_idx end);
fgn_edge_idx       fgn_graph_edge_add   (fgn_graph_t &graph, const char *start, const char *end);
void               fgn_graph_edge_delete(fgn_graph_t &graph, fgn_edge_idx edge);
inline int32_t     fgn_graph_edge_count (const fgn_graph_t &graph)                   { return graph.edge_ct; }
inline fgn_edge_t &fgn_graph_edge_get   (const fgn_graph_t &graph, fgn_edge_idx idx) { return graph.edges[idx]; }

///////////////////////////////////////////

template<typename T> T &fgn_graph_node_data(const fgn_graph_t &graph, fgn_node_idx idx);
template<typename T> T *fgn_graph_node_data(const fgn_graph_t &graph, const char  *id);
template<typename T> T &fgn_graph_edge_data(const fgn_graph_t &graph, fgn_edge_idx idx);

void                    fgn_data_add    (fgn_data_t &data, const char *key, const char *value);
void                    fgn_data_destroy(fgn_data_t &data);
template<typename T> T &fgn_data_get    (const fgn_data_t &data);

///////////////////////////////////////////
/// Custom data storage and parsing     ///
///////////////////////////////////////////

struct fgn_parser_t {
	size_t             type_size;
	_fgn_parse_item_t *items;
	int32_t            item_ct;
	int32_t            item_cap;
};
struct fgn_parse_state_t {
	fgn_graph_t &graph;
	fgn_node_idx curr_node;
	fgn_edge_idx curr_edge;
};
struct _fgn_parse_item_t {
	const char *key;
	fgn_hash_t  key_hash;
	int32_t     offset;
	bool  (*parse)(fgn_parse_state_t state, const char *value_text, void *out_data);
	char *(*write)(fgn_parse_state_t state, void *value);
};

template<typename T> inline void fgn_parser_create(fgn_parser_t &parser) { parser.type_size = sizeof(T); }
void fgn_parser_add(fgn_parser_t &parser, char *name, int32_t offset,
	bool  (*parse)(fgn_parse_state_t state, const char *value_text, void *out_data),
	char *(*write)(fgn_parse_state_t state, void *value));
void fgn_parse(fgn_graph_t &graph, const fgn_parser_t *parser_node = nullptr, const fgn_parser_t *parser_edge = nullptr, const fgn_parser_t *parser_graph = nullptr);
void fgn_parse(fgn_library_t &lib, const fgn_parser_t *parser_node = nullptr, const fgn_parser_t *parser_edge = nullptr, const fgn_parser_t *parser_graph = nullptr);

bool  fgn_parse_float (fgn_parse_state_t state, const char *value_text, void *out_data);
char *fgn_write_float (fgn_parse_state_t state, void *value);
bool  fgn_parse_float2(fgn_parse_state_t state, const char *value_text, void *out_data);
char *fgn_write_float2(fgn_parse_state_t state, void *value);
bool  fgn_parse_int32 (fgn_parse_state_t state, const char *value_text, void *out_data);
char *fgn_write_int32 (fgn_parse_state_t state, void *value);
bool  fgn_parse_string(fgn_parse_state_t state, const char *value_text, void *out_data);
char *fgn_write_string(fgn_parse_state_t state, void *value);
bool  fgn_parse_nodeid(fgn_parse_state_t state, const char *value_text, void *out_data);
char *fgn_write_nodeid(fgn_parse_state_t state, void *value);

/* /////////////////////////////////////////// **
** //////////// Implementation! ////////////// **
** /////////////////////////////////////////// */

#ifdef FERR_GRAPH_NET_IMPLEMENTATION

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

///////////////////////////////////////////
/// Private helper functions            ///
///////////////////////////////////////////

// String utilities
bool        _fgn_str_eq      (const char *a, const char *b);
bool        _fgn_str_starts  (const char *a, const char *b);
fgn_hash_t  _fgn_str_hash    (const char *string);
char       *_fgn_str_copy    (const char *string);
void        _fgn_str_append  (char **string, int32_t &count, int32_t &cap, const char *text, ...);
char       *_fgn_str_make    (const char *text, ...);
char       *_fgn_str_escape  (const char *str);
void        _fgn_str_unescape(char *str);

// File parsing
const char *_fgn_str_trim     (const char *str);
const char *_fgn_str_next_line(const char *str);
const char *_fgn_str_next_word(const char *str, char sep);
char       *_fgn_str_copy_line(const char *str);
char       *_fgn_str_copy_word(const char *str, char sep);
bool        _fgn_str_line_has (const char *str, char sep);
bool        _fgn_str_contains (const char *str, char sep);

// Array modification
template<typename T> int32_t _fgn_arr_add(T **arr, int32_t quantity, int32_t &count, int32_t &capacity);
template<typename T> void    _fgn_arr_remove(T **arr, int32_t index, int32_t &count);

///////////////////////////////////////////

int32_t fgn_load     (fgn_library_t &lib, const char *filedata) {
	enum active_ {
		active_none,
		active_graph,
		active_node,
		active_edge,
	};

	int32_t result = 0;
	const char *curr = filedata;
	curr = _fgn_str_trim(curr);

	// Read data now
	active_     active     = active_none;
	fgn_graph_t *curr_graph = nullptr;
	fgn_node_t  *curr_node  = nullptr;
	fgn_edge_t  *curr_edge  = nullptr;
	while (*curr != '\0') {
		if (*curr == '-') {
			// Parse the first 3 characters and make sure they're right
			curr++;
			char type = *curr;
			curr++;
			if (*curr != ' ' || *curr != '\t')
				result = 1;
			curr++;

			// Check what type this line is
			if        (type == 'g') {
				active = active_graph;

				char *id = _fgn_str_copy_line(curr);
				fgn_graph_idx graph_idx =  fgn_lib_add(lib, id);
				curr_graph              = &fgn_lib_get(lib, graph_idx);

				free(id);
			} else if (type == 'n') {
				active = active_node;

				char *id = nullptr, *type = nullptr;
				if (_fgn_str_line_has(curr, ':')) {
					id   = _fgn_str_copy_word(curr, ':');
					type = _fgn_str_copy_word(_fgn_str_next_word(curr, ':'),':');
				} else {
					id   = _fgn_str_copy_line(curr);
				}
				curr_node = &fgn_graph_node_get(*curr_graph, fgn_graph_node_add(*curr_graph, id));

				free(id);
				free(type);
			} else if (type == 'e') {
				active = active_edge;

				const char *end  = _fgn_str_next_word(curr, ',');
				char *start_copy = _fgn_str_copy_word(curr, ',');
				char *end_copy   = _fgn_str_copy_word(end,  ',');
				curr_edge = &fgn_graph_edge_get( *curr_graph, fgn_graph_edge_add(*curr_graph, 
					fgn_graph_node_findid(*curr_graph, start_copy), 
					fgn_graph_node_findid(*curr_graph, end_copy)));

				free(start_copy);
				free(end_copy);
			} else {
				result = 2;
			}
		} else if (*curr == '#') {
		} else {
			// Add a kvp to the active item
			char *key   = _fgn_str_copy_word(curr, ' ');
			char *value =  _fgn_str_copy_line(_fgn_str_next_word(curr, ' '));
			_fgn_str_unescape(value);
			switch (active) {
			case active_graph: {
				if (curr_graph != nullptr)
					fgn_data_add(curr_graph->data, key, value);
			}break;
			case active_node: {
				if (curr_node != nullptr)
					fgn_data_add(curr_node->data, key, value);
			}break;
			case active_edge: {
				if (curr_edge != nullptr)
					fgn_data_add(curr_edge->data, key, value);
			}break;
			default: {
				fgn_data_add(lib.data, key, value);
			}
			}
			free(key);
			free(value);
		}
		curr = _fgn_str_trim(_fgn_str_next_line(curr));
	}

	return result;
}
int32_t fgn_load_file(fgn_library_t &lib, const char *filename) {
	FILE *fp = nullptr;
	if (fopen_s(&fp, filename, "rb") != 0 && fp == nullptr)
		return 1;

	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *filedata = (char*)malloc(length+1);
	fread(filedata, length, 1, fp);
	filedata[length] = '\0';
	fclose(fp);

	int32_t result = fgn_load(lib, filedata);

	free(filedata);
	return result;
}
char   *fgn_save     (fgn_library_t &lib, const fgn_parser_t *parser_node, const fgn_parser_t *parser_edge, const fgn_parser_t *parser_graph) {
	char *result = nullptr;
	int32_t ct=0, cap=0;

	// Write the data pairs for the library
	for (int32_t i = 0; i < lib.data.pair_ct; i++) {
		char *text = _fgn_str_escape(lib.data.pairs[i].value);
		_fgn_str_append(&result, ct, cap, "%s %s\n", lib.data.pairs[i].key, text == nullptr ? lib.data.pairs[i].value : text);
		free(text);
	}
	_fgn_str_append(&result, ct, cap, "\n");

	// And append each of the graphs
	for (int32_t i = 0; i < lib.graph_ct; i++) {
		char *graph_text = fgn_save(lib.graphs[i], parser_node, parser_edge, parser_graph);
		_fgn_str_append(&result, ct, cap, graph_text);
		free(graph_text);
	}

	return result;
}
int32_t fgn_save_file(fgn_library_t &lib, const char *filename, const fgn_parser_t *parser_node, const fgn_parser_t *parser_edge, const fgn_parser_t *parser_graph) {
	FILE *fp = nullptr;
	if (fopen_s(&fp, filename, "wb") != 0 && fp == nullptr)
		return 1;

	char *data = fgn_save(lib, parser_node, parser_edge, parser_graph);
	fwrite(data, strlen(data), 1, fp);
	fclose(fp);
	free(data);

	return 0;
}
char   *fgn_save     (fgn_graph_t &graph, const fgn_parser_t *parser_node, const fgn_parser_t *parser_edge, const fgn_parser_t *parser_graph) {
	char *result = nullptr;
	int32_t ct=0, cap=0;
	fgn_parse_state_t state = {graph, -1, -1};
	
	auto write_data = [&result, &ct, &cap, state](fgn_data_t &data, const fgn_parser_t *parser) {
		// Write the data we parsed into a struct earlier
		if (parser != nullptr && data.data != nullptr) {
			for (size_t i = 0; i < parser->item_ct; i++) {
				if (parser->items[i].write == nullptr)
					continue;
				char *value = parser->items[i].write(state, ((uint8_t *)data.data) + parser->items[i].offset);
				if (value != nullptr) {
					char *text = _fgn_str_escape(value);
					_fgn_str_append(&result, ct, cap, "\t%s %s\n", parser->items[i].key, text == nullptr ? value : text);
					free(text);
				}
				free(value);
			}
		}
		// Write any key value pairs
		for (int32_t i = 0; i < data.pair_ct; i++) {
			char *text = _fgn_str_escape(data.pairs[i].value);
			_fgn_str_append(&result, ct, cap, "\t%s %s\n", data.pairs[i].key, text == nullptr ? data.pairs[i].value : text);
			free(text);
		}
	};

	_fgn_str_append(&result, ct, cap, "-g %s\n", graph.id);
	write_data(graph.data, parser_graph);

	_fgn_str_append(&result, ct, cap, "\n");
	for (int32_t n = 0; n < graph.node_ct; n++) {
		_fgn_str_append(&result, ct, cap, "-n %s\n", graph.nodes[n].id);
		state.curr_node = n;
		write_data(graph.nodes[n].data, parser_node);
	}
	state.curr_node = -1;

	_fgn_str_append(&result, ct, cap, "\n");
	for (int32_t e = 0; e < graph.edge_ct; e++) {
		_fgn_str_append(&result, ct, cap, "-e %s, %s\n", graph.nodes[graph.edges[e].start].id, graph.nodes[graph.edges[e].end].id);
		state.curr_edge = e;
		write_data(graph.edges[e].data, parser_edge);
	}
	return result;
}
int32_t fgn_save_file(fgn_graph_t &lib, const char *filename, const fgn_parser_t *parser_node, const fgn_parser_t *parser_edge, const fgn_parser_t *parser_graph) {
	FILE *fp = nullptr;
	if (fopen_s(&fp, filename, "wb") != 0 && fp == nullptr)
		return 1;

	char *data = fgn_save(lib, parser_node, parser_edge, parser_graph);
	fwrite(data, strlen(data), 1, fp);
	fclose(fp);
	free(data);

	return 0;
}
void    fgn_destroy  (fgn_node_t &node) {
	free(node.id);
	free(node.in_edges);
	free(node.out_edges);
	fgn_data_destroy(node.data);
}
void    fgn_destroy  (fgn_library_t &lib) {
	fgn_data_destroy(lib.data);
	for (int32_t i = 0; i < lib.graph_ct; i++) {
		fgn_destroy(lib.graphs[i]);
	}
	free(lib.graphs);
	lib = {};
}
void    fgn_destroy  (fgn_graph_t &graph) {
	fgn_data_destroy(graph.data);
	for (int32_t i = 0; i < graph.edge_ct; i++) fgn_data_destroy(graph.edges[i].data);
	for (int32_t i = 0; i < graph.node_ct; i++) fgn_destroy(graph.nodes[i]);
	free(graph.edges);
	free(graph.nodes);
	free(graph.id);
}

///////////////////////////////////////////

fgn_graph_idx fgn_lib_add(fgn_library_t &lib, const char *id) {
	fgn_graph_idx result = _fgn_arr_add(&lib.graphs, 1, lib.graph_ct, lib.graph_cap);
	fgn_graph_set_id(lib.graphs[result], id);
	return result;
}
fgn_node_idx  fgn_lib_findid(const fgn_library_t &lib, const char *id) {
	fgn_hash_t hash = _fgn_str_hash(id);
	for (fgn_node_idx i = 0; i < lib.graph_ct; i++) {
		if (hash == lib.graphs[i].id_hash && _fgn_str_eq(id, lib.graphs[i].id))
			return i;
	}
	return -1;
}

///////////////////////////////////////////

void          fgn_graph_set_id     (fgn_graph_t &graph, const char *id) {
	graph.id      = _fgn_str_copy(id);
	graph.id_hash = _fgn_str_hash(id);
}
fgn_node_idx  fgn_graph_node_add   (fgn_graph_t &graph, const char *id) {
	assert(fgn_graph_node_findid(graph, id) == -1);
	fgn_node_idx result = _fgn_arr_add(&graph.nodes, 1, graph.node_ct, graph.node_cap);
	fgn_graph_node_setid(graph, result, id);
	return result;
}
void          fgn_graph_node_setid (fgn_graph_t &graph, fgn_node_idx idx, const char *text_id) {
	graph.nodes[idx].id      = _fgn_str_copy(text_id);
	graph.nodes[idx].id_hash = _fgn_str_hash(text_id);
}
fgn_node_idx  fgn_graph_node_findid(const fgn_graph_t &graph, const char *id) {
	fgn_hash_t hash = _fgn_str_hash(id);
	for (fgn_node_idx i = 0; i < graph.node_ct; i++) {
		if (hash == graph.nodes[i].id_hash && _fgn_str_eq(id, graph.nodes[i].id))
			return i;
	}
	return -1;
}
void          fgn_graph_node_delete(fgn_graph_t &graph, fgn_node_idx node) {
	for (int32_t i = 0; i < graph.edge_ct; i++) {
		fgn_edge_t &e = graph.edges[i];
		if (e.start == node || e.end == node) {
			fgn_graph_edge_delete(graph, i);
			i--;
		} else {
			if (e.start > node) e.start--;
			if (e.end   > node) e.end--;
		}
	}

	fgn_destroy(graph.nodes[node]);
	_fgn_arr_remove(&graph.nodes, node, graph.node_ct);
}
void          fgn_graph_node_delete(fgn_graph_t &graph, const char *node) {
	fgn_graph_node_delete(graph, fgn_graph_node_findid(graph, node));

}

fgn_edge_idx  fgn_graph_edge_add   (fgn_graph_t &graph, fgn_node_idx start, fgn_node_idx end) {
	assert(start >= 0 && end >= 0 && start < graph.node_ct && end < graph.node_ct);
	assert(start != end);
	fgn_edge_idx result = _fgn_arr_add(&graph.edges, 1, graph.edge_ct, graph.edge_cap);
	graph.edges[result].start = start;
	graph.edges[result].end   = end;

	// Cache edges on the node for fast lookup
	fgn_node_t &node_s = graph.nodes[start];
	node_s.out_edges[_fgn_arr_add(&node_s.out_edges, 1, node_s.out_ct, node_s.out_cap)] = result;
	fgn_node_t &node_e = graph.nodes[end];
	node_e.in_edges [_fgn_arr_add(&node_e.in_edges,  1, node_e.in_ct,  node_e.in_cap )] = result;
	return result;
}
fgn_edge_idx  fgn_graph_edge_add   (fgn_graph_t &graph, const char *start, const char *end) {
	return fgn_graph_edge_add(graph, fgn_graph_node_findid(graph, start), fgn_graph_node_findid(graph, end));
}
void          fgn_graph_edge_delete(fgn_graph_t &graph, fgn_edge_idx edge) {
	fgn_data_destroy(graph.edges->data);
	_fgn_arr_remove(&graph.edges, edge, graph.edge_ct);

	for (int32_t i = 0; i < graph.node_ct; i++) {
		fgn_node_t &n = graph.nodes[i];
		for (int32_t e = 0; e < n.in_ct; e++) {
			if (edge == n.in_edges[e]) {
				_fgn_arr_remove(&n.in_edges, e, n.in_ct);
				e--;
			} else if (n.in_edges[e] > edge) {
				n.in_edges[e]--;
			}
		}

		for (int32_t e = 0; e < n.out_ct; e++) {
			if (edge == n.out_edges[e]) {
				_fgn_arr_remove(&n.out_edges, e, n.out_ct);
				e--;
			} else if (n.out_edges[e] > edge) {
				n.out_edges[e]--;
			}
		}
	}
}

///////////////////////////////////////////

template<typename T> T &fgn_graph_node_data(const fgn_graph_t &graph, fgn_node_idx idx) {
	assert(idx >= 0 && idx < graph.node_ct);
	if (graph.nodes[idx].data.data == nullptr) {
		graph.nodes[idx].data.data = malloc(sizeof(T));
		memset(graph.nodes[idx].data.data, 0, sizeof(T));
	}
	return *(T*)graph.nodes[idx].data.data;
}
template<typename T> T *fgn_graph_node_data(const fgn_graph_t &graph, const char *id) {
	fgn_node_idx i = fgn_graph_node_findid(graph, id);
	return i == -1 ? nullptr : &fgn_graph_node_data<T>(graph, i);
}
template<typename T> T &fgn_graph_edge_data(const fgn_graph_t &graph, fgn_edge_idx idx) {
	assert(idx >= 0 && idx < graph.edge_ct);
	if (graph.edges[idx].data.data == nullptr) {
		graph.edges[idx].data.data = malloc(sizeof(T));
		memset(graph.edges[idx].data.data, 0, sizeof(T));
	}
	return *(T*)graph.edges[idx].data.data;
}

void                    fgn_data_add    (fgn_data_t &data, const char *key, const char *value) {
	int32_t i = _fgn_arr_add(&data.pairs, 1, data.pair_ct, data.pair_cap);
	data.pairs[i].key      = _fgn_str_copy(key);
	data.pairs[i].key_hash = _fgn_str_hash(key);
	data.pairs[i].value    = _fgn_str_copy(value);
}
void                    fgn_data_destroy(fgn_data_t &data) {
	for (int32_t i = 0; i < data.pair_ct; i++) {
		free(data.pairs[i].key);
		free(data.pairs[i].value);
	}
	free(data.pairs);
	data = {};
}
template<typename T> T &fgn_data_get    (const fgn_data_t &data) {
	return *(T *)data.data;
}

///////////////////////////////////////////

void fgn_parser_add(fgn_parser_t &parser, const char *name, int32_t offset,
	bool  (*parse)(fgn_parse_state_t state, const char *value_text, void *out_data),
	char *(*write)(fgn_parse_state_t state, void *value)) {
	int32_t i = _fgn_arr_add(&parser.items, 1, parser.item_ct, parser.item_cap);
	parser.items[i].key      = name;
	parser.items[i].key_hash = _fgn_str_hash(name);
	parser.items[i].offset   = offset;
	parser.items[i].parse    = parse;
	parser.items[i].write    = write;
}

void _fgn_parse(const fgn_parser_t &parser, fgn_parse_state_t state, fgn_data_t &data) {
	data.data = malloc(parser.type_size);
	memset(data.data, 0, parser.type_size);

	// Parse as many key/value pairs as possible!
	int32_t parsed = 0;
	for (int32_t i = 0; i < data.pair_ct; i++) {
		for (size_t p = 0; p < parser.item_ct; p++) {
			if (data.pairs[i].key_hash == parser.items[p].key_hash) {
				if (parser.items[p].parse(state, data.pairs[i].value, ((uint8_t *)data.data) + parser.items[p].offset)) {
					free(data.pairs[i].key);
					free(data.pairs[i].value);
					data.pairs[i].key = nullptr;
					parsed += 1;
				}
				break;
			}
		}
	}

	// Remove the parsed pairs from the list by creating a new list!
	_fgn_pair_t *old_pairs = data.pairs;
	int32_t     old_count = data.pair_ct;
	data.pair_cap = old_count - parsed;
	data.pair_ct  = 0;
	data.pairs    = nullptr;
	if (data.pair_cap > 0) {
		data.pairs = (_fgn_pair_t *)malloc(sizeof(_fgn_pair_t) * data.pair_cap);
		for (int32_t i = 0; i < old_count; i++) {
			if (old_pairs[i].key != nullptr) {
				data.pairs[data.pair_ct] = old_pairs[i];
				data.pair_ct += 1;
			}
		}
	}
	free(old_pairs);
}
void fgn_parse(fgn_graph_t &graph, const fgn_parser_t *parser_node, const fgn_parser_t *parser_edge, const fgn_parser_t *parser_graph ) {
	fgn_parse_state_t state = { graph, -1, -1 };

	// Parse the graph's data
	if (parser_graph != nullptr)
		_fgn_parse(*parser_graph, state, graph.data);

	// Parse node data
	if (parser_node != nullptr) {
		for (int32_t i = 0; i < graph.node_ct; i++) {
			state.curr_node = i;
			_fgn_parse(*parser_node, state, graph.nodes[i].data);
		}
	}
	state.curr_node = -1;

	// Parse edge data
	if (parser_edge != nullptr) {
		for (int32_t i = 0; i < graph.edge_ct; i++) {
			state.curr_edge = i;
			_fgn_parse(*parser_edge, state, graph.edges[i].data);
		}
	}
}
void fgn_parse(fgn_library_t &lib, const fgn_parser_t *parser_node, const fgn_parser_t *parser_edge, const fgn_parser_t *parser_graph) {
	for (int32_t i = 0; i < lib.graph_ct; i++) {
		fgn_parse(lib.graphs[i], parser_node, parser_edge, parser_graph);
	}
}

///////////////////////////////////////////

bool  fgn_parse_float (fgn_parse_state_t state, const char *value_text, void *out_data) {
	*((float*)out_data) = atof(value_text);
	return true;
}
char *fgn_write_float (fgn_parse_state_t state, void *value) {
	if (*(float *)value == 0) return nullptr;
	return _fgn_str_make("%g", *(float*)value);
}
bool  fgn_parse_float2(fgn_parse_state_t state, const char *value_text, void *out_data) {
	((float*)out_data)[0] = atof(value_text);
	((float*)out_data)[1] = atof(_fgn_str_next_word(value_text,','));
	return true;
}
char *fgn_write_float2(fgn_parse_state_t state,void *value) {
	if (((float *)value)[0] == 0 && ((float *)value)[1] == 0) return false;
	return _fgn_str_make("%g, %g", ((float*)value)[0], ((float*)value)[1]);
}
bool  fgn_parse_float3(fgn_parse_state_t state, const char *value_text, void *out_data) {
	((float*)out_data)[0] = atof(value_text);
	value_text = _fgn_str_next_word(value_text, ',');
	((float*)out_data)[1] = atof(value_text);
	value_text = _fgn_str_next_word(value_text, ',');
	((float*)out_data)[2] = atof(value_text);
	return true;
}
char *fgn_write_float3(fgn_parse_state_t state, void *value) {
	if (((float *)value)[0] == 0 && ((float *)value)[1] == 0 && ((float *)value)[2] == 0) return false;
	return _fgn_str_make("%g, %g, %g", ((float*)value)[0], ((float*)value)[1], ((float*)value)[2]);
}
bool  fgn_parse_int32 (fgn_parse_state_t state, const char *value_text, void *out_data) {
	*((int32_t*)out_data) = atoi(value_text);
	return true;
}
char *fgn_write_int32 (fgn_parse_state_t state, void *value) {
	if (*(int *)value == 0) return nullptr;
	return _fgn_str_make("%d", *(int*)value);
}
bool  fgn_parse_string(fgn_parse_state_t state, const char *value_text, void *out_data) {
	*((char**)out_data) = _fgn_str_copy(value_text);
	return true;
}
char *fgn_write_string(fgn_parse_state_t state, void *value) {
	if (*(char **)value == nullptr) return nullptr;
	return _fgn_str_copy(*(char **)value);
}
bool  fgn_parse_nodeid(fgn_parse_state_t state, const char *value_text, void *out_data) {
	*((fgn_node_idx*)out_data) = fgn_graph_node_findid(state.graph, value_text);
	return *((fgn_node_idx *)out_data) != -1;
}
char *fgn_write_nodeid(fgn_parse_state_t state, void *value) {
	if (*(fgn_node_idx *)value == -1) return nullptr;
	return _fgn_str_copy(fgn_graph_node_get(state.graph, *(fgn_node_idx*)value).id);
}

///////////////////////////////////////////
/// Private helper functions            ///
///////////////////////////////////////////

bool        _fgn_str_eq    (const char *a, const char *b) {
	while (*a != '\0' && *b != '\0') {
		if (*a != *b)
			return false;
		a++;
		b++;
	}
	return *a == *b;
}
bool        _fgn_str_starts(const char *a, const char *b) {
	while (*a != '\0' && *b != '\0') {
		if (*a != *b)
			return false;
		a++;
		b++;
	}
	return *b == '\0';
}
fgn_hash_t  _fgn_str_hash  (const char *string) {
	// FNV-1a hash (64bit): http://isthe.com/chongo/tech/comp/fnv/
	uint64_t hash = 14695981039346656037;
	uint8_t  c;
	while (c = *string++)
		hash = (hash ^ c) * 1099511628211;
	return hash;
}
char       *_fgn_str_copy  (const char *string) {
	size_t len = strlen(string);
	char *result = (char*)malloc(len + 1); 
	memcpy(result, string, len); 
	result[len] = '\0'; 
	return result; 
}
void        _fgn_str_append(char **string, int32_t &count, int32_t &cap, const char *text, ...) {
	va_list argptr;
	va_start(argptr, text);
	size_t length = vsnprintf(nullptr, 0, text, argptr);
	int32_t start = _fgn_arr_add(string, length, count, cap);
	vsnprintf((*string)+start, length+1, text, argptr);
	va_end(argptr);
}
char       *_fgn_str_make  (const char *text, ...) {
	va_list argptr;
	va_start(argptr, text);
	size_t length = vsnprintf(nullptr, 0, text, argptr);
	char *result = (char*)malloc(length+1);
	vsnprintf(result, length+1, text, argptr);
	va_end(argptr);
	return result;
}

///////////////////////////////////////////

const char *_fgn_str_trim     (const char *str) { 
	while (*str == '\t' || *str == ' ' || *str == '\r' || *str == '\n') 
		str++; 
	return str; 
}
const char *_fgn_str_next_line(const char *str)           { 
	bool q = false; 
	while (*str != '\0' && (q || (*str != '\n' && *str != '\r'               ))) { 
		if (*str == '"') q = !q; 
		str++; 
	} 
	return str; 
}
const char *_fgn_str_next_word(const char *str, char sep) { 
	bool q = false; 
	while (*str != '\0' && (q || (*str != '\n' && *str != '\r' && *str != sep))) { 
		if (*str == '"') q = !q; 
		str++; 
	} 
	if (*str == sep) str++; 
	return _fgn_str_trim(str);
}
char       *_fgn_str_copy_line(const char *str) { 
	const char *end = _fgn_str_next_line(str); 
	char *result = (char*)malloc((end - str) + 1); 
	memcpy(result, str, end - str); 
	result[end - str] = '\0'; 
	return result;
}
char       *_fgn_str_copy_word(const char *str, char sep) { 
	const char *end = str;
	bool q = false; 
	while (*end != '\0' && (q || (*end != '\n' && *end != '\r' && *end != sep))) { 
		if (*end == '"') q = !q; 
		end++; 
	}
	char *result = (char*)malloc((end - str) + 1); 
	memcpy(result, str, end - str); 
	result[end - str] = '\0'; 
	return result; 
}
bool        _fgn_str_line_has (const char *str, char sep) {
	bool q = false; 
	while (*str != '\0' && (q || (*str != '\n' && *str != '\r'))) { 
		if (*str == '"') q = !q;
		if (*str == sep) return true;
		str++; 
	} 
	return false;
}
bool        _fgn_str_contains (const char *str, char sep) {
	while (*str != '\0') { 
		if (*str == sep) return true;
		str++;
	} 
	return false;
}
char       *_fgn_str_escape   (const char *str) {
	if (_fgn_str_contains(str, '\n') || _fgn_str_contains(str, '"')) {
		if (_fgn_str_contains(str, '"')) {
			int32_t len = (int32_t)strlen(str);
			// Count the number of quotes in the text
			int count = 0;
			for (int32_t i = 0; i < len; i++) {
				if (str[i] == '"' || str[i] == '\\')
					count++;
			}
			// Replace the quotes with \' and any \ with \\ 
			char *result = (char*)malloc(len + 2 + count * 2);
			int   curr   = 1;
			result[0] = '"';
			for (int32_t i = 0; i < len; i++) {
				if (str[i] == '\\') {
					result[curr] = '\\'; curr++;
					result[curr] = '\\'; curr++;
				} else if (str[i] == '"') {
					result[curr] = '\\'; curr++;
					result[curr] = '\''; curr++;
				} else {
					result[curr] = str[i];
					curr++;
				}
			}
			// Finish it off with a quote and a terminating character
			result[curr] = '"'; curr++;
			result[curr] = '\0';
			return result;
		} else {
			return _fgn_str_make("\"%s\"", str);
		}
	}
	return nullptr;
}
void        _fgn_str_unescape (char *str) {
	char *curr   = str;
	char *write  = str;
	bool  escape = false;
	while (*curr != '\0') {
		if (escape) {
			if (*curr == '\'')
				*write = '\"';
			else if (*curr == '\\')
				*write = '\\';
		} else if (*curr != '"') {
			*write = *curr;
		}
		escape = *curr == '\\';
		if (!escape && *curr != '"')
			write++;
		curr++;
	}
	*write = *curr;
}

///////////////////////////////////////////

template<typename T> int32_t _fgn_arr_add   (T **arr, int32_t quantity, int32_t &count, int32_t &capacity) {
	int32_t result = count;
	count += quantity;
	if (count >= capacity) {
		capacity = capacity*2 > count ? capacity * 2 : count+1;
		*arr = (T*)realloc(*arr, sizeof(T) * capacity);
	}
	(*arr)[result] = {};
	return result;
}
template<typename T> void    _fgn_arr_remove(T **arr, int32_t index, int32_t &count) {
	if (index < count-1)
		memmove(&(*arr)[index], &(*arr)[index + 1], sizeof(T) * (count - (index+1)));
	count -= 1;
}

#endif
#endif