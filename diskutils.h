#ifndef DISKUTILS_H
#define DISKUTILS_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdio>

using namespace std;

#pragma pack(push, 1)
struct metadata {
    char type;         
    char name[23];     
    int size;           
    int firstblock;     
};
#pragma pack(pop)

extern char *D;
extern int NBLOCKS;
extern int NFREEBLOCKS;
extern int RBN;

void joindisk();
void leavedisk();
int  getfreeblock();
void freeblock(int blk);
int  get_fat(int blk);
void set_fat(int blk, int next);
int  get_bitmap(int blk);
void set_bitmap(int blk, int val);

vector<string> split_path(const string &path);
int  find_in_dir(int dir_blk, const string &name, metadata &out_meta);
int  resolve_path(const string &path, int current_dir_block, string &filename, metadata &out_meta);
void add_to_dir(int dir_blk, const metadata &meta);
void remove_from_dir(int dir_blk, const string &name);

void md_cmd(string path, int cbn);
void ls_cmd(string path, int cbn, bool detailed);
void cp_cmd(string src, string dest, int cbn);
void prn_cmd(string path, int cbn);
int  cd_cmd(string path, int cbn, string &cwd_str);

#endif