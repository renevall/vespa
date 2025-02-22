// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include <cstddef>
#include <cstdint>

namespace vespalib { class asciistream; }

namespace vespalib::datastore {

class EntryRef {
protected:
    uint32_t _ref;
public:
    EntryRef() noexcept : _ref(0u) { }
    explicit EntryRef(uint32_t ref_) noexcept : _ref(ref_) { }
    uint32_t ref() const noexcept { return _ref; }
    uint32_t hash() const noexcept { return _ref; }
    bool valid() const noexcept { return _ref != 0u; }
    bool operator==(const EntryRef &rhs) const noexcept { return _ref == rhs._ref; }
    bool operator!=(const EntryRef &rhs) const noexcept { return _ref != rhs._ref; }
    bool operator <(const EntryRef &rhs) const noexcept { return _ref < rhs._ref; }
    bool operator <=(const EntryRef &rhs) const noexcept { return _ref <= rhs._ref; }
};

/**
 * Class for entry reference where we use OffsetBits bits for offset into buffer,
 * and (32 - OffsetBits) bits for buffer id.
 **/
template <uint32_t OffsetBits, uint32_t BufferBits = 32u - OffsetBits>
class EntryRefT : public EntryRef {
public:
    EntryRefT() noexcept : EntryRef() {}
    EntryRefT(size_t offset_, uint32_t bufferId_) noexcept;
    EntryRefT(const EntryRef & ref_) noexcept : EntryRef(ref_.ref()) {}
    size_t offset() const noexcept { return _ref & (offsetSize() - 1); }
    uint32_t bufferId() const noexcept { return _ref >> OffsetBits; }
    static size_t offsetSize() noexcept { return 1ul << OffsetBits; }
    static uint32_t numBuffers() noexcept { return 1 << BufferBits; }
    static size_t align(size_t val) noexcept { return val; }
    static size_t pad(size_t val) noexcept { (void) val; return 0ul; }
    static constexpr bool isAlignedType = false;
    // TODO: Remove following temporary methods when removing
    // AlignedEntryRefT
    size_t unscaled_offset() const noexcept { return offset(); }
    static size_t unscaled_offset_size() noexcept { return offsetSize(); }
};

/**
 * Class for entry reference that is similar to EntryRefT,
 * except that we use (2^OffsetAlign) byte alignment on the offset.
 **/
template <uint32_t OffsetBits, uint32_t OffsetAlign>
class AlignedEntryRefT : public EntryRefT<OffsetBits> {
private:
    typedef EntryRefT<OffsetBits> ParentType;
    static const uint32_t PadConstant = ((1 << OffsetAlign) - 1);
public:
    AlignedEntryRefT() noexcept : ParentType() {}
    AlignedEntryRefT(size_t offset_, uint32_t bufferId_) noexcept :
        ParentType(align(offset_) >> OffsetAlign, bufferId_) {}
    AlignedEntryRefT(const EntryRef & ref_) noexcept : ParentType(ref_) {}
    size_t offset() const { return ParentType::offset() << OffsetAlign; }
    static size_t offsetSize() { return ParentType::offsetSize() << OffsetAlign; }
    static size_t align(size_t val) { return val + pad(val); }
    static size_t pad(size_t val) { return (-val & PadConstant); }
    static constexpr bool isAlignedType = true;
};

vespalib::asciistream& operator<<(vespalib::asciistream& os, const EntryRef& ref);

}
