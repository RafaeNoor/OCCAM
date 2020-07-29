/** Insert dummy main function if one does not exist */

#include <string.h>
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Value.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Demangle/Demangle.h"

#include "seadsa/CompleteCallGraph.hh"
#include "seadsa/InitializePasses.hh"

//#include "seahorn/Support/SeaDebug.h"
#include "boost/format.hpp"

static llvm::cl::opt<std::string>
EntryPoint("entry-point",
        llvm::cl::desc ("Entry point if main does not exist"),
        llvm::cl::init (""));

namespace previrt {
    namespace transforms {


        using namespace llvm;


        class DummyMainFunction : public ModulePass
        {
            DenseMap<const Type*, Constant*> m_ndfn;

            std::vector<std::string> parseArgs(std::string str){
                std::vector<std::string> args;
                
                std::string arg = "";

                for(unsigned i=0;i<str.size(); i++){
                    if(str[i] == ','){
                        args.push_back(arg);
                        arg = "";
                    } else {
                        arg += str[i];
                    }
                }
                if(arg!="") args.push_back(arg);
                return args;
            }



            Function& makeNewNondetFn (Module &m, Type &type, unsigned num, std::string prefix)
            {
                std::string name;
                unsigned c = num;
                do
                    name = boost::str (boost::format (prefix + "%d") % (c++));
                while (m.getNamedValue (name));
                Function *res = dyn_cast<Function>(m.getOrInsertFunction (name, &type).getCallee());
                assert (res);
                return *res;
            }

            Constant* getNondetFn (Type *type, Module& M) {
                Constant* res = m_ndfn [type];
                if (!res) {
                    res = &makeNewNondetFn (M, *type, m_ndfn.size (), "verifier.nondet.");
                    m_ndfn[type] = res;
                }
                return res;
            }

            std::map<std::string, std::vector<std::string>> getFnMap(Module& m){
                std::map<std::string, std::vector<std::string>> fn_map;

                for(auto &F: m){
                    std::string demangled = demangle(F.getName());
                    demangled = demangled.substr(0,demangled.find_first_of("("));
                    
                    if(fn_map.find(demangled) == fn_map.end()){
                        std::vector<std::string> names;
                        fn_map[demangled] = names;
                    }

                    fn_map[demangled].push_back(F.getName());
                }

                return fn_map;
            }

            void printFnMapInfo(std::map<std::string, std::vector<std::string>> fn_map){
                errs()<<"|\tPrinting Function Name Map\t|\n";
                errs()<<"_______________________________________________\n";
                for(auto iter = fn_map.begin(),e = fn_map.end(); iter != e; iter++){
                    std::string fn = iter->first;
                    std::vector< std::string > vec = iter-> second;

                    errs()<<fn<<":\n";
                    for(auto nm: vec){
                        errs()<<"\t"<<nm<<"\n";
                    }
                    errs()<<"_______________________________________________\n";
                }
            }

            void makePrintf(Module& m, CallInst* ci, IRBuilder<> builder){

                Type* charType = Type::getInt8PtrTy(m.getContext());
                FunctionType *printf_type = FunctionType::get(charType,true);
                auto *res = cast<Function>(m.getOrInsertFunction("printf", printf_type).getCallee());

                assert(res && "printf not found in module");

                Type* type = res->getReturnType();

                // Testing with fixed type
                std::string type_fmt = "%d";


                SmallVector<Value*, 16> Args;

                std::string fmt = "Creating print call for " + type_fmt;
                Value* global_fmt = builder.CreateGlobalStringPtr(fmt.c_str());

                Args.push_back(global_fmt);
                Args.push_back((Value *) ci);

                CallInst* printFunc = builder.CreateCall(res, Args);

                errs() <<"DummyMainFunction: created print call "<< *printFunc << "\n";

            }

            public:

            static char ID;

            DummyMainFunction () : ModulePass (ID) {
                // Initialize sea-dsa pass
                llvm::PassRegistry &Registry = *llvm::PassRegistry::getPassRegistry();
                llvm::initializeCompleteCallGraphPass(Registry);
            }

            bool runOnModule (Module &M)
            {

                if (M.getFunction ("main")) { 
                    errs () << "DummyMainFunction: Main already exists.\n";
                    return false;
                }      

                auto fn_map = getFnMap(M);

                printFnMapInfo(fn_map);

                // Parse Comma seperated function names into a vector
                std::vector<std::string> EntryFunctionsNames = parseArgs(EntryPoint);
                SmallVector<Function*, 16> EntryFunctions;


                errs()<<"Invoked Dummy main on:\n";

                for(unsigned i=0;i<EntryFunctionsNames.size();i++){
                    errs()<<i<<"\t"<<EntryFunctionsNames[i]<<"\n";

                    if(EntryFunctionsNames[i] != "" && EntryFunctionsNames[i] != "none"){
                        if(fn_map.find(EntryFunctionsNames[i]) != fn_map.end()){
                            std::vector<std::string> mangled_names = fn_map.find(EntryFunctionsNames[i])->second;

                            for(auto fn_nm: mangled_names){
                                Function* fptr = M.getFunction(fn_nm);
                                if(fptr){ EntryFunctions.push_back(fptr); }
                                else{
                                    errs()<<"DummyMainFunction: This shouldn't have happend ;(\n";
                                    assert(false);
                                }
                            }

                        } else {
                            errs()<<"DummyMainFunction: "<<EntryFunctionsNames[i]<<" is not present in current module...\n";
                        }
                    }
                }

                if(!EntryFunctions.size()){
                    errs()<<"DummyMainFunction: None of the specified functions exist in this module, aborting pass...\n";
                    return false;
                }

                // --- Create main
                LLVMContext &ctx = M.getContext ();
                Type* intTy = Type::getInt32Ty (ctx);

                ArrayRef <Type*> params;
                Function *main = Function::Create (FunctionType::get (intTy, params, false), 
                        GlobalValue::LinkageTypes::ExternalLinkage, 
                        "main", &M);

                LLVMContext& C = main->getContext();
                MDNode* N = MDNode::get(C,MDString::get(C,"dummy"));
                main->setMetadata("dummy.metadata",N);

                IRBuilder<> B (ctx);
                BasicBlock *BB = BasicBlock::Create (ctx, "", main);
                B.SetInsertPoint (BB, BB->begin ());

                for (auto F: EntryFunctions){//FunctionsToCall) {
                    // -- create a call with non-deterministic actual parameters
                    SmallVector<Value*, 16> Args;
                    for (auto &A : F->args ()) {
                        Constant *ndf = getNondetFn (A.getType (), M);
                        Args.push_back (B.CreateCall (ndf));
                    }

                    CallInst* CI = B.CreateCall (F, Args);

                    errs () << "DummyMainFunction: created a call " << *CI << "\n";
                    makePrintf(M,CI,B);

                }

                // -- return of main
                // our favourite exit code
                B.CreateRet ( ConstantInt::get (intTy, 42));


                return true;
            }

            void getAnalysisUsage (AnalysisUsage &AU)
            { AU.setPreservesAll ();}

            virtual StringRef getPassName () const 
            {return "Add dummy main function";}
        };


        char DummyMainFunction::ID = 0;

        Pass* createDummyMainFunctionPass (){
            return new DummyMainFunction ();
        }




    } // end namespace   
} // end namespace   

static llvm::RegisterPass<previrt::transforms::DummyMainFunction>
X("LibToMain",
        "Convert a library bitcode into a standalone module with a main function which has non-deterministic calls to certain functions",false,false);



