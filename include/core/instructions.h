#include "cpu.h"

namespace Instructions {
using Umibozu::SM83;

void HALT(SM83 *c);
void LD_HL_SP_E8(SM83 *c);
void LD_R_R(u8 &r_1, u8 r_2);
void ADD_SP_E8(SM83 *c);
void LD_M_R(SM83 *c,const u16 address, u8 val);
void LD_SP_U16(SM83 *c, u16 val);
void LD_R16_U16(SM83 *c,Umibozu::SM83::REG_16 &r_1, u16 val);
void LD_U16_SP(SM83 *c,u16 address, u16 sp_val);
void LD_R_AMV(SM83 *c,u8 &r_1, Umibozu::SM83::REG_16 &r_16);
void DEC(SM83 *c,u8 &r);
void SCF(SM83 *c);
void NOP(SM83 *c);
void DEC_R16(SM83 *c, Umibozu::SM83::REG_16 &r); // FIXME: nonsensical
void DEC_SP(SM83 *c);
void CCF(SM83 *c);
void ADD_HL_BC(SM83 *c);
void ADD_HL_DE(SM83 *c);
void DAA(SM83 *c);
void INC(SM83 *c, u8 &r);
void INC_16(SM83 *c,Umibozu::SM83::REG_16 &r);
void ADD(SM83 *c,u8 &r, u8 r_2);
void CP(SM83 *c,const u8 &r, const u8 &r_2);
void OR(SM83 *c,u8 &r, u8 r_2);
void POP(SM83 *c,Umibozu::SM83::REG_16 &r);
void PUSH(SM83 *c,Umibozu::SM83::REG_16 &r);
void RRCA(SM83 *c);
void RLCA(SM83 *c);
void RLA(SM83 *c);
void RST(SM83 *c, u8 pc_new);
void ADC(SM83 *c, u8 &r, u8 r_2);
void SBC(SM83 *c, u8 &r, u8 r_2);
void SUB(SM83 *c, u8 &r, u8 r_2);
void AND(SM83 *c, u8 &r, u8 r_2);
void XOR(SM83 *c, u8 &r, u8 r_2);
void RLC(SM83 *c, u8 &r);
void RLC_HL(SM83 *c);
void RRC(SM83 *c, u8 &r);
void RRC_HL(SM83 *c);
void SLA(SM83 *c, u8 &r);
void SRA(SM83 *c, u8 &r);
void SRA_HL(SM83 *c);
void SLA_HL(SM83 *c);
void RR(SM83 *c,u8 &r);
void RL(SM83 *c,u8 &r);
void RL_HL(SM83 *c);
void RR_HL(SM83 *c);
void SWAP(SM83 *c, u8 &r);
void SWAP_HL(SM83 *c);
void SRL(SM83 *c, u8 &r);
void SRL_HL(SM83 *c);
void SET(SM83 *c, u8 p, u8 &r);
void RES(SM83 *c, u8 p, u8 &r);
void BIT(SM83 *c,const u8 p, const u8 &r);
void RRA(SM83 *c);
void STOP();
} // namespace Instructions