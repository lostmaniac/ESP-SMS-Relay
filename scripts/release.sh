#!/bin/bash

# ESP-SMS-Relay 发布脚本
# 用于创建新版本标签和触发自动发布

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查是否在git仓库中
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    print_error "当前目录不是git仓库"
    exit 1
fi

# 检查工作目录是否干净
if ! git diff-index --quiet HEAD --; then
    print_error "工作目录有未提交的更改，请先提交或暂存"
    git status --porcelain
    exit 1
fi

# 获取当前分支
current_branch=$(git branch --show-current)
print_info "当前分支: $current_branch"

# 检查是否在main分支
if [ "$current_branch" != "main" ]; then
    print_warning "当前不在main分支，建议切换到main分支进行发布"
    read -p "是否继续? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# 获取最新的标签
last_tag=$(git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
print_info "上一个版本: $last_tag"

# 显示自上次发布以来的提交
print_info "自上次发布以来的提交:"
git log $last_tag..HEAD --oneline --no-merges

# 提示输入新版本号
echo
print_info "请输入新版本号 (格式: v1.2.3):"
read -p "版本号: " new_version

# 验证版本号格式
if [[ ! $new_version =~ ^v[0-9]+\.[0-9]+\.[0-9]+(-[a-zA-Z0-9]+\.[0-9]+)?$ ]]; then
    print_error "版本号格式不正确，应为 v1.2.3 或 v1.2.3-beta.1"
    exit 1
fi

# 检查标签是否已存在
if git rev-parse "$new_version" >/dev/null 2>&1; then
    print_error "标签 $new_version 已存在"
    exit 1
fi

# 确认发布
echo
print_warning "即将创建标签 $new_version 并推送到远程仓库"
print_warning "这将触发自动构建和发布流程"
read -p "确认发布? (y/N): " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    print_info "发布已取消"
    exit 0
fi

# 创建标签
print_info "创建标签 $new_version..."
git tag -a "$new_version" -m "Release $new_version"

# 推送标签
print_info "推送标签到远程仓库..."
git push origin "$new_version"

print_success "标签 $new_version 已成功创建并推送"
print_info "GitHub Actions 将自动开始构建和发布流程"
print_info "请访问 GitHub 仓库查看构建状态和发布进度"

# 显示相关链接
remote_url=$(git config --get remote.origin.url)
if [[ $remote_url =~ github\.com[:/]([^/]+)/([^/\.]+) ]]; then
    repo_owner="${BASH_REMATCH[1]}"
    repo_name="${BASH_REMATCH[2]}"
    print_info "Actions 页面: https://github.com/$repo_owner/$repo_name/actions"
    print_info "Releases 页面: https://github.com/$repo_owner/$repo_name/releases"
fi

echo
print_success "发布流程已启动！"