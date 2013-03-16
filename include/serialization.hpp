#pragma once

#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/AST.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Token.h"
#include "clang/Lex/MacroInfo.h"
#include "clang/Lex/PPCallbacks.h"
#include "clang/AST/ASTContext.h"
#include "clang/Lex/Preprocessor.h"

#include <serializer/json/json.h>
#include <serializer/json/impl.h>
#include <serializer/string_escaper.h>

#include <iostream>

typedef std::pair<std::string, std::string> AV;
typedef std::vector<AV> AVV;
typedef std::map<std::string, AVV> EAV;
typedef std::map<std::string, std::string> EV;
typedef std::map<std::string, clang::PresumedLoc> LM;

template <>
struct format_override<AV, JsonOutStream> {
  static void format(JsonOutStream& out, const AV& obj) {
    out << "{\"" << obj.first << "\":" << "\"" << escape_string(obj.second) << "\"}";
  }
};

template <typename Type, typename Context>
struct JsonVisitor;

template <typename Decl, typename Context>
void visit(JsonObject& obj, const Decl& decl, const Context& ctx) {
  JsonVisitor<Decl, Context>::visit(obj, decl, ctx);
}

template <typename Type, typename Context>
struct JsonVisitor<Type*, Context> {
  static void visit(JsonObject& obj, Type* decl, const Context& ctx) {
    ::visit(obj, *decl, ctx);
  }
};

#define VISIT_SPEC(Type, ...)                                                  \
template <typename Context>                                                    \
struct JsonVisitor<Type, Context> {                                            \
  static void visit(JsonObject& obj, const Type& decl, const Context& ctx) {   \
    __VA_ARGS__                                                                \
  }                                                                            \
};

#define DEFAULT_VISIT_SPEC(Type) \
VISIT_SPEC(Type, obj["internal_type"] = #Type;);

DEFAULT_VISIT_SPEC(clang::AccessSpecDecl);

DEFAULT_VISIT_SPEC(clang::BlockDecl);

VISIT_SPEC(clang::ClassScopeFunctionSpecializationDecl,
  obj["specialization"] = true;
);

DEFAULT_VISIT_SPEC(clang::FileScopeAsmDecl);

DEFAULT_VISIT_SPEC(clang::FriendDecl);

DEFAULT_VISIT_SPEC(clang::FriendTemplateDecl);

DEFAULT_VISIT_SPEC(clang::ImportDecl);

DEFAULT_VISIT_SPEC(clang::LinkageSpecDecl);

DEFAULT_VISIT_SPEC(clang::LabelDecl);

DEFAULT_VISIT_SPEC(clang::NamespaceDecl);

DEFAULT_VISIT_SPEC(clang::NamespaceAliasDecl);

DEFAULT_VISIT_SPEC(clang::ObjCCompatibleAliasDecl);

DEFAULT_VISIT_SPEC(clang::ObjCCategoryDecl);

DEFAULT_VISIT_SPEC(clang::ObjCCategoryImplDecl);

DEFAULT_VISIT_SPEC(clang::ObjCImplementationDecl);

DEFAULT_VISIT_SPEC(clang::ObjCInterfaceDecl);

DEFAULT_VISIT_SPEC(clang::ObjCProtocolDecl);

DEFAULT_VISIT_SPEC(clang::ObjCMethodDecl);

DEFAULT_VISIT_SPEC(clang::ObjCPropertyDecl);

DEFAULT_VISIT_SPEC(clang::ClassTemplateDecl);

DEFAULT_VISIT_SPEC(clang::FunctionTemplateDecl);

DEFAULT_VISIT_SPEC(clang::TypeAliasTemplateDecl);

DEFAULT_VISIT_SPEC(clang::TemplateTemplateParmDecl);

DEFAULT_VISIT_SPEC(clang::EnumDecl);

DEFAULT_VISIT_SPEC(clang::RecordDecl);

DEFAULT_VISIT_SPEC(clang::CXXRecordDecl);

DEFAULT_VISIT_SPEC(clang::ClassTemplateSpecializationDecl);

DEFAULT_VISIT_SPEC(clang::ClassTemplatePartialSpecializationDecl);

DEFAULT_VISIT_SPEC(clang::TemplateTypeParmDecl);

DEFAULT_VISIT_SPEC(clang::TypeAliasDecl);

VISIT_SPEC(clang::TypedefDecl,
  obj["type"] = decl.getUnderlyingType().getAsString();
);

DEFAULT_VISIT_SPEC(clang::UnresolvedUsingTypenameDecl);

DEFAULT_VISIT_SPEC(clang::UsingDecl);

DEFAULT_VISIT_SPEC(clang::UsingDirectiveDecl);

DEFAULT_VISIT_SPEC(clang::UsingShadowDecl);

VISIT_SPEC(clang::FieldDecl,
  obj["type"] = decl.getType().getAsString();
);

DEFAULT_VISIT_SPEC(clang::ObjCAtDefsFieldDecl);

DEFAULT_VISIT_SPEC(clang::ObjCIvarDecl);

VISIT_SPEC(clang::FunctionDecl,
  obj["isInlineSpecified"] = decl.isInlineSpecified();
  obj["isOutOfLine"] = decl.isOutOfLine();

  if (decl.isInlined()) {
    obj["isInlined"] = true;
    obj["isInlineDefinitionExternallyVisible"] = decl.isInlineDefinitionExternallyVisible();
  }
  else
    obj["isInlined"] = false;

  if (decl.isOverloadedOperator()) {
    obj["isOverloadedOperator"] = true;
    //obj["isReservedGlobalPlacementOperator"] = decl.isReservedGlobalPlacementOperator();
  }
  else
    obj["isOverloadedOperator"] = false;

  obj["isFunctionTemplateSpecialization"] = decl.isFunctionTemplateSpecialization();
  obj["isTemplateInstantiation"] = decl.isTemplateInstantiation();
  obj["hasBody"] = decl.hasBody();
  obj["hasTrivialBody"] = decl.hasTrivialBody();
  obj["isThisDeclarationADefinition"] = decl.isThisDeclarationADefinition();
  obj["doesThisDeclarationHaveABody"] = decl.doesThisDeclarationHaveABody();
  obj["isDefined"] = decl.isDefined();
  obj["isVariadic"] = decl.isVariadic();
  obj["isVirtualAsWritten"] = decl.isVirtualAsWritten();
  obj["isPure"] = decl.isPure();
  obj["isLateTemplateParsed"] = decl.isLateTemplateParsed();
  obj["isTrivial"] = decl.isTrivial();
  obj["isDefaulted"] = decl.isDefaulted();
  obj["isExplicitlyDefaulted"] = decl.isExplicitlyDefaulted();
  obj["hasImplicitReturnZero"] = decl.hasImplicitReturnZero();
  obj["hasPrototype"] = decl.hasPrototype();
  obj["hasWrittenPrototype"] = decl.hasWrittenPrototype();
  obj["hasInheritedPrototype"] = decl.hasInheritedPrototype();
  obj["isConstexpr"] = decl.isConstexpr();
  obj["isDeleted"] = decl.isDeleted();
  obj["isMain"] = decl.isMain();
  obj["isExternC"] = decl.isExternC();
  obj["isGlobal"] = decl.isGlobal();
  obj["hasSkippedBody"] = decl.hasSkippedBody();
  obj["isImplicitlyInstantiable"] = decl.isImplicitlyInstantiable();

  obj["resultType"] = decl.getResultType().getAsString();
  JsonArray params;
  for (auto itr = decl.param_begin(); itr != decl.param_end(); ++itr) {
    JsonObject param;
    ::visit(param, *itr, ctx);
    params.emplace_back(param);
  }
  obj["params"] = params;
);

DEFAULT_VISIT_SPEC(clang::CXXMethodDecl);

DEFAULT_VISIT_SPEC(clang::CXXConstructorDecl);

DEFAULT_VISIT_SPEC(clang::CXXConversionDecl);

DEFAULT_VISIT_SPEC(clang::CXXDestructorDecl);

DEFAULT_VISIT_SPEC(clang::NonTypeTemplateParmDecl);

VISIT_SPEC(clang::VarDecl, 
  obj["type"] = decl.getType().getAsString();
);

DEFAULT_VISIT_SPEC(clang::ImplicitParamDecl);

VISIT_SPEC(clang::ParmVarDecl,
  obj["name"] = decl.getNameAsString();
  obj["type"] = decl.getOriginalType().getAsString();
);

DEFAULT_VISIT_SPEC(clang::EnumConstantDecl);

DEFAULT_VISIT_SPEC(clang::IndirectFieldDecl);

DEFAULT_VISIT_SPEC(clang::UnresolvedUsingValueDecl);

DEFAULT_VISIT_SPEC(clang::ObjCPropertyImplDecl);

DEFAULT_VISIT_SPEC(clang::StaticAssertDecl);

DEFAULT_VISIT_SPEC(clang::TranslationUnitDecl);

VISIT_SPEC(clang::SourceRange,
  JsonObject begin;
  ::visit(begin, decl.getBegin(), ctx);
  obj["begin"] = begin;

  JsonObject end;
  ::visit(end, decl.getEnd(), ctx);
  obj["end"] = end;
);

VISIT_SPEC(clang::SourceLocation,
  const auto& sm = ctx.getSourceManager();
  clang::FullSourceLoc loc(decl, sm);
  if (loc.isValid()) {
    obj["line"] = JsonNumber(loc.getSpellingLineNumber());
    obj["column"] = JsonNumber(loc.getSpellingColumnNumber());

    const auto fileEntry = sm.getFileEntryForID(loc.getFileID());
    if (fileEntry != nullptr) {
      obj["file"] = fileEntry->getName();
      const auto dirEntry = fileEntry->getDir();
      if (dirEntry != nullptr)
        obj["dir"] = dirEntry->getName();
    }
  }
);

template <typename Context> 
struct JsonVisitor<clang::Decl, Context> {
  static void visit(JsonObject& obj, const clang::Decl& decl, const Context& ctx) {
    obj["node_type"] = decl.getDeclKindName();
    if (clang::NamedDecl::classof(&decl)) {
      obj["name"] = static_cast<const clang::NamedDecl&>(decl).getNameAsString();
    }

    JsonObject src;
    ::visit(src, decl.getSourceRange(), ctx);
    obj["sourceRange"] = src;

    switch (decl.getKind()) {
      #define ABSTRACT_DECL(DECL)
      #define DECL(CLASS, BASE) \
      case clang::Decl::CLASS: { \
        ::visit(obj, static_cast<const clang::CLASS##Decl&>(decl), ctx); \
        break; \
      }
      #include "clang/AST/DeclNodes.inc"
    }

    if (clang::DeclContext::classof(&decl)) {
      const auto declCtx = clang::Decl::castToDeclContext(&decl);
      JsonArray arr;
      for (auto itr = declCtx->decls_begin(); itr != declCtx->decls_end(); ++itr) {
        JsonObject child;
        ::visit(child, *itr, ctx);
        arr.emplace_back(child);
      }
      obj["context"] = arr;
    }
  }                                                              
};