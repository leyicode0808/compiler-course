#!/bin/sh
set -eu

out="submission"

if [ -e "$out" ]; then
    echo "submission already exists; move or remove it before rebuilding" >&2
    exit 1
fi

mkdir -p "$out/report" "$out/tools" "$out/manual-check" "$out/proj1" "$out/proj2" "$out/proj3" "$out/proj4" "$out/proj5"

cp Makefile "$out/"
mkdir -p "$out/src"
cp src/* "$out/src/"
cp report/final-course-report.docx "$out/report/"
cp report/final-course-report.pdf "$out/report/"
cp report/practice-log.docx "$out/report/"
cp report/practice-log.pdf "$out/report/"
cp tools/check_env.sh tools/extract_tests.sh \
	tools/run_lexer_tests.sh tools/run_parser_tests.sh tools/run_semantic_tests.sh \
	tools/run_ir_tests.sh tools/run_irsim_tests.sh tools/run_one_irsim_case.sh tools/build_irsim_image.sh \
	tools/run_irsim_file.sh \
	tools/run_mips_tests.sh tools/run_one_mips_case.sh tools/run_opt_tests.sh \
	tools/run_manual_pipeline.sh "$out/tools/"
cp examples/manual/gcd.cmm "$out/manual-check/"
cp submission/manual-check/README.txt "$out/manual-check/"

cp docs/project-navigation-manual.md "$out/"
cat > "$out/README.txt" <<'EOF'
编译原理课程设计最终提交材料

Makefile
src/
  顶层可运行源码和构建入口。进入 submission 目录后可直接构建并运行手动验收脚本。

report/
  final-course-report.docx  最终课程设计报告 DOCX 版本
  final-course-report.pdf   最终课程设计报告 PDF 版本
  practice-log.docx         学生实践类课程学习日志 DOCX 版本
  practice-log.pdf          学生实践类课程学习日志 PDF 版本

proj1/ 到 proj5/
  分别对应实践一到实践五。每个目录包含相关源码、测试用例、运行脚本、说明文档和 note.txt。
  验收运行时以 submission/src 为构建源码；proj1-5 中的源码作为对应实践材料留存。
  proj1-5/src 按实践阶段累计放置：proj1 放前端，proj2 加语义，proj3 加 IR，proj4 加 MIPS，proj5 加优化器。

project-navigation-manual.md
  项目导航说明，用于快速定位源码、脚本、测试命令和演示流程。

manual-check/
  手写 C-- 程序验收说明和示例 gcd.cmm。

tools/
  顶层验收脚本。进入 submission 目录后，make lexer-test、make parser-test、
  make semantic-test、make ir-test、make mips-test、make opt-test 和手动流水线
  都调用这里的脚本；测试材料从 proj1-5 对应目录读取。
EOF

mkdir -p "$out/proj1/src" "$out/proj1/tools" "$out/proj1/tests" "$out/proj1/docs" "$out/proj1/report" "$out/proj1/course"
cp Makefile "$out/proj1/"
cp src/lexer.l src/lexer_main.c src/token.h src/parser.y src/parser_main.c src/ast.c src/ast.h "$out/proj1/src/"
cp tools/check_env.sh tools/extract_tests.sh tools/run_lexer_tests.sh tools/run_parser_tests.sh "$out/proj1/tools/"
cp -R tests/project1 "$out/proj1/tests/"
cp docs/practice1.1-lexer.md docs/practice1.2-parser.md docs/cmm-grammar.md "$out/proj1/docs/"
cp report/practice1.1-lexer-report.md report/practice1.1-lexer-report.docx report/practice1.2-parser-report.md report/practice1.2-parser-report.docx "$out/proj1/report/"
cp 编译原理课程设计/1/Project_1.pdf 编译原理课程设计/1/Tests_1.zip 编译原理课程设计/Appendix.pdf "$out/proj1/course/"
cat > "$out/proj1/note.txt" <<'EOF'
proj1：实践一材料，包含实践 1.1 Flex 词法分析和实践 1.2 Bison 语法分析。

主要源码：
  src/lexer.l              Flex 词法规则
  src/lexer_main.c         独立词法分析入口
  src/token.h              token 枚举定义
  src/parser.y             Bison 语法规则
  src/parser_main.c        语法分析入口
  src/ast.c, src/ast.h     语法树结构和打印

测试与脚本：
  tests/project1/          Project 1 测试输入和期望输出
  tools/run_lexer_tests.sh 实践 1.1 测试脚本
  tools/run_parser_tests.sh 实践 1.2 测试脚本
  Makefile                 提供 make lexer-test、make parser-test

文档：
  docs/                    实践说明和 C-- 文法整理
  report/                  实践一相关报告
  course/                  原始课程 Project 1 要求、测试压缩包和附录
EOF

mkdir -p "$out/proj2/src" "$out/proj2/tools" "$out/proj2/tests" "$out/proj2/docs" "$out/proj2/report" "$out/proj2/course"
cp Makefile "$out/proj2/"
cp src/lexer.l src/lexer_main.c src/token.h src/parser.y src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h "$out/proj2/src/"
cp tools/check_env.sh tools/extract_tests.sh tools/run_semantic_tests.sh "$out/proj2/tools/"
cp -R tests/project2 "$out/proj2/tests/"
cp docs/practice2-semantic.md docs/cmm-grammar.md "$out/proj2/docs/"
cp report/practice2-semantic-report.md report/practice2-semantic-report.docx "$out/proj2/report/"
cp 编译原理课程设计/2/Project_2.pdf 编译原理课程设计/2/Tests_2.zip 编译原理课程设计/Appendix.pdf "$out/proj2/course/"
cat > "$out/proj2/note.txt" <<'EOF'
proj2：实践二材料，语义分析与符号表设计实现。

主要源码：
  src/semantic.c, src/semantic.h  类型系统、符号表、作用域和语义检查
  src/parser.y, src/lexer.l       语义分析依赖的语法树前端
  src/ast.c, src/ast.h            AST 数据结构

测试与脚本：
  tests/project2/                 Project 2 测试输入和期望输出
  tools/run_semantic_tests.sh     语义分析测试脚本
  Makefile                        提供 make semantic-test

文档：
  docs/practice2-semantic.md      实践二实现说明
  report/                         实践二报告
  course/                         原始课程 Project 2 要求、测试压缩包和附录
EOF

mkdir -p "$out/proj3/src" "$out/proj3/tools" "$out/proj3/tests" "$out/proj3/docs" "$out/proj3/report" "$out/proj3/course"
cp Makefile "$out/proj3/"
cp src/lexer.l src/lexer_main.c src/token.h src/parser.y src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h "$out/proj3/src/"
cp tools/check_env.sh tools/extract_tests.sh tools/run_ir_tests.sh tools/run_one_irsim_case.sh tools/run_irsim_tests.sh tools/build_irsim_image.sh "$out/proj3/tools/"
cp -R tests/project3 "$out/proj3/tests/"
cp docs/practice3-ir.md docs/cmm-grammar.md "$out/proj3/docs/"
cp report/practice3-ir-report.md report/practice3-ir-report.docx "$out/proj3/report/"
cp 编译原理课程设计/3/Project_3.pdf 编译原理课程设计/3/Tests_3.zip 编译原理课程设计/3/irsim.zip 编译原理课程设计/Appendix.pdf "$out/proj3/course/"
cat > "$out/proj3/note.txt" <<'EOF'
proj3：实践三材料，中间代码生成与 IRSim 验证。

主要源码：
  src/irgen.c, src/irgen.h        AST 到三地址 IR 的翻译
  src/semantic.c, src/semantic.h  IR 生成前的语义检查
  src/parser.y, src/lexer.l       前端依赖

测试与脚本：
  tests/project3/                 Project 3 测试输入
  tools/run_ir_tests.sh           批量生成 IR
  tools/run_one_irsim_case.sh     执行单个样例 IR 并打印 IRSim 输出
  tools/run_irsim_tests.sh        调用课程 IRSim 验证 IR
  tools/build_irsim_image.sh      Python 3.8 + PyQt5 的 IRSim Docker 环境
  Makefile                        提供 make ir-test、make irsim-test

文档：
  docs/practice3-ir.md            实践三实现说明
  report/                         实践三报告
  course/                         原始课程 Project 3 要求、测试压缩包、IRSim 和附录
EOF

mkdir -p "$out/proj4/src" "$out/proj4/tools" "$out/proj4/tests" "$out/proj4/examples" "$out/proj4/docs" "$out/proj4/report" "$out/proj4/course"
cp Makefile "$out/proj4/"
cp src/lexer.l src/lexer_main.c src/token.h src/parser.y src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h src/mipsgen.c src/mipsgen.h "$out/proj4/src/"
cp tools/check_env.sh tools/extract_tests.sh tools/run_one_mips_case.sh tools/run_mips_tests.sh "$out/proj4/tools/"
cp -R tests/project3 "$out/proj4/tests/"
cp -R examples/project4 "$out/proj4/examples/"
cp docs/practice4-mips.md docs/cmm-grammar.md "$out/proj4/docs/"
cp report/practice4-mips-report.md report/practice4-mips-report.docx "$out/proj4/report/"
cp 编译原理课程设计/4/Project_4.pdf 编译原理课程设计/4/MIPS32_and_SPIM.pdf 编译原理课程设计/Appendix.pdf "$out/proj4/course/"
cat > "$out/proj4/note.txt" <<'EOF'
proj4：实践四材料，MIPS32 目标代码生成与 SPIM 验证。

主要源码：
  src/mipsgen.c, src/mipsgen.h    IR 到 MIPS32 汇编的后端
  src/irgen.c, src/irgen.h        目标代码生成前的 IR 生成
  src/parser.y, src/lexer.l       前端依赖

测试与脚本：
  tests/project3/                 复用 Project 3 输入进行 MIPS 运行验证
  examples/project4/              Project 4 公开样例整理
  tools/run_one_mips_case.sh      执行单个样例，生成汇编并打印 SPIM 输出
  tools/run_mips_tests.sh         SPIM 运行测试脚本
  Makefile                        提供 make mips-test

文档：
  docs/practice4-mips.md          实践四实现说明
  report/                         实践四报告
  course/                         原始 Project 4 要求、MIPS32/SPIM 资料和附录
EOF

mkdir -p "$out/proj5/src" "$out/proj5/tools" "$out/proj5/tests" "$out/proj5/examples" "$out/proj5/docs" "$out/proj5/report" "$out/proj5/course"
cp Makefile "$out/proj5/"
cp src/lexer.l src/lexer_main.c src/token.h src/parser.y src/parser_main.c src/ast.c src/ast.h src/semantic.c src/semantic.h src/irgen.c src/irgen.h src/mipsgen.c src/mipsgen.h src/iropt.c src/iropt.h src/iropt_main.c "$out/proj5/src/"
cp tools/check_env.sh tools/extract_tests.sh tools/run_ir_tests.sh tools/run_irsim_tests.sh tools/run_opt_tests.sh tools/build_irsim_image.sh "$out/proj5/tools/"
cp -R tests/project3 "$out/proj5/tests/"
cp -R examples/project5 "$out/proj5/examples/"
cp docs/practice5-opt.md docs/cmm-grammar.md "$out/proj5/docs/"
cp report/practice5-opt-report.md report/practice5-opt-report.docx "$out/proj5/report/"
cp 编译原理课程设计/5/Project_5.pdf 编译原理课程设计/5/cmmc_optimizer-main.zip 编译原理课程设计/Appendix.pdf "$out/proj5/course/"
cat > "$out/proj5/note.txt" <<'EOF'
proj5：实践五材料，中间代码优化。

主要源码：
  src/iropt.c, src/iropt.h        IR 优化器实现
  src/iropt_main.c                优化器命令行入口
  src/irgen.c, src/irgen.h        生成待优化 IR 的前置模块

测试与脚本：
  examples/project5/              Project 5 必做优化样例
  tests/project3/                 生成 IR 并验证优化前后语义一致
  tools/run_opt_tests.sh          优化器测试脚本
  tools/run_irsim_tests.sh        IRSim 验证脚本
  Makefile                        提供 make opt-test

文档：
  docs/practice5-opt.md           实践五实现说明
  report/                         实践五报告
  course/                         原始 Project 5 要求、参考优化器压缩包和附录
EOF

echo "submission materials created in $out"
