// FasterEdge 开源项目 - Github: https://github.com/FasterEdge - Gitee: https://gitee.com/FasterEdge
// fe_axi_config_kv.sv — FasterEdge PL 侧 KV 配置存储（AXI4-Lite 从属）
// 16 个槽位，每槽 64B = 16 个 32 位字：
//   word 0     : key 哈希（0 = 空槽；哈希算法 FNV-1a，与 micro_blaze/User/fe_port.c 一致）
//   word 1     : value 字节长度
//   word 2..15 : value 内容（56B，小端打包）
// 主机（MicroBlaze / ARM PS）经 AXI4-Lite 直接读写，本模块为被动存储。
module fe_axi_config_kv #(
    parameter integer SLOTS = 16,                 // 槽位数
    parameter integer SLOT_WORDS = 16             // 每槽 32 位字数（64B）
)(
    input  wire        s_axi_aclk,
    input  wire        s_axi_aresetn,
    // 写地址通道
    input  wire [11:0] s_axi_awaddr,              // 4KB 空间内按字节寻址（实际用 1KB）
    input  wire [2:0]  s_axi_awprot,
    input  wire        s_axi_awvalid,
    output reg         s_axi_awready,
    // 写数据通道
    input  wire [31:0] s_axi_wdata,
    input  wire [3:0]  s_axi_wstrb,
    input  wire        s_axi_wvalid,
    output reg         s_axi_wready,
    // 写响应通道
    output reg  [1:0]  s_axi_bresp,
    output reg         s_axi_bvalid,
    input  wire        s_axi_bready,
    // 读地址通道
    input  wire [11:0] s_axi_araddr,
    input  wire [2:0]  s_axi_arprot,
    input  wire        s_axi_arvalid,
    output reg         s_axi_arready,
    // 读数据通道
    output reg  [31:0] s_axi_rdata,
    output reg  [1:0]  s_axi_rresp,
    output reg         s_axi_rvalid,
    input  wire        s_axi_rready
);

    localparam integer TOTAL_WORDS = SLOTS * SLOT_WORDS;   // 256 字 = 1KB
    localparam [1:0] OKAY = 2'b00;

    // ------------------------------------------------------------
    // 存储阵列（综合为 LUTRAM/BRAM）
    // ------------------------------------------------------------
    reg [31:0] mem [0:TOTAL_WORDS-1];
    integer i;
    initial for (i = 0; i < TOTAL_WORDS; i = i + 1) mem[i] = 32'h0;

    // ------------------------------------------------------------
    // 写事务。AXI4-Lite 的 AW 与 W 是独立通道，可能在不同周期到达；
    // 分别锁存后再提交，避免只接受“同周期 AWVALID+WVALID”的隐性死锁。
    // ------------------------------------------------------------
    reg  [11:0] awaddr_q;
    reg  [31:0] wdata_q;
    reg  [3:0]  wstrb_q;
    reg         aw_pending;
    reg         w_pending;

    wire aw_hs = s_axi_awvalid && s_axi_awready;
    wire w_hs  = s_axi_wvalid  && s_axi_wready;
    wire have_aw = aw_pending || aw_hs;
    wire have_w  = w_pending  || w_hs;
    wire [11:0] write_addr = aw_pending ? awaddr_q : s_axi_awaddr;
    wire [31:0] write_data = w_pending  ? wdata_q  : s_axi_wdata;
    wire [3:0]  write_strb = w_pending  ? wstrb_q  : s_axi_wstrb;

    always @(*) begin
        s_axi_awready = s_axi_aresetn && !s_axi_bvalid && !aw_pending;
        s_axi_wready  = s_axi_aresetn && !s_axi_bvalid && !w_pending;
    end

    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            aw_pending  <= 1'b0;
            w_pending   <= 1'b0;
            awaddr_q    <= 12'h0;
            wdata_q     <= 32'h0;
            wstrb_q     <= 4'h0;
            s_axi_bvalid <= 1'b0;
            s_axi_bresp  <= OKAY;
        end else begin
            if (aw_hs) begin
                awaddr_q   <= s_axi_awaddr;
                aw_pending <= 1'b1;
            end
            if (w_hs) begin
                wdata_q   <= s_axi_wdata;
                wstrb_q   <= s_axi_wstrb;
                w_pending <= 1'b1;
            end

            if (!s_axi_bvalid && have_aw && have_w) begin
                aw_pending   <= 1'b0;
                w_pending    <= 1'b0;
                s_axi_bvalid <= 1'b1;
                s_axi_bresp  <= OKAY;
                if (write_addr[11:2] < TOTAL_WORDS) begin
                    if (write_strb[0]) mem[write_addr[11:2]][7:0]   <= write_data[7:0];
                    if (write_strb[1]) mem[write_addr[11:2]][15:8]  <= write_data[15:8];
                    if (write_strb[2]) mem[write_addr[11:2]][23:16] <= write_data[23:16];
                    if (write_strb[3]) mem[write_addr[11:2]][31:24] <= write_data[31:24];
                end
            end else if (s_axi_bvalid && s_axi_bready) begin
                s_axi_bvalid <= 1'b0;
            end
        end
    end

    // ------------------------------------------------------------
    // 读事务
    // ------------------------------------------------------------
    wire ar_hs = s_axi_arvalid && s_axi_arready;

    reg [11:0] araddr_q;
    reg        doing_read;

    always @(posedge s_axi_aclk) begin
        if (!s_axi_aresetn) begin
            s_axi_arready <= 1'b0;
            doing_read    <= 1'b0;
            araddr_q      <= 12'h0;
            s_axi_rvalid  <= 1'b0;
            s_axi_rdata   <= 32'h0;
            s_axi_rresp   <= OKAY;
        end else begin
            s_axi_arready <= 1'b0;
            if (!doing_read && !s_axi_rvalid) begin
                s_axi_arready <= 1'b1;
                if (ar_hs) begin
                    araddr_q   <= s_axi_araddr;
                    doing_read <= 1'b1;
                end
            end
            if (doing_read) begin
                doing_read  <= 1'b0;
                s_axi_rvalid <= 1'b1;
                s_axi_rresp  <= OKAY;
                if (araddr_q[11:2] < TOTAL_WORDS)
                    s_axi_rdata <= mem[araddr_q[11:2]];
                else
                    s_axi_rdata <= 32'h0;
            end
            if (s_axi_rvalid && s_axi_rready) s_axi_rvalid <= 1'b0;
        end
    end

    // 未使用的通道
    wire unused = ^{s_axi_awprot, s_axi_arprot};

endmodule
