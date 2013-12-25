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

#ifndef util_h_
#define util_h_

// C++
#include <iostream>

// Replicant
#include <replicant.h>

inline void
report_error(replicant_client* cl, replicant_returncode status)
{
    std::cerr << "could not lock: "
              /* XXX << cl.last_error().message()*/
              << " [" << status << "]" << std::endl;
    (void)cl;
}

inline bool
wait_for_op(replicant_client* cl,
            int64_t op_id,
            replicant_returncode* op_status)
{
    if (op_id < 0)
    {
        report_error(cl, *op_status);
        return false;
    }

    replicant_returncode loop_status;

    if (cl->loop(op_id, -1, &loop_status) < 0)
    {
        report_error(cl, loop_status);
        return false;
    }

    if (*op_status != REPLICANT_SUCCESS)
    {
        report_error(cl, *op_status);
        return false;
    }

    return true;
}

#endif //  util_h_
