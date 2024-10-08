/*****************************************************************************/
/**
 * @file    geTime.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2015/02/18
 * @brief   Manages all time related functionality.
 *
 * Manages all time related functionality.
 *
 * @bug     No known bugs.
 */
/*****************************************************************************/
#pragma once

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "gePrerequisitesUtilities.h"
#include "geModule.h"

namespace geEngineSDK {
  using std::atomic;
  using std::time_t;

  /**
   * @brief Manages all time related functionality.
   * @note  Sim thread only unless where specified otherwise.
   */
  class GE_UTILITIES_EXPORT Time : public Module<Time>
  {
   public:
    Time();
    ~Time();

    /**
     * @brief Gets the time elapsed since application start. Only gets updated once per frame.
     * @return  The time since application start, in seconds.
     */
    float
    getTime() const {
      return m_timeSinceStart;
    }

    /**
     * @copydoc Time::getTime()
     */
    uint64 getTimeMs() const {
      return m_timeSinceStartMs;
    }

    /**
     * @brief Gets the time since last frame was executed. Only gets updated once per frame.
     * @return  Time since last frame was executed, in seconds.
     */
    float
    getFrameDelta() const {
      return m_frameDelta;
    }

    /**
     * @brief Returns the step (in seconds) between fixed frame updates.
     */
    float
    getFixedFrameDelta() const {
      return static_cast<float>(m_fixedStep * MICROSEC_TO_SEC);
    }

    /**
     * @brief Returns the time (in seconds) the latest frame has started.
     */
    float
    getLastFrameTime() const {
      return static_cast<float>(m_lastFrameTime * MICROSEC_TO_SEC);
    }

    /**
     * @brief Returns the time (in seconds) the latest fixed update has started.
     */
    float
    getLastFixedUpdateTime() const {
      return static_cast<float>(m_lastFixedUpdateTime * MICROSEC_TO_SEC);
    }

    /**
     * @brief Returns the sequential index of the current frame. First frame is 0.
     * @return  The current frame.
     * @note  Thread safe, but only counts sim thread frames.
     */
    uint64
    getFrameIdx() const {
      return m_currentFrame.load();
    }

    /**
     * @brief Returns the precise time since application start, in microseconds.
     *        Unlike other time methods this is not only updated every frame,
     *        but will return exact time at the moment it is called.
     * @return  Time in microseconds.
     * @note  You will generally only want to use this for performance measurements
     *        and similar. Use non-precise methods in majority of code as it is
     *        useful to keep the time value equal in all methods during a single frame.
     */
    uint64
    getTimePrecise() const;

    /**
     * @brief Gets the time at which the application was started, counting from system start.
     * @return  The time since system to application start, in milliseconds.
     */
    uint64
    getStartTimeMs() const {
      return m_appStartTime;
    }

    /**
     * @brief Gets the current date and time in textual form.
     * @param[in] isUTC Outputs the date and time in Coordinated Universal Time,
     *            otherwise in local time.
     * @return    A String containing the current date and time.
     * @note  Thread safe.
     * @note  The output format is:
     *        [DayOfWeek], [Month] [NumericalDate], [NumericalYear] [HH]::[MM]::[SS].
     */
    String
    getCurrentDateTimeString(bool isUTC);

    /**
     * @brief Gets the current time in textual form
     * @param[in]	isUTC Outputs the time in Coordinated Universal Time,
     *            otherwise in local time.
     * @return  A String containing the current time.
     * @note  Thread safe.
     * @note  The output format is [HH]::[MM]::[SS].
     */
    String
    getCurrentTimeString(bool isUTC);

    /**
     * @brief Gets the date and time where the application has been started in textual form.
     * @param[in]	isUTC Outputs the date and time in Coordinated Universal Time,
     *            otherwise in local time.
     * @return	A String containing the application startup date and time.
     * @note Thread safe.
     * @note  The output format is:
     *        [DayOfWeek], [Month] [NumericalDate], [NumericalYear] [HH]::[MM]::[SS].
     */
    String
    getAppStartUpDateString(bool isUTC);

    /**
     * @brief Called every frame. Should only be called by Application.
     */
    void
    _update();

    /**
     * @brief Calculates the number of fixed update iterations required and
     *        their step size. Values depend on the current time and previous
     *        calls to _advanceFixedUpdate().;
     * @param[out]  step  Duration of the fixed step in microseconds. In most
     *              cases this is the same duration as the	fixed time delta,
     *              but in the cases where frame is taking a very long time the
     *              step might be increased to avoid a large number of fixed
     *              updates per frame.
     * @return      Returns the number of fixed frame updates to execute
     *              (each of @p step duration). In most cases this will be
     *              either 1 or 0, or a larger amount of frames are taking a
     *              long time to execute (longer than a multiple of fixed
     *              frame step).
     */
    uint32
    _getFixedUpdateStep(uint64& step);

    /**
     * @brief Advances the fixed update timers by @p step microseconds.
     *        Should be called once for each iteration as returned by
     *        _getFixedUpdateStep(), per frame.
     */
    void
    _advanceFixedUpdate(uint64 step);

    /**
     * @brief Multiply with time in microseconds to get a time in seconds.
     */
    static const double MICROSEC_TO_SEC;
   private:
    /**
     * @brief Maximum number of fixed updates that can ever be accumulated.
     */
    static CONSTEXPR uint32 MAX_ACCUM_FIXED_UPDATES = 200;

    /**
     * @brief Determines how many new fixed updates are regenerated per frame.
     */
    static CONSTEXPR uint32 NEW_FIXED_UPDATES_PER_FRAME = 4;

    float m_frameDelta = 0.0f;      /**< Frame delta in seconds */
    float m_timeSinceStart = 0.0f;  /**< Time since start in seconds */
    uint64 m_timeSinceStartMs = 0u;
    bool m_firstFrame = true;

    uint64 m_appStartTime = 0u;     /**< Time the application started, in microseconds */
    uint64 m_lastFrameTime = 0u;    /**< Time since last runOneFrame call, In microseconds */
    atomic<uint32> m_currentFrame{0UL};

    uint64 m_fixedStep = 16666; //60 times a second in microseconds
    uint64 m_lastFixedUpdateTime = 0;
    bool m_firstFixedFrame = true;
    uint32 m_numRemainingFixedUpdates = MAX_ACCUM_FIXED_UPDATES;

    time_t m_appStartUpDate;

    Timer* m_timer;
  };

  /**
   * @brief Easier way to access the Time module.
   */
  GE_UTILITIES_EXPORT Time&
  g_time();
}
