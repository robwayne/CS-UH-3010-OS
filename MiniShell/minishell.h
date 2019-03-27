#ifndef MINI_SHELL_H
#define MINI_SHELL_H

#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <unistd.h>
#include <sstream>
#include <dirent.h>
#include <stdexcept>
#include <exception>

using namespace std;

#define rep_iter(it, begin, end) for(auto it=begin;it!=end;it++)
#define BUFF_SIZE 1000

void rtrim_str(string& s);
void ltrim_str(string& s);
string trim_str(const string& s);
bool is_number(const string& s);
pair<string, string> parse_line(const string& line);

//enum type used in execute_command, used to determine what command has been executed/requested to be executed
enum CommandType {
    EXT = -1, CD, PWD, HIST, EXP, REC_CMD, EXCL
};

class MiniShell {
public:
    MiniShell();
    ~MiniShell();
    void execute_command(const CommandType& cmd, const string& arguments = "");
    string execute_history_command(const string& cmd_num);
private:
    vector<string> _history;
    unordered_map<string, string> shell_vars;
    void exit();
    void read_data();
    void persist_data();
    void cd(const string& path);
    void pwd();
    void list_history();
    void export_variable(const string& var);
    void record_command(const string& cmd);
    void list_shell_vars();
    void add_shell_var(const string& var);
    void find_external_command(const string& args);
    pair<string, string> parse_shell_var(string var);
    string wd;
};

#endif