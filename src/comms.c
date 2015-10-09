#include "comms.h"

#include <pebble.h>
#include <stdint.h>

enum {
  KEY_REQUEST,
  KEY_RESPONSE_TEXT,
  KEY_RESPONSE_SUCCESS
};

static uint32_t s_fail_count;

static void retry_timeout_cb(void *data) {
  app_message_outbox_send();
}

static void handle_msg_in_receive(DictionaryIterator *iterator, void *context) {
  bool success = false;
  Tuple *t = dict_find(iterator, KEY_RESPONSE_SUCCESS);
  if (t) {
    success = (bool)t->value->int8;
  }
  if (!success) {
    comms_handle_response_failure();
    return;
  }

  char *text = NULL;
  t = dict_find(iterator, KEY_RESPONSE_TEXT);
  if (t) {
    text = t->value->cstring;
  }
  comms_handle_response(text);
}

static void handle_msg_in_dropped(AppMessageResult reason, void *context) {
  comms_handle_response_failure();
}

static void handle_msg_out_success(DictionaryIterator *iterator, void *context) {
  comms_handle_send_result(true);
}

static void handle_msg_out_failed(DictionaryIterator *iterator, AppMessageResult reason,
                                  void *context) {
  s_fail_count++;
  if (s_fail_count < 3) {
    app_timer_register(200, retry_timeout_cb, NULL);
  } else {
    comms_handle_send_result(false);
  }
}

bool comms_send_request(char *text) {
  s_fail_count = 0;
  DictionaryIterator *dict;
  app_message_outbox_begin(&dict);
  if (!dict) {
    return false;
  }
  dict_write_cstring(dict, KEY_REQUEST, text);
  return app_message_outbox_send() == APP_MSG_OK;
}

void comms_init(void) {
  app_message_register_inbox_received(handle_msg_in_receive);
  app_message_register_inbox_dropped(handle_msg_in_dropped);
  app_message_register_outbox_sent(handle_msg_out_success);
  app_message_register_outbox_failed(handle_msg_out_failed);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}


