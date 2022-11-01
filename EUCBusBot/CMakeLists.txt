# CMakeList.txt: CMake-Projekt für "EUCBusBot". Schließen Sie die Quelle ein, und definieren Sie
# projektspezifische Logik hier.
#
cmake_minimum_required(VERSION 3.12)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# Fügen Sie der ausführbaren Datei dieses Projekts eine Quelle hinzu.
add_executable(EUCBusBot "EUCBusBot.cpp" "Bot.cpp" "ConfigManager.cpp" "BusReport.h" "BusReport.cpp" "ReportRecord.h" "ReportRecord.cpp" "Utility.h" "Driver.h" "Driver.cpp" "Utility.cpp")

add_subdirectory(libs/DPP)

set_property(TARGET EUCBusBot PROPERTY CXX_STANDARD 20)

# TODO: Fügen Sie bei Bedarf Tests hinzu, und installieren Sie Ziele.

find_package(MySQLConnectorCPP REQUIRED)

# Linking libraries
target_link_libraries(${PROJECT_NAME}
	dpp
	${MYSQLCONNECTORCPP_LIBRARY}
)
	 
# Specify includes
target_include_directories(${PROJECT_NAME} PRIVATE
	libs/DPP/include
	${MYSQLCONNECTORCPP_INCLUDE_DIR}
)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")