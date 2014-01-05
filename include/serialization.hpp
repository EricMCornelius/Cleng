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

template <typename Type, typename Context>
struct JsonVisitor;

template <typename Decl, typename Context>
void visit(json::Object& obj, const Decl& decl, const Context& ctx) {
  JsonVisitor<Decl, Context>::visit(obj, decl, ctx);
}

template <typename Context> 
void dispatch_decl(json::Object& obj, const clang::Decl& decl, const Context& ctx) { 
  switch (decl.getKind()) {
    #define ABSTRACT_DECL(DECL)
    #define DECL(CLASS, BASE) \
    case clang::Decl::CLASS: { \
      ::visit(obj, static_cast<const clang::CLASS##Decl&>(decl), ctx); \
      break; \
    }
    #include "clang/AST/DeclNodes.inc"
  }
};

template <typename Type, typename Context>
struct JsonVisitor<Type*, Context> {
  static void visit(json::Object& obj, Type* decl, const Context& ctx) {
    ::visit(obj, *decl, ctx);
  }
};

#define VISIT_SPEC(Type, ...)                                                  \
template <typename Context>                                                    \
struct JsonVisitor<Type, Context> {                                            \
  static void visit(json::Object& obj, const Type& decl, const Context& ctx) {   \
    __VA_ARGS__                                                                \
  }                                                                            \
};

#define DEFAULT_VISIT_SPEC(Type) \
VISIT_SPEC(Type, obj["internal_type"] = #Type;);

DEFAULT_VISIT_SPEC(clang::AccessSpecDecl);

DEFAULT_VISIT_SPEC(clang::BlockDecl);

VISIT_SPEC(clang::ClassScopeFunctionSpecializationDecl,
  obj["specialization"] = true;

  ::visit(obj, static_cast<const clang::Decl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::FileScopeAsmDecl);

DEFAULT_VISIT_SPEC(clang::FriendDecl);

DEFAULT_VISIT_SPEC(clang::FriendTemplateDecl);

DEFAULT_VISIT_SPEC(clang::ImportDecl);

VISIT_SPEC(clang::LinkageSpecDecl,
  obj["language"] = (decl.getLanguage() == clang::LinkageSpecDecl::lang_cxx) ? "c++" : "c";

  ::visit(obj, static_cast<const clang::Decl&>(decl), ctx);
  ::visit(obj, static_cast<const clang::DeclContext&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::LabelDecl);

VISIT_SPEC(clang::NamespaceDecl,
  obj["isAnonymousNamespace"] = decl.isAnonymousNamespace();
  obj["isInline"] = decl.isInline();
  obj["isOriginalNamespace"] = decl.isOriginalNamespace();

  ::visit(obj, static_cast<const clang::NamedDecl&>(decl), ctx);
  ::visit(obj, static_cast<const clang::DeclContext&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::NamespaceAliasDecl);

DEFAULT_VISIT_SPEC(clang::ObjCCompatibleAliasDecl);

DEFAULT_VISIT_SPEC(clang::ObjCCategoryDecl);

DEFAULT_VISIT_SPEC(clang::ObjCCategoryImplDecl);

DEFAULT_VISIT_SPEC(clang::ObjCImplementationDecl);

DEFAULT_VISIT_SPEC(clang::ObjCInterfaceDecl);

DEFAULT_VISIT_SPEC(clang::ObjCProtocolDecl);

DEFAULT_VISIT_SPEC(clang::ObjCMethodDecl);

DEFAULT_VISIT_SPEC(clang::ObjCPropertyDecl);

DEFAULT_VISIT_SPEC(clang::CapturedDecl);

DEFAULT_VISIT_SPEC(clang::EmptyDecl);

DEFAULT_VISIT_SPEC(clang::MSPropertyDecl);

DEFAULT_VISIT_SPEC(clang::OMPThreadPrivateDecl);

VISIT_SPEC(clang::TemplateArgumentList,
  json::Array arguments;
  for (auto idx = 0; idx < decl.size(); ++idx) {
    json::Object argument;
    ::visit(argument, decl[idx], ctx);
    arguments.emplace_back(argument);
  }
  obj["templateArguments"] = arguments;
);

VISIT_SPEC(clang::TemplateArgument,
  obj["isNull"] = decl.isNull();
  obj["isDependent"] = decl.isDependent();
  obj["isInstantiationDependent"] = decl.isInstantiationDependent();
  obj["containsUnexpandedParameterPack"] = decl.containsUnexpandedParameterPack();
  obj["isPackExpansion"] = decl.isPackExpansion();
  obj["type"] = decl.getAsType().getAsString();

  const auto val = decl.getAsDecl();
  if (val)
    ::visit(obj, *val, ctx);

  obj["isDeclForReferenceParam"] = decl.isDeclForReferenceParam();

  if (decl.pack_size() > 0) {
    json::Array parameter_pack;
    for (auto itr = decl.pack_begin(); itr != decl.pack_end(); ++itr) {
      json::Object param;
      ::visit(param, *itr, ctx);
      parameter_pack.emplace_back(param);
    }
    obj["parameterPack"] = parameter_pack;
  }
);

VISIT_SPEC(clang::TemplateParameterList,
  json::Array parameters;
  for (const auto& declItr : decl) {
    json::Object parameter;
    ::visit(parameter, *declItr, ctx);
    parameters.emplace_back(parameter);
  }
  obj["templateParameters"] = parameters;
);

VISIT_SPEC(clang::TemplateDecl,
  const auto templateParamList = decl.getTemplateParameters();
  if (templateParamList != nullptr)
    ::visit(obj, *templateParamList, ctx);

  ::visit(obj, static_cast<const clang::NamedDecl&>(decl), ctx);
)

VISIT_SPEC(clang::ClassTemplateDecl,
  obj["isThisDeclarationADefinition"] = decl.isThisDeclarationADefinition();

  json::Array specializations;
  auto& cdecl = const_cast<clang::ClassTemplateDecl&>(decl);
  for (auto itr = cdecl.spec_begin(); itr != cdecl.spec_end(); ++itr) {
    json::Object specialization;
    ::visit(specialization, *itr, ctx);
    specializations.emplace_back(specialization);
  }
  obj["specializations"] = specializations;

  json::Array partial_specializations;
  for (auto itr = cdecl.partial_spec_begin(); itr != cdecl.partial_spec_end(); ++itr) {
    json::Object partial_specialization;
    ::visit(partial_specialization, *itr, ctx);
    partial_specializations.emplace_back(partial_specialization);
  }
  obj["partial_specializations"] = partial_specializations;

  ::visit(obj, decl.getTemplatedDecl(), ctx);
  ::visit(obj, static_cast<const clang::RedeclarableTemplateDecl&>(decl), ctx);
);

VISIT_SPEC(clang::FunctionTemplateDecl,
  obj["isThisDeclarationADefinition"] = decl.isThisDeclarationADefinition();

  json::Array specializations;
  auto& cdecl = const_cast<clang::FunctionTemplateDecl&>(decl);
  for (auto itr = cdecl.spec_begin(); itr != cdecl.spec_end(); ++itr) {
    json::Object specialization;
    ::visit(specialization, *itr, ctx);
    specializations.emplace_back(specialization);
  }
  obj["specializations"] = specializations;

  ::visit(obj, *decl.getTemplatedDecl(), ctx);
  ::visit(obj, static_cast<const clang::RedeclarableTemplateDecl&>(decl), ctx);
);

VISIT_SPEC(clang::RedeclarableTemplateDecl,
  obj["isMemberSpecialization"] = const_cast<clang::RedeclarableTemplateDecl&>(decl).isMemberSpecialization();

  ::visit(obj, static_cast<const clang::TemplateDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::TypeAliasTemplateDecl);

DEFAULT_VISIT_SPEC(clang::TemplateTemplateParmDecl);

DEFAULT_VISIT_SPEC(clang::EnumDecl);

VISIT_SPEC(clang::TypeDecl,
  auto type = decl.getTypeForDecl();
  if (type) {

  }
  //obj["type"] = decl.getTypeForDecl()->getCanonicalTypeInternal().getAsString();

  ::visit(obj, static_cast<const clang::NamedDecl&>(decl), ctx);
);

VISIT_SPEC(clang::TagDecl,
  // TagDecl
  obj["isThisDeclarationADefinition"] = decl.isThisDeclarationADefinition();
  obj["isCompleteDefinition"] = decl.isCompleteDefinition();
  obj["isBeingDefined"] = decl.isBeingDefined();
  obj["isEmbeddedInDeclarator"] = decl.isEmbeddedInDeclarator();
  obj["isFreeStanding"] = decl.isFreeStanding();
  obj["isDependentType"] = decl.isDependentType();
  obj["kind"] = decl.getKindName();
  obj["isStruct"] = decl.isStruct();
  obj["isInterface"] = decl.isInterface();
  obj["isClass"] = decl.isClass();
  obj["isUnion"] = decl.isUnion();
  obj["isEnum"] = decl.isEnum();
  //obj["hasNameForLinkage"] = decl.hasNameForLinkage();

  ::visit(obj, static_cast<const clang::TypeDecl&>(decl), ctx);
  ::visit(obj, static_cast<const clang::DeclContext&>(decl), ctx);  
);

VISIT_SPEC(clang::RecordDecl,
  // RecordDecl
  obj["hasFlexibleArrayMember"] = decl.hasFlexibleArrayMember();
  obj["isAnonymousStructOrUnion"] = decl.isAnonymousStructOrUnion();
  obj["hasObjectMember"] = decl.hasObjectMember();
  //obj["hasVolatileMember"] = decl.hasVolatileMember();
  obj["isInjectedClassName"] = decl.isInjectedClassName();

  json::Array fields;
  for (auto itr = decl.field_begin(); itr != decl.field_end(); ++itr) {
    json::Object field;
    ::visit(field, *itr, ctx);
    fields.emplace_back(field);
  }
  obj["fields"] = fields;

  ::visit(obj, static_cast<const clang::TagDecl&>(decl), ctx);
);

VISIT_SPEC(clang::CXXRecordDecl,
  // CXXRecordDecl
  obj["hasDefinition"] = decl.hasDefinition();

  if (obj["hasDefinition"].as<bool>()) {

  obj["isDynamicClass"] = decl.isDynamicClass();
  obj["hasAnyDependentBases"] = decl.hasAnyDependentBases();
  obj["hasFriends"] = decl.hasFriends();
  obj["hasSimpleMoveConstructor"] = decl.hasSimpleMoveConstructor();
  obj["hasSimpleMoveAssignment"] = decl.hasSimpleMoveAssignment();
  obj["hasSimpleDestructor"] = decl.hasSimpleDestructor();
  obj["hasDefaultConstructor"] = decl.hasDefaultConstructor();
  obj["needsImplicitDefaultConstructor"] = decl.needsImplicitDefaultConstructor();
  obj["hasUserDeclaredConstructor"] = decl.hasUserDeclaredConstructor();
  obj["hasUserProvidedDefaultConstructor"] = decl.hasUserProvidedDefaultConstructor();
  obj["hasUserDeclaredCopyConstructor"] = decl.hasUserDeclaredCopyConstructor();
  obj["needsImplicitCopyConstructor"] = decl.needsImplicitCopyConstructor();
  obj["needsOverloadResolutionForCopyConstructor"] = decl.needsOverloadResolutionForCopyConstructor();
  obj["implicitCopyConstructorHasConstParam"] = decl.implicitCopyConstructorHasConstParam();
  obj["hasCopyConstructorWithConstParam"] = decl.hasCopyConstructorWithConstParam();
  obj["hasUserDeclaredMoveOperation"] = decl.hasUserDeclaredMoveOperation();
  obj["hasUserDeclaredMoveConstructor"] = decl.hasUserDeclaredMoveConstructor();
  obj["hasMoveConstructor"] = decl.hasMoveConstructor();
  obj["hasFailedImplicitMoveConstructor"] = decl.hasFailedImplicitMoveConstructor();
  obj["needsImplicitMoveConstructor"] = decl.needsImplicitMoveConstructor();
  obj["needsOverloadResolutionForMoveConstructor"] = decl.needsOverloadResolutionForMoveConstructor();
  obj["hasUserDeclaredCopyAssignment"] = decl.hasUserDeclaredCopyAssignment();
  obj["needsImplicitCopyAssignment"] = decl.needsImplicitCopyAssignment();
  obj["needsOverloadResolutionForCopyAssignment"] = decl.needsOverloadResolutionForCopyAssignment();
  obj["implicitCopyAssignmentHasConstParam"] = decl.implicitCopyAssignmentHasConstParam();
  obj["hasCopyAssignmentWithConstParam"] = decl.hasCopyAssignmentWithConstParam();
  obj["hasUserDeclaredMoveAssignment"] = decl.hasUserDeclaredMoveAssignment();
  obj["hasMoveAssignmeTnt"] = decl.hasMoveAssignment();
  obj["hasFailedImplicitMoveAssignment"] = decl.hasFailedImplicitMoveAssignment();
  obj["needsImplicitMoveAssignment"] = decl.needsImplicitMoveAssignment();
  obj["needsOverloadResolutionForMoveAssignment"] = decl.needsOverloadResolutionForMoveAssignment();
  obj["hasUserDeclaredDestructor"] = decl.hasUserDeclaredDestructor();
  obj["needsImplicitDestructor"] = decl.needsImplicitDestructor();
  obj["needsOverloadResolutionForDestructor"] = decl.needsOverloadResolutionForDestructor();
  obj["isLambda"] = decl.isLambda();
  obj["isAggregate"] = decl.isAggregate();
  obj["hasInClassInitializer"] = decl.hasInClassInitializer();
  obj["hasUninitializedReferenceMember"] = decl.hasUninitializedReferenceMember();
  obj["isPOD"] = decl.isPOD();
  obj["isCLike"] = decl.isCLike();
  obj["isEmpty"] = decl.isEmpty();
  obj["isPolymorphic"] = decl.isPolymorphic();
  obj["isAbstract"] = decl.isAbstract();
  obj["isStandardLayout"] = decl.isStandardLayout();
  obj["hasMutableFields"] = decl.hasMutableFields();
  obj["hasTrivialDefaultConstructor"] = decl.hasTrivialDefaultConstructor();
  obj["hasNonTrivialDefaultConstructor"] = decl.hasNonTrivialDefaultConstructor();
  obj["hasConstexprNonCopyMoveConstructor"] = decl.hasConstexprNonCopyMoveConstructor();
  obj["defaultedDefaultConstructorIsConstexpr"] = decl.defaultedDefaultConstructorIsConstexpr();
  obj["hasConstexprDefaultConstructor"] = decl.hasConstexprDefaultConstructor();
  obj["hasTrivialCopyConstructor"] = decl.hasTrivialCopyConstructor();
  obj["hasNonTrivialCopyConstructor"] = decl.hasNonTrivialCopyConstructor();
  obj["hasTrivialCopyAssignment"] = decl.hasTrivialCopyAssignment();
  obj["hasNonTrivialCopyAssignment"] = decl.hasNonTrivialCopyAssignment();
  obj["hasTrivialMoveAssignment"] = decl.hasTrivialMoveAssignment();
  obj["hasNonTrivialMoveAssignment"] = decl.hasNonTrivialMoveAssignment();
  obj["hasTrivialDestructor"] = decl.hasTrivialDestructor();
  obj["hasNonTrivialDestructor"] = decl.hasNonTrivialDestructor();
  obj["hasIrrelevantDestructor"] = decl.hasIrrelevantDestructor();
  obj["hasNonLiteralTypeFieldsOrBases"] = decl.hasNonLiteralTypeFieldsOrBases();
  obj["isTriviallyCopyable"] = decl.isTriviallyCopyable();
  obj["isTrivial"] = decl.isTrivial();
  obj["isLiteral"] = decl.isLiteral();
  obj["mayBeAbstract"] = decl.mayBeAbstract();
  obj["isDependentLambda"] = decl.isDependentLambda();

  json::Array bases;
  for (auto itr = decl.bases_begin(); itr != decl.bases_end(); ++itr) {
    json::Object base;
    ::visit(base, *itr, ctx);
    bases.emplace_back(base);
  };
  obj["bases"] = bases;

  json::Array vbases;
  for (auto itr = decl.vbases_begin(); itr != decl.vbases_end(); ++itr) { 
    json::Object vbase;
    ::visit(vbase, *itr, ctx);
    vbases.emplace_back(vbase);
  }
  obj["vbases"] = vbases;

  json::Array methods;
  for (auto itr = decl.method_begin(); itr != decl.method_end(); ++itr) { 
    json::Object method;
    ::visit(method, *itr, ctx);
    methods.emplace_back(method);
  }
  obj["methods"] = methods;

  json::Array ctors;
  for (auto itr = decl.ctor_begin(); itr != decl.ctor_end(); ++itr) {
    json::Object ctor;
    ::visit(ctor, *itr, ctx);
    ctors.emplace_back(ctor);
  }
  obj["ctors"] = ctors;

  json::Array friends;
  for (auto itr = decl.friend_begin(); itr != decl.friend_end(); ++itr) {
    json::Object friend_;
    ::visit(friend_, *itr, ctx);
    friends.emplace_back(friend_);
  }
  obj["friends"] = friends;

  }

  ::visit(obj, static_cast<const clang::RecordDecl&>(decl), ctx);
);

VISIT_SPEC(clang::CXXBaseSpecifier,
  obj["isVirtual"] = decl.isVirtual();
  obj["isBaseOfClass"] = decl.isBaseOfClass();
  obj["isPackExpansion"] = decl.isPackExpansion();
  obj["getInheritConstructors"] = decl.getInheritConstructors();
  obj["type"] = decl.getType().getAsString();

  json::Object src;
  ::visit(src, decl.getSourceRange(), ctx);
  obj["sourceRange"] = src;
);

VISIT_SPEC(clang::ClassTemplateSpecializationDecl,
  obj["isExplicitSpecialization"] = decl.isExplicitSpecialization();

  ::visit(obj, decl.getTemplateArgs(), ctx);
  ::visit(obj, static_cast<const clang::CXXRecordDecl&>(decl), ctx);
);

VISIT_SPEC(clang::ClassTemplatePartialSpecializationDecl,
  obj["isMemberSpecialization"] = const_cast<clang::ClassTemplatePartialSpecializationDecl&>(decl).isMemberSpecialization();

  const auto params = decl.getTemplateParameters();
  if (params)
    ::visit(obj, *params, ctx);

  ::visit(obj, static_cast<const clang::ClassTemplateSpecializationDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::TemplateTypeParmDecl);

DEFAULT_VISIT_SPEC(clang::TypeAliasDecl);

VISIT_SPEC(clang::TypedefDecl,
  ::visit(obj, static_cast<const clang::TypedefNameDecl&>(decl), ctx);
);

VISIT_SPEC(clang::TypedefNameDecl,
  obj["type"] = decl.getUnderlyingType().getAsString();

  ::visit(obj, static_cast<const clang::TypeDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::UnresolvedUsingTypenameDecl);

DEFAULT_VISIT_SPEC(clang::UsingDecl);

DEFAULT_VISIT_SPEC(clang::UsingDirectiveDecl);

DEFAULT_VISIT_SPEC(clang::UsingShadowDecl);

VISIT_SPEC(clang::FieldDecl,
  obj["index"] = json::Number(decl.getFieldIndex());
  obj["isMutable"] = decl.isMutable();
  obj["isBitField"] = decl.isBitField();
  obj["isUnnamedBitfield"] = decl.isUnnamedBitfield();
  obj["isAnonymousStructOrUnion"] = decl.isAnonymousStructOrUnion();
  obj["hasInClassInitializer"] = decl.hasInClassInitializer();

  ::visit(obj, static_cast<const clang::DeclaratorDecl&>(decl), ctx);
);

VISIT_SPEC(clang::DeclaratorDecl,
  ::visit(obj, static_cast<const clang::ValueDecl&>(decl), ctx);
);

VISIT_SPEC(clang::ValueDecl,
  obj["type"] = decl.getType().getAsString();
  obj["isWeak"] = decl.isWeak();

  ::visit(obj, static_cast<const clang::NamedDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::ObjCAtDefsFieldDecl);

DEFAULT_VISIT_SPEC(clang::ObjCIvarDecl);

VISIT_SPEC(clang::FunctionDecl,
  obj["isInlineSpecified"] = decl.isInlineSpecified();
  obj["isOutOfLine"] = decl.isOutOfLine();

  if (decl.hasBody()) {
    obj["hasBody"] = true;

    if (decl.isInlined()) {
      obj["isInlined"] = true;
      obj["isInlineDefinitionExternallyVisible"] = decl.isInlineDefinitionExternallyVisible();
    }
    else
      obj["isInlined"] = false;
  }
  else {
    obj["hasBody"] = false;
  }

  obj["isOverloadedOperator"] = decl.isOverloadedOperator();
  obj["isFunctionTemplateSpecialization"] = decl.isFunctionTemplateSpecialization();
  obj["isTemplateInstantiation"] = decl.isTemplateInstantiation();
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
  json::Array params;
  for (auto itr = decl.param_begin(); itr != decl.param_end(); ++itr) {
    json::Object param;
    ::visit(param, *itr, ctx);
    params.emplace_back(param);
  }
  obj["params"] = params;

  ::visit(obj, static_cast<const clang::DeclaratorDecl&>(decl), ctx);
  ::visit(obj, static_cast<const clang::DeclContext&>(decl), ctx);
);

VISIT_SPEC(clang::CXXMethodDecl,
  obj["isStatic"] = decl.isStatic();
  obj["isInstance"] = decl.isInstance();
  obj["isConst"] = decl.isConst();
  obj["isVolatile"] = decl.isVolatile();
  obj["isVirtual"] = decl.isVirtual();
  obj["isUsualDeallocationFunction"] = decl.isUsualDeallocationFunction();
  obj["isCopyAssignmentOperator"] = decl.isCopyAssignmentOperator();
  obj["isMoveAssignmentOperator"] = decl.isMoveAssignmentOperator();
  obj["isUserProvided"] = decl.isUserProvided();
  obj["hasInlineBody"] = decl.hasInlineBody();
  obj["isLambdaStaticInvoker"] = decl.isLambdaStaticInvoker();

  ::visit(obj, static_cast<const clang::FunctionDecl&>(decl), ctx);
);

VISIT_SPEC(clang::CXXConstructorDecl,
  obj["isExplicit"] = decl.isExplicit();

  if (decl.isThisDeclarationADefinition()) {
    obj["isExplicitSpecified"] = decl.isExplicitSpecified();
    obj["isImplicitlyDefined"] = decl.isImplicitlyDefined();
  }

  obj["isDelgatingConstructor"] = decl.isDelegatingConstructor();
  obj["isDefaultConstructor"] = decl.isDefaultConstructor();
  obj["isCopyConstructor"] = decl.isCopyConstructor();
  obj["isMoveConstructor"] = decl.isMoveConstructor();
  obj["isCopyOrMoveConstructor"] = decl.isCopyOrMoveConstructor();
  obj["isConvertingConstructor"] = decl.isConvertingConstructor(false);
  obj["isExplicitConvertingConstructor"] = decl.isConvertingConstructor(true);
  obj["isSpecializationCopyingObject"] = decl.isSpecializationCopyingObject();

  ::visit(obj, static_cast<const clang::CXXMethodDecl&>(decl), ctx);
);

VISIT_SPEC(clang::CXXConversionDecl, 
  obj["isExplicitSpecified"] = decl.isExplicitSpecified();
  obj["isExplicit"] = decl.isExplicit();
  obj["conversionType"] = decl.getConversionType().getAsString();
  obj["isLambdaToBlockPointerConversion"] = decl.isLambdaToBlockPointerConversion();

  ::visit(obj, static_cast<const clang::CXXMethodDecl&>(decl), ctx);
);

VISIT_SPEC(clang::CXXDestructorDecl,
  if (decl.isThisDeclarationADefinition())
    obj["isImplicitlyDefined"] = decl.isImplicitlyDefined();

  ::visit(obj, static_cast<const clang::CXXMethodDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::NonTypeTemplateParmDecl);

VISIT_SPEC(clang::VarDecl, 
  //obj["isThreadSpecified"] = decl.isThreadSpecified();
  obj["hasLocalStorage"] = decl.hasLocalStorage();
  obj["isStaticLocal"] = decl.isStaticLocal();
  obj["hasExternalStorage"] = decl.hasExternalStorage();
  obj["hasGlobalStorage"] = decl.hasGlobalStorage();
  obj["isExternC"] = decl.isExternC();
  obj["isLocalVarDecl"] = decl.isLocalVarDecl();
  obj["isFunctionOrMethodVarDecl"] = decl.isFunctionOrMethodVarDecl();
  obj["isStaticDataMember"] = decl.isStaticDataMember();
  obj["isOutOfLine"] = decl.isOutOfLine();
  obj["isFileVarDecl"] = decl.isFileVarDecl();
  obj["hasInit"] = decl.hasInit();
  //obj["extendsLifetimeOfTemporary"] = decl.extendsLifetimeOfTemporary();
  obj["isUsableInConstantExpressions"] = decl.isUsableInConstantExpressions(const_cast<clang::ASTContext&>(ctx));
  //obj["isInitKnownICE"] = decl.isInitKnownICE();
  //obj["isInitICE"] = decl.isInitICE();
  //obj["checkInitIsICE"] = decl.checkInitIsICE();
  obj["isDirectInit"] = decl.isDirectInit();
  obj["isExceptionVariable"] = decl.isExceptionVariable();
  obj["isNRVOVariable"] = decl.isNRVOVariable();
  obj["isCXXForRangeDecl"] = decl.isCXXForRangeDecl();
  obj["isConstexpr"] = decl.isConstexpr();

  ::visit(obj, static_cast<const clang::DeclaratorDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::ImplicitParamDecl);

VISIT_SPEC(clang::ParmVarDecl,
  obj["functionScopeDepth"] = json::Number(decl.getFunctionScopeDepth());
  obj["functionScopeIndex"] = json::Number(decl.getFunctionScopeIndex());
  obj["isKNRPromoted"] = decl.isKNRPromoted();
  obj["hasDefaultArg"] = decl.hasDefaultArg();
  obj["hasUnparsedDefaultArg"] = decl.hasUnparsedDefaultArg();
  obj["hasUninstantiatedDefaultArg"] = decl.hasUninstantiatedDefaultArg();
  obj["hasInheritedDefaultArg"] = decl.hasInheritedDefaultArg();
  obj["isParameterPack"] = decl.isParameterPack();

  ::visit(obj, static_cast<const clang::VarDecl&>(decl), ctx);
);

VISIT_SPEC(clang::EnumConstantDecl,
  // radix 10
  const auto& initVal = decl.getInitVal().toString(10);
  obj["value"] = json::Number(std::atoi(initVal.c_str()));

  ::visit(obj, static_cast<const clang::ValueDecl&>(decl), ctx);
);

DEFAULT_VISIT_SPEC(clang::IndirectFieldDecl);

DEFAULT_VISIT_SPEC(clang::UnresolvedUsingValueDecl);

DEFAULT_VISIT_SPEC(clang::ObjCPropertyImplDecl);

DEFAULT_VISIT_SPEC(clang::StaticAssertDecl);

DEFAULT_VISIT_SPEC(clang::TranslationUnitDecl);

VISIT_SPEC(clang::MacroInfo,
  obj["node_type"] = "Macro";

  std::stringstream cat;
  for (auto itr = decl.tokens_begin(); itr != decl.tokens_end(); ++itr)
    cat << ctx.getSpelling(*itr);
  obj["definition"] = escape_string(cat.str());

  json::Object begin;
  ::visit(begin, decl.getDefinitionLoc(), ctx);
  obj["begin"] = begin;

  json::Object end;
  ::visit(end, decl.getDefinitionEndLoc(), ctx);
  obj["end"] = end;

  if (decl.getNumTokens() > 0) {
    if (decl.isObjectLike())
      obj["kind"] = clang::tok::getTokenName(decl.getReplacementToken(0).getKind());
    else if (decl.isFunctionLike())
      obj["kind"] = "mixin";
  }
);

VISIT_SPEC(clang::Token,
  obj["name"] = ctx.getSpelling(decl);
);

VISIT_SPEC(clang::SourceRange,
  json::Object begin;
  ::visit(begin, decl.getBegin(), ctx);
  obj["begin"] = begin;

  json::Object end;
  ::visit(end, decl.getEnd(), ctx);
  obj["end"] = end;
);

VISIT_SPEC(clang::SourceLocation,
  const auto& sm = ctx.getSourceManager();
  clang::FullSourceLoc loc(decl, sm);
  if (loc.isValid()) {
    obj["line"] = json::Number(loc.getSpellingLineNumber());
    obj["column"] = json::Number(loc.getSpellingColumnNumber());

    const auto fileEntry = sm.getFileEntryForID(loc.getFileID());
    if (fileEntry != nullptr) {
      obj["file"] = fileEntry->getName();
      const auto dirEntry = fileEntry->getDir();
      if (dirEntry != nullptr)
        obj["dir"] = dirEntry->getName();
    }
  }
);

VISIT_SPEC(clang::NamedDecl,
  obj["name"] = decl.getNameAsString();
  obj["qualifiedName"] = decl.getQualifiedNameAsString();
  obj["hasLinkage"] = decl.hasLinkage();
  obj["isHidden"] = decl.isHidden();
  obj["isCXXClassMember"] = decl.isCXXClassMember();
  obj["isCXXInstanceMember"] = decl.isCXXInstanceMember();

  ::visit(obj, static_cast<const clang::Decl&>(decl), ctx);
);

VISIT_SPEC(clang::DeclContext,
  obj["node_type"] = decl.getDeclKindName();
  obj["isClosure"] = decl.isClosure();
  obj["isFunctionOrMethod"] = decl.isFunctionOrMethod();
  obj["isFileContext"] = decl.isFileContext();
  obj["isTranslationUnit"] = decl.isTranslationUnit();
  obj["isRecord"] = decl.isRecord();
  obj["isNamespace"] = decl.isNamespace();
  obj["isInlineNamespace"] = decl.isInlineNamespace();
  obj["isDependentContext"] = decl.isDependentContext();
  obj["isTransparentContext"] = decl.isTransparentContext();
  //obj["isExternCContext"] = decl.isExternCContext();
  obj["hasExternalLexicalStorage"] = decl.hasExternalLexicalStorage();
  obj["hasExternalVisibleStorage"] = decl.hasExternalVisibleStorage();

  json::Array declarations;
  for (auto itr = decl.decls_begin(); itr != decl.decls_end(); ++itr) {
    json::Object declaration;
    dispatch_decl(declaration, **itr, ctx);
    declarations.emplace_back(declaration);
  }
  obj["context"] = declarations;
);

VISIT_SPEC(clang::Decl,
  json::Object src;
  ::visit(src, decl.getSourceRange(), ctx);
  obj["sourceRange"] = src;

  obj["node_type"] = decl.getDeclKindName();  
)

/*
template <typename Context> 
struct JsonVisitor<clang::Decl, Context> {
  static void visit(json::Object& obj, const clang::Decl& decl, const Context& ctx) { 
    json::Object src;
    ::visit(src, decl.getSourceRange(), ctx);
    obj["sourceRange"] = src;

    obj["node_type"] = decl.getDeclKindName();

    switch (decl.getKind()) {
      #define ABSTRACT_DECL(DECL)
      #define DECL(CLASS, BASE) \
      case clang::Decl::CLASS: { \
        ::visit(obj, static_cast<const clang::CLASS##Decl&>(decl), ctx); \
        break; \
      }
      #include "clang/AST/DeclNodes.inc"
    }
  }
};

/*
template <typename Context> 
struct JsonVisitor<clang::Decl, Context> {
  static void visit(json::Object& obj, const clang::Decl& decl, const Context& ctx) {
    obj["node_type"] = decl.getDeclKindName();
    //if (clang::NamedDecl::classof(&decl))
      //::visit(obj, static_cast<const clang::NamedDecl&>(decl), ctx);
      //obj["name"] = static_cast<const clang::NamedDecl&>(decl).getNameAsString();
    //}

    json::Object src;
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
      json::Array arr;
      for (auto itr = declCtx->decls_begin(); itr != declCtx->decls_end(); ++itr) {
        json::Object child;
        ::visit(child, *itr, ctx);
        arr.emplace_back(child);
      }
      obj["context"] = arr;
    }
  }                                                              
};
*/
