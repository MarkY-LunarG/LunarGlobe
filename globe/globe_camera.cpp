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

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

GlobeCamera::GlobeCamera() : _projection_matrix(glm::mat4(1.0f)) {}

void GlobeCamera::SetPerspectiveProjection(float aspect_ratio, float field_of_view, float near, float far) {
    glm::vec3 pos{0.f, 0.f, -1.f};
    glm::vec3 forward{0.f, 0.f, 1.f};
    glm::vec3 up{0.f, -1.f, 0.f};
    auto v = glm::lookAtRH(pos, pos + forward, up);
    _projection_matrix = glm::perspectiveRH(glm::radians(field_of_view), aspect_ratio, near, far);
}

void GlobeCamera::SetFrustumProjection(float left, float right, float top, float bottom, float near, float far) {
    _projection_matrix = glm::frustum(left, right, bottom, top, near, far);
}

void GlobeCamera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
    _projection_matrix = glm::mat4(0.f);
    float inverse_right_minus_left = 1.f / (right - left);
    float inverse_top_minus_bottom = 1.f / (top - bottom);
    float negative_inverse_far_minus_near = -1.f / (far - near);
    _projection_matrix[0][0] = 2.f * inverse_right_minus_left;
    _projection_matrix[0][3] = -(right + left) * inverse_right_minus_left;
    _projection_matrix[1][1] = 2.f * inverse_top_minus_bottom;
    _projection_matrix[1][3] = -(top + bottom) * inverse_top_minus_bottom;
    _projection_matrix[2][2] = 2.f * negative_inverse_far_minus_near;
    _projection_matrix[2][3] = (far + near) * negative_inverse_far_minus_near;
    _projection_matrix[3][3] = 1.f;
}

void GlobeCamera::SetViewPosition(float x, float y, float z) {}

void GlobeCamera::SetViewOrientation(float x, float y, float z) {}

const glm::mat4* GlobeCamera::ViewMatrix() { return nullptr; }

const glm::mat4* GlobeCamera::ProjectionMatrix() { return &_projection_matrix; }
