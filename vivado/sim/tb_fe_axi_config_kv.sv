// tb_fe_axi_config_kv.sv — fe_axi_config_kv 自校验仿真
// 用 iverilog 运行：
//   iverilog -g2012 -o tb.vvp sim/tb_fe_axi_config_kv.sv rtl/fe_axi_config_kv.sv
//   vvp tb.vvp
// 预期输出：ALL TESTS PASSED
`timescale 1ns / 1ps

module tb_fe_axi_config_kv;

    reg         aclk = 0;
    reg         aresetn = 0;

    reg  [11:0] awaddr = 0;
    reg  [2:0]  awprot = 0;
    reg         awvalid = 0;
    wire        awready;
    reg  [31:0] wdata = 0;
    reg  [3:0]  wstrb = 0;
    reg         wvalid = 0;
    wire        wready;
    wire [1:0]  bresp;
    wire        bvalid;
    reg         bready = 1;

    reg  [11:0] araddr = 0;
    reg  [2:0]  arprot = 0;
    reg         arvalid = 0;
    wire        arready;
    wire [31:0] rdata;
    wire [1:0]  rresp;
    wire        rvalid;
    reg         rready = 1;

    always #5 aclk = ~aclk;

    fe_axi_config_kv dut (
        .s_axi_aclk(aclk), .s_axi_aresetn(aresetn),
        .s_axi_awaddr(awaddr), .s_axi_awprot(awprot), .s_axi_awvalid(awvalid), .s_axi_awready(awready),
        .s_axi_wdata(wdata), .s_axi_wstrb(wstrb), .s_axi_wvalid(wvalid), .s_axi_wready(wready),
        .s_axi_bresp(bresp), .s_axi_bvalid(bvalid), .s_axi_bready(bready),
        .s_axi_araddr(araddr), .s_axi_arprot(arprot), .s_axi_arvalid(arvalid), .s_axi_arready(arready),
        .s_axi_rdata(rdata), .s_axi_rresp(rresp), .s_axi_rvalid(rvalid), .s_axi_rready(rready)
    );

    integer errors = 0;

    task axi_write(input [11:0] addr, input [31:0] data);
        begin
            @(posedge aclk);
            awaddr <= addr; awvalid <= 1;
            wdata  <= data; wstrb  <= 4'hF; wvalid <= 1;
            while (!(awready && wready)) @(posedge aclk);
            @(posedge aclk);
            awvalid <= 0; wvalid <= 0;
            while (!bvalid) @(posedge aclk);
            @(posedge aclk);
        end
    endtask

    // Exercise legal AXI4-Lite behavior where address and data arrive in
    // different cycles.  A slave must not require both VALID signals together.
    task axi_write_split(input [11:0] addr, input [31:0] data, input integer address_first);
        begin
            if (address_first) begin
                @(posedge aclk);
                awaddr <= addr; awvalid <= 1;
                while (!awready) @(posedge aclk);
                @(posedge aclk); awvalid <= 0;
                repeat (2) @(posedge aclk);
                wdata <= data; wstrb <= 4'hF; wvalid <= 1;
                while (!wready) @(posedge aclk);
                @(posedge aclk); wvalid <= 0;
            end else begin
                @(posedge aclk);
                wdata <= data; wstrb <= 4'hF; wvalid <= 1;
                while (!wready) @(posedge aclk);
                @(posedge aclk); wvalid <= 0;
                repeat (2) @(posedge aclk);
                awaddr <= addr; awvalid <= 1;
                while (!awready) @(posedge aclk);
                @(posedge aclk); awvalid <= 0;
            end
            while (!bvalid) @(posedge aclk);
            @(posedge aclk);
        end
    endtask

    task axi_read(input [11:0] addr, output [31:0] data);
        begin
            @(posedge aclk);
            araddr <= addr; arvalid <= 1;
            while (!arready) @(posedge aclk);
            @(posedge aclk);
            arvalid <= 0;
            while (!rvalid) @(posedge aclk);
            data = rdata;
            @(posedge aclk);
        end
    endtask

    reg [31:0] rd;
    integer n;

    initial begin
        // FNV-1a("fe_cfg#wifi.ssid") 参考值由 Python 生成：
        //   h=2166136261; for c in "fe_cfg": h=(h^ord(c))*16777619&0xffffffff
        //                  h=(h^35)*16777619&0xffffffff
        //                  for c in "wifi.ssid": h=(h^ord(c))*16777619&0xffffffff
        localparam [31:0] HASH_WIFI = 32'hE0CD180C;
        repeat (4) @(posedge aclk);
        aresetn = 1;
        repeat (4) @(posedge aclk);

        // 1) 上电后全空
        axi_read(12'h000, rd);
        if (rd !== 32'h0) begin $display("FAIL: slot0.hash reset = %h", rd); errors = errors + 1; end

        // 2) 写槽位 0：hash + len(9) + "MyNet" 小端打包
        axi_write(12'h000, HASH_WIFI);
        axi_write(12'h004, 32'd5);
        axi_write(12'h008, 32'h654E_794D);  // "MyNe" 小端
        axi_write(12'h00C, 32'h0000_0074);  // 't'
        // 读回
        axi_read(12'h000, rd);
        if (rd !== HASH_WIFI) begin $display("FAIL: hash readback = %h", rd); errors = errors + 1; end
        axi_read(12'h004, rd);
        if (rd !== 32'd5)    begin $display("FAIL: len readback = %h", rd);  errors = errors + 1; end
        axi_read(12'h008, rd);
        if (rd !== 32'h654E_794D) begin $display("FAIL: val0 readback = %h", rd); errors = errors + 1; end
        axi_read(12'h00C, rd);
        if (rd !== 32'h0000_0074) begin $display("FAIL: val1 readback = %h", rd); errors = errors + 1; end

        // 3) 槽位 1 起始地址 = 0x40
        axi_write(12'h040, 32'h12345678);
        axi_read(12'h040, rd);
        if (rd !== 32'h12345678) begin $display("FAIL: slot1 write/read"); errors = errors + 1; end

        // 4) AW/W 分周期到达，地址先到与数据先到都必须成功
        axi_write_split(12'h080, 32'hA5A55A5A, 1);
        axi_read(12'h080, rd);
        if (rd !== 32'hA5A55A5A) begin $display("FAIL: split AW-first write/read"); errors = errors + 1; end
        axi_write_split(12'h084, 32'h55AA33CC, 0);
        axi_read(12'h084, rd);
        if (rd !== 32'h55AA33CC) begin $display("FAIL: split W-first write/read"); errors = errors + 1; end

        // 5) 越界地址（>=1KB）读应为 0
        axi_read(12'h400, rd);
        if (rd !== 32'h0) begin $display("FAIL: oob read = %h", rd); errors = errors + 1; end

        // 6) 清除槽位 0（写 0 模拟 delete）
        axi_write(12'h000, 32'h0);
        axi_read(12'h000, rd);
        if (rd !== 32'h0) begin $display("FAIL: slot0 clear"); errors = errors + 1; end

        if (errors == 0) $display("ALL TESTS PASSED");
        else $display("%0d ERRORS", errors);
        $finish;
    end

    // 仿真超时保护
    initial begin
        #100000;
        $display("FAIL: simulation timeout");
        $finish;
    end
endmodule
