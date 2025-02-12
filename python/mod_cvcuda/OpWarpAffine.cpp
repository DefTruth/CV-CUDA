/*
 * SPDX-FileCopyrightText: Copyright (c) 2022-2023 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Operators.hpp"

#include <common/PyUtil.hpp>
#include <common/String.hpp>
#include <cvcuda/OpWarpAffine.hpp>
#include <cvcuda/Types.h>
#include <nvcv/cuda/TypeTraits.hpp>
#include <nvcv/python/ImageBatchVarShape.hpp>
#include <nvcv/python/ResourceGuard.hpp>
#include <nvcv/python/Stream.hpp>
#include <nvcv/python/Tensor.hpp>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace cvcudapy {

namespace {

using pyarray = py::array_t<float, py::array::c_style | py::array::forcecast>;

Tensor WarpAffineInto(Tensor &output, Tensor &input, const pyarray &xform, const int32_t flags,
                      const NVCVBorderType borderMode, const pyarray &borderValue, std::optional<Stream> pstream)
{
    if (!pstream)
    {
        pstream = Stream::Current();
    }

    size_t bValueSize = borderValue.size();
    size_t bValueDims = borderValue.ndim();
    if (bValueSize > 4 || bValueDims != 1)
    {
        throw std::runtime_error(util::FormatString(
            "Channels of borderValue should <= 4 and dimension should be 2, current is '%lu', '%lu' respectively",
            bValueSize, bValueDims));
    }
    float4 bValue;
    for (size_t i = 0; i < 4; i++)
    {
        nvcv::cuda::GetElement(bValue, i) = bValueSize > i ? *borderValue.data(i) : 0.f;
    }

    size_t xformDims = xform.ndim();
    if (!(xformDims == 2 && xform.shape(0) == 2 && xform.shape(1) == 3))
    {
        throw std::runtime_error(
            util::FormatString("Details of transformation matrix: nDim == 2, shape == (2, 3) but current is "
                               "'%lu', ('%lu', '%lu') respectively",
                               xformDims, xform.shape(0), xform.shape(1)));
    }

    NVCVAffineTransform xformOutput;
    for (size_t i = 0; i < 2; i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            xformOutput[i * 3 + j] = *xform.data(i, j);
        }
    }

    auto warpAffine = CreateOperator<cvcuda::WarpAffine>(0);

    ResourceGuard guard(*pstream);
    guard.add(LockMode::LOCK_READ, {input});
    guard.add(LockMode::LOCK_WRITE, {output});
    guard.add(LockMode::LOCK_NONE, {*warpAffine});

    warpAffine->submit(pstream->cudaHandle(), input, output, xformOutput, flags, borderMode, bValue);

    return output;
}

Tensor WarpAffine(Tensor &input, const pyarray &xform, const int32_t flags, const NVCVBorderType borderMode,
                  const pyarray &borderValue, std::optional<Stream> pstream)
{
    Tensor output = Tensor::Create(input.shape(), input.dtype());

    return WarpAffineInto(output, input, xform, flags, borderMode, borderValue, pstream);
}

ImageBatchVarShape WarpAffineVarShapeInto(ImageBatchVarShape &output, ImageBatchVarShape &input, Tensor &xform,
                                          const int32_t flags, const NVCVBorderType borderMode,
                                          const pyarray &borderValue, std::optional<Stream> pstream)
{
    if (!pstream)
    {
        pstream = Stream::Current();
    }

    size_t bValueSize = borderValue.size();
    size_t bValueDims = borderValue.ndim();
    if (bValueSize > 4 || bValueDims != 1)
    {
        throw std::runtime_error(util::FormatString(
            "Channels of borderValue should <= 4 and dimension should be 2, current is '%lu', '%lu' respectively",
            bValueSize, bValueDims));
    }
    float4 bValue;
    for (size_t i = 0; i < 4; i++)
    {
        nvcv::cuda::GetElement(bValue, i) = bValueSize > i ? *borderValue.data(i) : 0.f;
    }

    auto warpAffine = CreateOperator<cvcuda::WarpAffine>(input.capacity());

    ResourceGuard guard(*pstream);
    guard.add(LockMode::LOCK_READ, {input, xform});
    guard.add(LockMode::LOCK_WRITE, {output});
    guard.add(LockMode::LOCK_WRITE, {*warpAffine});

    warpAffine->submit(pstream->cudaHandle(), input, output, xform, flags, borderMode, bValue);

    return output;
}

ImageBatchVarShape WarpAffineVarShape(ImageBatchVarShape &input, Tensor &xform, const int32_t flags,
                                      const NVCVBorderType borderMode, const pyarray &borderValue,
                                      std::optional<Stream> pstream)
{
    ImageBatchVarShape output = ImageBatchVarShape::Create(input.capacity());

    for (int i = 0; i < input.numImages(); ++i)
    {
        nvcv::ImageFormat format = input[i].format();
        nvcv::Size2D      size   = input[i].size();
        auto              image  = Image::Create(size, format);
        output.pushBack(image);
    }

    return WarpAffineVarShapeInto(output, input, xform, flags, borderMode, borderValue, pstream);
}

} // namespace

void ExportOpWarpAffine(py::module &m)
{
    using namespace pybind11::literals;

    m.def("warp_affine", &WarpAffine, "src"_a, "xform"_a, "flags"_a, py::kw_only(),
          "border_mode"_a = NVCVBorderType::NVCV_BORDER_CONSTANT, "border_value"_a = 0, "stream"_a = nullptr);

    m.def("warp_affine_into", &WarpAffineInto, "dst"_a, "src"_a, "xform"_a, "flags"_a, py::kw_only(),
          "border_mode"_a = NVCVBorderType::NVCV_BORDER_CONSTANT, "border_value"_a = 0, "stream"_a = nullptr);

    m.def("warp_affine", &WarpAffineVarShape, "src"_a, "xform"_a, "flags"_a, py::kw_only(),
          "border_mode"_a = NVCVBorderType::NVCV_BORDER_CONSTANT, "border_value"_a = 0, "stream"_a = nullptr);

    m.def("warp_affine_into", &WarpAffineVarShapeInto, "dst"_a, "src"_a, "xform"_a, "flags"_a, py::kw_only(),
          "border_mode"_a = NVCVBorderType::NVCV_BORDER_CONSTANT, "border_value"_a = 0, "stream"_a = nullptr);
}

} // namespace cvcudapy
