project(moddmr)

set(moddmr_CODEC_SOURCES
    dmrcodec/BPTC19696.cpp
    dmrcodec/Golay2087.cpp
    dmrcodec/Golay24128.cpp
    dmrcodec/RS129.cpp
    dmrcodec/CRC.cpp
    dmrcodec/DMRLC.cpp
    dmrcodec/DMRFullLC.cpp
    dmrcodec/DMRTrellis.cpp
    dmrcodec/DMRSlotType.cpp
    dmrcodec/AMBEFEC.cpp
    dmrcodec/DMREMB.cpp
    dmrcodec/Hamming.cpp
    dmrcodec/QR1676.cpp
    dmrcodec/Utils.cpp
    dmrcodec/Sync.cpp
)

# OpenDMR/OP25 AMBE+2 encoder (IMBE analysis + mbelib feedback path)
set(moddmr_AMBE_SOURCES
    dmrambeencoder.cpp
    opendmr/encoder/mbeenc.cpp
    opendmr/encoder/cgolay24128.cpp
    opendmr/encoder/aux_sub.cc
    opendmr/encoder/basicop2.cc
    opendmr/encoder/ch_encode.cc
    opendmr/encoder/dc_rmv.cc
    opendmr/encoder/dsp_sub.cc
    opendmr/encoder/encode.cc
    opendmr/encoder/imbe_vocoder.cc
    opendmr/encoder/imbe_vocoder_impl.cc
    opendmr/encoder/math_sub.cc
    opendmr/encoder/pe_lpf.cc
    opendmr/encoder/pitch_est.cc
    opendmr/encoder/pitch_ref.cc
    opendmr/encoder/qnt_sub.cc
    opendmr/encoder/rand_gen.cc
    opendmr/encoder/sa_encode.cc
    opendmr/encoder/tbls.cc
    opendmr/encoder/v_uv_det.cc
    opendmr/mbelib/mbelib.c
    opendmr/mbelib/ambe3600x2450.c
    opendmr/mbelib/ecc.c
)

set(moddmr_SOURCES
    dmrmod.cpp
    dmrmodbaseband.cpp
    dmrmodsource.cpp
    dmrmodplugin.cpp
    dmrmodsettings.cpp
    dmrmodwebapiadapter.cpp
    dmr4fsk.cpp
    dmrframebuilder.cpp
    ${moddmr_CODEC_SOURCES}
    ${moddmr_AMBE_SOURCES}
)

set(moddmr_HEADERS
    dmrmod.h
    dmrmodbaseband.h
    dmrmodsource.h
    dmrmodplugin.h
    dmrmodsettings.h
    dmrmodwebapiadapter.h
    dmr4fsk.h
    dmrframebuilder.h
    dmrambeencoder.h
)

include_directories(
    ${CMAKE_SOURCE_DIR}/swagger/sdrangel/code/qt5/client
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/dmrcodec
    ${CMAKE_CURRENT_SOURCE_DIR}/opendmr/encoder
    ${CMAKE_CURRENT_SOURCE_DIR}/opendmr/mbelib
)

add_definitions(-DUSE_DMR)

if(NOT SERVER_MODE)
    set(moddmr_SOURCES
        ${moddmr_SOURCES}
        dmrmodgui.cpp
        dmrmodgui.ui
    )
    set(moddmr_HEADERS
        ${moddmr_HEADERS}
        dmrmodgui.h
    )
    set(TARGET_NAME ${PLUGINS_PREFIX}moddmr)
    set(TARGET_LIB "Qt::Widgets")
    set(TARGET_LIB_GUI "sdrgui")
    set(INSTALL_FOLDER ${INSTALL_PLUGINS_DIR})
else()
    set(TARGET_NAME ${PLUGINSSRV_PREFIX}moddmrsrv)
    set(TARGET_LIB "")
    set(TARGET_LIB_GUI "")
    set(INSTALL_FOLDER ${INSTALL_PLUGINSSRV_DIR})
endif()

if(NOT Qt6_FOUND)
    add_library(${TARGET_NAME} ${moddmr_SOURCES})
else()
    qt_add_plugin(${TARGET_NAME} CLASS_NAME DMRModPlugin)
    target_sources(${TARGET_NAME} PRIVATE ${moddmr_SOURCES})
endif()

if(NOT BUILD_SHARED_LIBS)
    set_property(GLOBAL APPEND PROPERTY STATIC_PLUGINS_PROPERTY ${TARGET_NAME})
endif()

# OpenDMR/IMBE vocoder is noisy under pedantic flags; keep SDRangel defaults elsewhere
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set_source_files_properties(${moddmr_AMBE_SOURCES} PROPERTIES
        COMPILE_FLAGS "-Wno-unused-parameter -Wno-unused-variable -Wno-sign-compare"
    )
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
    Qt::Core
    ${TARGET_LIB}
    sdrbase
    ${TARGET_LIB_GUI}
    swagger
    m
)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_FOLDER})

if (WIN32)
    install(FILES $<TARGET_PROPERTY:${TARGET_NAME},RUNTIME_OUTPUT_DIRECTORY>/${TARGET_NAME}stripped.pdb CONFIGURATIONS Release DESTINATION ${INSTALL_FOLDER} RENAME ${TARGET_NAME}.pdb )
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${INSTALL_FOLDER} )
endif()
