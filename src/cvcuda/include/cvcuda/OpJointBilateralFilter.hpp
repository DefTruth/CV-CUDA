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

/**
 * @file OpJointBilateralFilter.hpp
 *
 * @brief Defines the public C++ Class for the JointBilateralFilter operation.
 * @defgroup NVCV_CPP_ALGORITHM_JOINT_BILATERAL_FILTER JointBilateralFilter
 * @{
 */

#ifndef CVCUDA_JOINT_BILATERAL_FILTER_HPP
#define CVCUDA_JOINT_BILATERAL_FILTER_HPP

#include "IOperator.hpp"
#include "OpJointBilateralFilter.h"

#include <cuda_runtime.h>
#include <nvcv/IImageBatch.hpp>
#include <nvcv/ITensor.hpp>
#include <nvcv/ImageFormat.hpp>
#include <nvcv/Size.hpp>
#include <nvcv/alloc/Requirements.hpp>

namespace cvcuda {

class JointBilateralFilter final : public IOperator
{
public:
    explicit JointBilateralFilter();

    ~JointBilateralFilter();

    void operator()(cudaStream_t stream, nvcv::ITensor &in, nvcv::ITensor &inColor, nvcv::ITensor &out, int diameter,
                    float sigmaColor, float sigmaSpace, NVCVBorderType borderMode);

    void operator()(cudaStream_t stream, nvcv::IImageBatch &in, nvcv::IImageBatch &inColor, nvcv::IImageBatch &out,
                    nvcv::ITensor &diameterData, nvcv::ITensor &sigmaColorData, nvcv::ITensor &sigmaSpace,
                    NVCVBorderType borderMode);

    virtual NVCVOperatorHandle handle() const noexcept override;

private:
    NVCVOperatorHandle m_handle;
};

inline JointBilateralFilter::JointBilateralFilter()
{
    nvcv::detail::CheckThrow(cvcudaJointBilateralFilterCreate(&m_handle));
    assert(m_handle);
}

inline JointBilateralFilter::~JointBilateralFilter()
{
    nvcvOperatorDestroy(m_handle);
    m_handle = nullptr;
}

inline void JointBilateralFilter::operator()(cudaStream_t stream, nvcv::ITensor &in, nvcv::ITensor &inColor,
                                             nvcv::ITensor &out, int diameter, float sigmaColor, float sigmaSpace,
                                             NVCVBorderType borderMode)
{
    nvcv::detail::CheckThrow(cvcudaJointBilateralFilterSubmit(
        m_handle, stream, in.handle(), inColor.handle(), out.handle(), diameter, sigmaColor, sigmaSpace, borderMode));
}

inline void JointBilateralFilter::operator()(cudaStream_t stream, nvcv::IImageBatch &in, nvcv::IImageBatch &inColor,
                                             nvcv::IImageBatch &out, nvcv::ITensor &diameterData,
                                             nvcv::ITensor &sigmaColorData, nvcv::ITensor &sigmaSpaceData,
                                             NVCVBorderType borderMode)
{
    nvcv::detail::CheckThrow(cvcudaJointBilateralFilterVarShapeSubmit(
        m_handle, stream, in.handle(), inColor.handle(), out.handle(), diameterData.handle(), sigmaColorData.handle(),
        sigmaSpaceData.handle(), borderMode));
}

inline NVCVOperatorHandle JointBilateralFilter::handle() const noexcept
{
    return m_handle;
}

} // namespace cvcuda

#endif // CVCUDA_JOINT_BILATERAL_FILTER_HPP
