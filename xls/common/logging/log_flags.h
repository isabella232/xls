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

#ifndef THIRD_PARTY_XLS_COMMON_LOGGING_LOG_FLAGS_H_
#define THIRD_PARTY_XLS_COMMON_LOGGING_LOG_FLAGS_H_

#include "absl/base/log_severity.h"
#include "absl/flags/flag.h"

ABSL_DECLARE_FLAG(int, minloglevel);
ABSL_DECLARE_FLAG(bool, logtostderr);
ABSL_DECLARE_FLAG(bool, alsologtostderr);
ABSL_DECLARE_FLAG(int, stderrthreshold);
ABSL_DECLARE_FLAG(bool, log_prefix);

#endif  // THIRD_PARTY_XLS_COMMON_LOGGING_LOG_FLAGS_H_
