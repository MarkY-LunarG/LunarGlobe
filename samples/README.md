# LunarGlobe Samples

## 01 - Triangle

<img src="screenshots/01_first_triangle.png" height="256px">

This is the "beginning to use Vulkan" example.
Or, more realistically, "beginning to use LunarGlobe" example.

It renders an indexed colored triangle with the MVP matrix
passed in via a uniform buffer.

## 02 - Dynamic Uniform Buffer

<img src="screenshots/02_dynamic_uniform_buffer.png" height="256px">

Building on top of the above triangle example, I added
the ability to spin the triangle around the Y-axis.
This is done by utilizing a dynamic uniform buffer
object and updating the offset before each draw to point
to an updated matrix.

Also, this is the first example to use GLM.

## 03 - Multi-Texture

<img src="screenshots/03_multi_texture.png" height="256px">

Building on the dynamic uniform app, I added the ability
to sample from two textures.
Also, instead of passing in the matrix every frame, pass
in a position for an ellipse.
The shader then tests the ellipse position versus the
incoming texture coordinates.
If the texture coordinates are within the ellipse, then
the shader samples from the second texture.
Otherwise, the shader samples the first texture.

## 04 - Push Constants

<img src="screenshots/04_push_constants.png" height="256px">

Building on the multi-texture app, I added the ability
to control what's going on in the shader using 3 push constants.
The first push constant is an integer value which is used to
indicate what behavior the shader should perform when sampling
from the two textures.
The second push constant defines the X radius of the
ellipse.
The third push constant defines the Y radius of the
ellipse.


## 05 - Simple GLM

<img src="screenshots/05_simple_glm.png" height="256px">

Stepping back, I wanted to setup a simple sample which
used GLM to setup and move a camera and objects.

The sample does the following:
 * Use a camera (defined as position and orientation) to
   generate both a projection and view matrix.
 * Define multiple objects in a single vertex and index
   list.
 * Use a dynamic uniform buffer to pass along the camera
   projection and view matrices to the shader.
 * Use push constants to define the current model matrix.
