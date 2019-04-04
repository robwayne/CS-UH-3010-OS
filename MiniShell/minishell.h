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
#include <cstdlib>
#include <sstream>
#include <cstdio>
#include <stdexcept>
#include <exception>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

using namespace std;

#define rep_iter(it, begin, end) for(auto it=begin;it!=end;it++)
#define BUFF_SIZE 1000

extern char **environ;

void rtrim_str(string& s);
void ltrim_str(string& s);
string trim_str(const string& s);
void trim_str_ip(string &s);
bool is_number(const string& s);
string get_io_file(const string& str, char sep);

class MiniShell {
public:
    MiniShell();
    ~MiniShell();
    void execute();

private:
    vector<string> _history;
    unordered_map<string, string> shell_vars;

    string execute_history_command(string& cmd);
    pair<string, string> get_cmd_with_args(const string &str);
    vector<string> split_by_sep(const string &str, char sep);
    pair<string, string> parse_shell_var(string var);
    bool is_internal_cmd(string cmd);
    string get_hist_num(string& str);

    void read_data();
    void persist_data();
    void cd(const string& path);
    void pwd(int fd);
    void list_history(int fd);
    void export_variable(const string& var, int fd);
    void record_command(const string& cmd);
    void list_shell_vars(int fd);
    void add_shell_var(const string& var);
    void set_env();
    void set_cwd();
    void get_cstring_array(vector<string>& s_argv, char ** argv);
    void setup_redirection(string& redirect, int& in, int& out, int& err);
    void execute_internal_command(vector<string>& cmd, int fd);
    void expand_shell_vars(string& str);
    void replace_hist_commands(string& command);
};

#endif