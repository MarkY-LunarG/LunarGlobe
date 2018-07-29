# Build Instructions

Instructions for building this repository on Linux, and Windows.

## Index

1. [Contributing](#contributing-to-the-repository)
2. [Repository Set-Up](#repository-set-up)
3. [Windows Build](#building-on-windows)
4. [Linux Build](#building-on-linux)

## Contributing to the Repository

If you intend to contribute, the preferred work flow is for you to develop
your contribution in a fork of this repository in your GitHub account and
then submit a pull request.
Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file in this repository for more details.

## Repository Set-Up

### Pre-Requisites

#### Update Your Graphics Display Drivers
This repository does not contain a Vulkan-capable driver.
Before proceeding, it is strongly recommended that you obtain a Vulkan driver from your
graphics hardware vendor and install it properly.

#### Install the Vulkan SDK

Download the Vulkan SDK from the LunarXchange web-site:
https://vulkan.lunarg.com/

### Download the Repository

To create your local git repository:

    git clone https://github.com/MarkY-LunarG/LunarGlobe.git

## Building On Windows

### Windows Build Requirements

Windows 7+ with the following software packages:

- Microsoft Visual Studio 2013 Update 4 Professional, VS2015 (any version), or VS2017 (any version).
- [CMake](http://www.cmake.org/download/)
  - Tell the installer to "Add CMake to the system PATH" environment variable.
- [Git](http://git-scm.com/download/win)
  - Tell the installer to allow it to be used for "Developer Prompt" as well as "Git Bash".
  - Tell the installer to treat line endings "as is" (i.e. both DOS and Unix-style line endings).
  - Install both the 32-bit and 64-bit versions, as the 64-bit installer does not install the
    32-bit libraries and tools.

### Windows Build - Microsoft Visual Studio

1. Open a Developer Command Prompt for VS201x
2. Change directory to `LunarGlobe` -- the root of the cloned git repository
3. Run `git submodule update --init --recursive` -- this will download in-tree external dependencies
4. Create a `build` directory, change into that directory, and run cmake

For example, assuming an SDK is installed, for VS2017 (generators for other versions are [specified here](#cmake-visual-studio-generators)):

```
cmake "Visual Studio 15 2017 Win64" \
       -DVULKAN_HEADERS_INSTALL_DIR=absolute_path_to_header_install \
       -DVULKAN_LOADER_INSTALL_DIR=absolute_path_to_header_install \
       ..
```

This will create a Windows solution file named `LunarGlobe.sln` in the build directory.

If no SDK is installed, you will need to point to the appropriate Vulkan Headers and Vulkan Loader repository.


Launch Visual Studio and open the "LunarGlobe.sln" solution file in the build folder.
You may select "Debug" or "Release" from the Solution Configurations drop-down list.
Start a build by selecting the Build->Build Solution menu item.
This solution copies the loader it built to each program's build directory
to ensure that the program uses the loader built from this solution.

## Building On Linux

### Linux Build Requirements

This repository has been built and tested on the two most recent Ubuntu LTS versions.
Currently, the oldest supported version is Ubuntu 14.04, meaning that the minimum supported compiler versions are GCC 4.8.2 and Clang 3.4, although earlier versions may work.
It should be straightforward to adapt this repository to other Linux distributions.

**Required Package List:**

    sudo apt-get install git cmake build-essential

### Linux Build

Example debug build

See **Loader and Validation Layer Dependencies** for more information and other options:

1. In a Linux terminal, `cd LunarGlobe` -- the root of the cloned git repository
2. Execute: `git submodule update --init --recursive` -- this will download in-tree external components
3. Create a `build` directory, change into that directory, and run cmake:

        mkdir build
        cd build
        # If an SDK is installed and the setup-env.sh script has been run,
        cmake -DCMAKE_BUILD_TYPE=Debug ..

4. Run `make -j8` to begin the build

If your build system supports ccache, you can enable that via CMake option `-DUSE_CCACHE=On`
