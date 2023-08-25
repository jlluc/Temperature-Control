#include <xc.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "I2C_Support.h"
#include "Setup_Alarm_Time.h"
#include "Main_Screen.h"
#include "Main.h"
#include "Setup_Time.h"
#include "TFT_ST7735.h"

extern char setup_alarm_time[]; 
extern unsigned char alarm_second, alarm_minute, alarm_hour, alarm_date;
extern unsigned char setup_alarm_second, setup_alarm_minute, setup_alarm_hour;
extern int INT0_flag, INT1_flag, INT2_flag;
extern char *txt;

void Setup_Alarm_Time(void)
{
    char Key_Next_Flag, Key_Up_Flag, Key_Dn_Flag;
    char Select_Field;    
    Select_Field = 0;
    DS3231_Read_Alarm_Time();                   // Read alarm time
    alarm_second = bcd_2_dec(alarm_second);     // convert to dec for use
    alarm_minute = bcd_2_dec(alarm_minute);
    alarm_hour   = bcd_2_dec(alarm_hour);
    setup_alarm_second = alarm_second;          // Beginning setup alarm time as current set alarm time
    setup_alarm_minute = alarm_minute;
    setup_alarm_hour = alarm_hour;
    Initialize_Setup_Alarm_Time_Screen();       // Initialize the alarm setup screen
    Update_Setup_Alarm_Time_Screen();           // Update the alarm screen with latest info

    while (enter_setup == 1)
    {   
        if (INT0_flag == 1)
        {
            INT0_flag = 0;
            Key_Up_Flag = 1;
        }
        if (INT1_flag == 1)
        {
            INT1_flag = 0;
            Key_Dn_Flag = 1;
        }
        if (INT2_flag == 1)
        {
            INT2_flag = 0;
            Key_Next_Flag = 1;
        }
        if (Key_Next_Flag == 1 )                        // If user switches
        {        
            Select_Field++;                             // Move to next field
            if (Select_Field == 3){ Select_Field = 0;}    // Wrap around if necessary
            Update_Setup_Screen_Cursor(Select_Field);   // Update cursor
            Key_Next_Flag = 0;
        }
        if (Key_Up_Flag == 1)
        {
            switch (Select_Field)
            {
                case 0:
                setup_alarm_hour++;
                if (setup_alarm_hour >= 24) {setup_alarm_hour = 0;}
                printf ("incremented hour for alarm \r\n");
                break;
                
                case 1:
                setup_alarm_minute++;
                if (setup_alarm_minute >= 60) {setup_alarm_minute = 0;}
                printf ("incremented minute for alarm \r\n");
                break;
                
                case 2:
                setup_alarm_second++;
                if (setup_alarm_second >= 60) {setup_alarm_second = 0;}
                printf ("incremented second for alarm \r\n");
                break;
                
                default:
                    break;
            }
            Update_Setup_Alarm_Time_Screen();
            Key_Up_Flag = 0;
        }
        if (Key_Dn_Flag == 1)
        {
            switch (Select_Field)
            {
                case 0:
                if (setup_alarm_hour <= 0) setup_alarm_hour = 23;
                else setup_alarm_hour--;
                printf ("decremented hour for alarm \r\n");
                break;
                
                case 1:
                if (setup_alarm_minute <= 0) setup_alarm_minute = 59;
                else setup_alarm_minute--;
                printf ("decremented minute for alarm \r\n");
                break;
                
                case 2:
                if (setup_alarm_second <= 0) setup_alarm_second = 59;
                else setup_alarm_second--;
                printf ("decremented second for alarm \r\n");
                break;
                
                default:
                    break;
            }
            Update_Setup_Alarm_Time_Screen();
            Key_Dn_Flag = 0;
        }
    }         
    DS3231_Write_Alarm_Time();                          // Write alarm time
    DS3231_Read_Alarm_Time();                           // Read alarm time
    DS3231_Read_Time();                                 // Read current time
    Initialize_Screen();                                // Initialize main screen before returning
}
     
void Initialize_Setup_Alarm_Time_Screen(void) 
{ 
    fillScreen(ST7735_BLACK);                                   // Fills background of screen with color passed to it
 
    strcpy(txt, "ECE3301L Fall20 Final\0");                     // Text displayed 
    drawtext(start_x , start_y, txt, ST7735_WHITE  , ST7735_BLACK, TS_1);
                                                                // X and Y coordinates of where the text is to be displayed

    strcpy(txt, "Alrm Setup");                                  // Text displayed 
    drawtext(start_x+5 , start_y+20, txt, ST7735_MAGENTA, ST7735_BLACK, TS_2); 
    
    strcpy(txt, "Time");
    drawtext(time_x  , time_y , txt, ST7735_BLUE   , ST7735_BLACK, TS_1);
    fillRect(data_time_x-1, data_time_y+16, 25,2,ST7735_CYAN);
}

void Update_Setup_Alarm_Time_Screen(void)
{
    setup_alarm_time[0]  = (setup_alarm_hour/10)  + '0';
    setup_alarm_time[1]  = (setup_alarm_hour%10)  + '0';
    setup_alarm_time[3]  = (setup_alarm_minute/10)  + '0';
    setup_alarm_time[4]  = (setup_alarm_minute%10)  + '0';
    setup_alarm_time[6]  = (setup_alarm_second/10)  + '0';
    setup_alarm_time[7]  = (setup_alarm_second%10)  + '0';
    drawtext(data_time_x, data_time_y, setup_alarm_time, ST7735_CYAN, ST7735_BLACK, TS_2);
}
 
