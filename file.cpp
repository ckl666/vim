#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ROW 100   //小文件存放的最大行数
#define MAX_FILE 1024 
#define MAX_COL 256
#define READ_ONE 128
#define MAX_LONG 10

/*
 * 小文件的命名
 * 
 */
void sub_file_name(char *str)
{
    int len = strlen(str);
    int i = 0;
    while(i < len)
    {
        if(str[i] == '0')
        {
            str[i] = '9';
            i++;
        }
        else
        {
            str[i] -= 1;
            break;
        }
    }
    i = len-1;
    while(str[i] == '0')
    {
        str[i] = 0;
        i--;
    }
}

void add_file_name(char *str)
{
    int len = strlen(str);
    int i = 0;
    while(i < len)
    {
        if(str[i] == '9')
        {
            str[i] = '0';
            if(str[i+1] == 0)
            {
                str[i+1] = '1';
                break;
            }
            else
            {
                str[i+1] += 1;
                i++;
            }
        }
        else
        {
            str[i] += 1;
            break;
        }
    }
}

void FileSplit(char *path)
{
    FILE *fp =fopen(path,"r");
    if(fp == NULL)
    {
        return ;
    }
    
    char buff[MAX_FILE];
    char str[MAX_LONG] = {0};
    str[0] = '0';

    //构造文件名
    add_file_name(str);
    char file_name[MAX_LONG+10] = {0};

    strcpy(file_name,str);
    strcat(file_name,".txt");
    FILE *fp_min = fopen(file_name,"w");

    int line = 0;
    while(fgets(buff,MAX_FILE,fp) != NULL)
    {
        if(line >= 100)
        {
            fclose(fp_min);
            add_file_name(str);        
            strcpy(file_name,str);
            strcat(file_name,".txt");
            FILE *fp_min = fopen(file_name,"w");

            line = 0;
        }
        fwrite(buff,sizeof(char),strlen(buff),fp_min);
        line++;
    }
    if(line < 100)
    {
        fclose(fp_min);
    }
    fclose(fp);
}

void FileCombine(char *path)
{
    FILE *fp = fopen(path,"w");
    if(fp == NULL)
    {
        return ;
    }
    
    char buff[MAX_FILE];
    char str[MAX_LONG] = {0};
    str[0] = '0';

    //构造文件名
    add_file_name(str);
    char file_name[MAX_LONG+10] = {0};

    strcpy(file_name,str);
    strcat(file_name,".txt");
    FILE *fp_min = fopen(file_name,"r");

    while(fp_min != NULL)
    {
        int n = 0;
        while((n = fread(buff,sizeof(char),sizeof(buff),fp_min)) != 0)
        {
            fwrite(buff,sizeof(char),n,fp);
        }
        fclose(fp_min);

        add_file_name(str);
        strcpy(file_name,str);
        strcat(file_name,".txt");
        fp_min = fopen(file_name,"r");
    }
 
}

