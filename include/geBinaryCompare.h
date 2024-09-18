/*****************************************************************************/
/**
 * @file    geBinaryCompare.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2019/06/27
 * @brief   Represents an interface RTTI objects need to implement if they want
 *          to provide custom method of comparing objects for equality.
 *
 * Represents an interface RTTI objects need to implement if they want to
 * provide custom method of comparing their objects for equality.
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

namespace geEngineSDK {
  struct SerializationContext;

  /**
   * @brief Represents an interface RTTI objects need to implement if they want
   *        to provide custom method of comparing their objects for equality.
   */
  class GE_UTILITIES_EXPORT ICompare
  {
   public:
    virtual ~ICompare() = default;

    /**
     * @brief Checks if two IReflectable objects are equal.
     */
    virtual bool
    run(IReflectable& a, IReflectable& b) = 0;
  };

  /**
   * @brief Compares native IReflectable objects for equality.
   */
  class GE_UTILITIES_EXPORT BinaryCompare : public ICompare
  {
   public:
    BinaryCompare();
    virtual ~BinaryCompare() = default;

    /**
     * @copydoc ICompare::run
     */
    bool
    run(IReflectable& a, IReflectable& b) override;

   protected:
    /**
     * @brief Checks if two IReflectable objects are equal. Inserts the results
     *        into an object map so multiple references to the same object
     *        don't need to be checked twice.
     */
    bool
    compare(IReflectable& a, IReflectable& b);

    UnorderedSet<IReflectable*> m_objectMap;
    SerializationContext* m_context = nullptr;
    FrameAlloc* m_alloc = nullptr;
  };
}
