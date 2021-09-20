/*****************************************************************************/
/**
 * @file    geBinaryCompare.cpp
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

/*****************************************************************************/
/**
 * Includes
 */
/*****************************************************************************/
#include "geBinaryCompare.h"
#include "geRTTIType.h"
#include "geDataStream.h"

namespace geEngineSDK {
  namespace impl {
    template<class T>
    struct ScopeGuard
    {
      ~ScopeGuard() {
        callback();
      }

      T callback;
    };

    template<class T>
    ScopeGuard<T>
    make_scope_guard(T callback) {
      return ScopeGuard<T>{callback};
    }
  }

  BinaryCompare::BinaryCompare()
    : m_alloc(&g_frameAlloc())
  {}

  bool
  BinaryCompare::run(IReflectable& a, IReflectable& b) {
    m_alloc->markFrame();
    bool output = compare(a, b);
    m_objectMap.clear();
    m_alloc->clear();

    return output;
  }

  bool
  BinaryCompare::compare(IReflectable& a, IReflectable& b) {
    RTTITypeBase* rtti = a.getRTTI();

    if (b.getRTTI() != rtti) {
      return false;
    }

    struct RTTIPair
    {
      RTTITypeBase* rttiA;
      RTTITypeBase* rttiB;
    };

    FrameStack<RTTIPair> rttiInstances;
    auto cleanup = impl::make_scope_guard([&]() {
      while (!rttiInstances.empty()) {
        RTTIPair rttiPair = rttiInstances.top();
        rttiPair.rttiA->onSerializationEnded(&a, m_context);
        rttiPair.rttiB->onSerializationEnded(&b, m_context);
        m_alloc->destruct(rttiPair.rttiA);
        m_alloc->destruct(rttiPair.rttiB);

        rttiInstances.pop();
      }
    });

    //If an object has base classes, we need to iterate through all of them
    do {
      RTTITypeBase* rttiInstanceA = rtti->_clone(*m_alloc);
      RTTITypeBase* rttiInstanceB = rtti->_clone(*m_alloc);
      rttiInstances.push({ rttiInstanceA, rttiInstanceB });

      rttiInstanceA->onSerializationStarted(&a, m_context);
      rttiInstanceB->onSerializationStarted(&b, m_context);

      const uint32 numFields = rtti->getNumFields();
      for (uint32 i = 0; i < numFields; ++i) {
        RTTIField* curGenericField = rtti->getField(i);
        if (curGenericField->m_isVectorType) {
          const uint32 arrayNumElemsA = curGenericField->getArraySize(rttiInstanceA, &a);
          const uint32 arrayNumElemsB = curGenericField->getArraySize(rttiInstanceB, &b);

          if (arrayNumElemsA != arrayNumElemsB) {
            return false;
          }

          switch (curGenericField->m_type)
          {
            case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
            {
              auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

              for (uint32 arrIdx = 0; arrIdx < arrayNumElemsA; ++arrIdx) {
                auto childObjectA = curField->getArrayValue(rttiInstanceA, &a, arrIdx);
                auto childObjectB = curField->getArrayValue(rttiInstanceB, &b, arrIdx);

                if (childObjectA != childObjectB) {
                  if (nullptr == childObjectA || nullptr == childObjectB) {
                    return false;
                  }

                  RTTITypeBase* childRtti = nullptr;
                  if (childObjectA->getRTTI() == childObjectB->getRTTI()) {
                    childRtti = childObjectA->getRTTI();
                  }

                  if (nullptr != childRtti) {
                    ICompare& handler = childRtti->getCompareHandler();
                    if (!handler.run(*childObjectA, *childObjectB)) {
                      return false;
                    }
                  }
                  else {
                    return false;
                  }
                }
              }

              break;
            }

            case SERIALIZABLE_FIELD_TYPE::kReflectable:
            {
              auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

              for (uint32 arrIdx = 0; arrIdx < arrayNumElemsA; ++arrIdx) {
                auto& childObjectA = curField->getArrayValue(rttiInstanceA, &a, arrIdx);
                auto& childObjectB = curField->getArrayValue(rttiInstanceB, &b, arrIdx);

                RTTITypeBase* childRtti = nullptr;
                if (childObjectA.getRTTI() == childObjectB.getRTTI()) {
                  childRtti = childObjectA.getRTTI();
                }

                if (nullptr != childRtti) {
                  ICompare& handler = childRtti->getCompareHandler();
                  if (!handler.run(childObjectA, childObjectB)) {
                    return false;
                  }
                }
                else {
                  return false;
                }
              }

              break;
            }

            case SERIALIZABLE_FIELD_TYPE::kPlain:
            {
              auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

              for (uint32 arrIdx = 0; arrIdx < arrayNumElemsA; ++arrIdx) {
                uint32 typeSizeA = 0;
                uint32 typeSizeB = 0;
                if (curField->hasDynamicSize()) {
                  typeSizeA = curField->getArrayElemDynamicSize(rttiInstanceA, &a, arrIdx);
                  typeSizeB = curField->getArrayElemDynamicSize(rttiInstanceB, &b, arrIdx);
                }
                else {
                  typeSizeA = typeSizeB = curField->getTypeSize();
                }

                if (typeSizeA != typeSizeB) {
                  return false;
                }

                // Note: Ideally avoid doing copies here, and compare field values directly
                auto dataA = ge_managed_stack_alloc(typeSizeA);
                auto dataB = ge_managed_stack_alloc(typeSizeB);

                curField->arrayElemToBuffer(rttiInstanceA, &a, arrIdx, dataA);
                curField->arrayElemToBuffer(rttiInstanceB, &b, arrIdx, dataB);

                if (memcmp(dataA, dataB, typeSizeA) != 0) {
                  return false;
                }
              }

              break;
            }

            default:
              GE_EXCEPT(InternalErrorException,
                        "Error encoding data. Encountered a type I don't "
                        "know how to encode. Type: " +
                        toString(uint32(curGenericField->m_type)) +
                        ", Is array: " +
                        toString(curGenericField->m_isVectorType));
          }
        }
        else {
          switch (curGenericField->m_type)
          {
            case SERIALIZABLE_FIELD_TYPE::kReflectablePtr:
            {
              auto curField = static_cast<RTTIReflectablePtrFieldBase*>(curGenericField);

              auto childObjectA = curField->getValue(rttiInstanceA, &a);
              auto childObjectB = curField->getValue(rttiInstanceB, &b);

              if (childObjectA != childObjectB) {
                if (nullptr == childObjectA || nullptr == childObjectB) {
                  return false;
                }

                RTTITypeBase* childRtti = nullptr;
                if (childObjectA->getRTTI() == childObjectB->getRTTI()) {
                  childRtti = childObjectA->getRTTI();
                }

                if (nullptr != childRtti) {
                  ICompare& handler = childRtti->getCompareHandler();
                  if (!handler.run(*childObjectA, *childObjectB)) {
                    return false;
                  }
                }
                else {
                  return false;
                }
              }

              break;
            }

            case SERIALIZABLE_FIELD_TYPE::kReflectable:
            {
              auto curField = static_cast<RTTIReflectableFieldBase*>(curGenericField);

              auto& childObjectA = curField->getValue(rttiInstanceA, &a);
              auto& childObjectB = curField->getValue(rttiInstanceB, &b);

              RTTITypeBase* childRtti = nullptr;
              if (childObjectA.getRTTI() == childObjectB.getRTTI()) {
                childRtti = childObjectA.getRTTI();
              }

              if (nullptr != childRtti) {
                ICompare& handler = childRtti->getCompareHandler();
                if (!handler.run(childObjectA, childObjectB)) {
                  return false;
                }
              }
              else {
                return false;
              }

              break;
            }

            case SERIALIZABLE_FIELD_TYPE::kPlain:
            {
              auto curField = static_cast<RTTIPlainFieldBase*>(curGenericField);

              uint32 typeSizeA = 0;
              uint32 typeSizeB = 0;
              if (curField->hasDynamicSize()) {
                typeSizeA = curField->getDynamicSize(rttiInstanceA, &a);
                typeSizeB = curField->getDynamicSize(rttiInstanceB, &b);
              }
              else {
                typeSizeA = typeSizeB = curField->getTypeSize();
              }

              if (typeSizeA != typeSizeB) {
                return false;
              }

              // Note: Ideally avoid doing copies here, and compare field values directly
              auto dataA = ge_managed_stack_alloc(typeSizeA);
              auto dataB = ge_managed_stack_alloc(typeSizeB);

              curField->toBuffer(rttiInstanceA, &a, dataA);
              curField->toBuffer(rttiInstanceB, &b, dataB);

              if (memcmp(dataA, dataB, typeSizeA) != 0) {
                return false;
              }

              break;
            }

            case SERIALIZABLE_FIELD_TYPE::kDataBlock:
            {
              auto curField = static_cast<RTTIManagedDataBlockFieldBase*>(curGenericField);

              uint32 dataBlockSizeA = 0, dataBlockSizeB = 0;
              auto blockStreamA = curField->getValue(rttiInstanceA, &a, dataBlockSizeA);
              auto blockStreamB = curField->getValue(rttiInstanceB, &b, dataBlockSizeB);

              if (dataBlockSizeA != dataBlockSizeB) {
                return false;
              }

              auto dataA = ge_managed_stack_alloc(dataBlockSizeA);
              auto dataB = ge_managed_stack_alloc(dataBlockSizeB);

              blockStreamA->read(dataA, dataBlockSizeA);
              blockStreamB->read(dataB, dataBlockSizeB);

              if (memcmp(dataA, dataB, dataBlockSizeA) != 0) {
                return false;
              }

              break;
            }

            default:
              GE_EXCEPT(InternalErrorException,
                        "Error encoding data. Encountered a type I don't "
                        "know how to encode. Type: " +
                        toString(uint32(curGenericField->m_type)) +
                        ", Is array: " +
                        toString(curGenericField->m_isVectorType));
          }
        }
      }

      rtti = rtti->getBaseClass();

    } while (rtti != nullptr); // Repeat until we reach the top of the inheritance hierarchy

    return true;
  }
}
