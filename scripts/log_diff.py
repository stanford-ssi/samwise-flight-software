"""
This program calculates the difference between two codebases using delta encoding. 
It should NOT be pushed in the final version of the codebase.
Edit codebase paths to run the program.
@author: Darrow Hartman
@date: 2025-04-06
"""

import os
import difflib
import argparse

def get_all_c_files(base_dir):
    c_files = {}
    for root, _, files in os.walk(base_dir):
        for f in files:
            if f.endswith(".c") or f.endswith(".h"):
                full_path = os.path.join(root, f)
                relative_path = os.path.relpath(full_path, base_dir)
                c_files[relative_path] = full_path
    return c_files

def delta_encode_files(file1_path, file2_path):
    with open(file1_path, 'r', encoding='utf-8', errors='ignore') as f1:
        lines1 = f1.readlines()
    with open(file2_path, 'r', encoding='utf-8', errors='ignore') as f2:
        lines2 = f2.readlines()

    diff = difflib.unified_diff(
        lines1, lines2,
        fromfile=file1_path,
        tofile=file2_path,
        lineterm=''
    )
    return list(diff)

def delta_encode_codebases(dir1, dir2, output_log_file):
    files1 = get_all_c_files(dir1)
    files2 = get_all_c_files(dir2)

    all_keys = set(files1.keys()).union(files2.keys())

    with open(output_log_file, 'w') as log:
        for key in sorted(all_keys):
            path1 = files1.get(key)
            path2 = files2.get(key)

            if path1 and path2:
                # File exists in both
                diff = delta_encode_files(path1, path2)
                if diff:
                    log.write(f"\n--- Delta for {key} ---\n")
                    log.write('\n'.join(diff))
                    log.write("\n")
            elif path1:
                # File deleted in new codebase
                log.write(f"\n--- {key} deleted in {dir2} ---\n")
                with open(path1, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                    for line in lines:
                        log.write(f"- {line}")
            elif path2:
                # File added in new codebase
                log.write(f"\n--- {key} added in {dir2} ---\n")
                with open(path2, 'r', encoding='utf-8', errors='ignore') as f:
                    lines = f.readlines()
                    for line in lines:
                        log.write(f"+ {line}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Calculate differences between two codebases using delta encoding.')
    parser.add_argument('--old', '-o', required=True, help='Path to the old codebase')
    parser.add_argument('--new', '-n', required=True, help='Path to the new codebase')
    parser.add_argument('--output', '-out', default='delta_log.txt', help='Output log file path (default: delta_log.txt)')
    
    args = parser.parse_args()
    
    # Convert paths to absolute paths
    old_path = os.path.abspath(args.old)
    new_path = os.path.abspath(args.new)
    output_path = os.path.abspath(args.output)
    
    delta_encode_codebases(old_path, new_path, output_path)
    print(f"Delta encoding log saved to {output_path}")
