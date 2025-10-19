/** (C) 9 of October, 2025 Burt Sumner */
/** Free to use, but this copyright message must remain here */
#include <GBAdev_memdef.h>
#include "chess_transposition_table.h"



IWRAM_CODE BOOL TTable_Probe(const TTable_t *tt, 
                             TTableEnt_t *query_entry) {
  if (!query_entry)
    return FALSE;
  const u64 KEY = query_entry->key;
  const TTableEnt_t *const SLOT_BUCKETS = tt->slots[KEY&TTABLE_IDX_MASK].buckets;
  u64 curkey;
  const u8 MINDEPTH = query_entry->depth;
  const u8 CURGEN = query_entry->gen;
  for (int  i=0; TTABLE_CLUSTER_SIZE>i; ++i) {
    curkey = SLOT_BUCKETS[i].key;
    if (!curkey || KEY!=curkey)
      continue;
    if (CURGEN!=SLOT_BUCKETS[i].gen)
      continue;
    if (MINDEPTH > SLOT_BUCKETS[i].depth)
      continue;
    *query_entry = SLOT_BUCKETS[i];
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
