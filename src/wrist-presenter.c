#include <pebble.h>

static Window *window;
static TextLayer *text_layer;
static TextLayer *current_slide_text_layer;
static TextLayer *total_slide_text_layer;

static AppSync sync;
static uint8_t sync_buffer[64];

enum AppSyncKey{
    KEY_BUTTON_PRESS = 0x0,     // TUPLE_INT
    KEY_SLIDE_CURRENT = 0x1,    // TUPLE_INT
    KEY_SLIDE_TOTAL = 0x2,      // TUPLE_INT
};

static bool mobile_app_connected = false;

//
// AppSync
//
static void sync_error_callback(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {

  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
  mobile_app_connected = false;
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(text_layer, "Launch phone app");
}

static void sync_tuple_changed_callback(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
  switch (key) {
    case KEY_BUTTON_PRESS:
      break;

    case KEY_SLIDE_CURRENT:
      text_layer_set_text(current_slide_text_layer, new_tuple->value->cstring);
      if ((mobile_app_connected == false) && (new_tuple->length > 1)) {
        mobile_app_connected = true;
        text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
        text_layer_set_text(text_layer, "of");
      }
      break;

    case KEY_SLIDE_TOTAL:
      text_layer_set_text(total_slide_text_layer, new_tuple->value->cstring);
      if ((mobile_app_connected == false) && (new_tuple->length > 1)) {
        mobile_app_connected = true;
        text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_LIGHT));
        text_layer_set_text(text_layer, "of");
      }
      break;

    default:
      APP_LOG(APP_LOG_LEVEL_DEBUG, "Unrecognized app sync key: %ld", key);
      return;
  }
}

static void next_slide(void) {

  Tuplet value = TupletInteger(KEY_BUTTON_PRESS, 3);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void previous_slide(void) {

  Tuplet value = TupletInteger(KEY_BUTTON_PRESS, 1);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

static void update(void) {

  Tuplet value = TupletInteger(KEY_BUTTON_PRESS, 2);

  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  if (iter == NULL) {
    return;
  }

  dict_write_tuplet(iter, &value);
  dict_write_end(iter);

  app_message_outbox_send();
}

//
// Click handlers
//
static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Select Button Pressed");
  update();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Up Button Pressed");
  previous_slide();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Down Button Pressed");
  next_slide();
}

//
// Setup and destroy
//
static void click_config_provider(void *context) {

  window_single_repeating_click_subscribe(BUTTON_ID_SELECT, 250, select_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_UP, 250, up_click_handler);
  window_single_repeating_click_subscribe(BUTTON_ID_DOWN, 250, down_click_handler);
}

static void window_load(Window *window) {

  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  current_slide_text_layer = text_layer_create((GRect) { .origin = { 0, 0 }, .size = { bounds.size.w, 50 } });
  text_layer_set_font(current_slide_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(current_slide_text_layer, "");
  text_layer_set_text_alignment(current_slide_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(current_slide_text_layer));

  text_layer = text_layer_create((GRect) { .origin = { 0, 50 }, .size = { bounds.size.w, 50 } });
  text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
  text_layer_set_text(text_layer, "Launch phone app");
  text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_layer));

  total_slide_text_layer = text_layer_create((GRect) { .origin = { 0, 100 }, .size = { bounds.size.w, 50 } });
  text_layer_set_font(total_slide_text_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text(total_slide_text_layer, "");
  text_layer_set_text_alignment(total_slide_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(total_slide_text_layer));

  Tuplet initial_values[] = {
    TupletInteger(KEY_BUTTON_PRESS, (uint8_t)2),
    TupletCString(KEY_SLIDE_CURRENT, ""),
    TupletCString(KEY_SLIDE_TOTAL, ""),
  };
  
  app_sync_init(&sync, sync_buffer, sizeof(sync_buffer), initial_values, ARRAY_LENGTH(initial_values),
                  sync_tuple_changed_callback, sync_error_callback, NULL);

  update();
}

static void window_unload(Window *window) {

  app_sync_deinit(&sync);
  text_layer_destroy(current_slide_text_layer);
  text_layer_destroy(text_layer);
  text_layer_destroy(total_slide_text_layer);
}

static void init(void) {

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });

  const int inbound_size = 64;
  const int outbound_size = 64;
  app_message_open(inbound_size, outbound_size);

  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {

  window_destroy(window);
}

int main(void) {

  init();
  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);
  app_event_loop();
  deinit();
}
