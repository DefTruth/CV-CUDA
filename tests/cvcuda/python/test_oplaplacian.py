# SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
# SPDX-License-Identifier: Apache-2.0
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import cvcuda
import pytest as t
import numpy as np
import cvcuda_util as util


RNG = np.random.default_rng(0)


@t.mark.parametrize(
    "input, ksize, scale, border",
    [
        (
            cvcuda.Tensor((5, 16, 23, 4), np.uint8, "NHWC"),
            3,
            1.0,
            cvcuda.Border.CONSTANT,
        ),
        (
            cvcuda.Tensor((4, 4, 3), np.float32, "HWC"),
            1,
            0.8,
            cvcuda.Border.REPLICATE,
        ),
        (
            cvcuda.Tensor((3, 88, 13, 3), np.uint16, "NHWC"),
            3,
            1.7,
            cvcuda.Border.REFLECT,
        ),
        (
            cvcuda.Tensor((3, 4, 4), np.uint16, "HWC"),
            1,
            0.5,
            cvcuda.Border.WRAP,
        ),
        (
            cvcuda.Tensor((1, 2, 3, 4), np.uint8, "NHWC"),
            3,
            1.6,
            cvcuda.Border.REFLECT101,
        ),
    ],
)
def test_op_laplacian(input, ksize, scale, border):
    out = cvcuda.laplacian(input, ksize, scale, border)
    assert out.layout == input.layout
    assert out.shape == input.shape
    assert out.dtype == input.dtype

    stream = cvcuda.Stream()
    out = cvcuda.Tensor(input.shape, input.dtype, input.layout)
    tmp = cvcuda.laplacian_into(
        src=input,
        dst=out,
        ksize=ksize,
        scale=scale,
        border=border,
        stream=stream,
    )
    assert tmp is out
    assert out.layout == input.layout
    assert out.shape == input.shape
    assert out.dtype == input.dtype


@t.mark.parametrize(
    "num_images, img_format, img_size, max_pixel, max_ksize, max_scale, border",
    [
        (
            10,
            cvcuda.Format.RGB8,
            (123, 321),
            256,
            3,
            3.0,
            cvcuda.Border.CONSTANT,
        ),
        (
            7,
            cvcuda.Format.RGBf32,
            (62, 35),
            1.0,
            1,
            2.0,
            cvcuda.Border.REPLICATE,
        ),
        (
            1,
            cvcuda.Format.F32,
            (33, 48),
            1234,
            3,
            1.5,
            cvcuda.Border.REFLECT,
        ),
        (
            1,
            cvcuda.Format.U8,
            (23, 18),
            123,
            1,
            1.23,
            cvcuda.Border.WRAP,
        ),
        (
            6,
            cvcuda.Format.F32,
            (77, 42),
            123456,
            3,
            3.21,
            cvcuda.Border.REFLECT101,
        ),
    ],
)
def test_op_laplacianvarshape(
    num_images, img_format, img_size, max_pixel, max_ksize, max_scale, border
):

    input = util.create_image_batch(
        num_images, img_format, size=img_size, max_random=max_pixel, rng=RNG
    )

    ksize = util.create_tensor(
        (num_images, 1),
        np.int32,
        "NC",
        max_random=max_ksize,
        rng=RNG,
        transform_dist=util.dist_odd,
    )

    scale = cvcuda.Tensor(
        (num_images,),
        np.float32,
        "N",
    )

    out = cvcuda.laplacian(
        input,
        ksize,
        scale,
        border,
    )
    assert len(out) == len(input)
    assert out.capacity == input.capacity
    assert out.uniqueformat == input.uniqueformat
    assert out.maxsize == input.maxsize

    stream = cvcuda.Stream()
    out = util.clone_image_batch(input)
    tmp = cvcuda.laplacian_into(
        src=input,
        dst=out,
        ksize=ksize,
        scale=scale,
        border=border,
        stream=stream,
    )
    assert tmp is out
    assert len(out) == len(input)
    assert out.capacity == input.capacity
    assert out.uniqueformat == input.uniqueformat
    assert out.maxsize == input.maxsize
