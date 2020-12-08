# Race Capture Firmware
#
# Copyright (C) 2016 Autosport Labs
#
# This file is part of the Race Capture firmware suite
#
# This is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This software is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
#
# See the GNU General Public License for more details. You should
# have received a copy of the GNU General Public License along with
# this code. If not, see <http://www.gnu.org/licenses/>.

#-----Macros---------------------------------
NAME=rc-log-sim

BIN := rc-log-sim
BUILD_BIN_DIR = $(BUILD_DIR)/bin
BUILD_DIR := build
CAN_OBD2_DIR := can_obd2
COMMAND_SRC = $(RCP_SRC)/command
DEMO_SRC := src
FREE_RTOS_KERNEL_DIR = $(RCP_TEST)/FreeRTOS_Kernel
GPS_DIR := gps
LAP_STATS_DIR:=lap_stats
MK2_SRC=$(RCP_BASE)/platform/mk2
MOCK_DIR=$(RCP_TEST)/logger_mock
RCP_BASE:=submodules/firmware
RCP_INC=$(RCP_BASE)/include
RCP_SRC=$(RCP_BASE)/src
RCP_TEST=$(RCP_BASE)/test
UTIL_DIR:=util

INCLUDES = \
-I. \
-I./include \
-I$(RCP_BASE) \
-I$(RCP_BASE)/logger \
-I$(MK2_SRC)/hal/fat_sd_stm32/fatfs \
-I$(RCP_INC) \
-I$(RCP_INC)/logging \
-I$(RCP_INC)/gps \
-I$(RCP_INC)/gsm \
-I$(RCP_INC)/jsmn \
-I$(RCP_INC)/api \
-I$(RCP_INC)/filter \
-I$(RCP_INC)/logger \
-I$(RCP_INC)/channels \
-I$(RCP_INC)/virtual_channel \
-I$(RCP_INC)/tracks \
-I$(RCP_INC)/tasks \
-I$(RCP_INC)/messaging \
-I$(RCP_INC)/modem \
-I$(RCP_INC)/lua \
-I$(RCP_INC)/command \
-I$(RCP_INC)/predictive_timer \
-I$(RCP_INC)/auto_config \
-I$(RCP_INC)/util \
-I$(RCP_INC)/serial \
-I$(RCP_INC)/devices \
-I$(RCP_INC)/drivers \
-I$(RCP_INC) \
-I$(RCP_INC)/usart \
-I$(RCP_INC)/usb_comm \
-I$(RCP_INC)/imu \
-I$(RCP_INC)/ADC \
-I$(RCP_INC)/timer \
-I$(RCP_INC)/PWM \
-I$(RCP_INC)/LED \
-I$(RCP_INC)/cpu \
-I$(RCP_INC)/CAN \
-I$(RCP_INC)/OBD2 \
-I$(RCP_INC)/GPIO \
-I$(RCP_INC)/watchdog \
-I$(RCP_INC)/memory \
-I$(RCP_INC)/sdcard \
-I$(RCP_INC)/system \
-I$(RCP_INC)/lap_stats \
-I$(RCP_INC)/units \
-I$(MOCK_DIR) \
-I$(GPS_DIR) \
-I$(CAN_OBD2_DIR) \
-I$(LAP_STATS_DIR) \
-I$(FREE_RTOS_KERNEL_DIR)/ \
-I$(FREE_RTOS_KERNEL_DIR)/include \
-I$(FREE_RTOS_KERNEL_DIR)/include_testing \
-I$(UTIL_DIR) \
-I$(RCP_SRC) \
-I$(RCP_SRC)/devices \
-I$(RCP_SRC)/lap_stats \
-I$(RCP_SRC)/logger \
-I$(RCP_SRC)/modem \
-I$(RCP_TEST)/ \

#-----File Dependencies----------------------

DEMO_FILES = \
$(DEMO_SRC)/rc-log-sim.c

FIRMWARE_FILES = \
$(FREE_RTOS_KERNEL_DIR)/stubs/task.c \
$(MOCK_DIR)/LED_device_mock.c \
$(RCP_SRC)/LED/led.c \
$(RCP_SRC)/gps/dateTime.c \
$(RCP_SRC)/gps/geopoint.c \
$(RCP_SRC)/gps/gps.c \
$(RCP_SRC)/predictive_timer/predictive_timer_2.c \
$(RCP_SRC)/util/convert.c \
$(RCP_TEST)/mock_gps_device.c \

OBJ_TEST := $(addprefix build/, $(addsuffix .o, $(basename $(FIRMWARE_FILES) $(DEMO_FILES))))

# set up compiler and options
CC := gcc

# Disable Opts during test builds since we have hit GCC Bugzilla – Bug 56689
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=56689 on our official build
# system.
#
# [jenkins@ip-172-30-0-247 workspace]$ gcc --version
# gcc (GCC) 4.8.5 20150623 (Red Hat 4.8.5-4)
#
# Should remove this once we are on a newer GCC.
#
CFLAGS := -O0 -DRCP_TESTING $(INCLUDES)

#-----Suffix Rules---------------------------
# set up C++ suffixes and relationship between .cc and .o files

dir_guard=@mkdir -p $(@D)

build/%.o: %.c
	$(dir_guard)
	$(CCACHE) $(CC) $(CFLAGS) -c -D_RCP_BASE_FILE_="\"$(notdir $<): \"" $< -o $@

all: $(BUILD_BIN_DIR)/rc-log-sim

$(BUILD_BIN_DIR)/rc-log-sim: $(OBJ_TEST)
	$(dir_guard)
	$(CC) $(CFLAGS) -o build/bin/$(NAME) $(OBJ_TEST) -lm

clean:
	rm -rf build

.PHONY: all clean
