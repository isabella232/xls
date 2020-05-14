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

#include "xls/passes/bdd_query_engine.h"

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "xls/common/status/matchers.h"
#include "xls/ir/bits.h"
#include "xls/ir/function.h"
#include "xls/ir/function_builder.h"
#include "xls/ir/ir_test_base.h"
#include "xls/ir/package.h"

namespace xls {
namespace {

class BddQueryEngineTest : public IrTestBase {
 protected:
  // Convenience methods for testing implication, equality, and inverse for
  // single-bit node values.
  bool Implies(const QueryEngine& engine, Node* a, Node* b) {
    return engine.Implies(BitLocation(a, 0), BitLocation(b, 0));
  }
  bool KnownEquals(const QueryEngine& engine, Node* a, Node* b) {
    return engine.KnownEquals(BitLocation(a, 0), BitLocation(b, 0));
  }
  bool KnownNotEquals(const QueryEngine& engine, Node* a, Node* b) {
    return engine.KnownNotEquals(BitLocation(a, 0), BitLocation(b, 0));
  }
};

TEST_F(BddQueryEngineTest, EqualToPredicates) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  BValue x = fb.Param("x", p->GetBitsType(8));
  BValue y = fb.Param("y", p->GetBitsType(8));
  BValue x_eq_0 = fb.Eq(x, fb.Literal(UBits(0, 8)));
  BValue x_eq_0_2 = fb.Eq(x, fb.Literal(UBits(0, 8)));
  BValue x_ne_0 = fb.Not(x_eq_0);
  BValue x_eq_42 = fb.Eq(x, fb.Literal(UBits(7, 8)));
  BValue y_eq_42 = fb.Eq(y, fb.Literal(UBits(7, 8)));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  XLS_ASSERT_OK_AND_ASSIGN(auto query_engine, BddQueryEngine::Run(f));

  EXPECT_TRUE(query_engine->AtMostOneNodeTrue({}));
  EXPECT_FALSE(query_engine->AtMostOneBitTrue(x.node()));
  EXPECT_TRUE(query_engine->AtMostOneNodeTrue({x_eq_0.node(), x_eq_42.node()}));
  EXPECT_TRUE(query_engine->AtLeastOneNodeTrue({x_eq_0.node(), x_ne_0.node()}));

  EXPECT_TRUE(KnownEquals(*query_engine, x_eq_0.node(), x_eq_0.node()));
  EXPECT_TRUE(KnownEquals(*query_engine, x_eq_0.node(), x_eq_0_2.node()));
  EXPECT_FALSE(KnownNotEquals(*query_engine, x_eq_0.node(), x_eq_0_2.node()));
  EXPECT_TRUE(KnownNotEquals(*query_engine, x_eq_0.node(), x_ne_0.node()));

  EXPECT_TRUE(Implies(*query_engine, x_eq_0.node(), x_eq_0.node()));
  EXPECT_TRUE(Implies(*query_engine, x_eq_0.node(), x_eq_0_2.node()));
  EXPECT_FALSE(Implies(*query_engine, x_eq_0.node(), x_eq_42.node()));

  // Unrelated values 'x' and 'y' should have no relationships.
  EXPECT_FALSE(Implies(*query_engine, x_eq_42.node(), y_eq_42.node()));
  EXPECT_FALSE(KnownEquals(*query_engine, x_eq_42.node(), y_eq_42.node()));
  EXPECT_FALSE(KnownNotEquals(*query_engine, x_eq_42.node(), y_eq_42.node()));
  EXPECT_FALSE(
      query_engine->AtMostOneNodeTrue({x_eq_42.node(), y_eq_42.node()}));
  EXPECT_FALSE(
      query_engine->AtLeastOneNodeTrue({x_eq_42.node(), y_eq_42.node()}));
}

TEST_F(BddQueryEngineTest, VariousComparisonPredicates) {
  auto p = CreatePackage();
  FunctionBuilder fb(TestName(), p.get());
  BValue x = fb.Param("x", p->GetBitsType(32));
  BValue x_eq_42 = fb.Eq(x, fb.Literal(UBits(42, 32)));
  BValue x_lt_42 = fb.ULt(x, fb.Literal(UBits(42, 32)));
  BValue x_ge_20 = fb.UGe(x, fb.Literal(UBits(20, 32)));
  BValue x_lt_20 = fb.ULt(x, fb.Literal(UBits(20, 32)));
  BValue x_eq_7 = fb.Eq(x, fb.Literal(UBits(7, 32)));
  BValue x_eq_999 = fb.Eq(x, fb.Literal(UBits(999, 32)));
  XLS_ASSERT_OK_AND_ASSIGN(Function * f, fb.Build());
  XLS_ASSERT_OK_AND_ASSIGN(auto query_engine, BddQueryEngine::Run(f));

  EXPECT_TRUE(query_engine->AtMostOneNodeTrue(
      {x_eq_42.node(), x_eq_7.node(), x_eq_999.node()}));
  EXPECT_FALSE(
      query_engine->AtMostOneNodeTrue({x_lt_42.node(), x_ge_20.node()}));

  EXPECT_TRUE(
      query_engine->AtLeastOneNodeTrue({x_lt_42.node(), x_ge_20.node()}));
  EXPECT_TRUE(
      query_engine->AtLeastOneNodeTrue({x_ge_20.node(), x_lt_20.node()}));

  EXPECT_TRUE(Implies(*query_engine, x_eq_7.node(), x_lt_42.node()));
  EXPECT_FALSE(Implies(*query_engine, x_lt_42.node(), x_eq_7.node()));
}

}  // namespace
}  // namespace xls
