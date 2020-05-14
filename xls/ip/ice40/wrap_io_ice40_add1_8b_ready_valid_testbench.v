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

`define DEFAULT_CLOCKS_PER_BAUD 8
`include "xls/ip/ice40/wrap_io_ice40_add1_8b_ready_valid.v"

module tb;
  integer i;
  reg clk = 0;
  reg rx_in = 1;
  wire tx_out;
  wire clear_to_send_out_n;

  `include "xls/ip/ice40/xls_assertions.inc"

  localparam ClocksPerBaud = `DEFAULT_CLOCKS_PER_BAUD;
  localparam TicksPerClock = 2;

  io_wrapper #(
    .ClocksPerBaud(ClocksPerBaud)
  ) dut(
    .clk(clk),
    .rx_in(rx_in),
    .tx_out(tx_out),
    .clear_to_send_out_n(clear_to_send_out_n)
  );

  initial begin
    #1 clk = 0;

    forever #1 clk = !clk;
  end

  // Make sure we finish after some reasonable amount of time.
  initial begin
    #1024 begin
      $display("ERROR: timeout, simulation ran too long");
      $finish;
    end
  end

  initial begin
    //$dumpfile("wrap_io_ice40_testbench.vcd");
    //$dumpvars(0, clk, rx_in, tx_out, clear_to_send_out_n, dut.state,
    //dut.state_next, dut.state_counter, dut.rx_byte, dut.rx_byte_valid,
    //dut.rx_byte_done, dut.tx_byte, dut.tx_byte_valid, dut.flat_input,
    //dut.flat_input_next, dut.flat_output);
    $display("Starting...\n");
    $monitor("%t: rx_byte: %b tx: %b cts_n: %b", $time, dut.rx_byte, tx_out, clear_to_send_out_n);

    #(TicksPerClock*ClocksPerBaud);

    rx_in <= 0;
    $display("Start bit; holding for %d", TicksPerClock*ClocksPerBaud);
    #(TicksPerClock*ClocksPerBaud);
    $display("Starting pattern.");
    for (i = 0; i < 8; i = i + 1) begin
      xls_assert(clear_to_send_out_n, 1, "transmitting from testbench");
      rx_in <= (i % 2 == 0);
      #(TicksPerClock*ClocksPerBaud);
    end
    rx_in <= 1;

    wait (!tx_out);
    #(TicksPerClock*ClocksPerBaud+1);  // start bit
    for (i = 0; i < 8; i = i + 1) begin
      xls_assert(((8'h56>>i)&1), tx_out, "receiving from testbench");
      #(TicksPerClock*ClocksPerBaud);
    end

    rx_in <= 0;
    $display("Start bit; holding for %d", TicksPerClock*ClocksPerBaud);
    #(TicksPerClock*ClocksPerBaud);
    $display("Starting pattern.");
    for (i = 0; i < 8; i = i + 1) begin
      xls_assert(clear_to_send_out_n, 1, "transmitting from testbench");
      rx_in <= ((i+1) % 2 == 0);
      #(TicksPerClock*ClocksPerBaud);
    end
    rx_in <= 1;

    #(TicksPerClock*ClocksPerBaud);
    xls_assert(clear_to_send_out_n, 0, "clear to send after transmit");
    $finish;
  end

endmodule
