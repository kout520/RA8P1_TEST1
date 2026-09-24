/**
 * Command Dispatcher — routes voice commands to peripherals (non-blocking)
 *
 * dispatch_command():  called when voice_cmd_predict returns a valid class.
 *                      Starts background task (fingerprint, NFC, etc.).
 *
 * dispatch_poll():     call every main loop iteration.  Advances the
 *                      background state machine without blocking audio.
 */

#ifndef __COMMAND_DISPATCH_H__
#define __COMMAND_DISPATCH_H__

#include <stdint.h>

int  dispatch_command(int cmd_class);
void dispatch_poll(void);

#endif
