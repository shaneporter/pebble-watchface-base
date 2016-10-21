#include <pebble.h>

// static variables:
static Window *s_main_window;
static TextLayer *s_time_layer;
static Layer *s_canvas, *s_battery_layer, *s_bluetooth_layer;
static int s_battery_level;

#define INSET 5

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  static char s_day_buffer[4];
  static char s_date_buffer[4];
  const char* const days[] = { "SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT" };

  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
                                          "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
  
  strcpy(s_day_buffer, days[tick_time->tm_wday]);  
  strftime(s_date_buffer, sizeof(s_date_buffer), "%d", tick_time);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void battery_callback(BatteryChargeState state) {
  // Record the new battery level
  s_battery_level = state.charge_percent;
  
  layer_mark_dirty(s_battery_layer);
}

static void bluetooth_callback(bool connected) {

  //APP_LOG(APP_LOG_LEVEL_DEBUG, "Got a bluetooth status update");

  layer_set_hidden((Layer*)s_bluetooth_layer, !connected);

  if(!connected) {
    vibes_double_pulse();
  }
}

static void layer_update_proc(Layer *layer, GContext *ctx) {
  //GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorPictonBlue);
  graphics_fill_circle(ctx, GPoint(89, 89), 75);

  // lines:
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_context_set_stroke_width(ctx, 1);

  graphics_draw_line(ctx, GPoint(33, 100), GPoint(147, 100));
  graphics_draw_line(ctx, GPoint(89, 100), GPoint(89, 127));
}

static void battery_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // battery is circle arc
  GRect frame = grect_inset(bounds, GEdgeInsets(1 * INSET));
  graphics_context_set_fill_color(ctx, GColorChromeYellow);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, INSET, 0, DEG_TO_TRIGANGLE((int)(float)(((float)s_battery_level / 100.0F) * 360)));
}

static void bluetooth_layer_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);

  // bluetooth is circle arc
  GRect frame = grect_inset(bounds, GEdgeInsets(3 * INSET));
  graphics_context_set_fill_color(ctx, GColorBlueMoon);
  graphics_fill_radial(ctx, frame, GOvalScaleModeFitCircle, INSET, 0, TRIG_MAX_ANGLE);
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(
      GRect(0, 53, bounds.size.w, 50));

  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorOxfordBlue);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

  s_canvas = layer_create(bounds);
  s_battery_layer = layer_create(bounds);
  s_bluetooth_layer = layer_create(bounds);

  layer_set_update_proc(s_canvas, layer_update_proc);
  layer_set_update_proc(s_battery_layer, battery_layer_update_proc);
  layer_set_update_proc(s_bluetooth_layer, bluetooth_layer_update_proc);

  layer_add_child(window_layer, s_canvas);
  layer_add_child(window_layer, s_battery_layer);
  layer_add_child(window_layer, s_bluetooth_layer);

  // Add it as a child layer to the Window's root layer
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
}

static void main_window_unload(Window *window) {
  // Destroy canvas, bluetooth and battery:
  layer_destroy(s_bluetooth_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_canvas);
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register for Bluetooth connection updates
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = bluetooth_callback
  });

  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // battery state:
  battery_state_service_subscribe(battery_callback);

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // make sure the time is displayed from the start:
  update_time();

  // Ensure battery level is displayed from the start
  battery_callback(battery_state_service_peek());

  // bluetooth state:
  bluetooth_callback(connection_service_peek_pebble_app_connection());
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
