cmake_minimum_required(VERSION 3.15)
project(launcher Java)
find_package(Java 1.7 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT JavaCheck)
set(CMAKE_JAVA_COMPILE_FLAGS -target 7 -source 7 -Xlint:deprecation -Xlint:unchecked)
execute_process(COMMAND "${Java_JAVAC_EXECUTABLE}" -version OUTPUT_VARIABLE _javacver ERROR_VARIABLE _javacver)
if(_javacver MATCHES "javac 1[89]|javac ([2-9][0-9]|[1-9][0-9]+)\\.")
    # ponytail: JDK>=9 dropped -source 7; the code is Java 7 compatible, classes still load on old JREs.
    set(CMAKE_JAVA_COMPILE_FLAGS -target 8 -source 8 -Xlint:-options -Xlint:deprecation -Xlint:unchecked)
endif()

set(SRC
    JavaCheck.java
)

add_jar(JavaCheck ${SRC})
install_jar(JavaCheck "${JARS_DEST_DIR}")
