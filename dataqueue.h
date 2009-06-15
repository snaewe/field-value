// -*- c++ -*-
#ifndef DATAQUEUE_H
#define DATAQUEUE_H

class DataQueue
{
public:
    typedef long value_type;

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
private:
    value_type value;
};
#endif
