#pragma once
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

#include <Private/FastDynamicCast.hpp>

namespace RHI::utils
{

template<typename InterfaceT>
  requires(std::is_object_v<InterfaceT>)
struct ObjectsTable final
{
  ObjectsTable() = default;
  ~ObjectsTable() = default;

  template<typename ObjectT, typename... Args>
    requires(std::is_base_of_v<InterfaceT, ObjectT> || std::is_same_v<InterfaceT, ObjectT>)
  ObjectT * Emplace(Args &&... args)
  {
    using WrapperT = HandledObject<ObjectT>;

    // Create the object
    auto obj = std::make_unique<WrapperT>(m_data.size(), std::forward<Args>(args)...);

    if (m_firstFreedObject != s_invalidTableIndex)
    {
      // Reuse freed slot
      TableIndex idx = m_firstFreedObject;
      FreedObject * freedObj = std::get_if<FreedObject>(&m_data[idx]);
      m_firstFreedObject = freedObj->next;

      obj->SetIndex(idx);
      auto && newObj = m_data[idx].template emplace<InterfaceUPtr>(std::move(obj));
      return FastDynamicCast<ObjectT>(newObj.get());
    }
    else
    {
      // Append new slot
      auto && newObj = m_data.emplace_back(std::move(obj));
      return FastDynamicCast<ObjectT>(std::get<InterfaceUPtr>(newObj).get());
    }
  }

  void Destroy(InterfaceT * object)
  {
    if (!object)
      return;

    auto * base = dynamic_cast<HandledObjectBase *>(object);
    if (!base)
      return; // Not an object created by this table

    TableIndex idx = base->GetIndex();
    auto && data = m_data[idx];
    data = FreedObject{m_firstFreedObject};
    m_firstFreedObject = idx;
  }

  std::unique_ptr<InterfaceT> Take(InterfaceT * object)
  {
    InterfaceUPtr result = nullptr;
    if (!object)
      return result;

    auto * base = dynamic_cast<HandledObjectBase *>(object);
    if (!base)
      return result; // Not an object created by this table

    TableIndex idx = base->GetIndex();
    result = std::move(std::get<InterfaceUPtr>(m_data[idx]));
    m_data[idx] = FreedObject{m_firstFreedObject};
    m_firstFreedObject = idx;
    return result;
  }

private:
  using InterfaceUPtr = std::unique_ptr<InterfaceT>;
  using TableIndex = size_t;
  static constexpr TableIndex s_invalidTableIndex = -1;

  struct HandledObjectBase
  {
    explicit HandledObjectBase(TableIndex i)
      : m_index(i)
    {
    }
    virtual ~HandledObjectBase() = default;
    void SetIndex(TableIndex i) noexcept { m_index = i; }
    TableIndex GetIndex() const noexcept { return m_index; }

  protected:
    TableIndex m_index = s_invalidTableIndex;
  };

  template<typename ObjectT>
  struct HandledObject final : HandledObjectBase,
                               ObjectT
  {
    template<typename... Args>
    explicit HandledObject(TableIndex i, Args &&... args)
      : HandledObjectBase(i)
      , ObjectT(std::forward<Args>(args)...)
    {
    }
    virtual ~HandledObject() override = default;
  };

  struct FreedObject;
  using StoredData = std::variant<FreedObject, InterfaceUPtr>;
  struct FreedObject
  {
    TableIndex next = s_invalidTableIndex;
  };
  std::vector<StoredData> m_data;
  TableIndex m_firstFreedObject = s_invalidTableIndex;
};

} // namespace RHI::utils
