#pragma once

#include "ImGui/imgui.h"
#include "../../ferr_graphnet.h"

///////////////////////////////////////////

enum fgn_inout {
	fgn_in,
	fgn_out
};

///////////////////////////////////////////

struct editor_node_t {
	ImVec2 node_min;
	ImVec2 node_max;
	bool   hidden;
};

///////////////////////////////////////////

void fgne_init();
void fgne_shutdown();

void fgne_draw(fgn_library_t &lib);
void fgne_draw(fgn_graph_t &graph);

// Configuration examples and defaults

void fgne_shell_default(fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state);
void fgne_shell_circle (fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state);

void fgne_meat_kvps(fgn_node_t &node);
void fgne_meat_name(fgn_node_t &node);

void fgne_edge_curve();
void fgne_edge_straight();

ImVec2 fgne_inouts_default(fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max);
ImVec2 fgne_inouts_center (fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max);