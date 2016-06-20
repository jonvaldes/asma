@echo off
	clang-format -style="{BasedOnStyle: 'llvm', IndentWidth: 4, ColumnLimit: 0, SpaceBeforeParens: Never}" -i %1
popd
