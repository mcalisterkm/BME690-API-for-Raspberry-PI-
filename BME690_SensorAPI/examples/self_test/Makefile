# Makefile for BME69x i forced mode

API_LOCATION ?= ../..

C_SRCS += \
$(API_LOCATION)/bme69x.c \
../common/rpi-common.c \
./self_test.c

INCLUDEPATHS += \
$(API_LOCATION)  \
$(API_LOCATION)/examples/self_test \
$(API_LOCATION)/examples/common

# CFLAGS += -g -Wall -Wextra -Werror -I$(API_LOCATION) -I./
 CFLAGS += -g -fPIC -Wall -I$(API_LOCATION) -I./ -I $(API_LOCATION)/examples/common
CC = gcc

all: bme69x.o rpi-common.o self_test.o self_test

bme69x.o: $(API_LOCATION)/bme69x.c
	$(CC) $(CFLAGS) -c $< -o $@

rpi-common.o: ../common/rpi-common.c
	$(CC) $(CFLAGS)  -c $< -o $@

self_test.o: self_test.c
	$(CC) $(CFLAGS)  -c $< -o $@

self_test: bme69x.o self_test.o rpi-common.o
	$(CC) $(CFLAGS) $^ -o $@

