// Copyright (c) 2013, Robert Escriva
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Replicant nor the names of its contributors may be
//       used to endorse or promote products derived from this software without
//       specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

// C++
#include <iostream>

// Replicant
#include <replicant.h>

// Distributed Lock
#include "util.h"

int
main(int argc, const char* argv[])
{
    if (argc != 3)
    {
        std::cerr << "usage: holder <host> <port>" << std::endl;
        return EXIT_FAILURE;
    }

    replicant_client cl(argv[1], atoi(argv[2]));
    replicant_returncode status;
    const char* output;
    size_t output_sz;
    int64_t sid = cl.send("lock", "holder", "", 0,
                          &status, &output, &output_sz);

    if (!wait_for_op(&cl, sid, &status))
    {
        return EXIT_FAILURE;
    }

    if (strnlen(output, output_sz) >= output_sz)
    {
        std::cerr << "lock object returned malformed message" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "lock held by: " << output << std::endl;
    replicant_destroy_output(output, output_sz);
    return EXIT_SUCCESS;
}
