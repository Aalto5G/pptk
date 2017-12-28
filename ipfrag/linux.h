/*
 * Authors:
 * - Fred N. van Kempen <waltje@uWalt.NL.Mugnet.ORG>
 * - Alan Cox <alan@lxorguk.ukuu.org.uk>
 * - Juha-Matti Tilli <juha-matti.tilli@iki.fi>
 *
 * Fixes:
 * - Alan Cox        :       Split from ip.c , see ip_input.c for history.
 * - David S. Miller :       Begin massive cleanup...
 * - Andi Kleen      :       Add sysctls.
 * - xxxx            :       Overlapfrag bug.
 * - Ultima          :       ip_expire() kernel panic.
 * - Bill Hawes      :       Frag accounting and evictor fixes.
 * - John McDonald   :       0 length frag bug.
 * - Alexey Kuznetsov:       SMP races, threading, cleanup.
 * - Patrick McHardy :       LRU queue of frag heads for evictor.
 * - Juha-Matti Tilli:       Modify implementation to work in user space.
 *
 * This file is covered by GNU GENERAL PUBLIC LICENSE Version 2.
 */

#ifndef _LINUX_H_
#define _LINUX_H_

#include <errno.h>
#include <stdint.h>
#include "iphdr.h"
#include "llalloc.h"
#include "packet.h"
#include "containerof.h"
#include "ipfrag.h"
#include "iphdr.h"
#include "ipcksum.h"
#include "time64.h"

#define IPSKB_FRAG_COMPLETE	(1<<(3))

/**
 * fragment queue flags
 *
 * @INET_FRAG_FIRST_IN: first fragment has arrived
 * @INET_FRAG_LAST_IN: final fragment has arrived
 * @INET_FRAG_COMPLETE: frag queue has been processed and is due for destruction
 */
enum {
        INET_FRAG_FIRST_IN      = 1<<(0),
        INET_FRAG_LAST_IN       = 1<<(1),
        INET_FRAG_COMPLETE      = 1<<(2),
};

struct inet_frag_queue {
        struct packet           *fragments;
        struct packet           *fragments_tail;
        int                     len;
        int                     meat;
        uint8_t                 flags;
        uint16_t                max_size;
};


struct ipq {
        struct inet_frag_queue q;
        uint16_t        max_df_size; /* largest frag with DF set seen */
};

void ipq_init(struct ipq *ipq);

void ipq_free(struct allocif *loc, struct ipq *ipq);

struct packet *ip_frag_reassemble(struct allocif *loc, struct ipq *qp);

int ip_frag_queue(struct allocif *loc, struct ipq *qp, struct packet *pkt);

#endif
