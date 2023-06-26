#ifndef __B_H_HPP__
#define __B_H_HPP__

#include <vector>
#include <bitset>
#include <cassert>

struct BranchPredictor {
    virtual bool predict(uint32_t pc) = 0;
    virtual void update(uint32_t pc, bool taken) = 0;
};

struct SaturatingBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> table;
    SaturatingBranchPredictor(int value) : table(1 << 14, value) {}

    bool predict(uint32_t pc) {
        std::bitset<14> index_14(pc & 0x3fff);
        // std::cout<< index_14<<std::endl;
        uint32_t ind = index_14.to_ulong();
        std::bitset<2> value = table[ind];
        return value.test(1);
    }

    void update(uint32_t pc, bool taken) {
        std::bitset<14> index_14(pc & 0x3fff);
        // std::cout<< index_14<<std::endl;
        uint32_t ind = index_14.to_ulong();
        std::bitset<2> value = table[ind];
        if (taken) {
            if (value == std::bitset<2>("00")) {
                value = std::bitset<2>("01");
            }
            else if(value == std::bitset<2>("01")) {
                value = std::bitset<2>("10");
            }
            else if(value == std::bitset<2>("10")) {
                value = std::bitset<2>("11");
            }
        } else {
            if (value == std::bitset<2>("01")) {
                value = std::bitset<2>("00");
            }
            else if(value == std::bitset<2>("10")) {
                value = std::bitset<2>("01");
            }
            else if(value == std::bitset<2>("11")) {
                value = std::bitset<2>("10");
            }
        }
        table[ind] = value;
    }
};

struct BHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    BHRBranchPredictor(int value) : bhrTable(1 << 2, value), bhr(value) {}

    bool predict(uint32_t pc) {
        uint32_t index = bhr.to_ulong();
        std::bitset<2> value = bhrTable[index];
        return value.test(1);
    }

    void update(uint32_t pc, bool taken) {
        uint32_t index = bhr.to_ulong();
        std::bitset<2> value = bhrTable[index];
        if (taken) {
            if (value != std::bitset<2>("11")) {
                value = value.to_ulong() + 1;
            }
        } else {
            if (value != std::bitset<2>("00")) {
                value = value.to_ulong() - 1;
            }
        }
        bhr <<= 1;
        bhr.set(0, taken);
        bhrTable[index] = value;
    }
};


struct SaturatingBHRBranchPredictor : public BranchPredictor {
    std::vector<std::bitset<2>> bhrTable;
    std::bitset<2> bhr;
    std::vector<std::bitset<2>> table;
    std::vector<std::bitset<2>> combination;
    SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
        assert(size <= (1 << 16));
    }

    bool predict(uint32_t pc) {
        uint32_t bhr_ind = bhr.to_ulong();
        std::bitset<14> index_14(pc & 0x3fff);
        uint32_t sat_ind = index_14.to_ulong();
        uint32_t comb_ind= sat_ind*4+bhr_ind;
        std::bitset<2> value = combination[comb_ind];
        return value.test(1);
    }

    void update(uint32_t pc, bool taken) {
        uint32_t bhr_ind = bhr.to_ulong();
        std::bitset<14> index_14(pc & 0x3fff);
        uint32_t sat_ind = index_14.to_ulong();
        uint32_t comb_ind= sat_ind*4+bhr_ind;
        std::bitset<2> value = combination[comb_ind];
        if (taken) {
            if (value != std::bitset<2>("11")) {
                value = value.to_ulong() + 1;
            }
            
        } else {
            if (value != std::bitset<2>("00")) {
                value = value.to_ulong() - 1;
            }
        }
        combination[comb_ind] = value;
        bhr <<= 1;
        bhr.set(0, taken);
    }
};

// alternate logic for struct SaturatingBHRBranchPredictor : public BranchPredictor {
//     std::vector<std::bitset<2>> bhrTable;
//     std::bitset<2> bhr;
//     std::vector<std::bitset<2>> table;
//     std::vector<std::bitset<2>> combination;
//     SaturatingBHRBranchPredictor(int value, int size) : bhrTable(1 << 2, value), bhr(value), table(1 << 14, value), combination(size, value) {
//         assert(size <= (1 << 16));
//     }

//     bool predict(uint32_t pc) {
//         uint32_t bhr_ind = bhr.to_ulong();
//         std::bitset<14> index_14(pc & 0x3fff);
//         uint32_t sat_ind = index_14.to_ulong();
//         std::bitset<2> val = table[sat_ind];
//         std::bitset<2> va = bhrTable[bhr_ind];
//         if ((va[1] == 0)&&(val[1]==0)){
//             return va[1];
//         }
//         else if (val == std::bitset<2>("11")){
//             return val[1];
//         }
//         else if (val == std::bitset<2>("00")){
//             return val[1];
//         }
//         else if (va == std::bitset<2>("11")){
//             return va[1];
//         }
//         else if (va == std::bitset<2>("00")){
//             return va[1];
//         }
//         else if ((va==std::bitset<2>("01"))&&(val == std::bitset<2>("10"))){
//             return va[0];
//         }
//         else if ((va==std::bitset<2>("10"))&&(val == std::bitset<2>("10"))){
//             return va[1];
//         }
//         else if ((va==std::bitset<2>("10"))&&(val == std::bitset<2>("01"))){
//             return va[0];
//         }
//         else{
//             return false;
//         }
//     }

//     void update(uint32_t pc, bool taken) {
//         uint32_t bhr_ind = bhr.to_ulong();
//         std::bitset<14> index_14(pc & 0x3fff);
//         uint32_t sat_ind = index_14.to_ulong();
//         std::bitset<2> val = table[sat_ind];
//         std::bitset<2> va = bhrTable[bhr_ind];
//         if (taken) {
//             if (va != std::bitset<2>("11")) {
//                 va = va.to_ulong() + 1;
//             }
//             if (val != std::bitset<2>("11")) {
//                 val = val.to_ulong() + 1;
//             }
            
//         } else {
//             if (val != std::bitset<2>("00")) {
//                 val = val.to_ulong() - 1;
//             }
//             if (va != std::bitset<2>("00")) {
//                 va = va.to_ulong() - 1;
//             }
//         }
//         bhr <<= 1;
//         bhr.set(0, taken);
//         bhrTable[bhr_ind] = va;
//         table[sat_ind] = val;
//     }
// };

#endif
