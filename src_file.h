//
// Created by david on 10/12/20.
//

#pragma once
struct CodeGen;
struct RedPackage;

enum SourceType
{
    ROOT,
    PKG_MAIN,
    NON_ROOT,
    C_IMPORT,
};


void add_source_file(CodeGen* code_gen, RedPackage* red_package, Buffer* resolved_path, Buffer* source_code, SourceType src_type);
