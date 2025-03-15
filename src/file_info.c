/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 
#include <stdarg.h>
#include <time.h> 

#include <ctype.h>
#include <string.h> 
#include <inttypes.h>

#include <unistd.h>
#include <fcntl.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "file_info.h"

#ifdef __MINGW64__
char folder_delimiters = '\\';
#else
char folder_delimiters = '/';
#endif

int64_t fstat_filesize(const char *filename)
{
	int64_t filesize = -1;
	
    int fd;
    struct stat statbuf;

    fd = open(filename, O_RDONLY, S_IRUSR | S_IRGRP);
    if (fd == -1)
    {
        printf("failed to open %s\n", filename);
        filesize = -1;
		goto error_out;
    }

    if (fstat(fd, &statbuf) == -1)
    {
        printf("failed to fstat %s\n", filename);
        filesize = -1;
		goto error_out;
    }

	filesize = statbuf.st_size;

    printf("[*] fstat_filesize - file: %s, size: %ld\n", filename, statbuf.st_size);

    if (close(fd) == -1)
    {
        printf("failed to fclose %s\n", filename);
        filesize = -1;
		goto error_out;
    }

error_out:
	
	return filesize;
	
}

int64_t stat_filesize(const char *filename)
{
    struct stat statbuf;

    if (stat(filename, &statbuf) == -1)
    {
        printf("failed to stat %s\n", filename);
        return -1;
    }

    printf("[*] stat_filesize - file: %s, size: %ld\n", filename, statbuf.st_size);

	return statbuf.st_size;
}

int dump_mem(void *pData, unsigned long size, const char * format, ...)
{	
	FILE * pOutFile;
	char filename[256] = {0};

  	va_list args;
	
  	va_start (args, format);
  	vsnprintf (filename, 256, format, args);
  	va_end (args);

	printf("%s() dump to file %s\n", __func__, filename);

	if((pOutFile = fopen (filename, "wb")) != NULL)
	{
		printf("outFileSize = %ld\n", size);
		fwrite(pData, 1, size, pOutFile);
		fclose(pOutFile);
		return 0;
	}
	else
	{
		printf("fopen %s fail !!!\n", filename);
	}
	
	return -1;
}

void *read_file_to_string(const char *filename, unsigned int *filesize)
{
	int64_t FileSize;
	void *FileBuf = NULL;

	if((FileSize = stat_filesize(filename)) > 0)
	{
		FILE * pFile;
		if((pFile = fopen (filename, "rb")) != NULL)
		{
			if((FileBuf=malloc(FileSize)) != NULL )
			{
				fread(FileBuf, 1, FileSize, pFile);
			}
			fclose(pFile);
		}

		if(filesize)  *filesize = FileSize;
	}
	else
	{
		printf("stat_filesize = %" PRId64 "\n", FileSize);
		if(filesize)  *filesize = 0;
	}

	//printf("%s(%s) read %ld bytes\n", __func__, filename, FileSize);
	return FileBuf;
}

void *vafread(unsigned int *filesize, const char * format, ...)
{
	char *fileName;
	void *fileBuf = NULL;

  	va_list args;
	
  	va_start (args, format);
  	vasprintf (&fileName, format, args);
  	va_end (args);

	fileBuf = read_file_to_string(fileName, filesize);

	free(fileName);
	
	return fileBuf;
}

char *rand_filename(unsigned int num)
{
	unsigned int i;
	char *filename = malloc(num + 1);

	/* initialize random seed: */
	srand (time(NULL));

	for(i=0;i<num;)
	{
		filename[i] = rand() % 256;

		if(isalnum(filename[i]))  i++;
	}

	filename[num] = '\0';

	return filename;

}
char *folder_name(char *filename)
{
	char *tmp = strdup(filename);

	char *pDelim;
	char *folderName = NULL;
	if((pDelim = strrchr(tmp, folder_delimiters)) != NULL)   // last delimiters
	{
		*pDelim = '\0';
	}
	else
	{
		return NULL;
	}
	
	if((pDelim = strrchr(tmp, folder_delimiters)) != NULL) 
	{
		pDelim = strrchr(tmp, folder_delimiters);
		folderName = strdup(pDelim + 1);
	}
	else
	{
		folderName = strdup(tmp);
	}
	

	free(tmp);

	return folderName;
}

char *get_path(char *filename)
{
	char *tmp = strdup(filename);
	char *pDelim;
	
	if((pDelim = strrchr(tmp, folder_delimiters)) != NULL)   // last delimiters
	{
		*pDelim = '\0';
	}
	
	return tmp;
}
char *get_file_name(const char *path_fname)
{
	char *tmp = strdup(path_fname);
	char *pDelim;
	
	if((pDelim = strrchr(tmp, folder_delimiters)) != NULL)   // last delimiters
	{
		printf("%s find folder delimiters %s\n", __func__, pDelim);
		return strdup(pDelim+1);
	}

	printf("%s find %s\n", __func__, tmp);
	return tmp;
}
char *get_file_extension(char *filename)
{
	char *tmp = strdup(filename);
	char *pDelim;
	
	char *folderName = NULL;
	
	if((pDelim = strrchr(tmp, '.')) != NULL)
	{
		folderName = strdup(pDelim + 1);
	}
	
	free(tmp);

	return folderName;
}

int file_extension_compare(char *filename, char *ext)
{
	size_t filename_len = strlen(filename);
	size_t ext_len      = strlen(ext);

	printf("%s( %s, %s )\n", __func__, filename, ext);

	return strncasecmp(filename + (filename_len - ext_len), ext, ext_len);
	
}
char *get_filename_without_extension(char *filename, char *ext)
{
	char *tmp = strdup(filename);
	char *pDelim;
	
	char *folderName = NULL;

	if(ext)
	{
		if((pDelim = strstr (tmp, ext)) != NULL)  *pDelim = '\0';
	}
	else
	{
		if((pDelim = strrchr(tmp, '.')) != NULL)  *pDelim = '\0';  // last delimiters
	}

	if((pDelim = strrchr(tmp, folder_delimiters)) != NULL) 
	{
		pDelim = strrchr(tmp, folder_delimiters);
		folderName = strdup(pDelim + 1);
	}
	else
	{
		folderName = strdup(tmp);
	}
	
	free(tmp);

	return folderName;
}

int write_string_to_file(void *stream, unsigned int stream_size, const char * format, ...)
{
	FILE * pOutFile;
	
    struct stat statbuf;

	char filename[256] = {0};
	char *cPath = NULL;
	char *cFolder = NULL;
	int retValue;
	
  	va_list args;
	
  	va_start (args, format);
  	vsnprintf (filename, 256, format, args);
  	va_end (args);

	printf("write to %s\n", filename);
  
	cPath = get_path(filename);
	cFolder = folder_name(filename);

	if(cFolder != NULL)
	{
		printf("with folder %s\n", cFolder);
		
		if( stat(cPath,&statbuf) < 0 )
		{
			printf("folder not exist !!!!\n");
			printf("mkdir %s\n", cPath);
#ifdef __MINGW64__
			mkdir(cPath);
#else
			mkdir(cPath, 0777);
#endif
		}
	}
	
	if((pOutFile = fopen (filename,"wb")) != NULL)
	{
		fwrite(stream, 1, stream_size , pOutFile);
		fclose(pOutFile);

		retValue = 0;
	}
	else
	{
		printf("fopen %s fail !!!!\n", filename);
		retValue = -1;
	}
	


return retValue;

	
}


char **dir_list_read(char *path)
{

	char **dirlist = calloc(sizeof(char *), 1024);
	unsigned int cnt = 0;
	char **ptr;
	
	DIR *dp;
  	struct dirent *ep;
	dp = opendir (path);
	
	if (dp != NULL)
	{
		while ((ep = readdir (dp)) != NULL)
	  	{
			dirlist[cnt] = strdup(ep->d_name);
			dirlist[++cnt] = NULL;
	  	}

	    closedir (dp);
	}
	else
	{
		perror ("Couldn't open the directory");
	}
	
	for (ptr = dirlist; *ptr; ++ptr)
	{
		printf("dirlist : %s\n", *ptr);
	}
	return dirlist;
}


char *find_file_name(char **dirlist, char *ext)
{
	char **ptr;
	
	//uint8_t opt_nul_terminate_output = 0;

	for (ptr = dirlist; *ptr; ++ptr)
	{
		if(strstr(*ptr, ext) != NULL)
			return strdup(*ptr);
	}
	
	printf("%s() ext %s not found in tar !!!!\n", __func__,  ext);
	
	return NULL;
}

char *find_file_ext_name(char **dirlist, char *ext)
{
	char **ptr;
	char *pDelim;

	for (ptr = dirlist; *ptr; ++ptr)
	{
		if((pDelim = strrchr(*ptr, '.')) != NULL)
		{
			if(strncasecmp(pDelim+1, ext, strlen(ext)) == 0)
				return strdup(*ptr);
		}
	}
	
	printf("%s() ext %s not found in tar !!!!\n", __func__, ext);

	for (ptr = dirlist; *ptr; ++ptr)
	{
		printf("dirlist : %s\n", *ptr);
	}
	
	return NULL;
}

