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

#ifndef NVCV_TEST_COMMON_HASHMD5_HPP
#define NVCV_TEST_COMMON_HASHMD5_HPP

#include <util/Ranges.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

namespace nvcv::test {

class HashMD5
{
public:
    HashMD5();
    HashMD5(const HashMD5 &) = delete;
    ~HashMD5();

    void                    operator()(const void *data, size_t lenBytes);
    std::array<uint8_t, 16> getHashAndReset();

    template<class T>
    void operator()(const T &value)
    {
        static_assert(std::has_unique_object_representations_v<T>, "Can't hash this type");
        this->operator()(&value, sizeof(value));
    }

private:
    struct Impl;
    std::unique_ptr<Impl> pimpl;
};

template<class T>
std::enable_if_t<std::has_unique_object_representations_v<T> && !util::ranges::IsRange<T>> Update(HashMD5 &hash,
                                                                                                  const T &value)
{
    hash(value);
}

template<class T>
void Update(HashMD5 &hash, const T *value)
{
    static_assert(sizeof(T) == 0, "Won't do md5 of a pointer");
}

void Update(HashMD5 &hash, const char *value);

template<class R, std::enable_if_t<util::ranges::IsRange<R>, int> = 0>
void Update(nvcv::test::HashMD5 &hash, const R &r)
{
    Update(hash, util::ranges::Size(r));
    // With C++20 we should use std::ranges::contiguous_range instead
    if constexpr (util::ranges::IsRandomAccessRange<
                      R> && std::has_unique_object_representations_v<util::ranges::RangeValue<R>>)
    {
        // It's faster to do this if range is contiguous and elements have unique object representation
        hash(util::ranges::Data(r), util::ranges::Size(r) * sizeof(util::ranges::RangeValue<R>));
    }
    else
    {
        // Must go one by one
        for (auto &v : r)
        {
            Update(hash, v);
        }
    }
}

template<class T1, class T2, class... TT>
void Update(HashMD5 &hash, const T1 &v1, const T2 &v2, const TT &...v)
{
    Update(hash, v1);
    Update(hash, v2);

    (..., Update(hash, v));
}

template<class T>
std::enable_if_t<std::is_floating_point_v<T>> Update(HashMD5 &hash, const T &value)
{
    hash(std::hash<T>()(value));
}

} // namespace nvcv::test

namespace std {

template<typename... TT>
void Update(nvcv::test::HashMD5 &hash, const tuple<TT...> &t)
{
    if constexpr (has_unique_object_representations_v<tuple<TT...>>)
    {
        return hash(t);
    }

    auto th = forward_as_tuple(hash);

    apply(nvcv::test::Update<TT...>, tuple_cat(th, t));
};

inline void Update(nvcv::test::HashMD5 &hash, const string &s)
{
    return hash(s.data(), s.size());
}

inline void Update(nvcv::test::HashMD5 &hash, const string_view &s)
{
    return hash(s.data(), s.size());
}

inline void Update(nvcv::test::HashMD5 &hash, const std::type_info &t)
{
    return hash(t.hash_code());
}

template<class T>
void Update(nvcv::test::HashMD5 &hash, const optional<T> &o)
{
    using nvcv::test::Update;

    // We can't rely on std::hash<T> for optionals because they
    // require a valid hash specialization for T. Since our
    // types use HashValue overloads, we have to do this instead.
    if (o)
    {
        return Update(hash, *o);
    }
    else
    {
        return Update(hash, std::hash<optional<int>>()(nullopt));
    }
}

} // namespace std

#endif // NVCV_TEST_COMMON_HASHMD5_HPP
