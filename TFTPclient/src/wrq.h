#pragma once
#include <stdbool.h>

/* Sends a WRQ and takes care of the next ACK or OACK */
int send_wrq();
bool handle_wrq();