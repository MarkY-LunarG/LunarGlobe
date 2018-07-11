# LunarGravity

LunarGravity is a personal development tree that is a work in progress.
My intent is to design a modular interface that I can use to develop
more complicated Vulkan applications.
Overall, I'd like to eventually create a "demoscene" style application
that can be used to showcase Vulkan features.


## License

This project is licensed under the Apache License Version 2.0 dated
January 2004.
More information about this license can be read in the provided
[LICENSE.txt](LICENSE.txt) file as well as at the Apache License
website https://www.apache.org/licenses/.

Feel free to use it and release it as allowed under the license.


## Notice of Copyright

While this is a personal project, it should be seen as an extension
of my work at [LunarG](https://www.lunarg.com) and the
[LunarG VulkanTools repository](https://github.com/KhronosGroup/Vulkan-Tools), and
thus as copyrighted by LunarG starting in 2018.


## History

The majority of LunarGravity code is based on the original
[LunarG Cube demo source](https://github.com/KhronosGroup/Vulkan-Tools/tree/master/cube).
However, that code was very monolithic and wasn't flexible enough for my design.


## New Features To Add:

1.  Update to Modern CMake
2.  Reduce amount of surface capability queries
3.  Create separate threads for command generation, render and normal application
4.  Split window functionality into separate OS-specific classes
5.  Port over code from old gravity:
    * Shader Loading
    * Scene defining
    * Model loading

## Bugs
1.  Windows: Fix stutter after resize
