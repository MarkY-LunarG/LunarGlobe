//
// Project:                 LunarGlobe
// SPDX-License-Identifier: Apache-2.0
//
// File:                    globe/globe_camera.cpp
// Copyright(C):            2018-2019; LunarG, Inc.
// Author(s):               Mark Young <marky@lunarg.com>
//

#include <stdio.h>
#include <cmath>

#include "globe_camera.hpp"

GlobeCamera::GlobeCamera()
    : _projection_matrix(glm::mat4(1.0f)),
      _camera_position(glm::vec3(0.f, 0.f, -1.f)),
      _camera_orientation(glm::vec3(0.f, 0.f, 0.f)) {}

void GlobeCamera::SetPerspectiveProjection(float aspect_ratio, float field_of_view, float near, float far) {
    _projection_matrix = glm::perspectiveRH(glm::radians(field_of_view), aspect_ratio, near, far);
}

void GlobeCamera::SetFrustumProjection(float left, float right, float top, float bottom, float near, float far) {
    _projection_matrix = glm::frustumRH(left, right, bottom, top, near, far);
}

void GlobeCamera::SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far) {
    _projection_matrix = glm::orthoRH(left, right, bottom, top, near, far);
}

void GlobeCamera::SetCameraPosition(float camera_x, float camera_y, float camera_z) {
    _camera_position = glm::vec3(camera_x, camera_y, camera_z);
}

void GlobeCamera::SetCameraOrientation(float yaw, float pitch, float roll) {
    _camera_orientation = glm::vec3(glm::radians(pitch), glm::radians(yaw), glm::radians(roll));
}

glm::mat4 GlobeCamera::ViewMatrix() {
    glm::mat4 view_mat = glm::eulerAngleYXZ(_camera_orientation.y, _camera_orientation.x, _camera_orientation.z);
    return glm::translate(view_mat, _camera_position);
}
