# Makefile for BME69x i forced mode

API_LOCATION ?= ../..

C_SRCS += \
$(API_LOCATION)/bme69x.c \
../common/rpi-common.c \
./sequential_mode.c

INCLUDEPATHS += \
$(API_LOCATION)  \
$(API_LOCATION)/examples/sequential_mode \
$(API_LOCATION)/examples/common

# CFLAGS += -g -Wall -Wextra -Werror -I$(API_LOCATION) -I./
 CFLAGS += -g -fPIC -Wall -I$(API_LOCATION) -I./ -I $(API_LOCATION)/examples/common
CC = gcc

all: bme69x.o rpi-common.o sequential_mode.o sequential_mode

bme69x.o: $(API_LOCATION)/bme69x.c
	$(CC) $(CFLAGS) -c $< -o $@

rpi-common.o: ../common/rpi-common.c
	$(CC) $(CFLAGS)  -c $< -o $@

sequential_mode.o: sequential_mode.c
	$(CC) $(CFLAGS)  -c $< -o $@

sequential_mode: bme69x.o sequential_mode.o rpi-common.o
	$(CC) $(CFLAGS) $^ -o $@

