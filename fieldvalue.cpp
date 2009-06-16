#include "fieldvalue.h"
#include <gtest/gtest.h>

FieldValueBase::~FieldValueBase()
{
}

FieldValueBase::FieldValueBase()
{
}

int main(int argc, char* argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
