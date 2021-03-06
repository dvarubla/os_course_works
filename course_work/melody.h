#define QUARTER 400 
#define SHORT_WAIT (QUARTER/10)

#define WAIT_FREQ 0
#define C4_FREQ 261.63
#define DB4_FREQ 277.18
#define D4_FREQ 293.66
#define EB4_FREQ 311.13
#define E4_FREQ 329.63
#define F4_FREQ 349.23
#define GB4_FREQ 369.99
#define G4_FREQ 392
#define AB4_FREQ 415.3
#define A4_FREQ 440
#define BB4_FREQ 466.16
#define B4_FREQ 493.88

static const Note notes[]={
	{GB4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{G4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},

	{BB4_FREQ, QUARTER-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},
	{AB4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{BB4_FREQ, QUARTER/2},
	{WAIT_FREQ, QUARTER*1/4},
	{AB4_FREQ, QUARTER*5/4},

	{WAIT_FREQ, QUARTER*1/4},
	{D4_FREQ, QUARTER*7/4-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},
	{EB4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{F4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	
	{AB4_FREQ, QUARTER-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},
	{G4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{AB4_FREQ, QUARTER/2-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},
	{G4_FREQ, QUARTER},
	{WAIT_FREQ, QUARTER/2},

	{C4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{C4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{D4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{EB4_FREQ, QUARTER/2},
	{WAIT_FREQ, QUARTER/2},

	{G4_FREQ, QUARTER*6/4-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},
	{F4_FREQ, QUARTER*10/4-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},

	{EB4_FREQ, QUARTER-SHORT_WAIT},
	{WAIT_FREQ, SHORT_WAIT},
	{D4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{EB4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},
	{F4_FREQ, QUARTER*3/4},
	{WAIT_FREQ, QUARTER*1/4},

	{G4_FREQ, QUARTER*16/4},
	{WAIT_FREQ, QUARTER*2/4}
};

static const uint8_t num_notes=sizeof(notes)/sizeof(Note);

