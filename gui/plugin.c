/*
* This file is part of DrNokSnes
*
* Copyright (C) 2005 INdT - Instituto Nokia de Tecnologia
* http://www.indt.org/maemo
* Copyright (C) 2009 Javier S. Pedro <maemo@javispedro.com>
*
* This software is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1 of
* the License, or (at your option) any later version.
*
* This software is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this software; if not, write to the Free Software
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
* 02110-1301 USA
*
*/

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <startup_plugin.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <hildon/hildon-file-chooser-dialog.h>
#include <hildon/hildon-banner.h>

#include "../platform/hgw.h"
#include "plugin.h"

static GtkWidget * load_plugin(void);
static void unload_plugin(void);
static void write_config(void);
static GtkWidget ** load_menu(guint *);
static void update_menu(void);
static void plugin_callback(GtkWidget * menu_item, gpointer data);

GConfClient *gcc = NULL;
static GameStartupInfo gs;
GtkWidget *menu_items[2];

static StartupPluginInfo plugin_info = {
	load_plugin,
	unload_plugin,
	write_config,
	load_menu,
	update_menu,
	plugin_callback
};

STARTUP_INIT_PLUGIN(plugin_info, gs, FALSE, TRUE)

char * current_rom_file = 0;
static GtkLabel * rom_label;

static void set_rom(const char * rom_file)
{
	if (!rom_file) return;
	if (current_rom_file) g_free(current_rom_file);

	current_rom_file = g_strdup(rom_file);
	gtk_label_set_text(GTK_LABEL(rom_label), rom_file);

	game_state_update();
	save_clear();
}

static inline GtkWindow* get_parent_window() {
	return GTK_WINDOW(gs.ui->hildon_appview);
}

static void select_rom_callback(GtkWidget * button, gpointer data)
{
	GtkWidget * dialog;
	GtkFileFilter * filter;
	const gchar * current_filename = gtk_label_get_text(rom_label);
	gchar * filename = NULL;

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.smc");
	gtk_file_filter_add_pattern(filter, "*.sfc");
	gtk_file_filter_add_pattern(filter, "*.fig");

	dialog = hildon_file_chooser_dialog_new_with_properties(
		get_parent_window(),
		"action", GTK_FILE_CHOOSER_ACTION_OPEN, "filter", filter, NULL);

	if (current_filename && strlen(current_filename) > 1) {
		// By default open showing the last selected file
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), 
			current_filename);
	}

	gtk_widget_show_all(GTK_WIDGET(dialog));
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}

	gtk_widget_destroy(dialog);

	if (filename) {
		set_rom(filename);
		g_free(filename);
	}
}

static GtkWidget * load_plugin(void)
{
	g_type_init();
	gcc = gconf_client_get_default();

	GtkWidget* parent = gtk_vbox_new(FALSE, HILDON_MARGIN_DEFAULT);
	GtkWidget* parent_hbox = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
	GtkWidget* selectRomBtn = gtk_button_new_with_label("Select ROM...");
	rom_label = GTK_LABEL(gtk_label_new(NULL));
	
	gtk_widget_set_size_request(GTK_WIDGET(selectRomBtn),
								180, 50);

	g_signal_connect(G_OBJECT(selectRomBtn), "clicked",
					G_CALLBACK (select_rom_callback), NULL);

	gtk_box_pack_start(GTK_BOX(parent_hbox), selectRomBtn, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(parent_hbox), GTK_WIDGET(rom_label), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent), parent_hbox, FALSE, FALSE, 0);

	// Load current configuration from gconf
	set_rom(gconf_client_get_string(gcc, kGConfRomFile, NULL));

	return parent;
}

static void unload_plugin(void)
{
	if (current_rom_file) {
		g_free(current_rom_file);
		current_rom_file = 0;
	}
	game_state_clear();
	save_clear();
	g_object_unref(gcc);
}

static void write_config(void)
{
	if (current_rom_file) {
		gconf_client_set_string(gcc, kGConfRomFile, current_rom_file, NULL);
	} else {
		hildon_banner_show_information(GTK_WIDGET(get_parent_window()), NULL,
			"No ROM selected");
	}
}

static GtkWidget **load_menu(guint *nitems)
{
	*nitems = 0;
	
	return NULL;
}

static void update_menu(void)
{

}

static void plugin_callback(GtkWidget * menu_item, gpointer data)
{
	switch ((gint) data) {
		case 20:	// ME_GAME_OPEN
			save_load(get_parent_window());
			break;
		case 21:	// ME_GAME_SAVE
			save_save(get_parent_window());
			break;
		case 22:	// ME_GAME_SAVE_AS
			save_save_as(get_parent_window());
			break;
	}
}

