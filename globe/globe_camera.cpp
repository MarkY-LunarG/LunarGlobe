/*
 * LunarGlobe - globe_camera.cpp
 *
 * Copyright (C) 2018 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Young <marky@lunarg.com>
 */

#include <stdio.h>
#include <cmath>

#include "globe_camera.hpp"

GlobeCamera::GlobeCamera() : _projection_matrix(glm::mat4(1.0f)), _view_matrix(glm::mat4(1.0f)) {}

void GlobeCamera::SetPerspectiveProjection(float aspect_ratio, float field_of_view, float near, float far) {
    _projection_matrix = glm::perspectiveRH(glm::radians(field_of_view), aspect_ratio, near, far);
}

void GlobeCamera::SetFrustumProjection(float left, float right, float top, float bottom, float near, float far) {
    _projection_matrix = glm::frustumRH(left, right, bottom, top, near, far);
}

void GlobeCamera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
    _projection_matrix = glm::orthoRH(left, right, bottom, top, near, far);
}

void GlobeCamera::SetCameraPositions(float camera_x, float camera_y, float camera_z, float look_dir_x, float look_dir_y,
                                     float look_dir_z, float up_x, float up_y, float up_z) {
    glm::vec3 camera_pos = glm::vec3(camera_x, camera_y, camera_z);
    glm::vec3 look_vec = camera_pos + glm::vec3(look_dir_x, look_dir_y, look_dir_z);
    _view_matrix = glm::lookAtRH(camera_pos, look_vec, glm::vec3(up_x, up_y, up_z));
}
