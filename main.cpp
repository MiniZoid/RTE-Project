#include "mbed.h"
#include "LCD_DISCO_F429ZI.h"

#define CTRL_REG1 0x20               
#define CTRL_REG1_CONFIG 0b01'10'1'1'1'1  
#define CTRL_REG4 0x23               
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0  
#define SPI_FLAG 1
#define OUT_X_L 0x28
#define DEG_TO_RAD (17.5f * 0.0174532925199432957692236907684886f / 1000.0f)


bool recording = false;

EventFlags flags;

void spi_cb(int event) {
    flags.set(SPI_FLAG); 
}

InterruptIn button(BUTTON1); 
DigitalOut led(LED1);
DigitalOut locked(LED2);  
       


void track(){
    led = !led; 
    recording = !recording;
}

int main(){
    button.rise(&track);
    SPI spi(PF_9, PF_8, PF_7, PC_1, use_gpio_ssel);
    uint8_t write_buf[32], read_buf[32];
    spi.format(8, 3);
    spi.frequency(1'000'000);
    write_buf[0] = CTRL_REG1;
    write_buf[1] = CTRL_REG1_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);  
    flags.wait_all(SPI_FLAG);
    write_buf[0] = CTRL_REG4;
    write_buf[1] = CTRL_REG4_CONFIG;
    spi.transfer(write_buf, 2, read_buf, 2, spi_cb);  
    flags.wait_all(SPI_FLAG);

    while(1){
        if(recording){
            uint16_t raw_gx, raw_gy, raw_gz;  
            float gx, gy, gz;  
            write_buf[0] = OUT_X_L | 0x80 | 0x40; 
            spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
            flags.wait_all(SPI_FLAG);  
            raw_gx = (((uint16_t)read_buf[2]) << 8) | read_buf[1];
            raw_gy = (((uint16_t)read_buf[4]) << 8) | read_buf[3];
            raw_gz = (((uint16_t)read_buf[6]) << 8) | read_buf[5];
            printf("RAW Angular Speed -> gx: %d deg/s, gy: %d deg/s, gz: %d deg/s\n", raw_gx, raw_gy, raw_gz);
            //printf(">x_axis: %d|g\n", raw_gx);
            //printf(">y_axis: %d|g\n", raw_gy);
            //printf(">z_axis: %d|g\n", raw_gz);
            gx = raw_gx * DEG_TO_RAD;
            gy = raw_gy * DEG_TO_RAD;
            gz = raw_gz * DEG_TO_RAD;
            //printf("Angular Speed -> gx: %.2f rad/s, gy: %.2f rad/s, gz: %.2f rad/s\n", gx, gy, gz);
            thread_sleep_for(100);        
        }
    }
}