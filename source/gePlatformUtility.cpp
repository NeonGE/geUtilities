/*****************************************************************************/
/**
 * @file    gePlatformUtility.cpp
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2025/02/24
 * @brief   Platform Utilities implementation for multi-platform
 *
 * Implementation of multi-platform versions of some older system specific
 * implementations that were previously only available on Windows.
 *
 * @bug	    No known bugs.
 */
/*****************************************************************************/

/*****************************************************************************/
/**
 * Includes
 */
 /*****************************************************************************/
#include "gePrerequisitesUtilities.h"
#include "gePlatformUtility.h"

#if USING(GE_PLATFORM_WINDOWS) || USING(GE_PLATFORM_XBOX)
# include "Win32/geMinWindows.h"
#endif

#if USING(GE_PLATFORM_PS4) || USING(GE_PLATFORM_PS5)
# include <kernel.h>
#endif

namespace geEngineSDK {

  void
  PlatformUtility::terminate(bool force) {
    if (!force) {
#if USING(GE_PLATFORM_WINDOWS) || USING(GE_PLATFORM_XBOX)
      PostQuitMessage(0);
#elif USING(GE_PLATFORM_PS4) || USING(GE_PLATFORM_PS5)
      _exit(0);
#elif USING(GE_PLATFORM_LINUX) || USING(GE_PLATFORM_OSX) || \
      USING(GE_PLATFORM_ANDROID) || USING(GE_PLATFORM_IOS)
      _exit(0);
#endif
    }
    else {
#if USING(GE_PLATFORM_WINDOWS) || USING(GE_PLATFORM_XBOX)
      TerminateProcess(GetCurrentProcess(), 0);
#elif USING(GE_PLATFORM_PS4) || USING(GE_PLATFORM_PS5)
      _exit(0);
#elif USING(GE_PLATFORM_LINUX) || USING(GE_PLATFORM_OSX) || \
      USING(GE_PLATFORM_ANDROID) || USING(GE_PLATFORM_IOS)
      _exit(0);
#endif
    }
  }

} // geEngineSDK