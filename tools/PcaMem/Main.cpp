

#include <llvm-c/Core.h>  
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>	 
#include <llvm/Support/FileSystem.h>	 
#include <llvm/Bitcode/BitcodeWriterPass.h>  
#include <llvm/IR/LegacyPassManager.h>		 
#include <llvm/Support/Signals.h>	 
#include <llvm/IRReader/IRReader.h>	 
#include <llvm/Support/ToolOutputFile.h>  
#include <llvm/Support/PrettyStackTrace.h>  
#include <llvm/IR/LLVMContext.h>		 
#include <llvm/Support/SourceMgr.h>  
#include <llvm/Bitcode/BitcodeWriterPass.h>
#include <llvm/IR/DataLayout.h>
#include "llvmadpt/ModuleSet.h"
#include "MemCheck.h"
#include "common/Bitmap.h"
#include "common/SoftPara.h"
#include "common/Stat.h"



using namespace llvm;
using namespace std;


static cl::opt<std::string>
InputDirectory("dir", cl::desc("<input bitcode file directory>"), cl::value_desc("directory"));

static cl::opt<std::string>
InputFilename("file", cl::desc("<input bitcode file>"), cl::init("-"), cl::value_desc("filename"));

static cl::opt<std::string>
PreProcess("pre", cl::desc("<preprocess before analysis >"), cl::init("0"), cl::value_desc("switch"));

static cl::opt<std::string>
MemLeakTest("memleak-cases automatically test", cl::desc("<memleak-cases automatically test>"), cl::init("0"), cl::value_desc("memory leak test"));

static llvm::cl::opt<string> DumpDfg("dump-DDG", cl::init("0"), cl::desc("Dump dot graph of DDG"));



VOID GetModulePath (vector<string> &ModulePathVec)
{   
    if (InputFilename == "" || InputDirectory == "-")
    {
        errs() << "InputFilename is NULL \n ";
        return;
    }

    if (InputFilename != "-" && InputFilename != "")
    {
        std::string ModuleName = InputFilename;
        ModulePathVec.push_back (ModuleName);
    }
    else
    {  
        std::string ModuleDir = InputDirectory;       
        ModulePath ModulePt;        
        ModulePt.Visit(ModuleDir.c_str(), &ModulePathVec);
    }

    return;
}

string GetCaseName (string FileName)
{   
    INT Pos = FileName.find(".bc");
    if(Pos == -1)
    {
        return NULL;
    }

    return FileName.substr(0, Pos);
}

VOID RunPasses (vector<string> &ModulePathVec)
{
    Stat::StartTime ("LoadModule");
    ModuleManage ModuleMng (ModulePathVec);
    Stat::EndTime ("LoadModules");

    if (llaf::GetParaValue (PARA_PREPROFESS) == "1")
    {
        return;
    }

    Stat::StartTime ("Andersen");
    PointsTo PtsTo (ModuleMng, T_ANDRESEN);   
    Stat::EndTime ("Andersen");

    string CaseName = "";
    if (MemLeakTest == "1")
    {
        CaseName = GetCaseName(ModulePathVec[0]);
    }

    MemCheck McPass (CaseName);
    McPass.runOnModule (ModuleMng);

    printf("Total Memory usage:%u (K)\r\n", Stat::GetPhyMemUse ());

    return;
}

VOID GetParas(int argc, char ** argv)
{ 
    cl::ParseCommandLineOptions(argc, argv, "call graph analysis\n");

    if (PreProcess != "")
    {
        std::string Para  = PARA_PREPROFESS;
        std::string Value = PreProcess;
        llaf::SetParaValue (Para, Value);    
    }

    if (DumpDfg != "")
    {
        std::string Para  = PARA_DDG_DUMP;
        std::string Value = DumpDfg;
        llaf::SetParaValue (Para, Value);    
    }

    return;
}

int main(int argc, char ** argv) 
{ 
    vector<string> ModulePathVec;
    Stat st;
    
    PassRegistry &Registry = *PassRegistry::getPassRegistry();

    initializeCore(Registry);
    initializeScalarOpts(Registry);
    initializeIPO(Registry);
    initializeAnalysis(Registry);
    initializeTransformUtils(Registry);
    initializeInstCombine(Registry);
    initializeInstrumentation(Registry);
    initializeTarget(Registry);

    GetParas(argc, argv);
  
    GetModulePath(ModulePathVec);
    if (ModulePathVec.size() == 0)
    {
        errs()<<"get none module paths!!\n";
        return 0;
    }
    
    RunPasses (ModulePathVec);

    return 0;
}

