#pragma once

#include <stdint.h>
#include <stdbool.h>

void comms_init(void);

bool comms_send_request(char *text);

void comms_handle_send_result(bool success);

void comms_handle_response(char *text);

void comms_handle_response_failure(void);

