// Copyright Yahoo. Licensed under the terms of the Apache 2.0 license. See LICENSE in the project root.

#include "fileutil.hpp"
#include "filesizecalculator.h"
#include <vespa/vespalib/util/exceptions.h>
#include <vespa/vespalib/util/guard.h>
#include <cassert>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdexcept>

#include <vespa/log/log.h>
LOG_SETUP(".searchlib.util.fileutil");

using vespalib::make_string;
using vespalib::IllegalStateException;
using vespalib::GenericHeader;
using vespalib::FileDescriptor;
using vespalib::getLastErrorString;

namespace search {
namespace fileutil {

LoadedMmap::LoadedMmap(const vespalib::string &fileName)
    : LoadedBuffer(NULL, 0),
      _mapBuffer(NULL),
      _mapSize(0)
{
    FileDescriptor fd(open(fileName.c_str(), O_RDONLY, 0664));
    if (fd.valid()) {
        struct stat stbuf;
        int res = fstat(fd.fd(), &stbuf);
        if (res == 0) {
            uint64_t fileSize = stbuf.st_size;
            size_t sz = fileSize;
            if (sz) {
                void *tmpBuffer = mmap(NULL, sz, PROT_READ, MAP_PRIVATE, fd.fd(), 0);
                if (tmpBuffer != MAP_FAILED) {
                    _mapSize = sz;
                    _mapBuffer = tmpBuffer;
                    uint32_t hl = GenericHeader::getMinSize();
                    bool badHeader = true;
                    if (sz >= hl) {
                        GenericHeader::MMapReader rd(static_cast<const char *>(tmpBuffer), sz);
                        _header = std::make_unique<GenericHeader>();
                        size_t headerLen = _header->read(rd);
                        if ((headerLen <= _mapSize) &&
                            FileSizeCalculator::extractFileSize(*_header, headerLen, fileName, fileSize)) {
                            sz = fileSize;
                            _size = sz - headerLen;
                            _buffer = static_cast<char *>(_mapBuffer) + headerLen;
                            badHeader = false;
                        }
                    }
                    if (badHeader) {
                        throw IllegalStateException(make_string("bad file header: %s", fileName.c_str()));
                    }
                } else {
                    throw IllegalStateException(make_string("Failed mmaping '%s' of size %" PRIu64 " errno(%d)",
                                                            fileName.c_str(), static_cast<uint64_t>(sz), errno));
                }
            }
        } else {
            throw IllegalStateException(make_string("Failed fstat '%s' of fd %d with result = %d",
                                                    fileName.c_str(), fd.fd(), res));
        }
    } else {
        throw IllegalStateException(
                make_string("Failed opening '%s' for reading errno(%d)", fileName.c_str(), errno));
    }
}

LoadedMmap::~LoadedMmap() {
    madvise(_mapBuffer, _mapSize, MADV_DONTNEED);
    munmap(_mapBuffer, _mapSize);
}

}

std::unique_ptr<FastOS_FileInterface>
FileUtil::openFile(const vespalib::string &fileName)
{
    std::unique_ptr<Fast_BufferedFile> file(new Fast_BufferedFile());
    file->EnableDirectIO();
    if (!file->OpenReadOnly(fileName.c_str())) {
        LOG(error, "could not open %s: %s", file->GetFileName(), getLastErrorString().c_str());
        file->Close();
        throw IllegalStateException(make_string("Failed opening '%s' for direct IO reading.", file->GetFileName()));
    }
    return file;
}

using fileutil::LoadedBuffer;
using fileutil::LoadedMmap;

LoadedBuffer::UP
FileUtil::loadFile(const vespalib::string &fileName)
{
    LoadedBuffer::UP data(new LoadedMmap(fileName));
    FastOS_File file(fileName.c_str());
    if (!file.OpenReadOnly()) {
        LOG(error, "could not open %s: %s", file.GetFileName(), getLastErrorString().c_str());
    }
    file.Close();
    return data;
}


void FileReaderBase::handleError(ssize_t numRead, size_t wanted)
{
    if (numRead == 0) {
        throw std::runtime_error(vespalib::make_string("Trying to read past EOF of file %s", _file.GetFileName()));
    } else {
        throw std::runtime_error(vespalib::make_string("Partial read(%zd of %zu) of file %s", numRead, wanted, _file.GetFileName()));
    }
}

void FileWriterBase::handleError(ssize_t numRead, size_t wanted)
{
    if (numRead == 0) {
        throw std::runtime_error(vespalib::make_string("Failed writing anything to file %s", _file.GetFileName()));
    } else {
        throw std::runtime_error(vespalib::make_string("Partial read(%zd of %zu) of file %s", numRead, wanted, _file.GetFileName()));
    }
}

SequentialFileArray::SequentialFileArray(const vespalib::string & fname) :
    _backingFile(),
    _name(fname)
{
    _backingFile->EnableDirectIO();
}

SequentialFileArray::~SequentialFileArray()
{
    close();
}

void SequentialFileArray::rewind()
{
    assert(_backingFile->SetPosition(0));
}

void SequentialFileArray::close()
{
    _backingFile->Close();
}

void SequentialFileArray::erase()
{
    close();
    FastOS_File::Delete(_backingFile->GetFileName());
}

void SequentialFileArray::openReadOnly()
{
    _backingFile->ReadOpen(_name.c_str());
}

void SequentialFileArray::openWriteOnly()
{
    _backingFile->OpenWriteOnlyTruncate(_name.c_str());
}

ssize_t
FileReaderBase::read(void *buf, size_t sz) {
    ssize_t numRead = _file.Read(buf, sz);
    if (numRead != ssize_t(sz)) {
        handleError(numRead, sz);
    }
    return numRead;
}

ssize_t
FileWriterBase::write(const void *buf, size_t sz) {
    ssize_t numWritten = _file.Write2(buf, sz);
    if (numWritten != ssize_t(sz)) {
        handleError(numWritten, sz);
    }
    return numWritten;
}

}
