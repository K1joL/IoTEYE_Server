#include <atomic>
#include <utils/managedObject.hpp>

namespace ioteye::utils {

std::atomic<size_t> ManagedObject::m_idSequence{1};

ManagedObject::ManagedObject() {
    m_id = m_idSequence.fetch_add(1, std::memory_order_relaxed);
}

types::ObjID ManagedObject::getObjID() const {
    return m_id;
}

}  // namespace ioteye::utils