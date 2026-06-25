cmake_minimum_required(VERSION 3.21)

project(StremioLinuxFrontend VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Quick QuickControls2 Network Sql Concurrent)
find_package(ZLIB REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(stremio-linux
    src/main.cpp
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
    src/player/ExternalMpvPlayer.cpp
    src/player/ExternalMpvPlayer.h
    src/player/MpvVideoSurface.cpp
    src/player/MpvVideoSurface.h
)

qt_add_resources(stremio-linux "appresources"
    PREFIX "/"
    FILES
        resources/Sterzeck_badge.json
)

set_source_files_properties(qml/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(stremio-linux
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

target_link_libraries(stremio-linux
    PRIVATE
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::Network
        Qt6::Sql
        Qt6::Concurrent
        ZLIB::ZLIB
)

install(TARGETS stremio-linux
    BUNDLE DESTINATION .
    RUNTIME DESTINATION bin
)
