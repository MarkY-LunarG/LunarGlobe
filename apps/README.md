# LunarGlobe Applications

## Triangle

<img src="screenshots/01_first_triangle.png" height="256px">

This is the "beginning to use Vulkan" example.  Or, more
realistically, "beginning to use LunarGlobe" example.

It renders an indexed colored triangle with the MVP matrix
passed in via a uniform buffer.

## Dynamic Uniform Buffer 

<img src="screenshots/02_dynamic_uniform_buffer.png" height="256px">

Building on top of the above triangle example, I added
the ability to spin the triangle around the Y-axis.
This is done by utilizing a dynamic uniform buffer
object and updating the offset before each draw to point
to an updated matrix.

Also, this is the first example to use GLM.

## LunarGlobe Cube

<img src="screenshots/globe_cube.png" height="256px">

This is a translation of the
[LunarG Cube demo](https://github.com/KhronosGroup/Vulkan-Tools/blob/master/cube/cube.c)
ported into the LunarGlobe framework.
