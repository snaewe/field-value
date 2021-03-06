#ifndef FIELDVALUE_H
#define FIELDVALUE_H

#include <cstdlib>
#include <iostream>
#include <ostream>
#include <fstream>
#include <vector>
#include <numeric>

#include <boost/shared_ptr.hpp>
#include <boost/foreach.hpp>
#include <boost/dynamic_bitset.hpp>

#include <gtest/gtest.h>

#include "dataqueue.h"

template<typename ValueType>
class FieldValueBase
{
public:
    typedef ValueType value_type;

    virtual ~FieldValueBase()  {}
    
    virtual void update() = 0;
    virtual value_type getValue() const { return value_type(); }
    virtual void serializeTo(std::ostream& output) = 0;
    virtual void serializeNthValueTo(size_t index, std::ostream& output) = 0;
    
protected:
  FieldValueBase() {}
};

template<typename DataSource>
class FieldValue : public FieldValueBase<typename DataSource::value_type>
{
public:
    typedef typename DataSource::value_type value_type;
    
    void update() {  dataValue = dataSource.getNextValue(); }
    value_type getValue() const { return dataValue; }

    explicit FieldValue(DataSource& source)
    :dataSource(source)
    {
    }
    
    void serializeTo(std::ostream& output)
    {
      output << boost::get<long>(dataValue);
    }
    
    void serializeNthValueTo(size_t index, std::ostream& output)
    {
        output << '(' << index << ',' << boost::get<long>(dataValue) << ')';
    }
    
private:
    value_type  dataValue;
    DataSource&  dataSource;
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
    
    FromDefault()
    : defaultValue()
    {
    }
    explicit FromDefault(const value_type& value)
    : defaultValue(value)
    {
    }
    
    void setValue(const value_type& value)
    {
      defaultValue = value;
    }

    value_type getNextValue()
    {
        return defaultValue;
    }
    
private:
    value_type  defaultValue;    
};

template<typename ValueType>
class BitSetValue : public FieldValueBase<ValueType>
{
public:
  typedef boost::shared_ptr<FieldValueBase<ValueType> > FieldValueBase_ptr;
  typedef ValueType value_type;

  struct IllegalSize : public std::exception {};

private:
  typedef boost::dynamic_bitset<unsigned char> DynamicBitset;
  typedef std::pair<size_t, FieldValueBase_ptr> SizeAndField;

public:
  BitSetValue(std::size_t numBytes)
    :byteCount(numBytes)
  {
  }

  std::size_t getNumBytes() const { return byteCount;}

  void addBits(size_t numBits, FieldValueBase<ValueType>* fv)
  {
    addBits(numBits, FieldValueBase_ptr(fv));
  }

  void addBits(size_t numBits, FieldValueBase_ptr fv)
  {
    bits.push_back(std::make_pair(numBits, fv));
    checkSize();
  }

  void update()
  {
    BOOST_FOREACH(SizeAndField saf, bits)
    {
      saf.second->update();
    }
  }

  void serializeTo(std::ostream& output)
  {
    DynamicBitset dynBuf;

    BOOST_FOREACH(SizeAndField const& saf, bits)
    {
      boost::dynamic_bitset<> val(saf.first, boost::get<long>(saf.second->getValue()));
      for(size_t i=0; i<val.size(); ++i)
      {
        dynBuf.push_back(val[i]);
      }
    }
    output << dynBuf;
  }

  void serializeNthValueTo(size_t,std::ostream &)
  {
  }

private:
  struct GetBitSize
  {
    size_t operator()(size_t size, const SizeAndField& saf) const
    {
      return size+saf.first;
    }
  };

  void checkSize() const
  {
    size_t bitCount = 0;
    bitCount = std::accumulate(bits.begin(), bits.end(), bitCount, GetBitSize());
    if(bitCount > (getNumBytes()*8))
      throw IllegalSize();
  }

  std::size_t byteCount;
  std::vector<SizeAndField> bits;
};

template<typename ValueType>
class MultiFieldValue : public FieldValueBase<ValueType>
{
public:
    typedef boost::shared_ptr<FieldValueBase<ValueType> > FieldValueBase_ptr;
    typedef ValueType value_type;
    
    MultiFieldValue(size_t repeatCount)
    :repeat(repeatCount)
    {
    }

    size_t repeatCount() const { return repeat; }
    
    void addField(FieldValueBase<value_type>* fv)
    {
      addField(FieldValueBase_ptr(fv));
    }

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

const ::testing::TestInfo* const get_test_info()
{
  return ::testing::UnitTest::GetInstance()->current_test_info();
}

TEST(FieldValueTest, Default)
{
    DataQueue::value_type value;
    FromDefault defaultSource;
    defaultSource.setValue(value);
    FieldValueDefault   fromDefault(defaultSource);
}

TEST(FieldValueTest, FromInput)
{
    DataQueue input;
    FromQueue fromQueue(&input);
    FieldValueFromInput valueFromInput(fromQueue);
}

TEST(FieldValueTest, BitsetFieldTest)
{
  BitSetValue<DataQueue::value_type> bitsetValue(4);
    std::size_t length = bitsetValue.getNumBytes();
    ASSERT_EQ(length, 4);

    DataQueue::value_type value;
    FromDefault defaultSource;
    defaultSource.setValue(value);
    
    EXPECT_NO_THROW(
      bitsetValue.addBits(3, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(1, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(4, new FieldValueDefault(defaultSource));

      bitsetValue.addBits(3, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(1, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(4, new FieldValueDefault(defaultSource));

      bitsetValue.addBits(3, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(1, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(4, new FieldValueDefault(defaultSource));

      bitsetValue.addBits(3, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(1, new FieldValueDefault(defaultSource));
      bitsetValue.addBits(4, new FieldValueDefault(defaultSource));
    );

    EXPECT_THROW(
      bitsetValue.addBits(1, new FieldValueDefault(defaultSource)), 
      BitSetValue<DataQueue::value_type>::IllegalSize);
}

TEST(FieldValueTest, MultiFieldsNoRepeat)
{
    DataQueue       dataQueue;

    MultiFieldValue<DataQueue::value_type> single(1);
    ASSERT_EQ(1, single.repeatCount());
    single.addField(new FieldValueFromInput(FromQueue(&dataQueue)));

    single.update();

    const ::testing::TestInfo* const test_info =
    ::testing::UnitTest::GetInstance()->current_test_info();
    std::string ofname(test_info->name());
    ofname += ".out";
    std::ofstream output(ofname.c_str());
    output << "\n---- MultiField(repeat=1) ----\n";
    single.serializeTo(output);
    output << "\n----------------\n";
}

TEST(FieldValueTest, MultiFieldsYesRepeat)
{
    DataQueue       dataQueue[10] = { };
    for(size_t i=0; i<10; ++i)
    {
      //dataQueue[i].push(DataQueue::value_type(double(i+1.0)));
      dataQueue[i].push(DataQueue::value_type(long(i+1)));
    }
    const size_t numRepeat = 3;
    MultiFieldValue<DataQueue::value_type> multi(numRepeat);
    ASSERT_EQ(numRepeat, multi.repeatCount());

    for(int i=0; i<5; ++i)
        multi.addField(new FieldValueFromInput(FromQueue(&dataQueue[i+1])));
    multi.update();

    const ::testing::TestInfo* const test_info = get_test_info();
    std::string ofname(test_info->name());
    ofname += ".out";
    std::ofstream output(ofname.c_str());
    output << "\n---- MultiField(repeat=" << multi.repeatCount() <<") ----\n";
    multi.serializeTo(output);
    output << "\n---------------------\n";
}

#endif
