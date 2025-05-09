﻿//===-- llvm/Support/circular_raw_ostream.h - Buffered streams --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains raw_ostream implementations for streams to do circular
// buffering of their output.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_SUPPORT_CIRCULAR_RAW_OSTREAM_H
#define LLVM_SUPPORT_CIRCULAR_RAW_OSTREAM_H

#include "llvm/Support/raw_ostream.h"

namespace llvm 
{
  /// circular_raw_ostream - A raw_ostream which *can* save its data
  /// to a circular buffer, or can pass it through directly to an
  /// underlying stream if specified with a buffer of zero.
  ///
  class circular_raw_ostream : public raw_ostream {
  public:
    /// TAKE_OWNERSHIP - Tell this stream that it owns the underlying
    /// stream and is responsible for cleanup, memory management
    /// issues, etc.
    ///
    static const bool TAKE_OWNERSHIP = true;

    /// REFERENCE_ONLY - Tell this stream it should not manage the
    /// held stream.
    ///
    static const bool REFERENCE_ONLY = false;

  private:
    /// TheStream - The real stream we output to. We set it to be
    /// unbuffered, since we're already doing our own buffering.
    ///
    raw_ostream *TheStream;

    /// OwnsStream - Are we responsible for managing the underlying
    /// stream?
    ///
    bool OwnsStream;

    /// BufferSize - The size of the buffer in bytes.
    ///
    size_t BufferSize;

    /// BufferArray - The actual buffer storage.
    ///
    char *BufferArray;

    /// Cur - Pointer to the current output point in BufferArray.
    ///
    char *Cur;

    /// Filled - Indicate whether the buffer has been completely
    /// filled.  This helps avoid garbage output.
    ///
    bool Filled;

    /// Banner - A pointer to a banner to print before dumping the
    /// log.
    ///
    const char *Banner;

    /// flushBuffer - Dump the contents of the buffer to Stream.
    ///
    void flushBuffer() {
      if (Filled)
        // Write the older portion of the buffer.
        TheStream->write(Cur, BufferArray + BufferSize - Cur);
      // Write the newer portion of the buffer.
      TheStream->write(BufferArray, Cur - BufferArray);
      Cur = BufferArray;
      Filled = false;
    }

    virtual void write_impl(const char *Ptr, size_t Size) LLVM_OVERRIDE;

    /// current_pos - Return the current position within the stream,
    /// not counting the bytes currently in the buffer.
    ///
    virtual uint64_t current_pos() const LLVM_OVERRIDE {
      // This has the same effect as calling TheStream.current_pos(),
      // but that interface is private.
      return TheStream->tell() - TheStream->GetNumBytesInBuffer();
    }

  public:
    /// circular_raw_ostream - Construct an optionally
    /// circular-buffered stream, handing it an underlying stream to
    /// do the "real" output.
    ///
    /// As a side effect, if BuffSize is nonzero, the given Stream is
    /// set to be Unbuffered.  This is because circular_raw_ostream
    /// does its own buffering, so it doesn't want another layer of
    /// buffering to be happening underneath it.
    ///
    /// "Owns" tells the circular_raw_ostream whether it is
    /// responsible for managing the held stream, doing memory
    /// management of it, etc.
    ///
    circular_raw_ostream(raw_ostream &Stream, const char *Header,
                         size_t BuffSize = 0, bool Owns = REFERENCE_ONLY) 
        : raw_ostream(/*unbuffered*/true),
            TheStream(0),
            OwnsStream(Owns),
            BufferSize(BuffSize),
            BufferArray(0),
            Filled(false),
            Banner(Header) {
      if (BufferSize != 0)
        BufferArray = new char[BufferSize];
      Cur = BufferArray;
      setStream(Stream, Owns);
    }
    explicit circular_raw_ostream()
        : raw_ostream(/*unbuffered*/true),
            TheStream(0),
            OwnsStream(REFERENCE_ONLY),
            BufferArray(0),
            Filled(false),
            Banner("") {
      Cur = BufferArray;
    }

    ~circular_raw_ostream() {
      flush();
      flushBufferWithBanner();
      releaseStream();
      delete[] BufferArray;
    }

    /// setStream - Tell the circular_raw_ostream to output a
    /// different stream.  "Owns" tells circular_raw_ostream whether
    /// it should take responsibility for managing the underlying
    /// stream.
    ///
    void setStream(raw_ostream &Stream, bool Owns = REFERENCE_ONLY) {
      releaseStream();
      TheStream = &Stream;
      OwnsStream = Owns;
    }

    /// flushBufferWithBanner - Force output of the buffer along with
    /// a small header.
    ///
    void flushBufferWithBanner();

  private:
    /// releaseStream - Delete the held stream if needed. Otherwise,
    /// transfer the buffer settings from this circular_raw_ostream
    /// back to the underlying stream.
    ///
    void releaseStream() {
      if (!TheStream)
        return;
      if (OwnsStream)
        delete TheStream;
    }
  };
} // end llvm namespace


#endif
