/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2020-present Yutaro Hayakawa
 */

#include <stdint.h>
#include <linux/ptrace.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>

struct gso_info {
  unsigned int len;
  uint16_t gso_size;
  uint16_t gso_segs;
  uint32_t gso_type;
};

/*
 * These are the only subset of actual sk_buff or skb_shared_info
 * definitions but no problem. Because BPF-CORE feature of libbpf
 * loader takes care of rewrite this program based on actual
 * definition from kernel BTF
 */
struct sk_buff {
  unsigned int len;
  unsigned char *head;
  unsigned int end;
};

struct skb_shared_info {
  uint16_t gso_size;
  uint16_t gso_segs;
  uint32_t gso_type;
};

__hidden int
module(void *ctx, struct sk_buff *skb, uint8_t data[64])
{
  unsigned int end;
  unsigned char *head;
  struct skb_shared_info *shinfo;
  struct gso_info *info = (struct gso_info *)data;

  head = BPF_CORE_READ(skb, head);
  end = BPF_CORE_READ(skb, end);

  /*
   * This calcuration only works when the kernel is compiled
   * with NET_SKBUFF_DATA_USES_OFFSET=y because if it set to
   * 'n', type of end is unsigned char *.
   */
  shinfo = (struct skb_shared_info *)(head + end);

  info->len = BPF_CORE_READ(skb, len);
  info->gso_size = BPF_CORE_READ(shinfo, gso_size);
  info->gso_segs = BPF_CORE_READ(shinfo, gso_segs);
  info->gso_type = BPF_CORE_READ(shinfo, gso_type);

  return 0;
}
