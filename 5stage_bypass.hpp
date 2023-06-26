/**
 * @file MIPS_Processor.hpp
 * @author Mallika Prabhakar and Sayam Sethi
 * 
 */

#ifndef __MIPS_PROCESSOR_HPP__
#define __MIPS_PROCESSOR_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>

struct ControlSignals{
    int RegDst;
    int ALUop1;
    int ALUop0;
    int ALUsrc;
    int Branch = 0;
    int Mem_Read;
    int Mem_Write;
    int Reg_Write;
    int Mem_Reg;
};


struct IF_ID{
    int PC;
};


struct ID_EX{
    int PC;
    int ReadData1;
    int ReadData2;
    std::string sign_extend;
    int adder;
    int rd;
    ControlSignals controls;
};

struct EX_MEM{
    ControlSignals controls;
    int PC;
    int zero = 0;
    int ALUresult;
    int ReadData2;
    int rd;
};

struct MEM_WB{
    ControlSignals controls;
    int ReadData;
    int ALUresult;
    int rd;
};

struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext,instno = 0,max_val = 2147483647;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &,int,int)>> instructions;
	std::unordered_map<std::string, int> registerMap, address, controlNumbers, Types,count;
    std::unordered_map<int,int> dependreg,dependinst,completed,instmap,extract;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::unordered_map<int, int> memoryDelta;
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};

        controlNumbers = {{"add", 0}, {"sub", 0}, {"mul", 0}, {"beq", 3}, {"bne", 3}, {"slt", 0}, {"j", 4}, {"lw", 1}, {"sw", 2}, {"addi", 0}};

        Types = {{"add", 0}, {"sub", 0}, {"mul", 0}, {"beq", 1}, {"bne", 1}, {"slt", 0}, {"j", 4}, {"lw", 2}, {"sw", 2}, {"addi", 3}};

        count = {{"IF",0},{"ID",0},{"EX",0},{"MEM",0},{"WB",0}};

		for (int i = 0; i < 32; ++i){
            registerMap["$" + std::to_string(i)] = i;
            dependreg[i] = 0;
        }
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
	}

	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = extract[dependreg[registerMap[reg]]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

    // controlNumbers = {{"add", 0}, {"sub", 0}, {"mul", 0}, {"beq", 3}, {"bne", 3}, {"slt", 0}, {"j", 4}, {"lw", 1}, {"sw", 2}, {"addi", 0}};

    void assignControls(std::vector<std::string> &command,ControlSignals &signals){
        int type = controlNumbers[command[0]];
        switch(type){
            case 0:
                signals.RegDst = 1;
                signals.ALUop1 = 1;
                signals.ALUop0 = 0;
                signals.ALUsrc = 0;
                signals.Branch = 0;
                signals.Mem_Read = 0;
                signals.Mem_Write = 0;
                signals.Reg_Write = 1;
                signals.Mem_Reg = 0;
                break;
            case 1:
                signals.RegDst = 0;
                signals.ALUop1 = 0;
                signals.ALUop0 = 0;
                signals.ALUsrc = 1;
                signals.Branch = 0;
                signals.Mem_Read = 1;
                signals.Mem_Write = 0;
                signals.Reg_Write = 1;
                signals.Mem_Reg = 1;
                break;
            case 2:
                signals.RegDst = 0;
                signals.ALUop1 = 0;
                signals.ALUop0 = 0;
                signals.ALUsrc = 1;
                signals.Branch = 0;
                signals.Mem_Read = 0;
                signals.Mem_Write = 1;
                signals.Reg_Write = 0;
                signals.Mem_Reg = 0;
                break;
            case 3:
                signals.RegDst = 0;
                signals.ALUop1 = 0;
                signals.ALUop0 = 1;
                signals.ALUsrc = 0;
                signals.Branch = 1;
                signals.Mem_Read = 0;
                signals.Mem_Write = 0;
                signals.Reg_Write = 0;
                signals.Mem_Reg = 0;
                break;
            case 4:
                signals.RegDst = 0;
                signals.ALUop1 = 0;
                signals.ALUop0 = 0;
                signals.ALUsrc = 0;
                signals.Branch = 1;
                signals.Mem_Read = 0;
                signals.Mem_Write = 0;
                signals.Reg_Write = 0;
                signals.Mem_Reg = 0;
                break;
            default:
                break;
        }
    }

    //parse the reg
    std::string reg(std::string location){
        int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
		std::string reg = location.substr(lparen + 1);
		reg.pop_back();
        return reg;
    }

    // Types = {{"add", 0}, {"sub", 0}, {"mul", 0}, {"beq", 1}, {"bne", 1}, {"slt", 0}, {"j", 4}, {"lw", 2}, {"sw", 2}, {"addi", 3}};

    void signextend(std::vector<std::string> &command,ID_EX &id_ex,EX_MEM &ex_mem){
        int type = Types[command[0]];
        switch(type){
            case 0:
                id_ex.sign_extend = command[0];
                if (checkRegister(command[2])){
                    if (dependreg[registerMap[command[2]]]){
                        id_ex.ReadData1 = extract[dependreg[registerMap[command[2]]]];
                    }
                    else{
                        id_ex.ReadData1 = registers[registerMap[command[2]]];
                    }
                }
                else{
                    id_ex.ReadData1 = 0;
                }
                if (checkRegister(command[3])){
                    if (dependreg[registerMap[command[3]]]){
                        id_ex.ReadData2 = extract[dependreg[registerMap[command[3]]]];
                    }
                    else{
                        id_ex.ReadData2 = registers[registerMap[command[3]]];
                    }
                }
                else{
                    id_ex.ReadData2 = 0;
                }
                id_ex.adder = 0;
                id_ex.rd = registerMap[command[1]];
                dependreg[id_ex.rd] = count["ID"];
                break;
            case 1:
                id_ex.sign_extend = command[0];
                if (checkRegister(command[1])){
                    if (dependreg[registerMap[command[1]]]){
                        id_ex.ReadData1 = extract[dependreg[registerMap[command[1]]]];
                    }
                    else{
                        id_ex.ReadData1 = registers[registerMap[command[1]]];
                    }
                }
                else{
                    id_ex.ReadData1 = 0;
                }
                if (checkRegister(command[2])){
                    if (dependreg[registerMap[command[2]]]){
                        id_ex.ReadData2 = extract[dependreg[registerMap[command[2]]]];
                    }
                    else{
                        id_ex.ReadData2 = registers[registerMap[command[2]]];
                    }
                }
                else{
                    id_ex.ReadData2 = 0;
                }
                id_ex.adder = address[command[3]];
                dependinst[-1] = count["ID"];
                id_ex.rd = 0;
                break;
            case 2:
                id_ex.sign_extend = command[0];
                id_ex.ReadData1 = 0;
                if ((checkRegister(command[1])&&(id_ex.sign_extend == "sw"))){
                    if (dependreg[registerMap[command[1]]]){
                        id_ex.ReadData2 = extract[dependreg[registerMap[command[1]]]];
                    }
                    else{
                        id_ex.ReadData2 = registers[registerMap[command[1]]];
                    }
                }
                else{
                    id_ex.ReadData2 = 0;
                }
                if ((checkRegister(reg(command[2])))){
                    if (dependreg[registerMap[reg(command[2])]]){
                        id_ex.ReadData1 = extract[dependreg[registerMap[reg(command[2])]]];
                    }
                    else{
                        id_ex.ReadData1 = registers[registerMap[reg(command[2])]];
                    }
                }
                else{
                    id_ex.ReadData1 = 0;
                }

                id_ex.ReadData1 = 0;
                if (id_ex.ReadData1 != max_val){
                    id_ex.adder = locateAddress(command[2]);
                    }
                if (command[0] == "lw"){
                    id_ex.rd = registerMap[command[1]];
                    dependreg[id_ex.rd] = count["ID"];
                }   
                break;
            case 3:
                id_ex.sign_extend = command[0];
                if (checkRegister(command[2])){
                    if (dependreg[registerMap[command[2]]]){
                        id_ex.ReadData1 = extract[dependreg[registerMap[command[2]]]];
                    }
                    else{
                        id_ex.ReadData1 = registers[registerMap[command[2]]];
                    }
                }
                else{
                    id_ex.ReadData1 = 0;
                }
                id_ex.ReadData2 = stoi(command[3]);
                id_ex.adder = 0;
                id_ex.rd = registerMap[command[1]];
                dependreg[id_ex.rd] = count["ID"];
                break;
            case 4:
                id_ex.sign_extend = command[0];
                id_ex.ReadData1 = 0;
                id_ex.ReadData2 = 0;
                id_ex.adder = address[command[1]];
                dependinst[-1] = count["ID"];
                id_ex.rd = 0;
                break;
            default:
                break;
        }
    }

    //add
    int add(int data1,int data2){
        int out = data1 + data2;
        return out;
    }

    //addi
    int addi(int data1,int data2){
        int out = data1 + data2;
        return out;
    }

    //sub 
    int sub(int data1,int data2){
        int out = data1 - data2;
        return out;
    }

    //mul 
    int mul(int data1,int data2){
        int out = data1 * data2;
        return out;
    }

    //beq
    int beq(int data1,int data2){
        int out = data1 - data2;
        if (out == 0){
            return 0;
        }
        else{
            return 1;
        }
    }

    //bne
    int bne(int data1,int data2){
        int out = data1 - data2;
        if (out == 0){
            return 1;
        }
        else{
            return 0;
        }
    }

    //slt
    int slt(int data1,int data2){
        return data1 < data2;
    }

    //load word
    int lw(int data1,int data2){
        int out = data1 + data2;
        return out;
    }

    //store word
    int sw(int data1,int data2){
        int out = data1 + data2;
        return out;
    }

    //jump
    int j(int data1,int data2){
        return 0;
    }

    //IF stage
    void IF(IF_ID &if_id,EX_MEM &ex_mem){
        if ((dependinst[-1] == 0)||(completed[dependinst[-1]])){
            if((ex_mem.controls.Branch == 1)&&(ex_mem.zero == 1)){
                PCnext = ex_mem.PC + 1;
            }
            else{
                PCnext = instmap[count["IF"]] + 1;
            }
            instno = instno + 1;
            instmap[instno] = PCnext;
            count["IF"] = instno;
            if(completed[instno]){
                completed[instno] = 0;
                dependinst[instno] = 0;
            }
            if(extract[count["IF"]] != max_val){
                extract[count["IF"]] = max_val;
            }
            if (PCnext <= commands.size()){
                if_id.PC = instno;
                if (instmap[if_id.PC] <= commands.size()){
                    count["ID"] = if_id.PC;
                }
            }
        }
    }

    //ID stage
    void ID(IF_ID &if_id,ID_EX &id_ex,EX_MEM &ex_mem){
        id_ex.PC = if_id.PC;
        assignControls(commands[instmap[count["ID"]]-1],id_ex.controls);
        signextend(commands[instmap[count["ID"]]-1],id_ex,ex_mem);
        if (!((id_ex.ReadData1 == max_val)||(id_ex.ReadData2 == max_val))){
            count["EX"] = count["ID"];
        }
        else{
            count["EX"] = -1;
        }
    }

    //EX stage
    void EX(ID_EX &id_ex, EX_MEM &ex_mem){
        ex_mem.PC = id_ex.adder;
        int alu2;
        if (id_ex.controls.ALUsrc == 1){
            alu2 = id_ex.adder;
        }
        else{
            alu2 = id_ex.ReadData2;
        }
        if (Types[id_ex.sign_extend] == 2){
            id_ex.ReadData1 = 0;
        }
        int ret;
        // if (Types[id_ex.sign_extend] == 2){
        //     ret = (int) instructions[id_ex.sign_extend](*this,locateAddress(commands[instmap[count["EX"]]][2]),alu2);
        //     std::cout << "fuck" << locateAddress(commands[instmap[count["EX"]]][2]) << std::endl;
        // }
        // else{
            ret = (int) instructions[id_ex.sign_extend](*this,id_ex.ReadData1,alu2);
        // }
        if (Types[id_ex.sign_extend] != 2){
            extract[count["EX"]] = ret;
        }
        if (ret == 0){
            ex_mem.zero = 1;
        }
        else{
            ex_mem.zero = 0;
        }
        ex_mem.ALUresult = ret;
        ex_mem.rd = id_ex.rd;
        ex_mem.ReadData2 = id_ex.ReadData2;
        ex_mem.controls = id_ex.controls;
        count["MEM"] = count["EX"];
    }

    //MEM stage
    void MEM(EX_MEM &ex_mem,MEM_WB &mem_wb,IF_ID if_id){
        if ((ex_mem.controls.Branch == 1)&&(ex_mem.controls.Mem_Read == 0)){
            completed[count["MEM"]] = 1;
        }
        if(ex_mem.controls.Mem_Write == 1){
            if (data[ex_mem.ALUresult] != ex_mem.ReadData2){
                memoryDelta[ex_mem.ALUresult] = ex_mem.ReadData2;
            }
            data[ex_mem.ALUresult] = ex_mem.ReadData2;
            extract[count["MEM"]] = ex_mem.ReadData2;
            completed[count["MEM"]] = 1;
        }
        if (ex_mem.controls.Mem_Read == 1){
            mem_wb.ReadData = data[ex_mem.ALUresult];
            extract[count["MEM"]] = mem_wb.ReadData;
        }
        else{
            mem_wb.ReadData = 0;
        }
        mem_wb.rd = ex_mem.rd;
        mem_wb.controls = ex_mem.controls;
        mem_wb.ALUresult = ex_mem.ALUresult;
        count["WB"] = count["MEM"];
    }

    //Write Back
    void WB(MEM_WB &mem_wb){
        int write;
        if(mem_wb.controls.Mem_Reg == 1){
            write = mem_wb.ReadData;
        }
        else{
            write = mem_wb.ALUresult;
        }
        if (mem_wb.controls.Reg_Write == 1){
            registers[mem_wb.rd] = write;
            completed[count["WB"]] = 1;
        }
    }

	// execute the commands sequentially (pipelining)
	void executeCommandspipelinedbypass()
	{
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

        IF_ID if_id;
        ID_EX id_ex;
        EX_MEM ex_mem;
        MEM_WB mem_wb;

		int clockCycles = 0;
        printRegistersAndMemoryDelta(clockCycles);
        std::cout << '\n';
		while (1)
		{
			++clockCycles;
			int end = 0;
            if (!completed[count["WB"]]){
                end = 1;
                if (count["WB"] != 0){
                    WB(mem_wb);
                    if (instmap[count["WB"]] == commands.size()){
                        // std::cout << clockCycles << std::endl;
                        printRegistersAndMemoryDelta(clockCycles);
                        break;
                    }
                    // std::cout<< "fuck" << count["MEM"] << count["WB"]<<std::endl;
                }
            }
            if (!completed[count["MEM"]]){
                end = 1;
                if (count["MEM"] != 0){
                    MEM(ex_mem,mem_wb,if_id);
                }
            }
            if (count["EX"] != -1){
                if (!completed[count["EX"]]){
                    end = 1;
                    if (count["EX"] != 0){
                        EX(id_ex,ex_mem);
                    }
                }
                if (!completed[count["ID"]]){
                    end = 1;
                    if (count["ID"] != 0){
                        ID(if_id,id_ex,ex_mem);
                    }
                }
                if ((dependinst[count["IF"]] == 0)||(completed[dependinst[count["IF"]]])){
                    if (instmap[count["IF"]] <= commands.size()){
                        end = 1;
                        IF(if_id,ex_mem);
                    }
                }
            }
            if (!end){
                break;
            }   
            // std::cout << clockCycles << std::endl;
            printRegistersAndMemoryDelta(clockCycles);
            std::cout << '\n';
        } 
		handleExit(SUCCESS, clockCycles);
	}

	// print the register data in hexadecimal
	void printRegistersAndMemoryDelta(int clockCycle)
	{
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout << '\n';
		std::cout << memoryDelta.size() << ' ';
		for (auto &p : memoryDelta)
			std::cout << p.first << ' ' << p.second << ' ';
		memoryDelta.clear();
	}
};

#endif