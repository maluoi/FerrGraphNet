#include "fgn_editor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#include "../../ferr_graphnet.h"

///////////////////////////////////////////

fgn_graph_idx active_graph = 0;
fgn_node_idx  selected_out = -1;
fgn_node_idx  selected_in  = -1;
fgn_node_idx  delete_node  = -1;
fgn_node_idx  delete_edge  = -1;

editor_node_t *node_state    = nullptr;
int32_t        node_state_ct = 0;

const fgne_editor_config_t fgne_default_config = {
	fgne_shell_default,
	fgne_meat_kvps,
	fgne_edge_curve,
	fgne_inouts_default,
	fgne_newnode_default,
	false
};

///////////////////////////////////////////

void fgne_draw_bar(fgn_library_t *library, fgn_graph_t *graph);
void fgne_draw_edge(const fgne_editor_config_t &config, fgn_graph_t &graph, fgn_edge_idx edge_idx);
void fgne_context_menu(fgne_func_newnode_t newnode_func, bool ask_for_id, fgn_graph_t &graph);
ImVec2 fgne_inouts_default(fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max) { return { inout == fgn_in ? min.x : max.x, min.y + (index / (float)index_max + (1 / (float)index_max) * 0.5f) * (max.y-min.y) }; }
ImVec2 fgne_inouts_center (fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max) { return (min + max) / 2.f; }
ImVec2 fgne_inouts_circle (fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max);
void fgne_select_in (fgn_graph_t &graph, fgn_node_idx in_idx);
void fgne_select_out(fgn_graph_t &graph, fgn_node_idx out_idx);
bool fgne_id_popup(const char *title, char *id_buffer, int ib_buffer_size, void *data, bool (*is_valid)(const void *data, const char *id, const char **out_err));

float dist_to_line_sq(ImVec2 a, ImVec2 b, ImVec2 pt);
bool  valid_node_id (const void *data, const char *id, const char **out_err);
bool  valid_graph_id(const void *data, const char *id, const char **out_err);
void  make_unique_id(fgn_graph_t &graph, char *id_buffer, int id_buffer_size);

///////////////////////////////////////////

void fgne_init() {
}

///////////////////////////////////////////

void fgne_shutdown() {
	free(node_state);
}

///////////////////////////////////////////

void fgne_draw(fgn_library_t &lib, fgne_editor_state_t &state, const fgne_editor_config_t *config) {
	fgne_draw_bar(&lib, lib.graph_ct > lib.graph_ct > active_graph ? &lib.graphs[active_graph] : nullptr);

	if (lib.graph_ct > active_graph && active_graph >= 0)
		fgne_draw(lib.graphs[active_graph], state, config);
}

///////////////////////////////////////////

void fgne_draw_bar(fgn_library_t *library, fgn_graph_t *graph) {
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	
	ImGui::BeginChildFrame(ImGui::GetID("Header"), ImVec2(0, g.FontSize + style.FramePadding.y*4), ImGuiWindowFlags_AlwaysUseWindowPadding);
	
	if (library != nullptr) {
		ImGui::PushItemWidth(200);
		ImGui::Combo("##Graphs", &active_graph, [](void *data, int i, const char **out_text) {
			*out_text = ((fgn_library_t*)data)->graphs[i].id;
			return true;
		}, library, library->graph_ct);
		ImGui::PopItemWidth();

		ImGui::SameLine();
		ImGui::PushItemWidth(100);
		if (ImGui::Button("New Graph")) {
			ImGui::OpenPopup("Graph's Id");
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete")) {
			fgn_lib_delete(*library, active_graph);
			if (active_graph >= fgn_lib_count(*library))
				active_graph = fgn_lib_count(*library) - 1;
			selected_in  = -1;
			selected_out = -1;
			delete_edge  = -1;
			delete_node  = -1;
		}
		ImGui::PopItemWidth();
	}

	if (library != nullptr) {
		static char id[128] = "";
		if (fgne_id_popup("Graph's Id", id, sizeof(id), library, valid_graph_id)) {
			active_graph = fgn_lib_add(*library, id);
			id[0] = '\0';
		}
	}
	
	ImGui::EndChild();
}

///////////////////////////////////////////

void fgne_draw(fgn_graph_t &graph, fgne_editor_state_t &state, const fgne_editor_config_t *config) {
	if (config == nullptr)
		config = &fgne_default_config;

	fgne_func_newnode_t newnode_func = config->newnode_func == nullptr ? fgne_newnode_default : config->newnode_func;

	ImGui::BeginChild("ScrollArea", {0,0}, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove );

	// Make sure our state tracking has memory enough for each item
	if (node_state_ct != graph.node_ct) {
		node_state = (editor_node_t *)realloc(node_state, sizeof(editor_node_t) * graph.node_ct);
		if (graph.node_ct > node_state_ct)
			memset(&node_state[node_state_ct], 0, sizeof(editor_node_t) * (graph.node_ct - node_state_ct));
		node_state_ct = graph.node_ct;
	}

	// Draw edges first, so they go behind the nodes
	// We're pushing in some rood ids so that the draw function
	// can use whatever ids it wants without having to worry about
	// overlap.
	if (config->edge_func != nullptr) {
		ImGui::PushID("edges");
		for (int32_t i = 0; i < graph.edge_ct; i++) {
			ImGui::PushID(i);
			fgne_draw_edge(*config, graph, i);
			ImGui::PopID();
		}
		ImGui::PopID();
	}

	// And now draw the nodes!
	if (config->shell_func != nullptr) {
		ImGui::PushID("nodes");
		for (int32_t i = 0; i < graph.node_ct; i++) {
			ImGui::PushID(i);
			if (config->shell_func(graph, i, node_state[i], config->meat_func)) {
				fgn_node_t &node = fgn_graph_node_get(graph, i);
				node.position[0] += ImGui::GetMouseDragDelta(0).x;
				node.position[1] += ImGui::GetMouseDragDelta(0).y;
				ImGui::ResetMouseDragDelta(0);
				if (node.position[0] < 0)
					node.position[0] = 0;
				if (node.position[1] < 0)
					node.position[1] = 0;
			}
			ImGui::PopID();
		}
		ImGui::PopID();
	}

	fgne_context_menu(newnode_func, config->ask_for_id, graph);

	if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowFocused()) {
		// Drag the view around
		bool hovered, held;
		ImRect r = ImGui::GetCurrentWindow()->Rect();
		if (ImGui::IsMouseDown(0)) {
			ImGui::SetScrollX(ImGui::GetScrollX() - ImGui::GetMouseDragDelta(0).x);
			ImGui::SetScrollY(ImGui::GetScrollY() - ImGui::GetMouseDragDelta(0).y);
			ImGui::ResetMouseDragDelta(0);
		}

		// Discard selected in/out on a stationary mouse click
		static ImVec2 clicked_at = { -1,-1 };
		if (ImGui::IsMouseClicked(0)) {
			clicked_at = ImGui::GetMousePos();
		}
		if (ImGui::IsMouseReleased(0) && ImGui::GetMousePos().x == clicked_at.x && ImGui::GetMousePos().y == clicked_at.y) {
			selected_in = -1;
			selected_out = -1;
			clicked_at = { -1,-1 };
		}
	}
	
	// Draw line from the active node to the mouse
	if (selected_in != -1) {
		int32_t       ct = fgn_graph_node_get(graph, selected_in).in_ct;
		editor_node_t &s = node_state[selected_in];
		ImGui::GetWindowDrawList()->AddLine(
			fgne_inouts_default(fgn_in, s.node_min, s.node_max, ct, ct+1),
			ImGui::GetMousePos(), 
			ImGui::GetColorU32({ 1,1,1,1 }));
	}
	if (selected_out != -1) {
		int32_t       ct = fgn_graph_node_get(graph, selected_out).out_ct;
		editor_node_t &s = node_state[selected_out];
		ImGui::GetWindowDrawList()->AddLine(
			fgne_inouts_default(fgn_out, s.node_min, s.node_max, ct, ct+1),
			ImGui::GetMousePos(), 
			ImGui::GetColorU32({ 1,1,1,1 }));
	}

	// Delete items all the way at the end!
	if (delete_node != -1) {
		fgn_graph_node_delete(graph, delete_node);
		if (selected_in > delete_node)
			selected_in--;
		if (selected_out > delete_node)
			selected_out--;
		if (selected_in == delete_node)
			selected_in = -1;
		if (selected_out == delete_node)
			selected_out = -1;
		delete_node = -1;
	}
	if (delete_edge != -1) {
		fgn_graph_edge_delete(graph, delete_edge);
		delete_edge = -1;
	}

	ImGui::EndChild();
}

///////////////////////////////////////////

bool fgne_shell_default(fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state, fgne_func_meat_t node_meat) {
	fgn_node_t   &node   = fgn_graph_node_get(graph, node_idx);
	ImGuiWindow  *window = ImGui::GetCurrentWindow();
	ImGuiContext &g      = *GImGui;
	ImGuiStyle   &style  = ImGui::GetStyle();

	const ImVec2 label_size   = ImGui::CalcTextSize(node.id, nullptr, false);
	const float  frame_height = fmaxf(fminf(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y*2), label_size.y + style.FramePadding.y*2);

	// Begin the box
	ImVec2 node_pos = { node.position[0],node.position[1] + frame_height };
	ImGui::SetCursorPos(node_pos);
	ImGui::BeginGroup();

	// Draw the background first, we'll use the previous frame's size and position. A bit laggy when resizing, but not bad.
	window->DrawList->AddRectFilled(node_state.node_min, node_state.node_max, ImGui::GetColorU32(ImGuiCol_WindowBg), style.WindowRounding, ImDrawCornerFlags_All);

	// Node content
	ImGui::BeginGroup();
	if (!node_state.hidden) {
		node_meat(graph, node_idx);
	} else {
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::ItemSize({ pos,pos + ImVec2{80,ImGui::GetFrameHeight()} });
	}
	ImGui::EndGroup();

	// Find the bounds of the node
	node_state.node_min = ImGui::GetItemRectMin() - style.FramePadding - ImVec2{0,frame_height};
	node_state.node_max = ImGui::GetItemRectMax() + style.FramePadding ;
	
	// Add a header to the node
	ImVec2       node_head_max = { node_state.node_max.x, node_state.node_min.y + frame_height };
	const float  text_offset_x = g.FontSize + style.FramePadding.x;
	ImVec2       text_pos      = { node_pos.x + text_offset_x, node_state.node_min.y + fmaxf(style.FramePadding.y, window->DC.CurrLineTextBaseOffset) };
	
	// Manage the state of the node itself
	bool held = false, hovered = false;
	ImRect header_bounds = { node_state.node_min + ImVec2{text_offset_x,0}, node_head_max + ImVec2{-text_offset_x, 0} };
	ImGui::ButtonBehavior(header_bounds, window->GetID("header"), &hovered, &held);
	ImU32 bg_col = ImGui::GetColorU32((held) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);

	// Draw the back of the node
	window->DrawList->AddRectFilled(node_state.node_min, node_head_max, bg_col, style.WindowRounding, ImDrawCornerFlags_Top);
	window->DrawList->AddRect      (node_state.node_min, node_state.node_max, ImGui::GetColorU32(ImGuiCol_Border), style.WindowRounding, ImDrawCornerFlags_All, style.WindowBorderSize);
	
	// Draw the header buttons
	if (ImGui::CollapseButton(window->GetID("collapse"), ImVec2(node_state.node_min.x, node_state.node_min.y), nullptr)) { node_state.hidden = !node_state.hidden; }
	if (ImGui::CloseButton   (window->GetID("close"),    ImVec2(node_state.node_max.x - text_offset_x, node_state.node_min.y))) { delete_node = node_idx;  }
	window->DrawList->AddText(text_pos - ImVec2{ImGui::GetScrollX()-ImGui::GetWindowPos().x,0}, ImGui::GetColorU32(ImGuiCol_Text), node.id);
	
	// Draw edge buttons
	float vcenter = node_state.node_min.y + (node_state.node_max.y - node_state.node_min.y) / 2;
	ImVec2 pos    = ImVec2{ node_state.node_max.x, vcenter }  + ImVec2{ 0, -g.FontSize / 4.f };
	ImRect bounds = { pos, pos + ImVec2{ g.FontSize,g.FontSize } };
	ImGui::RenderArrow(window->DrawList, pos, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Right);
	if (ImGui::ButtonBehavior(bounds, ImGui::GetID("add_out"), &hovered, nullptr))
		fgne_select_out(graph, node_idx);
	window->DrawList->AddCircleFilled(bounds.GetCenter(), ImMax(2.0f, g.FontSize * 0.5f), ImGui::GetColorU32(ImGuiCol_ButtonHovered, hovered?1:0));

	pos    = ImVec2{ node_state.node_min.x, vcenter } + ImVec2{ -g.FontSize, -g.FontSize / 4.f };
	bounds = { pos, pos + ImVec2{ g.FontSize,g.FontSize } };
	ImGui::RenderArrow(window->DrawList, pos, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Left);
	if (ImGui::ButtonBehavior(bounds, ImGui::GetID("add_in"), &hovered, nullptr))
		fgne_select_in(graph, node_idx);
	window->DrawList->AddCircleFilled(bounds.GetCenter(), ImMax(2.0f, g.FontSize * 0.5f), ImGui::GetColorU32(ImGuiCol_ButtonHovered, hovered?1:0));

	ImGui::EndGroup();

	return held;
}

///////////////////////////////////////////

bool fgne_shell_circle(fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state, fgne_func_meat_t node_meat) {
	fgn_node_t &node     = fgn_graph_node_get(graph, node_idx);
	ImVec2      node_pos = { node.position[0], node.position[1] };
	ImGuiStyle &style    = ImGui::GetStyle();

	// Draw the background
	float  radius = (node_state.node_max.x - node_state.node_min.x)/2;
	ImGui::GetWindowDrawList()->AddCircleFilled((node_state.node_min + node_state.node_max)/2, radius, ImGui::GetColorU32(ImGuiCol_WindowBg), 24);

	// Draw and size the content of the node
	ImGui::SetCursorPos(node_pos);
	ImGui::BeginGroup();
	node_meat(graph, node_idx);
	ImGui::EndGroup();
	node_state.node_min = ImGui::GetItemRectMin() - style.FramePadding;
	node_state.node_max = ImGui::GetItemRectMax() + style.FramePadding;
	ImVec2 size   = (node_state.node_max - node_state.node_min)/2;
	ImVec2 center = (node_state.node_max + node_state.node_min)/2;
	radius = sqrtf(size.x*size.x + size.y*size.y);
	node_state.node_min = center - ImVec2{radius, radius};
	node_state.node_max = center + ImVec2{radius, radius};

	// Draw edge buttons
	const float sz = 8;
	bool hovered = false;
	ImVec2 pos    = node_state.node_max - ImVec2{ sz, radius + sz };
	ImRect bounds = { pos, pos + ImVec2{ sz*2,sz*2 } };
	if (ImGui::ButtonBehavior(bounds, ImGui::GetID("add_out"), &hovered, nullptr))
		fgne_select_out(graph, node_idx);
	ImGui::GetWindowDrawList()->AddCircleFilled(bounds.GetCenter(), sz, hovered ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : ImGui::GetColorU32(ImGuiCol_Button));

	pos    = node_state.node_min + ImVec2{ -sz, radius - sz };
	bounds = { pos, pos + ImVec2{ sz*2,sz*2 } };
	if (ImGui::ButtonBehavior(bounds, ImGui::GetID("add_in"), &hovered, nullptr))
		fgne_select_in(graph, node_idx);
	ImGui::GetWindowDrawList()->AddCircleFilled(bounds.GetCenter(), sz, hovered ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : ImGui::GetColorU32(ImGuiCol_Button));

	// Button behavior, and a border circle
	bool held = false, is_open = true;
	bool pressed = ImGui::ButtonBehavior({ node_state.node_min, node_state.node_max }, ImGui::GetID("header"), &hovered, &held);
	ImGui::GetWindowDrawList()->AddCircle((node_state.node_min + node_state.node_max)/2, radius, hovered ? ImGui::GetColorU32(ImGuiCol_ButtonHovered) : ImGui::GetColorU32(ImGuiCol_Button), 24);

	return held;
}

///////////////////////////////////////////

ImVec2 fgne_inouts_circle(fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max) {
	float radius = (max.x - min.x) / 2;
	float angle  = (index + 1) * (1.f / index_max) * IM_PI;
	angle = (IM_PI / 2.f) + angle;
	return (max+min)/2.f + ImVec2{ cosf(angle) * radius * (inout * -2 + 1), sinf(angle) * radius };
}

///////////////////////////////////////////

void fgne_draw_edge(const fgne_editor_config_t &config, fgn_graph_t &graph, fgn_edge_idx edge_idx) {
	fgn_edge_t    &edge    = fgn_graph_edge_get(graph, edge_idx);
	fgn_node_t    &start_n = fgn_graph_node_get(graph, edge.start);
	fgn_node_t    &end_n   = fgn_graph_node_get(graph, edge.end);
	editor_node_t &start   = node_state[edge.start];
	editor_node_t &end     = node_state[edge.end];
	int32_t start_i = 0, end_i = 0;
	for (int32_t i = 0; i < start_n.out_ct; i++) if (start_n.out_edges[i] == edge_idx) { start_i = i; break; }
	for (int32_t i = 0; i < end_n  .in_ct;  i++) if (end_n  .in_edges [i] == edge_idx) { end_i   = i; break; }
	ImVec2 p1 = config.inout_func(fgn_out, start.node_min, start.node_max, start_i, start_n.out_ct+1);
	ImVec2 p2 = config.inout_func(fgn_in,  end  .node_min, end  .node_max, end_i,   end_n  .in_ct +1);

	config.edge_func(graph, edge_idx, p1, p2);
}

///////////////////////////////////////////

void fgne_edge_curve(fgn_graph_t &graph, fgn_edge_idx edge_idx, ImVec2 p1, ImVec2 p2) {
	float dist = dist_to_line_sq(p1, p2, ImGui::GetMousePos());

	ImGui::GetWindowDrawList()->AddBezierCurve(p1, p1 + ImVec2{60, 0}, p2 - ImVec2{60, 0}, p2, dist < 30 * 30 ? ImGui::GetColorU32({ .7f,0,0,1 }) : ImGui::GetColorU32({ .5f,.5f,.5f,1 }), 1);

	if (dist < 30*30) {
		ImVec2 size = ImVec2{ GImGui->FontSize / 2, GImGui->FontSize / 2 } + GImGui->Style.FramePadding;
		ImGui::SetCursorPos((p1 + p2) / 2);
		if (ImGui::CloseButton(ImGui::GetID("delete_edge"), (p1 + p2) / 2 - size)) {
			delete_edge = edge_idx;
		}
	}
}

///////////////////////////////////////////

void fgne_edge_straight(fgn_graph_t &graph, fgn_edge_idx edge_idx, ImVec2 p1, ImVec2 p2) {
	float dist = dist_to_line_sq(p1, p2, ImGui::GetMousePos());

	ImGui::GetWindowDrawList()->AddLine( p1, p2, 
		dist < 30*30 ? ImGui::GetColorU32({ .7f,0,0,1 }) : ImGui::GetColorU32({ .5f,.5f,.5f,1 }));

	if (dist < 30*30) {
		ImVec2 size = ImVec2{ GImGui->FontSize / 2, GImGui->FontSize / 2 } + GImGui->Style.FramePadding;
		ImGui::SetCursorPos((p1 + p2) / 2);
		if (ImGui::CloseButton(ImGui::GetID("delete_edge"), (p1 + p2) / 2 - size)) {
			delete_edge = edge_idx;
		}
	}
}

///////////////////////////////////////////

void fgne_context_menu(fgne_func_newnode_t newnode_func, bool ask_for_id, fgn_graph_t &graph) {
	bool open_node_name = false;
	static ImVec2 context_pos = {};
	if (!ImGui::IsAnyItemHovered() && 
		ImGui::IsMouseClicked(1)) {
		ImGui::OpenPopup("context_menu");
		context_pos = ImGui::GetMousePos();
	}

	if (ImGui::BeginPopup("context_menu")) {
		if (ImGui::MenuItem("Add Node", "Ctrl+N", false, true)) {
			if (ask_for_id) {
				open_node_name = true;
			} else {
				char id[128];
				make_unique_id(graph, id, sizeof(id));
				newnode_func(graph, id, context_pos);
			}
		}
		ImGui::Separator();
		ImGui::EndPopup();
	}
	if (ImGui::BeginPopup("context_menu_node")) {
		if (ImGui::MenuItem("Rename", nullptr, false, true)) {
		}
		if (ImGui::MenuItem("Delete", nullptr, false, true)) {
		}
		ImGui::Separator();
		ImGui::EndPopup();
	}

	// Add node name dialog, this is outside of the menu due to 
	// the way ImGui::EndPopup interacts with OpenPopup!
	if (open_node_name) {
		ImGui::OpenPopup("Node's Id");
		open_node_name = false;
	}

	static char id[128] = "";
	if (fgne_id_popup("Node's Id", id, sizeof(id), &graph, valid_node_id)) {
		newnode_func(graph, id, context_pos);
		id[0] = '\0';
	}
}

///////////////////////////////////////////

void fgne_select_in(fgn_graph_t &graph, fgn_node_idx in_idx) {
	if (selected_out == in_idx) return;
	selected_in = in_idx;
	if (selected_in != -1 && selected_out != -1) {
		fgn_graph_edge_add(graph, selected_out, selected_in);

		selected_in = -1;
		selected_out = -1;
	}
}

///////////////////////////////////////////

void fgne_select_out(fgn_graph_t &graph, fgn_node_idx out_idx) {
	if (selected_in == out_idx) return;
	selected_out = out_idx;
	if (selected_in != -1 && selected_out != -1) {
		printf("Added edge!\n");
		fgn_graph_edge_add(graph, selected_out, selected_in);

		selected_in = -1;
		selected_out = -1;
	}
}

///////////////////////////////////////////

bool fgne_id_popup(const char *title, char *id_buffer, int ib_buffer_size, void *data, bool (*is_valid)(const void *data, const char *id, const char **out_err)) {
	static bool open   = true;
	bool        result = false;
	if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		static const char *err_text = nullptr;

		if (open) {
			ImGui::SetKeyboardFocusHere();
			err_text = nullptr;
			open = false;
		}

		bool confirm = ImGui::InputText("Id", id_buffer, ib_buffer_size, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
		if (err_text != nullptr)
			ImGui::Text(err_text);
		if (ImGui::Button("Create") || confirm) {
			if (strcmp(id_buffer, "") == 0) {
				err_text = "Id can't be empty!";
			} else if (is_valid(data, id_buffer, &err_text)) {
				ImGui::CloseCurrentPopup(); 
				open = true;
				result = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
			open = true;
		}
		ImGui::EndPopup();
	}
	return result;
}

///////////////////////////////////////////

inline float dot(ImVec2 a, ImVec2 b) {
	return a.x * b.x + a.y * b.y;
}

///////////////////////////////////////////

float dist_to_line_sq(ImVec2 a, ImVec2 b, ImVec2 pt) {
	ImVec2 diff = b - a;
	float length2 = dot(diff, diff);
	if (length2 == 0) {
		diff = pt - a;
		return dot(diff, diff);
	}
		
	const float t = fmaxf(0, fminf(1, dot(pt - a, diff) / length2));
	const ImVec2 projection = a + diff * t;  // Projection falls on the segment
	diff = pt - projection;
	return dot(diff, diff);
}

///////////////////////////////////////////

void fgne_meat_kvps(fgn_graph_t &graph, fgn_node_idx node_idx) {
	fgn_node_t &node = fgn_graph_node_get(graph, node_idx);
	ImGui::PushItemWidth(150);

	char new_value[512];
	for (size_t i = 0; i < node.data.pair_ct; i++) {
		sprintf_s(new_value, "%s", node.data.pairs[i].value);
		if (ImGui::InputText(node.data.pairs[i].key, new_value, 512)) {
			free(node.data.pairs[i].value);

			size_t len = strlen(new_value);
			node.data.pairs[i].value = (char*)malloc(len + 1); 
			memcpy(node.data.pairs[i].value, new_value, len); 
			node.data.pairs[i].value[len] = '\0'; 
		}
	}
	static char key_name[128] = {};
	ImGui::InputText("##key_name", key_name, 128);
	ImGui::SameLine();
	if (ImGui::Button("+")) {
		if (strcmp(key_name,"") != 0)
			fgn_data_add(node.data, key_name, "");
		key_name[0] = '\0';
	}

	ImGui::PopItemWidth();
}

///////////////////////////////////////////

void fgne_meat_name(fgn_graph_t &graph, fgn_node_idx node_idx) {
	const char *text = fgn_graph_node_get(graph, node_idx).id;
	ImVec2 text_size = ImGui::CalcTextSize(text);
	ImVec2 size = text_size + ImGui::GetStyle().FramePadding;
	size.x = fmaxf(size.x, 60);
	size.y = fmaxf(size.y, 40);
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	ImGui::GetWindowDrawList()->AddText((window->DC.CursorPos + (size / 2)) - text_size / 2, ImGui::GetColorU32(ImGuiCol_Text), text);
	ImGui::ItemSize({ {0,0}, size });
}

///////////////////////////////////////////

void  fgne_newnode_default(fgn_graph_t &graph, const char *id, ImVec2 pos) {
	fgn_node_idx new_node = fgn_graph_node_add(graph, id);
	fgn_node_t  &n        = fgn_graph_node_get(graph, new_node);
	n.position[0] = pos.x;
	n.position[1] = pos.y;
}

///////////////////////////////////////////

bool valid_node_id(const void *data, const char *id, const char **out_err) {
	fgn_graph_t *graph = (fgn_graph_t *)data;
	if (fgn_graph_node_findid(*graph, id) != -1) {
		*out_err = "That id is already in use!";
		return false;
	}
	return true;
}

///////////////////////////////////////////

bool valid_graph_id(const void *data, const char *id, const char **out_err) {
	fgn_library_t *lib = (fgn_library_t *)data;
	if (fgn_lib_findid(*lib, id) != -1) {
		*out_err = "That id is already in use!";
		return false;
	}
	return true;
}

///////////////////////////////////////////

void make_unique_id(fgn_graph_t &graph, char *id_buffer, int id_buffer_size) {
	int idx = graph.node_ct+1;
	sprintf_s(id_buffer, id_buffer_size, "node_%d", idx);
	while (fgn_graph_node_findid(graph, id_buffer) != -1) {
		idx++;
		sprintf_s(id_buffer, id_buffer_size, "node_%d", idx);
	}
}