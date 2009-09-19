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
#include <hildon/hildon-note.h>
#include <hildon/hildon-defines.h>

#if MAEMO_VERSION >= 5
#include <hildon/hildon-button.h>
#include <hildon/hildon-check-button.h>
#include <hildon/hildon-picker-button.h>
#include <hildon/hildon-touch-selector.h>
#include <hildon/hildon-gtk.h>
#else
#include <hildon/hildon-caption.h>
#endif

#include "../platform/hgw.h"
#include "plugin.h"

static GtkWidget * load_plugin(void);
static void unload_plugin(void);
static void write_config(void);
static GtkWidget ** load_menu(guint *);
static void update_menu(void);
static void plugin_callback(GtkWidget * menu_item, gpointer data);

GConfClient * gcc = NULL;
static GameStartupInfo gs;
static GtkWidget * menu_items[4];

static StartupPluginInfo plugin_info = {
	load_plugin,
	unload_plugin,
	write_config,
	load_menu,
	update_menu,
	plugin_callback
};

STARTUP_INIT_PLUGIN(plugin_info, gs, FALSE, TRUE)

gchar* current_rom_file = 0;
gboolean current_rom_file_exists = FALSE;

#if MAEMO_VERSION >= 5
static HildonButton* select_rom_btn;
static HildonCheckButton* audio_check;
static HildonPickerButton* framerate_picker;
static HildonCheckButton* display_fps_check;
static HildonCheckButton* turbo_check;
// speedhacks=no and accuracy=yes in fremantle
#else
static GtkLabel* rom_label;
static GtkCheckButton* audio_check;
static GtkCheckButton* turbo_check;
static GtkComboBox* framerate_combo;
static GtkCheckButton* accu_check;
static GtkCheckButton* display_fps_check;
static GtkComboBox* speedhacks_combo;
#endif

static void set_rom(const char * rom_file)
{
	if (current_rom_file) g_free(current_rom_file);
	if (!rom_file) {
		current_rom_file = NULL;
		return;
	}

	current_rom_file = g_strdup(rom_file);

	gchar * utf8_filename = g_filename_display_basename(rom_file);
#if MAEMO_VERSION >= 5
	hildon_button_set_value(select_rom_btn, utf8_filename);
#else
	gtk_label_set_text(GTK_LABEL(rom_label), utf8_filename);
#endif
	g_free(utf8_filename);

	current_rom_file_exists = g_file_test(current_rom_file,
		G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR);

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
	gchar * filename = NULL;

	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.smc");
	gtk_file_filter_add_pattern(filter, "*.sfc");
	gtk_file_filter_add_pattern(filter, "*.fig");
	gtk_file_filter_add_pattern(filter, "*.smc.gz");
	gtk_file_filter_add_pattern(filter, "*.sfc.gz");
	gtk_file_filter_add_pattern(filter, "*.fig.gz");

	dialog = hildon_file_chooser_dialog_new_with_properties(
		get_parent_window(),
		"action", GTK_FILE_CHOOSER_ACTION_OPEN,
		"local-only", TRUE,
		"filter", filter,
		NULL);
	hildon_file_chooser_dialog_set_show_upnp(HILDON_FILE_CHOOSER_DIALOG(dialog),
		FALSE);

	if (current_rom_file_exists) {
		// By default open showing the last selected file
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dialog), 
			current_rom_file);
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

static void controls_item_callback(GtkWidget * button, gpointer data)
{
	controls_setup();
	controls_dialog(get_parent_window());
}

static void about_item_callback(GtkWidget * button, gpointer data)
{
	about_dialog(get_parent_window());
}

static GtkWidget * load_plugin(void)
{
	g_type_init();
	gcc = gconf_client_get_default();

	GtkWidget* parent = gtk_vbox_new(FALSE, HILDON_MARGIN_DEFAULT);


/* Select ROM button */
#if MAEMO_VERSION >= 5
	select_rom_btn = HILDON_BUTTON(hildon_button_new_with_text(
		HILDON_SIZE_AUTO_WIDTH | HILDON_SIZE_FINGER_HEIGHT,
		HILDON_BUTTON_ARRANGEMENT_HORIZONTAL,
		"ROM",
		NULL));
	gtk_box_pack_start(GTK_BOX(parent), GTK_WIDGET(select_rom_btn), FALSE, FALSE, 0);
#else
{
	GtkWidget* rom_hbox = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
	GtkWidget* select_rom_btn = gtk_button_new_with_label("Select ROM...");
	gtk_widget_set_size_request(GTK_WIDGET(select_rom_btn),	180, 46);
	rom_label = GTK_LABEL(gtk_label_new(NULL));

	gtk_box_pack_start(GTK_BOX(rom_hbox), GTK_WIDGET(select_rom_btn), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(rom_hbox), GTK_WIDGET(rom_label), TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(parent), rom_hbox, FALSE, FALSE, 0);
}
#endif

/* First row of widgets */
#if MAEMO_VERSION >= 5
{
	GtkBox* opt_hbox1 = GTK_BOX(gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT));
	audio_check =
		HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_AUTO));
	gtk_button_set_label(GTK_BUTTON(audio_check), "Sound");

	framerate_picker = HILDON_PICKER_BUTTON(hildon_picker_button_new(
		HILDON_SIZE_AUTO, HILDON_BUTTON_ARRANGEMENT_HORIZONTAL));
	hildon_button_set_title(HILDON_BUTTON(framerate_picker), "Target framerate");

	HildonTouchSelector* framerate_sel =
		HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
	hildon_touch_selector_append_text(framerate_sel, "Auto");
	for (int i = 1; i < 10; i++) {
		gchar buffer[20];
		sprintf(buffer, "%d-%d", 50/i, 60/i);
		hildon_touch_selector_append_text(framerate_sel, buffer);
	}
	hildon_picker_button_set_selector(framerate_picker, framerate_sel);

	GtkBox* framerate_sel_box = GTK_BOX(gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT));

	display_fps_check =
		HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	gtk_button_set_label(GTK_BUTTON(display_fps_check),
		"Show while in game");
	turbo_check =
		HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	gtk_button_set_label(GTK_BUTTON(turbo_check),
		"Turbo mode");

	gtk_box_pack_start_defaults(framerate_sel_box, GTK_WIDGET(display_fps_check));
	gtk_box_pack_start_defaults(framerate_sel_box, GTK_WIDGET(turbo_check));
	gtk_box_pack_start(GTK_BOX(framerate_sel), GTK_WIDGET(framerate_sel_box), FALSE, FALSE, 0);
	gtk_widget_show_all(GTK_WIDGET(framerate_sel_box));

	gtk_box_pack_start_defaults(opt_hbox1, GTK_WIDGET(audio_check));
	gtk_box_pack_start_defaults(opt_hbox1, GTK_WIDGET(framerate_picker));
	gtk_box_pack_start(GTK_BOX(parent), GTK_WIDGET(opt_hbox1), FALSE, FALSE, 0);
}
#else
{
	GtkBox* opt_hbox1 = gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT);
	audio_check =
		GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Audio"));

	turbo_check =
		GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Turbo mode"));
	display_fps_check =
		GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Display framerate"));
	speedhacks_combo =
		GTK_COMBO_BOX(gtk_combo_box_new_text());

	gtk_box_pack_start(opt_hbox1, GTK_WIDGET(audio_check), FALSE, FALSE, 0);
	gtk_box_pack_start(opt_hbox1, GTK_WIDGET(display_fps_check), TRUE, FALSE, 0);
	gtk_box_pack_start(opt_hbox1, GTK_WIDGET(turbo_check), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), opt_hbox1, FALSE, FALSE, 0);
}
#endif

#if 0
	GtkBox* framerate_box = GTK_BOX(gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT));
	GtkWidget* framerate_label = gtk_label_new("Framerate:");
	gtk_box_pack_start(framerate_box, framerate_label, FALSE, FALSE, 0);
	gtk_box_pack_start(framerate_box, GTK_WIDGET(framerate_combo), FALSE, FALSE, 0);
#endif

/* Second row of widgets */
#if MAEMO_VERSION >= 5
#else
{
	GtkBox* opt_hbox2 = GTK_BOX(gtk_hbox_new(FALSE, HILDON_MARGIN_DEFAULT));

	accu_check =
		GTK_CHECK_BUTTON(gtk_check_button_new_with_label("Accurate graphics"));

	framerate_combo =
		GTK_COMBO_BOX(gtk_combo_box_new_text());
	GtkWidget* framerate_box = hildon_caption_new(NULL, "Framerate:",
		GTK_WIDGET(framerate_combo), NULL, HILDON_CAPTION_OPTIONAL);

	gtk_combo_box_append_text(framerate_combo, "Auto");
	for (int i = 1; i < 10; i++) {
		gchar buffer[20];
		sprintf(buffer, "%d-%d", 50/i, 60/i);
		gtk_combo_box_append_text(framerate_combo, buffer);
	}
	gtk_combo_box_append_text(speedhacks_combo, "No speedhacks");
	gtk_combo_box_append_text(speedhacks_combo, "Safe hacks only");
	gtk_combo_box_append_text(speedhacks_combo, "All speedhacks");

	gtk_box_pack_start(opt_hbox2, GTK_WIDGET(accu_check), FALSE, FALSE, 0);
	gtk_box_pack_start(opt_hbox2, GTK_WIDGET(framerate_box), TRUE, FALSE, 0);
	gtk_box_pack_start(opt_hbox2, GTK_WIDGET(speedhacks_combo), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(parent), GTK_WIDGET(opt_hbox2), FALSE, FALSE, 0);
}
#endif

/* Load current configuration from GConf */
#if MAEMO_VERSION >= 5
	hildon_check_button_set_active(audio_check,
		!gconf_client_get_bool(gcc, kGConfDisableAudio, NULL));
	hildon_picker_button_set_active(framerate_picker,
		gconf_client_get_int(gcc, kGConfFrameskip, NULL));
	hildon_check_button_set_active(turbo_check,
		gconf_client_get_bool(gcc, kGConfTurboMode, NULL));
	hildon_check_button_set_active(display_fps_check,
		gconf_client_get_int(gcc, kGConfDisplayFramerate, NULL));
#else
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audio_check),
		!gconf_client_get_bool(gcc, kGConfDisableAudio, NULL));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(turbo_check),
		gconf_client_get_bool(gcc, kGConfTurboMode, NULL));
	gtk_combo_box_set_active(framerate_combo,
		gconf_client_get_int(gcc, kGConfFrameskip, NULL));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(accu_check),
		gconf_client_get_bool(gcc, kGConfTransparency, NULL));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(display_fps_check),
		gconf_client_get_int(gcc, kGConfDisplayFramerate, NULL));
	gtk_combo_box_set_active(speedhacks_combo,
		gconf_client_get_int(gcc, kGConfSpeedhacks, NULL));
#endif

	set_rom(gconf_client_get_string(gcc, kGConfRomFile, NULL));

	// Connect signals
	g_signal_connect(G_OBJECT(select_rom_btn), "clicked",
					G_CALLBACK(select_rom_callback), NULL);
printf("ui done\n");
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
/* Write current settings to gconf */
#if MAEMO_VERSION >= 5
	gconf_client_set_bool(gcc, kGConfDisableAudio,
		!hildon_check_button_get_active(audio_check), NULL);
	gconf_client_set_int(gcc, kGConfFrameskip,
		hildon_picker_button_get_active(framerate_picker), NULL);
	gconf_client_set_bool(gcc, kGConfDisplayFramerate,
		hildon_check_button_get_active(display_fps_check), NULL);
	gconf_client_set_bool(gcc, kGConfTurboMode,
		hildon_check_button_get_active(turbo_check), NULL);

	// For now, transparencies are always enabled in Fremantle
	gconf_client_set_bool(gcc, kGConfTransparency, TRUE, NULL);
	// Speedhacks always disabled
	gconf_client_set_int(gcc, kGConfSpeedhacks,	FALSE, NULL);
#else
	gconf_client_set_bool(gcc, kGConfDisableAudio,
		!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(audio_check)), NULL);
	gconf_client_set_bool(gcc, kGConfTurboMode,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(turbo_check)), NULL);
	gconf_client_set_int(gcc, kGConfFrameskip,
		gtk_combo_box_get_active(framerate_combo), NULL);
	gconf_client_set_bool(gcc, kGConfTransparency,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(accu_check)), NULL);
	gconf_client_set_bool(gcc, kGConfDisplayFramerate,
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(display_fps_check)), NULL);
	gconf_client_set_int(gcc, kGConfSpeedhacks,
		gtk_combo_box_get_active(speedhacks_combo), NULL);
#endif

	if (current_rom_file) {
		gconf_client_set_string(gcc, kGConfRomFile, current_rom_file, NULL);
	}

	controls_setup();
}

static GtkWidget **load_menu(guint *nitems)
{
#if MAEMO_VERSION >= 5
	const HildonSizeType button_size =
		HILDON_SIZE_FINGER_HEIGHT | HILDON_SIZE_AUTO_WIDTH;
	menu_items[0] = hildon_gtk_button_new(button_size);
	gtk_button_set_label(GTK_BUTTON(menu_items[0]), "Placeholder");
	menu_items[1] = hildon_gtk_button_new(button_size);
	gtk_button_set_label(GTK_BUTTON(menu_items[1]), "Controls…");
	menu_items[2] = hildon_gtk_button_new(button_size);
	gtk_button_set_label(GTK_BUTTON(menu_items[2]), "About…");
	*nitems = 3;

	g_signal_connect(G_OBJECT(menu_items[1]), "clicked",
					G_CALLBACK(controls_item_callback), NULL);
	g_signal_connect(G_OBJECT(menu_items[2]), "clicked",
					G_CALLBACK(about_item_callback), NULL);
#else
	menu_items[0] = gtk_menu_item_new_with_label("Settings");
	menu_items[1] = gtk_menu_item_new_with_label("About…");
	*nitems = 2;

	GtkMenu* settings_menu = GTK_MENU(gtk_menu_new());
	GtkMenuItem* controls_item =
		GTK_MENU_ITEM(gtk_menu_item_new_with_label("Controls…"));
	
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_items[0]),
		GTK_WIDGET(settings_menu));
	gtk_menu_append(GTK_MENU(settings_menu), GTK_WIDGET(controls_item));

	g_signal_connect(G_OBJECT(controls_item), "activate",
					G_CALLBACK(controls_item_callback), NULL);
	g_signal_connect(G_OBJECT(menu_items[1]), "activate",
					G_CALLBACK(about_item_callback), NULL);
#endif

	return menu_items;
}

static void update_menu(void)
{
	// Nothing to update in the current menu
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
		case 30:	// MA_GAME_PLAYING_START
			if (!menu_item) {
				// Avoid duplicate message
				break;
			}
			if (!current_rom_file) {
				GtkWidget* note = hildon_note_new_information(get_parent_window(),
					"No ROM selected");
				gtk_dialog_run(GTK_DIALOG(note));
				gtk_widget_destroy(note);
			} else if (!current_rom_file_exists) {
				GtkWidget* note = hildon_note_new_information(get_parent_window(),
					"ROM file does not exist");
				gtk_dialog_run(GTK_DIALOG(note));
				gtk_widget_destroy(note);
			}
			break;
	}
}

