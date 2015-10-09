#include <pebble.h>

#include "comms.h"

#include <stdbool.h>
#include <stddef.h>

#define BUFFER_SIZE (512)

#define DEFAULT_STRING "Press select to start Translating!"

static Window *window;
static TextLayer *q_text_layer;
static TextLayer *a_text_layer;

static DictationSession *session;
static char *dictation_result = NULL;

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  dictation_session_start(session);
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect frame = layer_get_frame(window_layer);


  q_text_layer = text_layer_create((GRect) {
    .origin = { .x = 10, .y = 10},
    .size = { .w = frame.size.w - 20, .h = 60 }
  });
  text_layer_set_text(q_text_layer, DEFAULT_STRING);
  text_layer_set_text_alignment(q_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(q_text_layer));

  a_text_layer = text_layer_create((GRect) {
    .origin = { .x = 10, .y = 80},
    .size = { .w = frame.size.w - 20, .h = frame.size.h - 80 }
  });
  text_layer_set_text_alignment(a_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(a_text_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(q_text_layer);
  text_layer_destroy(a_text_layer);

  free(dictation_result);
}

static const char *get_status_string(DictationSessionStatus status) {
  static const char *STATUS_MAP[] = {
    [DictationSessionStatusSuccess] = "Success.",
    [DictationSessionStatusFailureTranscriptionRejected] = NULL,
    [DictationSessionStatusFailureTranscriptionRejectedWithError] = NULL,
    [DictationSessionStatusFailureSystemAborted] = "Error overload",
    [DictationSessionStatusFailureNoSpeechDetected] = "I did not hear you speak!",
    [DictationSessionStatusFailureConnectivityError] = "Bluetooth or internet not connected",
    [DictationSessionStatusFailureDisabled] = "Transcription disabled for this user",
    [DictationSessionStatusFailureInternalError] = "Internal error.",
    [DictationSessionStatusFailureRecognizerError] = "I did not understand what you said",
  };

  if (status < ARRAY_LENGTH(STATUS_MAP)) {
    return STATUS_MAP[status];
  } else {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Unrecognized status code received!");
  }
  return NULL;
}

static void handle_dictation_result(DictationSession *session,
                                    DictationSessionStatus status, char *transcription,
                                    void *context) {
  if (status == DictationSessionStatusSuccess) {
    if (dictation_result) {
      free(dictation_result);
    }
    const char *preamble = "ENG: ";
    size_t len = strlen(transcription);
    dictation_result = malloc(len + strlen(preamble) + 1);
    strcpy(dictation_result, preamble);
    strcat(dictation_result, transcription);
    text_layer_set_text(q_text_layer, dictation_result);
    comms_send_request(transcription);
  } else {
    const char *status_text = get_status_string(status);
    if (status_text) {
      text_layer_set_text(q_text_layer, status_text);
    } else {
      text_layer_set_text(q_text_layer, DEFAULT_STRING);
    }
  }
}

void comms_handle_send_result(bool success) {
  if (success) {
    text_layer_set_text(a_text_layer, "Consulting the cloud...");
  } else {
    text_layer_set_text(a_text_layer, "I failed you. Please try again");
  }
}

void comms_handle_response(char *text) {
  size_t len = strlen(text);
  dictation_result = malloc(len + 6);
  strcpy(dictation_result, "FRA: ");
  strcat(dictation_result, text);
  text_layer_set_text(a_text_layer, dictation_result);
}

void comms_handle_response_failure(void) {
  text_layer_set_text(a_text_layer, "I failed you. Please try again");
}

static void init(void) {
  session = dictation_session_create(BUFFER_SIZE, handle_dictation_result, NULL);
  if (!session) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "No phone connected, platform is not supported or phone app does "
            "not support dictation APIs!");
  }
  dictation_session_enable_confirmation(session, false /* is_enabled */);

  comms_init();

  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
}

static void deinit(void) {
  dictation_session_destroy(session);

  window_destroy(window);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
