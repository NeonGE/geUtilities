/*****************************************************************************/
/**
 * @file    geString.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2015/02/22
 * @brief   Base classes for the use of Strings objects.
 *
 * Base classes for the use of Strings objects
 * (CHAR Strings - ASCII, WCHAR - UNICODE and String Streams)
 *
 * @bug	    No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
* Includes
*/
/*****************************************************************************/
#include "geDegree.h"
#include "geRadian.h"
#include "geColor.h"
#include "geVector2.h"
#include "geVector2I.h"
#include "geVector3.h"
#include "geVector4.h"
#include "geQuaternion.h"
#include "geMatrix4.h"
#include "geException.h"
#include "geLog.h"
#include "geUnicode.h"

namespace geEngineSDK {
  using std::strftime;
  using std::gmtime;
  using std::localtime;

  const String StringUtil::BLANK;
  const WString StringUtil::WBLANK;

  void
  StringUtil::trim(String& str, bool left, bool right) {
    static const String delims = " \t\r";
    trim(str, delims, left, right);
  }

  void
  StringUtil::trim(WString& str, bool left, bool right) {
    static const WString delims = L" \t\r";
    trim(str, delims, left, right);
  }

  void
  StringUtil::trim(String& str, const String& delims, bool left, bool right) {
    if (right) { //Trim right
      str.erase(str.find_last_not_of(delims) + 1);
    }
    if (left) { //Trim left
      str.erase(0, str.find_first_not_of(delims));
    }
  }

  void
  StringUtil::trim(WString& str, const WString& delims, bool left, bool right) {
    if (right) { //Trim right
      str.erase(str.find_last_not_of(delims) + 1);
    }
    if (left) { //Trim left
      str.erase(0, str.find_first_not_of(delims));
    }
  }

  Vector<String>
  StringUtil::split(const String& str, const String& delims, uint32 maxSplits) {
    return splitInternal<ANSICHAR>(str, delims, maxSplits);
  }

  Vector<WString>
  StringUtil::split(const WString& str, const WString& delims, uint32 maxSplits) {
    return splitInternal<UNICHAR>(str, delims, maxSplits);
  }

  Vector<String>
  StringUtil::tokenise(const String& str,
                       const String& singleDelims, 
                       const String& doubleDelims, 
                       uint32 maxSplits) {
    return tokeniseInternal<ANSICHAR>(str, singleDelims, doubleDelims, maxSplits);
  }

  Vector<WString>
  StringUtil::tokenise(const WString& str, 
                       const WString& singleDelims, 
                       const WString& doubleDelims, 
                       uint32 maxSplits) {
    return tokeniseInternal<UNICHAR>(str, singleDelims, doubleDelims, maxSplits);
  }

  void
  StringUtil::toLowerCase(String& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](char c) {
      return static_cast<char>(::tolower(c));
    });
  }

  void
  StringUtil::toLowerCase(WString& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t c) {
      return static_cast<wchar_t>(::tolower(c));
    });
  }

  void
  StringUtil::toUpperCase(String& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](char c) {
      return static_cast<char>(::toupper(c));
    });
  }

  void
  StringUtil::toUpperCase(WString& str) {
    std::transform(str.begin(), str.end(), str.begin(), [](wchar_t c) {
      return static_cast<wchar_t>(::toupper(c));
    });
  }

  bool
  StringUtil::startsWith(const String& str, const String& pattern, bool lowerCase) {
    return startsWithInternal<ANSICHAR>(str, pattern, lowerCase);
  }

  bool
  StringUtil::startsWith(const WString& str, const WString& pattern, bool lowerCase) {
    return startsWithInternal<UNICHAR>(str, pattern, lowerCase);
  }

  bool
  StringUtil::endsWith(const String& str, const String& pattern, bool lowerCase) {
    return endsWithInternal<ANSICHAR>(str, pattern, lowerCase);
  }

  bool
  StringUtil::endsWith(const WString& str, const WString& pattern, bool lowerCase) {
    return endsWithInternal<UNICHAR>(str, pattern, lowerCase);
  }

  bool
  StringUtil::match(const String& str, const String& pattern, bool caseSensitive) {
    return matchInternal<ANSICHAR>(str, pattern, caseSensitive);
  }

  bool
  StringUtil::match(const WString& str, const WString& pattern, bool caseSensitive) {
    return matchInternal<UNICHAR>(str, pattern, caseSensitive);
  }

  const String
  StringUtil::replaceAll(const String& source, 
                         const String& replaceWhat, 
                         const String& replaceWithWhat) {
    return replaceAllInternal<ANSICHAR>(source, replaceWhat, replaceWithWhat);
  }

  const WString 
  StringUtil::replaceAll(const WString& source, 
                         const WString& replaceWhat, 
                         const WString& replaceWithWhat) {
    return replaceAllInternal<UNICHAR>(source, replaceWhat, replaceWithWhat);
  }

  /***************************************************************************/
  /**
  * VARIOUS TO STRING CONVERSIONS
  */
  /***************************************************************************/

  WString
  toWString(const String& source) {
    auto beg = (String::const_iterator)source.begin();
    auto end = (String::const_iterator)source.end();

    return WString(beg, end);
  }

  WString
  toWString(const char* source) {
    return toWString(String(source));
  }

  WString
  toWString(float val, 
            uint16 precision, 
            uint16 width, 
            char fill, 
            ios::fmtflags flags) {
    WStringStream stream;

    stream.precision(precision);
    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(double val, 
            uint16 precision, 
            uint16 width, 
            char fill, 
            ios::fmtflags flags) {
    WStringStream stream;

    stream.precision(precision);
    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(int32 val, uint16 width, char fill, ios::fmtflags flags) {
    WStringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(uint32 val, uint16 width, char fill, ios::fmtflags flags) {
    WStringStream stream;
    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(int64 val, uint16 width, char fill, ios::fmtflags flags) {
    WStringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(uint64 val, uint16 width, char fill, ios::fmtflags flags) {
    WStringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(ANSICHAR val, uint16 width, char fill, ios::fmtflags flags) {
    WStringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  WString
  toWString(UNICHAR val, uint16 width, char fill, ios::fmtflags flags) {
    WStringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

#ifdef _INC_RADIAH_H_
  WString
  toWString(Radian val, uint16 precision, uint16 width, char fill, ios::fmtflags flags) {
    return toWString(val.valueRadians(), precision, width, fill, flags);
  }

  String
  toString(Radian val, uint16 precision, uint16 width, char fill, ios::fmtflags flags) {
    return toString(val.valueRadians(), precision, width, fill, flags);
  }
#endif // _INC_RADIAH_H_

#ifdef _INC_DEGREE_H_
  WString
  toWString(Degree val, uint16 precision, uint16 width, char fill, ios::fmtflags flags) {
    return toWString(val.valueDegrees(), precision, width, fill, flags);
  }

  String
  toString(Degree val, uint16 precision, uint16 width, char fill, ios::fmtflags flags) {
    return toString(val.valueDegrees(), precision, width, fill, flags);
  }
#endif // _INC_DEGREE_H_

#ifdef _INC_VECTOR2_H_
  WString
  toWString(const Vector2& val) {
    WStringStream stream;
    stream << L"X=" << val.x << L" Y=" << val.y;
    return stream.str();
  }

  String
  toString(const Vector2& val) {
    StringStream stream;
    stream << "X=" << val.x << " Y=" << val.y;
    return stream.str();
  }
#endif // _INC_VECTOR2_H_

#ifdef _INC_VECTOR2I_H_
  WString
  toWString(const Vector2I& val) {
    WStringStream stream;
    stream << L"X=" << val.x << L" Y=" << val.y;
    return stream.str();
  }

  String
  toString(const Vector2I & val) {
    StringStream stream;
    stream << "X=" << val.x << " Y=" << val.y;
    return stream.str();
  }
#endif // _INC_VECTOR2I_H_

#ifdef _INC_VECTOR3_H_
  WString
  toWString(const Vector3& val) {
    WStringStream stream;
    stream << val.x << L" " << val.y << L" " << val.z;
    return stream.str();
  }

  String
  toString(const Vector3& val) {
    StringStream stream;
    stream << val.x << " " << val.y << " " << val.z;
    return stream.str();
  }
#endif // _INC_VECTOR3_H_

#ifdef _INC_VECTOR4_H_
  WString
  toWString(const Vector4& val) {
    WStringStream stream;
    stream << val.x << L" " << val.y << L" " << val.z << L" " << val.w;
    return stream.str();
  }

  String
  toString(const Vector4& val) {
    StringStream stream;
    stream << val.x << " " << val.y << " " << val.z << " " << val.w;
    return stream.str();
  }
#endif // _INC_VECTOR4_H_

#ifdef _INC_MATRIX3_H_
  WString
  toWString(const Matrix3& val) {
    WStringStream stream;
    stream << val[0][0] << L" "
      << val[0][1] << L" "
      << val[0][2] << L" "
      << val[1][0] << L" "
      << val[1][1] << L" "
      << val[1][2] << L" "
      << val[2][0] << L" "
      << val[2][1] << L" "
      << val[2][2];
    return stream.str();
  }

  String
  toString(const Matrix3& val) {
    StringStream stream;
    stream << val[0][0] << " "
      << val[0][1] << " "
      << val[0][2] << " "
      << val[1][0] << " "
      << val[1][1] << " "
      << val[1][2] << " "
      << val[2][0] << " "
      << val[2][1] << " "
      << val[2][2];
    return stream.str();
  }
#endif // _INC_MATRIX3_H_

#ifdef _INC_MATRIX4_H_
  WString
  toWString(const Matrix4& val) {
    WStringStream stream;
    stream << val.m[0][0] << L" "
      << val.m[0][1] << L" "
      << val.m[0][2] << L" "
      << val.m[0][3] << L" "
      << val.m[1][0] << L" "
      << val.m[1][1] << L" "
      << val.m[1][2] << L" "
      << val.m[1][3] << L" "
      << val.m[2][0] << L" "
      << val.m[2][1] << L" "
      << val.m[2][2] << L" "
      << val.m[2][3] << L" "
      << val.m[3][0] << L" "
      << val.m[3][1] << L" "
      << val.m[3][2] << L" "
      << val.m[3][3];
    return stream.str();
  }

  String
  toString(const Matrix4& val) {
    StringStream stream;
    stream << val.m[0][0] << " "
      << val.m[0][1] << " "
      << val.m[0][2] << " "
      << val.m[0][3] << " "
      << val.m[1][0] << " "
      << val.m[1][1] << " "
      << val.m[1][2] << " "
      << val.m[1][3] << " "
      << val.m[2][0] << " "
      << val.m[2][1] << " "
      << val.m[2][2] << " "
      << val.m[2][3] << " "
      << val.m[3][0] << " "
      << val.m[3][1] << " "
      << val.m[3][2] << " "
      << val.m[3][3];
    return stream.str();
  }
#endif // _INC_MATRIX4_H_

#ifdef _INC_QUATERNION_H_
  WString
  toWString(const Quaternion& val) {
    WStringStream stream;
    stream << val.w << L" " << val.x << L" " << val.y << L" " << val.z;
    return stream.str();
  }

  String
  toString(const Quaternion& val) {
    StringStream stream;
    stream << val.w << " " << val.x << " " << val.y << " " << val.z;
    return stream.str();
  }
#endif // _INC_QUATERNION_H_

#ifdef _INC_COLOR_H_
  WString
  toWString(const LinearColor& val) {
    WStringStream stream;
    stream << L"R=" << val.r << L" G=" << val.g << L" B=" << val.b << L" A=" << val.a;
    return stream.str();
  }

  WString
  toWString(const Color& val) {
    WStringStream stream;
    stream << L"R=" << val.r << L" G=" << val.g << L" B=" << val.b << L" A=" << val.a;
    return stream.str();
  }

  String
  toString(const LinearColor& val) {
    StringStream stream;
    stream << "R=" << val.r << " G=" << val.g << " B=" << val.b << " A=" << val.a;
    return stream.str();
  }

  String
  toString(const Color& val) {
    StringStream stream;
    stream << "R=" << val.r << " G=" << val.g << " B=" << val.b << " A=" << val.a;
    return stream.str();
  }
#endif // _INC_COLOR_H_

  WString
  toWString(bool val, bool yesNo) {
    if (val) {
      if (yesNo) {
        return L"yes";
      }
      else {
        return L"true";
      }
    }
    else {
      if (yesNo) {
        return L"no";
      }
      else {
        return L"false";
      }
    }
  }

  WString
  toWString(const Vector<geEngineSDK::WString>& val) {
    WStringStream stream;
    Vector<geEngineSDK::WString>::const_iterator i, iend, ibegin;

    ibegin = val.begin();
    iend = val.end();

    for (i = ibegin; i != iend; ++i) {
      if (i != ibegin) {
        stream << L" ";
      }

      stream << *i;
    }
    return stream.str();
  }

  String
  toString(const WString& source) {
    return UTF8::fromWide(source);
  }

  String
  toString(const wchar_t* source) {
    return toString(WString(source));
  }

  String
  toString(float val, uint16 precision, uint16 width, char fill, ios::fmtflags flags) {
    StringStream stream;

    stream.precision(precision);
    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  String
  toString(double val, uint16 precision, uint16 width, char fill, ios::fmtflags flags) {
    StringStream stream;
    stream.precision(precision);
    stream.width(width);
    stream.fill(fill);
    if (flags) {
      stream.setf(flags);
    }
    stream << val;
    return stream.str();
  }

  String
  toString(int32 val, uint16 width, char fill, ios::fmtflags flags) {
    StringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  String
  toString(uint32 val, uint16 width, char fill, ios::fmtflags flags) {
    StringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  String
  toString(int64 val, uint16 width, char fill, ios::fmtflags flags) {
    StringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags)
    {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  String
  toString(uint64 val, uint16 width, char fill, ios::fmtflags flags) {
    StringStream stream;

    stream.width(width);
    stream.fill(fill);

    if (flags) {
      stream.setf(flags);
    }
    stream << val;

    return stream.str();
  }

  String
  toString(bool val, bool yesNo) {
    if (val) {
      if (yesNo) {
        return "yes";
      }
      else {
        return "true";
      }
    }
    else {
      if (yesNo) {
        return "no";
      }
      else {
        return "false";
      }
    }
  }

  String
  toString(const UUID& val) {
    return val.toString();
  }

  String
  toString(const Path& val) {
    return val.toString();
  }

  String
  toString(const LogVerbosity& val) {
    switch (val)
    {
      case LogVerbosity::kFatal:
        return "Fatal";
      case LogVerbosity::kError:
        return "Error";
      case LogVerbosity::kWarning:
        return "Warning";
      default:
      case LogVerbosity::kInfo:
        return "Info";
      case LogVerbosity::kVerbose:
        return "Verbose";
      case LogVerbosity::kVeryVerbose:
        return "Very verbose";
    }
  }

#include <ctime>
#if USING(GE_COMPILER_MSVC)
# include <time.h>
#endif

#if USING(GE_COMPILER_MSVC)
# define LOCALTIME(tm, t) localtime_s(tm, t)
# define GMTIME(tm, t) gmtime_s(tm, t)
#else
# define LOCALTIME(tm, t) (*(tm) = *localtime(t))
# define GMTIME(tm, t) (*(tm) = *gmtime(t))
#endif

  String
  toString(time_t time,
           bool isUTC,
           bool useISO8601,
           TIME_TO_STRING_CONVERSION_TYPE::E type) {
    char out[100];
    String formatInput;
    if (useISO8601) {
      if (TIME_TO_STRING_CONVERSION_TYPE::kDate == type) {
        formatInput = "%F";
      }
      else if (TIME_TO_STRING_CONVERSION_TYPE::kTime == type) {
        formatInput = "%T";
      }
      else {
        formatInput = "%FT%TZ";
      }
    }
    else {
      if (TIME_TO_STRING_CONVERSION_TYPE::kDate == type) {
        formatInput = "%A, %B %d, %Y";
      }
      else if (type == TIME_TO_STRING_CONVERSION_TYPE::kTime) {
        formatInput = "%T";
      }
      else {
        formatInput = "%A, %B %d, %Y %T";
      }
    }

    tm timeinfo;
    if (isUTC) {
      GMTIME(&timeinfo, &time);
    }
    else {
      LOCALTIME(&timeinfo, &time);
    }
    strftime(out, sizeof(out), formatInput.c_str(), &timeinfo);

    return String(out);
  }

  String
  toString(const Vector<geEngineSDK::String>& val) {
    StringStream stream;
    Vector<geEngineSDK::String>::const_iterator i, iend, ibegin;

    ibegin = val.begin();
    iend = val.end();

    for (i = ibegin; i != iend; ++i) {
      if (i != ibegin) {
        stream << " ";
      }
      stream << *i;
    }
    return stream.str();
  }

  float
  parseFloat(const String& val, float defaultValue) {
    //Use istringstream for direct correspondence with ToString
    StringStream str(val);
    float ret = defaultValue;
    str >> ret;
    return ret;
  }

  int32
  parseInt(const String& val, int32 defaultValue) {
    //Use istringstream for direct correspondence with ToString
    StringStream str(val);
    int32 ret = defaultValue;
    str >> ret;
    return ret;
  }

  uint32
  parseUnsignedInt(const String& val, uint32 defaultValue) {
    //Use istringstream for direct correspondence with ToString
    StringStream str(val);
    int64 ret = defaultValue;
    str >> ret;
    if (ret < 0) {
      //This is a fix for windows systems where if a signed is sent,
      //it won't return 0
      return 0;
    }

    return static_cast<uint32>(ret);
  }

  int64
  parseInt64(const String & val, int64 defaultValue) {
    //Use istringstream for direct correspondence with toString
    StringStream str(val);
    int64 ret = defaultValue;
    str >> ret;
    return ret;
  }

  uint64
  parseUnsignedInt64(const String & val, uint64 defaultValue) {
    // Use istringstream for direct correspondence with toString
    StringStream str(val);
    uint64 ret = defaultValue;
    str >> ret;
    return ret;
  }

  bool
  parseBool(const String& val, bool defaultValue) {
    if (StringUtil::startsWith(val, "true") || 
        StringUtil::startsWith(val, "yes") || 
        StringUtil::startsWith(val, "1")) {
      return true;
    }
    else if (StringUtil::startsWith(val, "false") || 
             StringUtil::startsWith(val, "no") || 
             StringUtil::startsWith(val, "0")) {
      return false;
    }
    else {
      return defaultValue;
    }
  }

  bool
  isNumber(const String& val) {
    StringStream str(val);
    float tst;
    str >> tst;
    return !str.fail() && str.eof();
  }

  float
  parseFloat(const WString& val, float defaultValue) {
    //Use istringstream for direct correspondence with toString
    WStringStream str(val);
    float ret = defaultValue;
    str >> ret;

    return ret;
  }

  int32
  parseInt(const WString& val, int32 defaultValue) {
    //Use istringstream for direct correspondence with toString
    WStringStream str(val);
    int32 ret = defaultValue;
    str >> ret;

    return ret;
  }

  uint32
  parseUnsignedInt(const WString& val, uint32 defaultValue) {
    //Use istringstream for direct correspondence with toString
    WStringStream str(val);
    uint32 ret = defaultValue;
    str >> ret;

    return ret;
  }

  int64
  parseInt64(const WString & val, int64 defaultValue) {
    // Use istringstream for direct correspondence with toString
    WStringStream str(val);
    int64 ret = defaultValue;
    str >> ret;
    return ret;
  }

  uint64
  parseUnsignedInt64(const WString & val, uint64 defaultValue) {
    //Use istringstream for direct correspondence with toString
    WStringStream str(val);
    uint64 ret = defaultValue;
    str >> ret;
    return ret;
  }

  bool
  parseBool(const WString& val, bool defaultValue) {
    if (StringUtil::startsWith(val, L"true") || 
        StringUtil::startsWith(val, L"yes") || 
        StringUtil::startsWith(val, L"1")) {
      return true;
    }
    else if (StringUtil::startsWith(val, L"false") || 
             StringUtil::startsWith(val, L"no") || 
             StringUtil::startsWith(val, L"0")) {
      return false;
    }
    else {
      return defaultValue;
    }
  }

  bool
  isNumber(const WString& val) {
    WStringStream str(val);
    float tst;
    str >> tst;
    return !str.fail() && str.eof();
  }

  void __string_throwDataOverflowException() {
    GE_EXCEPT(InternalErrorException, "Data overflow! Size doesn't fit into 64 bits.");
  }
}
