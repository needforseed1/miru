cmake_minimum_required(VERSION 3.21)

project(Miru VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Network Sql Concurrent Test)
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
    src/player/MpvLaunchOptions.cpp
    src/player/MpvLaunchOptions.h
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
        resources/fonts/InterVariable.ttf
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
        qml/components/ColorSettingRow.qml
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

enable_testing()

function(miru_add_qtest target)
    add_executable(${target} ${ARGN})
    target_link_libraries(${target} PRIVATE Qt6::Test)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

miru_add_qtest(tst_badge_matcher
    tests/tst_badge_matcher.cpp
    src/services/BadgeMatcher.cpp
    src/services/BadgeMatcher.h
)
target_link_libraries(tst_badge_matcher PRIVATE Qt6::Core)

miru_add_qtest(tst_aiostreams_client
    tests/tst_aiostreams_client.cpp
    src/services/AIOStreamsClient.cpp
    src/services/AIOStreamsClient.h
    src/services/BadgeMatcher.cpp
    src/services/BadgeMatcher.h
)
target_compile_definitions(tst_aiostreams_client PRIVATE MIRU_TESTING)
target_link_libraries(tst_aiostreams_client PRIVATE Qt6::Core Qt6::Network)

miru_add_qtest(tst_cinemeta_client
    tests/tst_cinemeta_client.cpp
    src/services/CinemetaClient.cpp
    src/services/CinemetaClient.h
)
target_compile_definitions(tst_cinemeta_client PRIVATE MIRU_TESTING)
target_link_libraries(tst_cinemeta_client PRIVATE Qt6::Core Qt6::Network)

miru_add_qtest(tst_mpv_launch_options
    tests/tst_mpv_launch_options.cpp
    src/player/MpvLaunchOptions.cpp
    src/player/MpvLaunchOptions.h
)
target_link_libraries(tst_mpv_launch_options PRIVATE Qt6::Core)

miru_add_qtest(tst_watch_history
    tests/tst_watch_history.cpp
    src/app/WatchHistory.cpp
    src/app/WatchHistory.h
)
target_link_libraries(tst_watch_history PRIVATE Qt6::Core)
set_tests_properties(tst_watch_history PROPERTIES
    ENVIRONMENT "XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-data"
)

miru_add_qtest(tst_application_settings
    tests/tst_application_settings.cpp
    src/app/ApplicationController.cpp
    src/app/ApplicationController.h
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
    src/player/MpvLaunchOptions.cpp
    src/player/MpvLaunchOptions.h
)
target_link_libraries(tst_application_settings
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Network
        Qt6::Sql
        Qt6::Concurrent
        ZLIB::ZLIB
)
set_tests_properties(tst_application_settings PROPERTIES
    ENVIRONMENT "QT_QPA_PLATFORM=offscreen;XDG_DATA_HOME=${CMAKE_CURRENT_BINARY_DIR}/test-data"
)
