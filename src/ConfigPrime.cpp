#include "llvm/Pass.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/ExecutionEngine/ExecutionEngine.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"

#include "interpreter/Interpreter.h"
#include "ConfigPrime.h"

using namespace llvm;

static cl::opt<std::string>
InputFile("Pconfig-prime-file",
	  cl::Hidden,
	  cl::desc("Specify input bitcode"));

static cl::list<std::string>
InputArgv("Pconfig-prime-input-arg",
	  cl::Hidden,
	  cl::desc("Specify one program argument"));

static cl::opt<unsigned>
UnknownArgs("Pconfig-prime-unknown-args",
	  cl::Hidden,
	  cl::init(0),
	  cl::desc("Specify the number of unknown parameters"));

namespace previrt {

std::vector<GenericValue> prepareArgumentsForMain(Function *mainFn,
						  // argc >= len(argv)
						  unsigned argc, 
						  const std::vector<std::string>& argv,
						  const std::vector<std::string>& envp,
						  ExecutionEngine &EE,
						  ArgvArray &CArgv,
						  ArgvArray &CEnv) {
  std::vector<GenericValue> GVArgs;
  GenericValue GVArgc;
  GVArgc.IntVal = APInt(32, argc);

  // Check main() type
  unsigned NumArgs = mainFn->getFunctionType()->getNumParams();
  FunctionType *FTy = mainFn->getFunctionType();
  Type* PPInt8Ty = Type::getInt8PtrTy(mainFn->getContext())->getPointerTo();

  // Check the argument types.
  if (NumArgs > 3)
    report_fatal_error("Invalid number of arguments of main() supplied");
  if (NumArgs >= 3 && FTy->getParamType(2) != PPInt8Ty)
    report_fatal_error("Invalid type for third argument of main() supplied");
  if (NumArgs >= 2 && FTy->getParamType(1) != PPInt8Ty)
    report_fatal_error("Invalid type for second argument of main() supplied");
  if (NumArgs >= 1 && !FTy->getParamType(0)->isIntegerTy(32))
    report_fatal_error("Invalid type for first argument of main() supplied");
  if (!FTy->getReturnType()->isIntegerTy() &&
      !FTy->getReturnType()->isVoidTy())
    report_fatal_error("Invalid return type of main() supplied");

  if (NumArgs) {
    // Arg #0 = argc.
    GVArgs.push_back(GVArgc); 
    if (NumArgs > 1) {
      // Arg #1 = argv.
      GVArgs.push_back(PTOGV(CArgv.reset(mainFn->getContext(), &EE, argv)));
      
      // assert(!isTargetNullPtr(this, GVTOP(GVArgs[1])) &&
      //        "argv[0] was null after CreateArgv");
      /// XXX: ignore for now envp
      //if (NumArgs > 2) {
      //   std::vector<std::string> EnvVars;
      //   for (unsigned i = 0; envp[i]; ++i)
      //     EnvVars.emplace_back(envp[i]);
      //   // Arg #2 = envp.
      // GVArgs.push_back(PTOGV(CEnv.reset(mainFn->getContext(), &EE, EnvVars)));
      //}
    }
  }
  return GVArgs;
}

ConfigPrime::ConfigPrime(): ModulePass(ID) {}

ConfigPrime::~ConfigPrime() {}

bool ConfigPrime::runOnModule(Module& M) {

  std::string ErrorMsg;
  
  std::unique_ptr<Module> M_ptr(&M);
  EngineBuilder builder(std::move(M_ptr));
  builder.setErrorStr(&ErrorMsg);
  builder.setEngineKind(EngineKind::Interpreter); // LLVM Interpreter
  
  std::unique_ptr<ExecutionEngine> EE(builder.create());
  if (!EE) {
    if (!ErrorMsg.empty())
      errs() << "Error creating EE: " << ErrorMsg << "\n";
    else
      errs() << "Unknown error creating EE!\n";
    return false;
  }

  if (Function* main=EE->FindFunctionNamed("main")) {

    // Run static constructors.    
    EE->runStaticConstructorsDestructors(false);

    // Run main
    std::vector<std::string> mainArgV;
    // Add the module's name to the start of the vector of arguments to main().    
    mainArgV.push_back(InputFile);
    unsigned i=1;
    for(auto a: InputArgv) {
      errs() << "ConfigPrime: reading argv[" << i++ << "] " << a << "\n";
      mainArgV.push_back(a);
    }
    unsigned argc = mainArgV.size() + UnknownArgs;
    std::vector<std::string> envp; /* unused */
    ArgvArray CArgv, CEnv; // they need to be alive while EE may use them
    std::vector<GenericValue> mainArgVGV =
      prepareArgumentsForMain(main, argc, mainArgV, envp, *EE, CArgv, CEnv);

    GenericValue Result = EE->runFunction(main,ArrayRef<GenericValue>(mainArgVGV));
    errs() << "ConfigPrime: execution of main returned with status " << Result.IntVal << "\n";

    // Run static destructors.    
    EE->runStaticConstructorsDestructors(true);

    // If the program didn't call exit explicitly, we should call it now.
    // This ensures that any atexit handlers get called correctly.
    auto &Context = M.getContext();
    Constant *Exit = M.getOrInsertFunction("exit", Type::getVoidTy(Context),
					   Type::getInt32Ty(Context));
    if (Function *ExitF = dyn_cast<Function>(Exit)) {
      std::vector<GenericValue> Args;
      Args.push_back(Result);
      EE->runFunction(ExitF, Args);
      errs() << "ERROR: exit(" << Result.IntVal << ") returned!\n";
      abort();
    } else {
      errs() << "ERROR: exit defined with wrong prototype!\n";
      abort();
    }

    // TODOX: Cast EE to Interpreter and use its internal state
  }

  return false;
}

void ConfigPrime::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

} // end namespace


char previrt::ConfigPrime::ID = 0;

static RegisterPass<previrt::ConfigPrime>
X("Pconfig-prime", "Configuration priming ", false, false);
  

