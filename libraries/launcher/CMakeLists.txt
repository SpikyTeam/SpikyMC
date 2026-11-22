cmake_minimum_required(VERSION 3.15)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT team.spiky.mc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 7 -source 7)
execute_process(COMMAND "${Java_JAVAC_EXECUTABLE}" -version OUTPUT_VARIABLE _javacver ERROR_VARIABLE _javacver)
if(_javacver MATCHES "javac 1[89]|javac ([2-9][0-9]|[1-9][0-9]+)\\.")
    # ponytail: JDK>=9 dropped -source 7; launcher jars only need to run on modern Java.
    set(CMAKE_JAVA_COMPILE_FLAGS -target 8 -source 8 -Xlint:-options)
endif()

set(SRC
    team/spiky/mc/EntryPoint.java
    team/spiky/mc/SystemProperties.java
    team/spiky/mc/launcher/Launcher.java
    team/spiky/mc/launcher/impl/AbstractLauncher.java
    team/spiky/mc/launcher/impl/StandardLauncher.java
    team/spiky/mc/exception/ParameterNotFoundException.java
    team/spiky/mc/exception/ParseException.java
    team/spiky/mc/utils/Parameters.java
    team/spiky/mc/utils/ReflectionUtils.java
    team/spiky/mc/utils/logging/Level.java
    team/spiky/mc/utils/logging/Log.java
    team/spiky/mc/legacy/LegacyProxy.java
)

set(LEGACY_SRC
    legacy/team/spiky/mc/legacy/LegacyFrame.java
    legacy/team/spiky/mc/legacy/LegacyLauncher.java
    legacy/team/spiky/mc/legacy/fix/online/Handler.java
    legacy/team/spiky/mc/legacy/fix/online/OnlineFixes.java
    legacy/team/spiky/mc/legacy/fix/online/OnlineModeFix.java
    legacy/team/spiky/mc/legacy/fix/online/SkinFix.java
    legacy/team/spiky/mc/legacy/utils/Base64.java
    legacy/team/spiky/mc/legacy/utils/api/MojangApi.java
    legacy/team/spiky/mc/legacy/utils/api/Texture.java
    legacy/team/spiky/mc/legacy/utils/json/JsonParseException.java
    legacy/team/spiky/mc/legacy/utils/json/JsonParser.java
    legacy/team/spiky/mc/legacy/utils/url/ByteArrayUrlConnection.java
    legacy/team/spiky/mc/legacy/utils/url/UrlUtils.java
    legacy/net/minecraft/Launcher.java
    legacy/team/spiky/mc/legacy/LegacyProxy.java
)

add_jar(NewLaunch ${SRC})
add_jar(NewLaunchLegacy ${LEGACY_SRC} INCLUDE_JARS NewLaunch)
install_jar(NewLaunch "${JARS_DEST_DIR}")
install_jar(NewLaunchLegacy "${JARS_DEST_DIR}")
