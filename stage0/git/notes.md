一级标题 <Alt+Ctrl+1>二级标题 <Alt+Ctrl+2>三级标题 <Alt+Ctrl+3>四级标题 <Alt+Ctrl+4>五级标题 <Alt+Ctrl+5>六级标题 <Alt+Ctrl+6>

# 速查卡（优先记住这些）

# 初始化/配置

git init                   # 在当前目录初始化仓库
git clone <url>            # 克隆远端仓库
git config --global user.name "Your Name"
git config --global user.email you@example.com
git config --global core.autocrlf input   # 跨平台换行推荐（Linux/macOS）

# 基本工作流

git status                 # 查看状态
git add <file/dir>         # 暂存变更
git commit -m "msg"        # 提交
git log --oneline --graph  # 提交历史（简洁图）

# 分支

git branch                 # 列出分支
git switch -c feat/x       # 新分支并切换（等价：git checkout -b）
git switch main            # 切回分支
git merge feat/x           # 合并分支
git rebase main            # 在 main 之上重放当前分支

# 远端

git remote -v              # 查看远端
git fetch origin           # 拉取但不合并
git pull --ff-only         # 只快进合并拉取
git push -u origin feat/x  # 首次推送并建立跟踪

# 撤销/回滚

git restore <file>         # 丢弃工作区改动
git restore --staged <f>   # 取消暂存
git reset --hard HEAD~1    # 回退提交（危险）
git revert <commit>        # 生成“反向提交”（安全）

# 其他常用

git stash push -m "wip"    # 临时搁置
git cherry-pick <commit>   # 拣选提交
git tag v1.0 -m "tag"      # 打标签
git bisect start           # 二分定位 Bug
git worktree add ../wt feature  # 多工作树

[ ]

所见即所得 <Alt+Ctrl+7>即时渲染 <Alt+Ctrl+8>分屏预览 <Alt+Ctrl+9>

大纲

DesktopTabletMobile/Wechat
