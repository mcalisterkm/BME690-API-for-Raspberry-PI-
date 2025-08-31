# Makefile for BME69x i forced mode

API_LOCATION ?= ../..

C_SRCS += \
$(API_LOCATION)/bme69x.c \
../common/rpi-common.c \
./parallel_mode.c

INCLUDEPATHS += \
$(API_LOCATION)  \
$(API_LOCATION)/examples/parallel_mode \
$(API_LOCATION)/examples/common

# CFLAGS += -g -Wall -Wextra -Werror -I$(API_LOCATION) -I./
 CFLAGS += -g -fPIC -Wall -I$(API_LOCATION) -I./ -I $(API_LOCATION)/examples/common
CC = gcc

all: bme69x.o rpi-common.o parallel_mode.o parallel_mode

bme69x.o: $(API_LOCATION)/bme69x.c
	$(CC) $(CFLAGS) -c $< -o $@

rpi-common.o: ../common/rpi-common.c
	$(CC) $(CFLAGS)  -c $< -o $@

parallel_mode.o: parallel_mode.c
	$(CC) $(CFLAGS)  -c $< -o $@

parallel_mode: bme69x.o parallel_mode.o rpi-common.o
	$(CC) $(CFLAGS) $^ -o $@

