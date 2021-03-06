/**
 * Copyright (c) 2012 - 2014 TideSDK contributors
 * http://www.tidesdk.org
 * Includes modified sources under the Apache 2 License
 * Copyright (c) 2008 - 2012 Appcelerator Inc
 * Refer to LICENSE for details of distribution and use.
 **/

#include "posix_pipe.h"

namespace ti
{
    PosixPipe::PosixPipe(bool isReader) :
        NativePipe(isReader),
        readHandle(-1),
        writeHandle(-1)
    {
    }

    void PosixPipe::CreateHandles()
    {
        int fds[2];
        int rc = pipe(fds);
        if (rc == 0)
        {
            readHandle = fds[0];
            writeHandle = fds[1];
        }
        else
        {
            throw ValueException::FromFormat(
                "Error creating pipe: %s (%d)", strerror(errno), errno);
        }
    }

    void PosixPipe::Close()
    {
        // Close should only be called if the process (or whatever)
        // it is attached to closes. Thus we want to make sure all
        // the output is cleared from the pipe before closing them.
        // A widowed pipe will eventually stop reading data.
        NativePipe::Close();
    }

    int PosixPipe::RawRead(char *buffer, int size)
    {
        int n;
        do
        {
            n = read(readHandle, buffer, size);
        }
        while (n < 0 && errno == EINTR);

        if (n >= 0)
        {
            return n;
        }
        else
        {
            throw ValueException::FromString("Error reading from anonymous pipe");
        }
    }

    int PosixPipe::RawWrite(const char *buffer, int size)
    {
        int n;
        do
        {
            n = write(writeHandle, buffer, size);
        }
        while (n < 0 && errno == EINTR);
        if (n >= 0)
        {
            return n;
        }
        else
        {
            throw ValueException::FromString("Error writing Bytes data to pipe");
        }
    }

    void PosixPipe::CloseNativeRead()
    {
        if (readHandle != -1)
        {
            close(this->readHandle);
            this->readHandle = -1;
        }
    }

    void PosixPipe::CloseNativeWrite()
    {
        if (writeHandle != -1)
        {
            close(writeHandle);
            this->writeHandle = -1;
        }
    }
}
