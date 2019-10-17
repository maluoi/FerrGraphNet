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

struct fgne_editor_config_t {
	void  (*node_shell )(fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state);
	void  (*node_meat  )(fgn_graph_t &graph, fgn_node_idx node_idx);
	void  (*node_edge  )(fgn_graph_t &graph, fgn_edge_idx edge_idx, ImVec2 p1, ImVec2 p2);
	ImVec2(*node_inouts)(fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max);
};

struct fgne_editor_state_t {
	fgn_graph_idx active_graph;
	fgn_node_idx  active_node;
};

///////////////////////////////////////////

void fgne_init();
void fgne_shutdown();

void fgne_draw(fgn_library_t &lib,   fgne_editor_state_t &state, const fgne_editor_config_t *config = nullptr);
void fgne_draw(fgn_graph_t   &graph, fgne_editor_state_t &state, const fgne_editor_config_t *config = nullptr);

// Configuration examples and defaults

void fgne_shell_default(fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state);
void fgne_shell_circle (fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state);

void fgne_meat_kvps(fgn_graph_t &graph, fgn_node_idx node_idx);
void fgne_meat_name(fgn_graph_t &graph, fgn_node_idx node_idx);

void fgne_edge_curve   (fgn_graph_t &graph, fgn_edge_idx edge_idx, ImVec2 p1, ImVec2 p2);
void fgne_edge_straight(fgn_graph_t &graph, fgn_edge_idx edge_idx, ImVec2 p1, ImVec2 p2);

ImVec2 fgne_inouts_default(fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max);
ImVec2 fgne_inouts_center (fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max);