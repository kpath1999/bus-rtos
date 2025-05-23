#
# Copyright (c) 2019 Nordic Semiconductor
#
# SPDX-License-Identifier: Apache-2.0, LicenseRef-BSD-5-Clause-Nordic

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(bus_rtos)

zephyr_library_sources(src/startup.c)
zephyr_library_sources(src/main.c)

zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_NRF_CLOUD src/assistance.c)
zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_SUPL src/assistance_supl.c)
zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL src/assistance_minimal.c)
zephyr_library_sources_ifdef(CONFIG_GNSS_SAMPLE_ASSISTANCE_MINIMAL src/mcc_location_table.c)

target_sources(app PRIVATE 
    src/main.c
    src/rtc.c
    src/accelerometer.c
)