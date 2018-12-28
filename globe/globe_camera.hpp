/*
 * LunarGlobe - globe_camera.hpp
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

#pragma once

#include <cstdint>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>

class GlobeCamera {
   public:
    GlobeCamera();
    virtual ~GlobeCamera() {}

    void SetPerspectiveProjection(float aspect_ratio, float field_of_view, float near, float far);
    void SetFrustumProjection(float left, float right, float top, float bottom, float near, float far);
    void SetOrthographicProjection(float left, float right, float top, float bottom, float near, float far);

    void SetCameraPositions(float camera_x, float camera_y, float camera_z, float focus_x, float focus_y, float focus_z,
                            float up_x, float up_y, float up_z);

    const glm::mat4* ViewMatrix() { return &_view_matrix; }
    const glm::mat4* ProjectionMatrix() { return &_projection_matrix; }

   protected:
    glm::mat4 _projection_matrix;
    glm::mat4 _view_matrix;
};
