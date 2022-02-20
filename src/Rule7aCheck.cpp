
//===--- Rule7aCheck.cpp - clang-tidy -------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Rule7aCheck.h"
#include <algorithm>
#include <iostream>
#include <regex>
#include <vector>

using namespace clang::ast_matchers;

namespace clang {
    namespace tidy {
        namespace eastwood {
            Rule7aCheck::Rule7aCheck(StringRef Name, ClangTidyContext *Context)
                : ClangTidyCheck(Name, Context),
                  debug_enabled(Options.get("debug", "false")) {
                if (this->debug_enabled == "true") {
                    this->debug = true;
                }
            }
            void Rule7aCheck::registerMatchers(MatchFinder *Finder) {
                Finder->addMatcher(functionDecl().bind("function_decl"), this);
            }
            void Rule7aCheck::check(const MatchFinder::MatchResult &Result) {

                const SourceManager &SM = *Result.SourceManager;

                if (auto MatchedDecl =
                        Result.Nodes.getNodeAs<FunctionDecl>("function_decl")) {
                    if (not MatchedDecl->isDefined()) {
                        return;
                    }
                    const FunctionDecl *FunctionDefinition =
                        MatchedDecl->getDefinition();
                    std::string fname =
                        FunctionDefinition->getNameInfo().getName().getAsString();
                    SourceRange FunctionDefinitionRange =
                        FunctionDefinition->getSourceRange();
                    unsigned int func_start_line = 0;
                    if (SM.isWrittenInMainFile(
                            FunctionDefinition->getReturnTypeSourceRange()
                                .getBegin())) {
                        func_start_line = SM.getSpellingLineNumber(
                            FunctionDefinition->getReturnTypeSourceRange().getBegin());
                    } else {
                        // if the return type is not in the main file, then we can't
                        // use it to check preceeding lines, grab the function name
                        // location instead
                        func_start_line = SM.getSpellingLineNumber(
                            FunctionDefinition->getNameInfo().getLoc());
                    }
                    if (MatchedDecl->doesThisDeclarationHaveABody() &&
                        FunctionDefinition) {
                        auto toks = this->relex_file(Result, "function_decl");
                        std::vector<Token> tokens;
                        for (auto t : *toks) {
                            // Make sure the lines we grab backward from the decl are on
                            // lines above and come before in the TU - this fixes #73
                            if (SM.getSpellingLineNumber(t.getLocation()) <
                                func_start_line) {
                                tokens.push_back(t);
                            } else {
                                break;
                            }
                        }

                        if (tokens.size() >= 3) {
                            std::vector<Token>::reverse_iterator rit = tokens.rbegin();

                            size_t comment_ct = 0;
                            for (size_t i = 0; rit != tokens.rend() && i < 3;
                                 rit++, i++) {
                                this->dout()
                                    << "[Rule7a : checking preheader token for " +
                                           fname + "] "
                                    << *this->tok_string(SM, *rit) << "\n";
                                if (rit->getKind() == tok::comment) {
                                    comment_ct++;
                                }
                            }
                            if (comment_ct == 0) {
                                diag(FunctionDefinitionRange.getBegin(),
                                     "Missing header comment for function " + fname +
                                         ".");
                                return;
                            }
                            std::string before_tok_str =
                                *this->tok_string(SM, tokens.at(tokens.size() - 3));
                            if (std::count(before_tok_str.begin(), before_tok_str.end(),
                                           '\n') < 2) {
                                diag(tokens.at(tokens.size() - 2).getLocation(),
                                     "At least one empty line required before function "
                                     "header "
                                     "comment for function " +
                                         fname + ".");
                            }

                            if (*this->tok_string(SM, tokens.back()) != "\n\n") {
                                diag(tokens.back().getEndLoc(),
                                     "Exactly one empty line required after function "
                                     "header "
                                     "comment for function " +
                                         fname + ".");
                            }

                            if (!isWhitespace(*SM.getCharacterData(
                                    tokens.at(tokens.size() - 3).getLocation()))) {
                                diag(tokens.at(tokens.size() - 2).getLocation(),
                                     "At least one empty line required before function "
                                     "header "
                                     "comment for function " +
                                         fname + ".");
                            }

                            if (!isWhitespace(*SM.getCharacterData(
                                    tokens.back().getLocation()))) {
                                diag(tokens.back().getEndLoc(),
                                     "Exactly one empty line required after function "
                                     "header "
                                     "comment for function " +
                                         fname + ".");
                            }

                            if (tokens.at(tokens.size() - 2).getKind() ==
                                tok::comment) {
                                std::string raw_header_comment =
                                    *this->tok_string(SM, tokens.at(tokens.size() - 2));
                                std::regex pre_regex{
                                    R"([\/][*]([^\n]*[\n][ ][*])([ ]*[^ ][^\n]*[\n][ ][*]){1,}[\/])"};
                                std::smatch result;
                                if (std::regex_match(raw_header_comment, result,
                                                     pre_regex)) {
                                    return;
                                }
                                diag(tokens.at(tokens.size() - 2).getLocation(),
                                     "Malformed function header comment for function " +
                                         fname + ".");
                                return;
                            }
                            diag(FunctionDefinitionRange.getBegin(),
                                 "Missing function header comment for function " +
                                     fname + ".");
                            return;
                        }
                        diag(MatchedDecl->getLocation(),
                             "Malformed function header comment for function " + fname +
                                 ".");
                    }
                }
            }
        } // namespace eastwood
    }     // namespace tidy
} // namespace clang
