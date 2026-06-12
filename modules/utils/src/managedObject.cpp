#include <utils/managedObject.hpp>

namespace ioteye::utils {

size_t ManagedObject::m_idSequence = 1;

ManagedObject::ManagedObject() {
    m_id = m_idSequence;
    ++m_idSequence;
}

types::ObjID ManagedObject::getObjID() const {
    return m_id;
}

}  // namespace ioteye::utils