/** Insert dummy main function if one does not exist */

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

            errs()<<"Invoked Dummy main on:"<<EntryPoint<<"\n";
            Function* Entry = nullptr;
            if (EntryPoint != "" && EntryPoint != "none")
                Entry = M.getFunction (EntryPoint);

            // --- Create main
            LLVMContext &ctx = M.getContext ();
            Type* intTy = Type::getInt32Ty (ctx);

            ArrayRef <Type*> params;
            Function *main = Function::Create (FunctionType::get (intTy, params, false), 
                    GlobalValue::LinkageTypes::ExternalLinkage, 
                    "main", &M);

            IRBuilder<> B (ctx);
            BasicBlock *BB = BasicBlock::Create (ctx, "", main);
            B.SetInsertPoint (BB, BB->begin ());

            std::vector<Function*> FunctionsToCall;
            if (Entry) {  
                FunctionsToCall.push_back (Entry);
            } else { 
                // --- if no selected entry found then we call to all
                //     non-declaration functions.
                for (auto &F: M) {
                    if (F.getName () == "main") // avoid recursive call to main
                        continue;
                    if (F.isDeclaration ())
                        continue;
                    FunctionsToCall.push_back (&F);
                }
            }

            for (auto F: FunctionsToCall) {
                // -- create a call with non-deterministic actual parameters
                SmallVector<Value*, 16> Args;
                for (auto &A : F->args ()) {
                    Constant *ndf = getNondetFn (A.getType (), M);
                    Args.push_back (B.CreateCall (ndf));
                }
                CallInst* CI = B.CreateCall (F, Args);
                errs () << "DummyMainFunction: created a call " << *CI << "\n";
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
                "Convert a library bitcode into a standalone module with a main function which has non-deterministic calls to certain functions");



