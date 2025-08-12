#include <filesystem>
#include <stdexcept>

#include "common.hpp"
#include "cpu.hpp"
#include "fmt/core.h"
#include "fstream"
#include "json/json.hpp"
using json = nlohmann::json;

struct CPU_Test_State {
  std::string name;
  std::vector<u8> ram;
  u16 PC;
  u16 SP;
  u8 A;
  u8 B;
  u8 C;
  u8 D;
  u8 E;
  u8 F;
  u8 H;
  u8 L;

  CPU_Test_State() { ram.resize(0x10000, 0); }
};
Umibozu::SM83 setup_cpu(CPU_Test_State& test) {
  // Instantiate new instance of the CPU, populate fields with initial test
  // values
  Umibozu::SM83 cpu;
  cpu.test_memory = test.ram; //NOLINT

  cpu.PC = test.PC;
  cpu.SP = test.SP;
  cpu.A  = test.A;
  cpu.B  = test.B;
  cpu.C  = test.C;
  cpu.D  = test.D;
  cpu.E  = test.E;
  cpu.F  = test.F;
  cpu.H  = test.H;
  cpu.L  = test.L;

  return cpu;
}
void run_tests() {
  u32 completed_test_count;
  
  std::ifstream f;
  const std::string test_directory = "tests/sm83-test-data/cpu_tests/v1";
  std::vector<std::filesystem::path> test_dirs;
  
  if (!std::filesystem::exists(test_directory)) {
    throw std::runtime_error(
        "could not find cpu tests, are you sure sm83 data is present?");
  }
  
  // get all test files
  auto dir_it = std::filesystem::directory_iterator(test_directory);
  for (auto& entry : dir_it) {
    fmt::println("{}", entry.path().c_str());
    test_dirs.push_back(entry.path());
  }

  // run all the test cases per opcode
  for (auto& test_dir : test_dirs) {
    if(test_dir.filename().string() == "76.json") continue; // cannot be tested because interrupts are disabled
    std::string path = fmt::format(
        "{}", test_dir.string());
    fmt::print("opcode: {} ", test_dir.filename().replace_extension("").c_str());
    f.open(path);
    json data = json::parse(f);

    for (u32 testIdx = 0; auto& test : data.items()) {
      CPU_Test_State cpu_initial_test_state;    // our initial test state
      CPU_Test_State cpu_result_state;  // the desired state

      cpu_initial_test_state.name = test.value()["name"];
      cpu_initial_test_state.PC   = std::stoi(
          test.value()["initial"]["cpu"]["pc"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.SP = std::stoi(
          test.value()["initial"]["cpu"]["sp"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.A = std::stoi(
          test.value()["initial"]["cpu"]["a"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.B = std::stoi(
          test.value()["initial"]["cpu"]["b"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.C = std::stoi(
          test.value()["initial"]["cpu"]["c"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.D = std::stoi(
          test.value()["initial"]["cpu"]["d"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.E = std::stoi(
          test.value()["initial"]["cpu"]["e"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.F = std::stoi(
          test.value()["initial"]["cpu"]["f"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.H = std::stoi(
          test.value()["initial"]["cpu"]["h"].get_ref<std::string&>(), 0, 16);
      cpu_initial_test_state.L = std::stoi(
          test.value()["initial"]["cpu"]["l"].get_ref<std::string&>(), 0, 16);

      for (auto& ramEntry : test.value()["initial"]["ram"]) {
        u32 key   = std::stoi(ramEntry.at(0).get_ref<std::string&>(), 0, 16);
        u32 value = std::stoi(ramEntry.at(1).get_ref<std::string&>(), 0, 16);
        cpu_initial_test_state.ram[key] = value;
      }

      cpu_result_state.name = test.value()["name"];
      cpu_result_state.PC   = std::stoi(
          test.value()["final"]["cpu"]["pc"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.SP = std::stoi(
          test.value()["final"]["cpu"]["sp"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.A = std::stoi(
          test.value()["final"]["cpu"]["a"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.B = std::stoi(
          test.value()["final"]["cpu"]["b"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.C = std::stoi(
          test.value()["final"]["cpu"]["c"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.D = std::stoi(
          test.value()["final"]["cpu"]["d"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.E = std::stoi(
          test.value()["final"]["cpu"]["e"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.F = std::stoi(
          test.value()["final"]["cpu"]["f"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.H = std::stoi(
          test.value()["final"]["cpu"]["h"].get_ref<std::string&>(), 0, 16);
      cpu_result_state.L = std::stoi(
          test.value()["final"]["cpu"]["l"].get_ref<std::string&>(), 0, 16);

      for (auto& ramEntry : test.value()["final"]["ram"]) {
        u32 key   = std::stoi(ramEntry.at(0).get_ref<std::string&>(), 0, 16);
        u32 value = std::stoi(ramEntry.at(1).get_ref<std::string&>(), 0, 16);
        cpu_result_state.ram[key] = value;
      }

      auto cpu = setup_cpu(cpu_initial_test_state);

      cpu.run_instruction();

      if (cpu_result_state.PC != cpu.PC) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} program counter mismatch, "
                        "expected: {:#04X} actual: {:#04X}",
                        test_dir.filename().string(), testIdx, cpu_result_state.PC, cpu.PC));
      };
      if (cpu_result_state.SP != cpu.SP) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} stack pointer mismatch, "
                        "expected: {:#04X} actual: {:#04X}",
                        test_dir.filename().string(), testIdx, cpu_result_state.SP, cpu.SP));
      };
      if (cpu_result_state.A != cpu.A) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} accumulator mismatch, "
                        "expected: {:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.A, cpu.A));
      };
      if (cpu_result_state.B != cpu.B) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} B reg mismatch, expected: "
                        "{:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.B, cpu.B));
      };
      if (cpu_result_state.C != cpu.C) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} C reg mismatch, expected: "
                        "{:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.C, cpu.C));
      };

      if (cpu_result_state.D != cpu.D) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} D reg mismatch, expected: "
                        "{:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.D, cpu.D));
      };

      if (cpu_result_state.E != cpu.E) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} E reg mismatch, expected: "
                        "{:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.E, cpu.E));
      };

      if (cpu_result_state.H != cpu.H) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} H reg mismatch, expected: "
                        "{:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.H, cpu.H));
      };

      if (cpu_result_state.L != cpu.L) {
        throw std::runtime_error(
            fmt::format("[TEST] NAME: {}, I: {:d} L reg mismatch, expected: "
                        "{:#04x} actual: {:#04x}",
                        test_dir.filename().string(), testIdx, cpu_result_state.L, cpu.L));
      };

      for (u32 i = 0; auto& val : cpu_result_state.ram) {
        if (val != cpu.test_memory[i]) {
          fmt::println("t: ram[{:#04x}] = {:d} a: ram[{:#04x}] = {:d}", i, val,
                       i, cpu.test_memory[i]);
            assert(0);
        }
        i++;
      }

      completed_test_count++;
    }
    fmt::println("OK");
    f.close();
  }

  fmt::println("ALL GOOD! ran {:d} test cases", completed_test_count);
  
}

int main() {
  run_tests();
  return 0;
}
