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
 * @file OpChannelReorder.hpp
 *
 * @brief Defines the public C++ Class for the channel reorder operation.
 * @defgroup NVCV_CPP_ALGORITHM_CHANNEL_REORDER Channel Reorder
 * @{
 */

#ifndef CVCUDA_CHANNEL_REORDER_HPP
#define CVCUDA_CHANNEL_REORDER_HPP

#include "IOperator.hpp"
#include "OpChannelReorder.h"

#include <cuda_runtime.h>
#include <nvcv/IImageBatch.hpp>
#include <nvcv/ITensor.hpp>
#include <nvcv/alloc/Requirements.hpp>

namespace cvcuda {

class ChannelReorder final : public IOperator
{
public:
    explicit ChannelReorder();

    ~ChannelReorder();

    void operator()(cudaStream_t stream, nvcv::IImageBatchVarShape &in, nvcv::IImageBatchVarShape &out,
                    nvcv::ITensor &orders);

    virtual NVCVOperatorHandle handle() const noexcept override;

private:
    NVCVOperatorHandle m_handle;
};

inline ChannelReorder::ChannelReorder()
{
    nvcv::detail::CheckThrow(cvcudaChannelReorderCreate(&m_handle));
    assert(m_handle);
}

inline ChannelReorder::~ChannelReorder()
{
    nvcvOperatorDestroy(m_handle);
    m_handle = nullptr;
}

inline void ChannelReorder::operator()(cudaStream_t stream, nvcv::IImageBatchVarShape &in,
                                       nvcv::IImageBatchVarShape &out, nvcv::ITensor &orders)
{
    nvcv::detail::CheckThrow(
        cvcudaChannelReorderVarShapeSubmit(m_handle, stream, in.handle(), out.handle(), orders.handle()));
}

inline NVCVOperatorHandle ChannelReorder::handle() const noexcept
{
    return m_handle;
}

} // namespace cvcuda

#endif // CVCUDA_CHANNEL_REORDER_HPP
