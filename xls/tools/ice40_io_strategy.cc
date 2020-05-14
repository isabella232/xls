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

#include "xls/tools/ice40_io_strategy.h"

#include <filesystem>

#include "xls/common/file/filesystem.h"
#include "xls/common/file/get_runfile_path.h"
#include "xls/common/status/status_macros.h"

namespace xls {
namespace verilog {

Ice40IoStrategy::Ice40IoStrategy(VerilogFile* f) : f_(f) {
  for (const char* include : kIncludes) {
    f_->Add(f_->Make<Include>(include));
  }
}

absl::Status Ice40IoStrategy::AddTopLevelDependencies(LogicRef1* clk,
                                                      Reset reset, Module* m) {
  VerilogFile* f = m->parent();
  clk_ = clk;
  rx_in_ = m->AddInput("rx_in");
  tx_out_ = m->AddOutput("tx_out");
  LogicRef1* clear_to_send_out_n = m->AddOutput("clear_to_send_out_n");
  clear_to_send_ = m->AddWire1("clear_to_send");
  m->Add<ContinuousAssignment>(clear_to_send_out_n,
                               f->LogicalNot(clear_to_send_));

  // The UARTs use a synchronous active-low reset.
  if (!reset.active_low || reset.asynchronous) {
    return absl::UnimplementedError(
        "ICE40 IO strategy expects a synchronous active-low reset signal.");
  }

  rst_n_ = reset.signal;

  clocks_per_baud_ = m->AddParameter(
      "ClocksPerBaud", f_->Make<MacroRef>("DEFAULT_CLOCKS_PER_BAUD"));

  return absl::OkStatus();
}

absl::Status Ice40IoStrategy::InstantiateIoBlocks(Input input, Output output,
                                                  Module* m) {
  // Instantiate the UART receiver.
  m->Add<Instantiation>(
      "uart_receiver", "rx",
      /*parameters=*/
      std::vector<Connection>{{"ClocksPerBaud", clocks_per_baud_}},
      std::vector<Connection>{{"clk", clk_},
                              {"rst_n", rst_n_},
                              {"rx", rx_in_},
                              {"rx_byte_out", input.rx_byte},
                              {"rx_byte_valid_out", input.rx_byte_valid},
                              {"rx_byte_done", input.rx_byte_done},
                              {"clear_to_send_out", clear_to_send_}});

  // Instantiate the UART transmitter.
  m->Add<Instantiation>(
      "uart_transmitter", "tx",
      /*parameters=*/
      std::vector<Connection>{{"ClocksPerBaud", clocks_per_baud_}},
      std::vector<Connection>{
          {"clk", clk_},
          {"rst_n", rst_n_},
          {"tx_byte", output.tx_byte},
          {"tx_byte_valid", output.tx_byte_valid},
          {"tx_byte_done_out", output.tx_byte_ready},
          {"tx_out", tx_out_},
      });

  return absl::OkStatus();
}

xabsl::StatusOr<std::vector<VerilogInclude>> Ice40IoStrategy::GetIncludes() {
  std::vector<VerilogInclude> includes;
  for (const char* rel_path : kIncludes) {
    VerilogInclude include;
    include.relative_path = rel_path;
    XLS_ASSIGN_OR_RETURN(
        include.verilog_text,
        xls::GetFileContents(xls::GetXlsRunfilePath(rel_path)));
    includes.push_back(std::move(include));
  }
  return includes;
}

}  // namespace verilog
}  // namespace xls
