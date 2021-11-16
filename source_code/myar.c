// THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING 
// A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - RENHAO XUE


#include <ar.h>
#include <getopt.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <utime.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>


struct meta {
    char name[16]; //room for null
    int mode;
    int size;
    time_t mtime; // a time_t is a long
};


struct file_name{
    char *file;
    char *arch;
};


int fill_ar_hdr(char *filename, struct ar_hdr *hdr){
    struct stat file_stats;
    // get stat of the file need to append
    if (stat(filename, &file_stats) == -1){
        perror("stat system call");
        exit(1);
    }
    // copy file name, get rid of null and insert /
    strcpy(hdr->ar_name, filename);
    int flag = 0;
    for (int i = 0; i < 16; i++){
        if (hdr->ar_name[i] == '\0' && flag == 0){
            hdr->ar_name[i] = '/';
            flag = 1;
        }
        else if (flag == 1){
            hdr->ar_name[i] = ' ';
        }
    }

    // convert file_stats to hdr
    // convert mtime
    sprintf(hdr->ar_date, "%-11ld", file_stats.st_mtime);
    hdr->ar_date[11] = ' ';
    // convert uid and gid
    char uid[7];
    sprintf(hdr->ar_uid, "%-5d", file_stats.st_uid);
    sprintf(uid, "%-6d", file_stats.st_uid);
    if (uid[5] == ' '){
        hdr->ar_uid[5] = ' ';
    }else{
        hdr->ar_uid[5] = uid[5];
    }

    char gid[7];
    sprintf(hdr->ar_gid, "%-5d", file_stats.st_gid);
    sprintf(gid, "%-6d", file_stats.st_gid);
    if (gid[5] == ' '){
        hdr->ar_gid[5] = ' ';
    }else{
        hdr->ar_gid[5] = gid[5];
    }

    // convert file siae
    char file_size[11];
    sprintf(hdr->ar_size, "%-9lld", file_stats.st_size);
    sprintf(file_size, "%-10lld", file_stats.st_size);
    if (file_size[9] == ' '){
        hdr->ar_size[9] = ' ';
    }else{
        hdr->ar_size[9] = file_size[9];
    }
    // convert file mode
    sprintf(hdr->ar_mode, "%-7o", file_stats.st_mode);
    hdr->ar_mode[7] = ' ';
    // putting magic string
    hdr->ar_fmag[0] = '`'; 
    hdr->ar_fmag[1] = '\n'; 
    return 0;
}


int fill_meta(struct ar_hdr hdr, struct meta *meta){
    strcpy(meta->name, hdr.ar_name);
    meta->mode = atoi(hdr.ar_mode) - 100000;
    meta->size = atoi(hdr.ar_size);
    meta->mtime = atoi(hdr.ar_date);
    return 0;
}


void quick_add(struct file_name files, int blksize){
    int output; // file descriptor
    int input;
    int already_there = 0;  // flag
    // set buffer
    char* buffer = (char*)malloc(blksize);

    // open archive file
    // Using O_CREAT with O_EXCL can detect if a file already exist
    // compare it with 0 to see if it succeeded
    if ((output = open(files.arch, O_WRONLY|O_CREAT|O_EXCL, 0644)) < 0){
        // If not successful, see if the file already exist
        if (errno == EEXIST){
            if ((output = open(files.arch, O_WRONLY|O_CREAT)) == -1){
                perror("open arch2");
                exit(1);
            }
            already_there = 1;
        }
        else{
            perror("open arch1");
            exit(1);
        }
    } 
    // see if the file is already there
    if (already_there == 0){
        // write magic string
        int n;
        if ((n = write(output, ARMAG, SARMAG)) != SARMAG){
            perror("Arch Magic String");
            exit(1);
        }
    }
    // create a header structure
    struct ar_hdr hdr;
    // fill ar_hdr
    fill_ar_hdr(files.file, &hdr);
    // setting offset
    lseek(output, 0, SEEK_END);
    // append hdr
    int n;
    int k;
    if ((n = write(output, &hdr, sizeof(hdr))) < 0){
        perror("error write header");
        exit(1);
    }
    lseek(output, 0, SEEK_END);

    // append content
    input = open(files.file, O_RDONLY);
    while ((n = read(input, buffer, blksize)) > 0){
        k = write(output, buffer, n);
        if (n == -1){
            perror("fail to read archive");
            exit(1);
        }
        if (k == -1){
            perror("fail to write to archive");
            exit(1);
        }
    }
    // see if archive needs a padding byte
    if (atoi(hdr.ar_size) % 2 != 0){
        write(output, "\n", 1);
    }

    free(buffer);
    close(output);
    close(input);
}


void extract(struct file_name files, int blksize){
    struct meta file_meta;
    int arch;
    int output;
    // open file descriptor
    if ((arch = open(files.arch, O_RDONLY)) < 0){
        perror("error reading the archive file");
        exit(1);
    }
    // detect archive file format
    int n;
    int k;
    char* buf = (char*)malloc(8);
    n = read(arch, buf, 8);
    if (n < 8 || strcmp(buf, ARMAG) != 0){
        perror("archive file invalid");
        exit(1);
    }
    free(buf);
    char* buf1 = (char*)malloc(60);
    // check each file name and see if matches
    while((n = read(arch, buf1, 60)) == 60){
        // extract ar header
        struct ar_hdr hdr;
        // sscanf takes 60 bytes buf1 and format them into hdr fields
        sscanf(buf1, "%s%s%s%s%s%s%s", hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag);
        // not match, go on to the next file name
        if (strncmp(hdr.ar_name, files.file, strlen(files.file)) != 0){
            if (atoi(hdr.ar_size) % 2 == 0){
                lseek(arch, atoi(hdr.ar_size), SEEK_CUR);
            }
            else{
                lseek(arch, atoi(hdr.ar_size)+1, SEEK_CUR);
            }
            continue;
        }
        // matches
        else{
            // allocate buffer size 
            char* buffer = (char*)malloc(blksize);

            // fill meta information
            fill_meta(hdr, &file_meta);       
            // create extracted file, or truncate, give it proper permission as in ar_hdr
            if ((output = open(files.file, O_WRONLY|O_CREAT|O_TRUNC, strtoul(hdr.ar_mode, NULL, 8))) < 0){
                perror("error creating file");
                exit(1);
            }
            // write out the extracted file
            int total = 0;
            while ((n = read(arch, buffer, 1)) > 0 && total < atoi(hdr.ar_size)){
                k = write(output, buffer, n);
                total = total + n;
                if (n == -1){
                    perror("fail to read archive");
                    exit(1);
                }
                if (k == -1){
                    perror("fail to write to archive");
                    exit(1);
                }
            }
            // reset meta information (atime, mtime)
            struct utimbuf time_buf;
            time_buf.actime = file_meta.mtime;
            time_buf.modtime = file_meta.mtime;
            if ((utime(files.file, &time_buf)) < 0){
                perror("utime failed");
                exit(1);
            }
            free(buffer);
            break;
        }
    }
    close(output);
    close(arch);
    free(buf1);
}


void extract_with_no_meta(struct file_name files, int blksize){
    struct meta file_meta;
    int arch;
    int output;
    // open file descriptor
    if ((arch = open(files.arch, O_RDONLY)) < 0){
        perror("error reading the archive file");
        exit(1);
    }
    // detect archive file format
    int n;
    int k;
    char* buf = (char*)malloc(8);
    n = read(arch, buf, 8);
    if (n < 8 || strcmp(buf, ARMAG) != 0){
        perror("archive file invalid");
        exit(1);
    }
    free(buf);
    char* buf1 = (char*)malloc(60);
    // check each file name and see if matches
    while((n = read(arch, buf1, 60)) == 60){
        // extract ar header
        struct ar_hdr hdr;
        // sscanf takes 60 bytes buf1 and format them into hdr fields
        sscanf(buf1, "%s%s%s%s%s%s%s", hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag);
        // not match, go on to the next file name
        if (strncmp(hdr.ar_name, files.file, strlen(files.file)) != 0){
            if (atoi(hdr.ar_size) % 2 == 0){
                lseek(arch, atoi(hdr.ar_size), SEEK_CUR);
            }
            else{
                lseek(arch, atoi(hdr.ar_size)+1, SEEK_CUR);
            }
            continue;
        }
        // matches
        else{
            // allocate buffer size 
            char* buffer = (char*)malloc(blksize);

            // fill meta information
            fill_meta(hdr, &file_meta);       
            // create extracted file, or truncate, give it proper permission as in ar_hdr
            if ((output = open(files.file, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0){
                perror("error creating file");
                exit(1);
            }
            // write out the extracted file
            int total = 0;
            while ((n = read(arch, buffer, 1)) > 0 && total < atoi(hdr.ar_size)){
                k = write(output, buffer, n);
                total = total + n;
                if (n == -1){
                    perror("fail to read archive");
                    exit(1);
                }
                if (k == -1){
                    perror("fail to write to archive");
                    exit(1);
                }
            }
            free(buffer);
            break;
        }
    }
    close(output);
    close(arch);
    free(buf1);
}


void list_file_names(char* arch){
    int arch1;
    int n;
    int k;
    // open archive file
    if ((arch1 = open(arch, O_RDONLY)) < 0){
        perror("error reading the archive file");
        exit(1);
    }
    // check if the file is a valid archive file
    char* buf = (char*)malloc(8);
    n = read(arch1, buf, 8);
    if (n < 8 || strcmp(buf, ARMAG) != 0){
        perror("archive file invalid");
        exit(1);
    }
    free(buf);
    // look at hdr, store the name
    char* buf1 = (char*)malloc(60);
    while((n = read(arch1, buf1, 60)) == 60){
        // extract ar header
        struct ar_hdr hdr;
        // sscanf takes 60 bytes buf1 and format them into hdr fields
        sscanf(buf1, "%s%s%s%s%s%s%s", hdr.ar_name, hdr.ar_date, hdr.ar_uid, hdr.ar_gid, hdr.ar_mode, hdr.ar_size, hdr.ar_fmag);
        // print the name char by char, ignore the /
        for (int i = 0; i < strlen(hdr.ar_name); i++){
            if (strcmp(&hdr.ar_name[i], "/") == 0){
                printf("    ");
                break;
            }
            else{
                printf("%c", hdr.ar_name[i]);
            }
        }
        // see if content is padded with one byte
        // lseek to the next ar_hdr
        if (atoi(hdr.ar_size) % 2 == 0){
            lseek(arch1, atoi(hdr.ar_size), SEEK_CUR);
        }
        else{
            lseek(arch1, atoi(hdr.ar_size)+1, SEEK_CUR);
        }
    }
    close(arch1);
    free(buf1);
}


int main( int argc, char *argv[] ){
    int opt;
    struct file_name option;
    int xoflag = -1;
    int blksize;
    int days;
    time_t now;
    time_t seconds;
    DIR *d;
    struct dirent *dir;

    while((opt=getopt(argc, argv, "qxotA:")) != -1){
        switch(opt){
            case 'q':
                option.arch = argv[2];
                option.file = argv[3];
                struct stat metainfo;
                stat(argv[3], &metainfo);
                blksize = metainfo.st_blksize;
                quick_add(option, blksize);
                break;
            case 'x':
                xoflag = 0;
                break;
            case 'o':
                if (xoflag == 0){
                    xoflag = xoflag + 1;
                }
                break;
            case 't':
                list_file_names(argv[2]);
                break;
            case 'A':
                days = atoi(argv[2]);
                // convert days into seconds
                seconds = days * 24 * 60 * 60;
                // open current directory
                if (!(d=opendir("."))){
                    perror("opendir");
                    exit(1);
                }
                // loop through all file names
                while((dir = readdir(d)) != NULL){
                    // get the stat for each file
                    struct stat stats;
                    if ((stat(dir->d_name, &stats)) == -1){
                        perror("stat system call");
                        exit(1);
                    }
                    // see how long since last modified
                    now = time(NULL);
                    if (now - stats.st_mtime >= seconds){
                        // if more than specified days, do quick add
                        option.arch = argv[3];
                        option.file = dir->d_name;
                        quick_add(option, stats.st_blksize);
                    }
                }
                break;
            case '?':
                printf("usage: ./myar [qxotvdA:] archive-file [file]");
        }
    }
    if (xoflag == 0){
        int i = 3;
        while (i < argc){
            option.arch = argv[2];
            option.file = argv[i];
            extract_with_no_meta(option, blksize);
            i++;
        }
    }
    else if (xoflag == 1){
        int i = 3;
        while (i < argc){
            option.arch = argv[3];
            option.file = argv[i];
            extract(option, blksize);
            i++;
        }
    }
}


