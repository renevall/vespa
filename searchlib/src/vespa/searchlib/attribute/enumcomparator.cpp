// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "enumcomparator.h"
#include <vespa/searchlib/util/foldedstringcompare.h>

namespace search {

namespace {

FoldedStringCompare _strCmp;

}

template <typename EntryT>
EnumStoreComparator<EntryT>::EnumStoreComparator(const DataStoreType& data_store, const EntryT& fallback_value)
    : ParentType(data_store, fallback_value)
{
}

template <typename EntryT>
EnumStoreComparator<EntryT>::EnumStoreComparator(const DataStoreType& data_store)
    : ParentType(data_store)
{
}

template <typename EntryT>
bool
EnumStoreComparator<EntryT>::equal_helper(const EntryT& lhs, const EntryT& rhs)
{
    return vespalib::datastore::UniqueStoreComparatorHelper<EntryT>::equal(lhs, rhs);
}

EnumStoreStringComparator::EnumStoreStringComparator(const DataStoreType& data_store, bool fold)
    : ParentType(data_store, nullptr),
      _fold(fold),
      _prefix(false),
      _prefix_len(0)
{
}

EnumStoreStringComparator::EnumStoreStringComparator(const DataStoreType& data_store, bool fold, const char* fallback_value)
    : ParentType(data_store, fallback_value),
      _fold(fold),
      _prefix(false),
      _prefix_len(0)
{
}

EnumStoreStringComparator::EnumStoreStringComparator(const DataStoreType& data_store, const char* fallback_value, bool prefix)
    : ParentType(data_store, fallback_value),
      _fold(true),
      _prefix(prefix),
      _prefix_len(0)
{
    if (use_prefix()) {
        _prefix_len = _strCmp.size(fallback_value);
    }
}

bool
EnumStoreStringComparator::less(const vespalib::datastore::EntryRef lhs, const vespalib::datastore::EntryRef rhs) const {
    return _fold
        ? (use_prefix()
            ? (_strCmp.compareFoldedPrefix(get(lhs), get(rhs), _prefix_len) < 0)
            : (_strCmp.compareFolded(get(lhs), get(rhs)) < 0))
        : (_strCmp.compare(get(lhs), get(rhs)) < 0);

}

bool
EnumStoreStringComparator::equal(const vespalib::datastore::EntryRef lhs, const vespalib::datastore::EntryRef rhs) const {
    return _fold
        ? (_strCmp.compareFolded(get(lhs), get(rhs)) == 0)
        : (_strCmp.compare(get(lhs), get(rhs)) == 0);
}

template class EnumStoreComparator<int8_t>;
template class EnumStoreComparator<int16_t>;
template class EnumStoreComparator<int32_t>;
template class EnumStoreComparator<int64_t>;
template class EnumStoreComparator<float>;
template class EnumStoreComparator<double>;

}

