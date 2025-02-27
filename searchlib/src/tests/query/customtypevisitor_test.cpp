// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.
// Unit tests for customtypevisitor.

#include <vespa/searchlib/query/tree/customtypevisitor.h>
#include <vespa/searchlib/query/tree/intermediatenodes.h>
#include <vespa/searchlib/query/tree/termnodes.h>
#include <vespa/vespalib/testkit/testapp.h>

#include <vespa/log/log.h>
LOG_SETUP("customtypevisitor_test");

using std::string;

using namespace search::query;

namespace {

template <class Base>
struct InitTerm : Base {
    InitTerm() : Base(typename Base::Type(), "view", 0, Weight(0)) {}
};

struct MyAnd : And {};
struct MyAndNot : AndNot {};
struct MyEquiv : Equiv {};
struct MyNear : Near { MyNear() : Near(1) {} };
struct MyONear : ONear { MyONear() : ONear(1) {} };
struct MyOr : Or {};
struct MyPhrase : Phrase { MyPhrase() : Phrase("view", 0, Weight(42)) {} };
struct MySameElement : SameElement { MySameElement() : SameElement("view") {} };
struct MyRank : Rank {};
struct MyNumberTerm : InitTerm<NumberTerm>  {};
struct MyLocationTerm : InitTerm<LocationTerm> {};
struct MyPrefixTerm : InitTerm<PrefixTerm>  {};
struct MyRangeTerm : InitTerm<RangeTerm> {};
struct MyStringTerm : InitTerm<StringTerm>  {};
struct MySubstrTerm : InitTerm<SubstringTerm>  {};
struct MySuffixTerm : InitTerm<SuffixTerm>  {};
struct MyWeakAnd : WeakAnd { MyWeakAnd() : WeakAnd(1234, "view") {} };
struct MyWeightedSetTerm : WeightedSetTerm { MyWeightedSetTerm() : WeightedSetTerm(0, "view", 0, Weight(42)) {} };
struct MyDotProduct : DotProduct { MyDotProduct() : DotProduct(0, "view", 0, Weight(42)) {} };
struct MyWandTerm : WandTerm { MyWandTerm() : WandTerm(0, "view", 0, Weight(42), 57, 67, 77.7) {} };
struct MyPredicateQuery : InitTerm<PredicateQuery> {};
struct MyRegExpTerm : InitTerm<RegExpTerm>  {};
struct MyNearestNeighborTerm : NearestNeighborTerm {};

struct MyQueryNodeTypes {
    typedef MyAnd And;
    typedef MyAndNot AndNot;
    typedef MyEquiv Equiv;
    typedef MyNumberTerm NumberTerm;
    typedef MyLocationTerm LocationTerm;
    typedef MyNear Near;
    typedef MyONear ONear;
    typedef MyOr Or;
    typedef MyPhrase Phrase;
    typedef MySameElement SameElement;
    typedef MyPrefixTerm PrefixTerm;
    typedef MyRangeTerm RangeTerm;
    typedef MyRank Rank;
    typedef MyStringTerm StringTerm;
    typedef MySubstrTerm SubstringTerm;
    typedef MySuffixTerm SuffixTerm;
    typedef MyWeakAnd WeakAnd;
    typedef MyWeightedSetTerm WeightedSetTerm;
    typedef MyDotProduct DotProduct;
    typedef MyWandTerm WandTerm;
    typedef MyPredicateQuery PredicateQuery;
    typedef MyRegExpTerm RegExpTerm;
    typedef MyNearestNeighborTerm NearestNeighborTerm;
};

class MyCustomVisitor : public CustomTypeVisitor<MyQueryNodeTypes>
{
public:
    template <typename T>
    bool &isVisited() {
        static bool b;
        return b;
    }

    template <typename T> void setVisited() { isVisited<T>() = true; }

    void visit(MyAnd &) override { setVisited<MyAnd>(); }
    void visit(MyAndNot &) override { setVisited<MyAndNot>(); }
    void visit(MyEquiv &) override { setVisited<MyEquiv>(); }
    void visit(MyNumberTerm &) override { setVisited<MyNumberTerm>(); }
    void visit(MyLocationTerm &) override { setVisited<MyLocationTerm>(); }
    void visit(MyNear &) override { setVisited<MyNear>(); }
    void visit(MyONear &) override { setVisited<MyONear>(); }
    void visit(MyOr &) override { setVisited<MyOr>(); }
    void visit(MyPhrase &) override { setVisited<MyPhrase>(); }
    void visit(MySameElement &) override { setVisited<MySameElement>(); }
    void visit(MyPrefixTerm &) override { setVisited<MyPrefixTerm>(); }
    void visit(MyRangeTerm &) override { setVisited<MyRangeTerm>(); }
    void visit(MyRank &) override { setVisited<MyRank>(); }
    void visit(MyStringTerm &) override { setVisited<MyStringTerm>(); }
    void visit(MySubstrTerm &) override { setVisited<MySubstrTerm>(); }
    void visit(MySuffixTerm &) override { setVisited<MySuffixTerm>(); }
    void visit(MyWeakAnd &) override { setVisited<MyWeakAnd>(); }
    void visit(MyWeightedSetTerm &) override { setVisited<MyWeightedSetTerm>(); }
    void visit(MyDotProduct &) override { setVisited<MyDotProduct>(); }
    void visit(MyWandTerm &) override { setVisited<MyWandTerm>(); }
    void visit(MyPredicateQuery &) override { setVisited<MyPredicateQuery>(); }
    void visit(MyRegExpTerm &) override { setVisited<MyRegExpTerm>(); }
    void visit(MyNearestNeighborTerm &) override { setVisited<MyNearestNeighborTerm>(); }
};

template <class T>
void requireThatNodeIsVisited() {
    MyCustomVisitor visitor;
    Node::UP query(new T);
    visitor.isVisited<T>() = false;
    query->accept(visitor);
    ASSERT_TRUE(visitor.isVisited<T>());
}

TEST("customtypevisitor_test") {

    requireThatNodeIsVisited<MyAnd>();
    requireThatNodeIsVisited<MyAndNot>();
    requireThatNodeIsVisited<MyNear>();
    requireThatNodeIsVisited<MyONear>();
    requireThatNodeIsVisited<MyOr>();
    requireThatNodeIsVisited<MyPhrase>();
    requireThatNodeIsVisited<MySameElement>();
    requireThatNodeIsVisited<MyRangeTerm>();
    requireThatNodeIsVisited<MyRank>();
    requireThatNodeIsVisited<MyNumberTerm>();
    requireThatNodeIsVisited<MyPrefixTerm>();
    requireThatNodeIsVisited<MyStringTerm>();
    requireThatNodeIsVisited<MySubstrTerm>();
    requireThatNodeIsVisited<MySuffixTerm>();
    requireThatNodeIsVisited<MyWeightedSetTerm>();
    requireThatNodeIsVisited<MyDotProduct>();
    requireThatNodeIsVisited<MyWandTerm>();
    requireThatNodeIsVisited<MyPredicateQuery>();
    requireThatNodeIsVisited<MyRegExpTerm>();
}
}  // namespace

TEST_MAIN() { TEST_RUN_ALL(); }