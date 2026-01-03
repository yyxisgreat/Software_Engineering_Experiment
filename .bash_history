sudo apt update
sudo apt install -y build-essential cmake git
cmake --version
g++ --version
git --version
cd ~
git clone https://github.com/yyxisgreat/Software_Engineering_Experiment.git
ssh-keygen -t ed25519 -C "2022270903009@std.uestc.edu.cn"
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519
cat ~/.ssh/id_ed25519.pub
ssh -T git@github.com
git clone git@github.com:yyxisgreat/Software_Engineering_Experiment.git
cd Software_Engineering_Experiment
git checkout -b test-push
echo "test" > PUSH_TEST.txt
git add PUSH_TEST.txt
git commit -m "test push"
git push -u origin test-push
git config --global user.name "Jamiesnow"
git config --global user.email "2022270903009@std.uestc.edu.cn"
git config --global --list
cd ~/Software_Engineering_Experiment
# 确保在 test-push 分支
git branch
git add PUSH_TEST.txt
git commit -m "test push"
git push
git rm PUSH_TEST.txt
git commit -m "remove test file"
git push
git checkout main
git pull
git checkout -b feature/xxx
mkdir -p build
cd build
cmake ..
make -j
ls -lah
cd ~/Software_Engineering_Experiment
cd build
./backup-restore backup ../test/source ../test/repo
nano src/metadata/metadata.cpp
mkdir -p build
cd build
cmake ..
make -j
mkdir -p build
cd build
cmake ..
make -j
pwd
ls -lah ..
pwd
ls -lah ..
cd ~/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake -G "Unix Makefiles" ..
make -j
ls -lah | grep backup
cd ~/Software_Engineering_Experiment/build
./backup-restore backup ../test/source ../test/repo
./backup-restore restore ../test/repo ../test/target
diff -r ../test/source ../test/target || true
cd ~/Software_Engineering_Experiment
ls
code .
ls /mnt/c/Users
cd /mnt/c/Users/PC/Downloads
ls
cp modified_code.zip ~/
unzip -o modified_code.zip
sudo apt install unzip
unzip -o modified_code.zip
cd ~
ls
unzip -o modified_code.zip
ls src/filters
cd ~
ls
pwd
cd ~/Software_Engineering_Experiment-main
ls src/filters
cp -r src CMakeLists.txt README.md build.sh .gitignore test gui storage    ~/Software_Engineering_Experiment/
cp -r src CMakeLists.txt README.md ~/Software_Engineering_Experiment/
cp -r test gui storage build.sh .gitignore ~/Software_Engineering_Experiment/ 2>/dev/null
cd ~/Software_Engineering_Experiment
ls src/filters
cd ~/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake ..
make -j
cd ~/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake ..
make -j
cd ~/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake ..
make -j
cd ~/Software_Engineering_Experiment
ls -lah build/backup-restore
cd ~/Software_Engineering_Experiment
rm -rf demo_src demo_repo demo_dst
mkdir demo_src demo_repo demo_dst
echo "hello world" > demo_src/a.txt
mkfifo demo_src/myfifo
ln -s a.txt demo_src/link_to_a
ls -l demo_src
cd ~/Software_Engineering_Experiment
./build/backup-restore backup demo_src demo_repo
sed -n '1,50p' demo_repo/index.txt
rm -rf demo_repo/*
mkdir -p demo_repo
./build/backup-restore backup demo_src demo_repo --type regular
cat demo_repo/index.txt
rm -rf demo_repo/*
mkdir -p demo_repo
./build/backup-restore backup demo_src demo_repo --name-contains a
cat demo_repo/index.txt
rm -rf demo_dst
mkdir demo_dst
./build/backup-restore restore demo_repo demo_dst
rm -rf demo_src demo_repo demo_dst
mkdir demo_src demo_repo demo_dst
echo "hello world" > demo_src/a.txt
mkfifo demo_src/myfifo
ln -s a.txt demo_src/link_to_a
./build/backup-restore backup demo_src demo_repo
cd ~/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake ..
make -j
cd ~/Software_Engineering_Experiment
rm -rf demo_src demo_repo demo_dst
mkdir demo_src demo_repo demo_dst
echo "hello world" > demo_src/a.txt
mkfifo demo_src/myfifo
ln -s a.txt demo_src/link_to_a
./build/backup-restore backup demo_src demo_repo
cat demo_repo/index.txt
ls -l demo_repo/data
./build/backup-restore restore demo_repo demo_dst
ls -l demo_dst
readlink demo_dst/link_to_a
git status
cd ~/Software_Engineering_Experiment
ls
git status
git branch --show-current
rm -rf demo_src demo_repo demo_dst
nano .gitignore
git add CMakeLists.txt src/ README.md build.sh
git add .gitignore
git status
git commit -m "V2: add file-type support, metadata extension, and custom filters"
git push -u origin feature/xxx
git add README.md ./设计书.md ./设计书v2.md  CMakeLists.txt src/ .gitignore
git commit -m "V2: update README and extend backup features (file types, metadata, filters)"
git push
git add README.md
git commit -m "V2: update README and extend backup features (file types, metadata, filters)"
git push
cd ~
ls
cd Software_Engineering_Experiment
code .
cd ~
ls
code.
code
code .
cd ~
mkdir -p work
cd work
cp "/mnt/d/Gemini Chrome下载/integrated_project.zip" .
ls
cd ~/work
mkdir -p Software_Engineering_Experiment
unzip -o integrated_project.zip -d Software_Engineering_Experiment
cd Software_Engineering_Experiment
ls
sudo apt update
sudo apt install -y build-essential cmake unzip
cd ~/work/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake ..
make -j
cd ~/work/Software_Engineering_Experiment
rm -rf build
mkdir build
cd build
cmake ..
make -j
ls -lah ./backup-restore
cd ~/work/Software_Engineering_Experiment
chmod +x run_tests.sh
bash run_tests.sh
cd ~/work/Software_Engineering_Experiment
ls -l demo_src
stat demo_src/a.txt
readlink demo_src/a.txt || echo "a.txt 不是符号链接"
cd ~/work/Software_Engineering_Experiment
# 1) 看看仓库 data 里 a.txt 到底是什么
ls -l demo_repo/data/a.txt
# 2) 读一下它是不是链接，指向哪里
readlink demo_repo/data/a.txt || echo "repo/data/a.txt 不是符号链接"
# 3) 看索引里怎么记录 a.txt
cat demo_repo/index.txt
python3 --version
python3 -m venv .venv
sudo apt update
sudo apt install -y python3 python3-venv
python3 -m venv .venv
source .venv/bin/activate
ls -al .venv
which python
python --version
source .venv/bin/activate
which python
python --version
python test_functions.py
sudo apt update
sudo apt install -y python3-tk
python -c "import tkinter; print('tk ok')"
mkdir -p ~/projects/demo_source/subdir
echo "hello" > ~/projects/demo_source/regular.txt
echo "nested" > ~/projects/demo_source/subdir/nested.txt
ln -s regular.txt ~/projects/demo_source/symlink.txt
mkfifo ~/projects/demo_source/pipe
python netdisk_gui.py
sudo apt update
sudo apt install -y fonts-noto-cjk fonts-wqy-zenhei
pwd
mkdir -p ~/projects
cd ~/projects
unzip /mnt/d/Gemini\ Chrome下载/integrated_project_modified.zip -d netdisk_project
cd netdisk_project
ls
code .
pwd
uname -a
pwd
uname -a
mkdir -p ~/projects/netdisk
cd ~/projects/netdisk
unzip "/mnt/d/Gemini Chrome下载/final_project_code.zip" -d final_project
cd final_project
ls
sudo apt update
sudo apt install -y python3 python3-venv python3-tk fonts-noto-cjk fonts-wqy-zenhei
python3 -m venv .venv
source .venv/bin/activate
python --version
which python
python test_functions.py
python netdisk_gui.py
mkdir -p ~/netdisk_demo/source/subdir
echo "hello world" > ~/netdisk_demo/source/a.txt
echo "nested file" > ~/netdisk_demo/source/subdir/b.txt
mkdir -p ~/netdisk_demo/repo
ln -s a.txt ~/netdisk_demo/source/link_to_a.txt
mkfifo ~/netdisk_demo/source/myfifo
chmod 640 ~/netdisk_demo/source/a.txt
touch -t 202401020304 ~/netdisk_demo/source/a.txt
stat ~/netdisk_demo/source/a.txt
python netdisk_gui.py
find ~/netdisk_demo/source -type f -o -type p -o -type l
mkdir -p ~/meta_test/src
echo "meta" > ~/meta_test/src/a.txt
chmod 640 ~/meta_test/src/a.txt
touch -m -d "2020-01-01 12:34:56" ~/meta_test/src/a.txt
stat ~/meta_test/src/a.txt
stat ~/meta_test/dst/a.txt
stat ~/meta_test/a.txt
stat -c "%Y" ~/score_test/src/file.txt
dd if=/dev/zero of=~/score_test/src/big.bin bs=1K count=100
id -u
id -g
cd ~/backup_restore_project/python
# 我先按你的截图猜 repo 在这里：
REPO=~/score_test/data
# 检查 repo 是否有效：必须同时存在 index.txt 和 data/
ls -l "$REPO"
ls -l "$REPO/index.txt" "$REPO/data" >/dev/null && echo "OK: repo 结构正确" || echo "ERROR: 这个目录不是仓库repo"
ls ~/score_test/src
mkdir -p ~/score_test/repo
cd ~/backup_restore_project/python
python3 - <<'PY'
from pathlib import Path
from netdisk import backup_directory

src = Path.home() / "score_test" / "src"
repo = Path.home() / "score_test" / "repo"

result = backup_directory(src, repo)
print("Backup results:", result)
PY

cd ~/backup_restore_project/python
python3 - <<'PY'
import netdisk
print("netdisk loaded from:", netdisk.__file__)
print("Has Repository:", hasattr(netdisk, "Repository"))
print("Has Backup:", hasattr(netdisk, "Backup"))
print("Has Restore:", hasattr(netdisk, "Restore"))
PY

python3 - <<'PY'
from pathlib import Path
from netdisk import Repository, Backup

src = Path.home() / "score_test" / "src"
repo = Path.home() / "score_test" / "repo"

repository = Repository(repo)
repository.initialize()

backup = Backup(repository)
ok, fail, skip = backup.execute(src)

print(f"Backup results: success={ok}, failed={fail}, skipped={skip}")
PY

cd ~/backup_restore_project/python
# 我先按你的截图猜 repo 在这里：
REPO=~/score_test/data
# 检查 repo 是否有效：必须同时存在 index.txt 和 data/
ls -l "$REPO"
ls -l "$REPO/index.txt" "$REPO/data" >/dev/null && echo "OK: repo 结构正确" || echo "ERROR: 这个目录不是仓库repo"
mkdir -p ~/pkg_tests/packages
mkdir -p ~/pkg_tests/imported
ls -l ~/score_test/repo/index.txt ~/score_test/repo/data && echo "OK: repo 结构正确"
cd ~/backup_restore_project/python
python3 - <<'PY'
from pathlib import Path
from netdisk import export_repo_to_package, PackAlg, CompressAlg, EncryptAlg

repo = Path.home() / "score_test" / "repo"
pkg  = Path.home() / "pkg_tests" / "packages" / "repo_header_none_none.pkg"

export_repo_to_package(
    repo,
    pkg,
    PackAlg.HEADER,
    CompressAlg.NONE,
    EncryptAlg.NONE,
    password=""
)

print("Export OK:", pkg)
PY

rm -rf ~/pkg_tests/imported/repo_header_none_none
diff -r ~/score_test/repo ~/pkg_tests/imported/repo_header_none_none   && echo "Passed header, none, none"
rm -rf ~/pkg_tests/imported/repo_header_none_none
mkdir  ~/pkg_tests/imported/repo_header_none_none
diff -r ~/score_test/repo ~/pkg_tests/imported/repo_header_none_none   && echo "Passed header, none, none"
rm -rf ~/pkg_tests/imported/repo_header_none_none
python3 - <<'PY'
from pathlib import Path
from netdisk import import_package_to_repo

pkg = Path.home() / "pkg_tests" / "packages" / "repo_header_none_none.pkg"
dst = Path.home() / "pkg_tests" / "imported" / "repo_header_none_none"

import_package_to_repo(pkg, dst, password="")
print("Import OK:", dst)
PY

ls -l ~/pkg_tests/imported/repo_header_none_none
diff -r ~/score_test/repo ~/pkg_tests/imported/repo_header_none_none   && echo "Passed header, none, none"
rm -rf ~/pkg_tests/imported/repo_header_none_none
python3 run_tests.py
cd ~/backup_restore_project
ls
docker --version
docker ps
mkdir -p ~/backup_restore_project
unzip -o "/mnt/d/Gemini Chrome下载/final_package.zip" -d ~/backup_restore_project
cd ~/backup_restore_project
ls
sudo apt update
sudo apt install -y python3 python3-pip python3-tk file
cd ~/backup_restore_project/python
python3 run_tests.py
python3 test_metadata.py
echo $DISPLAY
sudo apt install -y fonts-noto-cjk
python3 netdisk_gui.py
mkdir -p ~/score_test/src/subdir
echo "hello" > ~/score_test/src/file.txt
echo "nested" > ~/score_test/src/subdir/nested.txt
# 创建符号链接（指向 file.txt）
ln -s "file.txt" ~/score_test/src/link.lnk
# 创建FIFO管道
mkfifo ~/score_test/src/pipe
ls -l ~/score_test/src
file ~/score_test/src/pipe ~/score_test/src/link.lnk
python3 netdisk_gui.py
source /home/xiaowangzi/backup_restore_project/.venv/bin/activate
stat -c "%Y" ~/score_test/src/file.txt
1767416450
id -u
rm -rf ~/pkg_tests/imported/repo_header_none_none
diff -r ~/score_test/repo ~/pkg_tests/imported/repo_header_none_none   && echo "Passed header, none, none"
rm -rf ~/pkg_tests/imported/repo_header_none_none

cd ~/backup_restore_project/python
python3 run_tests.py
