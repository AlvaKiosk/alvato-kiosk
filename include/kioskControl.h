/* Board: Kiosk Control v1.0.3 */
#define RGB_LED     2
#define NUM_LEDS    1

#define COININ      35
#define BILLIN      33
#define ENCOIN      26

#define UNLOCK      13

#define RXU1        16
#define TXU1        17

#define RXU2        27 // DIO
#define TXU2        25 // CLK

#define I2C_SDA     21
#define I2C_SCL     22

#define IRQ1        32
#define RST         12

#define BUZZ        4

#define SCLK        18
#define MISO        19
#define MOSI        23
#define CS1         5
#define CS2         14

#define DOORSEN     39
#define KEY         34

#define INTERRUPT_SET ( (1ULL<<BILLIN) |(1ULL<<COININ) | (1ULL<<IRQ1))
    //*******************************


#define INPUT_SET ( (1ULL<<DOORSEN) |(1ULL<<KEY) )
#define OUTPUT_SET ( (1ULL<<ENCOIN) |(1ULL<<UNLOCK) |(1ULL<<BUZZ) |(1ULL<<CS1) |(1ULL<<CS2) )
