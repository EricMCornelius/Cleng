#include "serialization.hpp"

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>

#include <stdexcept>
#include <functional>

using namespace clang;

namespace {

class JsonASTPrinter : public ASTConsumer {
public:
  explicit JsonASTPrinter(ASTContext& context, JsonArray& output) : _context(context), _output(output) { }

  virtual void HandleTranslationUnit(clang::ASTContext& context) {
    //Context.getTranslationUnitDecl();
  }

  virtual bool HandleTopLevelDecl(DeclGroupRef g) {
    for (auto& decl : g) {
      JsonObject obj;
      dispatch_decl(obj, *decl, _context);
      _output.emplace_back(obj);
    }

    return true;
  }

private:
  ASTContext& _context;
  JsonArray& _output;
};

class PreprocessorCallbacks : public PPCallbacks {
public:
  PreprocessorCallbacks(Preprocessor& processor, JsonArray& output) : _processor(processor), _output(output) { }

  virtual void MacroDefined(const Token& identifier, const MacroInfo* info) {
    JsonObject obj;
    visit(obj, identifier, _processor);
    visit(obj, *info, _processor);
    _output.emplace_back(obj);
  }

  virtual void FileChanged(SourceLocation loc, FileChangeReason reason, SrcMgr::CharacteristicKind type, FileID id) {
    //std::cout << _processor.getSourceManager().getBufferName(loc) << std::endl;
  }

  virtual void EndOfMainFile() {
    //std::cout << "Done" << std::endl;
  }

private:
  Preprocessor& _processor;
  JsonArray& _output;
};

typedef std::map<std::string, std::function<void()>> CallbackMap;

CallbackMap initArgumentActions() {
  CallbackMap ArgumentActions;
  ArgumentActions["help"] = [](){ std::cout << "[help]" << std::endl; };
  return ArgumentActions;
}

CallbackMap ArgumentActions = initArgumentActions();

class PrintASTAction : public PluginASTAction {
protected:
  virtual ASTConsumer* CreateASTConsumer(CompilerInstance &Compiler, llvm::StringRef InFile) {
    static JsonASTPrinter printer(Compiler.getASTContext(), _output);
    return &printer;
  }

  virtual void ExecuteAction() {
    auto& pp = getCompilerInstance().getPreprocessor();
    static PreprocessorCallbacks cbs(pp, _output);
    pp.addPPCallbacks(&cbs);

    PluginASTAction::ExecuteAction();

    std::ofstream output("output.json");
    JsonOutStream out(output);
    format(out, _output);

  }

  bool ParseArgs(const CompilerInstance &CI,
                 const std::vector<std::string>& args) {

    for (auto& arg : args) {
      auto action = ArgumentActions.find(arg);
      if (action == ArgumentActions.end()) {
        DiagnosticsEngine &D = CI.getDiagnostics();
        unsigned DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "invalid argument '" + arg + "'");
        D.Report(DiagID);

        std::string opts;
        for (auto& action : ArgumentActions)
          opts += action.first + " ";
        DiagID = D.getCustomDiagID(DiagnosticsEngine::Error, "Allowed options: " + opts);
        D.Report(DiagID);
        return false;
      }
      else {
        action->second();
      }
    }

    return true;
  }

  JsonArray _output;
};

}

static FrontendPluginRegistry::Add<PrintASTAction>
X("cleng", "print type info");
