/* dtls -- a very basic DTLS implementation
 *
 * Copyright (C) 2011--2013 Olaf Bergmann <bergmann@tzi.org>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * @file dtls_time.c
 * @brief Clock Handling
 */

#include "types.h"
#include "dtls_config.h"
#include "dtls_time.h"
#include "dtls_time.h"

#ifdef WITH_CONTIKI
clock_time_t dtls_clock_offset;

void dtls_clock_init(void) {
  clock_init();
  dtls_clock_offset = clock_time();
}

void
dtls_ticks(dtls_tick_t *t) {
  *t = clock_time();
}

#elif defined WITH_ARDUINO

unsigned long (*get_current_time)();
time_t dtls_clock_offset;

void dtls_clock_init(unsigned long (*get_cur_time)(void)) {
  get_current_time = get_cur_time;
  dtls_clock_offset = (*get_current_time)();
}

void dtls_ticks(dtls_tick_t *t) {
    unsigned long x = (*get_current_time)();
    *t = x * DTLS_TICKS_PER_SECOND / 1000000;
}

void dtls_get_time(uint32_t *t) {
    *t = (*get_current_time)();
}

// TODO clock with arduino FIXME

#else /* WITH_CONTIKI */

time_t dtls_clock_offset;

void
dtls_clock_init(void) {
  dtls_clock_offset = time(NULL);

#  ifdef __GNUC__
  /* Issue a warning when using gcc. Other prepropressors do
   *  not seem to have a similar feature. */
#   warning "cannot initialize clock"
#  endif
  dtls_clock_offset = 0;
}

void dtls_ticks(dtls_tick_t *t) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  *t = (tv.tv_sec - dtls_clock_offset) * DTLS_TICKS_PER_SECOND
    + (tv.tv_usec * DTLS_TICKS_PER_SECOND / 1000000);
}

#endif /* WITH_CONTIKI */
