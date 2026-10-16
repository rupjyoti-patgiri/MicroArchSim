#include <gtest/gtest.h>
#include "instruction.h"
#include "types.h"

using namespace microarchsim;

TEST(InstructionTest, DecodeRTypeAdd) {
    // ADD x1, x2, x3
    // opcode: 0110011, rd: 1, funct3: 000, rs1: 2, rs2: 3, funct7: 0000000
    // binary: 0000000 00011 00010 000 00001 0110011 = 0x003100B3
    Word raw = 0x003100B3;
    DecodedInstruction dec = decode(raw);

    EXPECT_EQ(dec.id, InstructionID::ADD);
    EXPECT_EQ(dec.format, InstructionFormat::R_TYPE);
    EXPECT_EQ(dec.rd, 1);
    EXPECT_EQ(dec.rs1, 2);
    EXPECT_EQ(dec.rs2, 3);
    EXPECT_TRUE(dec.reg_write);
    EXPECT_FALSE(dec.mem_read);
    EXPECT_FALSE(dec.mem_write);
    EXPECT_EQ(dec.alu_op, ALUOp::ADD);
}

TEST(InstructionTest, DecodeRTypeSub) {
    // SUB x4, x5, x6
    // opcode: 0110011, rd: 4, funct3: 000, rs1: 5, rs2: 6, funct7: 0100000
    // binary: 0100000 00110 00101 000 00100 0110011 = 0x40628233
    Word raw = 0x40628233;
    DecodedInstruction dec = decode(raw);

    EXPECT_EQ(dec.id, InstructionID::SUB);
    EXPECT_EQ(dec.rd, 4);
    EXPECT_EQ(dec.rs1, 5);
    EXPECT_EQ(dec.rs2, 6);
    EXPECT_EQ(dec.alu_op, ALUOp::SUB);
}

TEST(InstructionTest, DecodeITypeAddi) {
    // ADDI x1, x0, 42
    // imm[11:0] = 42 (0x02A), rs1 = 0, funct3 = 000, rd = 1, opcode = 0010011
    Word raw = 0x02A00093;
    DecodedInstruction dec = decode(raw);

    EXPECT_EQ(dec.id, InstructionID::ADDI);
    EXPECT_EQ(dec.format, InstructionFormat::I_TYPE);
    EXPECT_EQ(dec.rd, 1);
    EXPECT_EQ(dec.rs1, 0);
    EXPECT_EQ(dec.immediate, 42);
    EXPECT_TRUE(dec.reg_write);
}

TEST(InstructionTest, DecodeITypeNegativeImm) {
    // ADDI x2, x1, -10
    // imm = -10 (0xFF6 in 12-bit)
    // 111111110110 00001 000 00010 0010011 = 0xFF608113
    Word raw = 0xFF608113;
    DecodedInstruction dec = decode(raw);

    EXPECT_EQ(dec.id, InstructionID::ADDI);
    EXPECT_EQ(dec.rd, 2);
    EXPECT_EQ(dec.rs1, 1);
    EXPECT_EQ(dec.immediate, -10);
}

TEST(InstructionTest, DecodeLoadWord) {
    // LW x5, 8(x2)
    // imm = 8, rs1 = 2, funct3 = 010, rd = 5, opcode = 0000011
    Word raw = 0x00812283;
    DecodedInstruction dec = decode(raw);

    EXPECT_EQ(dec.id, InstructionID::LW);
    EXPECT_EQ(dec.format, InstructionFormat::I_TYPE);
    EXPECT_EQ(dec.rd, 5);
    EXPECT_EQ(dec.rs1, 2);
    EXPECT_EQ(dec.immediate, 8);
    EXPECT_TRUE(dec.mem_read);
    EXPECT_TRUE(dec.mem_to_reg);
    EXPECT_TRUE(dec.reg_write);
}

TEST(InstructionTest, DecodeStoreWord) {
    // SW x5, 12(x2)
    // imm[11:5] = 0, rs2 = 5, rs1 = 2, funct3 = 010, imm[4:0] = 12, opcode = 0100011
    // 0000000 00101 00010 010 01100 0100011 = 0x00512623
    Word raw = 0x00512623;
    DecodedInstruction dec = decode(raw);

    EXPECT_EQ(dec.id, InstructionID::SW);
    EXPECT_EQ(dec.format, InstructionFormat::S_TYPE);
    EXPECT_EQ(dec.rs1, 2);
    EXPECT_EQ(dec.rs2, 5);
    EXPECT_EQ(dec.immediate, 12);
    EXPECT_TRUE(dec.mem_write);
    EXPECT_FALSE(dec.reg_write);
}

TEST(InstructionTest, DecodeBranchBeqBne) {
    // BEQ x1, x2, 16
    // B-type imm encoding: imm=16 -> offset
    // 0000000 00010 00001 000 10000 1100011 = 0x00208863
    Word raw_beq = 0x00208863;
    DecodedInstruction dec_beq = decode(raw_beq);
    EXPECT_EQ(dec_beq.id, InstructionID::BEQ);
    EXPECT_TRUE(dec_beq.is_branch);
    EXPECT_EQ(dec_beq.rs1, 1);
    EXPECT_EQ(dec_beq.rs2, 2);
    EXPECT_EQ(dec_beq.immediate, 16);

    // BNE x1, x2, -4
    // 1111111 00010 00001 001 11100 1100011 -> imm = -4
    Word raw_bne = 0xFE209EE3;
    DecodedInstruction dec_bne = decode(raw_bne);
    EXPECT_EQ(dec_bne.id, InstructionID::BNE);
    EXPECT_TRUE(dec_bne.is_branch);
    EXPECT_EQ(dec_bne.immediate, -4);
}

TEST(InstructionTest, DecodeJAL) {
    // JAL x1, 20
    // J-type
    Word raw = 0x014000EF;
    DecodedInstruction dec = decode(raw);
    EXPECT_EQ(dec.id, InstructionID::JAL);
    EXPECT_TRUE(dec.is_jump);
    EXPECT_EQ(dec.rd, 1);
    EXPECT_EQ(dec.immediate, 20);
}

TEST(InstructionTest, DecodeLUIandAUIPC) {
    // LUI x10, 0x12345 (imm << 12)
    Word raw_lui = 0x12345537;
    DecodedInstruction dec_lui = decode(raw_lui);
    EXPECT_EQ(dec_lui.id, InstructionID::LUI);
    EXPECT_EQ(dec_lui.rd, 10);
    EXPECT_EQ(dec_lui.immediate, static_cast<SWord>(0x12345000));

    // AUIPC x11, 0x1000
    Word raw_auipc = 0x01000597;
    DecodedInstruction dec_auipc = decode(raw_auipc);
    EXPECT_EQ(dec_auipc.id, InstructionID::AUIPC);
    EXPECT_EQ(dec_auipc.rd, 11);
    EXPECT_EQ(dec_auipc.immediate, static_cast<SWord>(0x01000000));
}

TEST(InstructionTest, DecodeHaltAndNop) {
    DecodedInstruction nop = decode(NOP_INSTRUCTION);
    EXPECT_EQ(nop.id, InstructionID::NOP);

    DecodedInstruction halt = decode(HALT_INSTRUCTION);
    EXPECT_EQ(halt.id, InstructionID::HALT);
    EXPECT_TRUE(halt.is_halt);
}
