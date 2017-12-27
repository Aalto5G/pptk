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
#include "linux.h"

void ipq_init(struct ipq *ipq)
{
  ipq->max_df_size = 0;
  ipq->q.flags = 0;
  ipq->q.len = 0;
  ipq->q.meat = 0;
  ipq->q.max_size = 0;
  ipq->q.fragments = NULL;
  ipq->q.fragments_tail = NULL;
}

void ipq_free(struct allocif *loc, struct ipq *ipq)
{
  struct packet *pkt, *next;
  pkt = ipq->q.fragments;
  while (pkt != NULL)
  {
    next = CONTAINER_OF(pkt->node.next, struct packet, node);
    allocif_free(loc, pkt);
    pkt = next;
  }
  ipq->q.fragments = NULL;
  ipq->q.fragments_tail = NULL;
}

struct packet *ip_frag_reassemble(struct allocif *loc, struct ipq *qp)
{
  struct packet *pkt;
  struct packet *pkt2;
  void *ip2;
  char *pay2;
  void *ether;
  void *ether2;
  void *ip;
  char *pay;
  uint16_t frag_off, within_off;
  pkt = qp->q.fragments;
  //printf("%d\n", qp->q.len);
  //printf("%d\n", qp->q.meat);
  pkt2 = allocif_alloc(loc, packet_size(qp->q.len));
  ether2 = packet_data(pkt2);
  ip2 = ether_payload(ether2);
  ether = packet_data(pkt);
  ip = ether_payload(ether);
  //printf("copying %d\n", 14 + ip_hdr_len(ip));
  pkt2->sz = qp->q.len + 14 + ip_hdr_len(ip);
  memcpy(ether2, ether, 14 + ip_hdr_len(ip));
  ip_set_frag_off(ip2, 0);
  ip_set_more_frags(ip2, 0);
  ip_set_total_len(ip2, ip_hdr_len(ip2) + qp->q.len);
  ip_set_hdr_cksum_calc(ip2, ip_hdr_len(ip2));
  pay2 = ip_payload(ip2);
  while (pkt != NULL)
  {
    ip = ether_payload(packet_data(pkt));
    pay = ip_payload(ip);
    frag_off = ip_frag_off(ip);
    within_off = pkt->positive.offset - frag_off;
    memcpy(&pay2[pkt->positive.offset], &pay[within_off], pkt->sz - 14 - 20 - pkt->positive.pulled);
    //printf("%d(%d) %d\n", pkt->positive.offset, frag_off, (int)(frag_off + pkt->sz - 14 - 20 - pkt->positive.pulled));
    pkt = CONTAINER_OF(pkt->node.next, struct packet, node);
  }
  return pkt2;
}

int ip_frag_queue(struct allocif *loc, struct ipq *qp, struct packet *pkt)
{
        void *ip = ether_payload(packet_data(pkt));
        struct packet *prev, *next;
        unsigned int fragsize;
        int offset;
        int ihl, end;
        int err = -ENOENT;

        pkt->positive.pulled = 0;

        if (qp->q.flags & INET_FRAG_COMPLETE)
                goto err;

#if 0
        if (!(pkt->positive.flags & IPSKB_FRAG_COMPLETE) &&
            (ip_frag_too_far(qp)) &&
            (err = ip_frag_reinit(qp))) {
                ipq_kill(qp);
                goto err;
        }
#endif

        offset = ip_frag_off(ip);
        ihl = ip_hdr_len(ip);
        /* Determine the position of this fragment. */
        end = offset + pkt->sz - 14 - ihl;
        err = -EINVAL;

        /* Is this the final fragment? */
        if (ip_more_frags(ip) == 0) {
                /* If we already have some bits beyond end
                 * or have different end, the segment is corrupted.
                 */
                if (end < qp->q.len ||
                    ((qp->q.flags & INET_FRAG_LAST_IN) && end != qp->q.len))
                        goto err;
                qp->q.flags |= INET_FRAG_LAST_IN;
                qp->q.len = end;
        } else {
                if (end&7) {
                        end &= ~7;
                }
                if (end > qp->q.len) {
                        /* Some bits beyond end -> corruption. */
                        if (qp->q.flags & INET_FRAG_LAST_IN)
                                goto err;
                        qp->q.len = end;
                }
        }
        if (end == offset)
                goto err;

        err = -ENOMEM;
        //pkt->positive.pulled += 14 + ihl;

        /* Find out which fragments are in front and at the back of us
         * in the chain of fragments so far.  We must know where to put
         * this fragment, right?
         */
        prev = qp->q.fragments_tail;
        if (!prev || prev->positive.offset < offset) {
                next = NULL;
                goto found;
        }
        prev = NULL;
        for (next = qp->q.fragments; next != NULL;
             next = CONTAINER_OF(next->node.next, struct packet, node)) {
                if (next->positive.offset >= offset)
                        break;  /* bingo! */
                prev = next;
        }

found:
        /* We found where to put this one.  Check for overlap with
         * preceding fragment, and, if needed, align things so that
         * any overlaps are eliminated.
         */
        if (prev) {
                int i = (prev->positive.offset + prev->sz - 14 - 20 - prev->positive.pulled) - offset; // FIXME 20

                if (i > 0) {
                        offset += i;
                        err = -EINVAL;
                        if (end <= offset)
                                goto err;
                        err = -ENOMEM;
                        pkt->positive.pulled += i;
                }
        }

        err = -ENOMEM;

        while (next && next->positive.offset < end) {
                int i = end - next->positive.offset; /* overlap is 'i' bytes */

                if (i < (int)(next->sz - 14 - 20 - next->positive.pulled)) { // FIXME 20
                        /* Eat head of the next overlapped fragment
                         * and leave the loop. The next ones cannot overlap.
                         */
                        next->positive.pulled += i;
                        next->positive.offset += i;
                        qp->q.meat -= i;
                        break;
                } else {
                        struct packet *free_it = next;

                        /* Old fragment is completely overridden with
                         * new one drop it.
                         */
                        next = CONTAINER_OF(next->node.next, struct packet, node);

                        if (prev)
                                prev->node.next = &next->node;
                        else
                                qp->q.fragments = next;

                        qp->q.meat -= (free_it->sz - 14 - 20 - free_it->positive.pulled); // FIXME 20
                        allocif_free(loc, free_it);
                }
        }

        pkt->positive.offset = offset;

        /* Insert this fragment in the chain of fragments. */
        pkt->node.next = &next->node;
        if (!next)
                qp->q.fragments_tail = pkt;
        if (prev)
                prev->node.next = &pkt->node;
        else
                qp->q.fragments = pkt;

        qp->q.meat += pkt->sz - 14 - ihl - pkt->positive.pulled;
        if (offset == 0)
                qp->q.flags |= INET_FRAG_FIRST_IN;

        fragsize = pkt->sz - ihl - 14 - pkt->positive.pulled + ihl;

        if (fragsize > qp->q.max_size)
                qp->q.max_size = fragsize;

        if (ip_dont_frag(ip) &&
            fragsize > qp->max_df_size)
                qp->max_df_size = fragsize;

        if (qp->q.flags == (INET_FRAG_FIRST_IN | INET_FRAG_LAST_IN) &&
            qp->q.meat == qp->q.len) {
                return 0;
        }

        return -EINPROGRESS;

err:
        allocif_free(loc, pkt);
        return err;
}
