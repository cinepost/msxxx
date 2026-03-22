module virtual_dip_switch (
    input wire clk,           // FPGA System Clock
    input wire rst_n,         // Reset (Active Low)
    input wire pico_sclk,     // Serial Clock from Pico
    input wire pico_data,     // Serial Data from Pico
    input wire pico_latch,    // Latch signal from Pico
    output reg [7:0] sw_out   // 8-bit "Virtual Switch" output
);

    reg [7:0] shift_reg;
    reg [2:0] bit_cnt;

    // Shift in data from Pico on rising edge of Pico's clock
    always @(posedge pico_sclk or negedge rst_n) begin
        if (!rst_n) begin
            shift_reg <= 8'b0;
        end else begin
            shift_reg <= {shift_reg[6:0], pico_data};
        end
    end

    // Update the actual switch output when the Latch signal goes high
    always @(posedge pico_latch or negedge rst_n) begin
        if (!rst_n) begin
            sw_out <= 8'b0;
        end else begin
            sw_out <= shift_reg;
        end
    end

endmodule