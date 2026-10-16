/// @file main.cpp
/// @brief MicroArchSim command-line interface.

#include "types.h"
#include "config.h"
#include "assembler.h"
#include "reference_interpreter.h"
#include "pipeline.h"
#include "statistics.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>

using namespace microarchsim;

// ============================================================================
// Help text
// ============================================================================

static void print_help(const char* program_name) {
    std::cout << "MicroArchSim v1.0 — Cycle-Accurate CPU Pipeline Simulator\n\n"
              << "Usage: " << program_name << " [options]\n\n"
              << "Required:\n"
              << "  --program <path>        Path to assembly program file\n\n"
              << "Execution mode:\n"
              << "  --mode <mode>           reference | pipeline (default: pipeline)\n\n"
              << "Pipeline options:\n"
              << "  --forwarding <on|off>   Enable/disable data forwarding (default: on)\n"
              << "  --trace                 Print cycle-by-cycle pipeline trace\n\n"
              << "Cache options:\n"
              << "  --cache-size <bytes>    Cache size in bytes (default: 1024)\n"
              << "  --line-size <bytes>     Cache line size in bytes (default: 16)\n"
              << "  --associativity <N>     Cache associativity (default: 1 = direct-mapped)\n"
              << "  --hit-latency <cycles>  Cache hit latency (default: 1)\n"
              << "  --miss-penalty <cycles> Cache miss penalty (default: 10)\n"
              << "  --replacement <policy>  lru | fifo | random (default: lru)\n"
              << "  --write-policy <policy> writeback | writethrough (default: writeback)\n"
              << "  --no-cache              Disable data cache\n\n"
              << "Output options:\n"
              << "  --stats-csv <path>      Export statistics to CSV file\n"
              << "  --dump-registers        Print final register state\n"
              << "  --dump-memory           Print non-zero data memory\n"
              << "  --help                  Show this help message\n\n"
              << "Examples:\n"
              << "  " << program_name << " --program programs/data_hazards.asm --mode pipeline\n"
              << "  " << program_name << " --program programs/matrix.asm --mode reference\n"
              << "  " << program_name << " --program programs/memory_test.asm --cache-size 4096 --associativity 2\n";
}

// ============================================================================
// Argument parsing
// ============================================================================

static SimConfig parse_args(int argc, char* argv[]) {
    SimConfig config;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        auto next_arg = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw SimulatorError("Missing value for " + arg);
            }
            return argv[++i];
        };

        if (arg == "--help" || arg == "-h") {
            print_help(argv[0]);
            std::exit(0);
        }
        else if (arg == "--program") {
            config.program_path = next_arg();
        }
        else if (arg == "--mode") {
            std::string mode = next_arg();
            if (mode == "reference") config.mode = SimMode::REFERENCE;
            else if (mode == "pipeline") config.mode = SimMode::PIPELINE;
            else throw SimulatorError("Unknown mode: " + mode);
        }
        else if (arg == "--forwarding") {
            std::string val = next_arg();
            if (val == "on") config.forwarding_enabled = true;
            else if (val == "off") config.forwarding_enabled = false;
            else throw SimulatorError("--forwarding must be 'on' or 'off'");
        }
        else if (arg == "--trace") {
            config.trace_enabled = true;
        }
        else if (arg == "--cache-size") {
            config.cache.cache_size = static_cast<uint32_t>(std::stoul(next_arg()));
        }
        else if (arg == "--line-size") {
            config.cache.line_size = static_cast<uint32_t>(std::stoul(next_arg()));
        }
        else if (arg == "--associativity") {
            config.cache.associativity = static_cast<uint32_t>(std::stoul(next_arg()));
        }
        else if (arg == "--hit-latency") {
            config.cache.hit_latency = static_cast<uint32_t>(std::stoul(next_arg()));
        }
        else if (arg == "--miss-penalty") {
            config.cache.miss_penalty = static_cast<uint32_t>(std::stoul(next_arg()));
        }
        else if (arg == "--replacement") {
            std::string val = next_arg();
            if (val == "lru") config.cache.replacement = ReplacementPolicy::LRU;
            else if (val == "fifo") config.cache.replacement = ReplacementPolicy::FIFO;
            else if (val == "random") config.cache.replacement = ReplacementPolicy::RANDOM;
            else throw SimulatorError("Unknown replacement policy: " + val);
        }
        else if (arg == "--write-policy") {
            std::string val = next_arg();
            if (val == "writeback") config.cache.write_policy = WritePolicy::WRITE_BACK;
            else if (val == "writethrough") config.cache.write_policy = WritePolicy::WRITE_THROUGH;
            else throw SimulatorError("Unknown write policy: " + val);
        }
        else if (arg == "--no-cache") {
            config.cache_enabled = false;
        }
        else if (arg == "--stats-csv") {
            config.stats_csv_path = next_arg();
        }
        else if (arg == "--dump-registers") {
            config.dump_registers = true;
        }
        else if (arg == "--dump-memory") {
            config.dump_memory = true;
        }
        else {
            throw SimulatorError("Unknown option: " + arg);
        }
    }

    return config;
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    try {
        SimConfig config = parse_args(argc, argv);
        config.validate();

        // Assemble the program
        Assembler assembler;
        AssemblyResult program = assembler.assemble_file(config.program_path);

        std::cout << "Loaded " << program.instructions.size() << " instructions";
        if (!program.data.empty()) {
            std::cout << " and " << program.data.size() << " data words";
        }
        std::cout << " from " << config.program_path << "\n";

        if (config.mode == SimMode::REFERENCE) {
            // --- Reference Interpreter ---
            std::cout << "Running in REFERENCE mode...\n";

            ReferenceInterpreter interp(config.memory_size);
            interp.load_program(program.instructions);
            if (!program.data.empty()) {
                interp.load_data(program.data);
            }

            uint64_t count = interp.run();

            std::cout << "Execution complete: " << count << " instructions executed.\n";

            if (config.dump_registers) {
                std::cout << "\n--- Register State ---\n";
                interp.registers().dump(std::cout);
            }

            if (config.dump_memory) {
                std::cout << "\n--- Data Memory (non-zero) ---\n";
                interp.memory().dump(std::cout, DATA_BASE, DATA_BASE + 1024);
            }

        } else {
            // --- Pipeline Mode ---
            std::cout << "Running in PIPELINE mode";
            if (config.forwarding_enabled)
                std::cout << " (forwarding: ON)";
            else
                std::cout << " (forwarding: OFF)";
            if (config.cache_enabled)
                std::cout << " (cache: " << config.cache.cache_size << "B, "
                          << config.cache.associativity << "-way)";
            else
                std::cout << " (no cache)";
            std::cout << "\n";

            Pipeline pipeline(config);
            pipeline.load_program(program.instructions);
            if (!program.data.empty()) {
                pipeline.load_data(program.data);
            }

            pipeline.run();

            std::cout << "Execution complete.\n";

            // Print statistics
            Statistics stats = pipeline.get_stats();
            stats.print_summary(std::cout);

            if (config.dump_registers) {
                std::cout << "\n--- Register State ---\n";
                pipeline.registers().dump(std::cout);
            }

            if (config.dump_memory) {
                std::cout << "\n--- Data Memory (non-zero) ---\n";
                pipeline.data_memory().dump(std::cout, DATA_BASE, DATA_BASE + 1024);
            }

            // CSV export
            if (!config.stats_csv_path.empty()) {
                // Check if file already exists and has content
                std::ifstream check_file(config.stats_csv_path);
                bool write_header = !check_file.good() || check_file.peek() == std::ifstream::traits_type::eof();
                check_file.close();

                std::ofstream csv_file(config.stats_csv_path, std::ios::app);
                if (!csv_file.is_open()) {
                    std::cerr << "Warning: Could not open CSV file: "
                              << config.stats_csv_path << "\n";
                } else {
                    // Construct config label
                    std::string config_label = "pipeline";
                    if (config.forwarding_enabled) config_label += "_fwd";
                    else config_label += "_nofwd";
                    if (config.cache_enabled) {
                        config_label += "_cache" + std::to_string(config.cache.cache_size) +
                                        "_" + std::to_string(config.cache.associativity) + "way";
                    } else {
                        config_label += "_nocache";
                    }

                    stats.export_csv(csv_file, write_header, config.program_path, config_label);
                    std::cout << "Statistics exported to " << config.stats_csv_path << "\n";
                }
            }
        }

    } catch (const SimulatorError& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
