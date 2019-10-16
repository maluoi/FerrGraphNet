#pragma once

#include "ferr_graphnet.h"

void fgn_editor_init();
void fgn_editor_shutdown();

void fgn_editor_draw(fgn_library_t &lib);
void fgn_editor_draw(fgn_graph_t &graph);