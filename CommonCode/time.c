/*
AcessOS 0.2 Basic
Time Handler
*/
#include <acess.h>

//Constants					Jan Feb  Mar Apr  May  Jun   Jul   Aug  Sept  Oct  Nov   Dec
const short DAYS_BEFORE[] = {0, 31, 59, 90,120, 151, 181, 212, 243, 273, 304, 334};
#define UNIX_TO_2K	((30*365*3600*24) + (7*3600*24))	//Normal years + leap years
#define TIMER_COUNT	8

//Imports
extern void readRTC(char *years, char *month, char *day, char *hrs, char *min, char *sec);

// === PROTOTYPES ===
void updateTimestamp();

//=== Structures ===
typedef struct {
	int	type;
	int	tickCount;
	Uint32	tickEnd;
	int	arg;
	void (*callback)(int arg);
} timer_t;

//Globals
volatile Uint	time_ticks = 0;
Uint	time_msFract = 0;
Uint64	time_msWhole = 0;
Uint64	time_accurate = 0;
timer_t	timers[TIMER_COUNT];

//Code
/* Timer IRQ Handler
 * Fires at ~10kHz
 */
void time_handler(struct regs *r)
{
	int	i;
	
	// Increment Ticks
    time_ticks ++;
	
	//puts("Tock\n");

	// Calculate Miliseconds
	time_msFract += MS_PER_TICK_FRACT;
	if(time_msFract < MS_PER_TICK_FRACT)	// Detect Wrapping
		time_msWhole ++;
	time_msWhole += MS_PER_TICK_WHOLE;
	
	// Calculate new seconds
	if(time_msWhole > 1000) {
		time_accurate ++;
		time_msWhole -= 1000;
	}
	
	for( i = TIMER_COUNT; i--; )
	{
		if(timers[i].tickEnd == 0)
			continue;
		if(timers[i].tickEnd > time_ticks)
			continue;
		
		switch(timers[i].type) {
		case 0:
			break;
		case 1:	//One Shot
			timers[i].type = 0;
			timers[i].tickEnd = 0;
			timers[i].callback(timers[i].arg);
			break;
		case 2:	//Periodical
			timers[i].callback(timers[i].arg);
			timers[i].tickEnd = time_ticks + timers[i].tickCount;
			break;
		}
	}
}

/*
int time_setTimer(int delay, void(*callback)(int), int arg)
- Create a new timer
*/
int time_createTimer(int delay, void(*callback)(int), int arg, int oneshot)
{
	int i;
	for( i = TIMER_COUNT; i--; )
	{
		if(timers[i].type == 0) {
			timers[i].type = (oneshot?1:2);
			timers[i].tickCount = (int) (delay * TICKS_PER_MS);
			timers[i].tickEnd = time_ticks + timers[i].tickCount;
			timers[i].callback = callback;
			timers[i].arg = arg;
			return i;
		}
	}
	return -1;
}

/*
void time_removeTimer(int id)
- Remove a timer
*/
void time_removeTimer(int id)
{
	if(0 > id || id >= TIMER_COUNT)
		return;
	timers[id].type = 0;
	timers[id].tickEnd = 0;
}

/* This will continuously loop until the given time has
 * been reached.
 */
void time_wait(int ticks)
{
    unsigned long eticks;

    eticks = time_ticks + ticks;
    while(time_ticks < eticks) __asm__ ("hlt");
}

void Timer_Disable()
{
	irq_uninstall_handler(0);
}

/* Sets up the system clock by installing the timer handler
 * into IRQ0
 */
void time_install()
{
	//Install Handler (AcessOS will eventually call it from proc.c)
	irq_install_handler(0, time_handler);
	// Update Timestamp
//	updateTimestamp();
	// Set Timer Settings
	outportb(0x43, 0x34);	//Set Channel 0, Low/High, Rate Generator
	outportb(0x40, TIMER_DIVISOR&0xFF);	//Low Byte of Divisor
	outportb(0x40, (TIMER_DIVISOR>>8)&0xFF);	//High Byte
	// Start Interrupts
	__asm__ __volatile__ ("sti");
}

/* Update timestamp from RTC
 */
void updateTimestamp()
{
	char	yrs, mon, day;
	char	hr, min, sec;
	readRTC(&yrs, &mon, &day, &hr, &min, &sec);
	time_accurate = timestamp(sec, min, hr, day, mon, yrs);
}

/* Returns the current UNIX timestamp
 */
int now()
{
	return time_accurate;
}

/**
 * \fn Uint64 unow()
 * \brief Returns the 64-Bit count of miliseconds since the unix epoch
 */
Uint64 unow()
{
	//return time_accurate * 1000 + time_msWhole % 1000;
	return time_msWhole;
}

/* Converts a date into a Unix Timestamp
 */
int timestamp(int sec, int mins, int hrs, int day, int month, int year)
{
	int stamp;
	stamp = sec;
	stamp += mins*60;
	stamp += hrs*3600;
	stamp += day*3600*24;
	stamp += month*DAYS_BEFORE[month]*3600*24;
	if(	(
		((year&3) == 0 || year%100 != 0)
		|| (year%100 == 0 && ((year/100)&3) == 0)
		) && month > 1)	//Leap yr and after feb
		stamp += 3600*24;
	stamp += ((365*4+1)*((year-2000)&~3))*3600*24;
	stamp += ((year-2000)&3)*265*3600*24;
	stamp += UNIX_TO_2K;
	return stamp;
}
