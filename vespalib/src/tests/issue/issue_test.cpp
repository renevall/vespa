// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include <vespa/vespalib/util/issue.h>
#include <vespa/vespalib/gtest/gtest.h>

using namespace vespalib;

struct MyHandler : Issue::Handler {
    std::vector<vespalib::string> list;
    void handle(const Issue &issue) override {
        list.push_back(issue.message());
    }
};

std::vector<vespalib::string> make_list(std::vector<vespalib::string> list) {
    return list;
}

TEST(IssueTest, log_issue_not_captured) {
    Issue::report(Issue("this should be logged"));
}

TEST(IssueTest, capture_an_issue) {
    MyHandler my_handler;
    {
        Issue::report(Issue("this should be logged"));
        Issue::Binding my_binding = Issue::listen(my_handler);
        Issue::report(Issue("this should be captured"));
    }
    Issue::report(Issue("this should also be logged"));
    EXPECT_EQ(my_handler.list, make_list({"this should be captured"}));
}

TEST(IssueTest, capture_issues_with_nested_bindings) {
    MyHandler my_handler1;
    MyHandler my_handler2;
    {
        Issue::report(Issue("this should be logged"));
        auto my_binding1 = Issue::listen(my_handler1);
        Issue::report(Issue("issue1"));
        {
            auto my_binding2 = Issue::listen(my_handler2);
            Issue::report(Issue("issue2"));
        }
        Issue::report(Issue("issue3"));
    }
    Issue::report(Issue("this should also be logged"));
    EXPECT_EQ(my_handler1.list, make_list({"issue1", "issue3"}));
    EXPECT_EQ(my_handler2.list, make_list({"issue2"}));
}

TEST(IssueTest, handler_can_be_bound_multiple_times) {
    MyHandler my_handler;
    {
        Issue::report(Issue("this should be logged"));
        auto my_binding1 = Issue::listen(my_handler);
        Issue::report(Issue("issue1"));
        {
            auto my_binding2 = Issue::listen(my_handler);
            Issue::report(Issue("issue2"));
        }
        Issue::report(Issue("issue3"));
    }
    Issue::report(Issue("this should also be logged"));
    EXPECT_EQ(my_handler.list, make_list({"issue1", "issue2", "issue3"}));
}

GTEST_MAIN_RUN_ALL_TESTS()
