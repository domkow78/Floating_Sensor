#include "driver/uart.h"
//#include "ctype.h"
//#include "string.h"

/*
    Use in main loop, like this:
    String str;
    if (readUntil(Serial,str)){
        Serial.println(str);
        str.clear();
    }
*/

#define NOP() asm volatile ("nop")

//char* strtrim (char* s){
//    int i;
//    char* ptr = s;
//    while (isspace (*ptr)) ptr++;   // skip left side white spaces
//    for (i = strlen (ptr) - 1; (isspace (ptr[i])); i--) ;   // skip right side white spaces
//    ptr[i + 1] = '\0';
//    return ptr;
//}

/* Reads up to len bytes from uart, waiting until timeoutms or any of terminators, or len is reached.
 * Echoes back.
 * Reserve buf for len+1 bytes - the last one is \0
 */
uint8_t readUntil(uart_port_t uart, char* buf, uint8_t len, TickType_t timeoutms, const char* terminators = "\x0A\x0D"){
	TickType_t tend = xTaskGetTickCount() + pdMS_TO_TICKS(timeoutms);
	char c;
	uint8_t count = 0;
	if ((!len) || (!buf))
		return 0;
	while (xTaskGetTickCount() < tend) {
		if (uart_read_bytes(uart, &c, 1, pdMS_TO_TICKS(100)) > 0){
			uart_write_bytes(uart, &c, 1);
			tend = xTaskGetTickCount() + pdMS_TO_TICKS(timeoutms);
			if ((c == '\x0A')||(c == '\x0D')) {
				break;
			} else if (c > 0 && c < 127) {
				buf[count] = c;
				++count;
				if (count > len-1){
					break;
				}
			}
		}
	}
	buf[count] = '\0';
	return count;
//	uart_flush_input(uart);
}


//bool readUntil(HardwareSerial& inStream, String& str, const char* terminators = "\x0A\x0D"){
//    char ch;
//    while (inStream.available() > 0) {
//        ch = inStream.read();
//        //inStream.print(ch);
//        if (!strchr(terminators,ch)){
//            str+=ch;
//            //inStream.printf("\treadUntil: appended and have %s\n",str.c_str());
//        }
//        else {
//            //inStream.printf("\treadUntil: about to exti %s\n",str.c_str());
//            return true;
//        }
//    }
//    return false;
//}

/**
 * @brief delay implementation via NOP, kept in IRAM.
 * @param
 * @return none
 */
void IRAM_ATTR delayUsbyNops(uint32_t us)
{
	int64_t m = esp_timer_get_time();
	if(us){
		int64_t e = (m + us);
		if(m > e){ //overflow
			while(esp_timer_get_time() > e){
				NOP();
			}
		}
		while(esp_timer_get_time() < e){
			NOP();
		}
	}
}
