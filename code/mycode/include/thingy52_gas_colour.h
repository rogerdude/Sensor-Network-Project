#ifndef _THINGY52_GAS_COLOUR_H_
#define _THINGY52_GAS_COLOUR_H_

#define SAMPLING_PERIOD 1000

#define ON 1
#define OFF 0

#define MAGENTA 0
#define BLUE    1
#define GREEN   2
#define CYAN    3
#define WHITE   4
#define YELLOW  5
#define RED     6

int thingy52_rgb_init(void);
void thingy52_rgb_colour_set(int colour);
void thread_gas_col_control(void);
void thread_gas_col_set(void);

#endif