"""
netdisk_gui.py
---------------

This module implements a simple graphical user interface for the
backup/restore utility provided in ``netdisk.py``. The GUI is built
using Tkinter and organized into three tabs: Backup, Restore, and
Package. It allows the user to back up a directory tree into a
repository, apply various filters, restore data back to a target
directory, and export/import repositories into single-file packages
with optional compression and encryption.

Note: Running this GUI requires a graphical environment. When
executed in the provided runtime, the GUI will open in a separate
window allowing manual interaction. Progress bars update during
long-running operations.
"""

import os
import threading
import time
import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from pathlib import Path

from netdisk import (
    Repository,
    Backup,
    Restore,
    FilterChain,
    PathFilter,
    FileTypeFilter,
    NameFilter,
    TimeFilter,
    SizeFilter,
    UserFilter,
    PackAlg,
    CompressAlg,
    EncryptAlg,
    export_repo_to_package,
    import_package_to_repo,
    detect_file_type,
    FileType,
    Metadata,
)


class BackupThread(threading.Thread):
    """Thread to perform backup with progress updates."""

    def __init__(self, gui, source_root: Path, repo_path: Path, filters: FilterChain):
        super().__init__(daemon=True)
        self.gui = gui
        self.source_root = source_root
        self.repo_path = repo_path
        self.filters = filters

    def run(self):
        repo = Repository(self.repo_path)
        repo.initialize()
        # Build list of files first to determine progress range
        files: list[Path] = []
        for dirpath, dirnames, filenames in os.walk(self.source_root, followlinks=False):
            # Skip symlink directories to avoid cycles
            dirnames[:] = [d for d in dirnames if not (Path(dirpath) / d).is_symlink()]
            for name in filenames:
                fpath = Path(dirpath) / name
                if detect_file_type(fpath) == FileType.DIRECTORY:
                    continue
                files.append(fpath)
        total = len(files)
        # Update progress start
        self.gui.backup_progress['maximum'] = total
        self.gui.backup_progress['value'] = 0
        success = 0
        failed = 0
        skipped = 0
        # Load existing index
        repo.load_index()
        idx = 0
        for fpath in files:
            idx += 1
            # Check if user cancelled
            if self.gui.cancel_requested:
                break
            # Update progress bar and label
            self.gui.backup_progress['value'] = idx
            self.gui.status_var.set(f"备份中: {fpath}")
            self.gui.root.update_idletasks()
            # Apply filters
            if self.filters and not self.filters.should_include(fpath):
                skipped += 1
                continue
            ftype = detect_file_type(fpath)
            if ftype not in (FileType.REGULAR, FileType.SYMLINK, FileType.FIFO,
                             FileType.BLOCK, FileType.CHARACTER, FileType.SOCKET):
                skipped += 1
                continue
            rel = fpath.relative_to(self.source_root)
            meta = Metadata()
            if not meta.load_from_file(fpath):
                failed += 1
                continue
            if repo.store_file(fpath, rel, meta):
                success += 1
            else:
                failed += 1
        # Save index
        repo.save_index()
        self.gui.status_var.set(
            f"备份完成: 成功 {success} 个, 失败 {failed} 个, 跳过 {skipped} 个")
        self.gui.backup_progress['value'] = total


class RestoreThread(threading.Thread):
    """Thread to perform restore with progress updates."""

    def __init__(self, gui, repo_path: Path, target_root: Path):
        super().__init__(daemon=True)
        self.gui = gui
        self.repo_path = repo_path
        self.target_root = target_root

    def run(self):
        repo = Repository(self.repo_path)
        repo.load_index()
        files = list(repo.list_files())
        total = len(files)
        self.gui.restore_progress['maximum'] = total
        self.gui.restore_progress['value'] = 0
        success = 0
        failed = 0
        idx = 0
        for rel in files:
            idx += 1
            if self.gui.cancel_requested:
                break
            self.gui.restore_progress['value'] = idx
            self.gui.restore_status_var.set(f"还原中: {rel}")
            self.gui.root.update_idletasks()
            dest = self.target_root / rel
            if repo.restore_file(rel, dest):
                success += 1
            else:
                failed += 1
        self.gui.restore_status_var.set(
            f"还原完成: 成功 {success} 个, 失败 {failed} 个")
        self.gui.restore_progress['value'] = total


class NetDiskGUI:
    """Main GUI application class."""

    def __init__(self, root: tk.Tk):
        """
        Initialize the main GUI object.  We set a human‑friendly
        window title instead of the numeric fallback seen in some
        versions of the original GUI.  This ensures that the window
        caption displays correctly on systems where the default font
        lacks glyphs for Chinese characters.
        """
        self.root = root
        # Use a descriptive Chinese title for the application window.
        self.root.title("目录备份与还原工具")
        # Internal flag used to cancel long‑running operations.
        self.cancel_requested = False
        # Build the user interface components.
        self._build_ui()

    def _build_ui(self):
        nb = ttk.Notebook(self.root)
        self.backup_tab = ttk.Frame(nb)
        self.restore_tab = ttk.Frame(nb)
        self.package_tab = ttk.Frame(nb)
        nb.add(self.backup_tab, text="备份")
        nb.add(self.restore_tab, text="还原")
        nb.add(self.package_tab, text="打包/解包")
        nb.pack(fill="both", expand=True)
        self._build_backup_tab()
        self._build_restore_tab()
        self._build_package_tab()

    # ------------------------------------------------------------------ Backup
    def _build_backup_tab(self):
        f = self.backup_tab
        row = 0
        # Source directory
        tk.Label(f, text="源目录:").grid(row=row, column=0, sticky="e")
        self.source_entry = tk.Entry(f, width=40)
        self.source_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_source).grid(row=row, column=2)
        row += 1
        # Repo path
        tk.Label(f, text="仓库路径:").grid(row=row, column=0, sticky="e")
        self.repo_entry = tk.Entry(f, width=40)
        self.repo_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_repo).grid(row=row, column=2)
        row += 1
        # Include/exclude
        tk.Label(f, text="包含路径(逗号分隔):").grid(row=row, column=0, sticky="e")
        self.include_entry = tk.Entry(f, width=40)
        self.include_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        tk.Label(f, text="排除路径(逗号分隔):").grid(row=row, column=0, sticky="e")
        self.exclude_entry = tk.Entry(f, width=40)
        self.exclude_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        # File type filters
        tk.Label(f, text="文件类型:").grid(row=row, column=0, sticky="e")
        types_frame = ttk.Frame(f)
        types_frame.grid(row=row, column=1, sticky="w")
        self.type_vars = {}
        for t in [FileType.REGULAR, FileType.SYMLINK, FileType.FIFO,
                  FileType.BLOCK, FileType.CHARACTER, FileType.SOCKET]:
            var = tk.BooleanVar(value=False)
            chk = tk.Checkbutton(types_frame, text=t, variable=var)
            chk.pack(side="left")
            self.type_vars[t] = var
        row += 1
        # Name contains
        tk.Label(f, text="文件名包含(逗号分隔):").grid(row=row, column=0, sticky="e")
        self.name_entry = tk.Entry(f, width=40)
        self.name_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        # Time filters
        tk.Label(f, text="修改时间下界(Unix时间戳):").grid(row=row, column=0, sticky="e")
        self.mtime_after_entry = tk.Entry(f, width=40)
        self.mtime_after_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        tk.Label(f, text="修改时间上界(Unix时间戳):").grid(row=row, column=0, sticky="e")
        self.mtime_before_entry = tk.Entry(f, width=40)
        self.mtime_before_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        # Size filters
        tk.Label(f, text="最小大小(字节):").grid(row=row, column=0, sticky="e")
        self.min_size_entry = tk.Entry(f, width=40)
        self.min_size_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        tk.Label(f, text="最大大小(字节):").grid(row=row, column=0, sticky="e")
        self.max_size_entry = tk.Entry(f, width=40)
        self.max_size_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        # User filters
        tk.Label(f, text="用户ID:").grid(row=row, column=0, sticky="e")
        self.uid_entry = tk.Entry(f, width=15)
        self.uid_entry.grid(row=row, column=1, sticky="w", padx=5, pady=2)
        tk.Label(f, text="组ID:").grid(row=row, column=1, sticky="e", padx=(150,0))
        self.gid_entry = tk.Entry(f, width=15)
        self.gid_entry.grid(row=row, column=2, sticky="w", padx=5, pady=2)
        row += 1
        # Buttons and progress
        tk.Button(f, text="开始备份", command=self.start_backup).grid(row=row, column=0, pady=5)
        self.cancel_button = tk.Button(f, text="取消", command=self.cancel_operation, state="disabled")
        self.cancel_button.grid(row=row, column=1, pady=5, sticky="w")
        row += 1
        self.backup_progress = ttk.Progressbar(f, orient="horizontal", length=300, mode="determinate")
        self.backup_progress.grid(row=row, column=0, columnspan=3, padx=5, pady=5)
        row += 1
        self.status_var = tk.StringVar(value="等待操作...")
        tk.Label(f, textvariable=self.status_var).grid(row=row, column=0, columnspan=3, sticky="w", padx=5)

    # ----------------------------------------------------------------- Restore
    def _build_restore_tab(self):
        f = self.restore_tab
        row = 0
        tk.Label(f, text="仓库路径:").grid(row=row, column=0, sticky="e")
        self.restore_repo_entry = tk.Entry(f, width=40)
        self.restore_repo_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_restore_repo).grid(row=row, column=2)
        row += 1
        tk.Label(f, text="目标目录:").grid(row=row, column=0, sticky="e")
        self.target_entry = tk.Entry(f, width=40)
        self.target_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_target).grid(row=row, column=2)
        row += 1
        tk.Button(f, text="开始还原", command=self.start_restore).grid(row=row, column=0, pady=5)
        self.restore_cancel_button = tk.Button(f, text="取消", command=self.cancel_operation, state="disabled")
        self.restore_cancel_button.grid(row=row, column=1, sticky="w", pady=5)
        row += 1
        self.restore_progress = ttk.Progressbar(f, orient="horizontal", length=300, mode="determinate")
        self.restore_progress.grid(row=row, column=0, columnspan=3, padx=5, pady=5)
        row += 1
        self.restore_status_var = tk.StringVar(value="等待操作...")
        tk.Label(f, textvariable=self.restore_status_var).grid(row=row, column=0, columnspan=3, sticky="w", padx=5)

    # --------------------------------------------------------------- Packaging
    def _build_package_tab(self):
        f = self.package_tab
        row = 0
        # Export
        tk.Label(f, text="导出仓库路径:").grid(row=row, column=0, sticky="e")
        self.exp_repo_entry = tk.Entry(f, width=40)
        self.exp_repo_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_exp_repo).grid(row=row, column=2)
        row += 1
        tk.Label(f, text="输出包文件:").grid(row=row, column=0, sticky="e")
        self.pkg_file_entry = tk.Entry(f, width=40)
        self.pkg_file_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_pkg_file).grid(row=row, column=2)
        row += 1
        # Algorithms
        tk.Label(f, text="打包算法:").grid(row=row, column=0, sticky="e")
        self.pack_alg_var = tk.StringVar(value="header")
        ttk.OptionMenu(f, self.pack_alg_var, "header", "header", "toc").grid(row=row, column=1, sticky="w")
        row += 1
        tk.Label(f, text="压缩算法:").grid(row=row, column=0, sticky="e")
        self.compress_alg_var = tk.StringVar(value="none")
        ttk.OptionMenu(f, self.compress_alg_var, "none", "none", "rle").grid(row=row, column=1, sticky="w")
        row += 1
        tk.Label(f, text="加密算法:").grid(row=row, column=0, sticky="e")
        self.encrypt_alg_var = tk.StringVar(value="none")
        ttk.OptionMenu(f, self.encrypt_alg_var, "none", "none", "xor", "rc4").grid(row=row, column=1, sticky="w")
        row += 1
        tk.Label(f, text="密码(可选):").grid(row=row, column=0, sticky="e")
        self.password_entry = tk.Entry(f, width=40, show="*")
        self.password_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        tk.Button(f, text="导出包", command=self.start_export).grid(row=row, column=0, pady=5)
        row += 1
        # Import
        ttk.Separator(f, orient="horizontal").grid(row=row, column=0, columnspan=3, sticky="ew", pady=10)
        row += 1
        tk.Label(f, text="输入包文件:").grid(row=row, column=0, sticky="e")
        self.import_pkg_entry = tk.Entry(f, width=40)
        self.import_pkg_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_import_pkg).grid(row=row, column=2)
        row += 1
        tk.Label(f, text="导入仓库路径:").grid(row=row, column=0, sticky="e")
        self.import_repo_entry = tk.Entry(f, width=40)
        self.import_repo_entry.grid(row=row, column=1, padx=5, pady=2)
        tk.Button(f, text="选择", command=self._choose_import_repo).grid(row=row, column=2)
        row += 1
        tk.Label(f, text="密码(如果需要):").grid(row=row, column=0, sticky="e")
        self.import_password_entry = tk.Entry(f, width=40, show="*")
        self.import_password_entry.grid(row=row, column=1, padx=5, pady=2)
        row += 1
        tk.Button(f, text="导入包", command=self.start_import).grid(row=row, column=0, pady=5)
        row += 1
        self.package_status_var = tk.StringVar(value="等待操作...")
        tk.Label(f, textvariable=self.package_status_var).grid(row=row, column=0, columnspan=3, sticky="w", padx=5)

    # --------------------------------------------------------------- UI helpers
    def _choose_source(self):
        path = filedialog.askdirectory(title="选择源目录")
        if path:
            self.source_entry.delete(0, tk.END)
            self.source_entry.insert(0, path)

    def _choose_repo(self):
        path = filedialog.askdirectory(title="选择仓库目录")
        if path:
            self.repo_entry.delete(0, tk.END)
            self.repo_entry.insert(0, path)

    def _choose_restore_repo(self):
        path = filedialog.askdirectory(title="选择仓库目录")
        if path:
            self.restore_repo_entry.delete(0, tk.END)
            self.restore_repo_entry.insert(0, path)

    def _choose_target(self):
        path = filedialog.askdirectory(title="选择目标目录")
        if path:
            self.target_entry.delete(0, tk.END)
            self.target_entry.insert(0, path)

    def _choose_exp_repo(self):
        path = filedialog.askdirectory(title="选择导出仓库目录")
        if path:
            self.exp_repo_entry.delete(0, tk.END)
            self.exp_repo_entry.insert(0, path)

    def _choose_pkg_file(self):
        path = filedialog.asksaveasfilename(title="选择包文件", defaultextension=".pkg", filetypes=[("Package Files", "*.pkg"), ("All Files", "*.*")])
        if path:
            self.pkg_file_entry.delete(0, tk.END)
            self.pkg_file_entry.insert(0, path)

    def _choose_import_pkg(self):
        path = filedialog.askopenfilename(title="选择包文件", filetypes=[("Package Files", "*.pkg"), ("All Files", "*.*")])
        if path:
            self.import_pkg_entry.delete(0, tk.END)
            self.import_pkg_entry.insert(0, path)

    def _choose_import_repo(self):
        path = filedialog.askdirectory(title="选择导入仓库目录")
        if path:
            self.import_repo_entry.delete(0, tk.END)
            self.import_repo_entry.insert(0, path)

    # ------------------------------------------------------------------- Backup
    def start_backup(self):
        source = self.source_entry.get().strip()
        repo = self.repo_entry.get().strip()
        if not source or not repo:
            messagebox.showerror("错误", "源目录和仓库路径不能为空")
            return
        src_path = Path(source)
        repo_path = Path(repo)
        if not src_path.exists():
            messagebox.showerror("错误", f"源目录不存在: {src_path}")
            return
        # Build filter chain
        chain = FilterChain()
        # Path filter
        pf = PathFilter()
        includes = [p.strip() for p in self.include_entry.get().split(',') if p.strip()]
        for inc in includes:
            pf.add_include(inc)
        excludes = [p.strip() for p in self.exclude_entry.get().split(',') if p.strip()]
        for exc in excludes:
            pf.add_exclude(exc)
        if includes or excludes:
            chain.add_filter(pf)
        # File type filter
        ftf = FileTypeFilter()
        for t, var in self.type_vars.items():
            if var.get():
                ftf.add_allowed(t)
        if ftf.allowed:
            chain.add_filter(ftf)
        # Name filter
        name_contains = [s.strip() for s in self.name_entry.get().split(',') if s.strip()]
        nf = NameFilter()
        for s in name_contains:
            nf.add_contains(s)
        if name_contains:
            chain.add_filter(nf)
        # Time filter
        tf = TimeFilter()
        after_ts = self.mtime_after_entry.get().strip()
        before_ts = self.mtime_before_entry.get().strip()
        if after_ts:
            try:
                tf.set_after(int(after_ts))
            except ValueError:
                messagebox.showerror("错误", "修改时间下界必须是整数")
                return
        if before_ts:
            try:
                tf.set_before(int(before_ts))
            except ValueError:
                messagebox.showerror("错误", "修改时间上界必须是整数")
                return
        if tf.after is not None or tf.before is not None:
            chain.add_filter(tf)
        # Size filter
        sf = SizeFilter()
        min_size = self.min_size_entry.get().strip()
        max_size = self.max_size_entry.get().strip()
        if min_size:
            try:
                sf.set_min_size(int(min_size))
            except ValueError:
                messagebox.showerror("错误", "最小大小必须是整数")
                return
        if max_size:
            try:
                sf.set_max_size(int(max_size))
            except ValueError:
                messagebox.showerror("错误", "最大大小必须是整数")
                return
        if sf.min_size is not None or sf.max_size is not None:
            chain.add_filter(sf)
        # User filter
        uf = UserFilter()
        uid = self.uid_entry.get().strip()
        gid = self.gid_entry.get().strip()
        if uid:
            try:
                uf.set_uid(int(uid))
            except ValueError:
                messagebox.showerror("错误", "用户ID必须是整数")
                return
        if gid:
            try:
                uf.set_gid(int(gid))
            except ValueError:
                messagebox.showerror("错误", "组ID必须是整数")
                return
        if uf.uid is not None or uf.gid is not None:
            chain.add_filter(uf)
        # Start backup thread
        self.cancel_requested = False
        self.cancel_button.config(state="normal")
        self.status_var.set("开始备份...")
        thread = BackupThread(self, src_path, repo_path, chain)
        thread.start()

    def cancel_operation(self):
        self.cancel_requested = True
        self.status_var.set("操作已请求取消...")
        self.restore_status_var.set("操作已请求取消...")

    # ------------------------------------------------------------------ Restore
    def start_restore(self):
        repo = self.restore_repo_entry.get().strip()
        target = self.target_entry.get().strip()
        if not repo or not target:
            messagebox.showerror("错误", "仓库路径和目标目录不能为空")
            return
        repo_path = Path(repo)
        target_path = Path(target)
        if not repo_path.exists():
            messagebox.showerror("错误", f"仓库路径不存在: {repo_path}")
            return
        self.cancel_requested = False
        self.restore_cancel_button.config(state="normal")
        self.restore_status_var.set("开始还原...")
        thread = RestoreThread(self, repo_path, target_path)
        thread.start()

    # ---------------------------------------------------------------- Export
    def start_export(self):
        repo = self.exp_repo_entry.get().strip()
        pkg_file = self.pkg_file_entry.get().strip()
        if not repo or not pkg_file:
            messagebox.showerror("错误", "仓库路径和输出包文件不能为空")
            return
        pack_alg = PackAlg.HEADER if self.pack_alg_var.get() == "header" else PackAlg.TOC
        compress_alg = CompressAlg.NONE if self.compress_alg_var.get() == "none" else CompressAlg.RLE
        enc_selection = self.encrypt_alg_var.get()
        encrypt_alg = {
            "none": EncryptAlg.NONE,
            "xor": EncryptAlg.XOR,
            "rc4": EncryptAlg.RC4,
        }.get(enc_selection, EncryptAlg.NONE)
        password = self.password_entry.get()
        # Perform export in thread
        def export_task():
            try:
                export_repo_to_package(repo, pkg_file, pack_alg, compress_alg, encrypt_alg, password)
                self.package_status_var.set("导出完成")
            except Exception as e:
                self.package_status_var.set(f"导出失败: {e}")
        self.package_status_var.set("导出中...")
        threading.Thread(target=export_task, daemon=True).start()

    # ---------------------------------------------------------------- Import
    def start_import(self):
        pkg_file = self.import_pkg_entry.get().strip()
        repo = self.import_repo_entry.get().strip()
        if not pkg_file or not repo:
            messagebox.showerror("错误", "包文件和仓库路径不能为空")
            return
        password = self.import_password_entry.get()
        def import_task():
            try:
                import_package_to_repo(pkg_file, repo, password)
                self.package_status_var.set("导入完成")
            except Exception as e:
                self.package_status_var.set(f"导入失败: {e}")
        self.package_status_var.set("导入中...")
        threading.Thread(target=import_task, daemon=True).start()


def main():
    """
    Entry point for launching the GUI application.  Before creating the
    ``NetDiskGUI`` instance we attempt to select a default font that
    includes Chinese glyphs so that all labels and controls render
    properly on systems where the Tk default font lacks CJK support.
    """
    root = tk.Tk()
    # Attempt to configure a default font with CJK support.  Fallback
    # silently if the font is not available.  This list can be extended
    # with additional fonts commonly installed on Linux distributions.
    try:
        import tkinter.font as tkfont
        default_font = tkfont.nametofont("TkDefaultFont")
    except Exception:
        default_font = None
    if default_font:
        for fname in ["Noto Sans CJK SC", "WenQuanYi Zen Hei", "SimSun", "Microsoft YaHei"]:
            try:
                default_font.configure(family=fname, size=10)
                break
            except Exception:
                continue
        try:
            root.option_add("*Font", default_font)
        except Exception:
            pass
    app = NetDiskGUI(root)
    root.mainloop()


if __name__ == '__main__':
    main()