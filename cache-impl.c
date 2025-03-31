#include "cachelab.h"
#include <stdint.h>
// feel free to include any files you need above

int l1d_hits = 0;
int l1d_misses = 0;
int l1d_evictions = 0;
int l1i_hits = 0;
int l1i_misses = 0;
int l1i_evictions = 0;
int l2_hits = 0;
int l2_misses = 0;
int l2_evictions = 0;
int l3_hits = 0;
int l3_misses = 0;
int l3_evictions = 0;

// you can add your own data structures and functions below

// you are not allowed to modify the declaration of this function
int tick;

void cacheInit() {}

// you are not allowed to modify the declaration of this function

void cachelineReplace(CacheLine **cache,uint64_t set,uint64_t rep_line,uint64_t tag){
  cache[set][rep_line].tag=tag;
  cache[set][rep_line].valid=true;
  cache[set][rep_line].dirty=false;
  cache[set][rep_line].latest_used=tick;
}

void cacheAccessL2(uint64_t addr,uint32_t len){

}

void cacheAccessL1(uint64_t addr,uint32_t len,CacheLine ** L1cache){
  ++tick;
  uint64_t setid,tagid;
  uint16_t if_hit,rep_pos;
  if_hit=0;
  rep_pos=-1;
  // offset=addr%8;
  setid=(addr>>3)%4;
  tagid=(addr>>5);
  for(int i=0;i<L1_LINE_NUM;++i){
    if(!L1cache[setid][i].valid) continue;
    if(L1cache[setid][i].tag==tagid) {if_hit=true;break;} 
  }
  if(if_hit) return;
  cacheAccessL2(addr,len);
  for(int i=0;i<L1_LINE_NUM;++i){
    if(L1cache[setid][i].valid) continue;
    rep_pos=i;
    break;
  }
  if(rep_pos>=0){
    cachelineReplace(L1cache,setid,rep_pos,tagid);
    return;
  }

  for(int i=1;i<L1_LINE_NUM;++i){
    if(L1cache[setid][i].latest_used<L1cache[setid][rep_pos].latest_used)
      rep_pos=i;
  }
  if(L1cache[setid][rep_pos].dirty) storeL2(addr,len);
  cachelineReplace(L1cache,setid,rep_pos,tagid);
  return;
}


void cacheAccess(char op, uint64_t addr, uint32_t len) {
  if(op=='I') cacheAccessL1(addr,len,&l1icache);
}
