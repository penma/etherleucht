#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>

#define TASTER_STEP_MS 10
#define TASTER_STEP_OCRA ((F_CPU/1024 * TASTER_STEP_MS) / 1000 - 1)
#if TASTER_STEP_OCRA > 255
#error "Eingestelltes Taster-Abfrage-Intervall kann mit diesem Timer-Prescaler nicht erzeugt werden"
#endif
#if TASTER_STEP_OCRA < 16
#warning "OCR1A für eingestelltes Taster-Intervall ist niedrig und möglicherweise ungenau"
#endif

#define TABLE_TOP 209
#define TABLE_BOT 0

/*
static const __flash uint8_t ramp[TABLE_TOP+1] = {
21,21,21,21,22,22,22,23,23,23,23,24,24,24,25,25,25,26,26,26,27,27,27,28,28,28,29,29,30,30,30,31,31,32,32,32,33,33,34,34,35,35,35,36,36,37,37,38,38,39,39,40,40,41,41,42,42,43,43,44,44,45,45,46,47,47,48,48,49,50,50,51,51,52,53,53,54,55,55,56,57,58,58,59,60,60,61,62,63,63,64,65,66,67,67,68,69,70,71,72,73,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,94,95,96,97,98,99,101,102,103,104,106,107,108,109,111,112,113,115,116,118,119,120,122,123,125,126,128,129,131,133,134,136,137,139,141,142,144,146,148,149,151,153,155,157,159,161,162,164,166,168,170,172,174,177,179,181,183,185,187,190,192,194,196,199,201,204,206,208,211,213,216,219,221,224,227,229,232,235,238,240,243,246,249,250,251,252,253,254,255
};

static uint8_t brightness = TABLE_TOP;
static int enabled = 1;

static void update_light() {
	if (enabled) {
		TCCR0A |= (1 << COM0A1) | (1 << COM0B1);
		OCR0A = OCR0B = ramp[brightness];
	} else {
		TCCR0A &= ~((1 << COM0A1) | (1 << COM0B1));
	}
}
*/

/* in ms */
#define DIMMER_TAST_MIN (50/TASTER_STEP_MS)
#define DIMMER_START_DELAY (500/TASTER_STEP_MS)
#define DIMMER_DIRECTION_DELAY (1000/TASTER_STEP_MS)

#if 0
ISR (TIMER1_COMPA_vect) {
	static uint16_t hold_step = 0;
	static uint16_t save_step;

	if (PINB & (1 << PB3)) {
		save_step = hold_step;
		hold_step = 0;
	} else {
		if (hold_step != 0xffff) {
			hold_step++;
		}
	}

	static int dir = 0;
	static int wait = 0;

	if (hold_step == DIMMER_START_DELAY) {
		/* nach wartezeit sofort anfangen zu dimmen
		 * (auch dann, wenn man bereits am anschlag ist. deswegen der
		 * sonderfall)
		 */
		wait = 0;
		if (brightness == TABLE_TOP) {
			dir = 0;
		} else if (brightness == TABLE_BOT) {
			dir = 1;
		} else {
			dir = !dir;
		}
	} else if (hold_step > DIMMER_START_DELAY) {
		enabled = 1;
		if (!wait) {
			if (dir) {
				brightness++;
				if (brightness == TABLE_TOP) {
					wait = 1;
					save_step = hold_step;
				}
			} else {
				brightness--;
				if (brightness == TABLE_BOT) {
					wait = 1;
					dir = !dir;
					save_step = hold_step;
				}
			}
		} else if (wait) {
			if (hold_step > save_step + DIMMER_DIRECTION_DELAY) {
				wait = 0;
				if (brightness == TABLE_TOP) {
					dir = 0;
				} else {
					dir = 1;
				}
			}
		}
		update_light();
	} else if (hold_step == 0 && save_step > DIMMER_TAST_MIN && save_step <= DIMMER_START_DELAY) {
		/* schnell genug wieder losgelassen -> taster. licht togglen. */
		enabled = !enabled;
		update_light();
	}
}
#endif

int main (void) {
	enc28j60Init();

	while (1) {
		PORTD |= (1 << PD2);
		enc28j60PacketSend(46, "\xff\xff\xff\xff\xff\xff\x42\xcc\xcd\x84\xf4\x7c\x08\x00\x45\x00\x00\x20\x23\x42\x00\x00\xff\x11\x17\x31\xc0\xa8\x00\x02\xc0\xa8\x00\x07\x90\x01\x09\x26\x00\x0c\x00\x00\x7a\x6f\x6d\x67");
		_delay_ms(100);
		PORTD &= ~(1 << PD2);
		_delay_ms(900);

	}

//	init();
	DDRD |= (1 << PD5);
	DDRB |= (1 << PB2);

	/* fast pwm, 8-bit, toggle OC0A/B, prescaler 1 */
	TCCR0A = (1 << COM0A1) | (1 << COM0B1) | (1 << WGM01) | (1 << WGM00);
	TCCR0B = (1 << CS00);
	OCR0A = OCR0B = 0xff;

	/* tasterfoo */
	DDRB &= ~(1 << PB3);
	PORTB |= (1 << PB3);

	TCCR1A = 0;
	TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); /* prescaler 1/1024, CTC */
	OCR1A = TASTER_STEP_OCRA;
	TIMSK |= 1 << OCIE1A;
	sei();

	while (1) {
	}
}

