"""Simple test to verify metadata preservation during backup/restore.

This script creates a file with specific permissions and modification
time, backs it up into a repository using the netdisk API, restores
it to a new location, and then compares the mode and modification
time of the restored file to ensure they match the source.  UID/GID
preservation is skipped because it requires root privileges.

To run the test:

    python3 test_metadata.py
"""

import os
import time
import shutil
import tempfile
import stat
from pathlib import Path

from netdisk import Repository, Backup, Restore


def main():
    base = Path(tempfile.mkdtemp(prefix="test_meta_"))
    try:
        src = base / "src"
        repo = base / "repo"
        dst = base / "dst"
        src.mkdir()
        # Create a file with known content
        fpath = src / "meta.txt"
        fpath.write_text("metadata test", encoding="utf-8")
        # Set custom permissions and modification time
        os.chmod(fpath, 0o640)
        ts = int(time.time()) - 86400  # set mtime to 1 day ago
        os.utime(fpath, (ts, ts))
        # Backup
        repository = Repository(repo)
        repository.initialize()
        backup = Backup(repository)
        backup.execute(src)
        # Restore
        restore = Restore(repository)
        dst.mkdir()
        restore.execute(dst)
        restored = dst / "meta.txt"
        # Compare metadata
        src_stat = os.lstat(fpath)
        dst_stat = os.lstat(restored)
        same_mode = stat.S_IMODE(src_stat.st_mode) == stat.S_IMODE(dst_stat.st_mode)
        same_mtime = int(src_stat.st_mtime) == int(dst_stat.st_mtime)
        print(f"Permissions preserved: {same_mode}")
        print(f"Modification time preserved: {same_mtime}")
        if not (same_mode and same_mtime):
            raise AssertionError("metadata did not match after restore")
        print("Metadata test passed")
    finally:
        shutil.rmtree(base)


if __name__ == '__main__':
    main()