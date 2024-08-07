/*
 * SPDX-FileCopyrightText: Copyright (c) 2002-2022 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#include "nvtypes.h"

//
// This file was generated with FINN, an NVIDIA coding tool.
// Source file:      class/cl2080.finn
//

#include "nvlimits.h"
#include "cl2080_notification.h"

#define NV20_SUBDEVICE_0      (0x2080U) /* finn: Evaluated from "NV2080_ALLOC_PARAMETERS_MESSAGE_ID" */

/* NvAlloc parameteters */
#define NV2080_MAX_SUBDEVICES NV_MAX_SUBDEVICES

#define NV2080_ALLOC_PARAMETERS_MESSAGE_ID (0x2080U)

typedef struct NV2080_ALLOC_PARAMETERS {
    NvU32 subDeviceId;
} NV2080_ALLOC_PARAMETERS;

