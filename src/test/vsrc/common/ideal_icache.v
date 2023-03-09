import "DPI-C" function void readIdealIcache
(
  input longint gtimer,
  input bit valid,
  input bit port,
  input longint paddr,
  output bit hitInIdealICache,
  output longint hitData_0,
  output longint hitData_1,
  output longint hitData_2,
  output longint hitData_3,
  output longint hitData_4,
  output longint hitData_5,
  output longint hitData_6,
  output longint hitData_7
);

module read_ideal_icache(
  input clock,
  input [63:0] gtimer,
  input valid,
  input port,
  input [63:0] paddr,
  output hitInIdealICache,
  output [511:0] hitData
);

  wire w_hitInIdealICache;
  wire [511:0] w_hitData;
  
  assign hitInIdealICache = w_hitInIdealICache;
  assign hitData = w_hitData;
  /* verilator lint_off UNOPTFLAT */
  always @(*) begin
    readIdealIcache(
    gtimer,
    valid,
    port,
    paddr,
    w_hitInIdealICache,
    w_hitData[63:0],
    w_hitData[127:64],
    w_hitData[191:128],
    w_hitData[255:192],
    w_hitData[319:256],
    w_hitData[383:320],
    w_hitData[447:384],
    w_hitData[511:448]
    );
  end

endmodule

import "DPI-C" function void refillIdealIcache
(
  input longint gtimer,
  input bit valid,
  input longint paddr,
  input longint data_0,
  input longint data_1,
  input longint data_2,
  input longint data_3,
  input longint data_4,
  input longint data_5,
  input longint data_6,  
  input longint data_7
);

module refill_ideal_icache
(
  input clock,
  input [63:0] gtimer,
  input valid,
  input [63:0] paddr,
  input [511:0] data
);

  always @(posedge clock) begin
    refillIdealIcache (
      gtimer,
      valid,
      paddr,
      data[63:0],
      data[127:64],
      data[191:128],
      data[255:192],
      data[319:256],
      data[383:320],
      data[447:384],
      data[511:448]
    );
  end


endmodule