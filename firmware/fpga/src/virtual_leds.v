module virtual_leds (
    input  wire sclk,      // From Pico GP2
    input  wire mosi,      // From Pico GP3 (New DIP states)
    output wire miso,      // To Pico GP16 (Readback states)
    input  wire cs_n,      // From Pico GP7
    output reg [7:0] leds  // FPGA uses Pico's DIP values for LEDs
);

    reg [7:0] shift_reg;
    reg [7:0] internal_status = 8'b10100101; // Data FPGA sends to Pico

    assign miso = shift_reg[7];

    always @(posedge sclk or posedge cs_n) begin
        if (cs_n) begin
            shift_reg <= internal_status; // Prepare readback data
        end else begin
            shift_reg <= {shift_reg[6:0], mosi}; // Shift In/Out
        end
    end

    // Apply the "DIP Switches" from Pico to FPGA LEDs on CS high
    always @(posedge cs_n) begin
        leds <= shift_reg;
    end
endmodule