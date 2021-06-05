#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/boot.h>

#define     WIFI_NAME   "Farid Online"
#define     WIFI_PASS   "#FARIDME3124"
#define     CHANNEL     875087
#define     API         "YQ7NZ5VQY7DZO8CH"
#define     JUMP_APP()  {asm("jmp 0x0000");} 



uint8_t  data_array[64][6];
uint32_t final_data[64];
uint8_t  boot_buffer[128];

void WDT_reset(void){
WDTCSR=(1<<WDCE)|(1<<WDE);
WDTCSR=(1<<WDE);
while(1);
}

void WDT_disable(void){
cli();
MCUSR=0;
WDTCSR|=(1<<WDCE)|(1<<WDE);
WDTCSR=0;
}

void ESP_init(uint32_t BAUD){
WDT_disable();
uint16_t UBRR_VAL=(((F_CPU/16)/BAUD)-1);
UBRR0H=UBRR_VAL>>8;
UBRR0L=UBRR_VAL;
UCSR0B=(1<<RXEN0)|(1<<TXEN0);
UCSR0C=(1<<UCSZ00)|(1<<UCSZ01);
_delay_ms(100);
}

void ESP_single(unsigned char data){
while((UCSR0A & (1<<UDRE0))==0);
UDR0=data;
}

void ESP_print_text(char *c){
for(uint8_t i=0;i<strlen(c);i++){
   ESP_single(c[i]);
   }
}

uint8_t ESP_rx(void){
while((UCSR0A & (1<<RXC0))==0);
return UDR0;
}

void ESP_print_dec(uint32_t x){
if(x!=0){
  uint8_t i=0;
  uint32_t y=x;
  while(y!=0){ y=y/10; i++; }
  uint8_t num[i];
  uint32_t z=x;
  for(int j=(i-1);j>-1;j--) { num[j]=(z%10)+48; z=z/10;}
  for(uint8_t k=0;k<i;k++)  { ESP_single(num[k]); }
  }
else{ESP_single('0');}
}

void connect_wifi(void){
ESP_print_text("AT+CWMODE=3");
_delay_ms(1000);
ESP_print_text("AT+CWJAP=\"");
ESP_print_text(WIFI_NAME);
ESP_print_text("\",\"");
ESP_print_text(WIFI_PASS);
ESP_print_text("\"\r\n");
_delay_ms(10000);
ESP_print_text("AT+CIPMUX=0");
_delay_ms(1000);
}

void read_field(uint8_t field,uint32_t channel, char c[]){
ESP_print_text("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80\r\n");
_delay_ms(2000);
ESP_print_text("AT+CIPSEND=");
ESP_print_dec(71+field/10);
ESP_print_text("\r\n");
_delay_ms(2000);
ESP_print_text("GET /channels/");
ESP_print_dec(channel);
ESP_print_text("/fields/");
ESP_print_dec(field);
ESP_print_text(".csv?api_key=");
ESP_print_text(API);
ESP_print_text("&results=16\r\n");
}


uint32_t mpl(uint8_t dgt){
uint32_t mpl_val=1;
for(uint8_t i=0;i<dgt;i++){mpl_val*=10;}
return mpl_val;
}

void read_data(uint8_t field){
uint8_t entry=0;
uint8_t temp=0,bits=0,parameters[16];
uint32_t final=0;

read_field(field,CHANNEL,API);

START:
bits=0;
while(ESP_rx()!='U');
while(ESP_rx()!='T');
while(ESP_rx()!='C');
while(ESP_rx()!=',');

ENTRY:
temp=ESP_rx();
if(temp!=44){goto ENTRY;}

DATA:
temp=ESP_rx();
if(temp!=10){data_array[entry][bits]=temp-48;bits++;goto DATA;}
parameters[entry]=bits;

entry++;
if(entry<16){goto START;}

for(uint8_t ent=0;ent<16;ent++){
   for(uint8_t i=0;i<parameters[ent];i++){final+=data_array[ent][i]*mpl(parameters[ent]-1-i);}
   final_data[ent]=final;
   final=0;
   }
}


void boot_program_page (uint32_t page, uint8_t *buf){

    uint16_t i;
    uint8_t sreg;
    sreg = SREG;
    cli();
    eeprom_busy_wait ();
    boot_page_erase (page);
    boot_spm_busy_wait (); 
	
    for (i=0; i<SPM_PAGESIZE; i+=2)
    {
        uint16_t w = *buf++;
        w += (*buf++) << 8;
        boot_page_fill (page + i, w);
    }
	
    boot_page_write (page); 
    boot_spm_busy_wait();   
    boot_rww_enable ();
    SREG = sreg;
}

void boot_write_pages(void){
dbg_print_text("\n");
dbg_print_text("|========|");
 dbg_print_text("\n|");
for(uint8_t i=0;i<4;i++){
 read_data(i+1);
 dbg_print_text("=");
 for(uint8_t k=0;k<16;k++){boot_buffer[(2*k)+(32*i)]=final_data[k]/1000;boot_buffer[(2*k)+1+(32*i)]=final_data[k]%1000;}
 _delay_ms(2000);
 }
 boot_program_page(0*128,boot_buffer);
 
 for(uint8_t i=0;i<4;i++){
 read_data(i+5);
 dbg_print_text("=");
 for(uint8_t k=0;k<16;k++){boot_buffer[(2*k)+(32*i)]=final_data[k]/1000;boot_buffer[(2*k)+1+(32*i)]=final_data[k]%1000;}
 _delay_ms(2000);
 }
 dbg_print_text("|");
 
 boot_program_page(1*128,boot_buffer);
 dbg_print_text("\nProgram download complete!\nJumping to app!\n");
 JUMP_APP();
}