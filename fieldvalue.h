#ifndef FIELDVALUE_H
#define FIELDVALUE_H

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>

#include <gtest/gtest.h>

#include "dataqueue.h"

class FieldValueBase
{
public:
    virtual ~FieldValueBase();
    
    virtual void update() = 0;
    virtual void serializeTo(std::ostream& output) = 0;
    virtual void serializeNthValueTo(size_t index, std::ostream& output) = 0;
    
protected:
    FieldValueBase();
};

typedef boost::shared_ptr<FieldValueBase> FieldValueBase_ptr;

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
    
    void serializeNthValueTo(size_t index, std::ostream& output)
    {
        output << '(' << index << ',' << dataValue << ')';
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
class MultiFieldValue : public FieldValueBase
{
public:
    typedef ValueType value_type;
    
    MultiFieldValue(size_t repeatCount)
    :repeat(repeatCount)
    {
    }

    size_t repeatCount() const { return repeat; }
    
    void addField(FieldValueBase_ptr fv)
    {
        fields.push_back(fv);
    }
    
    void update()
    {
        BOOST_FOREACH(FieldValueBase_ptr fv, fields)
        {
            fv->update();
        }
    }

    void serializeTo(std::ostream& output)
    {
        for(size_t i=0; i<repeat; ++i)
        {
            serializeNthValueTo(i, output);
        }
    }
    
    void serializeNthValueTo(size_t index, std::ostream& output)
    {
        BOOST_FOREACH(FieldValueBase_ptr fv, fields)
        {
            fv->serializeNthValueTo(index, output);
        }
    }

    private:
    size_t repeat;
    std::vector<FieldValueBase_ptr> fields;
};

typedef FieldValue<FromDefault> FieldValueDefault;
typedef FieldValue<FromQueue> FieldValueFromInput;
typedef BitSetValue<FromDefault> BitsetFieldValueDefault;
typedef BitSetValue<FromQueue> BitsetFieldValueFromInput;

TEST(FieldValueTest, Default)
{
    DataQueue::value_type value;
    FromDefault defaultSource(value);
    FieldValueDefault   fromDefault(defaultSource);
    BitsetFieldValueDefault bitsetFromDefault(defaultSource, 8, 3);
    std::size_t bits = bitsetFromDefault.getBits();
    ASSERT_EQ(bits, 3);
    std::size_t length = bitsetFromDefault.getBitsetLength();
    ASSERT_EQ(length, 8);
}

TEST(FieldValueTest, FromInput)
{
    DataQueue       dataQueue;
    FromQueue  queueSource(&dataQueue);
    FieldValueFromInput fromInput(queueSource);
    BitsetFieldValueFromInput bitsetFromInput(queueSource, 8, 3);
    std::size_t bits = bitsetFromInput.getBits();
    std::size_t length = bitsetFromInput.getBitsetLength();
}

TEST(FieldValueTest, MultiFields)
{
    DataQueue       dataQueue[10] = { 1,2,3,4,5,6,7,8,9,10 };

    MultiFieldValue<DataQueue::value_type> single(1);
    single.addField(FieldValueBase_ptr(new FieldValueFromInput(FromQueue(&dataQueue[0]))));

    single.update();

    std::cout << "\n---- MultiField(repeat=1) ----\n";
    single.serializeTo(std::cout);
    std::cout << "\n----------------\n";

    const size_t numFields = 3;
    MultiFieldValue<DataQueue::value_type> threeFields(numFields);
    for(int i=0; i<5; ++i)
        threeFields.addField(FieldValueBase_ptr(new FieldValueFromInput(FromQueue(&dataQueue[i+1]))));
    threeFields.update();
    std::cout << "\n---- MultiField(repeat=" << threeFields.repeatCount() <<") ----\n";
    threeFields.serializeTo(std::cout);
    std::cout << "\n---------------------\n";
}

#endif
