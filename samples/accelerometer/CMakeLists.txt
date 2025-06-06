#
# Copyright (c) 2019-2022 Actinius
#
# SPDX-License-Identifier: BSD-3-Clause
#

cmake_minimum_required(VERSION 3.8.2)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(accelerometer_sample)

target_sources(app PRIVATE src/main.c)
target_sources(app PRIVATE src/accelerometer.c)
zephyr_include_directories(src)
