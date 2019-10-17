#include "fgn_editor.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"

#include "../../ferr_graphnet.h"

///////////////////////////////////////////

fgn_parser_t  editor_parser = {};
fgn_graph_idx active_graph = 0;
fgn_node_idx  selected_out = -1;
fgn_node_idx  selected_in  = -1;
fgn_node_idx  delete_node  = -1;
fgn_node_idx  delete_edge  = -1;

editor_node_t *node_state    = nullptr;
int32_t        node_state_ct = 0;

///////////////////////////////////////////

void fgne_draw(fgn_library_t &lib);
void fgne_draw_bar(fgn_library_t *library, fgn_graph_t *graph);
void fgne_draw_edge(fgn_graph_t &graph, fgn_edge_t &edge, fgn_edge_idx edge_idx);
void fgne_context_menu(fgn_graph_t &graph);
ImVec2 fgne_inouts_default(fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max) { return { inout == fgn_in ? min.x : max.x, min.y + (index / (float)index_max + (1 / (float)index_max) * 0.5f) * (max.y-min.y) }; }
ImVec2 fgne_inouts_center (fgn_inout inout, ImVec2 min, ImVec2 max, int index, int index_max) { return (min + max) / 2.f; }
void fgne_select_in (fgn_graph_t &graph, fgn_node_idx in_idx);
void fgne_select_out(fgn_graph_t &graph, fgn_node_idx out_idx);
void fgne_id_popup(const char *title, void *data, bool (*create)(void *data, const char *id, const char **out_err));

float dist_to_line_sq(ImVec2 a, ImVec2 b, ImVec2 pt);

void fgne_meat_kvps(fgn_node_t &node);

///////////////////////////////////////////

void fgne_init() {
}

///////////////////////////////////////////

void fgne_shutdown() {
	free(node_state);
}

///////////////////////////////////////////

bool todo_remove_me = true;
void fgne_draw(fgn_library_t &lib) {

	if (todo_remove_me) {
		fgn_parse(lib, &editor_parser);
		todo_remove_me = false;
	}
	
	fgne_draw_bar(&lib, lib.graph_ct > lib.graph_ct > active_graph ? &lib.graphs[active_graph] : nullptr);

	if (lib.graph_ct > active_graph && active_graph >= 0)
		fgne_draw(lib.graphs[active_graph]);
}

///////////////////////////////////////////

void fgne_draw_bar(fgn_library_t *library, fgn_graph_t *graph) {
	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	
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
		fgne_id_popup("Graph's Id", library, [](void *data, const char *id, const char **out_err) {
			fgn_library_t *lib = (fgn_library_t*)data;
			if (fgn_lib_findid(*lib, id) != -1) {
				*out_err = "That id is already in use!";
				return false;
			}
			active_graph = fgn_lib_add(*lib, id);
			return true;
		});
	}
	
	ImGui::EndChild();
	//ImGui::PopStyleVar();
}

///////////////////////////////////////////

void fgne_draw(fgn_graph_t &graph) {
	ImGui::BeginChild("ScrollArea", {0,0}, true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoMove);

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
	ImGui::PushID("edges");
	for (int32_t i = 0; i < graph.edge_ct; i++) {
		ImGui::PushID(i);
		fgne_draw_edge(graph, graph.edges[i], i);
		ImGui::PopID();
	}
	ImGui::PopID();

	// And now draw the nodes!
	ImGui::PushID("nodes");
	for (int32_t i = 0; i < graph.node_ct; i++) {
		ImGui::PushID(i);
		fgne_shell_default(graph, i, node_state[i]);
		ImGui::PopID();
	}
	ImGui::PopID();

	fgne_context_menu(graph);

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

void fgne_shell_default(fgn_graph_t &graph, fgn_node_idx node_idx, editor_node_t &node_state) {
	fgn_node_t   &node   = fgn_graph_node_get(graph, node_idx);
	ImGuiWindow  *window = ImGui::GetCurrentWindow();
	ImGuiContext &g      = *GImGui;
	ImGuiStyle   &style  = g.Style;

	ImU32 node_back_col = ImGui::GetColorU32(ImGuiCol_WindowBg);
	ImU32 node_head_col = ImGui::GetColorU32({ 1,1,1,1 }); //ImGui::GetColorU32(ImGuiCol_TitleBg);
	ImU32 node_line_col = ImGui::GetColorU32(ImGuiCol_Border);
	float rounding      = 4;

	// Begin the box
	ImVec2 node_pos = { node.position[0],node.position[1] };
	ImGui::SetCursorPos(node_pos);
	ImGui::BeginGroup();

	const ImVec2 label_size   = ImGui::CalcTextSize(node.id, nullptr, false);
	const float  frame_height = fmaxf(fminf(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y*2), label_size.y + style.FramePadding.y*2);

	// Draw the background first, we'll use the previous frame's size and position. A bit laggy when resizing, but not bad.
	window->DrawList->AddRectFilled(node_state.node_min, node_state.node_max, node_back_col, style.WindowRounding, ImDrawCornerFlags_All);

	// Node content
	ImGui::BeginGroup();
	if (!node_state.hidden) {
		fgne_meat_kvps(node);
	} else {
		ImVec2 pos = ImGui::GetCursorPos();
		ImGui::ItemSize({ pos,pos + ImVec2{80,ImGui::GetFrameHeight()} });
	}
	ImGui::EndGroup();

	// Find the bounds of the node
	ImVec2 node_min = ImGui::GetItemRectMin();
	ImVec2 node_max = ImGui::GetItemRectMax();
	node_min -= style.FramePadding;
	node_max += style.FramePadding;
	node_state.node_min = node_min;
	node_state.node_max = node_max;
	
	// Add a header to the node
	
	node_min.y -= frame_height;
	ImVec2       node_head_max = { node_max.x, node_min.y + frame_height };
	const float  text_offset_x = g.FontSize + style.FramePadding.x;
	ImVec2       text_pos = { node_pos.x + text_offset_x, node_min.y + fmaxf(style.FramePadding.y, window->DC.CurrLineTextBaseOffset) };
	
	// Manage the state of the node itself
	bool held = false, hovered = false, is_open = true;
	bool pressed = ImGui::ButtonBehavior({ node_min + ImVec2{text_offset_x,0}, node_head_max + ImVec2{-text_offset_x, 0} }, window->GetID("header"), &hovered, &held);
	ImU32 bg_col = ImGui::GetColorU32((held) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);

	// Draw the back of the node
	window->DrawList->AddRectFilled(node_min, node_head_max, bg_col, style.WindowRounding, ImDrawCornerFlags_Top);
	//draw_list->AddRectFilled(node_min + ImVec2{0,frame_height}, node_max, node_back_col, style.WindowRounding, ImDrawCornerFlags_Bot);
	window->DrawList->AddRect(node_min, node_max, node_line_col, style.WindowRounding, ImDrawCornerFlags_All, style.WindowBorderSize);
	
	// Draw the header buttons
	if (ImGui::CollapseButton(window->GetID("collapse"), ImVec2(node_min.x, node_min.y), nullptr)) { node_state.hidden = !node_state.hidden; }
	if (ImGui::CloseButton(window->GetID("close"), ImVec2(node_max.x - text_offset_x, node_min.y))) { delete_node = node_idx;  }
	window->DrawList->AddText(text_pos - ImVec2{ImGui::GetScrollX()-ImGui::GetWindowPos().x,0}, ImGui::GetColorU32(ImGuiCol_Text), node.id);
	
	// Draw edge buttons
	ImVec2 pos    = fgne_inouts_default(fgn_out, node_min, node_max, node.out_ct, node.out_ct+1) + ImVec2{ 0, -g.FontSize / 4.f };
	ImRect bounds = { pos, pos + ImVec2{ g.FontSize,g.FontSize } };
	ImGui::RenderArrow(window->DrawList, pos, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Right);
	if (ImGui::ButtonBehavior(bounds, ImGui::GetID("add_out"), &hovered, nullptr))
		fgne_select_out(graph, node_idx);
	if (hovered)
		window->DrawList->AddCircleFilled(bounds.GetCenter(), ImMax(2.0f, g.FontSize * 0.5f), bg_col, 12);

	pos    = fgne_inouts_default(fgn_in, node_min, node_max, node.in_ct, node.in_ct+1) + ImVec2{ -g.FontSize, -g.FontSize / 4.f };
	bounds = { pos, pos + ImVec2{ g.FontSize,g.FontSize } };
	ImGui::RenderArrow(window->DrawList, pos, ImGui::GetColorU32(ImGuiCol_Text), ImGuiDir_Left);
	if (ImGui::ButtonBehavior(bounds, ImGui::GetID("add_in"), &hovered, nullptr))
		fgne_select_in(graph, node_idx);
	if (hovered)
		window->DrawList->AddCircleFilled(bounds.GetCenter(), ImMax(2.0f, g.FontSize * 0.5f), bg_col, 12);
	
	// Drag behavior
	if (pressed)
		ImGui::ResetMouseDragDelta(0);
	if (held) {
		node.position[0] += ImGui::GetMouseDragDelta(0).x;
		node.position[1] += ImGui::GetMouseDragDelta(0).y;
		ImGui::ResetMouseDragDelta(0);
	}
	if (node.position[1] < frame_height)
		node.position[1] = frame_height;

	bool node_hovered = ImGui::ItemHoverable({node_min, node_max}, window->GetID("body"));

	ImGui::EndGroup();

	// Context menu
	if (// && 
		ImGui::IsMouseClicked(1)) {
		if (node_hovered)
			ImGui::OpenPopup("context_menu_node");
	}
}

///////////////////////////////////////////

void fgne_draw_edge(fgn_graph_t &graph, fgn_edge_t &edge, fgn_edge_idx edge_idx) {
	fgn_node_t    &start_n = fgn_graph_node_get(graph, edge.start);
	fgn_node_t    &end_n   = fgn_graph_node_get(graph, edge.end);
	editor_node_t &start   = node_state[edge.start];
	editor_node_t &end     = node_state[edge.end];
	int32_t start_i = 0, end_i = 0;
	for (int32_t i = 0; i < start_n.out_ct; i++) if (start_n.out_edges[i] == edge_idx) { start_i = i; break; }
	for (int32_t i = 0; i < end_n  .in_ct;  i++) if (end_n  .in_edges [i] == edge_idx) { end_i   = i; break; }
	ImVec2 p1 = fgne_inouts_default(fgn_out, start.node_min, start.node_max, start_i, start_n.out_ct+1);
	ImVec2 p2 = fgne_inouts_default(fgn_in,  end  .node_min, end  .node_max, end_i,   end_n  .in_ct +1);
	float dist = dist_to_line_sq(p1, p2, ImGui::GetMousePos());

	ImGui::GetWindowDrawList()->AddBezierCurve(p1, p1 + ImVec2{60, 0}, p2 - ImVec2{60, 0}, p2, dist < 30 * 30 ? ImGui::GetColorU32({ .7f,0,0,1 }) : ImGui::GetColorU32({ .5f,.5f,.5f,1 }), 1);
	/*ImGui::GetWindowDrawList()->AddLine(
		p1, p2, 
		dist < 30*30 ? ImGui::GetColorU32({ .7f,0,0,1 }) : ImGui::GetColorU32({ .5f,.5f,.5f,1 }));*/

	if (dist < 30*30) {
		ImVec2 size = ImVec2{ GImGui->FontSize / 2, GImGui->FontSize / 2 } + GImGui->Style.FramePadding;
		ImGui::SetCursorPos((p1 + p2) / 2);
		if (ImGui::CloseButton(ImGui::GetID("delete_edge"), (p1 + p2) / 2 - size)) {
			delete_edge = edge_idx;
		}
	}
}

///////////////////////////////////////////

void fgne_context_menu(fgn_graph_t &graph) {
	bool open_node_name = false;
	static ImVec2 context_pos = {};
	if (!ImGui::IsAnyItemHovered() && 
		ImGui::IsMouseClicked(1)) {
		ImGui::OpenPopup("context_menu");
		context_pos = ImGui::GetMousePos();
	}

	if (ImGui::BeginPopup("context_menu")) {
		if (ImGui::MenuItem("Add Node", "Ctrl+N", false, true)) {
			open_node_name = true;
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

	// Add node name dialog
	if (open_node_name) {
		ImGui::OpenPopup("Node's Id");
		open_node_name = false;
	}
	fgne_id_popup("Node's Id", &graph, [](void *data, const char *id, const char **out_err) {
		fgn_graph_t *graph = (fgn_graph_t*)data;
		if (fgn_graph_node_findid(*graph, id) != -1) {
			*out_err = "That id is already in use!";
			return false;
		}

		fgn_node_idx new_node = fgn_graph_node_add(*graph, id);
		fgn_node_t  &n        = fgn_graph_node_get(*graph, new_node);
		n.position[0] = context_pos.x;
		n.position[1] = context_pos.y;
		return true;
	});
}

///////////////////////////////////////////

void fgne_select_in(fgn_graph_t &graph, fgn_node_idx in_idx) {
	if (selected_out == in_idx) return;
	selected_in = in_idx;
	if (selected_in != -1 && selected_out != -1) {
		printf("Added edge!\n");
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

void fgne_id_popup(const char *title, void *data, bool (*create)(void *data, const char *id, const char **out_err)) {
	static bool open = true;
	if (ImGui::BeginPopupModal(title, nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
		static char id_text [128] = {};
		static const char *err_text = nullptr;

		if (open) {
			ImGui::SetKeyboardFocusHere();
			err_text = nullptr;
			id_text[0] = '\0';
			open = false;
		}

		bool confirm = ImGui::InputText("Id", id_text, 128, ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_EnterReturnsTrue);
		if (err_text != nullptr)
			ImGui::Text(err_text);
		if (ImGui::Button("Create") || confirm) {
			if (strcmp(id_text, "") == 0) {
				err_text = "Id can't be empty!";
			} else if (create(data, id_text, &err_text)) {
				ImGui::CloseCurrentPopup(); 
				open = true;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel")) {
			ImGui::CloseCurrentPopup();
			open = true;
		}
		ImGui::EndPopup();
	}
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

void fgne_meat_kvps(fgn_node_t &node) {
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