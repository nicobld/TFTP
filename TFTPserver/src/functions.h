#pragma once

#include <stdint.h>

#include "tftp.h"

void ctrlc();
void die(const char[]);
void error_and_die(const char[]);

int getcommand();
void sendERR(uint16_t error, char message[], struct client_info client_info);

Mode checkMode();
/* strtol with checking */
bool strtol_check(long* val, char* str);

/* Inserts blockno into buf */
void insert_blockno(uint16_t blockno);
/* Gets blockno from buf */
uint16_t get_blockno();
void insert_opcode(uint16_t opcode);
uint16_t get_opcode();
void insert_errorcode(uint16_t errorcode);
uint16_t get_errorcode();