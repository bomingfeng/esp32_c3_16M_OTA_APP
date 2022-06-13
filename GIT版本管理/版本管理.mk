Git操作方法：
1。在要搭建的文件夹下右键"Git Bash Here"；或 在要克隆的文件夹下右键"Git Bash Here"
2。命令 git init ；或 命令git clone https://gitee.com/bomingfeng(或github.com)  c:/目录(省略时，克隆到当前路径)
3。命令 git status查看文件状态
4。命令 git add . 增加文件到暂存区
5。命令 git commit -m "create" 提交暂存区到本地仓库;"create" 消息内容，相当于备注
6。免密登陆远程仓库，生成公钥：在c：/Users/esp32-c3/.ssh目录下，右键"Git Bash Here"，"ssh-keygen -t rsa";在远程仓库增加SSH公钥
7。想把已有人代码上传到远程仓库：在远程仓库新建仓库，在本地任意新建文夹中clone克隆下，复制.git文件夹到已有的代码中，远程仓库和本地仓库关联了。
8。命令git branch 查看本地分支
9。命令git branch -a 查看所有分支(本地及远程仓库)
10。创建并切换到 dev 分支 git checkout -b dev origin/dev
11。添加主仓的远程仓库地址 git remote add upstream https://github.com/captain_a/project.git
12。查看当前仓库所连接的远程仓库 git remote -v
13。切换到 dev 分支 git checkout dev
14。获取 upstream 的最新内容 git fetch upstream dev
15。合并 git merge upstream/dev
16。将分支 push 到远程仓库git push origin feature/login_module:feature/login_module
17。push 到远程仓库 git push origin feature/login_module
18。Pull Request，请求与主仓进行合并
19。git remote -v：    查看远程仓库详细信息，包括仓库名称，关联地址
20。git remote remove orign：删除orign仓库
21。git remote add origin 仓库地址：添加远程仓库地址
22。gti push -u origin master： 提交到远程仓库的master主干
23。git remote rm origin 删除远程仓库网址


分支管理：
master 分支：
	主分支，最终的、稳定的、经过测试没有 bug 的、可部署于生产环境的分支，只能由 release 和 hotfix 分支合并，任何情况下都不能直接修改代码

dev 分支：
	主要开发分支，贯穿于整个项目的生命周期，始终保持最新版本，功能模块开发任务交给 feature 分支，测试任务交给 release 分支

hotfix 分支：
	热修复分支，当 master 分支部署到生产环境后发生紧急状况，需要及时处理时，该分支负责热修复，即在保证应用不下线的条件下，对 bug 进行紧急修复，该分支以 master 分支为基线，修复 bug 后，合并到 master 分支部署上线，同时也合并到 dev 分支保持最新进度；命名规则： hotfix/NAME 或 hotfix-NAME

feature 分支：
	功能模块开发分支，对应于一个特定的功能模块，该分支以 dev 分支为基线，完成开发工作后再合并到 dev 分支；命名规则：feature/NAME 或 feature-NAME

release 分支：预发布分支，在发布正式版本前进行全面测试和修复，该分支以 dev 分支为基线进行全面测试，若发生 bug 则可直接在该分支修复并提交，经过测试没有问题之后，合并到 master 分支部署上线，同时也合并到 dev 分支保持最新进度；命名规则：release/NAME 或 release-NAME

____________________________________________________
| 分支名称 | 分支职责              | 基线分支 | 合并分支    |
____________________________________________________
| master   | 主分支                  | -             | -          |
____________________________________________________
| dev        | 开发分支               | master   | -          |
____________________________________________________
| hotfix     | 热修复分支           | master   | master,dev | 
____________________________________________________
| feature   | 功能模块开发分支 | dev        | dev        |   
____________________________________________________      
| release   | 预发布分支           | dev        | master, dev |
____________________________________________________

总结：
master 分支和 dev 分支都贯穿于整个项目的生命周期
hotfix、feature、release 分支都是临时分支，分别负责热修复、功能模块开发、预发布

Commit message
每次 commit 到本地库时，必须添加 commit message，以对本次提交做出说明
在团队合作时，commit message 的书写格式也应当遵守相应规范，清晰明了的 commit message 有利于快速定位提交、自动生成 change log 文档
