/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef FNX_FRAME_PACER_H
#define FNX_FRAME_PACER_H
#include <stdint.h>
namespace fnx {
/* El reloj es monotónico. Si un marco llega más de un período tarde,
   se descarta la deuda: el juego nunca ejecuta una ráfaga de recuperación. */
struct FrameDeadline {
    uint64_t next = 0;
    void reset() { next = 0; }
    uint64_t advance(uint64_t now, uint64_t period) {
        if (!next || now > next + period) next = now + period;
        else next += period;
        return next;
    }
};
}
#endif
