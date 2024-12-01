#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>  // For off_t
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>     // For POSIX functions
#include <limits.h>     // For PATH_MAX and NAME_MAX
#include <getopt.h>

#ifndef DUQ_VERSION
#define DUQ_VERSION "unknown"
#endif

#define INITIAL_CAPACITY 1024
#define MAX_LINE_SIZE (PATH_MAX + NAME_MAX + 64)

typedef struct {
    off_t size;
    char *size_str;
    char *entry_display;
} Entry;

typedef struct {
    Entry *entries;
    size_t count;
    size_t capacity;
} EntryList;

void print_help(const char *prog_name);
void print_version(void);
void format_size(off_t size, char *buf, size_t buf_size);
off_t get_directory_size(const char *path);
void add_entry(off_t size, const char *size_str, const char *entry_display);
void process_entry(const char *path);
void process_directory(const char *path);
int compare_entries(const void *a, const void *b);
void free_entries(void);
void format_double(char *buf, size_t buf_size, double value);

EntryList entry_list = {NULL, 0, 0};
off_t total_size = 0;
int unit_mode = 0;
size_t max_size_length = 0;
off_t min_size_threshold = 0;
int filter_option_used = 0; // To enforce only one filtering option
int files_only = 0;
int directories_only = 0;
int reverse_sort = 0; // Variable to track reverse sorting

void print_help(const char *prog_name) {
    printf("Usage: %s [OPTION] [TARGET]\n", prog_name);
    printf("Disk usage analyzer with sorted file and directory sizes.\n\n");
    printf("  TARGET     Directory or file to list. Defaults to current directory.\n\n");
    printf("Options:\n");
    printf("  -h, --help              Display this help message and exit.\n");
    printf("  -v, --version           Display version information and exit.\n");
    printf("  -u, --units             Display sizes with units (B, K, M, G, T) with up to 3 decimal places.\n");
    printf("  -r, --reverse           Reverse sorting order (display from largest to smallest).\n");
    printf("  -f, --files-only        Discard directories; consider only files and symlinks.\n");
    printf("  -d, --directories-only  Discard files and symlinks; consider only directories.\n");
    printf("  -B <N>, --bytes <N>     Filter out entries smaller than N Bytes.\n");
    printf("  -K <X>, --kilobytes <X> Filter out entries smaller than X Kilobytes.\n");
    printf("  -M <X>, --megabytes <X> Filter out entries smaller than X Megabytes.\n");
    printf("  -G <X>, --gigabytes <X> Filter out entries smaller than X Gigabytes.\n");
    printf("  -T <X>, --terabytes <X> Filter out entries smaller than X Terabytes.\n");
    printf("\nNotes:\n");
    printf("  - Specify only one of -B, -K, -M, -G, or -T options.\n");
    printf("  - Cannot combine -f and -d options.\n");
}

void print_version(void) {
    printf("%s\n", DUQ_VERSION);
}

void format_double(char *buf, size_t buf_size, double value) {
    char tmp[64];
    snprintf(tmp, sizeof(tmp), "%.3f", value);
    // Remove trailing zeros
    char *p = tmp + strlen(tmp) - 1;
    while (p > tmp && *p == '0') {
        *p-- = '\0';
    }
    if (p > tmp && *p == '.') {
        *p = '\0';
    }
    strncpy(buf, tmp, buf_size);
    buf[buf_size - 1] = '\0'; // Ensure null termination
}

void format_size(off_t size, char *buf, size_t buf_size) {
    if (unit_mode) {
        // Unit mode: Convert size to appropriate unit, include unit letter without space
        const char *units[] = {"B", "K", "M", "G", "T", "P", "E", "Z", "Y"};
        int unit_index = 0;
        double s = (double)size;
        while (s >= 1024.0 && unit_index < (int)(sizeof(units)/sizeof(units[0])) - 1) {
            s /= 1024.0;
            unit_index++;
        }
        // Format the number without trailing zeros
        char number_buf[64];
        format_double(number_buf, sizeof(number_buf), s);
        // Combine number and unit without space
        snprintf(buf, buf_size, "%s%s", number_buf, units[unit_index]);
    } else {
        // Default mode: Print size in bytes without units
        snprintf(buf, buf_size, "%lld", (long long)size);
    }
}

off_t get_directory_size(const char *path) {
    DIR *dir;
    struct dirent *entry;
    struct stat sb;
    char full_path[PATH_MAX];
    off_t dir_size = 0;

    if (!(dir = opendir(path))) {
        return 0;
    }

    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);

        if (lstat(full_path, &sb) == -1) {
            continue;
        }

        if (S_ISDIR(sb.st_mode) && !S_ISLNK(sb.st_mode)) {
            // It's a directory, recurse into it
            dir_size += get_directory_size(full_path);
        } else if (S_ISLNK(sb.st_mode)) {
            // It's a symlink, add the size of the symlink itself
            dir_size += sb.st_size;
        } else {
            // Regular file, add its size
            dir_size += sb.st_size;
        }
    }

    closedir(dir);
    return dir_size;
}

void add_entry(off_t size, const char *size_str, const char *entry_display) {
    if (entry_list.count >= entry_list.capacity) {
        size_t new_capacity = (entry_list.capacity == 0) ? INITIAL_CAPACITY : entry_list.capacity * 2;
        Entry *new_entries = realloc(entry_list.entries, new_capacity * sizeof(Entry));
        if (!new_entries) {
            fprintf(stderr, "Error: Memory allocation failed.\n");
            exit(EXIT_FAILURE);
        }
        entry_list.entries = new_entries;
        entry_list.capacity = new_capacity;
    }
    entry_list.entries[entry_list.count].size = size;
    entry_list.entries[entry_list.count].size_str = strdup(size_str);
    entry_list.entries[entry_list.count].entry_display = strdup(entry_display);
    if (!entry_list.entries[entry_list.count].size_str || !entry_list.entries[entry_list.count].entry_display) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    entry_list.count++;
}

void process_entry(const char *path) {
    struct stat sb;
    char size_buf[64];
    char entry_display[MAX_LINE_SIZE];
    off_t size = 0;

    if (lstat(path, &sb) == -1) {
        return;
    }

    const char *basename = strrchr(path, '/');
    basename = (basename) ? basename + 1 : path;

    int is_symlink = S_ISLNK(sb.st_mode);
    int is_directory = S_ISDIR(sb.st_mode) && !is_symlink;
    int is_regular_file = S_ISREG(sb.st_mode);

    // Apply type filtering
    if (files_only && is_directory) {
        return;
    }
    if (directories_only && !is_directory) {
        return;
    }

    if (is_symlink) {
        // Entry is a symlink
        char symlink_target[PATH_MAX];
        ssize_t len = readlink(path, symlink_target, sizeof(symlink_target) - 1);
        if (len != -1) {
            symlink_target[len] = '\0';
        } else {
            symlink_target[0] = '\0';
        }
        size = sb.st_size;  // Size of the symlink itself
        snprintf(entry_display, sizeof(entry_display), "%s -> %s", basename, symlink_target);
    } else if (is_directory) {
        // Entry is a directory
        size = get_directory_size(path);
        snprintf(entry_display, sizeof(entry_display), "%s/", basename);
    } else if (is_regular_file) {
        // Entry is a regular file
        size = sb.st_size;
        snprintf(entry_display, sizeof(entry_display), "%s", basename);
    } else {
        // Other types (e.g., sockets, devices), skip
        return;
    }

    // Apply the minimum size threshold
    if (size < min_size_threshold) {
        return;
    }

    // Update total size
    total_size += size;

    format_size(size, size_buf, sizeof(size_buf));

    // Update max_size_length
    size_t size_length = strlen(size_buf);
    if (size_length > max_size_length) {
        max_size_length = size_length;
    }

    // Add the entry
    add_entry(size, size_buf, entry_display);
}

void process_directory(const char *path) {
    DIR *dir;
    struct dirent *entry;
    char full_path[PATH_MAX];

    if (!(dir = opendir(path))) {
        fprintf(stderr, "Error: cannot open directory '%s': %s\n", path, strerror(errno));
        exit(EXIT_FAILURE);
    }

    int has_entries = 0;
    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        has_entries = 1;

        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        process_entry(full_path);
    }

    if (!has_entries) {
        exit(EXIT_SUCCESS);
    }

    closedir(dir);
}

int compare_entries(const void *a, const void *b) {
    const Entry *ea = (const Entry *)a;
    const Entry *eb = (const Entry *)b;

    if (ea->size < eb->size) {
        return reverse_sort ? 1 : -1;
    } else if (ea->size > eb->size) {
        return reverse_sort ? -1 : 1;
    } else {
        return 0;
    }
}

void free_entries(void) {
    for (size_t i = 0; i < entry_list.count; i++) {
        free(entry_list.entries[i].size_str);
        free(entry_list.entries[i].entry_display);
    }
    free(entry_list.entries);
}

int main(int argc, char *argv[]) {
    const char *target = ".";
    int opt;
    int option_index = 0;
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {"units", no_argument, 0, 'u'},
        {"reverse", no_argument, 0, 'r'},
        {"files-only", no_argument, 0, 'f'},
        {"directories-only", no_argument, 0, 'd'},
        {"bytes", required_argument, 0, 'B'},
        {"kilobytes", required_argument, 0, 'K'},
        {"megabytes", required_argument, 0, 'M'},
        {"gigabytes", required_argument, 0, 'G'},
        {"terabytes", required_argument, 0, 'T'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "hvurfdB:K:M:G:T:", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
                break;
            case 'v':
                print_version();
                exit(EXIT_SUCCESS);
                break;
            case 'u':
                unit_mode = 1;
                break;
            case 'r':
                reverse_sort = 1;
                break;
            case 'f':
                if (directories_only) {
                    fprintf(stderr, "Error: Cannot combine -f and -d options.\n");
                    exit(EXIT_FAILURE);
                }
                files_only = 1;
                break;
            case 'd':
                if (files_only) {
                    fprintf(stderr, "Error: Cannot combine -f and -d options.\n");
                    exit(EXIT_FAILURE);
                }
                directories_only = 1;
                break;
            case 'B':
            case 'K':
            case 'M':
            case 'G':
            case 'T': {
                if (filter_option_used) {
                    fprintf(stderr, "Error: Only one of -B, -K, -M, -G, or -T options can be specified.\n");
                    exit(EXIT_FAILURE);
                }
                filter_option_used = 1;
                char *endptr;
                errno = 0;
                double value;
                if (opt == 'B') {
                    long long val = strtoll(optarg, &endptr, 10);
                    if (errno != 0 || *endptr != '\0' || val < 0) {
                        fprintf(stderr, "Error: Invalid value for -B option: '%s'\n", optarg);
                        exit(EXIT_FAILURE);
                    }
                    min_size_threshold = (off_t)val;
                } else {
                    value = strtod(optarg, &endptr);
                    if (errno != 0 || *endptr != '\0' || value < 0) {
                        fprintf(stderr, "Error: Invalid value for -%c option: '%s'\n", opt, optarg);
                        exit(EXIT_FAILURE);
                    }
                    switch (opt) {
                        case 'K':
                            min_size_threshold = (off_t)(value * 1024.0);
                            break;
                        case 'M':
                            min_size_threshold = (off_t)(value * 1024.0 * 1024.0);
                            break;
                        case 'G':
                            min_size_threshold = (off_t)(value * 1024.0 * 1024.0 * 1024.0);
                            break;
                        case 'T':
                            min_size_threshold = (off_t)(value * 1024.0 * 1024.0 * 1024.0 * 1024.0);
                            break;
                    }
                }
                break;
            }
            default:
                print_help(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (optind < argc) {
        target = argv[optind];
    }

    struct stat sb;
    if (lstat(target, &sb) == -1) {
        fprintf(stderr, "Error: '%s' does not exist.\n", target);
        exit(EXIT_FAILURE);
    }

    if (S_ISDIR(sb.st_mode) && !S_ISLNK(sb.st_mode)) {
        // Target is a directory (not a symlink)
        process_directory(target);
    } else {
        // Target is a file or symlink
        process_entry(target);
    }

    // Format total size
    char total_size_str[64];
    format_size(total_size, total_size_str, sizeof(total_size_str));

    // Update max_size_length if necessary
    size_t total_size_length = strlen(total_size_str);
    if (total_size_length > max_size_length) {
        max_size_length = total_size_length;
    }

    // Sort entries by size
    qsort(entry_list.entries, entry_list.count, sizeof(Entry), compare_entries);

    // Check if any entries are left after filtering
    if (entry_list.count == 0) {
        free_entries();
        exit(EXIT_SUCCESS);
    }

    // Print entries
    for (size_t i = 0; i < entry_list.count; i++) {
        printf("%*s %s\n", (int)max_size_length, entry_list.entries[i].size_str, entry_list.entries[i].entry_display);
    }

    // Print total size
    printf("%*s total\n", (int)max_size_length, total_size_str);

    free_entries();

    return 0;
}
