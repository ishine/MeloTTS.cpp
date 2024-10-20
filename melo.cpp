﻿
#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#endif

//#define DEBUG_MEMORY
#if defined(_WIN32) && defined(DEBUG_MEMORY)
#define PSAPI_VERSION 1 // PrintMemoryInfo 
#include <psapi.h>
#pragma comment(lib,"psapi.lib") //PrintMemoryInfod
#include "processthreadsapi.h"
#endif



#include "utils.h"
#include "tts.h"
#include "chinese_mix.h"
#include "parse_args.h"

#if defined(_WIN32) && defined(DEBUG_MEMORY)
// To ensure correct resolution of symbols, add Psapi.lib to TARGETLIBS
// and compile with -DPSAPI_VERSION=1
static void DebugMemoryInfo(const char* header)
{
    PROCESS_MEMORY_COUNTERS_EX2 pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
    {
        //The following printout corresponds to the value of Resource Memory, respectively
        printf("%s:\tCommit \t\t\t=  0x%08X- %u (KB)\n", header, pmc.PrivateUsage, pmc.PrivateUsage / 1024);
        printf("%s:\tWorkingSetSize\t\t\t=  0x%08X- %u (KB)\n", header, pmc.WorkingSetSize, pmc.WorkingSetSize / 1024);
        printf("%s:\tPrivateWorkingSetSize\t\t\t=  0x%08X- %u (KB)\n", header, pmc.PrivateWorkingSetSize, pmc.PrivateWorkingSetSize / 1024);
    }
}
#endif
int main(int argc, char** argv)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
#endif

    ConfigureOneDNNCache();
    Args args = parse_args(argc, argv);

    std::filesystem::path input_path = args.input_file;
    std::filesystem::path output_path = args.output_file;

    //Init lanugage module
    melo::chinese_mix::cmudict = std::make_shared<melo::CMUDict>(args.cmudict_path.string());
    melo::chinese_mix::jieba = std::make_shared<cppjieba::Jieba>(args.cppjieba_dict);
    melo::chinese_mix::pinyin_to_symbol_map = melo::chinese_mix::readPinyinFile(args.pinyin_to_symbol_map_path);
    melo::chinese_mix::pinyin = std::make_shared<cppinyin::PinyinEncoder>(args.cppinyin_resource);
    std::cout <<"Init language Module\n";

    // Init core
    std::unique_ptr<ov::Core> core_ptr = std::make_unique<ov::Core>();
    auto startTime = Time::now();
    melo::TTS model(core_ptr, args.zh_tts_path,"CPU",args.zh_bert_path,"CPU",args.vocab_bert_path, args.punc_dict_path, "ZH", false);
    auto initTime = get_duration_ms_till_now(startTime);
    std::cout << "model init time is" << initTime <<" ms" << std::endl;

    std::vector<std::string> texts = read_file_lines(input_path);
    for(int i = 0;i<1;++i){
        startTime = Time::now();
        model.tts_to_file(texts, output_path, 1, 0.95);
        auto inferTime = get_duration_ms_till_now(startTime);
        std::cout << "model infer time:" << inferTime << " ms"<< std::endl;
    }

}
