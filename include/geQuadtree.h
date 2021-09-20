/*****************************************************************************/
/**
 * @file    geQuadtree.h
 * @author  Samuel Prince (samuel.prince.quezada@gmail.com)
 * @date    2019/02/14
 * @brief   Spatial partitioning tree for 2D space.
 *
 * Spatial partitioning tree for 2D space.
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
#include "gePrerequisitesUtil.h"
#include "geMath.h"
#include "geVectorNI.h"
#include "geSIMD.h"
#include "gePoolAlloc.h"

namespace geEngineSDK {
  using std::swap;
  
  /**
   * @brief Identifier that may be used for finding an element in the quadtree.
   */
  class QuadtreeElementId
  {
   public:
    QuadtreeElementId() = default;

    QuadtreeElementId(void* node, uint32 elementIdx)
      : m_node(node),
        m_elementIdx(elementIdx)
    {}

   private:
    template<class, class>
    friend class Quadtree;

    void* m_node = nullptr;
    uint32 m_elementIdx = 0u;
  };

  /**
   * @brief Spatial partitioning tree for 2D space.
   * @tparam  ElemType  Type of elements to be stored in the tree.
   * @tparam  Options	  Class that controls various options of the tree. It
   *          must provide the following duodenum:
   *            - LoosePadding: Denominator used to determine how much padding
   *              to add to each child node. The extra padding percent is
   *              determined as (1.0f / LoosePadding). Larger padding ensures
   *              elements are less likely to get stuck on a higher node due to
   *              them straddling the boundary between the nodes.
   *            - MinElementsPerNode: Determines at which point should node's
   *              children be removed and moved back into the parent (node is
   *              collapsed). This can occur on element removal, when the
   *              element count drops below the specified number.
   *            - MaxElementsPerNode: Determines at which point should a node
   *              be split into child nodes. If an element counter moves past
   *              this number the elements will be added to child nodes, if
   *              possible. If a node is already at maximum depth, this is
   *              ignored.
   *            - MaxDepth: Maximum depth of nodes in the tree. Nodes at this
   *              depth will not be subdivided even if they element counts go
   *              past MaxElementsPerNode.
   *            -It must also provide the following methods:
   *            - "static simd::Rect2 getBounds(const ElemType&, void*)"
   *            - Returns the bounds for the provided element
   *            - "static void setElementId(const Quadtree::ElementId&, void*)"
   *            - Gets called when element's ID is first assigned or
   *              subsequently modified
   */
  template<class ElemType, class Options>
  class Quadtree
  {
    /**
     * @brief A sequential group of elements within a node.
     *        If number of elements exceeds the limit of the group multiple
     *        groups will be linked together in a linked list fashion.
     */
    struct ElementGroup
    {
      ElemType v[Options::maxElementsPerNode];
      ElementGroup* next = nullptr;
    };

    /**
     * @brief A sequential group of element bounds within a node.
     *        If number of elements exceeds the limit of the group multiple
     *        groups will be linked together in a linked list fashion.
     */
    struct ElementBoundGroup
    {
      simd::Rect2 v[Options::maxElementsPerNode];
      ElementBoundGroup* next = nullptr;
    };

    /**
     * @brief Container class for all elements (and their bounds) within a
     *        single node.
     */
    struct NodeElements
    {
      ElementGroup* values = nullptr;
      ElementBoundGroup* bounds = nullptr;
      uint32 count = 0;
    };

   public:
    /**
     * @brief Contains a reference to one of the eight child nodes in an
     *        quadtree node.
     */
    struct HChildNode
    {
      union {
        struct
        {
          uint32 x : 1;
          uint32 y : 1;
          uint32 empty : 1;
        };

        struct
        {
          uint32 index : 2;
          uint32 empty2 : 1;
        };
      };

      HChildNode()
        : empty(true)
      {}

      HChildNode(uint32 x, uint32 y)
        : x(x),
          y(y),
          empty(false)
      {}

      HChildNode(uint32 index)
        : index(index),
          empty2(false)
      {}
    };

    /**
     * @brief Contains a range of child nodes in an quadtree node.
     */
    struct NodeChildRange
    {
      union {
        struct
        {
          uint32 posX : 1;
          uint32 posY : 1;
          uint32 negX : 1;
          uint32 negY : 1;
        };

        struct
        {
          uint32 posBits : 2;
          uint32 negBits : 2;
        };

        uint32 allBits : 4;
      };

      /**
       * @brief Constructs a range overlapping no nodes.
       */
      NodeChildRange()
        : allBits(0)
      {}

      /**
       * @brief Constructs a range overlapping a single node.
       */
      NodeChildRange(HChildNode child)
        : posBits(child.index),
          negBits(~child.index)
      {}

      /**
       * @brief Checks if the range contains the provided child.
       */
      bool
      contains(HChildNode child) {
        NodeChildRange childRange(child);
        return (allBits & childRange.allBits) == childRange.allBits;
      }
    };

    /**
     * @brief Represents a single quadtree node.
     */
    class Node
    {
     public:
      /**
       * @brief Constructs a new leaf node with the specified parent.
       */
      Node(Node* parent)
        : m_parent(parent),
          m_totalNumElements(0),
          m_isLeaf(true)
      {}

      /**
       * @brief Returns a child node with the specified index. May return null.
       */
      Node*
      getChild(HChildNode child) const {
        return m_children[child.index];
      }

      /**
       * @brief Checks has the specified child node been created.
       */
      bool
      hasChild(HChildNode child) const {
        return m_children[child.index] != nullptr;
      }

     private:
      friend class ElementIterator;
      friend class Quadtree;

      /**
       * @brief Maps a global element index to a set of element groups and an
       *        index within those groups.
       */
      uint32
      mapToGroup(uint32 elementIdx,
                 ElementGroup** elements,
                 ElementBoundGroup** bounds) {
        uint32 numGroups = Math::divideAndRoundUp(m_elements.count,
                             static_cast<uint32>(Options::maxElementsPerNode));
        uint32 groupIdx = numGroups - elementIdx / Options::maxElementsPerNode - 1;

        *elements = m_elements.values;
        *bounds = m_elements.bounds;
        for (uint32 i = 0; i < groupIdx; ++i) {
          *elements = (*elements)->next;
          *bounds = (*bounds)->next;
        }

        return elementIdx % Options::maxElementsPerNode;
      }

      NodeElements m_elements;

      Node* m_parent;
      Node* m_children[4] = { nullptr, nullptr, nullptr, nullptr };

      uint32 m_totalNumElements : 31;
      uint32 m_isLeaf : 1;
    };

    /**
     * @brief Contains bounds for a specific node.
     *        This is necessary since the nodes themselves do not store bounds
     *        information. Instead we construct it on-the-fly as we traverse
     *        the tree, using this class.
     */
    class NodeBounds
    {
     public:
      NodeBounds() = default;

      /**
       * @brief Initializes a new bounds object using the provided node bounds.
       */
      NodeBounds(const simd::Rect2& bounds)
        : m_bounds(bounds) {
          static CONSTEXPR float chldExtentScl = 0.5f * (1.0f + 1.0f / Options::loosePadding);

        m_childExtent = bounds.extents.x * chldExtentScl;
        m_childOffset = bounds.extents.x - m_childExtent;
      }

      /**
       * @brief Returns the bounds of the node this object represents.
       */
      const simd::Rect2&
      getBounds() const {
        return m_bounds;
      }

      /**
       * @brief Attempts to find a child node that can fully contain the
       *        provided bounds.
       */
      HChildNode
      findContainingChild(const simd::Rect2& bounds) const {
        auto queryCenter = simd::load<simd::float32x4>(&bounds.center);

        auto nodeCenter = simd::load<simd::float32x4>(&m_bounds.center);
        auto childOffset = simd::load_splat<simd::float32x4>(&m_childOffset);

        auto negativeCenter = simd::sub(nodeCenter, childOffset);
        auto negativeDiff = simd::sub(queryCenter, negativeCenter);

        auto positiveCenter = simd::add(nodeCenter, childOffset);
        auto positiveDiff = simd::sub(positiveCenter, queryCenter);

        auto diff = simd::min(negativeDiff, positiveDiff);

        auto queryExtents = simd::load<simd::float32x4>(&bounds.extents);
        auto childExtent = simd::load_splat<simd::float32x4>(&m_childExtent);

        HChildNode output;

        simd::mask_float32x4 mask = simd::cmp_gt(simd::add(queryExtents, diff), childExtent);
        if (!simd::test_bits_any(simd::bit_cast<simd::uint32x4>(mask))) {
          auto ones = simd::make_uint<simd::uint32x4>(1, 1, 1, 1);
          auto zeroes = simd::make_uint<simd::uint32x4>(0, 0, 0, 0);

          //Find node closest to the query center
          mask = simd::cmp_gt(queryCenter, nodeCenter);
          auto result = simd::blend(ones, zeroes, mask);

          Vector4I scalarResult;
          simd::store(&scalarResult, result);

          output.x = scalarResult[0];
          output.y = scalarResult[1];

          output.empty = false;
        }

        return output;
      }

      /**
       * @brief Returns a range of child nodes that intersect the provided bounds.
       */
      NodeChildRange
      findIntersectingChildren(const simd::Rect2& bounds) const {
        auto queryCenter = simd::load<simd::float32x4>(&bounds.center);
        auto queryExtents = simd::load<simd::float32x4>(&bounds.extents);

        auto queryMax = simd::add(queryCenter, queryExtents);
        auto queryMin = simd::sub(queryCenter, queryExtents);

        auto nodeCenter = simd::load<simd::float32x4>(&m_bounds.center);
        auto childOffset = simd::load_splat<simd::float32x4>(&m_childOffset);

        auto negativeCenter = simd::sub(nodeCenter, childOffset);
        auto positiveCenter = simd::add(nodeCenter, childOffset);

        auto childExtent = simd::load_splat<simd::float32x4>(&m_childExtent);
        auto negativeMax = simd::add(negativeCenter, childExtent);
        auto positiveMin = simd::sub(positiveCenter, childExtent);

        NodeChildRange output;

        auto ones = simd::make_uint<simd::uint32x4>(1, 1, 1, 1);
        auto zeroes = simd::make_uint<simd::uint32x4>(0, 0, 0, 0);

        simd::mask_float32x4 mask = simd::cmp_gt(queryMax, positiveMin);
        simd::uint32x4 result = simd::blend(ones, zeroes, mask);

        Vector4I scalarResult;
        simd::store(&scalarResult, result);

        output.posX = scalarResult[0];
        output.posY = scalarResult[1];

        mask = simd::cmp_le(queryMin, negativeMax);
        result = simd::blend(ones, zeroes, mask);

        simd::store(&scalarResult, result);

        output.negX = scalarResult[0];
        output.negY = scalarResult[1];

        return output;
      }

      /**
       * @brief Calculates bounds for the provided child node.
       */
      NodeBounds
      getChild(HChildNode child) const {
        static CONSTEXPR const float map[2] = { -1.0f, 1.0f };

        return NodeBounds(simd::Rect2(
                 Vector2(m_bounds.center.x + m_childOffset * map[child.x],
                         m_bounds.center.y + m_childOffset * map[child.y]),
                 m_childExtent));
      }

     private:
      simd::Rect2 m_bounds;
      float m_childExtent;
      float m_childOffset;
    };

    /**
     * @brief Contains a reference to a specific quadtree node, as well as
     *        information about its bounds.
     */
    class HNode
    {
     public:
      HNode() = default;

      HNode(const Node* node, const NodeBounds& bounds)
        : m_node(node),
          m_bounds(bounds)
      {}

      /**
       * @brief Returns the referenced node.
       */
      const Node*
      getNode() const {
        return m_node;
      }

      /**
       * @brief Returns the node bounds.
       */
      const NodeBounds&
      getBounds() const {
        return m_bounds;
      }

     private:
      const Node* m_node = nullptr;
      NodeBounds m_bounds;
    };

    /**
     * @brief Iterator that iterates over quadtree nodes.
     *        By default only the first inserted node will be iterated over and
     *        it is up the the user to add new ones using pushChild().
     *        The iterator takes care of updating the node bounds accordingly.
     */
    class NodeIterator
    {
     public:
      /**
       * @brief Initializes the iterator, starting with the root quadtree node.
       */
      NodeIterator(const Quadtree& tree)
        : m_currentNode(HNode(&tree.m_root, tree.m_rootBounds)),
          m_stackAlloc()
          /*m_nodeStack(&m_stackAlloc)*/ {
        m_nodeStack.push_back(m_currentNode);
      }

      /**
       * @brief Initializes the iterator using a specific node and its bounds.
       */
      NodeIterator(const Node* node, const NodeBounds& bounds)
        : m_currentNode(HNode(node, bounds)),
          m_stackAlloc(),
          m_nodeStack(&m_stackAlloc) {
        m_nodeStack.push_back(m_currentNode);
      }

      /**
       * @brief Returns a reference to the current node. moveNext() must be
       *        called at least once and it must return true prior to
       *        attempting to access this data.
       */
      const HNode&
      getCurrent() const {
        return m_currentNode;
      }

      /**
       * @brief Moves to the next entry in the iterator.
       *        Iterator starts at a position before the first element,
       *        therefore this method must be called at least once before
       *        attempting to access the current node. If the method returns
       *        false it means the iterator end has been reached and attempting
       *        to access data will result in an error.
       */
      bool
      moveNext() {
        if (m_nodeStack.empty()) {
          m_currentNode = HNode();
          return false;
        }

        m_currentNode = m_nodeStack.back();
        m_nodeStack.erase(m_nodeStack.end() - 1);

        return true;
      }

      /**
       * @brief Inserts a child of the current node to be iterated over.
       */
      void
      pushChild(const HChildNode& child) {
        Node* childNode = m_currentNode.getNode()->getChild(child);
        NodeBounds childBounds = m_currentNode.getBounds().getChild(child);
        m_nodeStack.emplace_back(childNode, childBounds);
      }

     private:
      HNode m_currentNode;
      StaticAlloc<Options::maxDepth * 4 * sizeof(HNode), FreeAlloc> m_stackAlloc;
      //StaticVector<HNode, Options::maxDepth * 4> m_nodeStack;
      Vector<HNode> m_nodeStack;
    };

    /**
     * @brief Iterator that iterates over all elements in a single node.
     */
    class ElementIterator
    {
     public:
      ElementIterator() = default;

      /**
       * @brief Constructs an iterator that iterates over the specified node's
       *        elements.
       */
      ElementIterator(const Node* node)
        : m_currentIdx(-1),
          m_currentElemGroup(node->m_elements.values),
          m_currentBoundGroup(node->m_elements.bounds) {
        uint32 numGroups = Math::divideAndRoundUp(node->m_elements.count,
                             static_cast<uint32>(Options::maxElementsPerNode));
        m_elemsInGroup = node->m_elements.count -
                           (numGroups - 1) *
                           Options::maxElementsPerNode;
      }

      /**
       * @brief Moves to the next element in the node.
       *        Iterator starts at a position before the first element,
       *        therefore this method must be called at least once before
       *        attempting to access the current element data. If the method
       *        returns false it means iterator end has been reached and
       *        attempting to access data will result in an error.
       */
      bool
      moveNext() {
        if (!m_currentElemGroup) {
          return false;
        }

        ++m_currentIdx;

        if (static_cast<uint32>(m_currentIdx) == m_elemsInGroup) { //Next group
          m_currentElemGroup = m_currentElemGroup->next;
          m_currentBoundGroup = m_currentBoundGroup->next;
          m_elemsInGroup = Options::maxElementsPerNode; //Following groups are always full
          m_currentIdx = 0;

          if (!m_currentElemGroup) {
            return false;
          }
        }

        return true;
      }

      /**
       * @brief Returns the bounds of the current element. moveNext() must be
       *        called at least once and it must return true prior to
       *        attempting to access this data.
       */
      const simd::Rect2&
      getCurrentBounds() const {
        return m_currentBoundGroup->v[m_currentIdx];
      }

      /**
       * @brief Returns the contents of the current element.
       *        moveNext() must be called at least once and it must return true
       *        prior to attempting to access this data.
       */
      const ElemType&
      getCurrentElem() const {
        return m_currentElemGroup->v[m_currentIdx];
      }

     private:
      int32 m_currentIdx = -1;
      ElementGroup* m_currentElemGroup = nullptr;
      ElementBoundGroup* m_currentBoundGroup = nullptr;
      uint32 m_elemsInGroup = 0;
    };

    /**
     * @brief Iterators that iterates over all elements intersecting the
     *        specified Rect2.
     */
    class BoxIntersectIterator
    {
     public:
      /**
       * @brief Constructs an iterator that iterates over all elements in the
       *        specified tree that intersect the specified bounds.
       */
      BoxIntersectIterator(const Quadtree& tree, const Rect2& bounds)
        : m_nodeIter(tree),
          m_bounds(simd::Rect2(bounds))
      {}

      /**
       * @brief Returns the contents of the current element.
       *        moveNext() must be called at least once and it must return true
       *        prior to attempting to access this data.
       */
      const ElemType&
      getElement() const {
        return m_elemIter.getCurrentElem();
      }

      /**
       * @brief Moves to the next intersecting element.
       *        Iterator starts at a position before the first element,
       *        therefore this method must be called at least once before
       *        attempting to access the current element data. If the method
       *        returns false it means iterator end has been reached and
       *        attempting to access data will result in an error.
       */
      bool
      moveNext() {
        while (true) {
          //First check elements of the current node (if any)
          while (m_elemIter.moveNext()) {
            const simd::Rect2& bounds = m_elemIter.getCurrentBounds();
            if (bounds.overlaps(m_bounds)) {
              return true;
            }
          }

          //No more elements in this node, move to the next one
          if (!m_nodeIter.moveNext()) {
            return false; //No more nodes to check
          }

          const HNode& nodeRef = m_nodeIter.getCurrent();
          m_elemIter = ElementIterator(nodeRef.getNode());

          //Add all intersecting child nodes to the iterator
          NodeChildRange childRange = nodeRef.getBounds().
                                        findIntersectingChildren(m_bounds);
          for (uint32 i = 0; i < 4; ++i) {
            if (childRange.contains(i) && nodeRef.getNode()->hasChild(i)) {
              m_nodeIter.pushChild(i);
            }
          }
        }

        return false;
      }

     private:
      NodeIterator m_nodeIter;
      ElementIterator m_elemIter;
      simd::Rect2 m_bounds;
    };

    /**
     * @brief Constructs an quadtree with the specified bounds.
     * @param[in] center    Origin of the root node.
     * @param[in] extent    Extent (half-size) of the root node in all directions;
     * @param[in] context   Optional user context that will be passed along to
     *                      getBounds() and setElementId() methods on the
     *                      provided Options class.
     */
    Quadtree(const Vector2& center, float extent, void* context = nullptr)
      : m_rootBounds(simd::Rect2(center, extent)),
        m_minNodeExtent(extent * Math::pow(0.5f * (1.0f + 1.0f / Options::loosePadding),
                                           Options::maxDepth)),
        m_context(context)
    {}

    ~Quadtree() {
      destroyNode(&m_root);
    }

    /**
     * @brief Adds a new element to the quadtree.
     */
    void
    addElement(const ElemType& elem) {
      addElementToNode(elem, &m_root, m_rootBounds);
    }

    /**
     * @brief Removes an existing element from the quadtree.
     */
    void
    removeElement(const QuadtreeElementId& elemId) {
      Node* node = reinterpret_cast<Node*>(elemId.m_node);

      popElement(node, elemId.m_elementIdx);

      //Reduce element counts in this and any parent nodes, check if nodes need collapsing
      Node* iterNode = node;
      Node* nodeToCollapse = nullptr;
      while (iterNode) {
        --iterNode->m_totalNumElements;

        if (iterNode->m_totalNumElements < Options::minElementsPerNode) {
          nodeToCollapse = iterNode;
        }

        iterNode = iterNode->m_parent;
      }

      if (nodeToCollapse) { //Add all the child node elements to the current node
        ge_frame_mark();
        {
          FrameStack<Node*> todo;
          todo.push(node);

          while (!todo.empty()) {
            Node* curNode = todo.top();
            todo.pop();

            for (uint32 i = 0; i < 4; ++i) {
              if (curNode->hasChild(i)) {
                Node* childNode = curNode->getChild(i);

                ElementIterator elemIter(childNode);
                while (elemIter.moveNext()) {
                  pushElement(node, elemIter.getCurrentElem(), elemIter.getCurrentBounds());
                }

                todo.push(childNode);
              }
            }
          }
        }
        ge_frame_clear();

        node->m_isLeaf = true;

        //Recursively delete all child nodes
        for (uint32 i = 0; i < 4; ++i) {
          if (node->m_children[i]) {
            destroyNode(node->m_children[i]);

            m_nodeAlloc.destruct(node->m_children[i]);
            node->m_children[i] = nullptr;
          }
        }
      }
    }

   private:
    /**
     * @brief Adds a new element to the specified node. Potentially also subdivides the node.
     */
    void
    addElementToNode(const ElemType& elem,
                     Node* node,
                     const NodeBounds& nodeBounds) {
      simd::Rect2 elemBounds = Options::getBounds(elem, m_context);

      ++node->m_totalNumElements;
      if (node->m_isLeaf) {
        const simd::Rect2& bounds = nodeBounds.getBounds();

        //Check if the node has too many elements and should be broken up
        if ((node->m_elements.count + 1) > Options::maxElementsPerNode &&
            bounds.extents.x > m_minNodeExtent) {
          //Clear all elements from the current node
          NodeElements elements = node->m_elements;

          ElementIterator elemIter(node);
          node->m_elements = NodeElements();

          //Mark the node as non-leaf, allowing children to be created
          node->m_isLeaf = false;
          node->m_totalNumElements = 0;

          //Re-insert all previous elements into this node (likely creating child nodes)
          while (elemIter.moveNext()) {
            addElementToNode(elemIter.getCurrentElem(), node, nodeBounds);
          }

          //Free the element and bound groups from this node
          freeElements(elements);

          //Insert the current element
          addElementToNode(elem, node, nodeBounds);
        }
        else {
          //No need to sub-divide, just add the element to this node
          pushElement(node, elem, elemBounds);
        }
      }
      else {
        //Attempt to find a child the element fits into
        HChildNode child = nodeBounds.findContainingChild(elemBounds);

        if (child.empty) {
          //Element doesn't fit into a child, add it to this node
          pushElement(node, elem, elemBounds);
        }
        else {
          //Create the child node if needed, and add the element to it
          if (!node->m_children[child.index]) {
            node->m_children[child.index] = m_nodeAlloc.template construct<Node>(node);
          }

          addElementToNode(elem, node->m_children[child.index], nodeBounds.getChild(child));
        }
      }
    }

    /**
     * @brief Cleans up memory used by the provided node.
     *        Should be called instead of the node destructor.
     */
    void
    destroyNode(Node* node) {
      freeElements(node->m_elements);

      for (auto& entry : node->m_children) {
        if (entry != nullptr) {
          destroyNode(entry);
          m_nodeAlloc.destruct(entry);
        }
      }
    }

    /**
     * @brief Adds a new element to the node's element list.
     */
    void
    pushElement(Node* node, const ElemType& elem, const simd::Rect2& bounds) {
      NodeElements& elements = node->m_elements;

      uint32 freeIdx = elements.count % Options::maxElementsPerNode;
      if (0 == freeIdx) { //New group needed
        auto elementGroup = reinterpret_cast<ElementGroup*>
                              (m_elemAlloc.template construct<ElementGroup>());
        auto boundGroup = reinterpret_cast<ElementBoundGroup*>
                            (m_elemBoundsAlloc.template construct<ElementBoundGroup>());

        elementGroup->next = elements.values;
        boundGroup->next = elements.bounds;

        elements.values = elementGroup;
        elements.bounds = boundGroup;
      }

      elements.values->v[freeIdx] = elem;
      elements.bounds->v[freeIdx] = bounds;

      uint32 elementIdx = elements.count;
      Options::setElementId(elem, QuadtreeElementId(node, elementIdx), m_context);

      ++elements.count;
    }

    /**
     * @brief Removes the specified element from the node's element list.
     */
    void
    popElement(Node* node, uint32 elementIdx) {
      NodeElements& elements = node->m_elements;

      ElementGroup* elemGroup;
      ElementBoundGroup* boundGroup;
      elementIdx = node->mapToGroup(elementIdx, &elemGroup, &boundGroup);

      ElementGroup* lastElemGroup;
      ElementBoundGroup* lastBoundGroup;
      uint32 lastElementIdx = node->mapToGroup(elements.count - 1,
                                               &lastElemGroup,
                                               &lastBoundGroup);

      if (elements.count > 1) {
        swap(elemGroup->v[elementIdx], lastElemGroup->v[lastElementIdx]);
        swap(boundGroup->v[elementIdx], lastBoundGroup->v[lastElementIdx]);

        Options::setElementId(elemGroup->v[elementIdx],
                              QuadtreeElementId(node, elementIdx),
                              m_context);
      }

      if (0 == lastElementIdx) {
        //Last element in that group, remove it completely
        elements.values = lastElemGroup->next;
        elements.bounds = lastBoundGroup->next;

        m_elemAlloc.destruct(lastElemGroup);
        m_elemBoundsAlloc.destruct(lastBoundGroup);
      }

      --elements.count;
    }

    /**
     * @brief Clears all elements from a node.
     */
    void
    freeElements(NodeElements& elements) {
      //Free the element and bound groups from this node
      ElementGroup* curElemGroup = elements.values;
      while (curElemGroup) {
        ElementGroup* toDelete = curElemGroup;
        curElemGroup = curElemGroup->next;
        m_elemAlloc.destruct(toDelete);
      }

      ElementBoundGroup* curBoundGroup = elements.bounds;
      while (curBoundGroup) {
        ElementBoundGroup* toDelete = curBoundGroup;
        curBoundGroup = curBoundGroup->next;
        m_elemBoundsAlloc.destruct(toDelete);
      }

      elements.values = nullptr;
      elements.bounds = nullptr;
      elements.count = 0;
    }

    Node m_root { nullptr };
    NodeBounds m_rootBounds;
    float m_minNodeExtent;
    void* m_context;

    PoolAlloc<sizeof(Node)> m_nodeAlloc;
    PoolAlloc<sizeof(ElementGroup)> m_elemAlloc;
    PoolAlloc<sizeof(ElementBoundGroup), 512, 16> m_elemBoundsAlloc;
  };
}
