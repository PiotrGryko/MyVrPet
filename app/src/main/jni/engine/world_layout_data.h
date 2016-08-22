/* Copyright 2016 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
#define TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT

/*
 * Copyright 2014 Google Inc. All Rights Reserved.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * Contains vertex, normal and color data.
 */

#include <array>

class WorldLayoutData {
 public:

  std::array<float, 72> FLOOR_COORDS;
  std::array<float, 72> FLOOR_NORMALS;
  std::array<float, 96> FLOOR_COLORS;

  WorldLayoutData() {

    // The grid lines on the floor are rendered procedurally and large polygons
    // cause floating point precision problems on some architectures. So we
    // split the floor into 4 quadrants.
    FLOOR_COORDS = {{
      // +X, +Z quadrant
      200.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 200.0f,
      200.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 200.0f,
      200.0f, 0.0f, 200.0f,

      // -X, +Z quadrant
      0.0f, 0.0f, 0.0f,
      -200.0f, 0.0f, 0.0f,
      -200.0f, 0.0f, 200.0f,
      0.0f, 0.0f, 0.0f,
      -200.0f, 0.0f, 200.0f,
      0.0f, 0.0f, 200.0f,

      // +X, -Z quadrant
      200.0f, 0.0f, -200.0f,
      0.0f, 0.0f, -200.0f,
      0.0f, 0.0f, 0.0f,
      200.0f, 0.0f, -200.0f,
      0.0f, 0.0f, 0.0f,
      200.0f, 0.0f, 0.0f,

      // -X, -Z quadrant
      0.0f, 0.0f, -200.0f,
      -200.0f, 0.0f, -200.0f,
      -200.0f, 0.0f, 0.0f,
      0.0f, 0.0f, -200.0f,
      -200.0f, 0.0f, 0.0f,
      0.0f, 0.0f, 0.0f,
      }};

    FLOOR_NORMALS = {{
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      0.0f, 1.0f, 0.0f,
      }};

    FLOOR_COLORS = {{
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      0.0f, 0.3398f, 0.9023f, 1.0f,
      }};
  }
};

#endif  // TREASUREHUNT_APP_SRC_MAIN_JNI_WORLDLAYOUTDATA_H_  // NOLINT
