// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "atomic_entry_ref.h"
#include "buffer_type.hpp"
#include <cassert>
#include <cmath>

namespace vespalib::datastore {

namespace {

constexpr float DEFAULT_ALLOC_GROW_FACTOR = 0.2;

}

void
BufferTypeBase::CleanContext::extraBytesCleaned(size_t value)
{
    assert(_extraUsedBytes >= value);
    assert(_extraHoldBytes >= value);
    _extraUsedBytes -= value;
    _extraHoldBytes -= value;
}

BufferTypeBase::BufferTypeBase(uint32_t arraySize,
                               uint32_t minArrays,
                               uint32_t maxArrays,
                               uint32_t numArraysForNewBuffer,
                               float allocGrowFactor) noexcept
    : _arraySize(arraySize),
      _minArrays(std::min(minArrays, maxArrays)),
      _maxArrays(maxArrays),
      _numArraysForNewBuffer(std::min(numArraysForNewBuffer, maxArrays)),
      _allocGrowFactor(allocGrowFactor),
      _holdBuffers(0),
      _holdUsedElems(0),
      _aggr_counts(),
      _active_buffers()
{
}

BufferTypeBase::BufferTypeBase(uint32_t arraySize,
                               uint32_t minArrays,
                               uint32_t maxArrays) noexcept
    : BufferTypeBase(arraySize, minArrays, maxArrays, 0u, DEFAULT_ALLOC_GROW_FACTOR)
{
}

BufferTypeBase::~BufferTypeBase()
{
    assert(_holdBuffers == 0);
    assert(_holdUsedElems == 0);
    assert(_aggr_counts.empty());
    assert(_active_buffers.empty());
}

ElemCount
BufferTypeBase::getReservedElements(uint32_t bufferId) const
{
    return bufferId == 0 ? _arraySize : 0u;
}

void
BufferTypeBase::onActive(uint32_t bufferId, ElemCount* usedElems, ElemCount* deadElems, void* buffer)
{
    _aggr_counts.add_buffer(usedElems, deadElems);
    assert(std::find(_active_buffers.begin(), _active_buffers.end(), bufferId) == _active_buffers.end());
    _active_buffers.emplace_back(bufferId);
    size_t reservedElems = getReservedElements(bufferId);
    if (reservedElems != 0u) {
        initializeReservedElements(buffer, reservedElems);
        *usedElems = reservedElems;
        *deadElems = reservedElems;
    }
}

void
BufferTypeBase::onHold(uint32_t buffer_id, const ElemCount* usedElems, const ElemCount* deadElems)
{
    ++_holdBuffers;
    auto itr = std::find(_active_buffers.begin(), _active_buffers.end(), buffer_id);
    assert(itr != _active_buffers.end());
    _active_buffers.erase(itr);
    _aggr_counts.remove_buffer(usedElems, deadElems);
    _holdUsedElems += *usedElems;
}

void
BufferTypeBase::onFree(ElemCount usedElems)
{
    --_holdBuffers;
    assert(_holdUsedElems >= usedElems);
    _holdUsedElems -= usedElems;
}

void
BufferTypeBase::resume_primary_buffer(uint32_t buffer_id, ElemCount* used_elems, ElemCount* dead_elems)
{
    auto itr = std::find(_active_buffers.begin(), _active_buffers.end(), buffer_id);
    assert(itr != _active_buffers.end());
    _active_buffers.erase(itr);
    _active_buffers.emplace_back(buffer_id);
    _aggr_counts.remove_buffer(used_elems, dead_elems);
    _aggr_counts.add_buffer(used_elems, dead_elems);
}

const alloc::MemoryAllocator*
BufferTypeBase::get_memory_allocator() const
{
    return nullptr;
}

void
BufferTypeBase::clampMaxArrays(uint32_t maxArrays)
{
    _maxArrays = std::min(_maxArrays, maxArrays);
    _minArrays = std::min(_minArrays, _maxArrays);
    _numArraysForNewBuffer = std::min(_numArraysForNewBuffer, _maxArrays);
}

size_t
BufferTypeBase::calcArraysToAlloc(uint32_t bufferId, ElemCount elemsNeeded, bool resizing) const
{
    size_t reservedElems = getReservedElements(bufferId);
    BufferCounts last_bc;
    BufferCounts bc;
    if (resizing) {
        if (!_aggr_counts.empty()) {
            last_bc = _aggr_counts.last_buffer();
        }
    }
    bc = _aggr_counts.all_buffers();
    assert((bc.used_elems % _arraySize) == 0);
    assert((bc.dead_elems % _arraySize) == 0);
    assert(bc.used_elems >= bc.dead_elems);
    size_t neededArrays = (elemsNeeded + (resizing ? last_bc.used_elems : reservedElems) + _arraySize - 1) / _arraySize;

    size_t liveArrays = (bc.used_elems - bc.dead_elems) / _arraySize;
    size_t growArrays = (liveArrays * _allocGrowFactor);
    size_t usedArrays = last_bc.used_elems / _arraySize;
    size_t wantedArrays = std::max((resizing ? usedArrays : 0u) + growArrays,
                                   static_cast<size_t>(_minArrays));

    size_t result = wantedArrays;
    if (result < neededArrays) {
        result = neededArrays;
    }
    if (result > _maxArrays) {
        result = _maxArrays;
    }
    assert(result >= neededArrays);
    return result;
}

uint32_t
BufferTypeBase::get_scaled_num_arrays_for_new_buffer() const
{
    uint32_t active_buffers_count = get_active_buffers_count();
    if (active_buffers_count <= 1u || _numArraysForNewBuffer == 0u) {
        return _numArraysForNewBuffer;
    }
    double scale_factor = std::pow(1.0 + _allocGrowFactor, active_buffers_count - 1);
    double scaled_result = _numArraysForNewBuffer * scale_factor;
    if (scaled_result >= _maxArrays) {
        return _maxArrays;
    }
    return scaled_result;
}

BufferTypeBase::AggregatedBufferCounts::AggregatedBufferCounts()
    : _counts()
{
}

void
BufferTypeBase::AggregatedBufferCounts::add_buffer(const ElemCount* used_elems, const ElemCount* dead_elems)
{
    for (const auto& elem : _counts) {
        assert(elem.used_ptr != used_elems);
        assert(elem.dead_ptr != dead_elems);
    }
    _counts.emplace_back(used_elems, dead_elems);
}

void
BufferTypeBase::AggregatedBufferCounts::remove_buffer(const ElemCount* used_elems, const ElemCount* dead_elems)
{
    auto itr = std::find_if(_counts.begin(), _counts.end(),
                            [=](const auto& elem){ return elem.used_ptr == used_elems; });
    assert(itr != _counts.end());
    assert(itr->dead_ptr == dead_elems);
    _counts.erase(itr);
}

BufferTypeBase::BufferCounts
BufferTypeBase::AggregatedBufferCounts::last_buffer() const
{
    BufferCounts result;
    assert(!_counts.empty());
    const auto& last = _counts.back();
    result.used_elems += *last.used_ptr;
    result.dead_elems += *last.dead_ptr;
    return result;
}

BufferTypeBase::BufferCounts
BufferTypeBase::AggregatedBufferCounts::all_buffers() const
{
    BufferCounts result;
    for (const auto& elem : _counts) {
        result.used_elems += *elem.used_ptr;
        result.dead_elems += *elem.dead_ptr;
    }
    return result;
}

template class BufferType<char>;
template class BufferType<uint8_t>;
template class BufferType<uint32_t>;
template class BufferType<uint64_t>;
template class BufferType<int32_t>;
template class BufferType<std::string>;
template class BufferType<AtomicEntryRef>;

}

