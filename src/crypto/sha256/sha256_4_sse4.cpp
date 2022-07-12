//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//; Copyright (c) 2012, Intel Corporation 
//; 
//; All rights reserved. 
//; 
//; Redistribution and use in source and binary forms, with or without
//; modification, are permitted provided that the following conditions are
//; met: 
//; 
//; * Redistributions of source code must retain the above copyright
//;   notice, this list of conditions and the following disclaimer.  
//; 
//; * Redistributions in binary form must reproduce the above copyright
//;   notice, this list of conditions and the following disclaimer in the
//;   documentation and/or other materials provided with the
//;   distribution. 
//; 
//; * Neither the name of the Intel Corporation nor the names of its
//;   contributors may be used to endorse or promote products derived from
//;   this software without specific prior written permission. 
//; 
//; 
//; THIS SOFTWARE IS PROVIDED BY INTEL CORPORATION "AS IS" AND ANY
//; EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
//; PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL CORPORATION OR
//; CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
//; EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
//; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//; PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//; LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
//; NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//; SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//;
//; This code is described in an Intel White-Paper:
//; "Fast SHA-256 Implementations on Intel Architecture Processors"
//;
//; To find it, surf to https://www.intel.com/p/en_US/embedded
//; and search for that title.
//; The paper is expected to be released roughly at the end of April, 2012
//;
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//; This code schedules 1 blocks at a time, with 4 lanes per block
//;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
//
// Port to inline assembly provided by:
// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <bitcoin/system/crypto/sha256.hpp>

#include <bitcoin/system/define.hpp>
#include <bitcoin/system/endian/endian.hpp>
#include <bitcoin/system/math/math.hpp>

namespace libbitcoin {
namespace system {
namespace sha256 {

#if !defined(HAVE_INTEL_ASM)

void single_sse4(state&, const block1&) NOEXCEPT
{
    BC_ASSERT_MSG(false, "single_sse4 undefined");
}

void double_sse4(hash1&, const block1&) NOEXCEPT
{
    BC_ASSERT_MSG(false, "double_sse4 undefined");
}

#else

#ifndef VISUAL

////void single_sse4(state& state, const blocks& blocks) NOEXCEPT;
void single_sse4(uint32_t* state, const uint8_t* data, size_t blocks) NOEXCEPT
{
    BC_PUSH_WARNING(NO_ARRAY_TO_POINTER_DECAY)

    constexpr alignas(16) uint32_t k256[]
    {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
    };

    constexpr alignas(16) uint32_t flip_mask[]
    {
        0x00010203, 0x04050607, 0x08090a0b, 0x0c0d0e0f
    };

    constexpr alignas(16) uint32_t shuffle_00ba[]
    {
        0x03020100, 0x0b0a0908, 0xffffffff, 0xffffffff
    };

    constexpr alignas(16) uint32_t shuffle_dc00[]
    {
        0xffffffff, 0xffffffff, 0x03020100, 0x0b0a0908
    };

    uint32_t a, b, c, d, f, g, h, y0, y1, y2;
    uint64_t table;
    uint64_t input_end, input;
    alignas(16) uint32_t transfer[4];

    __asm__ __volatile__(
        "shl    $0x6,%2;"
        "je     Ldone_hash_%=;"
        "add    %1,%2;"
        "mov    %2,%14;"
        "mov    (%0),%3;"
        "mov    0x4(%0),%4;"
        "mov    0x8(%0),%5;"
        "mov    0xc(%0),%6;"
        "mov    0x10(%0),%k2;"
        "mov    0x14(%0),%7;"
        "mov    0x18(%0),%8;"
        "mov    0x1c(%0),%9;"
        "movdqa %18,%%xmm12;"
        "movdqa %19,%%xmm10;"
        "movdqa %20,%%xmm11;"

        "Lloop0_%=:"
        "lea    %17,%13;"
        "movdqu (%1),%%xmm4;"
        "pshufb %%xmm12,%%xmm4;"
        "movdqu 0x10(%1),%%xmm5;"
        "pshufb %%xmm12,%%xmm5;"
        "movdqu 0x20(%1),%%xmm6;"
        "pshufb %%xmm12,%%xmm6;"
        "movdqu 0x30(%1),%%xmm7;"
        "pshufb %%xmm12,%%xmm7;"
        "mov    %1,%15;"
        "mov    $3,%1;"

        "Lloop1_%=:"
        "movdqa 0x0(%13),%%xmm9;"
        "paddd  %%xmm4,%%xmm9;"
        "movdqa %%xmm9,%16;"
        "movdqa %%xmm7,%%xmm0;"
        "mov    %k2,%10;"
        "ror    $0xe,%10;"
        "mov    %3,%11;"
        "palignr $0x4,%%xmm6,%%xmm0;"
        "ror    $0x9,%11;"
        "xor    %k2,%10;"
        "mov    %7,%12;"
        "ror    $0x5,%10;"
        "movdqa %%xmm5,%%xmm1;"
        "xor    %3,%11;"
        "xor    %8,%12;"
        "paddd  %%xmm4,%%xmm0;"
        "xor    %k2,%10;"
        "and    %k2,%12;"
        "ror    $0xb,%11;"
        "palignr $0x4,%%xmm4,%%xmm1;"
        "xor    %3,%11;"
        "ror    $0x6,%10;"
        "xor    %8,%12;"
        "movdqa %%xmm1,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    %16,%12;"
        "movdqa %%xmm1,%%xmm3;"
        "mov    %3,%10;"
        "add    %12,%9;"
        "mov    %3,%12;"
        "pslld  $0x19,%%xmm1;"
        "or     %5,%10;"
        "add    %9,%6;"
        "and    %5,%12;"
        "psrld  $0x7,%%xmm2;"
        "and    %4,%10;"
        "add    %11,%9;"
        "por    %%xmm2,%%xmm1;"
        "or     %12,%10;"
        "add    %10,%9;"
        "movdqa %%xmm3,%%xmm2;"
        "mov    %6,%10;"
        "mov    %9,%11;"
        "movdqa %%xmm3,%%xmm8;"
        "ror    $0xe,%10;"
        "xor    %6,%10;"
        "mov    %k2,%12;"
        "ror    $0x9,%11;"
        "pslld  $0xe,%%xmm3;"
        "xor    %9,%11;"
        "ror    $0x5,%10;"
        "xor    %7,%12;"
        "psrld  $0x12,%%xmm2;"
        "ror    $0xb,%11;"
        "xor    %6,%10;"
        "and    %6,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm1;"
        "xor    %9,%11;"
        "xor    %7,%12;"
        "psrld  $0x3,%%xmm8;"
        "add    %10,%12;"
        "add    4+%16,%12;"
        "ror    $0x2,%11;"
        "pxor   %%xmm2,%%xmm1;"
        "mov    %9,%10;"
        "add    %12,%8;"
        "mov    %9,%12;"
        "pxor   %%xmm8,%%xmm1;"
        "or     %4,%10;"
        "add    %8,%5;"
        "and    %4,%12;"
        "pshufd $0xfa,%%xmm7,%%xmm2;"
        "and    %3,%10;"
        "add    %11,%8;"
        "paddd  %%xmm1,%%xmm0;"
        "or     %12,%10;"
        "add    %10,%8;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %5,%10;"
        "mov    %8,%11;"
        "ror    $0xe,%10;"
        "movdqa %%xmm2,%%xmm8;"
        "xor    %5,%10;"
        "ror    $0x9,%11;"
        "mov    %6,%12;"
        "xor    %8,%11;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %k2,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %5,%10;"
        "and    %5,%12;"
        "psrld  $0xa,%%xmm8;"
        "ror    $0xb,%11;"
        "xor    %8,%11;"
        "xor    %k2,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm2;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    8+%16,%12;"
        "pxor   %%xmm2,%%xmm8;"
        "mov    %8,%10;"
        "add    %12,%7;"
        "mov    %8,%12;"
        "pshufb %%xmm10,%%xmm8;"
        "or     %3,%10;"
        "add    %7,%4;"
        "and    %3,%12;"
        "paddd  %%xmm8,%%xmm0;"
        "and    %9,%10;"
        "add    %11,%7;"
        "pshufd $0x50,%%xmm0,%%xmm2;"
        "or     %12,%10;"
        "add    %10,%7;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %4,%10;"
        "ror    $0xe,%10;"
        "mov    %7,%11;"
        "movdqa %%xmm2,%%xmm4;"
        "ror    $0x9,%11;"
        "xor    %4,%10;"
        "mov    %5,%12;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %7,%11;"
        "xor    %6,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %4,%10;"
        "and    %4,%12;"
        "ror    $0xb,%11;"
        "psrld  $0xa,%%xmm4;"
        "xor    %7,%11;"
        "ror    $0x6,%10;"
        "xor    %6,%12;"
        "pxor   %%xmm3,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    12+%16,%12;"
        "pxor   %%xmm2,%%xmm4;"
        "mov    %7,%10;"
        "add    %12,%k2;"
        "mov    %7,%12;"
        "pshufb %%xmm11,%%xmm4;"
        "or     %9,%10;"
        "add    %k2,%3;"
        "and    %9,%12;"
        "paddd  %%xmm0,%%xmm4;"
        "and    %8,%10;"
        "add    %11,%k2;"
        "or     %12,%10;"
        "add    %10,%k2;"
        "movdqa 0x10(%13),%%xmm9;"
        "paddd  %%xmm5,%%xmm9;"
        "movdqa %%xmm9,%16;"
        "movdqa %%xmm4,%%xmm0;"
        "mov    %3,%10;"
        "ror    $0xe,%10;"
        "mov    %k2,%11;"
        "palignr $0x4,%%xmm7,%%xmm0;"
        "ror    $0x9,%11;"
        "xor    %3,%10;"
        "mov    %4,%12;"
        "ror    $0x5,%10;"
        "movdqa %%xmm6,%%xmm1;"
        "xor    %k2,%11;"
        "xor    %5,%12;"
        "paddd  %%xmm5,%%xmm0;"
        "xor    %3,%10;"
        "and    %3,%12;"
        "ror    $0xb,%11;"
        "palignr $0x4,%%xmm5,%%xmm1;"
        "xor    %k2,%11;"
        "ror    $0x6,%10;"
        "xor    %5,%12;"
        "movdqa %%xmm1,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    %16,%12;"
        "movdqa %%xmm1,%%xmm3;"
        "mov    %k2,%10;"
        "add    %12,%6;"
        "mov    %k2,%12;"
        "pslld  $0x19,%%xmm1;"
        "or     %8,%10;"
        "add    %6,%9;"
        "and    %8,%12;"
        "psrld  $0x7,%%xmm2;"
        "and    %7,%10;"
        "add    %11,%6;"
        "por    %%xmm2,%%xmm1;"
        "or     %12,%10;"
        "add    %10,%6;"
        "movdqa %%xmm3,%%xmm2;"
        "mov    %9,%10;"
        "mov    %6,%11;"
        "movdqa %%xmm3,%%xmm8;"
        "ror    $0xe,%10;"
        "xor    %9,%10;"
        "mov    %3,%12;"
        "ror    $0x9,%11;"
        "pslld  $0xe,%%xmm3;"
        "xor    %6,%11;"
        "ror    $0x5,%10;"
        "xor    %4,%12;"
        "psrld  $0x12,%%xmm2;"
        "ror    $0xb,%11;"
        "xor    %9,%10;"
        "and    %9,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm1;"
        "xor    %6,%11;"
        "xor    %4,%12;"
        "psrld  $0x3,%%xmm8;"
        "add    %10,%12;"
        "add    4+%16,%12;"
        "ror    $0x2,%11;"
        "pxor   %%xmm2,%%xmm1;"
        "mov    %6,%10;"
        "add    %12,%5;"
        "mov    %6,%12;"
        "pxor   %%xmm8,%%xmm1;"
        "or     %7,%10;"
        "add    %5,%8;"
        "and    %7,%12;"
        "pshufd $0xfa,%%xmm4,%%xmm2;"
        "and    %k2,%10;"
        "add    %11,%5;"
        "paddd  %%xmm1,%%xmm0;"
        "or     %12,%10;"
        "add    %10,%5;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %8,%10;"
        "mov    %5,%11;"
        "ror    $0xe,%10;"
        "movdqa %%xmm2,%%xmm8;"
        "xor    %8,%10;"
        "ror    $0x9,%11;"
        "mov    %9,%12;"
        "xor    %5,%11;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %3,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %8,%10;"
        "and    %8,%12;"
        "psrld  $0xa,%%xmm8;"
        "ror    $0xb,%11;"
        "xor    %5,%11;"
        "xor    %3,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm2;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    8+%16,%12;"
        "pxor   %%xmm2,%%xmm8;"
        "mov    %5,%10;"
        "add    %12,%4;"
        "mov    %5,%12;"
        "pshufb %%xmm10,%%xmm8;"
        "or     %k2,%10;"
        "add    %4,%7;"
        "and    %k2,%12;"
        "paddd  %%xmm8,%%xmm0;"
        "and    %6,%10;"
        "add    %11,%4;"
        "pshufd $0x50,%%xmm0,%%xmm2;"
        "or     %12,%10;"
        "add    %10,%4;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %7,%10;"
        "ror    $0xe,%10;"
        "mov    %4,%11;"
        "movdqa %%xmm2,%%xmm5;"
        "ror    $0x9,%11;"
        "xor    %7,%10;"
        "mov    %8,%12;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %4,%11;"
        "xor    %9,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %7,%10;"
        "and    %7,%12;"
        "ror    $0xb,%11;"
        "psrld  $0xa,%%xmm5;"
        "xor    %4,%11;"
        "ror    $0x6,%10;"
        "xor    %9,%12;"
        "pxor   %%xmm3,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    12+%16,%12;"
        "pxor   %%xmm2,%%xmm5;"
        "mov    %4,%10;"
        "add    %12,%3;"
        "mov    %4,%12;"
        "pshufb %%xmm11,%%xmm5;"
        "or     %6,%10;"
        "add    %3,%k2;"
        "and    %6,%12;"
        "paddd  %%xmm0,%%xmm5;"
        "and    %5,%10;"
        "add    %11,%3;"
        "or     %12,%10;"
        "add    %10,%3;"
        "movdqa 0x20(%13),%%xmm9;"
        "paddd  %%xmm6,%%xmm9;"
        "movdqa %%xmm9,%16;"
        "movdqa %%xmm5,%%xmm0;"
        "mov    %k2,%10;"
        "ror    $0xe,%10;"
        "mov    %3,%11;"
        "palignr $0x4,%%xmm4,%%xmm0;"
        "ror    $0x9,%11;"
        "xor    %k2,%10;"
        "mov    %7,%12;"
        "ror    $0x5,%10;"
        "movdqa %%xmm7,%%xmm1;"
        "xor    %3,%11;"
        "xor    %8,%12;"
        "paddd  %%xmm6,%%xmm0;"
        "xor    %k2,%10;"
        "and    %k2,%12;"
        "ror    $0xb,%11;"
        "palignr $0x4,%%xmm6,%%xmm1;"
        "xor    %3,%11;"
        "ror    $0x6,%10;"
        "xor    %8,%12;"
        "movdqa %%xmm1,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    %16,%12;"
        "movdqa %%xmm1,%%xmm3;"
        "mov    %3,%10;"
        "add    %12,%9;"
        "mov    %3,%12;"
        "pslld  $0x19,%%xmm1;"
        "or     %5,%10;"
        "add    %9,%6;"
        "and    %5,%12;"
        "psrld  $0x7,%%xmm2;"
        "and    %4,%10;"
        "add    %11,%9;"
        "por    %%xmm2,%%xmm1;"
        "or     %12,%10;"
        "add    %10,%9;"
        "movdqa %%xmm3,%%xmm2;"
        "mov    %6,%10;"
        "mov    %9,%11;"
        "movdqa %%xmm3,%%xmm8;"
        "ror    $0xe,%10;"
        "xor    %6,%10;"
        "mov    %k2,%12;"
        "ror    $0x9,%11;"
        "pslld  $0xe,%%xmm3;"
        "xor    %9,%11;"
        "ror    $0x5,%10;"
        "xor    %7,%12;"
        "psrld  $0x12,%%xmm2;"
        "ror    $0xb,%11;"
        "xor    %6,%10;"
        "and    %6,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm1;"
        "xor    %9,%11;"
        "xor    %7,%12;"
        "psrld  $0x3,%%xmm8;"
        "add    %10,%12;"
        "add    4+%16,%12;"
        "ror    $0x2,%11;"
        "pxor   %%xmm2,%%xmm1;"
        "mov    %9,%10;"
        "add    %12,%8;"
        "mov    %9,%12;"
        "pxor   %%xmm8,%%xmm1;"
        "or     %4,%10;"
        "add    %8,%5;"
        "and    %4,%12;"
        "pshufd $0xfa,%%xmm5,%%xmm2;"
        "and    %3,%10;"
        "add    %11,%8;"
        "paddd  %%xmm1,%%xmm0;"
        "or     %12,%10;"
        "add    %10,%8;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %5,%10;"
        "mov    %8,%11;"
        "ror    $0xe,%10;"
        "movdqa %%xmm2,%%xmm8;"
        "xor    %5,%10;"
        "ror    $0x9,%11;"
        "mov    %6,%12;"
        "xor    %8,%11;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %k2,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %5,%10;"
        "and    %5,%12;"
        "psrld  $0xa,%%xmm8;"
        "ror    $0xb,%11;"
        "xor    %8,%11;"
        "xor    %k2,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm2;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    8+%16,%12;"
        "pxor   %%xmm2,%%xmm8;"
        "mov    %8,%10;"
        "add    %12,%7;"
        "mov    %8,%12;"
        "pshufb %%xmm10,%%xmm8;"
        "or     %3,%10;"
        "add    %7,%4;"
        "and    %3,%12;"
        "paddd  %%xmm8,%%xmm0;"
        "and    %9,%10;"
        "add    %11,%7;"
        "pshufd $0x50,%%xmm0,%%xmm2;"
        "or     %12,%10;"
        "add    %10,%7;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %4,%10;"
        "ror    $0xe,%10;"
        "mov    %7,%11;"
        "movdqa %%xmm2,%%xmm6;"
        "ror    $0x9,%11;"
        "xor    %4,%10;"
        "mov    %5,%12;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %7,%11;"
        "xor    %6,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %4,%10;"
        "and    %4,%12;"
        "ror    $0xb,%11;"
        "psrld  $0xa,%%xmm6;"
        "xor    %7,%11;"
        "ror    $0x6,%10;"
        "xor    %6,%12;"
        "pxor   %%xmm3,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    12+%16,%12;"
        "pxor   %%xmm2,%%xmm6;"
        "mov    %7,%10;"
        "add    %12,%k2;"
        "mov    %7,%12;"
        "pshufb %%xmm11,%%xmm6;"
        "or     %9,%10;"
        "add    %k2,%3;"
        "and    %9,%12;"
        "paddd  %%xmm0,%%xmm6;"
        "and    %8,%10;"
        "add    %11,%k2;"
        "or     %12,%10;"
        "add    %10,%k2;"
        "movdqa 0x30(%13),%%xmm9;"
        "paddd  %%xmm7,%%xmm9;"
        "movdqa %%xmm9,%16;"
        "add    $0x40,%13;"
        "movdqa %%xmm6,%%xmm0;"
        "mov    %3,%10;"
        "ror    $0xe,%10;"
        "mov    %k2,%11;"
        "palignr $0x4,%%xmm5,%%xmm0;"
        "ror    $0x9,%11;"
        "xor    %3,%10;"
        "mov    %4,%12;"
        "ror    $0x5,%10;"
        "movdqa %%xmm4,%%xmm1;"
        "xor    %k2,%11;"
        "xor    %5,%12;"
        "paddd  %%xmm7,%%xmm0;"
        "xor    %3,%10;"
        "and    %3,%12;"
        "ror    $0xb,%11;"
        "palignr $0x4,%%xmm7,%%xmm1;"
        "xor    %k2,%11;"
        "ror    $0x6,%10;"
        "xor    %5,%12;"
        "movdqa %%xmm1,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    %16,%12;"
        "movdqa %%xmm1,%%xmm3;"
        "mov    %k2,%10;"
        "add    %12,%6;"
        "mov    %k2,%12;"
        "pslld  $0x19,%%xmm1;"
        "or     %8,%10;"
        "add    %6,%9;"
        "and    %8,%12;"
        "psrld  $0x7,%%xmm2;"
        "and    %7,%10;"
        "add    %11,%6;"
        "por    %%xmm2,%%xmm1;"
        "or     %12,%10;"
        "add    %10,%6;"
        "movdqa %%xmm3,%%xmm2;"
        "mov    %9,%10;"
        "mov    %6,%11;"
        "movdqa %%xmm3,%%xmm8;"
        "ror    $0xe,%10;"
        "xor    %9,%10;"
        "mov    %3,%12;"
        "ror    $0x9,%11;"
        "pslld  $0xe,%%xmm3;"
        "xor    %6,%11;"
        "ror    $0x5,%10;"
        "xor    %4,%12;"
        "psrld  $0x12,%%xmm2;"
        "ror    $0xb,%11;"
        "xor    %9,%10;"
        "and    %9,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm1;"
        "xor    %6,%11;"
        "xor    %4,%12;"
        "psrld  $0x3,%%xmm8;"
        "add    %10,%12;"
        "add    4+%16,%12;"
        "ror    $0x2,%11;"
        "pxor   %%xmm2,%%xmm1;"
        "mov    %6,%10;"
        "add    %12,%5;"
        "mov    %6,%12;"
        "pxor   %%xmm8,%%xmm1;"
        "or     %7,%10;"
        "add    %5,%8;"
        "and    %7,%12;"
        "pshufd $0xfa,%%xmm6,%%xmm2;"
        "and    %k2,%10;"
        "add    %11,%5;"
        "paddd  %%xmm1,%%xmm0;"
        "or     %12,%10;"
        "add    %10,%5;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %8,%10;"
        "mov    %5,%11;"
        "ror    $0xe,%10;"
        "movdqa %%xmm2,%%xmm8;"
        "xor    %8,%10;"
        "ror    $0x9,%11;"
        "mov    %9,%12;"
        "xor    %5,%11;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %3,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %8,%10;"
        "and    %8,%12;"
        "psrld  $0xa,%%xmm8;"
        "ror    $0xb,%11;"
        "xor    %5,%11;"
        "xor    %3,%12;"
        "ror    $0x6,%10;"
        "pxor   %%xmm3,%%xmm2;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    8+%16,%12;"
        "pxor   %%xmm2,%%xmm8;"
        "mov    %5,%10;"
        "add    %12,%4;"
        "mov    %5,%12;"
        "pshufb %%xmm10,%%xmm8;"
        "or     %k2,%10;"
        "add    %4,%7;"
        "and    %k2,%12;"
        "paddd  %%xmm8,%%xmm0;"
        "and    %6,%10;"
        "add    %11,%4;"
        "pshufd $0x50,%%xmm0,%%xmm2;"
        "or     %12,%10;"
        "add    %10,%4;"
        "movdqa %%xmm2,%%xmm3;"
        "mov    %7,%10;"
        "ror    $0xe,%10;"
        "mov    %4,%11;"
        "movdqa %%xmm2,%%xmm7;"
        "ror    $0x9,%11;"
        "xor    %7,%10;"
        "mov    %8,%12;"
        "ror    $0x5,%10;"
        "psrlq  $0x11,%%xmm2;"
        "xor    %4,%11;"
        "xor    %9,%12;"
        "psrlq  $0x13,%%xmm3;"
        "xor    %7,%10;"
        "and    %7,%12;"
        "ror    $0xb,%11;"
        "psrld  $0xa,%%xmm7;"
        "xor    %4,%11;"
        "ror    $0x6,%10;"
        "xor    %9,%12;"
        "pxor   %%xmm3,%%xmm2;"
        "ror    $0x2,%11;"
        "add    %10,%12;"
        "add    12+%16,%12;"
        "pxor   %%xmm2,%%xmm7;"
        "mov    %4,%10;"
        "add    %12,%3;"
        "mov    %4,%12;"
        "pshufb %%xmm11,%%xmm7;"
        "or     %6,%10;"
        "add    %3,%k2;"
        "and    %6,%12;"
        "paddd  %%xmm0,%%xmm7;"
        "and    %5,%10;"
        "add    %11,%3;"
        "or     %12,%10;"
        "add    %10,%3;"
        "sub    $0x1,%1;"
        "jne    Lloop1_%=;"
        "mov    $0x2,%1;"

        "Lloop2_%=:"
        "paddd  0x0(%13),%%xmm4;"
        "movdqa %%xmm4,%16;"
        "mov    %k2,%10;"
        "ror    $0xe,%10;"
        "mov    %3,%11;"
        "xor    %k2,%10;"
        "ror    $0x9,%11;"
        "mov    %7,%12;"
        "xor    %3,%11;"
        "ror    $0x5,%10;"
        "xor    %8,%12;"
        "xor    %k2,%10;"
        "ror    $0xb,%11;"
        "and    %k2,%12;"
        "xor    %3,%11;"
        "ror    $0x6,%10;"
        "xor    %8,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    %16,%12;"
        "mov    %3,%10;"
        "add    %12,%9;"
        "mov    %3,%12;"
        "or     %5,%10;"
        "add    %9,%6;"
        "and    %5,%12;"
        "and    %4,%10;"
        "add    %11,%9;"
        "or     %12,%10;"
        "add    %10,%9;"
        "mov    %6,%10;"
        "ror    $0xe,%10;"
        "mov    %9,%11;"
        "xor    %6,%10;"
        "ror    $0x9,%11;"
        "mov    %k2,%12;"
        "xor    %9,%11;"
        "ror    $0x5,%10;"
        "xor    %7,%12;"
        "xor    %6,%10;"
        "ror    $0xb,%11;"
        "and    %6,%12;"
        "xor    %9,%11;"
        "ror    $0x6,%10;"
        "xor    %7,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    4+%16,%12;"
        "mov    %9,%10;"
        "add    %12,%8;"
        "mov    %9,%12;"
        "or     %4,%10;"
        "add    %8,%5;"
        "and    %4,%12;"
        "and    %3,%10;"
        "add    %11,%8;"
        "or     %12,%10;"
        "add    %10,%8;"
        "mov    %5,%10;"
        "ror    $0xe,%10;"
        "mov    %8,%11;"
        "xor    %5,%10;"
        "ror    $0x9,%11;"
        "mov    %6,%12;"
        "xor    %8,%11;"
        "ror    $0x5,%10;"
        "xor    %k2,%12;"
        "xor    %5,%10;"
        "ror    $0xb,%11;"
        "and    %5,%12;"
        "xor    %8,%11;"
        "ror    $0x6,%10;"
        "xor    %k2,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    8+%16,%12;"
        "mov    %8,%10;"
        "add    %12,%7;"
        "mov    %8,%12;"
        "or     %3,%10;"
        "add    %7,%4;"
        "and    %3,%12;"
        "and    %9,%10;"
        "add    %11,%7;"
        "or     %12,%10;"
        "add    %10,%7;"
        "mov    %4,%10;"
        "ror    $0xe,%10;"
        "mov    %7,%11;"
        "xor    %4,%10;"
        "ror    $0x9,%11;"
        "mov    %5,%12;"
        "xor    %7,%11;"
        "ror    $0x5,%10;"
        "xor    %6,%12;"
        "xor    %4,%10;"
        "ror    $0xb,%11;"
        "and    %4,%12;"
        "xor    %7,%11;"
        "ror    $0x6,%10;"
        "xor    %6,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    12+%16,%12;"
        "mov    %7,%10;"
        "add    %12,%k2;"
        "mov    %7,%12;"
        "or     %9,%10;"
        "add    %k2,%3;"
        "and    %9,%12;"
        "and    %8,%10;"
        "add    %11,%k2;"
        "or     %12,%10;"
        "add    %10,%k2;"
        "paddd  0x10(%13),%%xmm5;"
        "movdqa %%xmm5,%16;"
        "add    $0x20,%13;"
        "mov    %3,%10;"
        "ror    $0xe,%10;"
        "mov    %k2,%11;"
        "xor    %3,%10;"
        "ror    $0x9,%11;"
        "mov    %4,%12;"
        "xor    %k2,%11;"
        "ror    $0x5,%10;"
        "xor    %5,%12;"
        "xor    %3,%10;"
        "ror    $0xb,%11;"
        "and    %3,%12;"
        "xor    %k2,%11;"
        "ror    $0x6,%10;"
        "xor    %5,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    %16,%12;"
        "mov    %k2,%10;"
        "add    %12,%6;"
        "mov    %k2,%12;"
        "or     %8,%10;"
        "add    %6,%9;"
        "and    %8,%12;"
        "and    %7,%10;"
        "add    %11,%6;"
        "or     %12,%10;"
        "add    %10,%6;"
        "mov    %9,%10;"
        "ror    $0xe,%10;"
        "mov    %6,%11;"
        "xor    %9,%10;"
        "ror    $0x9,%11;"
        "mov    %3,%12;"
        "xor    %6,%11;"
        "ror    $0x5,%10;"
        "xor    %4,%12;"
        "xor    %9,%10;"
        "ror    $0xb,%11;"
        "and    %9,%12;"
        "xor    %6,%11;"
        "ror    $0x6,%10;"
        "xor    %4,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    4+%16,%12;"
        "mov    %6,%10;"
        "add    %12,%5;"
        "mov    %6,%12;"
        "or     %7,%10;"
        "add    %5,%8;"
        "and    %7,%12;"
        "and    %k2,%10;"
        "add    %11,%5;"
        "or     %12,%10;"
        "add    %10,%5;"
        "mov    %8,%10;"
        "ror    $0xe,%10;"
        "mov    %5,%11;"
        "xor    %8,%10;"
        "ror    $0x9,%11;"
        "mov    %9,%12;"
        "xor    %5,%11;"
        "ror    $0x5,%10;"
        "xor    %3,%12;"
        "xor    %8,%10;"
        "ror    $0xb,%11;"
        "and    %8,%12;"
        "xor    %5,%11;"
        "ror    $0x6,%10;"
        "xor    %3,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    8+%16,%12;"
        "mov    %5,%10;"
        "add    %12,%4;"
        "mov    %5,%12;"
        "or     %k2,%10;"
        "add    %4,%7;"
        "and    %k2,%12;"
        "and    %6,%10;"
        "add    %11,%4;"
        "or     %12,%10;"
        "add    %10,%4;"
        "mov    %7,%10;"
        "ror    $0xe,%10;"
        "mov    %4,%11;"
        "xor    %7,%10;"
        "ror    $0x9,%11;"
        "mov    %8,%12;"
        "xor    %4,%11;"
        "ror    $0x5,%10;"
        "xor    %9,%12;"
        "xor    %7,%10;"
        "ror    $0xb,%11;"
        "and    %7,%12;"
        "xor    %4,%11;"
        "ror    $0x6,%10;"
        "xor    %9,%12;"
        "add    %10,%12;"
        "ror    $0x2,%11;"
        "add    12+%16,%12;"
        "mov    %4,%10;"
        "add    %12,%3;"
        "mov    %4,%12;"
        "or     %6,%10;"
        "add    %3,%k2;"
        "and    %6,%12;"
        "and    %5,%10;"
        "add    %11,%3;"
        "or     %12,%10;"
        "add    %10,%3;"
        "movdqa %%xmm6,%%xmm4;"
        "movdqa %%xmm7,%%xmm5;"
        "sub    $0x1,%1;"
        "jne    Lloop2_%=;"
        "add    (%0),%3;"
        "mov    %3,(%0);"
        "add    0x4(%0),%4;"
        "mov    %4,0x4(%0);"
        "add    0x8(%0),%5;"
        "mov    %5,0x8(%0);"
        "add    0xc(%0),%6;"
        "mov    %6,0xc(%0);"
        "add    0x10(%0),%k2;"
        "mov    %k2,0x10(%0);"
        "add    0x14(%0),%7;"
        "mov    %7,0x14(%0);"
        "add    0x18(%0),%8;"
        "mov    %8,0x18(%0);"
        "add    0x1c(%0),%9;"
        "mov    %9,0x1c(%0);"
        "mov    %15,%1;"
        "add    $0x40,%1;"
        "cmp    %14,%1;"
        "jne    Lloop0_%=;"

        "Ldone_hash_%=:"

        : "+r"(state), "+r"(data), "+r"(blocks), "=r"(a), "=r"(b), "=r"(c), "=r"(d), /* e = chunk */ "=r"(f), "=r"(g), "=r"(h), "=r"(y0), "=r"(y1), "=r"(y2), "=r"(table), "+m"(input_end), "+m"(input), "+m"(transfer)
        : "m"(k256), "m"(flip_mask), "m"(shuffle_00ba), "m"(shuffle_dc00)
        : "cc", "memory", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6", "xmm7", "xmm8", "xmm9", "xmm10", "xmm11", "xmm12"
   );

   BC_POP_WARNING()
}

#endif // VISUAL

void single_sse4(state& state, const block& block) NOEXCEPT
{
    return single_sse4(state.data(), block.data(), one);
}

// ----------------------------------------------------------------------------

void single_sse4(state& state, const block1& blocks) NOEXCEPT
{
    return single_sse4(state, blocks.front());
}

void double_sse4(hash1& out, const block1& blocks) NOEXCEPT
{
    auto state = sha256::initial;
    single_sse4(state, blocks);
    single_sse4(state, sha256::pad_64);
    auto buffer = sha256::padded_32;
    to_big_endian_set(narrowing_array_cast<uint32_t, state_size>(buffer), state);
    state = sha256::initial;
    single_sse4(state, buffer);
    to_big_endian_set(array_cast<uint32_t>(out.front()), state);
}

#endif // HAVE_INTEL_ASM

} // namespace sha256
} // namespace system
} // namespace libbitcoin
