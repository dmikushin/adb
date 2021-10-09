/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sysdeps/stat.h"

#include <errno.h>
#include <sys/stat.h>
#ifdef _WIN32
#include "dirent_win32.h"
#endif
#include <sys/types.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#include <string>

#include <android-base/utf8.h>

template <typename>
struct argType;

template <typename R, typename A1, typename A2>
struct argType<R(A1, A2)> { using type = typename std::remove_pointer<A2>::type; };

// Version of stat() that takes a UTF-8 path.
int adb_stat(const char* path, struct adb_stat* s) {
// This definition of wstat seems to be missing from <sys/stat.h>.
#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS == 64)
#ifdef _USE_32BIT_TIME_T
#define wstat _wstat32i64
#else
#define wstat _wstat64
#endif
#else
#define wstat _wstat
#endif

    std::wstring path_wide;
    if (!android::base::UTF8ToWide(path, &path_wide)) {
        errno = ENOENT;
        return -1;
    }

    // If the path has a trailing slash, stat will fail with ENOENT regardless of whether the path
    // is a directory or not.
    bool expected_directory = false;
    while (*path_wide.rbegin() == u'/' || *path_wide.rbegin() == u'\\') {
        path_wide.pop_back();
        expected_directory = true;
    }

    typename argType<decltype(wstat)>::type st;
    int result = wstat(path_wide.c_str(), &st);
    if (result == 0 && expected_directory) {
        if (!S_ISDIR(st.st_mode)) {
            errno = ENOTDIR;
            return -1;
        }
    }

    memcpy(s, &st, sizeof(st));
    return result;
}
