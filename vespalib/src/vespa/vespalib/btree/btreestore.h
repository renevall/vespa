// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#pragma once

#include "btreenode.h"
#include "btreebuilder.h"
#include "btreeroot.h"
#include "noaggrcalc.h"
#include "minmaxaggrcalc.h"
#include <vespa/vespalib/datastore/datastore.h>
#include <vespa/vespalib/datastore/handle.h>

namespace vespalib::btree {

template <typename KeyT,
          typename DataT,
          typename AggrT,
          typename CompareT,
          typename TraitsT,
          typename AggrCalcT = NoAggrCalc>
class BTreeStore
{
public:
    typedef KeyT KeyType;
    typedef DataT DataType;
    typedef AggrT AggregatedType;
    typedef datastore::DataStoreT<datastore::EntryRefT<22> > DataStoreType;
    typedef DataStoreType::RefType RefType;
    typedef BTreeKeyData<KeyT, DataT> KeyDataType;

    typedef BTreeRoot<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT> BTreeType;
    typedef BTreeInternalNode<KeyT, AggrT,
                              TraitsT::INTERNAL_SLOTS> InternalNodeType;
    typedef BTreeLeafNode<KeyT, DataT, AggrT, TraitsT::LEAF_SLOTS>
    LeafNodeType;
    typedef datastore::Handle<BTreeType> BTreeTypeRefPair;
    typedef datastore::Handle<KeyDataType> KeyDataTypeRefPair;
    typedef typename InternalNodeType::RefPair InternalNodeTypeRefPair;
    typedef typename LeafNodeType::RefPair LeafNodeTypeRefPair;
    typedef vespalib::GenerationHandler::generation_t generation_t;
    typedef BTreeNodeAllocator<KeyT, DataT, AggrT,
                               TraitsT::INTERNAL_SLOTS,
                               TraitsT::LEAF_SLOTS> NodeAllocatorType;
    typedef typename BTreeType::Iterator Iterator;
    typedef typename BTreeType::ConstIterator ConstIterator;
    typedef const KeyDataType * AddIter;
    typedef const KeyType * RemoveIter;
    typedef BTreeBuilder<KeyT, DataT, AggrT,
                         TraitsT::INTERNAL_SLOTS,
                         TraitsT::LEAF_SLOTS,
                         AggrCalcT> Builder;
    using EntryRef = datastore::EntryRef;
    template <typename EntryType>
    using BufferType = datastore::BufferType<EntryType>;
    using BufferState = datastore::BufferState;

    static constexpr uint32_t clusterLimit = 8;
    
    enum BufferTypes
    {
        BUFFERTYPE_ARRAY1 = 0,
        BUFFERTYPE_ARRAY2 = 1,
        BUFFERTYPE_ARRAY3 = 2,
        BUFFERTYPE_ARRAY4 = 3,
        BUFFERTYPE_ARRAY5 = 4,
        BUFFERTYPE_ARRAY6 = 5,
        BUFFERTYPE_ARRAY7 = 6,
        BUFFERTYPE_ARRAY8 = 7,
        BUFFERTYPE_BTREE = 8
    };
protected:
    struct TreeReclaimer {
        static void reclaim(BTreeType * tree) {
            tree->recycle();
        }
    };

    DataStoreType _store;
    BufferType<BTreeType> _treeType;
    BufferType<KeyDataType> _small1Type;
    BufferType<KeyDataType> _small2Type;
    BufferType<KeyDataType> _small3Type;
    BufferType<KeyDataType> _small4Type;
    BufferType<KeyDataType> _small5Type;
    BufferType<KeyDataType> _small6Type;
    BufferType<KeyDataType> _small7Type;
    BufferType<KeyDataType> _small8Type;
    NodeAllocatorType _allocator;
    AggrCalcT _aggrCalc;
    Builder _builder;

    BTreeType * getWTreeEntry(RefType ref) {
        return _store.getEntry<BTreeType>(ref);
    }

public:
    BTreeStore();

    BTreeStore(bool init);

    ~BTreeStore();

    const NodeAllocatorType &getAllocator() const { return _allocator; }

    void
    disableFreeLists() {
        _store.disableFreeLists();
        _allocator.disableFreeLists();
    }

    void
    disableElemHoldList()
    {
        _store.disableElemHoldList();
        _allocator.disableElemHoldList();
    }

    BTreeTypeRefPair
    allocNewBTree() {
        return _store.allocator<BTreeType>(BUFFERTYPE_BTREE).alloc();
    }

    BTreeTypeRefPair
    allocBTree() {
        return _store.freeListAllocator<BTreeType, TreeReclaimer>(BUFFERTYPE_BTREE).alloc();
    }

    BTreeTypeRefPair
    allocNewBTreeCopy(const BTreeType &rhs) {
        return _store.allocator<BTreeType>(BUFFERTYPE_BTREE).alloc(rhs);
    }

    BTreeTypeRefPair
    allocBTreeCopy(const BTreeType &rhs) {
        return _store.freeListAllocator<BTreeType, datastore::DefaultReclaimer<BTreeType> >(BUFFERTYPE_BTREE).alloc(rhs);
    }

    KeyDataTypeRefPair
    allocNewKeyData(uint32_t clusterSize);

    KeyDataTypeRefPair
    allocKeyData(uint32_t clusterSize);

    KeyDataTypeRefPair
    allocNewKeyDataCopy(const KeyDataType *rhs, uint32_t clusterSize);

    KeyDataTypeRefPair
    allocKeyDataCopy(const KeyDataType *rhs, uint32_t clusterSize);

    std::vector<uint32_t>
    startCompact();

    void
    finishCompact(const std::vector<uint32_t> &toHold);


    const KeyDataType *
    lower_bound(const KeyDataType *b, const KeyDataType *e,
                const KeyType &key, CompareT comp);

    void
    makeTree(EntryRef &ref,
             const KeyDataType *array, uint32_t clusterSize);

    void
    makeArray(EntryRef &ref, EntryRef leafRef, LeafNodeType *leafNode);

    bool
    insert(EntryRef &ref,
           const KeyType &key, const DataType &data,
           CompareT comp = CompareT());

    bool
    remove(EntryRef &ref,
           const KeyType &key,
           CompareT comp = CompareT());

    uint32_t
    getNewClusterSize(const KeyDataType *o,
                      const KeyDataType *oe,
                      AddIter a,
                      AddIter ae,
                      RemoveIter r,
                      RemoveIter re,
                      CompareT comp);

    void
    applyCluster(const KeyDataType *o,
                 const KeyDataType *oe,
                 KeyDataType *d,
                 const KeyDataType *de,
                 AddIter a,
                 AddIter ae,
                 RemoveIter r,
                 RemoveIter re,
                 CompareT comp);


    void
    applyModifyTree(BTreeType *tree,
                    AddIter a,
                    AddIter ae,
                    RemoveIter r,
                    RemoveIter re,
                    CompareT comp);

    void
    applyBuildTree(BTreeType *tree,
                   AddIter a,
                   AddIter ae,
                   RemoveIter r,
                   RemoveIter re,
                   CompareT comp);

    void
    applyNewArray(EntryRef &ref,
                  AddIter aOrg,
                  AddIter ae);
    
    void
    applyNewTree(EntryRef &ref,
                 AddIter a,
                 AddIter ae,
                 CompareT comp);
    
    void
    applyNew(EntryRef &ref,
             AddIter a,
             AddIter ae,
             CompareT comp);


    bool
    applyCluster(EntryRef &ref,
                 uint32_t clusterSize,
                 AddIter a,
                 AddIter ae,
                 RemoveIter r,
                 RemoveIter re,
                 CompareT comp);

    void
    applyTree(BTreeType *tree,
              AddIter a,
              AddIter ae,
              RemoveIter r,
              RemoveIter re,
              CompareT comp);

    void
    normalizeTree(EntryRef &ref,
                  BTreeType *tree,
                  bool wasArray);
    /**
     * Apply multiple changes at once.
     *
     * additions and removals should be sorted on key without duplicates.
     * Overlap between additions and removals indicates updates.
     */
    void
    apply(EntryRef &ref,
          AddIter a,
          AddIter ae,
          RemoveIter r,
          RemoveIter re,
          CompareT comp = CompareT());

    void
    clear(const EntryRef ref);

    size_t
    size(const EntryRef ref) const;

    size_t
    frozenSize(const EntryRef ref) const;

    Iterator
    begin(const EntryRef ref) const;

    ConstIterator
    beginFrozen(const EntryRef ref) const;

    void
    beginFrozen(const EntryRef ref, std::vector<ConstIterator> &where) const;

    uint32_t
    getTypeId(RefType ref) const
    {
        return _store.getBufferState(ref.bufferId()).getTypeId();
    }

    static bool
    isSmallArray(uint32_t typeId)
    {
        return typeId < clusterLimit;
    }

    bool
    isSmallArray(const EntryRef ref) const;

    /**
     * Returns the cluster size for the type id.
     * Cluster size == 0 means we have a tree for the given reference.
     * The reference must be valid.
     **/
    static uint32_t
    getClusterSize(uint32_t typeId)
    {
        return (typeId < clusterLimit) ? typeId + 1 : 0;
    }

    /**
     * Returns the cluster size for the entry pointed to by the given reference.
     * Cluster size == 0 means we have a tree for the given reference.
     * The reference must be valid.
     **/
    uint32_t
    getClusterSize(RefType ref) const
    {
        return getClusterSize(getTypeId(ref));
    }

    const BTreeType * getTreeEntry(RefType ref) const {
        return _store.getEntry<BTreeType>(ref);
    }

    const KeyDataType * getKeyDataEntry(RefType ref, uint32_t arraySize) const {
        return _store.getEntryArray<KeyDataType>(ref, arraySize);
    }

    void freeze() {
        _allocator.freeze();
    }

    // Inherit doc from DataStoreBase
    void
    trimHoldLists(generation_t usedGen)
    {
        _allocator.trimHoldLists(usedGen);
        _store.trimHoldLists(usedGen);
    }

    // Inherit doc from DataStoreBase
    void
    transferHoldLists(generation_t generation)
    {
        _allocator.transferHoldLists(generation);
        _store.transferHoldLists(generation);
    }

    void
    clearHoldLists()
    {
        _allocator.clearHoldLists();
        _store.clearHoldLists();
    }


    // Inherit doc from DataStoreBase
    vespalib::MemoryUsage getMemoryUsage() const {
        vespalib::MemoryUsage usage;
        usage.merge(_allocator.getMemoryUsage());
        usage.merge(_store.getMemoryUsage());
        return usage;
    }

    void
    clearBuilder()
    {
        _builder.clear();
    }

    AggregatedType
    getAggregated(const EntryRef ref) const;

    template <typename FunctionType>
    void
    foreach_unfrozen_key(EntryRef ref, FunctionType func) const;

    template <typename FunctionType>
    void
    foreach_frozen_key(EntryRef ref, FunctionType func) const;

    template <typename FunctionType>
    void
    foreach_unfrozen(EntryRef ref, FunctionType func) const;

    template <typename FunctionType>
    void
    foreach_frozen(EntryRef ref, FunctionType func) const;

    std::vector<uint32_t> start_compact_worst_btree_nodes();
    void finish_compact_worst_btree_nodes(const std::vector<uint32_t>& to_hold);
    void move_btree_nodes(EntryRef ref);

    std::vector<uint32_t> start_compact_worst_buffers();
    EntryRef move(EntryRef ref);

private:
    static constexpr size_t MIN_BUFFER_ARRAYS = 128u;
    template <typename FunctionType, bool Frozen>
    void
    foreach_key(EntryRef ref, FunctionType func) const;

    template <typename FunctionType, bool Frozen>
    void
    foreach(EntryRef ref, FunctionType func) const;
};

template <typename KeyT, typename DataT, typename AggrT, typename CompareT,
          typename TraitsT, typename AggrCalcT>
template <typename FunctionType>
void
BTreeStore<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT>::
foreach_unfrozen_key(EntryRef ref, FunctionType func) const {
    foreach_key<FunctionType, false>(ref, func);
}

template <typename KeyT, typename DataT, typename AggrT, typename CompareT,
          typename TraitsT, typename AggrCalcT>
template <typename FunctionType>
void
BTreeStore<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT>::
foreach_frozen_key(EntryRef ref, FunctionType func) const
{
    foreach_key<FunctionType, true>(ref, func);
}

template <typename KeyT, typename DataT, typename AggrT, typename CompareT,
        typename TraitsT, typename AggrCalcT>
template <typename FunctionType>
void
BTreeStore<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT>::
foreach_unfrozen(EntryRef ref, FunctionType func) const
{
    foreach<FunctionType, false>(ref, func);
}


template <typename KeyT, typename DataT, typename AggrT, typename CompareT,
          typename TraitsT, typename AggrCalcT>
template <typename FunctionType>
void
BTreeStore<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT>::
foreach_frozen(EntryRef ref, FunctionType func) const
{
    foreach<FunctionType, true>(ref, func);
}

template <typename KeyT, typename DataT, typename AggrT, typename CompareT,
        typename TraitsT, typename AggrCalcT>
template <typename FunctionType, bool Frozen>
void
BTreeStore<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT>::
foreach_key(EntryRef ref, FunctionType func) const
{
    if (!ref.valid())
        return;
    RefType iRef(ref);
    uint32_t clusterSize = getClusterSize(iRef);
    if (clusterSize == 0) {
        const BTreeType *tree = getTreeEntry(iRef);
        _allocator.getNodeStore().foreach_key(Frozen ? tree->getFrozenRoot() : tree->getRoot(), func);
    } else {
        const KeyDataType *p = getKeyDataEntry(iRef, clusterSize);
        const KeyDataType *pe = p + clusterSize;
        for (; p != pe; ++p) {
            func(p->_key);
        }
    }
}

template <typename KeyT, typename DataT, typename AggrT, typename CompareT,
          typename TraitsT, typename AggrCalcT>
template <typename FunctionType, bool Frozen>
void
BTreeStore<KeyT, DataT, AggrT, CompareT, TraitsT, AggrCalcT>::
foreach(EntryRef ref, FunctionType func) const
{
    if (!ref.valid())
        return;
    RefType iRef(ref);
    uint32_t clusterSize = getClusterSize(iRef);
    if (clusterSize == 0) {
        const BTreeType *tree = getTreeEntry(iRef);
        _allocator.getNodeStore().foreach(Frozen ? tree->getFrozenRoot() : tree->getRoot(), func);
    } else {
        const KeyDataType *p = getKeyDataEntry(iRef, clusterSize);
        const KeyDataType *pe = p + clusterSize;
        for (; p != pe; ++p) {
            func(p->_key, p->getData());
        }
    }
}


extern template class BTreeStore<uint32_t, uint32_t,
                                 NoAggregated,
                                 std::less<uint32_t>,
                                 BTreeDefaultTraits>;

extern template class BTreeStore<uint32_t, BTreeNoLeafData,
                                 NoAggregated,
                                 std::less<uint32_t>,
                                 BTreeDefaultTraits>;

extern template class BTreeStore<uint32_t, int32_t,
                                 MinMaxAggregated,
                                 std::less<uint32_t>,
                                 BTreeDefaultTraits,
                                 MinMaxAggrCalc>;

}

namespace vespalib::datastore {

using namespace btree;

extern template class BufferType<BTreeRoot<uint32_t, uint32_t, NoAggregated, std::less<uint32_t>, BTreeDefaultTraits>>;
extern template class BufferType<BTreeRoot<uint32_t, BTreeNoLeafData, NoAggregated, std::less<uint32_t>, BTreeDefaultTraits>>;
extern template class BufferType<BTreeRoot<uint32_t, int32_t, MinMaxAggregated, std::less<uint32_t>, BTreeDefaultTraits, MinMaxAggrCalc>>;

}
