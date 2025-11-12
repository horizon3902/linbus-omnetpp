//
// Generated file, do not edit! Created by opp_msgtool 6.0 from LinDataFrame.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "LinDataFrame_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

Register_Class(LinDataFrame)

LinDataFrame::LinDataFrame(const char *name, short kind) : ::omnetpp::cPacket(name, kind)
{
}

LinDataFrame::LinDataFrame(const LinDataFrame& other) : ::omnetpp::cPacket(other)
{
    copy(other);
}

LinDataFrame::~LinDataFrame()
{
}

LinDataFrame& LinDataFrame::operator=(const LinDataFrame& other)
{
    if (this == &other) return *this;
    ::omnetpp::cPacket::operator=(other);
    copy(other);
    return *this;
}

void LinDataFrame::copy(const LinDataFrame& other)
{
    this->frameId = other.frameId;
    this->isResponse_ = other.isResponse_;
    this->dataLength = other.dataLength;
    for (size_t i = 0; i < 8; i++) {
        this->data[i] = other.data[i];
    }
}

void LinDataFrame::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::omnetpp::cPacket::parsimPack(b);
    doParsimPacking(b,this->frameId);
    doParsimPacking(b,this->isResponse_);
    doParsimPacking(b,this->dataLength);
    doParsimArrayPacking(b,this->data,8);
}

void LinDataFrame::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::omnetpp::cPacket::parsimUnpack(b);
    doParsimUnpacking(b,this->frameId);
    doParsimUnpacking(b,this->isResponse_);
    doParsimUnpacking(b,this->dataLength);
    doParsimArrayUnpacking(b,this->data,8);
}

int LinDataFrame::getFrameId() const
{
    return this->frameId;
}

void LinDataFrame::setFrameId(int frameId)
{
    this->frameId = frameId;
}

bool LinDataFrame::isResponse() const
{
    return this->isResponse_;
}

void LinDataFrame::setIsResponse(bool isResponse)
{
    this->isResponse_ = isResponse;
}

int LinDataFrame::getDataLength() const
{
    return this->dataLength;
}

void LinDataFrame::setDataLength(int dataLength)
{
    this->dataLength = dataLength;
}

size_t LinDataFrame::getDataArraySize() const
{
    return 8;
}

char LinDataFrame::getData(size_t k) const
{
    if (k >= 8) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)8, (unsigned long)k);
    return this->data[k];
}

void LinDataFrame::setData(size_t k, char data)
{
    if (k >= 8) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)8, (unsigned long)k);
    this->data[k] = data;
}

class LinDataFrameDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_frameId,
        FIELD_isResponse,
        FIELD_dataLength,
        FIELD_data,
    };
  public:
    LinDataFrameDescriptor();
    virtual ~LinDataFrameDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(LinDataFrameDescriptor)

LinDataFrameDescriptor::LinDataFrameDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(LinDataFrame)), "omnetpp::cPacket")
{
    propertyNames = nullptr;
}

LinDataFrameDescriptor::~LinDataFrameDescriptor()
{
    delete[] propertyNames;
}

bool LinDataFrameDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<LinDataFrame *>(obj)!=nullptr;
}

const char **LinDataFrameDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *LinDataFrameDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int LinDataFrameDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 4+base->getFieldCount() : 4;
}

unsigned int LinDataFrameDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_frameId
        FD_ISEDITABLE,    // FIELD_isResponse
        FD_ISEDITABLE,    // FIELD_dataLength
        FD_ISARRAY | FD_ISEDITABLE,    // FIELD_data
    };
    return (field >= 0 && field < 4) ? fieldTypeFlags[field] : 0;
}

const char *LinDataFrameDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "frameId",
        "isResponse",
        "dataLength",
        "data",
    };
    return (field >= 0 && field < 4) ? fieldNames[field] : nullptr;
}

int LinDataFrameDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "frameId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "isResponse") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "dataLength") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "data") == 0) return baseIndex + 3;
    return base ? base->findField(fieldName) : -1;
}

const char *LinDataFrameDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_frameId
        "bool",    // FIELD_isResponse
        "int",    // FIELD_dataLength
        "char",    // FIELD_data
    };
    return (field >= 0 && field < 4) ? fieldTypeStrings[field] : nullptr;
}

const char **LinDataFrameDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *LinDataFrameDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int LinDataFrameDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        case FIELD_data: return 8;
        default: return 0;
    }
}

void LinDataFrameDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'LinDataFrame'", field);
    }
}

const char *LinDataFrameDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string LinDataFrameDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        case FIELD_frameId: return long2string(pp->getFrameId());
        case FIELD_isResponse: return bool2string(pp->isResponse());
        case FIELD_dataLength: return long2string(pp->getDataLength());
        case FIELD_data: return long2string(pp->getData(i));
        default: return "";
    }
}

void LinDataFrameDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        case FIELD_frameId: pp->setFrameId(string2long(value)); break;
        case FIELD_isResponse: pp->setIsResponse(string2bool(value)); break;
        case FIELD_dataLength: pp->setDataLength(string2long(value)); break;
        case FIELD_data: pp->setData(i,string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LinDataFrame'", field);
    }
}

omnetpp::cValue LinDataFrameDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        case FIELD_frameId: return pp->getFrameId();
        case FIELD_isResponse: return pp->isResponse();
        case FIELD_dataLength: return pp->getDataLength();
        case FIELD_data: return pp->getData(i);
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'LinDataFrame' as cValue -- field index out of range?", field);
    }
}

void LinDataFrameDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        case FIELD_frameId: pp->setFrameId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_isResponse: pp->setIsResponse(value.boolValue()); break;
        case FIELD_dataLength: pp->setDataLength(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_data: pp->setData(i,omnetpp::checked_int_cast<char>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LinDataFrame'", field);
    }
}

const char *LinDataFrameDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr LinDataFrameDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void LinDataFrameDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    LinDataFrame *pp = omnetpp::fromAnyPtr<LinDataFrame>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LinDataFrame'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

