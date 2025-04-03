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

int tick;

void cacheInit() {}

void cachelineReplace(CacheLine *cache,uint64_t rep_pos,uint64_t tag){
  cache[rep_pos].tag=tag;
  cache[rep_pos].valid=true;
  cache[rep_pos].dirty=false;
  cache[rep_pos].latest_used=tick;
}

void storeL2(uint64_t addr){
  ++tick;
  uint64_t setid,tagid;
  setid=(addr>>3)%8;
  tagid=(addr>>6);
  for(int i=0;i<L2_LINE_NUM;++i){
    if(!l2ucache[setid][i].valid) continue;
    if(l2ucache[setid][i].tag==tagid){
      l2ucache[setid][i].dirty=true;
      return;
    }
  }
}

void storeL3(uint64_t addr){
  ++tick;
  uint64_t setid,tagid;
  setid=(addr>>3)%16;
  tagid=(addr>>7);
  for(int i=0;i<L3_LINE_NUM;++i){
    if(!l3ucache[setid][i].valid) continue;
    if(l3ucache[setid][i].tag==tagid){
      l3ucache[setid][i].dirty=true;
      return;
    }
  }
}

void backInvalidL1(uint64_t addr,CacheLine *L1cache){
  ++tick;
  uint64_t setid,tagid,k;
  setid=(addr>>3)%4;
  tagid=(addr>>5);
  uint16_t if_hit,pos;
  if_hit=false;
  for(int i=0;i<L1_LINE_NUM;++i){
    k=setid*L1_LINE_NUM+i;
    if(!L1cache[k].valid) continue;
    if(L1cache[k].tag==tagid) {
      if_hit=true;
      pos=k;
      break;
    } 
  }
  if(!if_hit) return;
  if(!L1cache[pos].valid) return;
  L1cache[pos].valid=false;
  if(L1cache[pos].dirty) storeL2(addr);
}

void backInvalidL2(uint64_t addr){
  ++tick;
  uint64_t setid,tagid;
  setid=(addr>>3)%8;
  tagid=(addr>>6);
  uint16_t if_hit,pos;
  if_hit=false;
  for(int i=0;i<L2_LINE_NUM;++i){
    if(!l2ucache[setid][i].valid) continue;
    if(l2ucache[setid][i].tag==tagid) {
      if_hit=true;
      pos=i;
      break;
    } 
  }
  if(!if_hit) return;
  if(!l2ucache[setid][pos].valid) return;
  l2ucache[setid][pos].valid=false;
  backInvalidL1(addr,(CacheLine*)&l1icache);
  backInvalidL1(addr,(CacheLine*)&l1dcache);
  if(l2ucache[setid][pos].dirty) storeL3(addr);
  return;
}

void cacheAccessL3(uint64_t addr){
  ++tick;
  uint64_t setid,tagid;
  uint16_t rep_pos;
  rep_pos=-1;
  // offset=addr%8;
  setid=(addr>>3)%16;
  tagid=(addr>>7);
  for(int i=0;i<L3_LINE_NUM;++i){
    if(!l3ucache[setid][i].valid) continue;
    if(l3ucache[setid][i].tag==tagid) {
      ++l3_hits;
      return;
    } 
  }
  ++l3_misses;
  for(int i=0;i<L3_LINE_NUM;++i){
    if(l3ucache[setid][i].valid) continue;
    rep_pos=i;
    break;
  }
  if(rep_pos>=0){
    cachelineReplace((CacheLine*)&l3ucache,rep_pos,tagid);
    return;
  }
  ++l3_evictions;
  for(int i=1;i<L3_LINE_NUM;++i){
    if(l3ucache[setid][i].latest_used<l3ucache[setid][rep_pos].latest_used)
      rep_pos=i;
  }
  backInvalidL2(addr);
  rep_pos=setid*L3_LINE_NUM+rep_pos;
  cachelineReplace((CacheLine*)&l3ucache,rep_pos,tagid);
  return;
}

void cacheAccessL2(uint64_t addr,CacheLine *L1cache){
  ++tick;
  uint64_t setid,tagid;
  uint16_t rep_pos;
  rep_pos=-1;
  // offset=addr%8;
  setid=(addr>>3)%8;
  tagid=(addr>>6);
  for(int i=0;i<L2_LINE_NUM;++i){
    if(!l2ucache[setid][i].valid) continue;
    if(l2ucache[setid][i].tag==tagid) {
      ++l2_hits;
      return;
    } 
  }
  ++l2_misses;
  cacheAccessL3(addr);
  for(int i=0;i<L2_LINE_NUM;++i){
    if(l2ucache[setid][i].valid) continue;
    rep_pos=i;
    break;
  }
  if(rep_pos>=0){
    cachelineReplace((CacheLine*)&l2ucache,rep_pos,tagid);
    return;
  }
  ++l2_evictions;
  for(int i=1;i<L2_LINE_NUM;++i){
    if(l2ucache[setid][i].latest_used<l2ucache[setid][rep_pos].latest_used)
      rep_pos=i;
  }
  if(l2ucache[setid][rep_pos].dirty) storeL3(addr);
  backInvalidL1(addr,L1cache);
  rep_pos=setid*L2_LINE_NUM+rep_pos;
  cachelineReplace((CacheLine*)&l2ucache,rep_pos,tagid);
  return;
}

void cacheAccessL1(uint64_t addr,CacheLine *L1cache){
  ++tick;
  uint64_t setid,tagid,k;
  uint16_t rep_pos;
  rep_pos=-1;
  // offset=addr%8;
  setid=(addr>>3)%4;
  tagid=(addr>>5);
  for(int i=0;i<L1_LINE_NUM;++i){
    k=setid*L1_LINE_NUM+i;
    if(!L1cache[k].valid) continue;
    if(L1cache[k].tag==tagid) {
      if(L1cache==(CacheLine*)&l1dcache) ++l1d_hits;
      else ++l1i_hits;
      return;
    } 
  }
  if(L1cache==(CacheLine*)&l1dcache) ++l1d_misses;
  else ++l1i_misses;
  cacheAccessL2(addr,L1cache);
  for(int i=0;i<L1_LINE_NUM;++i){
    k=setid*L1_LINE_NUM+i;
    if(L1cache[k].valid) continue;
    rep_pos=i;
    break;
  }
  if(rep_pos>=0){
    cachelineReplace(L1cache,rep_pos,tagid);
    return;
  }
  if(L1cache==(CacheLine*)&l1dcache) ++l1d_evictions;
  else ++l1i_evictions;
  for(int i=1;i<L1_LINE_NUM;++i){
    k=setid*L1_LINE_NUM+i;
    if(L1cache[k].latest_used<L1cache[rep_pos].latest_used)
      rep_pos=L1_LINE_NUM+i;
  }
  if(L1cache[rep_pos].dirty) storeL2(addr);
  cachelineReplace(L1cache,rep_pos,tagid);
  return;
}

void cacheStoreL1(uint64_t addr){
  ++tick;
  uint64_t setid,tagid,k;
  uint16_t rep_pos;
  rep_pos=-1;
  // offset=addr%8;
  setid=(addr>>3)%4;
  tagid=(addr>>5);
  for(int i=0;i<L1_LINE_NUM;++i){
    if(!l1dcache[setid][i].valid) continue;
    if(l1dcache[setid][i].tag==tagid) {
      l1dcache[setid][i].dirty=true;
      ++l1d_hits;
      return;
    } 
  }
  ++l1d_misses;
  cacheAccessL2(addr,(CacheLine*)&l1dcache);
  for(int i=0;i<L1_LINE_NUM;++i){
    if(l1dcache[setid][i].valid) continue;
    rep_pos=i;
    break;
  }
  if(rep_pos>=0){
    k=setid*L1_LINE_NUM+rep_pos;
    cachelineReplace((CacheLine*)&l1dcache,k,tagid);
    l1dcache[setid][rep_pos].dirty=true;
    return;
  }
  ++l1d_evictions;
  for(int i=1;i<L1_LINE_NUM;++i){
    if(l1dcache[setid][i].latest_used<l1dcache[setid][rep_pos].latest_used)
      rep_pos=i;
  }
  if(l1dcache[setid][rep_pos].dirty) storeL2(addr);
  k=setid*L1_LINE_NUM+rep_pos;
  cachelineReplace((CacheLine*)&l1dcache,k,tagid);
  l1dcache[setid][rep_pos].dirty=true;
  return;
}

void cacheAccess(char op, uint64_t addr, uint32_t len) {
  if(op=='I') cacheAccessL1(addr,(CacheLine*)&l1icache);
  else if(op=='L') cacheAccessL1(addr,(CacheLine*)&l1dcache);
  else if(op=='S') cacheStoreL1(addr);
  else if(op=='M'){
    cacheAccessL1(addr,(CacheLine*)&l1icache);
    cacheStoreL1(addr);
  }
}
