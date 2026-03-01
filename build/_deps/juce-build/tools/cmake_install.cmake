# Install script for directory: /app/build/_deps/juce-src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/app/build/_deps/juce-build/tools/modules/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("/app/build/_deps/juce-build/tools/extras/Build/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/JUCE-7.0.9" TYPE FILE FILES
    "/app/build/_deps/juce-build/tools/JUCEConfigVersion.cmake"
    "/app/build/_deps/juce-build/tools/JUCEConfig.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/JUCECheckAtomic.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/JUCEHelperTargets.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/JUCEModuleSupport.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/JUCEUtils.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/JuceLV2Defines.h.in"
    "/app/build/_deps/juce-src/extras/Build/CMake/LaunchScreen.storyboard"
    "/app/build/_deps/juce-src/extras/Build/CMake/PIPAudioProcessor.cpp.in"
    "/app/build/_deps/juce-src/extras/Build/CMake/PIPAudioProcessorWithARA.cpp.in"
    "/app/build/_deps/juce-src/extras/Build/CMake/PIPComponent.cpp.in"
    "/app/build/_deps/juce-src/extras/Build/CMake/PIPConsole.cpp.in"
    "/app/build/_deps/juce-src/extras/Build/CMake/RecentFilesMenuTemplate.nib"
    "/app/build/_deps/juce-src/extras/Build/CMake/UnityPluginGUIScript.cs.in"
    "/app/build/_deps/juce-src/extras/Build/CMake/checkBundleSigning.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/copyDir.cmake"
    "/app/build/_deps/juce-src/extras/Build/CMake/juce_runtime_arch_detection.cpp"
    "/app/build/_deps/juce-src/extras/Build/CMake/juce_LinuxSubprocessHelper.cpp"
    )
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/app/build/_deps/juce-build/tools/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
