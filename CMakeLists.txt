# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(PVE_VIEW)

# Nastavení partition table
idf_build_set_property(PARTITION_TABLE_FILE "${CMAKE_SOURCE_DIR}/partition_table/partition_table.csv")

# Kontrola cesty k partition table
message(STATUS "Using partition table file: ${PARTITION_TABLE_FILE}")

# Vytvoření SPIFFS partition obrazu
spiffs_create_partition_image(spiffs ${CMAKE_SOURCE_DIR}/data FLASH_IN_PROJECT)

