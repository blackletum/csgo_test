import os
import re
from datetime import datetime

# Config
MIN_VERSION = "3.5"
PROJECT_ROOT = "."
LOG_DIR = "./logs"
LOG_FILE = os.path.join(LOG_DIR, "cmake_patch_log.txt")

# Regex patterns
CMAKE_MIN_REQ_RE = re.compile(r"^\s*cmake_minimum_required\s*\(\s*VERSION\s+([\d\.]+)", re.IGNORECASE)
CMAKE_POLICY_RE = re.compile(r"^\s*cmake_policy\s*\(\s*VERSION\s+([\d\.]+)", re.IGNORECASE)

def parse_version(version_str):
    return tuple(map(int, version_str.strip().split(".")[:2]))

def needs_update(old, new):
    return parse_version(old) < parse_version(new)

def update_file(path, log_entries):
    updated = False
    with open(path, "r", encoding="utf-8") as f:
        lines = f.readlines()

    for i, line in enumerate(lines):
        min_match = CMAKE_MIN_REQ_RE.match(line)
        policy_match = CMAKE_POLICY_RE.match(line)

        if min_match and needs_update(min_match.group(1), MIN_VERSION):
            lines[i] = re.sub(CMAKE_MIN_REQ_RE, f"cmake_minimum_required(VERSION {MIN_VERSION}", line)
            updated = True

        if policy_match and needs_update(policy_match.group(1), MIN_VERSION):
            lines[i] = re.sub(CMAKE_POLICY_RE, f"cmake_policy(VERSION {MIN_VERSION}", line)
            updated = True

    if updated:
        with open(path, "w", encoding="utf-8") as f:
            f.writelines(lines)
        print(f"[UPDATED] {path}")
        log_entries.append(path)

def write_log(entries):
    os.makedirs(LOG_DIR, exist_ok=True)
    with open(LOG_FILE, "a", encoding="utf-8") as f:
        f.write(f"\n--- CMake Patch Run @ {datetime.now()} ---\n")
        for path in entries:
            f.write(f"{path}\n")

def walk_and_patch(root):
    modified_files = []
    for dirpath, _, filenames in os.walk(root):
        for filename in filenames:
            if filename == "CMakeLists.txt":
                full_path = os.path.join(dirpath, filename)
                update_file(full_path, modified_files)
    if modified_files:
        write_log(modified_files)
    else:
        print("[OK] No changes needed.")

if __name__ == "__main__":
    walk_and_patch(PROJECT_ROOT)