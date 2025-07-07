import os

def analyze_codebase(root_dir):
    total_lines = 0
    total_chars = 0
    code_extensions = {'.c', '.h', '.rs', '.py', '.js', '.html', '.css', '.java', '.cpp', '.hpp', '.go', '.fls', '.toml'}

    print(f"Starting analysis in: {root_dir}")
    print(f"Looking for files with extensions: {', '.join(code_extensions)}\n")

    for subdir, _, files in os.walk(root_dir):
        if '.git' in subdir:
            continue

        for filename in files:
            if any(filename.endswith(ext) for ext in code_extensions):
                filepath = os.path.join(subdir, filename)
                try:
                    with open(filepath, 'r', encoding='utf-8', errors='ignore') as f:
                        lines = f.readlines()
                        num_lines = len(lines)
                        num_chars = sum(len(line) for line in lines)

                        total_lines += num_lines
                        total_chars += num_chars
                except Exception as e:
                    print(f"Could not read file {filepath}: {e}")

    print("--- Analysis Complete ---")
    print(f"Total lines of code: {total_lines:,}")
    print(f"Total characters:    {total_chars:,}")
    print("-------------------------")

if __name__ == "__main__":
    target_directory = "/home/maxim/Documents/FishyC/"
    analyze_codebase(target_directory)
