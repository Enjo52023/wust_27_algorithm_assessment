#!/bin/bash

APP=build/app

help() {
    echo "用法: ./run.sh <子命令>"
    echo "  build          编译项目"
    echo "  run            编译并运行"
    echo "  run -- 参数    编译并带参数运行"
    echo "  clean          清理编译产物"
    echo "  help           显示帮助信息"
}

build() {
    echo "开始编译..."
    cmake -S . -B build && cmake --build build
    if [ $? -ne 0 ]; then
        echo "编译失败" >&2
        return 1
    fi
    echo "编译成功: $APP"
}

case "$1" in
    build)
        build
        ;;
    run)
        build || exit 1
        shift
        [ "$1" = "--" ] && shift
        ./$APP "$@"
        ;;
    clean)
        rm -rf build
        echo "已清理 build 目录"
        ;;
    help)
        help
        ;;
    "")
        echo "错误: 缺少子命令" >&2
        exit 1
        ;;
    *)
        echo "错误: 未知的子命令 \"$1\"" >&2
        echo "可以用 ./run.sh help 查看用法" >&2
        exit 1
        ;;
esac
