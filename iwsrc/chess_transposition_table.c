/** (C) 9 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */
#include <GBAdev_memdef.h>
#include "chess_transposition_table.h"

static int tt_probes=0, tt_hits = 0;

IWRAM_CODE BOOL TTable_Probe(TTable_t *tt,
                             TTableEnt_t *query_entry) {
  if (NULL==query_entry)
    return FALSE;
  assert (NULL!=tt);
  ++tt_probes;
  const u64 KEY = query_entry->key;
  const TTableEnt_t *const SLOT_BUCKETS = tt->slots[KEY&TTABLE_IDX_MASK].buckets;
  u64 curkey;
  u32 slot_idx = KEY&TTABLE_IDX_MASK;
  const u8 MAXDEPTH = query_entry->depth;
  const u8 CURGEN = query_entry->gen;
  for (int  i=0; TTABLE_CLUSTER_SIZE>i; ++i) {
    curkey = SLOT_BUCKETS[i].key;
    if (!curkey || KEY!=curkey)
      continue;
    if ((CURGEN-SLOT_BUCKETS[i].gen)>TTENT_TIME_TO_LIVE) {
      // Lazy delete by setting key equal to zero.
      tt->slots[slot_idx].buckets[i].key = 0ULL;
      continue;
    }
    if (MAXDEPTH < SLOT_BUCKETS[i].depth)
      continue;
    *query_entry = SLOT_BUCKETS[i];
    ++tt_hits;
    return TRUE;
  }
  return FALSE;
}

IWRAM_CODE void TTable_Insert(TTable_t *tt, const TTableEnt_t *entry) {
  TTableEnt_t *const slot_buckets = tt->slots[(entry->key)&TTABLE_IDX_MASK].buckets;
  int target=0;
  for (int i=0; TTABLE_CLUSTER_SIZE>i; ++i) {
    if (!slot_buckets[i].key) {
      target = i;
      break;
    }
    if (slot_buckets[i].gen != entry->gen) {
      target = i;
      break;
    }
    if (slot_buckets[i].depth < slot_buckets[target].depth)
      target = i;
  }
  slot_buckets[target] = *entry;
}
