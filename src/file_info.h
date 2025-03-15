#pragma once


#include <stdint.h> 

#ifdef  __cplusplus
extern  "C" {
#endif 

int64_t fstat_filesize(const char *filename);


int64_t stat_filesize(const char *filename);

int dump_mem(void *pData, unsigned long size, const char * format, ...);

void *read_file_to_string(const char *filename, unsigned int *filesize);
void *vafread(unsigned int *filesize, const char * format, ...);

char *rand_filename(unsigned int num);

char *folder_name(char *filename);

char *get_path(char *filename);
char *get_file_name(const char *path_fname);

char *get_file_extension(char *filename);
int file_extension_compare(char *filename, char *ext);

char *get_filename_without_extension(char *filename, char *ext);

int write_string_to_file(void *stream, unsigned int stream_size, const char * format, ...);

char **dir_list_read(char *path);
char *find_file_name(char **dirlist, char *ext);
char *find_file_ext_name(char **dirlist, char *ext);


#ifdef  __cplusplus
}
#endif 


