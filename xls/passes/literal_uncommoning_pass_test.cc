// Copyright 2020 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "xls/passes/literal_uncommoning_pass.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"
#include "xls/common/status/statusor.h"
#include "xls/ir/function.h"
#include "xls/ir/ir_test_base.h"
#include "xls/ir/package.h"

namespace xls {
namespace {

using status_testing::IsOkAndHolds;

class LiteralUncommoningPassTest : public IrTestBase {
 protected:
  LiteralUncommoningPassTest() = default;

  xabsl::StatusOr<bool> Run(Package* p) {
    PassResults results;
    return LiteralUncommoningPass().Run(p, PassOptions(), &results);
  }
};

TEST_F(LiteralUncommoningPassTest, SingleLiteralNoChange) {
  auto p = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, ParseFunction(R"(
     fn single_literal() -> bits[2] {
        ret one: bits[2] = literal(value=1)
     }
  )",
                                                       p.get()));
  EXPECT_EQ(f->node_count(), 1);
  EXPECT_THAT(Run(p.get()), IsOkAndHolds(false));
  EXPECT_EQ(f->node_count(), 1);
}

TEST_F(LiteralUncommoningPassTest, TwoLiteralsNoChange) {
  auto p = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, ParseFunction(R"(
     fn single_literal() -> bits[42] {
        one: bits[42] = literal(value=1)
        two: bits[42] = literal(value=2)
        ret and: bits[42] = and(one, two)
     }
  )",
                                                       p.get()));
  EXPECT_EQ(f->node_count(), 3);
  EXPECT_THAT(Run(p.get()), IsOkAndHolds(false));
  EXPECT_EQ(f->node_count(), 3);
}

TEST_F(LiteralUncommoningPassTest, LiteralHasMultipleUses) {
  auto p = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, ParseFunction(R"(
     fn single_literal() -> bits[42] {
        literal.1: bits[42] = literal(value=123)
        neg.2: bits[42] = neg(literal.1)
        not.3: bits[42] = not(literal.1)
        ret not.4: bits[42] = not(literal.1)
     }
  )",
                                                       p.get()));
  EXPECT_EQ(f->node_count(), 4);
  EXPECT_THAT(Run(p.get()), IsOkAndHolds(true));
  EXPECT_EQ(f->node_count(), 6);
}

TEST_F(LiteralUncommoningPassTest, LiteralDuplicatedInOperands) {
  auto p = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, ParseFunction(R"(
     fn single_literal() -> bits[42] {
        literal.1: bits[42] = literal(value=123)
        ret and: bits[42] = and(literal.1, literal.1)
     }
  )",
                                                       p.get()));
  EXPECT_EQ(f->node_count(), 2);
  EXPECT_EQ(f->return_value()->operand(0), f->return_value()->operand(1));

  EXPECT_THAT(Run(p.get()), IsOkAndHolds(true));

  EXPECT_EQ(f->node_count(), 3);
  ASSERT_TRUE(f->return_value()->operand(0)->Is<Literal>());
  ASSERT_TRUE(f->return_value()->operand(1)->Is<Literal>());
  EXPECT_NE(f->return_value()->operand(0), f->return_value()->operand(1));
  EXPECT_EQ(f->return_value()->operand(0)->As<Literal>()->value().bits(),
            UBits(123, 42));
  EXPECT_EQ(f->return_value()->operand(1)->As<Literal>()->value().bits(),
            UBits(123, 42));
}

TEST_F(LiteralUncommoningPassTest, DoNotUncommonArrays) {
  auto p = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, ParseFunction(R"(
     fn single_literal(x: bits[32], y: bits[32]) -> bits[32] {
        literal.1: bits[32][4] = literal(value=[1, 2, 3, 4])
        array_index.2: bits[32] = array_index(literal.1, x)
        array_index.3: bits[32] = array_index(literal.1, y)
        ret add.4: bits[32] = add(array_index.2, array_index.3)
     }
  )",
                                                       p.get()));
  EXPECT_EQ(f->node_count(), 6);
  EXPECT_THAT(Run(p.get()), IsOkAndHolds(false));
  EXPECT_EQ(f->node_count(), 6);
}

TEST_F(LiteralUncommoningPassTest, DoNotUncommonTuples) {
  auto p = CreatePackage();
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, ParseFunction(R"(
     fn single_literal() -> bits[32] {
        literal.1: (bits[32], bits[32]) = literal(value=(42, 2))
        tuple_index.2: bits[32] = tuple_index(literal.1, index=0)
        tuple_index.3: bits[32] = tuple_index(literal.1, index=1)
        ret add.4: bits[32] = add(tuple_index.2, tuple_index.3)
     }
  )",
                                                       p.get()));
  EXPECT_EQ(f->node_count(), 4);
  EXPECT_THAT(Run(p.get()), IsOkAndHolds(false));
  EXPECT_EQ(f->node_count(), 4);
}

}  // namespace
}  // namespace xls
