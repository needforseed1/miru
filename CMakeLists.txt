cmake_minimum_required(VERSION 3.21)

project(Miru VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Network Sql Concurrent)
find_package(ZLIB REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(miru
    src/main.cpp
    src/app/ApplicationController.cpp
    src/app/ApplicationController.h
    src/app/CachingNetworkFactory.h
    src/app/WatchHistory.cpp
    src/app/WatchHistory.h
    src/services/CinemetaClient.cpp
    src/services/CinemetaClient.h
    src/services/AIOStreamsClient.cpp
    src/services/AIOStreamsClient.h
    src/services/BadgeMatcher.cpp
    src/services/BadgeMatcher.h
    src/services/SubtitlesClient.cpp
    src/services/SubtitlesClient.h
    src/services/ImdbRatings.cpp
    src/services/ImdbRatings.h
    src/services/TraktClient.cpp
    src/services/TraktClient.h
    src/player/ExternalMpvPlayer.cpp
    src/player/ExternalMpvPlayer.h
    src/player/MpvVideoSurface.cpp
    src/player/MpvVideoSurface.h
)

if(APPLE)
    set(MIRU_BUNDLED_MPV "" CACHE FILEPATH "Path to an mpv executable to copy into the macOS app bundle")

    set_target_properties(miru PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME "Miru"
        MACOSX_BUNDLE_BUNDLE_NAME "Miru"
        MACOSX_BUNDLE_GUI_IDENTIFIER "io.github.needforseed1.miru"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    )

    if(MIRU_BUNDLED_MPV)
        add_custom_command(TARGET miru POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:miru>/Contents/Resources/mpv"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${MIRU_BUNDLED_MPV}"
                    "$<TARGET_BUNDLE_DIR:miru>/Contents/Resources/mpv/mpv"
            COMMENT "Bundling mpv into the macOS app"
            VERBATIM
        )
    endif()
endif()

qt_add_resources(miru "appresources"
    PREFIX "/"
    FILES
        resources/Sterzeck_badge.json
)

set(MIRU_MPV_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources/mpv")
if(EXISTS "${MIRU_MPV_RESOURCES_DIR}")
    if(APPLE)
        set(MIRU_MPV_RESOURCES_OUTPUT "$<TARGET_BUNDLE_DIR:miru>/Contents/Resources/mpv")
    else()
        set(MIRU_MPV_RESOURCES_OUTPUT "$<TARGET_FILE_DIR:miru>/mpv")
    endif()

    add_custom_command(TARGET miru POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${MIRU_MPV_RESOURCES_DIR}"
                "${MIRU_MPV_RESOURCES_OUTPUT}"
        COMMENT "Copying bundled mpv resources"
        VERBATIM
    )
endif()

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(miru
    URI StremioLinux
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/AppButton.qml
        qml/components/BadgePill.qml
        qml/components/Pill.qml
        qml/components/SearchField.qml
        qml/components/SearchSpinner.qml
        qml/components/PosterCard.qml
        qml/components/ResumeCard.qml
        qml/components/StreamCard.qml
        qml/components/SectionRail.qml
        qml/components/EpisodeList.qml
        qml/components/SettingsCard.qml
        qml/components/SettingsField.qml
        qml/components/SettingsCheck.qml
        qml/components/ChoiceChip.qml
        qml/components/AddonUrlField.qml
)

target_link_libraries(miru
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Network
        Qt6::Sql
        Qt6::Concurrent
        ZLIB::ZLIB
)

install(TARGETS miru
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)

install(DIRECTORY resources/mpv DESTINATION share/miru)
