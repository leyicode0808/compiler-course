#ifndef CMM_IRGEN_H
#define CMM_IRGEN_H

#include "ast.h"

int ir_generate(AstNode *root, const char *output_path);

#endif
