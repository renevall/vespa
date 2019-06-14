// Copyright 2017 Yahoo Holdings. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/searchlib/common/sequencedtaskexecutor.h>
#include <vespa/searchlib/diskindex/diskindex.h>
#include <vespa/searchlib/diskindex/fusion.h>
#include <vespa/searchlib/diskindex/indexbuilder.h>
#include <vespa/searchlib/diskindex/zcposoccrandread.h>
#include <vespa/searchlib/fef/fieldpositionsiterator.h>
#include <vespa/searchlib/fef/termfieldmatchdata.h>
#include <vespa/searchlib/index/docbuilder.h>
#include <vespa/searchlib/index/dummyfileheadercontext.h>
#include <vespa/searchlib/memoryindex/document_inverter.h>
#include <vespa/searchlib/memoryindex/field_index_collection.h>
#include <vespa/searchlib/memoryindex/posting_iterator.h>
#include <vespa/searchlib/test/index/mock_field_length_inspector.h>
#include <vespa/searchlib/util/filekit.h>
#include <vespa/vespalib/btree/btreenode.hpp>
#include <vespa/vespalib/btree/btreenodeallocator.hpp>
#include <vespa/vespalib/btree/btreeroot.hpp>
#include <vespa/vespalib/io/fileutil.h>
#include <vespa/vespalib/util/threadstackexecutor.h>
#include <gtest/gtest.h>

#include <vespa/log/log.h>
LOG_SETUP("fusion_test");

namespace search {

using document::Document;
using fef::FieldPositionsIterator;
using fef::TermFieldMatchData;
using fef::TermFieldMatchDataArray;
using memoryindex::DocumentInverter;
using memoryindex::FieldIndexCollection;
using queryeval::SearchIterator;
using search::common::FileHeaderContext;
using search::index::schema::CollectionType;
using search::index::schema::DataType;
using search::index::test::MockFieldLengthInspector;

using namespace index;

namespace diskindex {

class MyMockFieldLengthInspector : public IFieldLengthInspector {
    FieldLengthInfo get_field_length_info(const vespalib::string& field_name) const override {
        if (field_name == "f0") {
            return FieldLengthInfo(3.5, 21);
        } else {
            return FieldLengthInfo();
        }
    }
};

class FusionTest : public ::testing::Test
{
protected:
    Schema _schema;
    const Schema & getSchema() const { return _schema; }

    void requireThatFusionIsWorking(const vespalib::string &prefix, bool directio, bool readmmap);
    void make_empty_index(const vespalib::string &dump_dir, const IFieldLengthInspector &field_length_inspector);
    void merge_empty_indexes(const vespalib::string &dump_dir, const std::vector<vespalib::string> &sources);
public:
    FusionTest();
};

namespace {

void
myPushDocument(DocumentInverter &inv)
{
    inv.pushDocuments(std::shared_ptr<IDestructorCallback>());
}

}

vespalib::string
toString(FieldPositionsIterator posItr, bool hasElements = false, bool hasWeights = false)
{
    vespalib::asciistream ss;
    ss << "{";
    ss << posItr.getFieldLength() << ":";
    bool first = true;
    for (; posItr.valid(); posItr.next()) {
        if (!first) ss << ",";
        ss << posItr.getPosition();
        first = false;
        if (hasElements) {
            ss << "[e=" << posItr.getElementId();
            if (hasWeights)
                ss << ",w=" << posItr.getElementWeight();
            ss << ",l=" << posItr.getElementLen() << "]";
        }
    }
    ss << "}";
    return ss.str();
}

void
validateDiskIndex(DiskIndex &dw, bool f2HasElements, bool f3HasWeights)
{
    typedef DiskIndex::LookupResult LR;
    typedef index::PostingListHandle PH;
    typedef search::queryeval::SearchIterator SB;

    const Schema &schema(dw.getSchema());

    {
        uint32_t id1(schema.getIndexFieldId("f0"));
        LR::UP lr1(dw.lookup(id1, "c"));
        ASSERT_TRUE(lr1);
        PH::UP wh1(dw.readPostingList(*lr1));
        ASSERT_TRUE(wh1);
        TermFieldMatchData f0;
        TermFieldMatchDataArray a;
        a.add(&f0);
        SB::UP sbap(wh1->createIterator(lr1->counts, a));
        sbap->initFullRange();
        EXPECT_EQ(vespalib::string("{1000000:}"), toString(f0.getIterator()));
        EXPECT_TRUE(sbap->seek(10));
        sbap->unpack(10);
        EXPECT_EQ(vespalib::string("{7:2}"), toString(f0.getIterator()));
    }
    {
        uint32_t id1(schema.getIndexFieldId("f2"));
        LR::UP lr1(dw.lookup(id1, "ax"));
        ASSERT_TRUE(lr1);
        PH::UP wh1(dw.readPostingList(*lr1));
        ASSERT_TRUE(wh1);
        TermFieldMatchData f2;
        TermFieldMatchDataArray a;
        a.add(&f2);
        SB::UP sbap(wh1->createIterator(lr1->counts, a));
        sbap->initFullRange();
        EXPECT_EQ(vespalib::string("{1000000:}"), toString(f2.getIterator()));
        EXPECT_TRUE(sbap->seek(10));
        sbap->unpack(10);
        if (f2HasElements) {
            EXPECT_EQ(vespalib::string("{3:0[e=0,l=3],0[e=1,l=1]}"),
                      toString(f2.getIterator(), true));
        } else {
            EXPECT_EQ(vespalib::string("{3:0[e=0,l=3]}"),
                      toString(f2.getIterator(), true));
        }
    }
    {
        uint32_t id1(schema.getIndexFieldId("f3"));
        LR::UP lr1(dw.lookup(id1, "wx"));
        ASSERT_TRUE(lr1);
        PH::UP wh1(dw.readPostingList(*lr1));
        ASSERT_TRUE(wh1);
        TermFieldMatchData f3;
        TermFieldMatchDataArray a;
        a.add(&f3);
        SB::UP sbap(wh1->createIterator(lr1->counts, a));
        sbap->initFullRange();
        EXPECT_EQ(vespalib::string("{1000000:}"), toString(f3.getIterator()));
        EXPECT_TRUE(sbap->seek(10));
        sbap->unpack(10);
        if (f3HasWeights) {
            EXPECT_EQ(vespalib::string("{2:0[e=0,w=4,l=2]}"),
                      toString(f3.getIterator(), true, true));
        } else {
            EXPECT_EQ(vespalib::string("{2:0[e=0,w=1,l=2]}"),
                      toString(f3.getIterator(), true, true));
        }
    }
    {
        uint32_t id1(schema.getIndexFieldId("f3"));;
        LR::UP lr1(dw.lookup(id1, "zz"));
        ASSERT_TRUE(lr1);
        PH::UP wh1(dw.readPostingList(*lr1));
        ASSERT_TRUE(wh1);
        TermFieldMatchData f3;
        TermFieldMatchDataArray a;
        a.add(&f3);
        SB::UP sbap(wh1->createIterator(lr1->counts, a));
        sbap->initFullRange();
        EXPECT_EQ(vespalib::string("{1000000:}"), toString(f3.getIterator()));
        EXPECT_TRUE(sbap->seek(11));
        sbap->unpack(11);
        if (f3HasWeights) {
            EXPECT_EQ(vespalib::string("{1:0[e=0,w=-27,l=1]}"),
                      toString(f3.getIterator(), true, true));
        } else {
            EXPECT_EQ(vespalib::string("{1:0[e=0,w=1,l=1]}"),
                      toString(f3.getIterator(), true, true));
        }
    }
    {
        uint32_t id1(schema.getIndexFieldId("f3"));;
        LR::UP lr1(dw.lookup(id1, "zz0"));
        ASSERT_TRUE(lr1);
        PH::UP wh1(dw.readPostingList(*lr1));
        ASSERT_TRUE(wh1);
        TermFieldMatchData f3;
        TermFieldMatchDataArray a;
        a.add(&f3);
        SB::UP sbap(wh1->createIterator(lr1->counts, a));
        sbap->initFullRange();
        EXPECT_EQ(vespalib::string("{1000000:}"), toString(f3.getIterator()));
        EXPECT_TRUE(sbap->seek(12));
        sbap->unpack(12);
        if (f3HasWeights) {
            EXPECT_EQ(vespalib::string("{1:0[e=0,w=0,l=1]}"),
                         toString(f3.getIterator(), true, true));
        } else {
            EXPECT_EQ(vespalib::string("{1:0[e=0,w=1,l=1]}"),
                      toString(f3.getIterator(), true, true));
        }
    }
}


void
FusionTest::requireThatFusionIsWorking(const vespalib::string &prefix, bool directio, bool readmmap)
{
    Schema schema;
    Schema schema2;
    Schema schema3;
    for (SchemaUtil::IndexIterator it(getSchema()); it.isValid(); ++it) {
        const Schema::IndexField &iField = _schema.getIndexField(it.getIndex());
        schema.addIndexField(Schema::IndexField(iField.getName(),
                                     iField.getDataType(),
                                     iField.getCollectionType()));
        if (iField.getCollectionType() == CollectionType::WEIGHTEDSET) {
            schema2.addIndexField(Schema::IndexField(iField.getName(),
                                                     iField.getDataType(),
                                                     CollectionType::ARRAY));
        } else {
            schema2.addIndexField(Schema::IndexField(iField.getName(),
                                                     iField.getDataType(),
                                                     iField.getCollectionType()));
        }
        schema3.addIndexField(Schema::IndexField(iField.getName(),
                                      iField.getDataType(),
                                      CollectionType::SINGLE));
    }
    schema3.addIndexField(Schema::IndexField("f4", DataType::STRING));
    schema.addFieldSet(Schema::FieldSet("nc0").
                              addField("f0").addField("f1"));
    schema2.addFieldSet(Schema::FieldSet("nc0").
                               addField("f1").addField("f0"));
    schema3.addFieldSet(Schema::FieldSet("nc2").
                               addField("f0").addField("f1").
                               addField("f2").addField("f3").
                               addField("f4"));
    FieldIndexCollection fic(schema, MockFieldLengthInspector());
    DocBuilder b(schema);
    SequencedTaskExecutor invertThreads(2);
    SequencedTaskExecutor pushThreads(2);
    DocumentInverter inv(schema, invertThreads, pushThreads, fic);
    Document::UP doc;

    b.startDocument("doc::10");
    b.startIndexField("f0").
        addStr("a").addStr("b").addStr("c").addStr("d").
        addStr("e").addStr("f").addStr("z").
        endField();
    b.startIndexField("f1").
        addStr("w").addStr("x").
        addStr("y").addStr("z").
        endField();
    b.startIndexField("f2").
        startElement(4).addStr("ax").addStr("ay").addStr("z").endElement().
        startElement(5).addStr("ax").endElement().
        endField();
    b.startIndexField("f3").
        startElement(4).addStr("wx").addStr("z").endElement().
        endField();

    doc = b.endDocument();
    inv.invertDocument(10, *doc);
    invertThreads.sync();
    myPushDocument(inv);
    pushThreads.sync();

    b.startDocument("doc::11").
        startIndexField("f3").
        startElement(-27).addStr("zz").endElement().
        endField();
    doc = b.endDocument();
    inv.invertDocument(11, *doc);
    invertThreads.sync();
    myPushDocument(inv);
    pushThreads.sync();

    b.startDocument("doc::12").
        startIndexField("f3").
        startElement(0).addStr("zz0").endElement().
        endField();
    doc = b.endDocument();
    inv.invertDocument(12, *doc);
    invertThreads.sync();
    myPushDocument(inv);
    pushThreads.sync();

    IndexBuilder ib(schema);
    vespalib::string dump2dir = prefix + "dump2";
    ib.setPrefix(dump2dir);
    uint32_t numDocs = 12 + 1;
    uint32_t numWords = fic.getNumUniqueWords();
    bool dynamicKPosOcc = false;
    MockFieldLengthInspector mock_field_length_inspector;
    TuneFileIndexing tuneFileIndexing;
    TuneFileSearch tuneFileSearch;
    DummyFileHeaderContext fileHeaderContext;
    if (directio) {
        tuneFileIndexing._read.setWantDirectIO();
        tuneFileIndexing._write.setWantDirectIO();
        tuneFileSearch._read.setWantDirectIO();
    }
    if (readmmap) {
        tuneFileSearch._read.setWantMemoryMap();
    }
    ib.open(numDocs, numWords, mock_field_length_inspector, tuneFileIndexing, fileHeaderContext);
    fic.dump(ib);
    ib.close();

    vespalib::string tsName = dump2dir + "/.teststamp";
    typedef search::FileKit FileKit;
    ASSERT_TRUE(FileKit::createStamp(tsName));
    ASSERT_TRUE(FileKit::hasStamp(tsName));
    ASSERT_TRUE(FileKit::removeStamp(tsName));
    ASSERT_FALSE(FileKit::hasStamp(tsName));
    vespalib::ThreadStackExecutor executor(4, 0x10000);

    do {
        DiskIndex dw2(prefix + "dump2");
        ASSERT_TRUE(dw2.setup(tuneFileSearch));
        validateDiskIndex(dw2, true, true);
    } while (0);

    do {
        std::vector<vespalib::string> sources;
        SelectorArray selector(numDocs, 0);
        sources.push_back(prefix + "dump2");
        ASSERT_TRUE(Fusion::merge(schema, prefix + "dump3", sources, selector,
                                  dynamicKPosOcc,
                                  tuneFileIndexing,fileHeaderContext, executor));
    } while (0);
    do {
        DiskIndex dw3(prefix + "dump3");
        ASSERT_TRUE(dw3.setup(tuneFileSearch));
        validateDiskIndex(dw3, true, true);
    } while (0);
    do {
        std::vector<vespalib::string> sources;
        SelectorArray selector(numDocs, 0);
        sources.push_back(prefix + "dump3");
        ASSERT_TRUE(Fusion::merge(schema2, prefix + "dump4", sources, selector,
                                  dynamicKPosOcc,
                                  tuneFileIndexing, fileHeaderContext, executor));
    } while (0);
    do {
        DiskIndex dw4(prefix + "dump4");
        ASSERT_TRUE(dw4.setup(tuneFileSearch));
        validateDiskIndex(dw4, true, false);
    } while (0);
    do {
        std::vector<vespalib::string> sources;
        SelectorArray selector(numDocs, 0);
        sources.push_back(prefix + "dump3");
        ASSERT_TRUE(Fusion::merge(schema3, prefix + "dump5", sources, selector,
                                  dynamicKPosOcc,
                                  tuneFileIndexing, fileHeaderContext, executor));
    } while (0);
    do {
        DiskIndex dw5(prefix + "dump5");
        ASSERT_TRUE(dw5.setup(tuneFileSearch));
        validateDiskIndex(dw5, false, false);
    } while (0);
    do {
        std::vector<vespalib::string> sources;
        SelectorArray selector(numDocs, 0);
        sources.push_back(prefix + "dump3");
        ASSERT_TRUE(Fusion::merge(schema, prefix + "dump6", sources, selector,
                                  !dynamicKPosOcc,
                                  tuneFileIndexing, fileHeaderContext, executor));
    } while (0);
    do {
        DiskIndex dw6(prefix + "dump6");
        ASSERT_TRUE(dw6.setup(tuneFileSearch));
        validateDiskIndex(dw6, true, true);
    } while (0);
    do {
        std::vector<vespalib::string> sources;
        SelectorArray selector(numDocs, 0);
        sources.push_back(prefix + "dump2");
        ASSERT_TRUE(Fusion::merge(schema, prefix + "dump3", sources, selector,
                                  dynamicKPosOcc,
                                  tuneFileIndexing, fileHeaderContext, executor));
    } while (0);
    do {
        DiskIndex dw3(prefix + "dump3");
        ASSERT_TRUE(dw3.setup(tuneFileSearch));
        validateDiskIndex(dw3, true, true);
    } while (0);
}

void
FusionTest::make_empty_index(const vespalib::string &dump_dir, const IFieldLengthInspector &field_length_inspector)
{
    FieldIndexCollection fic(_schema, field_length_inspector);
    uint32_t numDocs = 1;
    uint32_t numWords = 1;
    IndexBuilder ib(_schema);
    TuneFileIndexing tuneFileIndexing;
    DummyFileHeaderContext fileHeaderContext;
    ib.setPrefix(dump_dir);
    ib.open(numDocs, numWords, field_length_inspector, tuneFileIndexing, fileHeaderContext);
    fic.dump(ib);
    ib.close();
}

void
FusionTest::merge_empty_indexes(const vespalib::string &dump_dir, const std::vector<vespalib::string> &sources)
{
    vespalib::ThreadStackExecutor executor(4, 0x10000);
    TuneFileIndexing tuneFileIndexing;
    DummyFileHeaderContext fileHeaderContext;
    SelectorArray selector(1, 0);
    ASSERT_TRUE(Fusion::merge(_schema, dump_dir, sources, selector,
                              false,
                              tuneFileIndexing, fileHeaderContext, executor));
}

FusionTest::FusionTest()
    : ::testing::Test(),
      _schema()
{
    _schema.addIndexField(Schema::IndexField("f0", DataType::STRING));
    _schema.addIndexField(Schema::IndexField("f1", DataType::STRING));
    _schema.addIndexField(Schema::IndexField("f2", DataType::STRING, CollectionType::ARRAY));
    _schema.addIndexField(Schema::IndexField("f3", DataType::STRING, CollectionType::WEIGHTEDSET));
}

TEST_F(FusionTest, require_that_normal_fusion_is_working)
{
    requireThatFusionIsWorking("", false, false);
}

TEST_F(FusionTest, require_that_directio_fusion_is_working)
{
    requireThatFusionIsWorking("d", true, false);
}

TEST_F(FusionTest, require_that_mmap_fusion_is_working)
{
    requireThatFusionIsWorking("m", false, true);
}

TEST_F(FusionTest, require_that_directiommap_fusion_is_working)
{
    requireThatFusionIsWorking("dm", true, true);
}

namespace {

void clean_field_length_testdirs()
{
    vespalib::rmdir("fldump2", true);
    vespalib::rmdir("fldump3", true);
    vespalib::rmdir("fldump4", true);
}

}

TEST_F(FusionTest, require_that_average_field_length_is_preserved)
{
    clean_field_length_testdirs();
    make_empty_index("fldump2", MockFieldLengthInspector());
    make_empty_index("fldump3", MyMockFieldLengthInspector());
    merge_empty_indexes("fldump4", {"fldump2", "fldump3"});
    DiskIndex disk_index("fldump4");
    ASSERT_TRUE(disk_index.setup(TuneFileSearch()));
    EXPECT_EQ(3.5, disk_index.get_field_length_info("f0").get_average_field_length());
    clean_field_length_testdirs();
}

}

}

int
main(int argc, char* argv[])
{
    if (argc > 0) {
        search::index::DummyFileHeaderContext::setCreator(argv[0]);
    }
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
