#include "hw_clock.h"


#define MAX_UINT32 0xFFFFFFFF
#define US_PER_SECOND 1000000
#define MR0_INT 0x01
#define MR1_INT 0x02
#define CR0_INT 0x10

static int timer_initialized = 0;
static uint64_t stored_ticks = 0;
static void (*trigger_task)(struct timeval *tv);
static struct TimedTask *task_list = NULL;

/***** functionized for readability *****/
static inline void enable_timed_task() {
  LPC_TIM2->MCR |= (0x1 << 3);
}

static inline void disable_timed_task() {
  LPC_TIM2->MCR &= ~(0x1 << 3);
}

static inline void set_task_interrupt(struct timeval *tv) {
  uint64_t time_us = tv->tv_sec;
  time_us = time_us * US_PER_SECOND + tv->tv_usec;
  LPC_TIM2->MR1 = (uint32_t) (time_us % MAX_UINT32);
}

static inline void handle_timer_overflow() {
  stored_ticks += (uint64_t) MAX_UINT32; // save overflowed 0xFFFFFFFF
}

static inline void run_timed_task() {
  struct TimedTask *task = pop_task_list(&task_list);
  task->task();
  free_timed_task(task);
  if (task_list) {
    set_task_interrupt(task_list->time);
  } else {
    disable_timed_task();
  }
}

static inline void run_trigger_task() {
  struct timeval tv;
  uint64_t ticks = stored_ticks + (uint64_t) LPC_TIM2->CR0;
  tv.tv_usec = (time_t) (ticks % US_PER_SECOND);
  tv.tv_sec = (time_t) (ticks / US_PER_SECOND);
  trigger_task(&tv);
}

static void timer2_interrupt_handler() {
  if (LPC_TIM2->IR & MR0_INT) {
    handle_timer_overflow();
    wait_us(1000); // magic to not trigger the interrupt twice or something
    LPC_TIM2->IR |= MR0_INT; // clear MR0 interrupt
  } else if (LPC_TIM2->IR & MR1_INT) {
    run_timed_task();
    wait_us(1000); // magic again
    LPC_TIM2->IR |= MR1_INT; // clear MR1 interrupt
  } else if (LPC_TIM2->IR & CR0_INT) {
    run_trigger_task();
    wait_us(1000); // more magic
    LPC_TIM2->IR |= CR0_INT; // clear CR0 interrupt
  }
}

static void init_hw_timer() {
  LPC_PINCON->PINSEL0 |= (0x3 << 8) | // set P0.4 (p30) to CAP2.0
                         (0x3 << 10); // set P0.5 (p29) to CAP2.1
  LPC_PINCON->PINMODE0 |= (0x1 << 8) | // set P0.4 to repeater mode
                          (0x3 << 10); // set P0.5 to pull down
  LPC_SC->PCONP |= (0x1 << 22); // power LPC_TIM2 on
  LPC_SC->PCLKSEL1 |= (0x1 << 12); // set PCLK_TIMER2 to CCLK
  LPC_TIM2->CTCR = 0x0; // set LPC_TIM2 to timer mode
  LPC_TIM2->TCR = 0x2; // reset LPC_TIM2
  LPC_TIM2->PR = SystemCoreClock / US_PER_SECOND - 1; // prescale makes TC tick per us
  LPC_TIM2->MR0 = MAX_UINT32; // interrupt when overflow
  LPC_TIM2->MCR |= (0x1 << 0) | // interrupt when MR0 matches
                   (0x1 << 1) | // reset TC when MR0 matches
                   (0x0 << 2);  // don't stop timer when MR0 matches
  LPC_TIM2->CCR |= (0x1 << 0) | // capture on rising edge
                   (0x1 << 1) | // capture on falling edge
                   (0x1 << 2);  // interrupt on capture
  NVIC_SetVector(TIMER2_IRQn, (uint32_t) &timer2_interrupt_handler);
  NVIC_EnableIRQ(TIMER2_IRQn);

  timer_initialized = 1;
  LPC_TIM2->TCR = 0x01; // start counting
}

void getTime(struct timeval *tv) {
  if (!timer_initialized) {
    init_hw_timer();
  }

  uint64_t ticks = stored_ticks + (uint64_t) LPC_TIM2->TC;
  tv->tv_usec = (time_t) (ticks % US_PER_SECOND);
  tv->tv_sec = (time_t) (ticks / US_PER_SECOND);
}

void runAtTime(void (*schedFunc)(void), struct timeval *tv) {
  struct timeval now;
  struct TimedTask *t;

  getTime(&now);
  if (is_time_earlier(tv, &now)) {
    // error, can't schedule earlier than now
    printf("ERROR: UNABLE TO SCHEDULE TASK DURING PAST TIME\n\r");
    return;
  }

  t = (struct TimedTask *) malloc(sizeof(struct TimedTask));
  t->task = schedFunc;
  t->time = (struct timeval *) malloc(sizeof(struct timeval));
  memcpy(t->time, tv, sizeof(struct timeval));
  t->next_task = NULL;
  t->prev_task = NULL;

  enable_timed_task();
  insert_timed_task(&task_list, t);
  set_task_interrupt(task_list->time);
}

void runAtTrigger(void (*trigFunc)(struct timeval *tv)) {
  trigger_task = trigFunc;
}
