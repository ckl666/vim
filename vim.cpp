#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <fcntl.h>
#include <string.h>
#include <deque>
#include <string>
#include <stack>
#include "file.c"

using namespace std;
/*
 *
 * 海量数据：文件的内容无法全部写入内存中，模仿操作系统执行程序的过程，
 *           先将一部分文件读入内存，在屏幕显示一部分，在缓冲区保存一部分，
 *           当需要读取缓冲区以外的内容时，再从文件中写入内存一部分，将暂时不读的先写入文件
 *           （撤销）
 */
#define MAX_BUFF 100 //缓冲区的最大行数
#define MAX_X  38    //最大行数
#define MAX_Y  170   //最大列数
#define KEY_ENTE 10  //回车
#define KEY_TAB 9    //tab
#define KEY_s 115
#define KEY_u 117
#define KEY_d 100
#define KEY_q 113
#define KEY_i 105
#define KEY_r 114
#define KEY_x 120
#define KEY_C 67
#define KEY_ESC 27    //esp

typedef struct Node
{
    off_t fr_seek;      //记录文件的前面读到的位置
    off_t re_seel;      //记录文件后面读到的位
    int x;
    int y;  //记录光标的位置
    deque<string> front;    //屏幕前的缓冲区
    deque<string> rear;     //屏幕后的缓冲区
    deque<string> cur;      //当前屏幕的内容
}Node;

//清除字符串后面的空格
void deal(char *str)
{
    int len = strlen(str);
    while(len > 0)
    {
        if(str[len-1] == ' ')
        {
            len--;
        }
        else
        {
            break;
        }
    }
    str[len] = '\n';
    str[len+1] = '\0';
}

//保存上缓冲区的内容
void save_up(const char *path,deque<string> &front)
{
    int fd = open(path,O_WRONLY|O_CREAT,0600);
    assert(fd != -1);

    char str[MAX_Y];
    while(!front.empty())
    {
        strcpy(str,front.front().c_str());
        deal(str);
        front.pop_front();
        write(fd,str,strlen(str));
    }
    close(fd);
}

//保存缓冲区下方的内容
void save_down(const char *path,deque<string> &rear)
{
    int fd = open(path,O_WRONLY|O_CREAT,0600);
    assert(fd != -1);

    char str[MAX_Y];
    while(!rear.empty())
    {
        strcpy(str,rear.back().c_str());
        deal(str);
        rear.pop_back();
        write(fd,str,strlen(str));
    }
    close(fd);
}

//读取缓冲区上方的内容
void read_up_down(const char *path,deque<string> &buff,int flag)
{
    FILE *fp = fopen(path,"r");
    if(fp == NULL)
    {
        return ;
    }
    char str[MAX_Y];
    if(flag == 1)   //表示上方的缓冲区
    {
        while(fgets(str,sizeof(str),fp) != NULL)
        {
            buff.push_back(str);
        }
    }
    else if(flag == 0)           //表示下方的缓冲区
    {
        while(fgets(str,sizeof(str),fp) != NULL)
        {
            buff.push_front(str);
        }
    }
}

//保存文件
void save(deque<string> &front,deque<string> &rear,deque<string> &name)
{
    //重新打开文件，清除文件原来的内容
    //int fd = open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);
    const char *path = name.front().c_str();
    name.pop_front();
    int fd = open(path,O_WRONLY|O_CREAT|O_TRUNC,0600);
    if(fd == -1)
    {
        return ;
    }
    int i = 0;
    char str[MAX_Y+1];
    int x,y;
    getyx(stdscr,x,y);
    //先将front缓冲区的内容写进文件
    while(!front.empty())
    {
        strcpy(str,front.front().c_str());
        deal(str);
        front.pop_front();
        write(fd,str,strlen(str));
        memset(str,0,sizeof(str));
    }
    //将屏幕上的写进rear缓冲区
    i = MAX_X;
    while(i >= 0)
    {
        move(i,0);
        if(instr(str)>0)
        {
            rear.push_back(str);
        }
        i--;
    }
    //若文件后面都是空白，自动消除，先将缓冲区的字符串处理，从back位置处插入front 缓冲区
    while(!rear.empty())
    {
        strcpy(str,rear.back().c_str());
        deal(str);
        rear.pop_back();
        front.push_back(str);
    }
    //此时文件的后面在front缓冲区的back位置处
    while(!front.empty() && (strcmp(front.back().c_str(),"\n") == 0))
    {
        front.pop_back();
    }
    //从front的front处开始读取文件的内容
    while(!front.empty())
    {
        strcpy(str,front.front().c_str());
        write(fd,str,strlen(str));
        front.pop_front();
    }
    move(x,y);
    close(fd);

    //清空其他打开的文件
    while(!name.empty())
    {
        path = name.front().c_str();
        name.pop_front();
        FILE *fp = fopen(path,"w");
        fclose(fp);
    }
}

void FileMoveUp(deque<string> &front,deque<string> &name)
{
    //取最上方的一行写进缓冲区front
    move(0,0);
    char str[MAX_Y+1];
    instr(str);
    front.push_back(str);
    if(front.size() >= MAX_BUFF && name.size() > 1)
    {
        save_up(name.front().c_str(),front);
        name.pop_front();
    }
    //删除最上方一行
    deleteln();
    //最下方一行的内容更新
}

void FileMoveDown(deque<string> &rear,deque<string> &name)
{
    int i = MAX_X-1;
    char str[MAX_Y+1];
    move(MAX_X,0);
    instr(str);
    rear.push_back(str);
    //缓冲区的内容多余某个阈值时,将缓冲区的内容写入文件中;
    if(rear.size() >= MAX_BUFF && name.size() > 1)
    {
        save_down(name.back().c_str(),rear);
        name.pop_back();
    }

    //将屏幕上的内容向下移动一行
    for(; i >= 0; i--)
    {
        move(i,0);
        instr(str);
        move(i+1,0);
        printw("%s",str);
        refresh();
    }
}

void str_tok(char *des,const char *src,char ch)
{
    int i = 0;
    while(src[i] != ch && src[i] != '\0')
    {
        des[i] = src[i];
        i++;
    }
    des[i] = '\0';
}

void direction_up(deque<string> &front,deque<string> &rear,deque<string> &name)
{
    //如果光标在最上方并且front缓冲区不为空
    int x,y;
    getyx(stdscr,x,y);
    if(x == 0)
    {
        if(front.empty())
        {
            char str[MAX_LONG] = {0};
            str_tok(str,name.front().c_str(),'.');
            sub_file_name(str);
            if(str[0] != 0)
            {
                //构造文件的名字
                strcat(str,".txt");    
                //从新文件读取内容
                read_up_down(str,front,1); 
                name.push_front(str);
            }
            else
            {
                return ;
            }
        }
        FileMoveDown(rear,name);
        //读缓冲区的内容写在最上方
        move(0,0);
        printw("%s",front.back().c_str());
        front.pop_back();
        move(x,y);
    }
    move(x-1,y);
}

void direction_down(deque<string> &front,deque<string> &rear,deque<string> &name)
{
    int x,y;
    getyx(stdscr,x,y);
    //如果当前光标在最下方并且rear缓冲区的内容不为空
    if(x == MAX_X)
    {
        if(rear.empty())
        {
            char str[MAX_LONG] = {0};
            str_tok(str,name.back().c_str(),'.');
            strcat(str,".txt");
            add_file_name(str);
            read_up_down(str,rear,0);
            if(rear.empty())
            {
                return ;
            }
            name.push_back(str);
        }
        //屏幕所有内容上移
        FileMoveUp(front,name);
        move(MAX_X,0);
        
        printw("%s",rear.back().c_str());
        rear.pop_back();
        refresh();
        //光标回到原位置
        move(x,y);
    }
    else
    {
        move(x+1,y);
    }
}
//方向键控制（调用的多，单独拿出来）
//name 存储打开文件的名字
void Direction(int key,deque<string> &front,deque<string> &rear,deque<string> &name)
{
    int x,y;
    getyx(stdscr,x,y);
    switch(key)
    {
        case KEY_UP: direction_up(front,rear,name);break;
        case KEY_DOWN: direction_down(front,rear,name);break;
        case KEY_LEFT: move(x,y-1); break;
        case KEY_RIGHT: move(x,y+1); break;
    }
}

//插入模式
int Insert(deque<string> &front,deque<string> &rear,deque<string> &name)
{
    int tag = 0;
    while(true)
    {
        int key,x,y;
        refresh();    
        key = getch();
        getyx(stdscr,x,y);
        if(key >= 32 && key <= 126)
        {
            tag = 1;
            insch(key);
            move(x,y+1);
        }
        else
        {
            switch(key)
            {
                //回车键
                case KEY_ENTE: 
                    { 
                        tag = 1;
                        char str[MAX_Y+1];
                        if(x == MAX_X)
                        {
                            move(0,0);
                            instr(str);
                            front.push_back(str);
                            deleteln();
                            move(MAX_X,0);
                        }
                        else
                        {
                            move(MAX_X,0);
                            instr(str);
                            rear.push_back(str);
                            move(x+1,0); 
                            //插入一个空白行
                            insertln();
                        }
                    } break;
                //esp键
                case KEY_ESC:return tag;
                //tab键
                case KEY_TAB: 
                    { 
                        int num = y % 4;
                        move(x,(y+4-num));
                    } 
                    break;
                //退格键
                case KEY_BACKSPACE:
                    {
                        if(y > 0)
                        {
                            move(x,y-1);
                            delch();
                        }
                        else
                        {
                            move(x-1,y);
                            deleteln();
                        }
                        tag = 1;
                    }
                    break;
                default:
                    Direction(key,front,rear,name);
                    break;
            }
        }
    }
}

//恢复当前状态
void RecoverStat(deque<string> &front,deque<string> &rear,stack<Node> &cancle)
{
    if(cancle.empty())
    {
        return ;
    }
    Node node;
    node = cancle.top();
    front = node.front;
    rear = node.rear;
    int i = 0;
    for(; i < MAX_X; i++)
    {
        move(i,0);
        printw("%s",node.cur.front().c_str());
        node.cur.pop_front();
        refresh();
    }
    move(node.x,node.y);
    cancle.pop();
}
//保存当前的状态
void SaveStat(deque<string> &front,deque<string> &rear,stack<Node> &cancle)
{
    Node node;
    node.front = front;
    node.rear = rear;
    int x,y;
    getyx(stdscr,x,y);
    node.x = x;
    node.y = y;
    int i = 0;
    char str[MAX_Y];
    for(; i <= MAX_X; i++)
    {
        move(i,0);
        instr(str);
        node.cur.push_back(str);
    }
    cancle.push(node);
    move(x,y);
}

//命令行模式
void Command(deque<string> &front,deque<string> &rear,stack<Node> &cancle,deque<string> &name)
{
    //tag == 0表示未修改文件，不用再次保存，否则重新保存
    int tag = 0;
    while(1)
    {
        int key,x,y;
        key = getch();
        //获取光标的坐标
        getyx(stdscr,x,y);
        switch(key)
        {
            //ctrl + c 清除光标至结尾
            case KEY_C:tag = 1; SaveStat(front,rear,cancle);clrtoeol(); Insert(front,rear,name) ;break;
            //x删除光标处字符 并进入插入模式
            case KEY_x:tag = 1; SaveStat(front,rear,cancle);delch(); break;
            //i插入模式
            case KEY_i:
                {
                    SaveStat(front,rear,cancle);
                    tag = Insert(front,rear,name); 
                    if(tag == 0)
                    {
                        cancle.pop(); 
                    }
                    break;
                }
            //退出
            case KEY_q: return ;
            //u 撤销
            case KEY_u: RecoverStat(front,rear,cancle);break;
            //r 替换
            case KEY_r:
                {
                    tag = 1;
                    SaveStat(front,rear,cancle);
                    while(1)
                    {
                        key = getch();
                        if(key >= 32 && key <= 126)
                        {
                            printw("%c",key);
                            refresh();
                            break;
                        }
                        else
                        {
                            Direction(key,front,rear,name);
                        }
                    }
                }
                break;
            //d 删除一行
            case KEY_d:
                {
                    tag = 1; 
                    SaveStat(front,rear,cancle);
                    deleteln();
                    if(!rear.empty())
                    {
                        move(MAX_X,0);
                        printw("%s",rear.back().c_str());
                        rear.pop_back();
                        move(x,y);
                    }
                    refresh();
                    break;
                }
            //s 保存
            case KEY_s:
                {
                    while(!cancle.empty())
                    {
                        cancle.pop();
                    }
                    if(tag == 1) save(front,rear,name);
                    break;
                }
            default:
                Direction(key,front,rear,name);
                break;
        }
    }
}

//编辑器初始化
void editor()
{
    initscr();
    crmode();
    keypad(stdscr,true);
    noecho();
    clear();
    move(0,0);
}


//打开文件并显示
void Open(char *path,deque<string> &front, deque<string> &rear,deque<string> &name)
{
    FILE *fp = fopen(path,"ra");
    if(fp == NULL)
    {
        return ;
    }

    char buff[MAX_Y] = {0};
    int row = 0;
    while((fgets(buff,sizeof(buff),fp) != NULL) && row < MAX_X)
    {
        printw("%s",buff);
        row++;
        refresh();    
    }
    
    while(fgets(buff,sizeof(buff),fp) != NULL)
    {
        rear.push_front(buff); 
    }
    
    name.push_back(path);

    fclose(fp);

}



int main(int argc,char *argv[])
{
    if(argc == 1 || argc > 2)
    {
        return 0;
    }
    editor();
    //
    deque<string> front;
    deque<string> rear;
    deque<string> name;
    stack<Node> cancle;
    
    //分割
    FileSplit(argv[1]);
    char str[MAX_LONG];
    memset(str,0,sizeof(str));
    str[0] = '0';
    add_file_name(str);
    strcat(str,".txt");
    
    Open(str,front,rear,name);
    move(0,0);
    Command(front,rear,cancle,name);
    
    //合并
    FileCombine(argv[1]);
    endwin();
    return 0;
}
