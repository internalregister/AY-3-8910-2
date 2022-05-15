// Reading a SNG song file and playing it through one or two AY-3-8910 programmable sound generators
// This version uses an internal buffer constantly being filled and an interrupt to push the values into the AY-3-8910s
// with the correct timing

// Sérgio Vieira 2022

#include <SD.h>

const int SD_CARD_SELECT_PIN = 10;
const int RESET_PIN = 8;
const int PSG1_BC1_PIN = A5;
const int PSG1_BDIR_PIN = A4;
const int PSG2_BC1_PIN = A3;
const int PSG2_BDIR_PIN = A2;

void psg1_set_mode_inactive()
{
  digitalWrite(PSG1_BC1_PIN, LOW);
  digitalWrite(PSG1_BDIR_PIN, LOW);
}

void psg1_set_mode_latch()
{
  digitalWrite(PSG1_BC1_PIN, HIGH);
  digitalWrite(PSG1_BDIR_PIN, HIGH);
}

void psg1_set_mode_write()
{
  digitalWrite(PSG1_BC1_PIN, LOW);
  digitalWrite(PSG1_BDIR_PIN, HIGH);
}

void psg1_write_register(char reg, char value)
{
  psg1_set_mode_latch();  
  PORTD = reg;  
  psg1_set_mode_inactive();  
  psg1_set_mode_write();  
  PORTD = value;  
  psg1_set_mode_inactive();
}

void psg2_set_mode_inactive()
{
  digitalWrite(PSG2_BC1_PIN, LOW);
  digitalWrite(PSG2_BDIR_PIN, LOW);
}

void psg2_set_mode_latch()
{
  digitalWrite(PSG2_BC1_PIN, HIGH);
  digitalWrite(PSG2_BDIR_PIN, HIGH);
}

void psg2_set_mode_write()
{
  digitalWrite(PSG2_BC1_PIN, LOW);
  digitalWrite(PSG2_BDIR_PIN, HIGH);
}

void psg2_write_register(char reg, char value)
{
  psg2_set_mode_latch();  
  PORTD = reg;  
  psg2_set_mode_inactive();  
  psg2_set_mode_write();  
  PORTD = value;  
  psg2_set_mode_inactive();
}

File file;

// Open/re-open SNG file to be played
void reset_file()
{
  if (file)
  {
    file.close();
  }
  file = SD.open("output.sng");

  // Read header
  file.read();
  file.read();
  file.read();
  file.read();
  file.read();
}

void setup() {  
  // Set timer1 to produce 2 Mhz clock signal on pin 9
  TCCR1A = bit(COM1A0);
  TCCR1B = bit(WGM12) | bit(CS10);
  OCR1A = 3;
  pinMode(9, OUTPUT);

  cli();
  
  // Set timer2 to 2ms and Compare Interrupt
  TCCR2A = bit(WGM21);                
  TCCR2B = bit(CS22)| bit(CS21);      
  OCR2A = 0x7d;                       
  TIMSK2 = bit(OCIE2A);               
  TIFR2 |= bit(OCF2A);                
  TCNT2 = 0x00;                       

  pinMode(RESET_PIN, OUTPUT);
  pinMode(PSG1_BC1_PIN, OUTPUT);
  pinMode(PSG1_BDIR_PIN, OUTPUT);
  pinMode(PSG2_BC1_PIN, OUTPUT);
  pinMode(PSG2_BDIR_PIN, OUTPUT);

  DDRD = 0xff;
  PORTD = 0;

  // Reset AY-3-8910
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);

  psg1_set_mode_inactive();
  psg2_set_mode_inactive();

  SD.begin(SD_CARD_SELECT_PIN);
  reset_file();

  sei();
}

unsigned char song_buffer[256];                     // Song bytes buffer
unsigned char buffer_read = 0, buffer_write = 0;    // Tracking where the program is reading and writing in the buffer
bool buffer_filled = false;                         // Was the buffer ever filled

const int SONG_WAIT_REGISTER = 16;
const int SONG_END_OF_FILE_REGISTER = 255;

void loop() {
  if (file)
  {
    unsigned char reg = 0;
    unsigned char value = 0;

    // Constantly fill the song buffer until where the buffer is currently being read in the Timer 2 interrupt
    while (buffer_write != buffer_read || !buffer_filled)
    {
      reg = file.read();
      value = file.read();

      if (reg == SONG_END_OF_FILE_REGISTER)
      {
        reset_file();
      }
      else
      {
        song_buffer[buffer_write++] = reg;
        song_buffer[buffer_write++] = value;
      }

      buffer_filled = true;
    }
  }
}

int song_wait = 0;    // Wait for how many frames
int timer_count = 10; // Counter to adjust the interrupt code

// Timer 2 interrupt
ISR(TIMER2_COMPA_vect) {
  // Count to 10 (20ms = 2ms * 10) so that the code is executed every 20ms (50 Hz)
  if( timer_count > 0 )
  {
    timer_count--;
    return;
  }
  timer_count = 10;

  // If buffer hasn't been filled yet, don't do anything
  if (!buffer_filled)
  {
    return;
  }

  // Wait for *song_wait* frames
  if( song_wait > 0)
  {
    song_wait--;
    return;
  }

  unsigned char reg = 0;
  unsigned char value = 0;
  do
  {
    reg = song_buffer[buffer_read++];
    value = song_buffer[buffer_read++];

    if (reg == SONG_WAIT_REGISTER)
    {
      song_wait = value;
    }
    else
    {
      if (reg < 16)
      {
        psg1_write_register(reg, value);
      }
      else
      {
        psg2_write_register(reg & 0x0F, value);
      }
    }
  } while((reg & 0x7F) < 16);
}
