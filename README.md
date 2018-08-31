# LunarGlobe

LunarGlobe is a personal development tree that is a work in progress.
My intent is to design a modular interface that I can use to develop
more complicated Vulkan applications.
Overall, I'd like to eventually create a "demoscene" style application
that can be used to showcase Vulkan features.


## CI Build Status
| Platform | Build Status |
|:--------:|:------------:|
| Linux/Clang | [![Build Status](https://travis-ci.com/MarkY-LunarG/LunarGlobe.svg?branch=master)](https://travis-ci.com/MarkY-LunarG/LunarGlobe) |
| Windows | [![Build status](https://ci.appveyor.com/api/projects/status/ifpbda8eb0e47764/branch/master?svg=true)](https://ci.appveyor.com/project/MarkY-LunarG/lunarglobe/branch/master) |

## License

This project is licensed under the Apache License Version 2.0 dated
January 2004.
More information about this license can be read in the provided
[LICENSE.txt](LICENSE.txt) file as well as at the Apache License
website https://www.apache.org/licenses/.

Feel free to use it and release it as allowed under the license.

Several other projects have been included using Git Submodules.
These projects may have their own licenses.
Information on these licenses can be found in
[COPYRIGHT.txt](COPYRIGHT.txt).

## Notice of Copyright

While this is a personal project, it should be seen as an extension
of my work at [LunarG](https://www.lunarg.com) and the
[LunarG VulkanTools repository](https://github.com/KhronosGroup/Vulkan-Tools), and
thus as copyrighted by LunarG starting in 2018.


## History

LunarGlobe started out as a remodelling of the
[LunarG Cube demo source](https://github.com/KhronosGroup/Vulkan-Tools/tree/master/cube).
It is now split into a library and applications.  The "globe_cube"
application is very similar in behavior to the original LunarG Cube demo.

## Samples

I wanted to start out small by creating simple example on top of the new framework
so that I could be sure things worked properly.  And, to let me do a little more messing
around with basic Vulkan.  So, there are several [Samples](./samples/README.md) that
I created using it in a simple way.

## Applications

I also have a folder for more in depth applications/demos.  The full list can be found in the
[Applications Listing](./apps/README.md).

## Building

Instructions for building this repositor can be found in the [Build.md](./BUILD.md)
file.

