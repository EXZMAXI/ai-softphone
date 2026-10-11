// AI Softphone - a cross-platform desktop SIP softphone with an AI enrichment layer.
// https://ai-softphone.com
//
// SPDX-FileCopyrightText: Copyright (C) 2026 AI-Softphone Developers Team
// SPDX-FileCopyrightText: Copyright (C) 2026 Ilya Pleshchinskiy
// SPDX-License-Identifier: GPL-2.0-or-later
//
// This file is part of AI Softphone. There is NO WARRANTY, to the extent
// permitted by law. See the file LICENSE for the full terms.

#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace softphone::http::protobuf {

enum class Wire : std::uint8_t {
    Varint = 0,
    Fixed64 = 1,
    Bytes = 2,
    Fixed32 = 5,
};

inline void putVarint(std::string &into, std::uint64_t value)
{
    while (value >= 0x80) {
        into.push_back(static_cast<char>(static_cast<std::uint8_t>(value) | 0x80U));
        value >>= 7;
    }
    into.push_back(static_cast<char>(static_cast<std::uint8_t>(value)));
}

inline void putKey(std::string &into, unsigned field, Wire wire)
{
    putVarint(into, (static_cast<std::uint64_t>(field) << 3) | static_cast<std::uint8_t>(wire));
}

inline void putInt(std::string &into, unsigned field, std::int64_t value)
{
    putKey(into, field, Wire::Varint);
    putVarint(into, static_cast<std::uint64_t>(value));
}

inline void putEnum(std::string &into, unsigned field, int value)
{
    putInt(into, field, value);
}

inline void putBool(std::string &into, unsigned field, bool value)
{
    putInt(into, field, value ? 1 : 0);
}

inline void putBytes(std::string &into, unsigned field, std::string_view value)
{
    putKey(into, field, Wire::Bytes);
    putVarint(into, value.size());
    into.append(value);
}

inline void putBytes(std::string &into, unsigned field, std::span<const std::byte> value)
{
    putKey(into, field, Wire::Bytes);
    putVarint(into, value.size());
    into.append(reinterpret_cast<const char *>(value.data()), value.size());
}

inline void putMessage(std::string &into, unsigned field, const std::string &message)
{
    putBytes(into, field, std::string_view{message});
}

struct Field {
    unsigned number{0};
    Wire wire{Wire::Varint};

    std::uint64_t number64{0};

    std::string_view bytes;
};

class Reader {
public:
    explicit Reader(std::string_view from) noexcept : from_{from} {}

    [[nodiscard]] bool done() const noexcept
    {
        return broken_ || at_ >= from_.size();
    }

    [[nodiscard]] bool broken() const noexcept
    {
        return broken_;
    }

    [[nodiscard]] std::optional<Field> next() noexcept
    {
        if (done()) {
            return std::nullopt;
        }

        std::uint64_t key = 0;
        if (!varint(key)) {
            return std::nullopt;
        }

        Field found;
        found.number = static_cast<unsigned>(key >> 3);
        const auto wire = static_cast<std::uint8_t>(key & 0x07U);

        switch (wire) {
        case 0:
            found.wire = Wire::Varint;
            if (!varint(found.number64)) {
                return std::nullopt;
            }
            return found;
        case 1:
            found.wire = Wire::Fixed64;
            if (!fixed(8, found.number64)) {
                return std::nullopt;
            }
            return found;
        case 2: {
            found.wire = Wire::Bytes;
            std::uint64_t length = 0;
            if (!varint(length) || length > from_.size() - at_) {
                broken_ = true;
                return std::nullopt;
            }
            found.bytes = from_.substr(at_, static_cast<std::size_t>(length));
            at_ += static_cast<std::size_t>(length);
            return found;
        }
        case 5:
            found.wire = Wire::Fixed32;
            if (!fixed(4, found.number64)) {
                return std::nullopt;
            }
            return found;
        default:

            broken_ = true;
            return std::nullopt;
        }
    }

private:
    [[nodiscard]] bool varint(std::uint64_t &into) noexcept
    {
        into = 0;
        unsigned shift = 0;
        while (at_ < from_.size()) {
            const auto byte = static_cast<std::uint8_t>(from_[at_++]);
            into |= static_cast<std::uint64_t>(byte & 0x7FU) << shift;
            if ((byte & 0x80U) == 0) {
                return true;
            }
            shift += 7;
            if (shift >= 64) {
                broken_ = true;
                return false;
            }
        }
        broken_ = true;
        return false;
    }

    [[nodiscard]] bool fixed(std::size_t width, std::uint64_t &into) noexcept
    {
        if (from_.size() - at_ < width) {
            broken_ = true;
            return false;
        }
        into = 0;
        for (std::size_t which = 0; which < width; ++which) {
            into |= static_cast<std::uint64_t>(static_cast<std::uint8_t>(from_[at_ + which]))
                    << (8U * which);
        }
        at_ += width;
        return true;
    }

    std::string_view from_;
    std::size_t at_{0};
    bool broken_{false};
};

[[nodiscard]] inline double asDouble(std::uint64_t bits) noexcept
{
    double value = 0.0;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

[[nodiscard]] inline std::int64_t asInt(std::uint64_t bits) noexcept
{
    return static_cast<std::int64_t>(bits);
}

}
