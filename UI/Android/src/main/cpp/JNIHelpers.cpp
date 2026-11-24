/*
 * Copyright (c) 2023, Andrew Kaster <akaster@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "JNIHelpers.h"
#include <AK/Vector.h>

namespace Ladybird {

jstring JavaEnvironment::jstring_from_ak_string(String const& str)
{
    Vector<jchar> utf16;
    utf16.ensure_capacity(str.bytes().size());
    for (auto code_point : str.code_points()) {
        if (code_point <= 0xFFFF) {
            utf16.append(static_cast<jchar>(code_point));
        } else {
            u32 adjusted = code_point - 0x10000;
            utf16.append(static_cast<jchar>(0xD800 + (adjusted >> 10)));
            utf16.append(static_cast<jchar>(0xDC00 + (adjusted & 0x3FF)));
        }
    }
    return m_env->NewString(utf16.data(), static_cast<jsize>(utf16.size()));
}

}
