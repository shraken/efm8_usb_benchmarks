#ifndef  _MAIN_H_
#define  _MAIN_H_

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <c8051f3xx.h>
#include "init.h"

#define BUDDY_FW_INFO_SERIAL 0x12345678
#define BUDDY_FW_INFO_DATETIME 0x00000000

#define BUDDY_FW_FWREV_INFO_MAJOR 0
#define BUDDY_FW_FWREV_INFO_MINOR 5
#define BUDDY_FW_FWREV_INFO_TINY 0

#define BUDDY_FW_BOOTLREV_INFO_MAJOR 0
#define BUDDY_FW_BOOTLREV_INFO_MINOR 0
#define BUDDY_FW_BOOTLREV_INFO_TINY 0

void main(void);

#endif