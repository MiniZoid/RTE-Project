#include "mbed.h"
#include "drivers/LCD_DISCO_F429ZI.h"

#define CTRL_REG1 0x20               
#define CTRL_REG1_CONFIG 0b01'10'1'1'1'1  
#define CTRL_REG4 0x23               
#define CTRL_REG4_CONFIG 0b0'0'01'0'00'0  
#define SPI_FLAG 1
#define OUT_X_L 0x28


bool recording = false;
bool set = false;
bool unlocked = false;
int errs = 0;

float x[30];
float y[30];
float z[30];
float xCheck[30];
float yCheck[30];
float zCheck[30];

EventFlags flags;
LCD_DISCO_F429ZI lcd;

void spi_cb(int event) {
    flags.set(SPI_FLAG); 
}

InterruptIn button(BUTTON1); 
DigitalOut setMode(LED2);
DigitalOut unlockMode(LED1);  
       
void drawLockScreen(){
    lcd.Clear(LCD_COLOR_RED);
    lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Locked", CENTER_MODE);
}

void drawRecScreen(){
    lcd.Clear(LCD_COLOR_YELLOW);
    lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Recording...", CENTER_MODE);
}

void drawUnlockingScreen(){
    lcd.Clear(LCD_COLOR_BLUE);
    lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Unlocking...", CENTER_MODE);
}

void drawUnlockScreen(){
    lcd.Clear(LCD_COLOR_GREEN);
    lcd.DisplayStringAt(0, LINE(3), (uint8_t *)"Unlocked", CENTER_MODE);
    lcd.DisplayStringAt(0, LINE(5), (uint8_t *)"Press to lock", CENTER_MODE);
}

void track(){
    setMode = !setMode; 
    recording = !recording;
    unlockMode = !unlockMode;
}

bool tolCheck(float bench, float check){
    if(abs(bench-check)<2000){
        return true;
    }
    return false;
}

void checkPattern(){
    for(int ii=0; ii<30; ii++){
        if(tolCheck(x[ii], xCheck[ii]) == false && tolCheck(y[ii], yCheck[ii]) == false && tolCheck(z[ii], zCheck[ii]) == false){
            errs++;
            if(errs>5){
                drawLockScreen();
                errs = 0;
                return;
            }
        }
    }
    drawUnlockScreen();
    unlocked = true;
}

int main(){
    unlockMode = true;
    drawLockScreen();
    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"Pattern Not Set.", CENTER_MODE);
    lcd.DisplayStringAt(0, LINE(8), (uint8_t *)"Press to record", CENTER_MODE);
    lcd.DisplayStringAt(0, LINE(9), (uint8_t *)"3 sec pattern", CENTER_MODE);
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
    uint16_t raw_gx, raw_gy, raw_gz;  
    while(1){ 
        if(recording && !set){
            drawRecScreen();
            set = true;
            printf("Recording data! \n");
            for(int ii=0; ii<30; ii++){
                write_buf[0] = OUT_X_L | 0x80 | 0x40; 
                spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
                flags.wait_all(SPI_FLAG);
                if(ii == 0){
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"3", CENTER_MODE);  
                }  
                if(ii == 10){
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"2", CENTER_MODE);  
                }
                if(ii == 20){
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"1", CENTER_MODE);  
                }
                raw_gx = (((uint16_t)read_buf[2]) << 8) | read_buf[1];
                raw_gy = (((uint16_t)read_buf[4]) << 8) | read_buf[3];
                raw_gz = (((uint16_t)read_buf[6]) << 8) | read_buf[5];
                x[ii] = raw_gx;
                y[ii] = raw_gy;
                z[ii] = raw_gz;
                printf("x: %d, y: %d, z: %d \n", raw_gx, raw_gy, raw_gz);
                thread_sleep_for(100); 
            }
            printf("Recorded! \n");
            recording = false;
            unlockMode = !unlockMode;
            setMode = !setMode;
            drawLockScreen();     
        }
        if(set && recording && !unlocked){
            drawUnlockingScreen();
            printf("Checking data! \n");
            for(int ii=0; ii<30; ii++){
                write_buf[0] = OUT_X_L | 0x80 | 0x40; 
                spi.transfer(write_buf, 7, read_buf, 7, spi_cb);
                flags.wait_all(SPI_FLAG);  
                raw_gx = (((uint16_t)read_buf[2]) << 8) | read_buf[1];
                raw_gy = (((uint16_t)read_buf[4]) << 8) | read_buf[3];
                raw_gz = (((uint16_t)read_buf[6]) << 8) | read_buf[5];
                if(ii == 0){
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"3", CENTER_MODE);  
                }  
                if(ii == 10){
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"2", CENTER_MODE);  
                }
                if(ii == 20){
                    lcd.DisplayStringAt(0, LINE(7), (uint8_t *)"1", CENTER_MODE);  
                }
                xCheck[ii] = raw_gx;
                yCheck[ii] = raw_gy;
                zCheck[ii] = raw_gz;
                printf("x: %0.2f, y: %0.2f, z: %0.2f \n", xCheck[ii], yCheck[ii], zCheck[ii]);
                thread_sleep_for(100); 
            }
            printf("Checking done! \n");
            recording = false;
            unlockMode = !unlockMode;
            setMode = !setMode;
            checkPattern();
        }
        if(unlocked && recording){
            recording = false;
            unlocked = false;
            unlockMode = !unlockMode;
            setMode = !setMode;
            drawLockScreen();
        } 
    }
}