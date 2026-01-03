"""
netdisk.py
--------------

This module provides a pure-Python implementation of a simple
backup/restore system inspired by the provided C++ project. It supports
storing files and their metadata into a repository directory, restoring
files from a repository, applying various filters during backup, and
packing the repository into a single archive with optional
compression and encryption. The packing formats mirror the C++
implementation's two modes (per-file headers and a table-of-contents at
the end). The compression algorithm is a simple run-length encoding
(RLE) and two encryption algorithms are provided: an XOR stream
cipher and RC4. A GUI front end leveraging this module is provided
separately in ``netdisk_gui.py``.

The code is designed to run on a POSIX system and does not depend on
third-party libraries. It uses ``os`` and ``stat`` to interact with
the file system and ``tkinter`` for the GUI (in the separate module).

This implementation also addresses several issues noted in the C++
version:

* Named pipes (FIFOs) are supported by using ``os.mkfifo`` and
  including the necessary modules. Metadata for FIFOs is recorded
  correctly and restored via ``os.mkfifo`` with the saved mode.
* Symbolic link cycles are avoided by skipping symlinked directories
  during recursive traversal. This follows the guidance that
  directory symlinks should not be recursed into to prevent infinite
  loops【705979169631452†L80-L85】.
* Existing repository indices are loaded before a new backup so that
  successive backups append to the in-memory index rather than
  overwrite it. When saving the index, the full set of entries is
  written anew.
* When restoring special file types such as FIFOs or devices, the
  absence of a data file in ``data/`` does not cause the restore to
  fail. Instead, the file is created based on its metadata alone.

The packaging format closely follows the C++ implementation. Each
package begins with a 6-byte magic string (``b"SEXP01"``), a
version byte and three one-byte fields indicating the packing,
compression and encryption algorithms. A 32-bit little-endian length
followed by that many bytes encodes a random salt used during
encryption. For the header-per-file mode, each file's path length,
path, original size, payload size and payload are written in
sequence. For the table-of-contents mode, all payloads are written
first, then a TOC block containing each file's path, original size,
payload offset and payload size, and finally an 8‑byte little-endian
offset to the start of the TOC block.
"""

from __future__ import annotations

import os
import stat
import time
import json
import shutil
import struct
import random
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List, Optional, Tuple, Union, Iterable


# -----------------------------------------------------------------------------
# File type enumeration

class FileType:
    """Enumeration of supported file types."""
    REGULAR = "regular"
    SYMLINK = "symlink"
    FIFO = "fifo"
    BLOCK = "block"
    CHARACTER = "char"
    SOCKET = "socket"
    DIRECTORY = "directory"
    OTHER = "other"


def detect_file_type(path: Path) -> str:
    """Determine the type of a filesystem entry using lstat.

    Args:
        path: The filesystem path to examine.

    Returns:
        A string corresponding to a member of :class:`FileType`.
    """
    try:
        st = os.lstat(path)
    except FileNotFoundError:
        return FileType.OTHER
    mode = st.st_mode
    if stat.S_ISREG(mode):
        return FileType.REGULAR
    if stat.S_ISLNK(mode):
        return FileType.SYMLINK
    if stat.S_ISFIFO(mode):
        return FileType.FIFO
    if stat.S_ISBLK(mode):
        return FileType.BLOCK
    if stat.S_ISCHR(mode):
        return FileType.CHARACTER
    if stat.S_ISSOCK(mode):
        return FileType.SOCKET
    if stat.S_ISDIR(mode):
        return FileType.DIRECTORY
    return FileType.OTHER


# -----------------------------------------------------------------------------
# Metadata representation

@dataclass
class Metadata:
    """Represents file metadata preserved during backup and restored later."""

    mode: int = 0
    mtime: int = 0
    uid: int = 0
    gid: int = 0
    file_type: str = FileType.OTHER
    dev_major: int = 0
    dev_minor: int = 0
    is_symlink: bool = False
    symlink_target: str = ""

    def load_from_file(self, path: Path) -> bool:
        """Populate metadata from a filesystem path.

        Records the stat information using ``os.lstat`` so that symlink
        information is captured. If the entry is a symbolic link, the
        target path is saved in ``symlink_target``.

        Args:
            path: Path to the file whose metadata should be loaded.

        Returns:
            True on success, False if the file does not exist or stat fails.
        """
        try:
            st = os.lstat(path)
        except (FileNotFoundError, OSError):
            return False
        self.mode = st.st_mode
        self.mtime = int(st.st_mtime)
        self.uid = st.st_uid
        self.gid = st.st_gid
        self.file_type = detect_file_type(path)
        # Record device numbers for block/char devices
        if self.file_type in (FileType.BLOCK, FileType.CHARACTER):
            self.dev_major = os.major(st.st_rdev)
            self.dev_minor = os.minor(st.st_rdev)
        else:
            self.dev_major = 0
            self.dev_minor = 0
        if self.file_type == FileType.SYMLINK:
            try:
                self.symlink_target = os.readlink(path)
                self.is_symlink = True
            except (OSError, RuntimeError):
                self.symlink_target = ""
                self.is_symlink = True
        else:
            self.is_symlink = False
            self.symlink_target = ""
        return True

    def apply_to_file(self, path: Path) -> bool:
        """Apply stored mode and modification time to a filesystem path.

        Args:
            path: The path on which to set metadata.

        Returns:
            True if successful, False otherwise.
        """
        # Set permissions (excluding file type bits).
        try:
            # Only change mode bits that affect permissions; file type bits are preserved.
            perm_mask = stat.S_IMODE(self.mode)
            os.chmod(path, perm_mask, follow_symlinks=False)
        except PermissionError:
            # Cannot change permissions; ignore failure.
            return False
        # Set modification time (and access time to the same value)
        try:
            os.utime(path, (self.mtime, self.mtime), follow_symlinks=False)
        except Exception:
            return False
        # Note: Changing uid/gid requires root; skipped.
        return True

    def serialize(self) -> str:
        """Serialize metadata into a colon-separated string.

        The format mirrors the C++ implementation:

        ``mode:mtime:uid:gid:file_type:dev_major:dev_minor:is_symlink:symlink_target``.
        ``file_type`` is represented as a string.

        Returns:
            A string representation of this metadata.
        """
        return (
            f"{self.mode}:{self.mtime}:{self.uid}:{self.gid}:{self.file_type}:"
            f"{self.dev_major}:{self.dev_minor}:{1 if self.is_symlink else 0}:"
            f"{self.symlink_target}"
        )

    @staticmethod
    def deserialize(data: str) -> 'Metadata':
        """Parse a serialized metadata string back into a Metadata object.

        Args:
            data: The serialized metadata.

        Returns:
            A Metadata instance.

        Raises:
            ValueError if the format cannot be parsed.
        """
        fields = data.split(":", 8)
        if len(fields) < 9:
            raise ValueError("metadata string has too few fields")
        m = Metadata()
        try:
            m.mode = int(fields[0])
            m.mtime = int(fields[1])
            m.uid = int(fields[2])
            m.gid = int(fields[3])
            m.file_type = fields[4]
            m.dev_major = int(fields[5])
            m.dev_minor = int(fields[6])
            m.is_symlink = (int(fields[7]) == 1)
            m.symlink_target = fields[8]
        except Exception as exc:
            raise ValueError("failed to parse metadata: " + str(exc)) from exc
        return m


# -----------------------------------------------------------------------------
# Filters

class FilterBase:
    """Base class for backup filters. Subclasses should override
    :meth:`should_include` to determine whether a given file should be
    included in the backup.
    """

    def should_include(self, path: Path) -> bool:
        return True


class PathFilter(FilterBase):
    """Filter that includes or excludes specific paths."""

    def __init__(self):
        self.includes: List[Path] = []
        self.excludes: List[Path] = []

    def add_include(self, p: Union[str, Path]):
        self.includes.append(Path(p))

    def add_exclude(self, p: Union[str, Path]):
        self.excludes.append(Path(p))

    def should_include(self, path: Path) -> bool:
        # Convert to absolute path for comparison
        abs_path = path.resolve()
        # If includes list is non-empty, only include those prefixes
        if self.includes:
            if not any(abs_path.is_relative_to(inc.resolve()) for inc in self.includes):
                return False
        # Exclude any matching prefixes
        if any(abs_path.is_relative_to(exc.resolve()) for exc in self.excludes):
            return False
        return True


class FileTypeFilter(FilterBase):
    """Filter that allows a set of file types."""

    def __init__(self):
        self.allowed: set[str] = set()

    def add_allowed(self, ftype: str):
        self.allowed.add(ftype)

    def should_include(self, path: Path) -> bool:
        if not self.allowed:
            return True
        return detect_file_type(path) in self.allowed


class NameFilter(FilterBase):
    """Filter files whose names contain specified substrings."""

    def __init__(self):
        self.contains: List[str] = []

    def add_contains(self, text: str):
        self.contains.append(text)

    def should_include(self, path: Path) -> bool:
        name = path.name
        return all(sub in name for sub in self.contains) if self.contains else True


class TimeFilter(FilterBase):
    """Filter files by modification time."""

    def __init__(self):
        self.after: Optional[int] = None
        self.before: Optional[int] = None

    def set_after(self, ts: int):
        self.after = ts

    def set_before(self, ts: int):
        self.before = ts

    def should_include(self, path: Path) -> bool:
        try:
            mtime = int(os.lstat(path).st_mtime)
        except Exception:
            return False
        if self.after is not None and mtime < self.after:
            return False
        if self.before is not None and mtime > self.before:
            return False
        return True


class SizeFilter(FilterBase):
    """Filter files by size."""

    def __init__(self):
        self.min_size: Optional[int] = None
        self.max_size: Optional[int] = None

    def set_min_size(self, s: int):
        self.min_size = s

    def set_max_size(self, s: int):
        self.max_size = s

    def should_include(self, path: Path) -> bool:
        try:
            size = os.lstat(path).st_size
        except Exception:
            return False
        if self.min_size is not None and size < self.min_size:
            return False
        if self.max_size is not None and size > self.max_size:
            return False
        return True


class UserFilter(FilterBase):
    """Filter files by UID/GID."""

    def __init__(self):
        self.uid: Optional[int] = None
        self.gid: Optional[int] = None

    def set_uid(self, uid: int):
        self.uid = uid

    def set_gid(self, gid: int):
        self.gid = gid

    def should_include(self, path: Path) -> bool:
        try:
            st = os.lstat(path)
        except Exception:
            return False
        if self.uid is not None and st.st_uid != self.uid:
            return False
        if self.gid is not None and st.st_gid != self.gid:
            return False
        return True


class FilterChain(FilterBase):
    """Combine multiple filters with logical AND."""

    def __init__(self):
        self.filters: List[FilterBase] = []

    def add_filter(self, f: FilterBase):
        self.filters.append(f)

    def should_include(self, path: Path) -> bool:
        return all(f.should_include(path) for f in self.filters)


# -----------------------------------------------------------------------------
# Repository

class Repository:
    """Represents a backup repository stored on disk."""

    def __init__(self, repo_path: Union[str, Path]):
        self.repo_path = Path(repo_path)
        self.data_dir = self.repo_path / "data"
        self.index_file = self.repo_path / "index.txt"
        # relative path -> Metadata
        self.index: Dict[str, Metadata] = {}

    def initialize(self) -> bool:
        """Create the repository and data directories if they do not exist."""
        try:
            self.repo_path.mkdir(parents=True, exist_ok=True)
            self.data_dir.mkdir(parents=True, exist_ok=True)
            return True
        except Exception:
            return False

    def get_storage_path(self, relative_path: Path) -> Path:
        return self.data_dir / relative_path

    def load_index(self) -> bool:
        """Load index from index file if present. If the file does not exist
        the in-memory index remains empty. Directory structure is created as
        needed.
        """
        # Ensure repository directories exist
        self.repo_path.mkdir(parents=True, exist_ok=True)
        self.data_dir.mkdir(parents=True, exist_ok=True)
        if not self.index_file.exists():
            return True
        try:
            with self.index_file.open("r", encoding="utf-8") as f:
                self.index.clear()
                for line in f:
                    line = line.rstrip("\n")
                    if not line:
                        continue
                    parts = line.split("\t", 1)
                    if len(parts) != 2:
                        continue
                    rel_path_str, meta_str = parts
                    try:
                        meta = Metadata.deserialize(meta_str)
                    except Exception:
                        continue
                    # Use string key to avoid issues with platform-specific path
                    self.index[rel_path_str] = meta
            return True
        except Exception:
            return False

    def save_index(self) -> bool:
        """Write the current index map to disk. Overwrites any existing file."""
        try:
            with self.index_file.open("w", encoding="utf-8") as f:
                for rel_path, meta in self.index.items():
                    f.write(f"{rel_path}\t{meta.serialize()}\n")
            return True
        except Exception:
            return False

    def store_file(self, source_path: Union[str, Path], relative_path: Union[str, Path], metadata: Metadata) -> bool:
        """Store a single file's data and metadata into the repository.

        Args:
            source_path: Absolute path to the source file to copy.
            relative_path: Path relative to the backup root (used in index and data dir).
            metadata: The file's metadata to record.

        Returns:
            True on success, False otherwise.
        """
        spath = Path(source_path)
        rel = Path(relative_path)
        storage_path = self.get_storage_path(rel)
        try:
            storage_path.parent.mkdir(parents=True, exist_ok=True)
            ftype = metadata.file_type
            if ftype in (FileType.REGULAR, FileType.SYMLINK):
                # Copy file or symlink
                if ftype == FileType.SYMLINK:
                    target = os.readlink(spath)
                    # Remove any existing file
                    if storage_path.exists() or storage_path.is_symlink():
                        storage_path.unlink()
                    os.symlink(target, storage_path)
                else:
                    # Regular file
                    shutil.copy2(spath, storage_path)
            else:
                # Do not store data for special files (FIFO, devices, sockets)
                pass
            # Update index
            self.index[str(rel)] = metadata
            return True
        except Exception:
            return False

    def restore_file(self, relative_path: Union[str, Path], target_path: Union[str, Path]) -> bool:
        """Restore a single file given its relative path and repository metadata.

        If the file is a regular file or symlink, the data is copied from the
        repository. For FIFOs and devices, a new special file is created
        according to the metadata. Metadata (permissions and mtime) is then
        applied.

        Args:
            relative_path: The key used in the index.
            target_path: The destination path on disk.

        Returns:
            True if restored successfully, False otherwise.
        """
        rel_str = str(Path(relative_path))
        meta = self.index.get(rel_str)
        if meta is None:
            return False
        tpath = Path(target_path)
        try:
            # Ensure parent directory exists
            tpath.parent.mkdir(parents=True, exist_ok=True)
            ftype = meta.file_type
            if ftype in (FileType.REGULAR, FileType.SYMLINK):
                src = self.get_storage_path(Path(relative_path))
                if not src.exists() and ftype == FileType.REGULAR:
                    return False
                if ftype == FileType.SYMLINK:
                    # Create symlink
                    if tpath.exists() or tpath.is_symlink():
                        tpath.unlink()
                    os.symlink(meta.symlink_target, tpath)
                else:
                    shutil.copy2(src, tpath)
            elif ftype == FileType.FIFO:
                # Create FIFO
                # Remove existing file if any
                if tpath.exists() or tpath.is_symlink():
                    tpath.unlink()
                # Mode bits include permissions only
                mode_perm = stat.S_IMODE(meta.mode)
                os.mkfifo(tpath, mode_perm)
            elif ftype in (FileType.BLOCK, FileType.CHARACTER):
                # Create device file if possible
                try:
                    if tpath.exists():
                        tpath.unlink()
                    mode_perm = stat.S_IMODE(meta.mode)
                    dev = os.makedev(meta.dev_major, meta.dev_minor)
                    file_mode = stat.S_IFBLK if ftype == FileType.BLOCK else stat.S_IFCHR
                    os.mknod(tpath, file_mode | mode_perm, dev)
                except PermissionError:
                    # Cannot create device file without privileges; skip
                    pass
            elif ftype == FileType.SOCKET:
                # Cannot restore sockets; ignore
                pass
            # Apply metadata (permissions and mtime). For certain file types
            # (e.g., symlinks) this may raise an exception if the platform
            # does not allow changing attributes on the symlink itself.
            try:
                meta.apply_to_file(tpath)
            except Exception:
                # Ignore metadata application failures on special types
                pass
            return True
        except Exception:
            # Unexpected exception during restoration; indicate failure
            return False

    def list_files(self) -> List[str]:
        return list(self.index.keys())

    def get_metadata(self, relative_path: Union[str, Path]) -> Optional[Metadata]:
        return self.index.get(str(Path(relative_path)))


# -----------------------------------------------------------------------------
# Backup and restore operations

class Backup:
    """Backup operation to copy files from a source tree into a repository."""

    def __init__(self, repo: Repository):
        self.repo = repo

    def execute(self, source_root: Union[str, Path], filter_chain: Optional[FilterBase] = None) -> Tuple[int, int, int]:
        """Perform backup of ``source_root`` into ``repo``.

        Args:
            source_root: Root directory to back up.
            filter_chain: Optional filter chain to select files.

        Returns:
            A tuple ``(success_count, failed_count, skipped_count)`` summarizing
            the backup results.
        """
        root = Path(source_root)
        if not root.exists():
            raise FileNotFoundError(f"source directory does not exist: {root}")
        # Load existing index to avoid overwriting
        self.repo.load_index()
        success = 0
        failed = 0
        skipped = 0
        # Walk directory tree; skip symlinked directories to avoid loops
        for dirpath, dirnames, filenames in os.walk(root, followlinks=False):
            # Skip symlink directories explicitly to avoid cycles【705979169631452†L80-L85】
            # Remove any directory entries that are symlinks from dirnames
            dirnames[:] = [d for d in dirnames if not (Path(dirpath) / d).is_symlink()]
            for name in filenames:
                fpath = Path(dirpath) / name
                # Skip directories (in case they appear here) and other
                # unsupported types at this level
                ftype = detect_file_type(fpath)
                if ftype == FileType.DIRECTORY:
                    continue
                if filter_chain and not filter_chain.should_include(fpath):
                    skipped += 1
                    continue
                # Check if backup supports this type
                if ftype not in (FileType.REGULAR, FileType.SYMLINK, FileType.FIFO,
                                 FileType.BLOCK, FileType.CHARACTER, FileType.SOCKET):
                    skipped += 1
                    continue
                # Compute relative path
                try:
                    rel = fpath.relative_to(root)
                except ValueError:
                    # On different mount? skip
                    skipped += 1
                    continue
                meta = Metadata()
                if not meta.load_from_file(fpath):
                    failed += 1
                    continue
                if self.repo.store_file(fpath, rel, meta):
                    success += 1
                else:
                    failed += 1
        # Save index
        self.repo.save_index()
        return success, failed, skipped


class Restore:
    """Restore operation to reconstruct files from a repository."""

    def __init__(self, repo: Repository):
        self.repo = repo

    def execute(self, target_root: Union[str, Path]) -> Tuple[int, int]:
        """Restore all files from the repository into ``target_root``.

        Args:
            target_root: Destination directory to restore files into.

        Returns:
            A tuple ``(success_count, failed_count)`` summarizing results.
        """
        # Load index
        self.repo.load_index()
        success = 0
        failed = 0
        root = Path(target_root)
        for rel_path in self.repo.list_files():
            meta = self.repo.get_metadata(rel_path)
            if meta is None:
                failed += 1
                continue
            dest = root / rel_path
            if self.repo.restore_file(rel_path, dest):
                success += 1
            else:
                failed += 1
        return success, failed


# -----------------------------------------------------------------------------
# Compression algorithms (RLE)

def rle_compress(data: bytes) -> bytes:
    """Compress a byte sequence using run-length encoding.

    Format: pairs of (count, byte) where count is a single byte from 1..255.

    Args:
        data: Raw bytes to compress.

    Returns:
        Compressed bytes.
    """
    if not data:
        return b""
    out = bytearray()
    i = 0
    n = len(data)
    while i < n:
        b = data[i]
        j = i + 1
        # Count up to 255 identical bytes
        while j < n and data[j] == b and (j - i) < 255:
            j += 1
        count = j - i
        out.append(count)
        out.append(b)
        i = j
    return bytes(out)


def rle_decompress(data: bytes) -> bytes:
    """Decompress RLE-compressed bytes.

    Args:
        data: Compressed bytes, expected even length.

    Returns:
        Decompressed bytes.
    """
    if not data:
        return b""
    if len(data) % 2 != 0:
        raise ValueError("RLE data length must be even")
    out = bytearray()
    for i in range(0, len(data), 2):
        count = data[i]
        byte = data[i + 1]
        if count == 0:
            raise ValueError("RLE count cannot be zero")
        out.extend([byte] * count)
    return bytes(out)


# -----------------------------------------------------------------------------
# Encryption algorithms

def fnv1a32(password: str, salt: bytes) -> int:
    """Compute a 32-bit FNV-1a hash of a password and salt."""
    h = 2166136261
    for c in password.encode('utf-8'):
        h ^= c
        h = (h * 16777619) & 0xFFFFFFFF
    for b in salt:
        h ^= b
        h = (h * 16777619) & 0xFFFFFFFF
    return h


def xor_encrypt(data: bytes, password: str, salt: bytes) -> bytes:
    """Encrypt/decrypt data using a simple XOR stream cipher.

    The key stream is generated using a xorshift32 PRNG seeded with the
    FNV-1a hash of the password and salt.
    """
    out = bytearray(len(data))
    state = fnv1a32(password, salt)
    for i, b in enumerate(data):
        # Xorshift32
        state ^= (state << 13) & 0xFFFFFFFF
        state ^= (state >> 17) & 0xFFFFFFFF
        state ^= (state << 5) & 0xFFFFFFFF
        rnd = state & 0xFF
        out[i] = b ^ rnd
    return bytes(out)


def rc4_encrypt(data: bytes, password: str, salt: bytes) -> bytes:
    """Encrypt/decrypt data using the RC4 stream cipher."""
    # Construct key from password + salt
    key = password.encode('utf-8') + salt
    if not key:
        key = b"\0"
    # Initialize S-box
    S = list(range(256))
    j = 0
    # KSA
    for i in range(256):
        j = (j + S[i] + key[i % len(key)]) & 0xFF
        S[i], S[j] = S[j], S[i]
    # PRGA
    out = bytearray(len(data))
    i = 0
    j = 0
    for k in range(len(data)):
        i = (i + 1) & 0xFF
        j = (j + S[i]) & 0xFF
        S[i], S[j] = S[j], S[i]
        rnd = S[(S[i] + S[j]) & 0xFF]
        out[k] = data[k] ^ rnd
    return bytes(out)


# -----------------------------------------------------------------------------
# Packaging algorithms

class PackAlg:
    HEADER = 1
    TOC = 2


class CompressAlg:
    NONE = 0
    RLE = 1


class EncryptAlg:
    NONE = 0
    XOR = 1
    RC4 = 2


def generate_salt(n: int = 16) -> bytes:
    return bytes(random.randint(0, 255) for _ in range(n))


def apply_compress(data: bytes, alg: int) -> bytes:
    if alg == CompressAlg.RLE:
        return rle_compress(data)
    return data


def apply_decompress(data: bytes, alg: int) -> bytes:
    if alg == CompressAlg.RLE:
        return rle_decompress(data)
    return data


def apply_encrypt(data: bytes, alg: int, password: str, salt: bytes) -> bytes:
    if alg == EncryptAlg.XOR:
        return xor_encrypt(data, password, salt)
    if alg == EncryptAlg.RC4:
        return rc4_encrypt(data, password, salt)
    return data


def apply_decrypt(data: bytes, alg: int, password: str, salt: bytes) -> bytes:
    # XOR and RC4 are symmetric
    return apply_encrypt(data, alg, password, salt)


MAGIC = b"SEXP01"


def export_repo_to_package(repo_dir: Union[str, Path], package_file: Union[str, Path],
                           pack_alg: int = PackAlg.HEADER,
                           compress_alg: int = CompressAlg.NONE,
                           encrypt_alg: int = EncryptAlg.NONE,
                           password: str = "") -> None:
    """Export the contents of a repository directory into a single package file.

    This function walks the repository directory tree and records each
    file's relative path, type, original size and payload. A small
    header for each entry records the path length, the UTF‑8 encoded
    path, a one‑byte file type code (0=regular, 1=symlink, 2=FIFO,
    3=block device, 4=char device, 5=socket, 6=other), the original
    size (for regular files, the file length; for symlinks, ignored) and
    the payload size. The payload is either the raw file contents or, in
    the case of symlinks, the link target string. The payload is
    optionally compressed and encrypted according to the selected
    algorithms. In TOC mode, payloads are written sequentially and a
    table of contents (containing the same header information plus
    offsets and sizes) is appended at the end. A 6‑byte magic string
    ``b"SEXP01"``, a version byte and algorithm identifiers precede all
    entries.

    Args:
        repo_dir: The repository directory containing ``index.txt`` and ``data/``.
        package_file: The output package file path.
        pack_alg: Packing algorithm (1=header per file, 2=TOC at end).
        compress_alg: Compression algorithm (0=none, 1=RLE).
        encrypt_alg: Encryption algorithm (0=none, 1=XOR, 2=RC4).
        password: Password used for encryption. If encryption is requested and
            password is empty, a ``ValueError`` is raised.

    Raises:
        FileNotFoundError: If the repository does not exist.
        ValueError: If encryption is enabled but no password is provided.
        IOError: On failure to write the package file.
    """
    repo_path = Path(repo_dir)
    if not repo_path.exists():
        raise FileNotFoundError(f"repository not found: {repo_path}")
    if encrypt_alg != EncryptAlg.NONE and not password:
        raise ValueError("password is required for encryption")
    salt = generate_salt() if encrypt_alg != EncryptAlg.NONE else b""
    # Mapping from FileType to code
    type_to_code = {
        FileType.REGULAR: 0,
        FileType.SYMLINK: 1,
        FileType.FIFO: 2,
        FileType.BLOCK: 3,
        FileType.CHARACTER: 4,
        FileType.SOCKET: 5,
        FileType.OTHER: 6,
    }
    # Collect entries: (rel_path, type_code, original_size, payload_bytes)
    entries: List[Tuple[str, int, int, bytes]] = []
    # Walk repository tree
    for root, dirs, files in os.walk(repo_path):
        # Sort entries to have deterministic ordering
        files = sorted(files)
        for fname in files:
            abs_path = Path(root) / fname
            # Skip package file itself if inside repo_dir
            try:
                if Path(package_file).exists() and os.path.samefile(abs_path, package_file):
                    continue
            except Exception:
                pass
            # Only include files (we intentionally skip directories; their entries can be reconstructed)
            try:
                st = os.lstat(abs_path)
            except OSError:
                continue
            ftype = detect_file_type(abs_path)
            # Determine original size and raw payload depending on type
            if ftype == FileType.SYMLINK:
                try:
                    # Symlink: payload is link target
                    link_target = os.readlink(abs_path)
                    raw = link_target.encode('utf-8')
                    original_size = 0
                except OSError:
                    # Broken link; treat as other
                    raw = b""
                    original_size = 0
            elif ftype == FileType.REGULAR:
                try:
                    with open(abs_path, 'rb') as f:
                        raw = f.read()
                    original_size = len(raw)
                except Exception:
                    raw = b""
                    original_size = 0
            else:
                # For special types, no payload stored; original size ignored
                raw = b""
                original_size = 0
            # Compress and encrypt payload
            comp = apply_compress(raw, compress_alg)
            enc = apply_encrypt(comp, encrypt_alg, password, salt)
            rel = os.path.relpath(abs_path, repo_path)
            rel = rel.replace(os.sep, '/')
            type_code = type_to_code.get(ftype, 6)
            entries.append((rel, type_code, original_size, enc))
    # Write package file
    pkg_path = Path(package_file)
    pkg_path.parent.mkdir(parents=True, exist_ok=True)
    with open(pkg_path, 'wb') as out:
        # Header: magic + version + algorithm identifiers + salt length + salt
        out.write(MAGIC)
        out.write(struct.pack('<B', 1))  # version
        out.write(struct.pack('<B', pack_alg))
        out.write(struct.pack('<B', compress_alg))
        out.write(struct.pack('<B', encrypt_alg))
        out.write(struct.pack('<I', len(salt)))
        out.write(salt)
        if pack_alg == PackAlg.HEADER:
            # For each entry: path length, path bytes, type code, original size, payload size, payload
            for rel_path, type_code, original_size, payload in entries:
                rel_bytes = rel_path.encode('utf-8')
                out.write(struct.pack('<I', len(rel_bytes)))
                out.write(rel_bytes)
                out.write(struct.pack('<B', type_code))
                out.write(struct.pack('<Q', original_size))
                out.write(struct.pack('<Q', len(payload)))
                out.write(payload)
        else:
            # TOC mode: write payloads first and record offsets
            toc: List[Tuple[str, int, int, int, int]] = []  # rel_path, type_code, original_size, offset, size
            for rel_path, type_code, original_size, payload in entries:
                offset = out.tell()
                size = len(payload)
                out.write(payload)
                toc.append((rel_path, type_code, original_size, offset, size))
            toc_offset = out.tell()
            # Write TOC magic
            out.write(b'TOC1')
            out.write(struct.pack('<I', len(toc)))
            for rel_path, type_code, original_size, offset, size in toc:
                rel_bytes = rel_path.encode('utf-8')
                out.write(struct.pack('<I', len(rel_bytes)))
                out.write(rel_bytes)
                out.write(struct.pack('<B', type_code))
                out.write(struct.pack('<Q', original_size))
                out.write(struct.pack('<Q', offset))
                out.write(struct.pack('<Q', size))
            # Append TOC offset
            out.write(struct.pack('<Q', toc_offset))


def import_package_to_repo(package_file: Union[str, Path], repo_dir: Union[str, Path], password: str = "") -> None:
    """Import a package file into a repository directory.

    This function extracts entries previously written by
    :func:`export_repo_to_package` into a destination directory. It
    recreates files according to their stored type codes. Regular files
    are written using the raw payload, while symbolic links are
    recreated by reading the payload as a UTF‑8 encoded link target and
    calling ``os.symlink``. Special file types (FIFO, block/char
    devices, sockets and others) are recreated as empty files because
    repository contents do not rely on their data; these types rarely
    occur in a repository, but the codes are reserved for completeness.

    Args:
        package_file: Path to the package file.
        repo_dir: Path to the destination repository directory.
        password: Password for decryption.

    Raises:
        FileNotFoundError: If the package file does not exist.
        ValueError: If decryption fails due to missing password or
            corrupted format.
        IOError: On file reading/writing errors.
    """
    pkg_path = Path(package_file)
    if not pkg_path.exists():
        raise FileNotFoundError(f"package not found: {pkg_path}")
    repo_path = Path(repo_dir)
    repo_path.mkdir(parents=True, exist_ok=True)
    with open(pkg_path, 'rb') as f:
        magic = f.read(len(MAGIC))
        if magic != MAGIC:
            raise ValueError("package magic mismatch")
        version = struct.unpack('<B', f.read(1))[0]
        pack_alg = struct.unpack('<B', f.read(1))[0]
        compress_alg = struct.unpack('<B', f.read(1))[0]
        encrypt_alg = struct.unpack('<B', f.read(1))[0]
        salt_len = struct.unpack('<I', f.read(4))[0]
        salt = f.read(salt_len)
        if encrypt_alg != EncryptAlg.NONE and not password:
            raise ValueError("password is required for decryption")
        # Helper to process payload -> raw bytes
        def process_payload(payload: bytes) -> bytes:
            dec = apply_decrypt(payload, encrypt_alg, password, salt)
            raw = apply_decompress(dec, compress_alg)
            return raw
        # Reverse mapping for file type codes
        code_to_type = {
            0: FileType.REGULAR,
            1: FileType.SYMLINK,
            2: FileType.FIFO,
            3: FileType.BLOCK,
            4: FileType.CHARACTER,
            5: FileType.SOCKET,
            6: FileType.OTHER,
        }
        if pack_alg == PackAlg.HEADER:
            # Sequentially read entries
            while True:
                len_bytes = f.read(4)
                if not len_bytes:
                    break
                (name_len,) = struct.unpack('<I', len_bytes)
                name = f.read(name_len).decode('utf-8')
                type_code = struct.unpack('<B', f.read(1))[0]
                original_size = struct.unpack('<Q', f.read(8))[0]
                payload_size = struct.unpack('<Q', f.read(8))[0]
                payload = f.read(payload_size)
                raw = process_payload(payload)
                ftype = code_to_type.get(type_code, FileType.OTHER)
                out_path = repo_path / Path(name)
                out_path.parent.mkdir(parents=True, exist_ok=True)
                # Create file based on type
                if ftype == FileType.SYMLINK:
                    # Payload is UTF-8 encoded link target
                    link_target = raw.decode('utf-8') if raw else ''
                    # Remove existing file if any
                    if out_path.exists() or out_path.is_symlink():
                        out_path.unlink()
                    os.symlink(link_target, out_path)
                elif ftype == FileType.REGULAR:
                    # Write file content
                    with open(out_path, 'wb') as out:
                        out.write(raw)
                else:
                    # For other types, create empty file to preserve existence
                    with open(out_path, 'wb') as out:
                        out.write(raw)
        else:
            # TOC mode: read toc offset from end
            f.seek(-8, os.SEEK_END)
            toc_offset_bytes = f.read(8)
            if len(toc_offset_bytes) != 8:
                raise ValueError("invalid package structure: missing toc offset")
            toc_offset = struct.unpack('<Q', toc_offset_bytes)[0]
            # Jump to toc_offset
            f.seek(toc_offset, os.SEEK_SET)
            toc_magic = f.read(4)
            if toc_magic != b'TOC1':
                raise ValueError("TOC magic mismatch")
            toc_count = struct.unpack('<I', f.read(4))[0]
            toc_entries = []
            for _ in range(toc_count):
                name_len = struct.unpack('<I', f.read(4))[0]
                name = f.read(name_len).decode('utf-8')
                type_code = struct.unpack('<B', f.read(1))[0]
                original_size = struct.unpack('<Q', f.read(8))[0]
                offset = struct.unpack('<Q', f.read(8))[0]
                size = struct.unpack('<Q', f.read(8))[0]
                toc_entries.append((name, type_code, original_size, offset, size))
            # For each toc entry, restore file
            for name, type_code, original_size, offset, size in toc_entries:
                f.seek(offset, os.SEEK_SET)
                payload = f.read(size)
                raw = process_payload(payload)
                ftype = code_to_type.get(type_code, FileType.OTHER)
                out_path = repo_path / Path(name)
                out_path.parent.mkdir(parents=True, exist_ok=True)
                if ftype == FileType.SYMLINK:
                    link_target = raw.decode('utf-8') if raw else ''
                    if out_path.exists() or out_path.is_symlink():
                        out_path.unlink()
                    os.symlink(link_target, out_path)
                elif ftype == FileType.REGULAR:
                    with open(out_path, 'wb') as out:
                        out.write(raw)
                else:
                    with open(out_path, 'wb') as out:
                        out.write(raw)


# -----------------------------------------------------------------------------
# End of module
