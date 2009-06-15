#ifndef FIELDVALUE_H
#define FIELDVALUE_H

#include <cstdlib>
#include <ostream>

#include "dataqueue.h"

class FieldValueBase
{
public:
    virtual ~FieldValueBase();
    
    virtual void update();
    virtual void serializeTo(std::ostream& output) = 0;
    
protected:
    FieldValueBase();
};

template<typename DataSource>
class FieldValue : public FieldValueBase
{
public:
    typedef typename DataSource::value_type value_type;
    
    void update() {  dataValue = dataSource.getNextValue(); }
    value_type getValue() const { return dataValue; }

    explicit FieldValue(DataSource const& source)
    :dataSource(source)
    {
    }
    
    void serializeTo(std::ostream& output)
    {
        output << dataValue;
    }
    
private:
    value_type  dataValue;
    DataSource  dataSource;
};

class FromQueue
{
public:
    typedef DataQueue::value_type value_type;
    
    explicit FromQueue(DataQueue* queue)
    :queue_(queue)
    {
    }
    
    DataQueue::value_type getNextValue()
    {
      DataQueue::value_type val;
      queue_->getAnyValue(val);
      return val;
    }
    
private:
    DataQueue* queue_;
};

class FromDefault
{
public:
    typedef DataQueue::value_type value_type;
    
    explicit FromDefault(const value_type& value)
    : defaultValue(value)
    {
    }
    
    value_type getNextValue()
    {
        return defaultValue;
    }
    
private:
    value_type  defaultValue;    
};

template<typename DataSource>
class BitSetValue : public FieldValue<DataSource>
{
public:
    typedef typename FieldValue<DataSource>::value_type value_type;

    BitSetValue(DataSource const& source, std::size_t length, std::size_t numBits)
    :FieldValue<DataSource>(source),
    bitset_(length),
    bits_(numBits)
    {
    }
    
    std::size_t getBitsetLength() const { return bitset_;}
    std::size_t getBits() const { return bits_;}

  private:
    std::size_t bitset_;
    std::size_t bits_;
};

template<typename ValueType>
class MultiFieldValue
{
public:
    typedef ValueType value_type;
    
    MultiFieldValue(size_t repeatCount)
    :repeat(repeatCount)
    {
    }
    
    void addField(FieldValueBase* fv);
    
    void update();
    value_type getValueByIndex(size_t idx);
 
 private:
    size_t repeat;
};

typedef FieldValue<FromDefault> FieldValueDefault;
typedef FieldValue<FromQueue> FieldValueFromInput;
typedef BitSetValue<FromDefault> BitsetFieldValueDefault;
typedef BitSetValue<FromQueue> BitsetFieldValueFromInput;

void testDefault()
{
    DataQueue::value_type value;
    FromDefault defaultSource(value);
    FieldValueDefault   fromDefault(defaultSource);
    BitsetFieldValueDefault bitsetFromDefault(defaultSource, 8, 3);
    std::size_t bits = bitsetFromDefault.getBits();
    std::size_t length = bitsetFromDefault.getBitsetLength();
}

void testFromInput()
{
    DataQueue       dataQueue;
    FromQueue  queueSource(&dataQueue);
    FieldValueFromInput fromInput(queueSource);
    BitsetFieldValueFromInput bitsetFromInput(queueSource, 8, 3);
    std::size_t bits = bitsetFromInput.getBits();
    std::size_t length = bitsetFromInput.getBitsetLength();
}

#endif
