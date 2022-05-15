// Interfacing the Arduino with two AY-3-8910 PSGs and playing tones in the channel A of each PSG

// Sérgio Vieira 2022

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

void setup() {
  // Set up Timer 1 to output a 2 MHZ clock signal in Pin 9
  TCCR1A = bit(COM1A0);
  TCCR1B = bit(WGM12) | bit(CS10);
  OCR1A = 3;
  pinMode(9, OUTPUT);

  pinMode(RESET_PIN, OUTPUT);
  pinMode(PSG1_BC1_PIN, OUTPUT);
  pinMode(PSG1_BDIR_PIN, OUTPUT);
  pinMode(PSG2_BC1_PIN, OUTPUT);
  pinMode(PSG2_BDIR_PIN, OUTPUT);

  // Set pins 0 to 7 to output
  DDRD = 0xFF;
  // Set pins 0 to 7 to output LOW
  PORTD = 0x00;

  psg1_set_mode_inactive();
  psg2_set_mode_inactive();

  // Reset the AY-3-8910
  digitalWrite(RESET_PIN, LOW);
  delay(1);
  digitalWrite(RESET_PIN, HIGH);

  // Enable only the Tone Generator on Channel A
  psg1_write_register(7, 0b00111110);
  psg2_write_register(7, 0b00111110);
  
  // Set the amplitude (volume) to maximum on Channel A
  psg1_write_register(8, 0b00001111);
  psg2_write_register(8, 0b00001111);
}

void loop() {
  // Change the Tone Period for Channel A every 500ms

  psg1_write_register(0, 223);
  psg1_write_register(1, 1);
  psg2_write_register(0, 253);
  psg2_write_register(1, 0);

  delay(500);
  
  psg1_write_register(0, 170);
  psg1_write_register(1, 1);
  psg2_write_register(0, 28);
  psg2_write_register(1, 1);

  delay(500);
  
  psg1_write_register(0, 123);
  psg1_write_register(1, 1);
  psg2_write_register(0, 63);
  psg2_write_register(1, 1);

  delay(500);
  
  psg1_write_register(0, 102);
  psg1_write_register(1, 1);
  psg2_write_register(0, 102);
  psg2_write_register(1, 1);

  delay(500);
  
  psg1_write_register(0, 63);
  psg1_write_register(1, 1);
  psg2_write_register(0, 123);
  psg2_write_register(1, 1);

  delay(500);
  
  psg1_write_register(0, 28);
  psg1_write_register(1, 1);
  psg2_write_register(0, 170);
  psg2_write_register(1, 1);

  delay(500);
  
  psg1_write_register(0, 253);
  psg1_write_register(1, 0);
  psg2_write_register(0, 223);
  psg2_write_register(1, 1);

  delay(500);
}
