#
# Copyright (c) 2019 Lexmark International, Inc.
# SPDX-License-Identifier: See LICENSE.lxk
#
zephyr_library()

zephyr_library_sources(packet.c trace.c word.c gpio.c hwtrace.c uart.c)
