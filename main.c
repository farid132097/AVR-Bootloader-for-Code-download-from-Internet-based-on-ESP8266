#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include "debug.h"
#include "iboot_rec.h"




int main(void){

ESP_init(9600);
_delay_ms(5000);
dbg_enable();
dbg_print_text("\n\n\nDownloading firmware from internet!\n");
boot_write_pages();

while(1){
			  
			  
			  }

}