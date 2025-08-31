# Makefile for BME69x i forced mode

API_LOCATION ?= ../..

C_SRCS += \
$(API_LOCATION)/bme69x.c \
../common/rpi-common.c \
./forced_mode.c

INCLUDEPATHS += \
$(API_LOCATION)  \
$(API_LOCATION)/examples/forced_mode \
$(API_LOCATION)/examples/common

# CFLAGS += -g -Wall -Wextra -Werror -I$(API_LOCATION) -I./
 CFLAGS += -g -fPIC -Wall -I$(API_LOCATION) -I./ -I $(API_LOCATION)/examples/common
CC = gcc

all: bme69x.o rpi-common.o forced_mode.o forced_mode

bme69x.o: $(API_LOCATION)/bme69x.c
	$(CC) $(CFLAGS) -c $< -o $@

rpi-common.o: ../common/rpi-common.c
	$(CC) $(CFLAGS)  -c $< -o $@

forced_mode.o: forced_mode.c
	$(CC) $(CFLAGS)  -c $< -o $@

forced_mode: bme69x.o forced_mode.o rpi-common.o
	$(CC) $(CFLAGS) $^ -o $@

