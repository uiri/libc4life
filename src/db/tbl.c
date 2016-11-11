#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "col.h"
#include "coro.h"
#include "macros.h"
#include "rec.h"
#include "tbl.h"
#include "uid.h"

struct c4tbl *c4tbl_init(struct c4tbl *self, const char *name) {
  self->name = strdup(name);
  c4map_init(&self->recs, c4uids_cmp);
  return self;
}

void c4tbl_free(struct c4tbl *self) {
  free(self->name);
  c4map_free(&self->recs);
}

static void seq_free(struct c4seq *_seq) {
  struct c4tbl_seq *seq = STRUCTOF(_seq, struct c4tbl_seq, super);
  c4rec_free(&seq->rec);
}

static void *seq_next(struct c4seq *_seq) {
  struct c4tbl_seq *seq = STRUCTOF(_seq, struct c4tbl_seq, super);
  struct c4seq *recs_seq = &seq->recs_seq.super;
  struct c4map_it *it;

  if (!(it = c4seq_next(recs_seq))) { return NULL; }

  if (_seq->idx) { c4uid_copy(seq->rec.id, it->key); }   
  else { c4rec_init(&seq->rec, it->key); }
  c4map_clear(&seq->rec.flds);
  c4map_merge(&seq->rec.flds, it->val);
  return &seq->rec;
}

struct c4seq *c4tbl_seq(struct c4tbl *self, struct c4tbl_seq *seq) {
  c4seq_init(&seq->super);
  seq->super.free = seq_free;
  seq->super.next = seq_next;
  c4rec_init(&seq->rec, NULL);
  c4map_seq(&self->recs, &seq->recs_seq);
  return &seq->super;
}

struct c4rec *c4tbl_upsert(struct c4tbl *self, struct c4rec *rec) {
  size_t idx;
  struct c4map_it *it = c4map_find(&self->recs, rec->id, 0, &idx);
  struct c4map *flds = NULL;
  
  if (it) {
    flds = it->val;
    c4map_clear(flds);
  } else {
    flds = c4map_init(malloc(sizeof(struct c4map)), c4cols_cmp);
    c4map_insert(&self->recs, idx, rec->id, flds);
  }

  c4map_merge(flds, &rec->flds);
  return rec;
}
