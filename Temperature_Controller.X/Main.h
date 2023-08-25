
#define _XTAL_FREQ      8000000

#define ACK             1
#define NAK             0

#define ACCESS_CFG      0xAC
#define START_CONV      0xEE
#define READ_TEMP       0xAA
#define CONT_CONV       0x02

#define TFT_DC          PORTDbits.RD0
#define TFT_CS          PORTDbits.RD1
#define TFT_RST         PORTDbits.RD2

#define enter_setup     PORTAbits.RA1           // <-- Need to change the assignments here
#define setup_sel0      PORTAbits.RA2           // <-- Need to change the assignments here
#define setup_sel1      PORTAbits.RA3           // <-- Need to change the assignments here

#define SEC_LED         PORTEbits.RE1           // <-- Need to change the assignments here

#define FANEN_LED       PORTEbits.RE2           // <-- Need to change the assignments here
#define RTC_ALARM_NOT   PORTAbits.RA4           // <-- Need to change the assignments here




