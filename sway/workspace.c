#include <stdlib.h>
#include <stdbool.h>
#include <wlc/wlc.h>
#include "workspace.h"
#include "layout.h"
#include "list.h"
#include "log.h"
#include "container.h"
#include "handlers.h"
#include "config.h"
#include "stringop.h"

swayc_t *active_workspace = NULL;

char *workspace_next_name(void) {
	sway_log(L_DEBUG, "Workspace: Generating new name");
	int i;
	int l = 1;
	// Scan all workspace bindings to find the next available workspace name,
	// if none are found/available then default to a number
	struct sway_mode *mode = config->current_mode;

	for (i = 0; i < mode->bindings->length; ++i) {
		struct sway_binding *binding = mode->bindings->items[i];
		const char* command = binding->command;
		list_t *args = split_string(command, " ");

		if (strcmp("workspace", args->items[0]) == 0 && args->length > 1) {
			sway_log(L_DEBUG, "Got valid workspace command for target: '%s'", args->items[1]);
			char* target = malloc(strlen(args->items[1]) + 1);
			strcpy(target, args->items[1]);
			while (*target == ' ' || *target == '\t')
				target++; 

			// Make sure that the command references an actual workspace
			// not a command about workspaces
			if (strcmp(target, "next") == 0 ||
				strcmp(target, "prev") == 0 ||
				strcmp(target, "next_on_output") == 0 ||
				strcmp(target, "prev_on_output") == 0 ||
				strcmp(target, "number") == 0 ||
				strcmp(target, "back_and_forth") == 0 ||
				strcmp(target, "current") == 0)
				continue;
		   
			//Make sure that the workspace doesn't already exist 
			if (workspace_find_by_name(target)) {
			   continue; 
			}

			list_free(args);

			sway_log(L_DEBUG, "Workspace: Found free name %s", target);
			return target;
		}
	}
	// As a fall back, get the current number of active workspaces
	// and return that + 1 for the next workspace's name
	int ws_num = root_container.children->length;
	if (ws_num >= 10) {
		l = 2;
	} else if (ws_num >= 100) {
		l = 3;
	}
	char *name = malloc(l + 1);
	sprintf(name, "%d", ws_num++);
	return name;
}

swayc_t *workspace_create(const char* name) {
	swayc_t *parent = get_focused_container(&root_container);
	while (parent->type != C_OUTPUT) {
		parent = parent->parent;
	}
	return new_workspace(parent, name);
}

bool workspace_by_name(swayc_t *view, void *data) {
	return (view->type == C_WORKSPACE) && 
		   (strcasecmp(view->name, (char *) data) == 0);
}

void set_mask(swayc_t *view, void *data) {
	uint32_t *p = data;

	if (view->type == C_VIEW) {
		wlc_view_set_mask(view->handle, *p);
	}
	view->visible = (*p == 2);
}

swayc_t *workspace_find_by_name(const char* name) {
	return find_container(&root_container, workspace_by_name, (void *) name);
}

void workspace_switch(swayc_t *workspace) {
	if (workspace != active_workspace && active_workspace) {
		sway_log(L_DEBUG, "workspace: changing from '%s' to '%s'", active_workspace->name, workspace->name);
		uint32_t mask = 1;
		// set all c_views in the old workspace to the invisible mask
		container_map(active_workspace, set_mask, &mask);

		// and c_views in the new workspace to the visible mask
		mask = 2;
		container_map(workspace, set_mask, &mask);

		wlc_output_set_mask(wlc_get_focused_output(), 2);
		unfocus_all(&root_container);
		focus_view(workspace);
		destroy_workspace(active_workspace);
	}
	active_workspace = workspace;
}
