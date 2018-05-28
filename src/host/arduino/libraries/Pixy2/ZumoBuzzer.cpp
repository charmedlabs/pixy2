#ifndef F_CPU
#define F_CPU 16000000UL  // Standard Arduinos run at 16 MHz
#endif //!F_CPU

#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "ZumoBuzzer.h"

#ifdef __AVR_ATmega32U4__

// PD7 (OC4D)
#define BUZZER_DDR  DDRD
#define BUZZER      (1 << PORTD7)

#define TIMER4_CLK_8  0x4 // 2 MHz

#define ENABLE_TIMER_INTERRUPT()   TIMSK4 = (1 << TOIE4)
#define DISABLE_TIMER_INTERRUPT()  TIMSK4 = 0

#else // 168P or 328P

// PD3 (OC2B)
#define BUZZER_DDR  DDRD
#define BUZZER      (1 << PORTD3)

#define TIMER2_CLK_32  0x3  // 500 kHz

static const unsigned int cs2_divider[] = {0, 1, 8, 32, 64, 128, 256, 1024};

#define ENABLE_TIMER_INTERRUPT()   TIMSK2 = (1 << TOIE2)
#define DISABLE_TIMER_INTERRUPT()  TIMSK2 = 0

#endif

unsigned char buzzerInitialized = 0;
volatile unsigned char buzzerFinished = 1;  // flag: 0 while playing
const char *buzzerSequence = 0;

// declaring these globals as static means they won't conflict
// with globals in other .cpp files that share the same name
static volatile unsigned int buzzerTimeout = 0;    // tracks buzzer time limit
static char play_mode_setting = PLAY_AUTOMATIC;

extern volatile unsigned char buzzerFinished;  // flag: 0 while playing
extern const char *buzzerSequence;


static unsigned char use_program_space; // boolean: true if we should
                    // use program space

// music settings and defaults
static unsigned char octave = 4;        // the current octave
static unsigned int whole_note_duration = 2000;  // the duration of a whole note
static unsigned int note_type = 4;              // 4 for quarter, etc
static unsigned int duration = 500;        // the duration of a note in ms
static unsigned int volume = 15;        // the note volume
static unsigned char staccato = 0;       // true if playing staccato

// staccato handling
static unsigned char staccato_rest_duration;  // duration of a staccato
                        //  rest, or zero if it is time
                        //  to play a note

static void nextNote();

#ifdef __AVR_ATmega32U4__

// Timer4 overflow interrupt
ISR (TIMER4_OVF_vect)
{
  if (buzzerTimeout-- == 0)
  {
    DISABLE_TIMER_INTERRUPT();
    sei();                                    // re-enable global interrupts (nextNote() is very slow)
    TCCR4B = (TCCR4B & 0xF0) | TIMER4_CLK_8;  // select IO clock
    unsigned int top = (F_CPU/16) / 1000;     // set TOP for freq = 1 kHz: 
    TC4H = top >> 8;                          // top 2 bits... (TC4H temporarily stores top 2 bits of 10-bit accesses)
    OCR4C = top;                              // and bottom 8 bits
    TC4H = 0;                                 // 0% duty cycle: top 2 bits...
    OCR4D = 0;                                // and bottom 8 bits
    buzzerFinished = 1;
    if (buzzerSequence && (play_mode_setting == PLAY_AUTOMATIC))
      nextNote();
  }
}

#else

// Timer2 overflow interrupt
ISR (TIMER2_OVF_vect)
{
  if (buzzerTimeout-- == 0)
  {
    DISABLE_TIMER_INTERRUPT();
    sei();                                    // re-enable global interrupts (nextNote() is very slow)
    TCCR2B = (TCCR2B & 0xF8) | TIMER2_CLK_32; // select IO clock
    OCR2A = (F_CPU/64) / 1000;                // set TOP for freq = 1 kHz
    OCR2B = 0;                                // 0% duty cycle
    buzzerFinished = 1;
    if (buzzerSequence && (play_mode_setting == PLAY_AUTOMATIC))
      nextNote();
  }
}

#endif


// constructor

ZumoBuzzer::ZumoBuzzer()
{
}

// this is called by playFrequency()
inline void ZumoBuzzer::init()
{
  if (!buzzerInitialized)
  {
    buzzerInitialized = 1;
    init2();
  }
}

// initializes timer4 (32U4) or timer2 (328P) for buzzer control
void ZumoBuzzer::init2()
{
  DISABLE_TIMER_INTERRUPT();
  
#ifdef __AVR_ATmega32U4__
  TCCR4A = 0x00;  // bits 7 and 6 clear: normal port op., OC4A disconnected
                  // bits 5 and 4 clear: normal port op., OC4B disconnected
                  // bit 3 clear: no force output compare for channel A
                  // bit 2 clear: no force output compare for channel B
                  // bit 1 clear: disable PWM for channel A
                  // bit 0 clear: disable PWM for channel B
  
  TCCR4B = 0x04;  // bit 7 clear: disable PWM inversion
                  // bit 6 clear: no prescaler reset
                  // bits 5 and 4 clear: dead time prescaler 1
                  // bit 3 clear, 2 set, 1-0 clear: timer clock = CK/8
  
  TCCR4C = 0x09;  // bits 7 and 6 clear: normal port op., OC4A disconnected
                  // bits 5 and 4 clear: normal port op., OC4B disconnected
                  // bit 3 set, 2 clear: clear OC4D on comp match when upcounting, 
                  //                     set OC4D on comp match when downcounting
                  // bit 1 clear: no force output compare for channel D
                  // bit 0 set: enable PWM for channel 4

  TCCR4D = 0x01;  // bit 7 clear: disable fault protection interrupt
                  // bit 6 clear: disable fault protection mode
                  // bit 5 clear: disable fault protection noise canceler
                  // bit 4 clear: falling edge triggers fault
                  // bit 3 clear: disable fault protection analog comparator
                  // bit 2 clear: fault protection interrupt flag
                  // bit 1 clear, 0 set: select waveform generation mode,
                  //    phase- and frequency-correct PWM, TOP = OCR4C,
                  //    OCR4D set at BOTTOM, TOV4 flag set at BOTTOM

  // This sets timer 4 to run in phase- and frequency-correct PWM mode,
  //    where TOP = OCR4C, OCR4D is updated at BOTTOM, TOV1 Flag is set on BOTTOM.
  //    OC4D is cleared on compare match when upcounting, set on compare
  //    match when downcounting; OC4A and OC4B are disconnected.
  
  unsigned int top = (F_CPU/16) / 1000; // set TOP for freq = 1 kHz: 
  TC4H = top >> 8;                      // top 2 bits...
  OCR4C = top;                          // and bottom 8 bits
  TC4H = 0;                             // 0% duty cycle: top 2 bits...
  OCR4D = 0;                            // and bottom 8 bits
#else
  TCCR2A = 0x21;  // bits 7 and 6 clear: normal port op., OC4A disconnected
                  // bit 5 set, 4 clear: clear OC2B on comp match when upcounting, 
                  //                     set OC2B on comp match when downcounting
                  // bits 3 and 2: not used
                  // bit 1 clear, 0 set: combine with bit 3 of TCCR2B...
                  
  TCCR2B = 0x0B;  // bit 7 clear: no force output compare for channel A
                  // bit 6 clear: no force output compare for channel B
                  // bits 5 and 4: not used
                  // bit 3 set: combine with bits 1 and 0 of TCCR2A to
                  //    select waveform generation mode 5, phase-correct PWM,
                  //    TOP = OCR2A, OCR2B set at TOP, TOV2 flag set at BOTTOM
                  // bit 2 clear, 1-0 set: timer clock = clkT2S/32
                  
  // This sets timer 2 to run in phase-correct PWM mode, where TOP = OCR2A,
  //    OCR2B is updated at TOP, TOV2 Flag is set on BOTTOM. OC2B is cleared
  //    on compare match when upcounting, set on compare match when downcounting;
  //    OC2A is disconnected.
  // Note: if the PWM frequency and duty cycle are changed, the first
  //    cycle of the new frequency will be at the old duty cycle, since
  //    the duty cycle (OCR2B) is not updated until TOP.
  

  OCR2A = (F_CPU/64) / 1000;  // set TOP for freq = 1 kHz
  OCR2B = 0;                  // 0% duty cycle
#endif
  
  BUZZER_DDR |= BUZZER;    // buzzer pin set as an output
  sei();
}


// Set up timer 1 to play the desired frequency (in Hz or .1 Hz) for the
//   the desired duration (in ms). Allowed frequencies are 40 Hz to 10 kHz.
//   volume controls buzzer volume, with 15 being loudest and 0 being quietest.
// Note: frequency*duration/1000 must be less than 0xFFFF (65535).  This
//   means that you can't use a max duration of 65535 ms for frequencies
//   greater than 1 kHz.  For example, the max duration you can use for a
//   frequency of 10 kHz is 6553 ms.  If you use a duration longer than this,
//   you will cause an integer overflow that produces unexpected behavior.
void ZumoBuzzer::playFrequency(unsigned int freq, unsigned int dur, 
                     unsigned char volume)
{
  init(); // initializes the buzzer if necessary
  buzzerFinished = 0;
  
  unsigned int timeout;
  unsigned char multiplier = 1;
  
  if (freq & DIV_BY_10) // if frequency's DIV_BY_10 bit is set
  {                     //  then the true frequency is freq/10
    multiplier = 10;    //  (gives higher resolution for small freqs)
    freq &= ~DIV_BY_10; // clear DIV_BY_10 bit
  }

  unsigned char min = 40 * multiplier;
  if (freq < min) // min frequency allowed is 40 Hz
    freq = min;  
  if (multiplier == 1 && freq > 10000)
    freq = 10000;      // max frequency allowed is 10kHz

#ifdef __AVR_ATmega32U4__
  unsigned long top;
  unsigned char dividerExponent = 0;
  
  // calculate necessary clock source and counter top value to get freq
  top = (unsigned int)(((F_CPU/2 * multiplier) + (freq >> 1))/ freq);
  
  while (top > 1023)
  {
    dividerExponent++;
    top = (unsigned int)((((F_CPU/2 >> (dividerExponent)) * multiplier) + (freq >> 1))/ freq);
  }
#else   
  unsigned int top;
  unsigned char newCS2 = 2; // try prescaler divider of 8 first (minimum necessary for 10 kHz)
  unsigned int divider = cs2_divider[newCS2];

  // calculate necessary clock source and counter top value to get freq
  top = (unsigned int)(((F_CPU/16 * multiplier) + (freq >> 1))/ freq);
  
  while (top > 255)
  {
    divider = cs2_divider[++newCS2];
    top = (unsigned int)(((F_CPU/2/divider * multiplier) + (freq >> 1))/ freq);
  }
#endif

  // set timeout (duration):
  if (multiplier == 10)
    freq = (freq + 5) / 10;

  if (freq == 1000)
    timeout = dur;  // duration for silent notes is exact
  else
    timeout = (unsigned int)((long)dur * freq / 1000);
  
  if (volume > 15)
    volume = 15;

  DISABLE_TIMER_INTERRUPT();      // disable interrupts while writing to registers 
  
#ifdef __AVR_ATmega32U4__
  TCCR4B = (TCCR4B & 0xF0) | (dividerExponent + 1); // select timer 4 clock prescaler: divider = 2^n if CS4 = n+1
  TC4H = top >> 8;                                  // set timer 1 pwm frequency: top 2 bits...
  OCR4C = top;                                      // and bottom 8 bits
  unsigned int width = top >> (16 - volume);        // set duty cycle (volume):
  TC4H = width >> 8;                                // top 2 bits...
  OCR4D = width;                                    // and bottom 8 bits
  buzzerTimeout = timeout;                          // set buzzer duration
  
  TIFR4 |= 0xFF;  // clear any pending t4 overflow int.      
#else
  TCCR2B = (TCCR2B & 0xF8) | newCS2;  // select timer 2 clock prescaler
  OCR2A = top;                        // set timer 2 pwm frequency
  OCR2B = top >> (16 - volume);       // set duty cycle (volume)
  buzzerTimeout = timeout;            // set buzzer duration

  TIFR2 |= 0xFF;  // clear any pending t2 overflow int.     
#endif

  ENABLE_TIMER_INTERRUPT();  
}



// Determine the frequency for the specified note, then play that note
//  for the desired duration (in ms).  This is done without using floats
//  and without having to loop.  volume controls buzzer volume, with 15 being
//  loudest and 0 being quietest.
// Note: frequency*duration/1000 must be less than 0xFFFF (65535).  This
//  means that you can't use a max duration of 65535 ms for frequencies
//  greater than 1 kHz.  For example, the max duration you can use for a
//  frequency of 10 kHz is 6553 ms.  If you use a duration longer than this,
//  you will cause an integer overflow that produces unexpected behavior.
void ZumoBuzzer::playNote(unsigned char note, unsigned int dur,
                 unsigned char volume)
{
  // note = key + octave * 12, where 0 <= key < 12
  // example: A4 = A + 4 * 12, where A = 9 (so A4 = 57)
  // A note is converted to a frequency by the formula:
  //   Freq(n) = Freq(0) * a^n
  // where
  //   Freq(0) is chosen as A4, which is 440 Hz
  // and
  //   a = 2 ^ (1/12)
  // n is the number of notes you are away from A4.
  // One can see that the frequency will double every 12 notes.
  // This function exploits this property by defining the frequencies of the
  // 12 lowest notes allowed and then doubling the appropriate frequency
  // the appropriate number of times to get the frequency for the specified
  // note.

  // if note = 16, freq = 41.2 Hz (E1 - lower limit as freq must be >40 Hz)
  // if note = 57, freq = 440 Hz (A4 - central value of ET Scale)
  // if note = 111, freq = 9.96 kHz (D#9 - upper limit, freq must be <10 kHz)
  // if note = 255, freq = 1 kHz and buzzer is silent (silent note)

  // The most significant bit of freq is the "divide by 10" bit.  If set,
  // the units for frequency are .1 Hz, not Hz, and freq must be divided
  // by 10 to get the true frequency in Hz.  This allows for an extra digit
  // of resolution for low frequencies without the need for using floats.

  unsigned int freq = 0;
  unsigned char offset_note = note - 16;

  if (note == SILENT_NOTE || volume == 0)
  {
    freq = 1000;  // silent notes => use 1kHz freq (for cycle counter)
    playFrequency(freq, dur, 0);
    return;
  }

  if (note <= 16)
    offset_note = 0;
  else if (offset_note > 95)
    offset_note = 95;

  unsigned char exponent = offset_note / 12;

  // frequency table for the lowest 12 allowed notes
  //   frequencies are specified in tenths of a Hertz for added resolution
  switch (offset_note - exponent * 12)  // equivalent to (offset_note % 12)
  {
    case 0:        // note E1 = 41.2 Hz
      freq = 412;
      break;
    case 1:        // note F1 = 43.7 Hz
      freq = 437;
      break;
    case 2:        // note F#1 = 46.3 Hz
      freq = 463;
      break;
    case 3:        // note G1 = 49.0 Hz
      freq = 490;
      break;
    case 4:        // note G#1 = 51.9 Hz
      freq = 519;
      break;
    case 5:        // note A1 = 55.0 Hz
      freq = 550;
      break;
    case 6:        // note A#1 = 58.3 Hz
      freq = 583;
      break;
    case 7:        // note B1 = 61.7 Hz
      freq = 617;
      break;
    case 8:        // note C2 = 65.4 Hz
      freq = 654;
      break;
    case 9:        // note C#2 = 69.3 Hz
      freq = 693;
      break;
    case 10:      // note D2 = 73.4 Hz
      freq = 734;
      break;
    case 11:      // note D#2 = 77.8 Hz
      freq = 778;
      break;
  }

  if (exponent < 7)
  {
    freq = freq << exponent;  // frequency *= 2 ^ exponent
    if (exponent > 1)      // if the frequency is greater than 160 Hz
      freq = (freq + 5) / 10;  //   we don't need the extra resolution
    else
      freq += DIV_BY_10;    // else keep the added digit of resolution
  }
  else
    freq = (freq * 64 + 2) / 5;  // == freq * 2^7 / 10 without int overflow

  if (volume > 15)
    volume = 15;
  playFrequency(freq, dur, volume);  // set buzzer this freq/duration
}



// Returns 1 if the buzzer is currently playing, otherwise it returns 0
unsigned char ZumoBuzzer::isPlaying()
{
  return !buzzerFinished || buzzerSequence != 0;
}


// Plays the specified sequence of notes.  If the play mode is 
// PLAY_AUTOMATIC, the sequence of notes will play with no further
// action required by the user.  If the play mode is PLAY_CHECK,
// the user will need to call playCheck() in the main loop to initiate
// the playing of each new note in the sequence.  The play mode can
// be changed while the sequence is playing.  
// This is modeled after the PLAY commands in GW-BASIC, with just a
// few differences.
//
// The notes are specified by the characters C, D, E, F, G, A, and
// B, and they are played by default as "quarter notes" with a
// length of 500 ms.  This corresponds to a tempo of 120
// beats/min.  Other durations can be specified by putting a number
// immediately after the note.  For example, C8 specifies C played as an
// eighth note, with half the duration of a quarter note. The special
// note R plays a rest (no sound).
//
// Various control characters alter the sound:
//   '>' plays the next note one octave higher
//
//   '<' plays the next note one octave lower
//
//   '+' or '#' after a note raises any note one half-step
//
//   '-' after a note lowers any note one half-step
//
//   '.' after a note "dots" it, increasing the length by
//       50%.  Each additional dot adds half as much as the
//       previous dot, so that "A.." is 1.75 times the length of
//       "A".
//
//   'O' followed by a number sets the octave (default: O4).
//
//   'T' followed by a number sets the tempo (default: T120).
//
//   'L' followed by a number sets the default note duration to
//       the type specified by the number: 4 for quarter notes, 8
//       for eighth notes, 16 for sixteenth notes, etc.
//       (default: L4)
//
//   'V' followed by a number from 0-15 sets the music volume.
//       (default: V15)
//
//   'MS' sets all subsequent notes to play staccato - each note is played
//       for 1/2 of its allotted time, followed by an equal period
//       of silence.
//
//   'ML' sets all subsequent notes to play legato - each note is played
//       for its full length.  This is the default setting.
//
//   '!' resets all persistent settings to their defaults.
//
// The following plays a c major scale up and back down:
//   play("L16 V8 cdefgab>cbagfedc");
//
// Here is an example from Bach:
//   play("T240 L8 a gafaeada c+adaeafa <aa<bac#ada c#adaeaf4");
void ZumoBuzzer::play(const char *notes)
{
  DISABLE_TIMER_INTERRUPT();  // prevent this from being interrupted
  buzzerSequence = notes;
  use_program_space = 0;
  staccato_rest_duration = 0;
  nextNote();          // this re-enables the timer1 interrupt
}

void ZumoBuzzer::playFromProgramSpace(const char *notes_p)
{
  DISABLE_TIMER_INTERRUPT();  // prevent this from being interrupted
  buzzerSequence = notes_p;
  use_program_space = 1;
  staccato_rest_duration = 0;
  nextNote();          // this re-enables the timer1 interrupt
}


// stop all sound playback immediately
void ZumoBuzzer::stopPlaying()
{
  DISABLE_TIMER_INTERRUPT();          // disable interrupts
 
#ifdef __AVR_ATmega32U4__
  TCCR4B = (TCCR4B & 0xF0) | TIMER4_CLK_8;  // select IO clock
  unsigned int top = (F_CPU/16) / 1000;     // set TOP for freq = 1 kHz: 
  TC4H = top >> 8;                          // top 2 bits... (TC4H temporarily stores top 2 bits of 10-bit accesses)
  OCR4C = top;                              // and bottom 8 bits
  TC4H = 0;                                 // 0% duty cycle: top 2 bits...
  OCR4D = 0;                                // and bottom 8 bits
#else 
  TCCR2B = (TCCR2B & 0xF8) | TIMER2_CLK_32; // select IO clock
  OCR2A = (F_CPU/64) / 1000;                // set TOP for freq = 1 kHz
  OCR2B = 0;                                // 0% duty cycle
#endif

  buzzerFinished = 1;
  buzzerSequence = 0;
}

// Gets the current character, converting to lower-case and skipping
// spaces.  For any spaces, this automatically increments sequence!
static char currentCharacter()
{
  char c = 0;
  do
  {
    if(use_program_space)
      c = pgm_read_byte(buzzerSequence);
    else
      c = *buzzerSequence;

    if(c >= 'A' && c <= 'Z')
      c += 'a'-'A';
  } while(c == ' ' && (buzzerSequence ++));

  return c;
}

// Returns the numerical argument specified at buzzerSequence[0] and
// increments sequence to point to the character immediately after the
// argument.
static unsigned int getNumber()
{
  unsigned int arg = 0;

  // read all digits, one at a time
  char c = currentCharacter();
  while(c >= '0' && c <= '9')
  {
    arg *= 10;
    arg += c-'0';
    buzzerSequence ++;
    c = currentCharacter();
  }

  return arg;
}

static void nextNote()
{
  unsigned char note = 0;
  unsigned char rest = 0;
  unsigned char tmp_octave = octave; // the octave for this note
  unsigned int tmp_duration; // the duration of this note
  unsigned int dot_add;

  char c; // temporary variable

  // if we are playing staccato, after every note we play a rest
  if(staccato && staccato_rest_duration)
  {
    ZumoBuzzer::playNote(SILENT_NOTE, staccato_rest_duration, 0);
    staccato_rest_duration = 0;
    return;
  }

 parse_character:

  // Get current character
  c = currentCharacter();
  buzzerSequence ++;

  // Interpret the character.
  switch(c)
  {
  case '>':
    // shift the octave temporarily up
    tmp_octave ++;
    goto parse_character;
  case '<':
    // shift the octave temporarily down
    tmp_octave --;
    goto parse_character;
  case 'a':
    note = NOTE_A(0);
    break;
  case 'b':
    note = NOTE_B(0);
    break;
  case 'c':
    note = NOTE_C(0);
    break;
  case 'd':
    note = NOTE_D(0);
    break;
  case 'e':
    note = NOTE_E(0);
    break;
  case 'f':
    note = NOTE_F(0);
    break;
  case 'g':
    note = NOTE_G(0);
    break;
  case 'l':
    // set the default note duration
    note_type = getNumber();
    duration = whole_note_duration/note_type;
    goto parse_character;
  case 'm':
    // set music staccato or legato
    if(currentCharacter() == 'l')
      staccato = false;
    else
    {
      staccato = true;
      staccato_rest_duration = 0;
    }
    buzzerSequence ++;
    goto parse_character;
  case 'o':
    // set the octave permanently
    octave = getNumber();
    tmp_octave = octave;
    goto parse_character;
  case 'r':
    // Rest - the note value doesn't matter.
    rest = 1;
    break;
  case 't':
    // set the tempo
    whole_note_duration = 60*400/getNumber()*10;
    duration = whole_note_duration/note_type;
    goto parse_character;
  case 'v':
    // set the volume
    volume = getNumber();
    goto parse_character;
  case '!':
    // reset to defaults
    octave = 4;
    whole_note_duration = 2000;
    note_type = 4;
    duration = 500;
    volume = 15;
    staccato = 0;
    // reset temp variables that depend on the defaults
    tmp_octave = octave;
    tmp_duration = duration;
    goto parse_character;
  default:
    buzzerSequence = 0;
    return;
  }

  note += tmp_octave*12;

  // handle sharps and flats
  c = currentCharacter();
  while(c == '+' || c == '#')
  {
    buzzerSequence ++;
    note ++;
    c = currentCharacter();
  }
  while(c == '-')
  {
    buzzerSequence ++;
    note --;
    c = currentCharacter();
  }

  // set the duration of just this note
  tmp_duration = duration;

  // If the input is 'c16', make it a 16th note, etc.
  if(c > '0' && c < '9')
    tmp_duration = whole_note_duration/getNumber();

  // Handle dotted notes - the first dot adds 50%, and each
  // additional dot adds 50% of the previous dot.
  dot_add = tmp_duration/2;
  while(currentCharacter() == '.')
  {
    buzzerSequence ++;
    tmp_duration += dot_add;
    dot_add /= 2;
  }

  if(staccato)
  {
    staccato_rest_duration = tmp_duration / 2;
    tmp_duration -= staccato_rest_duration;
  }
  
  // this will re-enable the timer1 overflow interrupt
  ZumoBuzzer::playNote(rest ? SILENT_NOTE : note, tmp_duration, volume);
}


// This puts play() into a mode where instead of advancing to the
// next note in the sequence automatically, it waits until the
// function playCheck() is called. The idea is that you can
// put playCheck() in your main loop and avoid potential
// delays due to the note sequence being checked in the middle of
// a time sensitive calculation.  It is recommended that you use
// this function if you are doing anything that can't tolerate
// being interrupted for more than a few microseconds.
// Note that the play mode can be changed while a sequence is being
// played.
//
// Usage: playMode(PLAY_AUTOMATIC) makes it automatic (the
// default), playMode(PLAY_CHECK) sets it to a mode where you have
// to call playCheck().
void ZumoBuzzer::playMode(unsigned char mode)
{
  play_mode_setting = mode;

  // We want to check to make sure that we didn't miss a note if we
  // are going out of play-check mode.
  if(mode == PLAY_AUTOMATIC)
    playCheck();
}


// Checks whether it is time to start another note, and starts
// it if so.  If it is not yet time to start the next note, this method
// returns without doing anything.  Call this as often as possible 
// in your main loop to avoid delays between notes in the sequence.
//
// Returns true if it is still playing.
unsigned char ZumoBuzzer::playCheck()
{
  if(buzzerFinished && buzzerSequence != 0)
    nextNote();
  return buzzerSequence != 0;
}
