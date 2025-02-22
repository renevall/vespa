// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/eval/eval/tensor_spec.h>
#include <vespa/eval/eval/value.h>
#include <vespa/eval/eval/value_codec.h>
#include <vespa/searchcommon/attribute/iattributecontext.h>
#include <vespa/searchlib/attribute/attribute_blueprint_factory.h>
#include <vespa/searchlib/attribute/attribute_read_guard.h>
#include <vespa/searchlib/attribute/attributecontext.h>
#include <vespa/searchlib/attribute/attributefactory.h>
#include <vespa/searchlib/attribute/attribute.h>
#include <vespa/searchlib/fef/matchdata.h>
#include <vespa/searchlib/query/tree/location.h>
#include <vespa/searchlib/query/tree/point.h>
#include <vespa/searchlib/query/tree/simplequery.h>
#include <vespa/searchlib/queryeval/fake_requestcontext.h>
#include <vespa/searchlib/queryeval/filter_wrapper.h>
#include <vespa/searchlib/queryeval/leaf_blueprints.h>
#include <vespa/searchlib/queryeval/nearest_neighbor_blueprint.h>
#include <vespa/searchlib/tensor/dense_tensor_attribute.h>
#include <vespa/vespalib/gtest/gtest.h>

#include <vespa/log/log.h>
LOG_SETUP("attributeblueprint_test");

using search::AttributeGuard;
using search::AttributeVector;
using search::IAttributeManager;
using search::attribute::IAttributeContext;
using search::fef::MatchData;
using search::fef::TermFieldMatchData;
using search::query::Location;
using search::query::Node;
using search::query::Point;
using search::query::SimpleLocationTerm;
using search::query::SimplePrefixTerm;
using search::query::SimpleStringTerm;
using search::query::Weight;
using search::queryeval::Blueprint;
using search::queryeval::EmptyBlueprint;
using search::queryeval::FakeRequestContext;
using search::queryeval::FieldSpec;
using search::queryeval::FilterWrapper;
using search::queryeval::NearestNeighborBlueprint;
using search::queryeval::SearchIterator;
using std::string;
using std::vector;
using vespalib::eval::TensorSpec;
using vespalib::eval::Value;
using vespalib::eval::ValueType;
using namespace search::attribute;
using namespace search;

namespace {

const string field = "field";

class MyAttributeManager : public IAttributeManager {
    AttributeVector::SP _attribute_vector;

public:
    MyAttributeManager(MyAttributeManager && rhs) :
        IAttributeManager(),
        _attribute_vector(std::move(rhs._attribute_vector))
    {
    }

    MyAttributeManager(AttributeVector::SP attr)
        : _attribute_vector(std::move(attr))
    {
    }

    AttributeGuard::UP getAttribute(const string &) const override {
        return AttributeGuard::UP(new AttributeGuard(_attribute_vector));
    }

    std::unique_ptr<attribute::AttributeReadGuard>
    getAttributeReadGuard(const string &, bool stableEnumGuard) const override {
        if (_attribute_vector) {
            return _attribute_vector->makeReadGuard(stableEnumGuard);
        } else {
            return std::unique_ptr<attribute::AttributeReadGuard>();
        }
    }
    void getAttributeList(vector<AttributeGuard> &) const override {
        assert(!"Not implemented");
    }
    IAttributeContext::UP createContext() const override {
        assert(!"Not implemented");
        return IAttributeContext::UP();
    }

    void asyncForAttribute(const vespalib::string &, std::unique_ptr<IAttributeFunctor>) const override {
        assert(!"Not implemented");
    }

    std::shared_ptr<attribute::ReadableAttributeVector> readable_attribute_vector(const string&) const override {
        return _attribute_vector;
    }
};

constexpr uint32_t DOCID_LIMIT = 3;

bool
do_search(const Node &node, IAttributeManager &attribute_manager, bool expect_attribute_search_context = true)
{
    AttributeContext ac(attribute_manager);
    FakeRequestContext requestContext(&ac);
    MatchData::UP md(MatchData::makeTestInstance(1, 1));
    AttributeBlueprintFactory source;
    Blueprint::UP result = source.createBlueprint(requestContext, FieldSpec(field, 0, 0), node);
    assert(result.get());
    EXPECT_TRUE(!result->getState().estimate().empty);
    EXPECT_EQ(3u, result->getState().estimate().estHits);
    if (expect_attribute_search_context) {
        EXPECT_TRUE(result->get_attribute_search_context() != nullptr);
    } else {
        EXPECT_TRUE(result->get_attribute_search_context() == nullptr);
    }
    result->fetchPostings(queryeval::ExecuteInfo::TRUE);
    result->setDocIdLimit(DOCID_LIMIT);
    SearchIterator::UP iterator = result->createSearch(*md, true);
    assert((bool)iterator);
    iterator->initRange(1, 3);
    EXPECT_TRUE(!iterator->seek(1));
    return iterator->seek(2);
}

bool
search_for_term(const string &term, IAttributeManager &attribute_manager)
{
    SimpleStringTerm node(term, "field", 0, Weight(0));
    bool ret = do_search(node, attribute_manager);
    return ret;
}

template <typename ChildType, typename ParentType>
ChildType&
downcast(ParentType& parent)
{
    auto* result = dynamic_cast<ChildType*>(&parent);
    assert(result != nullptr);
    return *result;
}

struct StringAttributeFiller {
    using ValueType = vespalib::string;
    static void add(AttributeVector& attr, const vespalib::string& value) {
        auto& real = downcast<StringAttribute>(attr);
        real.update(attr.getNumDocs() - 1, value);
        real.commit();
    }
};

struct IntegerAttributeFiller {
    using ValueType = int64_t;
    static void add(AttributeVector& attr, int64_t value) {
        auto& real = downcast<IntegerAttribute>(attr);
        real.update(attr.getNumDocs() - 1, value);
        real.commit();
    }
};

template <typename FillerType>
void
fill(AttributeVector& attr, typename FillerType::ValueType value)
{
    AttributeVector::DocId docid;
    attr.addDoc(docid);
    attr.addDoc(docid);
    attr.addDoc(docid);
    assert(DOCID_LIMIT-1 == docid);
    FillerType::add(attr, value);
}

AttributeVector::SP
make_string_attribute(const std::string& value)
{
    Config cfg(BasicType::STRING, CollectionType::SINGLE);
    auto attr = AttributeFactory::createAttribute(field, cfg);
    fill<StringAttributeFiller>(*attr, value);
    return attr;
}

AttributeVector::SP
make_int_attribute(int64_t value)
{
    Config cfg(BasicType::INT32, CollectionType::SINGLE);
    auto attr = AttributeFactory::createAttribute(field, cfg);
    fill<IntegerAttributeFiller>(*attr, value);
    return attr;
}

AttributeVector::SP
make_fast_search_long_attribute(int64_t value)
{
    Config cfg(BasicType::fromType(int64_t()), CollectionType::SINGLE);
    cfg.setFastSearch(true);
    auto attr = AttributeFactory::createAttribute(field, cfg);
    fill<IntegerAttributeFiller>(*attr, value);
    return attr;
}

MyAttributeManager
makeAttributeManager(const std::string& value)
{
    return MyAttributeManager(make_string_attribute(value));
}

MyAttributeManager
makeAttributeManager(int64_t value)
{
    return MyAttributeManager(make_int_attribute(value));
}

MyAttributeManager
makeFastSearchLongAttribute(int64_t value)
{
    return MyAttributeManager(make_fast_search_long_attribute(value));
}

}  // namespace

TEST(AttributeBlueprintTest, require_that_iterators_can_be_created)
{
    auto attribute_manager = makeAttributeManager("foo");

    EXPECT_TRUE(search_for_term("foo", attribute_manager));
}

TEST(AttributeBlueprintTest, require_that_range_terms_work)
{
    auto attribute_manager = makeAttributeManager(int64_t(42));

    EXPECT_TRUE(search_for_term("[23;46]", attribute_manager));
    EXPECT_TRUE(!search_for_term("[10;23]", attribute_manager));
    EXPECT_TRUE(!search_for_term(">43", attribute_manager));
    EXPECT_TRUE(search_for_term("[10;]", attribute_manager));
}

TEST(AttributeBlueprintTest, require_that_prefix_terms_work)
{
    auto attribute_manager = makeAttributeManager("foo");

    SimplePrefixTerm node("fo", "field", 0, Weight(0));
    EXPECT_TRUE(do_search(node, attribute_manager));
}

TEST(AttributeBlueprintTest, require_that_location_terms_work)
{
    // 0xcc is z-curve for (10, 10).
    auto attribute_manager = makeAttributeManager(int64_t(0xcc));
    {
        SimpleLocationTerm node(Location(Point{10, 10}, 3, 0), field, 0, Weight(0));
        EXPECT_TRUE(do_search(node, attribute_manager, false));
    }
    {
        SimpleLocationTerm node(Location(Point{100, 100}, 3, 0), field, 0, Weight(0));
        EXPECT_TRUE(!do_search(node, attribute_manager, false));
    }
    {
        SimpleLocationTerm node(Location(Point{13, 13}, 4, 0), field, 0, Weight(0));
        EXPECT_TRUE(!do_search(node, attribute_manager, false));
    }
    {
        SimpleLocationTerm node(Location(Point{10, 13}, 3, 0), field, 0, Weight(0));
        EXPECT_TRUE(do_search(node, attribute_manager, false));
    }
}

TEST(AttributeBlueprintTest, require_that_fast_search_location_terms_work)
{
    // 0xcc is z-curve for (10, 10).
    auto attribute_manager = makeFastSearchLongAttribute(int64_t(0xcc));

    SimpleLocationTerm node(Location(Point{10, 10}, 3, 0), field, 0, Weight(0));
#if 0
    EXPECT_TRUE(search(node, attribute_manager));
    node = SimpleLocationTerm(Location(Point{100, 100}, 3, 0),field, 0, Weight(0));
    EXPECT_TRUE(!search(node, attribute_manager));
    node = SimpleLocationTerm(Location(Point{13, 13}, 4, 0),field, 0, Weight(0));
    EXPECT_TRUE(!search(node, attribute_manager));
    node = SimpleLocationTerm(Location(Point{10, 13}, 3, 0),field, 0, Weight(0));
    EXPECT_TRUE(search(node, attribute_manager));
#endif
}

AttributeVector::SP
make_tensor_attribute(const vespalib::string& name, const vespalib::string& tensor_spec)
{
    Config cfg(BasicType::TENSOR, CollectionType::SINGLE);
    cfg.setTensorType(ValueType::from_spec(tensor_spec));
    return AttributeFactory::createAttribute(name, cfg);
}

AttributeVector::SP
make_int_attribute(const vespalib::string& name)
{
    Config cfg(BasicType::INT32, CollectionType::SINGLE);
    return AttributeFactory::createAttribute(name, cfg);
}

class BlueprintFactoryFixture {
public:
    MyAttributeManager mgr;
    vespalib::string attr_name;
    AttributeContext attr_ctx;
    FakeRequestContext request_ctx;
    AttributeBlueprintFactory source;

    BlueprintFactoryFixture(AttributeVector::SP attr)
        : mgr(attr),
          attr_name(attr->getName()),
          attr_ctx(mgr),
          request_ctx(&attr_ctx),
          source()
    {
    }
    ~BlueprintFactoryFixture() {}
    Blueprint::UP create_blueprint(const Node& term) {
        auto result = source.createBlueprint(request_ctx, FieldSpec(attr_name, 0, 0), term);
        result->fetchPostings(queryeval::ExecuteInfo::TRUE);
        result->setDocIdLimit(DOCID_LIMIT);
        return result;
    }
};

class NearestNeighborFixture : public BlueprintFactoryFixture {
public:
    NearestNeighborFixture(AttributeVector::SP attr)
        : BlueprintFactoryFixture(std::move(attr))
    {
    }
    ~NearestNeighborFixture() {}
    void set_query_tensor(const TensorSpec& tensor_spec) {
        request_ctx.set_query_tensor("query_tensor", tensor_spec);
    }
    Blueprint::UP create_blueprint() {
        query::NearestNeighborTerm term("query_tensor", attr_name, 0, Weight(0), 7, true, 33, 100100.25);
        return BlueprintFactoryFixture::create_blueprint(term);
    }
};

void
expect_nearest_neighbor_blueprint(const vespalib::string& attribute_tensor_type_spec,
                                  const TensorSpec& query_tensor,
                                  const TensorSpec& converted_query_tensor)
{
    NearestNeighborFixture f(make_tensor_attribute(field, attribute_tensor_type_spec));
    f.set_query_tensor(query_tensor);

    auto result = f.create_blueprint();
    const auto& nearest = downcast<const NearestNeighborBlueprint>(*result);
    EXPECT_EQ(attribute_tensor_type_spec, nearest.get_attribute_tensor().getTensorType().to_spec());
    EXPECT_EQ(converted_query_tensor, spec_from_value(nearest.get_query_tensor()));
    EXPECT_EQ(7u, nearest.get_target_num_hits());
}

TEST(AttributeBlueprintTest, nearest_neighbor_blueprint_is_created_by_attribute_blueprint_factory)
{
    TensorSpec x_2_double = TensorSpec("tensor(x[2])").add({{"x", 0}}, 3).add({{"x", 1}}, 5);
    TensorSpec x_2_float = TensorSpec("tensor<float>(x[2])").add({{"x", 0}}, 3).add({{"x", 1}}, 5);

    // same cell type:
    expect_nearest_neighbor_blueprint("tensor(x[2])", x_2_double, x_2_double);
    expect_nearest_neighbor_blueprint("tensor<float>(x[2])", x_2_float, x_2_float);
    // convert cell type:
    expect_nearest_neighbor_blueprint("tensor(x[2])", x_2_float, x_2_double);
    expect_nearest_neighbor_blueprint("tensor<float>(x[2])", x_2_double, x_2_float);
}

void
expect_empty_blueprint(AttributeVector::SP attr, const TensorSpec& query_tensor, bool insert_query_tensor = true)
{
    NearestNeighborFixture f(attr);
    if (insert_query_tensor) {
        f.set_query_tensor(query_tensor);
    }
    auto result = f.create_blueprint();
    EXPECT_TRUE(dynamic_cast<EmptyBlueprint*>(result.get()) != nullptr);
}

void
expect_empty_blueprint(AttributeVector::SP attr)
{
    expect_empty_blueprint(std::move(attr), TensorSpec("double"), false);
}

TEST(AttributeBlueprintTest, empty_blueprint_is_created_when_nearest_neighbor_term_is_invalid)
{
    TensorSpec sparse_x = TensorSpec("tensor(x{})").add({{"x", "0"}}, 3);
    TensorSpec dense_y_2 = TensorSpec("tensor(y[2])").add({{"y", 0}}, 3).add({{"y", 1}}, 5);
    TensorSpec dense_x_3 = TensorSpec("tensor(x[3])").add({{"x", 0}}, 3).add({{"x", 1}}, 5).add({{"x", 2}}, 7);
    expect_empty_blueprint(make_int_attribute(field)); // attribute is not a tensor
    expect_empty_blueprint(make_tensor_attribute(field, "tensor(x{})")); // attribute is not a dense tensor
    expect_empty_blueprint(make_tensor_attribute(field, "tensor(x[2],y[2])")); // tensor type is not of order 1
    expect_empty_blueprint(make_tensor_attribute(field, "tensor(x[2])")); // query tensor not found
    expect_empty_blueprint(make_tensor_attribute(field, "tensor(x[2])"), sparse_x); // query tensor is not dense
    expect_empty_blueprint(make_tensor_attribute(field, "tensor(x[2])"), dense_y_2); // tensor types are not compatible
    expect_empty_blueprint(make_tensor_attribute(field, "tensor(x[2])"), dense_x_3); // tensor types are not same size
}

TEST(AttributeBlueprintTest, attribute_field_blueprint_wraps_filter_search_iterator)
{
    BlueprintFactoryFixture f(make_string_attribute("foo"));
    SimpleStringTerm term("foo", field, 0, Weight(0));
    auto blueprint = f.create_blueprint(term);

    auto itr = blueprint->createFilterSearch(true, Blueprint::FilterConstraint::UPPER_BOUND);
    auto& wrapper = downcast<FilterWrapper>(*itr);
    wrapper.initRange(1, 3);
    EXPECT_FALSE(wrapper.seek(1));
    EXPECT_TRUE(wrapper.seek(2));
}

GTEST_MAIN_RUN_ALL_TESTS()
