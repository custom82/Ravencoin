cmake_minimum_required(VERSION 3.20)

# Imposta la policy per sopprimere il warning
cmake_policy(SET CMP0167 NEW)

project(Ravencoin
    VERSION 4.6.0
    LANGUAGES C CXX
)

# Aggiungi questa riga per risolvere il warning di boost::bind
add_compile_definitions(BOOST_BIND_GLOBAL_PLACEHOLDERS)

# ======================================
# Opzioni di build
# ======================================
option(BUILD_RAVENCOIND   "Build ravend (daemon)"             ON)
option(BUILD_RAVENCOIN_CLI "Build raven-cli"                  ON)
option(BUILD_RAVENCOIN_TX  "Build raven-tx"                   ON)
option(BUILD_RAVENCOIN_QT  "Build raven-qt (Qt GUI)"          ON)
option(ENABLE_WALLET       "Enable wallet functionality"      ON)
option(ENABLE_ZMQ          "Enable ZMQ notifications"         ON)
option(ENABLE_UPNP         "Enable UPnP support"              ON)
option(USE_QRCODE          "Enable QRCode support"            ON)

# ======================================
# Compilatore
# ======================================
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Version info per clientversion.h
add_definitions(
    -DCLIENT_VERSION_MAJOR=4
    -DCLIENT_VERSION_MINOR=6
    -DCLIENT_VERSION_REVISION=0
    -DCLIENT_VERSION_BUILD=0
    -DCLIENT_VERSION_IS_RELEASE=0
    -DCOPYRIGHT_YEAR=2024
    -DCOPYRIGHT_HOLDERS="The %s developers"
    -DCOPYRIGHT_HOLDERS_SUBSTITUTION="Ravencoin"
    -DPACKAGE_NAME="Ravencoin"
)


# ======================================
# Dipendenze di base
# ======================================
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)

# pkg-config per varie librerie
find_package(PkgConfig REQUIRED)

# LevelDB - preferisci config CMake, fallback a pkg-config
find_package(leveldb CONFIG QUIET)

if (leveldb_FOUND)
    message(STATUS "LevelDB trovato con successo via CMake config.")
    if (TARGET leveldb::leveldb)
        set(leveldb_LIBRARIES leveldb::leveldb)
    endif()
    if (NOT leveldb_INCLUDE_DIR AND DEFINED leveldb_INCLUDE_DIRS)
        set(leveldb_INCLUDE_DIR ${leveldb_INCLUDE_DIRS})
    endif()
else()
    pkg_check_modules(LEVELDB REQUIRED leveldb)
    set(leveldb_INCLUDE_DIR ${LEVELDB_INCLUDE_DIRS})
    set(leveldb_LIBRARIES ${LEVELDB_LIBRARIES})
    message(STATUS "LevelDB trovato via pkg-config.")
endif()

# Boost - Usare la nuova sintassi
find_package(Boost 1.74 REQUIRED
    COMPONENTS
        thread filesystem program_options chrono date_time regex system atomic
)



pkg_check_modules(libevent REQUIRED libevent)
pkg_check_modules(libevent_pthreads REQUIRED libevent_pthreads)
pkg_check_modules(libunivalue REQUIRED libunivalue)


# ZMQ
if (ENABLE_ZMQ)
    pkg_check_modules(LIBZMQ REQUIRED libzmq)
endif()

# miniupnpc
if (ENABLE_UPNP)
    pkg_check_modules(MINIUPNPC REQUIRED miniupnpc)
endif()

# QRencode
if (USE_QRCODE)
    pkg_check_modules(QRENCODE REQUIRED libqrencode)
endif()

# sphlib
pkg_check_modules(LIBSPH REQUIRED libsph)

# libsecp256k1
pkg_check_modules(SECP256K1 REQUIRED libsecp256k1)

# BerkeleyDB
find_path(BDB_INCLUDE_DIR
    NAMES db_cxx.h
    PATHS /usr/include/db6.0 /usr/include /usr/local/include
    PATH_SUFFIXES db6.0 db5.0 db6 db5
)
find_library(BDB_LIBRARY
    NAMES db_cxx-6.0 db_cxx db_cxx-5.3 db_cxx-5
    PATHS /usr/lib64 /usr/lib /usr/local/lib
)

if (NOT BDB_INCLUDE_DIR OR NOT BDB_LIBRARY)
    message(FATAL_ERROR "BerkeleyDB (db_cxx) non trovato")
endif()

message(STATUS "BerkeleyDB include: ${BDB_INCLUDE_DIR}")
message(STATUS "BerkeleyDB library: ${BDB_LIBRARY}")

# ======================================
# Include path comuni
# ======================================
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/compat
    ${CMAKE_CURRENT_SOURCE_DIR}/src/consensus
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/ctaes
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/ethash/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/policy
    ${CMAKE_CURRENT_SOURCE_DIR}/src/primitives
    ${CMAKE_CURRENT_SOURCE_DIR}/src/script
    ${CMAKE_CURRENT_SOURCE_DIR}/src/support
    ${CMAKE_CURRENT_SOURCE_DIR}/src/util
    ${CMAKE_CURRENT_SOURCE_DIR}/src/assets
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rpc
    ${CMAKE_CURRENT_SOURCE_DIR}/src/wallet
    ${CMAKE_CURRENT_SOURCE_DIR}/src/zmq
    ${BDB_INCLUDE_DIR}
    ${LIBSPH_INCLUDE_DIR}
    ${SECP256K1_INCLUDE_DIR}
    ${leveldb_INCLUDE_DIR}  # Aggiunto qui
    ${CMAKE_DL_INCLUDE_LIBS}
)

set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qt
    ${CMAKE_CURRENT_SOURCE_DIR}/src/qt/forms
)

# ======================================
# Sorgenti core comuni
# ======================================
file(GLOB CORE_TOP_SRC
    src/*.cpp
    src/primitives/*.cpp
    src/assets/*.cpp
    src/compat/*.cpp
    src/consensus/*.cpp
    src/policy/*.cpp
    src/script/*.cpp
    src/support/*.cpp
    src/rpc/*.cpp
)

if (ENABLE_WALLET)
    file(GLOB CORE_WALLET_SRC
        src/wallet/*.cpp
    )
endif()

if (ENABLE_ZMQ)
    file(GLOB CORE_ZMQ_SRC
        src/zmq/*.cpp
    )
endif()

# Escludi i file di test dalla compilazione
file(GLOB CORE_CRYPTO_SRC
    src/crypto/*.c
    src/crypto/*.cpp
    src/crypto/ctaes/*.c
    src/crypto/ethash/lib/ethash/*.cpp
    src/crypto/ethash/lib/ethash/*.c
    src/crypto/ethash/lib/keccak/*.c
)

list(REMOVE_ITEM CORE_CRYPTO_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/md_helper.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/ctaes/bench.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/ctaes/test.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/crypto/ctaes/ctaes.c"
)

list(REMOVE_ITEM CORE_TOP_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/ravend.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/raven-cli.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/raven-tx.cpp"
)


set(RAVEN_CORE_SOURCES
    ${CORE_TOP_SRC}
    ${CORE_CRYPTO_SRC}
    ${CORE_ZMQ_SRC}
    ${CORE_WALLET_SRC}
)

# Evita che AutoUic cerchi un .ui inesistente per wallet.h
#set_source_files_properties(src/wallet/wallet.h PROPERTIES SKIP_AUTOUIC ON)

# ======================================
# Libreria "core" (OBJECT)
# ======================================
add_library(raven_core OBJECT ${RAVEN_CORE_SOURCES})

set_source_files_properties(
    src/crypto/sha2.c
    src/crypto/whirlpool.c
    src/crypto/whirlpoolx.c
    PROPERTIES
        LANGUAGE CXX
)

if (ENABLE_ZMQ)
    target_include_directories(raven_core PUBLIC ${LIBZMQ_INCLUDE_DIRS})
    target_link_libraries(raven_core PUBLIC ${LIBZMQ_LIBRARIES})
    target_compile_definitions(raven_core PUBLIC ENABLE_ZMQ)
endif()

if (ENABLE_UPNP)
    target_include_directories(raven_core PUBLIC ${MINIUPNPC_INCLUDE_DIRS})
    target_link_libraries(raven_core PUBLIC ${MINIUPNPC_LIBRARIES})
    target_compile_definitions(raven_core PUBLIC ENABLE_UPNP)
endif()

if (USE_QRCODE)
    target_include_directories(raven_core PUBLIC ${QRENCODE_INCLUDE_DIRS})
    target_link_libraries(raven_core PUBLIC ${QRENCODE_LIBRARIES})
    target_compile_definitions(raven_core PUBLIC USE_QRCODE)
endif()

if (ENABLE_WALLET)
    target_compile_definitions(raven_core PUBLIC ENABLE_WALLET)
endif()

# ======================================
# Binario: ravend
# ======================================
if (BUILD_RAVENCOIND)
    add_executable(ravend
        src/ravend.cpp
        $<TARGET_OBJECTS:raven_core>
    )

    target_link_libraries(ravend
        PRIVATE
            ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${leveldb_LIBRARIES}
            ${BDB_LIBRARY}
            ${SECP256K1_LDFLAGS}
            ${LIBSPH_LDFLAGS}
            ${libevent_LDFLAGS}  # Aggiunto
            ${libevent_pthreads_LDFLAGS}
            ${libunivalue_LDFLAGS}
            ${MINIUPNPC_LDFLAGS}
            ${CMAKE_DL_LIBS}  # Aggiunto

    )

#    install(TARGETS ravend RUNTIME DESTINATION bin)
endif()

# ======================================
# Binario: raven-cli
# ======================================
if (BUILD_RAVENCOIN_CLI)
    add_executable(raven-cli
        src/raven-cli.cpp
        $<TARGET_OBJECTS:raven_core>
    )

    target_link_libraries(raven-cli
        PRIVATE
            ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${BDB_LIBRARY}
            ${leveldb_LIBRARIES}
            ${BDB_LIBRARY}
            ${LIBSPH_LDFLAGS}
            ${SECP256K1_LDFLAGS}
            ${libevent_LDFLAGS}
            ${libevent_pthreads_LDFLAGS}
            ${libunivalue_LDFLAGS}
            ${MINIUPNPC_LDFLAGS} # Aggiunto
            ${CMAKE_DL_LIBS}  # Aggiunto
)

    install(TARGETS raven-cli RUNTIME DESTINATION bin)
endif()

# ======================================
# Binario: raven-tx
# ======================================
if (BUILD_RAVENCOIN_TX)
    add_executable(raven-tx
        src/raven-tx.cpp
        $<TARGET_OBJECTS:raven_core>
    )

    target_link_libraries(raven-tx
        PRIVATE
            ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${BDB_LIBRARY}
            ${leveldb_LIBRARIES}
            ${BDB_LIBRARY}
            ${LIBSPH_LDFLAGS}
            ${SECP256K1_LDFLAGS}
            ${libevent_LDFLAGS}
            ${libevent_pthreads_LDFLAGS}
            ${libunivalue_LDFLAGS}
            ${MINIUPNPC_LDFLAGS}# Aggiunto
            ${CMAKE_DL_LIBS}  # Aggiunto
            )

#    install(TARGETS raven-tx RUNTIME DESTINATION bin)
endif()

# ======================================
# Qt GUI: raven-qt (Qt6)
# ======================================
if (BUILD_RAVENCOIN_QT)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 6.2 REQUIRED COMPONENTS Core Widgets Gui Network)

    file(GLOB RAVEN_QT_SRC
        src/qt/*.cpp
        src/qt/*.mm
    )

    file(GLOB RAVEN_QT_FORMS
        src/qt/forms/*.ui
    )

    set(RAVEN_QT_RESOURCES
        src/qt/raven.qrc
    )

    # Avoid AutoUic interpreting core headers like ui_interface.h as generated UI headers.
    set_source_files_properties(${RAVEN_QT_SRC} PROPERTIES SKIP_AUTOUIC ON)

    add_executable(raven-qt
        src/qt/raven.cpp
        ${RAVEN_QT_SRC}
        ${RAVEN_QT_FORMS}
        ${RAVEN_QT_RESOURCES}
        $<TARGET_OBJECTS:raven_core>
    )

    target_link_libraries(raven-qt
        PRIVATE
            ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${BDB_LIBRARY}
            ${leveldb_LIBRARIES}
            ${BDB_LIBRARY}
            ${LIBSPH_LDFLAGS}
            ${SECP256K1_LDFLAGS}
            ${libevent_LDFLAGS}
            ${libevent_pthreads_LDFLAGS}
            ${libunivalue_LDFLAGS}
            ${MINIUPNPC_LDFLAGS}# Aggiunto
            ${CMAKE_DL_LIBS}  # Aggiunto
            )

    target_include_directories(raven-qt PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qt
        ${CMAKE_CURRENT_SOURCE_DIR}/src/qt/forms
    )

 #   install(TARGETS raven-qt RUNTIME DESTINATION bin)
endif()
