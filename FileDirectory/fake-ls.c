#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define DEFAULT_BLOCK_SIZE 1024 // default ls block size 1k

char cwd_buf[1024]; // current working directory buffer
int initial_flag = 0;

struct {
    int max_nlink_num;
    int max_user_name;
    int max_group_name;
    int max_size_num;
    unsigned short max_nlink;
    unsigned long max_size;
} max_field; // max field length used for format print

// function list
void print_file_mode(mode_t mode);
void print_link_number(nlink_t nlink);
void print_user_group(uid_t uid, gid_t gid);
void print_file_size(off_t size);
void print_file_last_modification_time(time_t time);
void print_file_name(const char * file_name);

void print_directory_entry(const struct dirent * dir_entry, struct stat * p_stat_buf);

void preprocess(DIR * dir_ptr, struct stat * p_stat_buf);

void print_directory_recursive(const char * dir, int depth);

// field print functions group
void print_file_mode(mode_t mode) {
    // process with file type
    switch (mode & S_IFMT) {
        case S_IFDIR:   putchar('d');   break;
        case S_IFCHR:   putchar('c');   break;
        case S_IFBLK:   putchar('b');   break;
        case S_IFREG:   putchar('-');   break;
        case S_IFLNK:   putchar('l');   break;
        case S_IFSOCK:  putchar('s');   break;
        case S_IFIFO:   putchar('p');   break;
    }
    // process with access permission
    if (mode & S_IRUSR) putchar('r');   else putchar('-');
    if (mode & S_IWUSR) putchar('w');   else putchar('-');
    if (mode & S_IXUSR) putchar('x');   else putchar('-');
    if (mode & S_IRGRP) putchar('r');   else putchar('-');
    if (mode & S_IWGRP) putchar('w');   else putchar('-');
    if (mode & S_IXGRP) putchar('x');   else putchar('-');
    if (mode & S_IROTH) putchar('r');   else putchar('-');
    if (mode & S_IWOTH) putchar('w');   else putchar('-');
    if (mode & S_IXOTH) putchar('x');   else putchar('-');
    // used for alignment
    putchar (' ');
}

void print_link_number(nlink_t nlink) {
    // print nlink number
    printf("%*lu ", max_field.max_nlink_num, nlink);
}

void print_user_group(uid_t uid, gid_t gid) {
    // print user name
    struct passwd * user = NULL;
    if ((user = getpwuid(uid)) == NULL) {
        printf("getpwuid failed: %s.\n", strerror(errno));
        exit(-1);
    }
    printf("%*s ", max_field.max_user_name, user->pw_name);

    // print group name
    struct group * group = NULL;
    if ((group = getgrgid(gid)) == NULL) {
        printf("getgrgid failed: %s.\n", strerror(errno));
        exit(-1);
    }
    printf("%*s ", max_field.max_group_name, group->gr_name);
}

void print_file_size(off_t size) {
    // print file size
    printf("%*ld ", max_field.max_size_num, size);
}

void print_file_last_modification_time(time_t time) {
    // generate format date string
    struct tm * tm = localtime(&time);
    char tm_buffer[20];
    strftime(tm_buffer, 20, "%b %e %H:%M", tm);
    // and print file's last modification time
    printf("%s ", tm_buffer);
}

void print_file_name(const char * file_name) {
    // print file name
    printf("%s", file_name);
}

void print_directory_entry(const struct dirent * dir_entry, struct stat * p_stat_buf) {
    // use lstat to extract stat from directory entry
    if (lstat(dir_entry->d_name, p_stat_buf) == -1) {
        printf("lstat failed: %s.\n", strerror(errno));
        exit(-1);
    }

    // print each field accordingly
    print_file_mode(p_stat_buf->st_mode);
    print_link_number(p_stat_buf->st_nlink);
    print_user_group(p_stat_buf->st_uid, p_stat_buf->st_gid);
    print_file_size(p_stat_buf->st_size);
    print_file_last_modification_time(p_stat_buf->st_mtime);
    print_file_name(dir_entry->d_name);

    // put \n to end this entry
    putchar('\n');
}

// preprocess loop, count total and record max field information
// as for counting total
// reference from stackoverflow:
// https://stackoverflow.com/questions/7401704/what-is-that-total-in-the-very-first-line-after-ls-l
void preprocess(DIR * dir_ptr, struct stat * p_stat_buf) {
    struct dirent * dir_entry = NULL;
    blksize_t total_size = 0;
    // clear anonymous struct max_field
    memset(&max_field, 0, sizeof(max_field));
    // traverse each directory entry
    while ((dir_entry = readdir(dir_ptr)) != NULL) {
        if (lstat(dir_entry->d_name, p_stat_buf) == -1) {
            printf("lstat failed: %s.\n", strerror(errno));
            exit(-1);
        }

        // skip . / .. / and hidden files(.*)
        if ((dir_entry->d_name)[0] == '.') continue;

        struct passwd * user;
        struct group * group;
        int user_length, group_length;
        // get user name and its length
        if ((user = getpwuid(p_stat_buf->st_uid)) == NULL) {
            printf("getpwuid failed: %s.\n", strerror(errno));
            exit(-1);
        }
        user_length = strlen(user->pw_name);
        // get group name and its length
        if ((group = getgrgid(p_stat_buf->st_gid)) == NULL) {
            printf("getgrgid failed: %s.\n", strerror(errno));
            exit(-1);
        }
        group_length = strlen(group->gr_name);

        // count for total size - reference from wikipedia
        // st_blksize – preferred block size for file system I/O,
        // which can depend upon both the system and the type of file system
        // st_blocks – number of blocks allocated in multiples of DEV_BSIZE (usually 512 bytes)
        // DEV_BSIZE is defined as S_BLKSIZE

        total_size += p_stat_buf->st_blocks * S_BLKSIZE;

        // update max_field
        max_field.max_nlink = (p_stat_buf->st_nlink > max_field.max_nlink) ? p_stat_buf->st_nlink : max_field.max_nlink;
        max_field.max_size = (p_stat_buf->st_size > max_field.max_size) ? p_stat_buf->st_size : max_field.max_size;
        max_field.max_user_name = (user_length > max_field.max_user_name) ? user_length : max_field.max_user_name;
        max_field.max_group_name = (group_length > max_field.max_group_name) ? group_length : max_field.max_group_name;
    }
    // convert max_nlink and max_size from actual number to bit length using log10
    max_field.max_nlink_num = (max_field.max_nlink == 0 ? 1 : (int)(log10(max_field.max_nlink) + 1));
    max_field.max_size_num = (max_field.max_size == 0 ? 1 : (int)(log10(max_field.max_size) + 1));
    // print total
    printf("total %lu\n", total_size / DEFAULT_BLOCK_SIZE);
    // rewind directory
    rewinddir(dir_ptr);
}

void print_directory_recursive(const char * dir, int depth) {
    // directory pointer and directory entry pointer
    DIR * dir_ptr;
    struct dirent * dir_entry;

    // stat struct
    struct stat stat_buf;

    // open directory and change current working directory to dir
    if ((dir_ptr = opendir(dir)) == NULL) {
        printf("opendir failed: %s.\n", strerror(errno));
        return;
    }
    if (chdir(dir) == -1) {
        printf("chdir failed: %s.\n", strerror(errno));
        exit(-1);
    }

    // use initial_flag to control corrent \n print
    if (initial_flag) putchar('\n'); else initial_flag = 1;

    // print (current working) directory's name using get getcwd
    if (getcwd(cwd_buf, sizeof(cwd_buf)) == NULL) {
        printf("getcwd failed: %s.\n", strerror(errno));
        exit(-1);
    }
    printf("%s:\n", cwd_buf);

    // preprocess loop
    preprocess(dir_ptr, &stat_buf);

    // second loop, used to print each directory entry in current directory
    while ((dir_entry = readdir(dir_ptr)) != NULL) {
        // skip . / .. / hidden files(.*)
        if ((dir_entry->d_name)[0] == '.') continue;
        // print directory entry
        print_directory_entry(dir_entry, &stat_buf);
    }
    // rewind directory
    rewinddir(dir_ptr);

    // third loop, used to call print_directory_recursive on sub directories
    while((dir_entry = readdir(dir_ptr)) != NULL) {
        if (dir_entry->d_type == DT_DIR) {
            // skip . / ..
            if ((dir_entry->d_name)[0] == '.') continue;

            // recursive call
            print_directory_recursive(dir_entry->d_name, depth + 1);

            // after traversal, go back to parent directory
            // chdir(dir) is WRONG cuz what dir points to changes from time to time
            // readdir returns a pointer points statically allocated memory
            // after recursions, the string that pointer points to changes
            if (chdir("..") == -1) {
                printf("chdir failed: %s.\n", strerror(errno));
                exit(-1);
            }
        }
    }

    // close opened directory
    if (closedir(dir_ptr) == -1) {
        printf("closedir failed: %s.\n", strerror(errno));
        exit(-1);
    }
}

// program entry
int main(int argc, char * argv[]) {
    if (argc > 2) {
        printf("%s: too many arguments.\n", argv[0]);
        exit(-1);
    }

    // set dir to argv[1] or . if no argument provided
    char * dir = ".";
    if (argc == 2) dir = argv[1];

    // the start of recursion
    print_directory_recursive(dir, 0);

    exit(0);
}
