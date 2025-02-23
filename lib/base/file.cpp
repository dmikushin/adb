/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "android-base/file.h"

#include <errno.h>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <sys/types.h>

#include <memory>
#include <mutex>
#include <string.h>
#include <vector>

#include "android-base/logging.h"
#include "android-base/macros.h"  // For TEMP_FAILURE_RETRY
#include "android-base/unique_fd.h"
#include "android-base/utf8.h"

#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#if defined(_WIN32)
#define read _read
#define write _write
#include <windows.h>
#define O_CLOEXEC O_NOINHERIT
#define O_NOFOLLOW 0
#endif

namespace android {
namespace base {

// Versions of standard library APIs that support UTF-8 strings.
using namespace android::base::utf8;

bool ReadFdToString(int fd, std::string* content) {
  content->clear();

  // Although original we had small files in mind, this code gets used for
  // very large files too, where the std::string growth heuristics might not
  // be suitable. https://code.google.com/p/android/issues/detail?id=258500.
  struct stat sb;
  if (fstat(fd, &sb) != -1 && sb.st_size > 0) {
    content->reserve(sb.st_size);
  }

  char buf[BUFSIZ];
  while (1) {
    auto n = TEMP_FAILURE_RETRY([&]{ return read(fd, &buf[0], sizeof(buf)); });
    if (n == 0) return true;
    else if (n < 0) return false;
    content->append(buf, n);
  }
}

bool ReadFileToString(const std::string& path, std::string* content, bool follow_symlinks) {
  content->clear();

  int flags = O_RDONLY | O_CLOEXEC | O_BINARY | (follow_symlinks ? 0 : O_NOFOLLOW);
  android::base::unique_fd fd(TEMP_FAILURE_RETRY([&] { return open(path.c_str(), flags); }));
  if (fd == -1) {
    return false;
  }
  return ReadFdToString(fd, content);
}

bool WriteStringToFd(const std::string& content, int fd) {
  const char* p = content.data();
  size_t left = content.size();
  while (left > 0) {
    auto n = TEMP_FAILURE_RETRY([&] { return write(fd, p, left); });
    if (n == -1) {
      return false;
    }
    p += n;
    left -= n;
  }
  return true;
}

static bool CleanUpAfterFailedWrite(const std::string& path) {
  // Something went wrong. Let's not leave a corrupt file lying around.
  int saved_errno = errno;
  unlink(path.c_str());
  errno = saved_errno;
  return false;
}

#if !defined(_WIN32)
bool WriteStringToFile(const std::string& content, const std::string& path,
                       mode_t mode, uid_t owner, gid_t group,
                       bool follow_symlinks) {
  int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_BINARY |
              (follow_symlinks ? 0 : O_NOFOLLOW);
  android::base::unique_fd fd(TEMP_FAILURE_RETRY([&] { return open(path.c_str(), flags, mode); }));
  if (fd == -1) {
    PLOG(ERROR) << "android::WriteStringToFile open failed";
    return false;
  }

  // We do an explicit fchmod here because we assume that the caller really
  // meant what they said and doesn't want the umask-influenced mode.
  if (fchmod(fd, mode) == -1) {
    PLOG(ERROR) << "android::WriteStringToFile fchmod failed";
    return CleanUpAfterFailedWrite(path);
  }
  if (fchown(fd, owner, group) == -1) {
    PLOG(ERROR) << "android::WriteStringToFile fchown failed";
    return CleanUpAfterFailedWrite(path);
  }
  if (!WriteStringToFd(content, fd)) {
    PLOG(ERROR) << "android::WriteStringToFile write failed";
    return CleanUpAfterFailedWrite(path);
  }
  return true;
}
#endif

bool WriteStringToFile(const std::string& content, const std::string& path,
                       bool follow_symlinks) {
  int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC | O_BINARY |
              (follow_symlinks ? 0 : O_NOFOLLOW);
  android::base::unique_fd fd(TEMP_FAILURE_RETRY([&] { return open(path.c_str(), flags, 0666); }));
  if (fd == -1) {
    return false;
  }
  return WriteStringToFd(content, fd) || CleanUpAfterFailedWrite(path);
}

bool ReadFully(int fd, void* data, size_t byte_count) {
  uint8_t* p = reinterpret_cast<uint8_t*>(data);
  size_t remaining = byte_count;
  while (remaining > 0) {
    auto n = TEMP_FAILURE_RETRY([&] { return read(fd, p, remaining); });
    if (n <= 0) return false;
    p += n;
    remaining -= n;
  }
  return true;
}

bool WriteFully(int fd, const void* data, size_t byte_count) {
  const uint8_t* p = reinterpret_cast<const uint8_t*>(data);
  size_t remaining = byte_count;
  while (remaining > 0) {
    auto n = TEMP_FAILURE_RETRY([&] { return write(fd, p, remaining); });
    if (n == -1) return false;
    p += n;
    remaining -= n;
  }
  return true;
}

bool RemoveFileIfExists(const std::string& path, std::string* err) {
  std::error_code ec;
  bool result = std::filesystem::remove(path, ec);
  if (!result)
    *err = ec.message();
  return result;
}

#if !defined(_WIN32)
bool Readlink(const std::string& path, std::string* result) {
  result->clear();

  // Most Linux file systems (ext2 and ext4, say) limit symbolic links to
  // 4095 bytes. Since we'll copy out into the string anyway, it doesn't
  // waste memory to just start there. We add 1 so that we can recognize
  // whether it actually fit (rather than being truncated to 4095).
  std::vector<char> buf(4095 + 1);
  while (true) {
    auto size = readlink(path.c_str(), &buf[0], buf.size());
    // Unrecoverable error?
    if (size == -1) return false;
    // It fit! (If size == buf.size(), it may have been truncated.)
    if (static_cast<size_t>(size) < buf.size()) {
      result->assign(&buf[0], size);
      return true;
    }
    // Double our buffer and try again.
    buf.resize(buf.size() * 2);
  }
}
#endif

#if !defined(_WIN32)
bool Realpath(const std::string& path, std::string* result) {
  result->clear();

  char* realpath_buf = realpath(path.c_str(), nullptr);
  if (realpath_buf == nullptr) {
    return false;
  }
  result->assign(realpath_buf);
  free(realpath_buf);
  return true;
}
#endif

std::string GetExecutablePath() {
#if defined(__linux__)
  std::string path;
  android::base::Readlink("/proc/self/exe", &path);
  return path;
#elif defined(__APPLE__)
  char path[PATH_MAX + 1];
  uint32_t path_len = sizeof(path);
  int rc = _NSGetExecutablePath(path, &path_len);
  if (rc < 0) {
    std::unique_ptr<char> path_buf(new char[path_len]);
    _NSGetExecutablePath(path_buf.get(), &path_len);
    return path_buf.get();
  }
  return path;
#elif defined(_WIN32)
  std::string path;
  path.reserve(1024);
  while (1) {
    DWORD result = GetModuleFileName(NULL, path.data(), path.capacity());
    if (result == 0) return "";
    if (result != path.capacity()) {
      path.resize(result);
      break;
    }
    path.reserve(path.capacity() + 1024);
  }
  return path;
#else
#error unknown OS
#endif
}

std::string GetExecutableDirectory() {
  return Dirname(GetExecutablePath());
}

std::string Basename(const std::string& path) {
  return std::filesystem::path(path).stem().string();
}

std::string Dirname(const std::string& path) {
  return std::filesystem::path(path).remove_filename().string();
}

}  // namespace base
}  // namespace android
