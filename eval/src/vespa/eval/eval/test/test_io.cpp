// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "test_io.h"
#include <vespa/vespalib/util/require.h>
#include <vespa/vespalib/data/slime/slime.h>
#include <vespa/vespalib/data/slime/json_format.h>
#include <vespa/vespalib/util/size_literals.h>
#include <unistd.h>
#include <assert.h>

using vespalib::Memory;
using vespalib::WritableMemory;
using vespalib::slime::JsonFormat;
using vespalib::slime::Cursor;

namespace vespalib::eval::test {

//-----------------------------------------------------------------------------

constexpr size_t CHUNK_SIZE = 16_Ki;
const char *num_tests_str = "num_tests";

//-----------------------------------------------------------------------------

Memory
StdIn::obtain()
{
    if ((_input.get().size == 0) && !_eof) {
        WritableMemory buf = _input.reserve(CHUNK_SIZE);
        ssize_t res = read(STDIN_FILENO, buf.data, buf.size);
        _eof = (res == 0);
        assert(res >= 0); // fail on stdio read errors
        _input.commit(res);
    }
    return _input.obtain();
}

Input &
StdIn::evict(size_t bytes)
{
    _input.evict(bytes);
    return *this;
}

//-----------------------------------------------------------------------------

WritableMemory
StdOut::reserve(size_t bytes)
{
    return _output.reserve(bytes);
}

Output &
StdOut::commit(size_t bytes)
{
    _output.commit(bytes);
    Memory buf = _output.obtain();
    ssize_t res = write(STDOUT_FILENO, buf.data, buf.size);
    assert(res == ssize_t(buf.size)); // fail on stdout write failures
    _output.evict(res);
    return *this;
}

//-----------------------------------------------------------------------------

bool
LineReader::read_line(vespalib::string &line)
{
    line.clear();
    for (auto mem = _input.obtain(); mem.size > 0; mem = _input.obtain()) {
        for (size_t i = 0; i < mem.size; ++i) {
            if (mem.data[i] == '\n') {
                _input.evict(i + 1);
                return true;
            } else {
                line.push_back(mem.data[i]);
            }
        }
        _input.evict(mem.size);
    }
    return !line.empty();
}

//-----------------------------------------------------------------------------

bool look_for_eof(Input &input) {
    for (auto mem = input.obtain(); mem.size > 0; mem = input.obtain()) {
        for (size_t i = 0; i < mem.size; ++i) {
            if (!isspace(mem.data[i])) {
                input.evict(i);
                return false;
            }
        }
        input.evict(mem.size);
    }
    return true;
}

void write_compact(const Slime &slime, Output &out) {
    JsonFormat::encode(slime, out, true);
    out.reserve(1).data[0] = '\n';
    out.commit(1);
}

//-----------------------------------------------------------------------------

void
TestWriter::maybe_write_test()
{
    if (_test.get().type().getId() != slime::NIX::ID) {
        REQUIRE(_test.get().fields() > 0u);
        REQUIRE(!_test[num_tests_str].valid());
        write_compact(_test, _out);
        ++_num_tests;
    }
}

TestWriter::TestWriter(Output &output)
    : _out(output),
      _test(),
      _num_tests(0)
{
}

Cursor &
TestWriter::create()
{
    maybe_write_test();
    _test = Slime();
    return _test.setObject();
}

TestWriter::~TestWriter()
{
    create().setLong(num_tests_str, _num_tests);
    write_compact(_test, _out); // summary
}

//-----------------------------------------------------------------------------

void for_each_test(Input &in,
                   const std::function<void(Slime&)> &handle_test,
                   const std::function<void(Slime&)> &handle_summary)
{
    size_t num_tests = 0;
    bool got_summary = false;
    while (in.obtain().size > 0) {
        Slime slime;
        if (JsonFormat::decode(in, slime)) {
            bool is_summary = slime[num_tests_str].valid();
            bool is_test = (!is_summary && (slime.get().fields() > 0));
            REQUIRE(is_test != is_summary);
            if (is_test) {
                ++num_tests;
                REQUIRE(!got_summary);
                handle_test(slime);
            } else {
                got_summary = true;
                REQUIRE_EQ(slime[num_tests_str].asLong(), int64_t(num_tests));
                handle_summary(slime);
            }
        } else {
            REQUIRE_EQ(in.obtain().size, 0u);
        }
    }
    REQUIRE(got_summary);
}

//-----------------------------------------------------------------------------

} // namespace vespalib::eval::test
