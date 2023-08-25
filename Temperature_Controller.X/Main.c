
#include <stdio.h>
#include <stdlib.h>
#include <xc.h>
#include <math.h>
#include <p18f4620.h>
#include <usart.h>
#include <string.h>

#include "Main.h"
#include "I2C_Support.h"
#include "I2C_Soft.h"
#include "TFT_ST7735.h"
#include "Interrupt.h"
#include "Main_Screen.h"
#include "Setup_Alarm_Time.h"
#include "Setup_Fan_Temp.h"
#include "Setup_Time.h"

#pragma config OSC      =   INTIO67
#pragma config BOREN    =   OFF
#pragma config WDT      =   OFF
#pragma config LVP      =   OFF
#pragma config CCP2MX   =   PORTBE

void Initialize_Screen(void); 
void Update_Screen(void);
void Do_Init(void);
float read_volt();
int get_duty_cycle(int,int);
int get_RPM(); 
void Monitor_Fan();
void Turn_Off_Fan();
void Turn_On_Fan();
unsigned int get_full_ADC();
void Get_Temp(void);
void Update_Volt(void);
void Test_Alarm(void);
void Activate_Buzzer();
void Deactivate_Buzzer();

void Main_Screen(void);
void Do_Setup(void);
void do_update_pwm(char);
void Set_RGB_Color(char);


char buffer[31]         = " ECE3301L Fall'20 L12\0";
char *nbr;
char *txt;
char tempC[]            = "+25";
char tempF[]            = "+77";
char time[]             = "00:00:00";
char date[]             = "00/00/00";
char alarm_time[]       = "00:00:00";
char Alarm_SW_Txt[]     = "OFF";                // text storage for Alarm Mode
char Fan_SW_Txt[]       = "OFF";                // text storage for Heater Mode
char Fan_Set_Temp_Txt[] = "075F";               // text storage for desired temperature
char Volt_Txt[]         = "0.00V";              // text storage for Volt     
char DC_Txt[]           = "000";                // text storage for Duty Cycle value
char RTC_ALARM_Txt[]    = "0";                  // text storage for alarm and time match
char RPM_Txt[]          = "0000";               // text storage for RPM

char setup_time[]       = "00:00:00";           //text storage for time in setup
char setup_date[]       = "01/01/00";           //text storage for date in setup
char setup_alarm_time[] = "00:00:00";           //text storage for alarm in setup
char setup_fan_text[]   = "075F";               //text storage for desired temp in setup

signed int DS1621_tempC, DS1621_tempF;

int INT0_flag, INT1_flag, INT2_flag, Tach_cnt;
int ALARMEN;
int FANEN;
int alarm_mode, MATCHED;
int color = 0;
unsigned char second, minute, hour, dow, day, month, year, old_sec;
unsigned char alarm_second, alarm_minute, alarm_hour, alarm_date;
unsigned char setup_alarm_second, setup_alarm_minute, setup_alarm_hour;
unsigned char setup_second, setup_minute, setup_hour, setup_day, setup_month, setup_year;
unsigned char setup_fan_temp = 75;
float volt;
int duty_cycle;
int rpm;

int Tach_cnt = 0;		


void putch (char c)
{   
    while (!TRMT);       
    TXREG = c;
}

void init_UART()
{
    OpenUSART (USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & USART_CONT_RX & USART_BRGH_HIGH, 25);
    OSCCON = 0x70;
}

void Init_ADC()                     // Initialize ADCON settings
{
    ADCON0 = 0X01;
    ADCON1 = 0X0E;
    ADCON2 = 0XA9;
}

void Init_IO()                      // Initialize input/output TRIS settings
{
    TRISA = 0X0F;
    TRISB = 0X07;
    TRISC = 0X01;
    TRISD = 0X00;
    TRISE = 0X00;
}

void Do_Init()                                      // Initialize the ports 
{ 
    init_UART();                                    // Initialize the uart
    OSCCON = 0x70;                                  // Set oscillator to 8 MHz 
    Init_ADC();
    Init_IO();
    RBPU = 0;
       
    TMR1L = 0x00;       //<-- put code here to program Timer 1 as in counter mode. Copy the code from                             
    T1CON = 0x03;       //<-- the Get_RPM() function
 
    T0CON = 0x03;       //<-- Program the Timer 0 to operate in time mode to do an interrupt every 500 msec                                                 
    TMR0L = 0xDB;                       // set the lower byte of TMR
    TMR0H = 0x0B;                       // set the upper byte of TMR
    INTCONbits.TMR0IF = 0;  //<-- Clear the interrupt flag        
    T0CONbits.TMR0ON = 1;   //<-- Enable the timer 0         
    INTCONbits.TMR0IE = 1;  //<-- enable timer 0 interrupt        
    Init_Interrupt();       //<-- initialize the other interupts
    
    I2C_Init(100000);               
    DS1621_Init();

} 

void main()
{
    Do_Init();                          // Initialization    

    txt = buffer;     

    Initialize_Screen();

    old_sec = 0xff;
    Turn_Off_Fan();
//  DS3231_Write_Initial_Alarm_Time();                  // uncommented this line if alarm time was corrupted    
    DS3231_Read_Time();                                 // Read time for the first time
    DS3231_Read_Alarm_Time();                           // Read alarm time for the first time
    DS3231_Turn_Off_Alarm();

    while(TRUE)
    { 
        if (enter_setup == 0)         // If setup switch is LOW...
        {
            Main_Screen();              // stay on main screen.
        }
        else                            // Else,
        {
            Do_Setup();                 // Go to setup screen.
        }
    }
}

void Main_Screen()
{
    if (INT0_flag == 1)
        {
            INT0_flag = 0;              //clear flag
            Turn_Off_Fan();             //call turn off fan function
        }
    if (INT1_flag == 1)
        {
            INT1_flag = 0;              //clear flag
            Turn_On_Fan();              //call turn on fan function
        }
    if (INT2_flag == 1)
        {
            
            INT2_flag = 0;              //clear flag
            if(ALARMEN == 0)            //toggles alarm
                ALARMEN = 1;                //switches on if off...
            else
                ALARMEN = 0;                //and switches off if on
        }
        
    DS3231_Read_Time();                                 // Read time
    if (old_sec != second)
    {
        old_sec = second;
        Get_Temp();                                     //call function to get temperature
        volt = read_volt();                             //call function to get voltage of photosensor
        if (FANEN == 1)                 //check fanen
            {
                Monitor_Fan();          //call monitor fan function
            }

        Test_Alarm();                   // call routine to handle the alarm  function

        printf ("%02x:%02x:%02x %02x/%02x/%02x\r\n ",hour,minute,second,month,day,year);    //print to TeraTerm to check time
        printf ("duty cycle = %d  RPM = %d \r\n", duty_cycle, rpm);                         //print to TeraTerm to check duty cycle and rpm for fan
        

        Update_Screen();                                                                    //update screen every second
    }    
}

void Do_Setup()
{
    // add code to decode the value of the switch 'setup_sel1' and 'setup_sel0' to call:
    // 00 for Setup_Time()
    // 01 for Setup_Alarm_Time()
    // 10 for Setup_Temp_Fan()
    while(enter_setup == 1){                                                    //enter setup mode
    if (setup_sel1 == 1)                                                        //select is 1X, sel0 is not necessary when sel1 = 1
    {
        Setup_Temp_Fan();                                                       //call temperature setup function
    }
    else 
    {
        if (setup_sel0 == 1)                                                    //select is 01
        {
            Setup_Alarm_Time();                                                 //call alarm setup function
        }
        else                                                                    //select is 00
        {
            Setup_Time();                                                       //call time setup function
        }
    }
  }
}

void Get_Temp(void)
{
    DS1621_tempC = DS1621_Read_Temp();              // Reads temp from DS1621 and stores in tempC


    if ((DS1621_tempC & 0x80) == 0x80)               //if temp is a complemented negative number
    {
        DS1621_tempC = 0x80 - (DS1621_tempC & 0x7f);      //conversion from complemented negative number for celsius
        DS1621_tempF = 32 - ((DS1621_tempC * 9) /5);      //conversion from complemented negative number for fahrenheit
        printf ("Temperature = -%dC or %dF\r\n\n", DS1621_tempC, DS1621_tempF);    //print statement to check negative temperature in TeraTerm
        DS1621_tempC = 0x80 | DS1621_tempC;               //restores complemented tempC after use     
    }
    else                                                                        //if temp is a positive number
    {
        DS1621_tempF = ((DS1621_tempC * 9) /5) + 32;                            //convert from celsius to fahrenheit normally
        printf ("Temperature = %dC or %dF\r\n\n", DS1621_tempC, DS1621_tempF);  //print to TeraTerm
    }
}

void Monitor_Fan()
{
    // add code to calculate duty cycle based on difference between the actual temperature
    // and the 'setup_fan_temp'
    // update the pwm
    // 
    duty_cycle = get_duty_cycle(DS1621_tempF, setup_fan_temp);          //duty cycle obtained from get function, using tempF and desired setup temp as variables
    do_update_pwm(duty_cycle);                                          //updates pwm for the fan based on duty cycle calculated
    rpm = get_RPM();                                                    //rpm is measured out and placed in this variable
}

float read_volt()
{
	int nStep = get_full_ADC();
    volt = nStep * 5 /1024.0;
	return (volt);
}

int get_duty_cycle(int temp, int set_temp)
{	
    int dc = 2*(set_temp - temp);                                        //duty cycle is equal to twice the difference of set_temp and temp
    if (dc > 100)
    {
        dc = 100;                                                        //ensures duty cycle can never go above 100
    }
    if (dc < 0)
    {
        dc = 0;                                                          //ensures duty cycle can never go below 0
    }
    return dc;                                                           //returns dc as int

}

int get_RPM()
{
    int RPS = Tach_cnt;                                            // variable RPS, rotations per second is set equal to tach_cnt from Timer0
    return (RPS*60);                                               //returns RPM by multiplying RPS by 60
}


void Turn_Off_Fan()
{
    duty_cycle = 0;                                     // Set DC to 0 for off
    do_update_pwm(duty_cycle);                          // Call function to update duty cycle
    rpm = 0;                                            // Set rpm to 0 for off
    FANEN = 0;                                          // Turn off FANEN
    FANEN_LED = 0;                                      // Turn off FANEN_LED
}

void Turn_On_Fan()
{
    FANEN = 1;                                          // Turn on FANEN
    FANEN_LED = 1;                                      // Turn on FANED_LED
}

void do_update_pwm(char duty_cycle) 
{ 
    float dc_f;
    int dc_I;
    PR2 = 0b00000100 ;                                  // set the frequency for 25 Khz
    T2CON = 0b00000111 ;                                //
    dc_f = ( 4.0 * duty_cycle / 20.0) ;                 // calculate factor of duty cycle versus a 25 Khz
                                                        // signal
    dc_I = (int) dc_f;                                  // get the integer part
    if (dc_I > duty_cycle) dc_I++;                      // round up function
    CCP1CON = ((dc_I & 0x03) << 4) | 0b00001100;
    CCPR1L = (dc_I) >> 2;
}

unsigned int get_full_ADC()
{
    unsigned int result;
    ADCON0bits.GO=1;                     // Start Conversion
    while(ADCON0bits.DONE==1);           // wait for conversion to be completed
    result = (ADRESH * 0x100) + ADRESL;  // combine result of upper byte and
                                        // lower byte into result
    return result;                       // return the result.
}

void Activate_Buzzer()
{
    PR2 = 0b11111001 ;
    T2CON = 0b00000101 ;
    CCPR2L = 0b01001010 ;
    CCP2CON = 0b00111100 ;
}

void Deactivate_Buzzer()
{
    CCP2CON = 0x0;
    PORTBbits.RB3 = 0;
}

void Test_Alarm()
{
    // put code to detect whether the alarm is activated, or deactivated or is waiting for the alarm to 
    // happen
    
    // the variable ALARMEN is used as the switch that is toggled by the user
    // the variable alarm_mode stored the actual mode of the alarm. If 1, the alarm is activated. If 0, no
    // alarm is enabled yet
    
    // the RTC_ALRAM_NOT is the signal coming from the RTC to show whether the alarm time does match with
    // the actual time. This signal is active low when the time matches.
    
    // Use a variable MATCHED to register the event that the time match has occurred. This is needed
    // to change the color of the RGB LED
    
    // this routine should perform the different conditions:
    // Case 1: switch is turned on but alarm_mode is not on
    // Case 2: switch is turned off but alarm mode is already on
    // Case 3: switch is on and alarm_mode is on. In this case, more checks are to be performed.
    // Use the provided function DS3231_Turn_On_Alarm() and DS3231_Turn_Off_Alarm() to activate or deactivate
    // the alarm.
    if (ALARMEN == 1 && alarm_mode == 0)              //signal to enable alarm is enabled, but alarm has not been turned on
    {
        DS3231_Turn_On_Alarm();                       //Alarm on DS3231 board is turned on
        alarm_mode = 1;                               //Alarm is officially on
    }
    if (ALARMEN == 0 && alarm_mode == 1)              //alarm has been turned on, but signal to enable alarmed is cleared
    {
        alarm_mode = 0;                               //Alarm is officially off
        DS3231_Turn_Off_Alarm();                      //turns off alarm on DS3231 board
        MATCHED = 0;                                  //sets matched to 0 to make sure alarm can't be triggered
        Deactivate_Buzzer();                          //force buzzer off
        Set_RGB_Color(0);                             //force RGB to no color
        color = 0;                                    //resets RGB cycle for when alarm is triggered next
    }
    if (ALARMEN == 1 && alarm_mode == 1)              //signal to enable alarm is true, and alarm is armed
    {
        if (RTC_ALARM_NOT == 0)                       //alarm time and time match
        {
            MATCHED = 1;                              //matched is set to indicate alarm needs to be triggered due to time match
            Activate_Buzzer();                        //buzzer is turned on
            
        }
        if (read_volt() > 2.5)                        //photosensor is in dark
        {
            MATCHED = 0;                              //clears matched, prevents RGB cycling and alarm from being triggered
            Deactivate_Buzzer();                      //forces buzzer off
            Set_RGB_Color(0);                         //force RGB to no color  
            color = 0;                                //resets RGB cycle for when alarm is triggered next  
            DS3231_Turn_Off_Alarm();                  //turns off alarm on DS3231 board
        }
        if (MATCHED == 1)                             
        {
            Set_RGB_Color(color);                     //color starts at 0
            color++;                                  //color is incremented every second to change RGB color
            if(color > 7)                          
            { 
               color = 0;                             //when color has finished cycling through all 8 colors, return to 0
            }
        }
    }

}

void Set_RGB_Color(char color)
{
    switch(color)
    {
        case 0: PORTDbits.RD4 = 0; PORTDbits.RD5 = 0; PORTDbits.RD6 = 0; break;             //sets RGB to no color
        case 1: PORTDbits.RD4 = 1; PORTDbits.RD5 = 0; PORTDbits.RD6 = 0; break;             //sets RGB to RED
        case 2: PORTDbits.RD4 = 0; PORTDbits.RD5 = 1; PORTDbits.RD6 = 0; break;             //sets RGB to GRN
        case 3: PORTDbits.RD4 = 1; PORTDbits.RD5 = 1; PORTDbits.RD6 = 0; break;             //sets RGB to YLW
        case 4: PORTDbits.RD4 = 0; PORTDbits.RD5 = 0; PORTDbits.RD6 = 1; break;             //sets RGB to BLU
        case 5: PORTDbits.RD4 = 1; PORTDbits.RD5 = 0; PORTDbits.RD6 = 1; break;             //sets RGB to PURPL
        case 6: PORTDbits.RD4 = 0; PORTDbits.RD5 = 1; PORTDbits.RD6 = 1; break;             //sets RGB to CYAN
        case 7: PORTDbits.RD4 = 1; PORTDbits.RD5 = 1; PORTDbits.RD6 = 1; break;             //sets RGB to WHT
    }
}

