#!/bin/bash

os_type="$(uname)"

build_type=""
if [ "$os_type" == "Darwin" ]; then
    build_type="osx"
elif [ "$os_type" == "Linux" ]; then
    build_type="linux"
else
    echo "Unknown OS: $os_type"
    exit 1
fi

base_path=$(dirname "$0")
code_path="$base_path/code"
bin_path="$base_path/bin"

tracy_path=""
build_debug=0
build_clang=0
build_asan=0
build_cpp=0
build_gdi=1
build_meta=1
build_tracy=0

if [ $build_type == "osx" ]; then
    build_clang=1
fi

# Manual parsing
while [[ $# -gt 0 ]]; do
    case "$1" in
        -tracy_path)
            tracy_path="$2"
            shift 2
            ;;

        -debug)
            build_debug=1
            shift
            ;;

        -clang)
            build_clang=1
            shift
            ;;

        -asan)
            build_asan=1
            shift
            ;;

        -bin_path)
            bin_path="$2"
            shift 2
            ;;
        
        -cpp)
            build_cpp=1
            shift
            ;;

        -no_gdi)
            build_gdi=0
            shift
            ;;
  esac
done

if [ -d "$tracy_path" ]; then
    build_tracy=1
fi

if [ ! -d "$bin_path" ]; then
    mkdir "$bin_path"
fi

app_includes=
app_defines="-DRPMALLOC_FIRST_CLASS_HEAPS=1"

if [ $build_debug -eq 1 ]; then 
    app_defines="$app_defines -DDEBUG_BUILD"
fi

clang_optimized_flag="-O0"
if [ $build_debug -eq 0 ]; then
    clang_optimized_flag="-O3"
fi

clang_compile_only="-c"
clang_warnings="-Wall -Werror -Wno-missing-braces -Wno-switch -Wno-unused-function -Wno-nullability-completeness -Wno-unused-variable -Wno-unused-private-field"
clang_flags="-g $clang_optimized_flag"
clang_out="-o"

if [ $build_clang -eq 1 ]; then 
    compile_only=$clang_compile_only
    compile_warnings=$clang_warnings
    compile_flags=$clang_flags
    compile_out=$clang_out
    compiler_c="clang -std=c11"
    compiler_cpp="clang++ -std=c++20"
fi

compiler=$compiler_c
c_ext="c"

if [ $build_cpp -eq 1 ]; then
    compiler=$compiler_cpp
    c_ext="cpp"
fi

if [ $build_tracy -eq 1 ]; then
    # Build tracy client here
    test=1
fi

pushd "$bin_path"
    $compiler_c $compile_flags $compile_warnings $compile_only $app_defines $app_includes "$code_path/third_party/rpmalloc/rpmalloc.c"
    $compiler $compile_flags $compile_warnings $compile_only $app_defines $app_includes "$code_path/base.$c_ext"
popd

frameworks=
if [ $build_type == "osx" ]; then
    frameworks="-framework Foundation"

    pushd "$bin_path"
        $compiler_c $compile_flags $compile_warnings $compile_only $app_defines $app_includes "$code_path/os/osx/osx_base.m"
    popd
fi

if [ $build_meta -eq 1 ]; then

    pushd "$bin_path"
        $compiler $compile_flags $compile_warnings $app_defines $app_includes $frameworks "$code_path/meta_program/meta_program.$c_ext" rpmalloc.o base.o osx_base.o $compile_out meta_program
    popd

fi

if [ $build_gdi -eq 1 ]; then
    pushd "$bin_path"
        ./meta_program -d "$code_path/gdi"
        $compiler_cpp $compile_flags $compile_warnings $compile_only $app_defines -DVK_USE_PLATFORM_METAL_EXT $app_includes -I"$code_path/third_party/Vulkan-Headers" -I"$code_path/third_party/VulkanMemoryAllocator" "$code_path/gdi/backend/vk/vk_vma_usage.cpp"
        $compiler $compile_flags $compile_warnings $compile_only $app_defines -DVK_USE_PLATFORM_METAL_EXT $app_includes -I"$code_path/third_party/Vulkan-Headers" -I"$code_path/third_party/VulkanMemoryAllocator" -I"$code_path" "$code_path/gdi/backend/vk/vk_gdi.$c_ext" $compile_out gdi.o
    popd
fi

obj_files="osx_base.o base.o rpmalloc.o"
if [ $build_gdi -eq 1 ]; then 
    obj_files="$obj_files gdi.o vk_vma_usage.o"
fi

pushd "$bin_path"
    ar rcs libbase.a $obj_files
popd