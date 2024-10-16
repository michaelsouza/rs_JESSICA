import os
import hashlib
from gitignore_parser import parse_gitignore
from filecmp import dircmp


def md5sum(filename):
    """Calculates the MD5 checksum of a file."""
    hash_md5 = hashlib.md5()
    with open(filename, "rb") as f:
        for chunk in iter(lambda: f.read(4096), b""):
            hash_md5.update(chunk)
    return hash_md5.hexdigest()


def compare_directories(dir1, dir2, gitignore_path):
    """Compares directories recursively, taking .gitignore into account."""
    gitignore = parse_gitignore(gitignore_path) if os.path.exists(gitignore_path) else lambda x: False

    # Get a comparison of the two directories
    comparison = dircmp(dir1, dir2)

    # Compare common files
    for common_file in comparison.common_files:
        path1 = os.path.join(dir1, common_file)
        path2 = os.path.join(dir2, common_file)
        if gitignore(path1) or gitignore(path2):
            continue
        if md5sum(path1) != md5sum(path2):
            print(f"DIFF in common file: {path1} != {path2}")

    # Check files only in dir1
    for file_only_in_dir1 in comparison.left_only:
        path1 = os.path.join(dir1, file_only_in_dir1)
        if gitignore(path1):
            continue
        print(f"File only in {dir1}: {path1}")

    # Check files only in dir2
    for file_only_in_dir2 in comparison.right_only:
        path2 = os.path.join(dir2, file_only_in_dir2)
        if gitignore(path2):
            continue
        print(f"File only in {dir2}: {path2}")

    # Recursively compare subdirectories
    for common_dir in comparison.common_dirs:
        compare_directories(
            os.path.join(dir1, common_dir),
            os.path.join(dir2, common_dir),
            gitignore_path
        )


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Check differences between two folders recursively.")
    parser.add_argument("folder1", type=str, help="First folder to compare.")
    parser.add_argument("folder2", type=str, help="Second folder to compare.")
    parser.add_argument("--gitignore", type=str, default=".gitignore", help="Path to .gitignore file (default is .gitignore in the root).")
    
    args = parser.parse_args()
    compare_directories(args.folder1, args.folder2, args.gitignore)