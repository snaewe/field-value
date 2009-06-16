// -*- c++ -*-
#ifndef DATAQUEUE_H
#define DATAQUEUE_H

#include <utility>
#include <vector>
#include <boost/variant.hpp>
#include <boost/mpl/vector.hpp>

namespace mpl = boost::mpl;

/*
 * <long, double, string> are the BaseTypes
 * a pair of int and a variant over BaseTypes is BaseItemID
 * a vector of BaseItemID is BaseItemIDList
 * a variant over BaseTypes+BaseItemID+BaseItemIDList is QueueItem
 */
typedef boost::mpl::vector<long, double, std::string> BaseTypes;
typedef boost::make_variant_over<BaseTypes>::type BaseItem;
typedef std::pair<int, BaseItem> BaseItemID;
typedef std::vector<BaseItemID> BaseItemIDList;
typedef boost::make_variant_over<
  mpl::push_back<
    mpl::push_back<BaseTypes, BaseItemID>::type, 
    BaseItemIDList>::type>::type QueueItem;

class DataQueue
{
public:
    typedef QueueItem value_type;

    DataQueue()
    :value()
    {
    }
    
    DataQueue(value_type initialValue)
    :value(initialValue)
    {
    }
    
    bool getAnyValue(value_type& val)
    {
        val = value;
        return true;
    }

    void push(value_type const& val)
    {
      value = val;
    }

private:
    value_type value;
};
#endif
