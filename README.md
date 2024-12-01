# Duq

A command-line tool for Unix-like systems for querying disk usage.

## Features

- **Displays Sizes**: Shows the size of each file, directory, or symbolic link.
- **Units Option**: Optionally display sizes with units (B, K, M, G, T) using
the `-u` flag.
- **Filtering**: Filter out entries smaller than a specified size using one of
the `-B`, `-K`, `-M`, `-G`, or `-T` options.
- **Handles Symlinks**: Symlinks are displayed with their targets.
- **Human-Readable Sizes**: In unit mode, sizes are formatted without trailing
zeros and no space between the number and the unit (e.g., `270M`, `1.234G`).
- **Sorted Output**: Entries are sorted from smallest to largest based on size.
- **Total Size**: Provides the total size of all listed entries.
- **Flexible Targeting**: Can list entries for the current directory or a
specified file/directory.

## Why Use Duq?

When managing disk space, it's essential to know which files or directories are
consuming the most space. Duq simplifies this by:

- Quickly identifying large files or directories.
- Providing a concise summary of directory contents with size information.
- Sorting entries by size for easy analysis.
- Filtering out insignificant entries to focus on what's important.

## Example Output

### Default Mode (Sizes in Bytes)

```bash
$ duq /usr
          0 libx32/
          0 emptyfile.txt
          4 tmp -> /tmp
         44 lib64/
      16071 games/
   14608973 lib32/
   51188336 sbin/
  130861414 libexec/
  142439756 include/
  374761882 src/
  482034160 local/
  819740296 bin/
 7083562589 share/
13531660099 lib/
22630873620 total
```

### Unit Mode (`-u` Option)

```bash
$ duq -u /usr
      0B libx32/
      0B emptyfile.txt
      4B tmp -> /tmp
     44B lib64/
 15.694K games/
 13.932M lib32/
 48.817M sbin/
124.799M libexec/
135.841M include/
357.401M src/
459.704M local/
781.765M bin/
  6.597G share/
 12.602G lib/
 21.077G total
```

### Filtering Entries (Only entries larger than 100MB)

```bash
$ duq -u -M100 /usr
124.799M libexec/
135.841M include/
357.401M src/
459.704M local/
781.765M bin/
  6.597G share/
 12.602G lib/
 21.015G total
```

## Usage

```bash
Usage: duq [OPTION] [TARGET]
List entries in a directory with their sizes sorted.

  TARGET     Directory or file to list. Defaults to current directory.

Options:
  -h, --help              Display this help message and exit.
  -v, --version           Display version information and exit.
  -u, --units             Display sizes with units (B, K, M, G, T) with up to 3 decimal places.
  -r, --reverse           Reverse sorting order (display from largest to smallest).
  -f, --files-only        Discard directories; consider only files and symlinks.
  -d, --directories-only  Discard files and symlinks; consider only directories.
  -B <N>, --bytes <N>     Filter out entries smaller than N Bytes.
  -K <X>, --kilobytes <X> Filter out entries smaller than X Kilobytes.
  -M <X>, --megabytes <X> Filter out entries smaller than X Megabytes.
  -G <X>, --gigabytes <X> Filter out entries smaller than X Gigabytes.
  -T <X>, --terabytes <X> Filter out entries smaller than X Terabytes.

Notes:
  - Specify only one of -B, -K, -M, -G, or -T options.
  - Cannot combine -f and -d options.

```

### Examples

- **List current directory:**

  ```bash
  $ duq
  ```

- **List a specific directory or file:**

  ```bash
  $ duq /path/to/directory
  ```

- **Display sizes with units:**

  ```bash
  $ duq -u
  ```

- **Filter out entries smaller than 500 Kilobytes:**

  ```bash
  $ duq -K 500
  ```

- **Filter out entries smaller than 1 Megabyte in unit mode:**

  ```bash
  $ duq -u -M 1
  ```

- **Invalid usage (specifying multiple filtering options):**

  ```bash
  $ duq -K 1 -M 1
  Error: Only one of -B, -K, -M, -G, or -T options can be specified.
  ```
  
- **Consider only directories, filter out files and symlinks:**

  ```bash
  $ duq -d
  ```

- **Consider only files and symlinks, filter out directories:**

  ```bash
  $ duq -f
  ```

## Installation

### Dependencies

- **CMake**: For configuring and generating the build files.
- **GCC (GNU Compiler Collection)**: For compiling the source code.
- **Make**: For building the project.
- **POSIX-compliant Environment**: Unix-like operating systems (Linux, macOS, etc.).

### Building from Source Using CMake

#### **1. Clone the Repository**

```bash
git clone https://github.com/heldercorreia/duq.git
```

#### **2. Navigate to the Project Directory**

```bash
cd duq
```

#### **3. Create a Build Directory and Run CMake**

```bash
mkdir src/build
cd src/build
cmake ..
```

- Optionally, specify a custom install prefix:

  ```bash
  cmake -DCMAKE_INSTALL_PREFIX=/custom/install/path ..
  ```

#### **4. Build the Program**

```bash
make
```

#### **5. Install the Program**

```bash
sudo make install
```

- Omit `sudo` if installing to a user-writable directory.


## Dependencies Installation

### On Ubuntu/Debian

Install CMake, GCC, and Make:

```bash
sudo apt update
sudo apt install build-essential cmake
```

### On Fedora

```bash
sudo dnf install gcc make cmake
```

### On macOS

Install [Homebrew](https://brew.sh/) if you haven't already, then:

```bash
brew install gcc cmake
```

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file
for details.

## Contributing

Contributions are welcome! Please submit a pull request or open an issue for any
changes or suggestions.

---

## Uninstallation

Only one file is installed in the entire file system.
If you need to uninstall `duq`, you can remove the installed executable:

```bash
sudo rm /usr/local/bin/duq
```

Or, if installed in a custom directory:

```bash
rm /custom/install/path/bin/duq
```

