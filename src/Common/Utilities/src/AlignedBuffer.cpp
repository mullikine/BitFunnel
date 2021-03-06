// The MIT License (MIT)

// Copyright (c) 2016, Microsoft

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#include <cstring>


#include "AlignedBuffer.h"
#include "BitFunnel/Exceptions.h"
#include "LoggerInterfaces/Check.h"

#ifdef BITFUNNEL_PLATFORM_WINDOWS
#include <Windows.h>   // For VirtualAlloc/VirtualFree.
#else

#include <sys/mman.h>  // For mmap/munmap.
#endif


namespace BitFunnel
{

    AlignedBuffer::AlignedBuffer(size_t size, int alignment)
    {
        m_requestedSize = size;

#ifdef BITFUNNEL_PLATFORM_WINDOWS
        size_t padding = 1ULL << alignment;
        m_actualSize = m_requestedSize + padding;
        m_rawBuffer = VirtualAlloc(nullptr, m_actualSize, MEM_COMMIT, PAGE_READWRITE);
        CHECK_NE(m_rawBuffer, nullptr) <<  "VirtualAlloc() failed.";
        m_alignedBuffer = (char *)(((size_t)m_rawBuffer + padding -1) & ~(padding -1));
#else
        // TODO: detect non-4k size?
        const int c_pageSize = 4096;

        // mmap will give us something page aligned and we assume that alignment
        // is sufficient.
        CHECK_LE(alignment, c_pageSize) << "Alignment > 4096.\n";
        m_actualSize = m_requestedSize;
        m_rawBuffer = mmap(nullptr, size,
                           PROT_READ | PROT_WRITE,
                           MAP_ANON | MAP_PRIVATE,
                           -1,  // No file descriptor.
                           0);

        // `MAP_FAILED` is implemented as an old-style cast on some old
        // Unix-derived platforms. Note that issuing a `#pragma GCC` here is
        // meant to cover both Clang and GCC, since the issue can manifest with
        // either toolchain. See #233.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
        if (m_rawBuffer == MAP_FAILED)
#pragma GCC diagnostic pop
        {
            CHECK_FAIL << "AlignedBuffer Failed to mmap: "
		       << std::strerror(errno)
		       << std::endl;
        }
        m_alignedBuffer = m_rawBuffer;
#endif
    }

    AlignedBuffer::~AlignedBuffer()
    {
        if (m_rawBuffer != nullptr)
        {
#ifdef BITFUNNEL_PLATFORM_WINDOWS
            VirtualFree(m_rawBuffer, 0, MEM_RELEASE);
#else
            // TODO: note that munamp == -1 indicates failure and we don't check
            // for failure. We should at least log this error.
            munmap(m_rawBuffer, m_actualSize);
#endif
        }
    }

    void *AlignedBuffer::GetBuffer() const
    {
        return m_alignedBuffer;
    }

    size_t AlignedBuffer::GetSize() const
    {
        return m_requestedSize;
    }
}
