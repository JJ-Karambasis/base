@echo off

pushd "%~dp0"
    cloc code --exclude-dir=third_party,meta --exclude-list-file=code/tests/utest.h -v -found=found.txt --ignored=ignored.txt
popd