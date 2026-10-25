// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2026 Wilwous
// Field-level SDK default contract. No padding or title startup assumptions.
#include <rex/system/thread_state.h>
#include <array>
#include <algorithm>
#include <new>
#include <cstdio>
#define FOR32(M) M(0) M(1) M(2) M(3) M(4) M(5) M(6) M(7) M(8) M(9) M(10) M(11) M(12) M(13) M(14) M(15) M(16) M(17) M(18) M(19) M(20) M(21) M(22) M(23) M(24) M(25) M(26) M(27) M(28) M(29) M(30) M(31)
#define FORV(M) FOR32(M) M(32) M(33) M(34) M(35) M(36) M(37) M(38) M(39) M(40) M(41) M(42) M(43) M(44) M(45) M(46) M(47) M(48) M(49) M(50) M(51) M(52) M(53) M(54) M(55) M(56) M(57) M(58) M(59) M(60) M(61) M(62) M(63) M(64) M(65) M(66) M(67) M(68) M(69) M(70) M(71) M(72) M(73) M(74) M(75) M(76) M(77) M(78) M(79) M(80) M(81) M(82) M(83) M(84) M(85) M(86) M(87) M(88) M(89) M(90) M(91) M(92) M(93) M(94) M(95) M(96) M(97) M(98) M(99) M(100) M(101) M(102) M(103) M(104) M(105) M(106) M(107) M(108) M(109) M(110) M(111) M(112) M(113) M(114) M(115) M(116) M(117) M(118) M(119) M(120) M(121) M(122) M(123) M(124) M(125) M(126) M(127)
static bool defaults(const PPCContext&c){
#define G(n) if(c.r##n.u64!=(n==1?0x13570000ull:n==13?0x24680000ull:0))return false;
 FOR32(G)
#undef G
#define F(n) if(c.f##n.u64)return false;
 FOR32(F)
#undef F
#define V(n) for(auto lane:c.v##n.u8)if(lane)return false;
 FORV(V)
#undef V
 const PPCCRRegister*cr[]{&c.cr0,&c.cr1,&c.cr2,&c.cr3,&c.cr4,&c.cr5,&c.cr6,&c.cr7};
 for(auto f:cr)if(f->lt||f->gt||f->eq||f->so)return false;
 return !c.lr&&!c.ctr.u64&&!c.reserved.u64&&!c.xer.so&&!c.xer.ov&&!c.xer.ca&&
  c.msr==0x200a000&&!c.fpscr.csr&&!c.vscr_sat&&!c.last_indirect_target;
}
int main(){
 using State=rex::runtime::ThreadState;unsigned checks=0;
 for(unsigned char pattern:{0xa5,0x5a,0xcc}){
  alignas(State)std::array<unsigned char,sizeof(State)>storage;std::fill(storage.begin(),storage.end(),pattern);
  auto*s=new(storage.data())State(123,0x13570000,0x24680000,nullptr);auto&c=*s->context();
  if(!defaults(c)){std::fprintf(stderr,"FAIL field initialization pattern=%u\n",pattern);return 1;}++checks;
  // Each incomplete semantic-field candidate must fail this ORIGINAL contract.
  auto reject=[&](auto&field,auto value){auto saved=field;field=value;bool refused=!defaults(c);field=saved;if(refused)++checks;return refused;};
  if(!reject(c.r8.u64,0xa500000000000000ull)||!reject(c.f31.u64,0xa500000000000000ull)||
     !reject(c.ctr.u64,0xa500000000000000ull)||!reject(c.cr7.so,uint8_t(1))||
     !reject(c.v127.u8[15],uint8_t(1))||!reject(c.r1.u64,0ull)||!reject(c.r13.u64,0ull))return 1;
  s->~State();
 }
 std::printf("{\"group\":\"context_fields\",\"result\":\"PASS\",\"checks\":%u,\"prefill_patterns\":3,\"rejected_incomplete_candidates\":21,\"gpr\":32,\"fpr\":32,\"vectors\":128,\"guest_functions_invoked\":0,\"title_provenance\":false}\n",checks);return 0;
}
