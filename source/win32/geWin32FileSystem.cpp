/*****************************************************************************/
/**
 * @file    geWin32FileSystem.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2016/09/19
 * @brief   Utility class for dealing with Win32 files.
 *
 * Utility class for dealing with Win32 files.
 *
 * @bug	    No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geFileSystem.h"

#if !USING(GE_USE_GENERIC_FILESYSTEM)

#include "geException.h"
#include "geDataStream.h"
#include "geDebug.h"
#include "geUnicode.h"
#include "Win32/geMinWindows.h"

namespace geEngineSDK {
  using std::time_t;
  using std::function;

  void
  win32_handleError(DWORD error, const WString& path) {
    switch (error)
    {
      case ERROR_FILE_NOT_FOUND:
        GE_LOG(kError, FileSystem, "File at path: \"{0}\" not found.", path);
        break;
      case ERROR_PATH_NOT_FOUND:
      case ERROR_BAD_NETPATH:
      case ERROR_CANT_RESOLVE_FILENAME:
      case ERROR_INVALID_DRIVE:
        GE_LOG(kError, FileSystem, "Path \"{0}\" not found.", path);
        break;
      case ERROR_ACCESS_DENIED:
        GE_LOG(kError, FileSystem, "Access to path \"{0}\" denied.", path);
        break;
      case ERROR_ALREADY_EXISTS:
      case ERROR_FILE_EXISTS:
        GE_LOG(kError, FileSystem, "File/folder at path \"{0}\" already exists.", path);
        break;
      case ERROR_INVALID_NAME:
      case ERROR_DIRECTORY:
      case ERROR_FILENAME_EXCED_RANGE:
      case ERROR_BAD_PATHNAME:
        GE_LOG(kError, FileSystem, "Invalid path string: \"{0}\".", path);
        break;
      case ERROR_FILE_READ_ONLY:
        GE_LOG(kError, FileSystem, "File at path \"{0}\" is read only.", path);
        break;
      case ERROR_CANNOT_MAKE:
        GE_LOG(kError, FileSystem, "Cannot create file/folder at path: \"{0}\".", path);
        break;
      case ERROR_DIR_NOT_EMPTY:
        GE_LOG(kError, FileSystem, "Directory at path \"{0}\" not empty.", path);
        break;
      case ERROR_WRITE_FAULT:
        GE_LOG(kError, FileSystem, "Error while writing a file at path \"{0}\".", path);
        break;
      case ERROR_READ_FAULT:
        GE_LOG(kError, FileSystem, "Error while reading a file at path \"{0}\".", path);
        break;
      case ERROR_SHARING_VIOLATION:
        GE_LOG(kError, FileSystem, "Sharing violation at path \"{0}\".", path);
        break;
      case ERROR_LOCK_VIOLATION:
        GE_LOG(kError, FileSystem, "Lock violation at path \"{0}\".", path);
        break;
      case ERROR_HANDLE_EOF:
        GE_LOG(kError, FileSystem, "End of file reached for file at path \"{0}\".", path);
        break;
      case ERROR_HANDLE_DISK_FULL:
      case ERROR_DISK_FULL:
        GE_LOG(kError, FileSystem, "Disk full.");
        break;
      case ERROR_NEGATIVE_SEEK:
        GE_LOG(kError, FileSystem, "Negative seek.");
        break;
      default:
        GE_LOG(kError, FileSystem, "Undefined file system exception: {0}",
               static_cast<uint32>(error));
        break;
    }
  }

  WString
  win32_getCurrentDirectory() {
    DWORD len = GetCurrentDirectoryW(0, nullptr);
    if (len > 0) {
      WString result;
      result.resize(len);

      DWORD n = GetCurrentDirectoryW(len, &result[0]);
      result.pop_back();

      if (n > 0 && n <= len) {
        if ('\\' != result[result.size() - 1]) {
          result.append(L"\\");
        }
        return result;
      }
    }

    return StringUtil::WBLANK;
  }

  WString
  win32_getTempDirectory() {
    DWORD len = GetTempPathW(0, nullptr);
    if (len > 0) {
      WString result;
      result.resize(len);

      DWORD n = GetTempPathW(len, &result[0]);

      //This kind of object doesn't need to finish with a null, so remove it
      result.pop_back();

      if (n > 0 && n <= len) {
        if ('\\' != result[result.size() - 1]) {
          result.append(L"\\");
        }

        return result;
      }
    }

    return StringUtil::WBLANK;
  }

  bool
  win32_pathExists(const WString& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    if (0xFFFFFFFF == attr) {
      switch (GetLastError())
      {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case ERROR_NOT_READY:
        case ERROR_INVALID_DRIVE:
          return false;
        default:
          win32_handleError(GetLastError(), path);
      }
    }
    return true;
  }

  bool
  win32_isDirectory(const WString& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    if (0xFFFFFFFF == attr) {
      win32_handleError(GetLastError(), path);
    }

    return (attr & FILE_ATTRIBUTE_DIRECTORY) != FALSE;
  }

  bool
  win32_isDevice(const WString& path) {
    WString ucPath = path;
    StringUtil::toUpperCase(ucPath);

    return
      ucPath.compare(0, 4, L"\\\\.\\") == 0 ||
      ucPath.compare(L"CON") == 0 ||
      ucPath.compare(L"PRN") == 0 ||
      ucPath.compare(L"AUX") == 0 ||
      ucPath.compare(L"NUL") == 0 ||
      ucPath.compare(L"LPT1") == 0 ||
      ucPath.compare(L"LPT2") == 0 ||
      ucPath.compare(L"LPT3") == 0 ||
      ucPath.compare(L"LPT4") == 0 ||
      ucPath.compare(L"LPT5") == 0 ||
      ucPath.compare(L"LPT6") == 0 ||
      ucPath.compare(L"LPT7") == 0 ||
      ucPath.compare(L"LPT8") == 0 ||
      ucPath.compare(L"LPT9") == 0 ||
      ucPath.compare(L"COM1") == 0 ||
      ucPath.compare(L"COM2") == 0 ||
      ucPath.compare(L"COM3") == 0 ||
      ucPath.compare(L"COM4") == 0 ||
      ucPath.compare(L"COM5") == 0 ||
      ucPath.compare(L"COM6") == 0 ||
      ucPath.compare(L"COM7") == 0 ||
      ucPath.compare(L"COM8") == 0 ||
      ucPath.compare(L"COM9") == 0;
  }

  bool
  win32_isFile(const WString& path) {
    return !win32_isDirectory(path) && !win32_isDevice(path);
  }

  bool
  win32_createDirectory(const WString& path) {
    if (win32_pathExists(path) && win32_isDirectory(path)) {
      return false;
    }

    if (CreateDirectoryW(path.c_str(), 0) == FALSE) {
      win32_handleError(GetLastError(), path);
    }

    return true;
  }

  uint64
  win32_getFileSize(const WString& path) {
    WIN32_FILE_ATTRIBUTE_DATA attrData;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &attrData) == FALSE) {
      win32_handleError(GetLastError(), path);
    }

    LARGE_INTEGER li;
    li.LowPart = attrData.nFileSizeLow;
    li.HighPart = attrData.nFileSizeHigh;
    return static_cast<uint64>(li.QuadPart);
  }

  time_t
  win32_getLastModifiedTime(const WString& path) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad) == 0) {
      win32_handleError(GetLastError(), path);
    }

    ULARGE_INTEGER ull;
    ull.LowPart = fad.ftLastWriteTime.dwLowDateTime;
    ull.HighPart = fad.ftLastWriteTime.dwHighDateTime;

    return static_cast<time_t>((ull.QuadPart / 10000000ULL) - 11644473600ULL);
  }

  SPtr<DataStream>
  FileSystem::openFile(const Path& fullPath, bool readOnly) {
    WString pathWString = UTF8::toWide(fullPath.toString());
    const UNICHAR* pathString = pathWString.c_str();

    if (!win32_pathExists(pathString) || !win32_isFile(pathString)) {
      GE_LOG(kWarning, Platform, "Attempting to open a file that doesn't exist: {0}",
             fullPath);
      return nullptr;
    }

    ACCESS_MODE::E accessMode = ACCESS_MODE::kREAD;
    if (!readOnly) {
      accessMode = static_cast<ACCESS_MODE::E>(accessMode | 
                                               static_cast<uint32>(ACCESS_MODE::kWRITE));
    }

    return ge_shared_ptr_new<FileDataStream>(fullPath, accessMode, true);
  }

  

  uint64
  FileSystem::getFileSize(const Path& fullPath) {
    return win32_getFileSize(UTF8::toWide(fullPath.toString()));
  }
  
  bool
  FileSystem::exists(const Path& fullPath) {
    return win32_pathExists(UTF8::toWide(fullPath.toString()));
  }

  bool
  FileSystem::isFile(const Path& fullPath) {
    WString pathStr = UTF8::toWide(fullPath.toString());
    return win32_pathExists(pathStr) && win32_isFile(pathStr);
  }

  bool
  FileSystem::isDirectory(const Path& fullPath) {
    WString pathStr = UTF8::toWide(fullPath.toString());
    return win32_pathExists(pathStr) && win32_isDirectory(pathStr);
  }

  void
  FileSystem::createDir(const Path& fullPath) {
    Path parentPath = fullPath;
    while (!exists(parentPath) && parentPath.getNumDirectories() > 0) {
      parentPath = parentPath.getParent();
    }

    for (SIZE_T i = parentPath.getNumDirectories(); i < fullPath.getNumDirectories(); ++i) {
      parentPath.append(Path(fullPath[i]));
      win32_createDirectory(UTF8::toWide(parentPath.toString()));
    }

    if (fullPath.isFile()) {
      win32_createDirectory(UTF8::toWide(fullPath.toString()));
    }
  }

  void
  FileSystem::getChildren(const Path& dirPath,
                          Vector<Path>& files,
                          Vector<Path>& directories) {
    WString findPath = UTF8::toWide(dirPath.toString());

    if (win32_isFile(findPath)) {
      return;
    }

    if (dirPath.isFile()) {
      // Assuming the file is a folder, just improperly formatted in Path
      findPath.append(L"\\*");
    }
    else {
      findPath.append(L"*");
    }

    WIN32_FIND_DATAW findData;
    HANDLE fileHandle = FindFirstFileW(findPath.c_str(), &findData);
    if (INVALID_HANDLE_VALUE == fileHandle) {
      win32_handleError(GetLastError(), findPath);
      return;
    }

    WString tempName;
    do {
      tempName = findData.cFileName;

      if (L"." != tempName && L".." != tempName) {
        Path fullPath = dirPath;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
          directories.push_back(
            fullPath.append(Path(UTF8::fromWide(tempName) + U8STRING("/")))
          );
        }
        else {
          files.push_back(fullPath.append(Path(UTF8::fromWide(tempName))));
        }
      }

      if (FindNextFileW(fileHandle, &findData) == FALSE) {
        if (GetLastError() != ERROR_NO_MORE_FILES) {
          win32_handleError(GetLastError(), findPath);
        }
        break;
      }
    } while (true);

    FindClose(fileHandle);
  }

  bool
  FileSystem::iterate(const Path& dirPath,
                      const function<bool(const Path&)>& fileCallback,
                      const function<bool(const Path&)>& dirCallback,
                      bool recursive) {
    WString findPath = UTF8::toWide(dirPath.toString());

    if (win32_isFile(findPath)) {
      return false;
    }

    if (dirPath.isFile()) {
      //Assuming the file is a folder, just improperly formatted in Path
      findPath.append(L"\\*");
    }
    else {
      findPath.append(L"*");
    }

    WIN32_FIND_DATAW findData;
    HANDLE fileHandle = FindFirstFileW(findPath.c_str(), &findData);
    if (INVALID_HANDLE_VALUE == fileHandle) {
      win32_handleError(GetLastError(), findPath);
      return false;
    }

    WString tempName;
    do {
      tempName = findData.cFileName;

      if (L"." != tempName && L".." != tempName) {
        Path fullPath = dirPath;
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
          Path childDir(fullPath.append(Path(UTF8::fromWide(tempName) + U8STRING("/"))));
          if (nullptr != dirCallback) {
            if (!dirCallback(childDir)) {
              FindClose(fileHandle);
              return false;
            }
          }

          if (recursive) {
            if (!iterate(childDir, fileCallback, dirCallback, recursive)) {
              FindClose(fileHandle);
              return false;
            }
          }
        }
        else {
          Path filePath(fullPath.append(Path(UTF8::fromWide(tempName))));
          if (nullptr != fileCallback) {
            if (!fileCallback(filePath)) {
              FindClose(fileHandle);
              return false;
            }
          }
        }
      }

      if (FindNextFileW(fileHandle, &findData) == FALSE) {
        if (GetLastError() != ERROR_NO_MORE_FILES) {
          win32_handleError(GetLastError(), findPath);
        }
        break;
      }
    } while (true);

    FindClose(fileHandle);
    return true;
  }

  time_t
  FileSystem::getLastModifiedTime(const Path& fullPath) {
    return win32_getLastModifiedTime(UTF8::toWide(fullPath.toString()));
  }

  Path
  FileSystem::getWorkingDirectoryPath() {
    const String utf8dir = UTF8::fromWide(win32_getCurrentDirectory());
    return Path(utf8dir);
  }

  Path
  FileSystem::getTempDirectoryPath() {
    const String utf8dir = UTF8::fromWide(win32_getTempDirectory());
    return Path(utf8dir);
  }

  void
  FileSystem::copyFile(const Path& from, const Path& to) {
    WString fromStr = UTF8::toWide(from.toString());
    WString toStr = UTF8::toWide(to.toString());

    if (CopyFileW(fromStr.c_str(), toStr.c_str(), FALSE) == FALSE) {
      win32_handleError(GetLastError(), fromStr);
    }
  }

  void
  FileSystem::removeFile(const Path& path) {
    WString pathStr = UTF8::toWide(path.toString());
    if (win32_isDirectory(pathStr)) {
      if (RemoveDirectoryW(pathStr.c_str()) == 0) {
        win32_handleError(GetLastError(), pathStr);
      }
    }
    else {
      if (DeleteFileW(pathStr.c_str()) == 0) {
        win32_handleError(GetLastError(), pathStr);
      }
    }
  }

  void
  FileSystem::moveFile(const Path& oldPath, const Path& newPath) {
    WString oldPathStr = UTF8::toWide(oldPath.toString());
    WString newPathStr = UTF8::toWide(newPath.toString());
    if (MoveFileW(oldPathStr.c_str(), newPathStr.c_str()) == 0) {
      win32_handleError(GetLastError(), oldPathStr);
    }
  }
}

#endif  // !USING(GE_USE_GENERIC_FILESYSTEM)