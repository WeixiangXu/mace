// Copyright 2018 Xiaomi, Inc.  All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "mace/core/operator.h"
#include "mace/core/testing/test_benchmark.h"
#include "mace/ops/ops_test_util.h"

namespace mace {
namespace ops {
namespace test {

namespace {
template <DeviceType D, typename T>
void BMBatchToSpace(
    int iters, int batch, int channels, int height, int width, int arg) {
  mace::testing::StopTiming();

  OpsTestNet net;
  if (D == DeviceType::CPU) {
    net.AddRandomInput<D, float>("Input", {batch, channels, height, width});
  } else if (D == DeviceType::GPU) {
    net.AddRandomInput<D, float>("Input", {batch, height, width, channels});
  }

  if (D == DeviceType::CPU) {
    OpDefBuilder("BatchToSpaceND", "BatchToSpaceNDTest")
        .Input("Input")
        .Output("Output")
        .AddIntsArg("crops", {0, 0, 0, 0})
        .AddIntsArg("block_shape", {arg, arg})
        .Finalize(net.NewOperatorDef());
  } else if (D == DeviceType::GPU) {
    BufferToImage<D, float>(&net, "Input", "InputImage",
                            kernels::BufferType::IN_OUT_CHANNEL);
    OpDefBuilder("BatchToSpaceND", "BatchToSpaceNDTest")
        .Input("InputImage")
        .Output("OutputImage")
        .AddIntsArg("crops", {0, 0, 0, 0})
        .AddIntsArg("block_shape", {arg, arg})
        .Finalize(net.NewOperatorDef());
  }
  // Warm-up
  for (int i = 0; i < 5; ++i) {
    net.RunOp(D);
  }
  net.Sync();

  mace::testing::StartTiming();
  while (iters--) {
    net.RunOp(D);
  }
  net.Sync();
}
}  // namespace

#define BM_BATCH_TO_SPACE_MACRO(N, H, W, C, ARG, TYPE, DEVICE)             \
  static void                                                              \
      BM_BATCH_TO_SPACE_##N##_##H##_##W##_##C##_##ARG##_##TYPE##_##DEVICE( \
          int iters) {                                                     \
    const int64_t tot = static_cast<int64_t>(iters) * N * C * H * W;       \
    mace::testing::MaccProcessed(tot);                                     \
    mace::testing::BytesProcessed(tot *(sizeof(TYPE)));                    \
    BMBatchToSpace<DEVICE, TYPE>(iters, N, C, H, W, ARG);                  \
  }                                                                        \
  BENCHMARK(BM_BATCH_TO_SPACE_##N##_##H##_##W##_##C##_##ARG##_##TYPE##_##DEVICE)

#define BM_BATCH_TO_SPACE(N, H, W, C, ARG) \
  BM_BATCH_TO_SPACE_MACRO(N, H, W, C, ARG, float, GPU); \
  BM_BATCH_TO_SPACE_MACRO(N, H, W, C, ARG, float, CPU);

BM_BATCH_TO_SPACE(128, 8, 8, 128, 2);
BM_BATCH_TO_SPACE(4, 128, 128, 32, 2);
BM_BATCH_TO_SPACE(16, 64, 64, 32, 4);
BM_BATCH_TO_SPACE(64, 32, 32, 32, 8);

}  // namespace test
}  // namespace ops
}  // namespace mace
