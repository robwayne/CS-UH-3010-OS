#include "minishell.h"

//Trims the leading (left) spaces from a string in place
void ltrim_str(string& s) {
    s.erase(s.begin(), find_if_not(s.begin(), s.end(), [](char c){ return isspace(c);}));
}

//Trims trailing (right) spaces from a string in place
void rtrim_str(string& s) {
    s.erase(find_if_not(s.rbegin(), s.rend(), [](char c) {return isspace(c);}).base(), s.end());
}

//trims left and right spaces of a copy of the string and returns that copy
string trim_str(const string& s) {
    if(s.empty()) return {};
    string str = s;
    ltrim_str(str);
    rtrim_str(str);
    return str;
}

//trims the strings in place without copying
void trim_str_ip(string &s){
    if(s.empty()) return;
    ltrim_str(s);
    rtrim_str(s);
}

//Parses a commandline command and its argument and returns the command and its args
//"ls -l -a" -> {"ls", "-l -a"}
pair<string, string> MiniShell::get_cmd_with_args(const string &str) {
    string s, b;
    stringstream sin(str);
    sin >> s;
    getline(sin, b);
    return {s,b};
}

//Determines whether or not a string represents a number or not, true if it is a number false otherwise
bool is_number(const string& s) {
    if(s.empty() || !isdigit(s[0]))return false;
    return (s.find_first_not_of("0123456789") == s.npos);
}

//MiniShell constructor, initializes variables, reads data from file, starts up shell
MiniShell::MiniShell() {
    set_cwd();
    this->read_data();
    set_env();
    if(this->shell_vars.find("PATH") == end(this->shell_vars)) shell_vars["PATH"] = "";
}

// MiniShell destuctor: - required for build
MiniShell::~MiniShell() { return; }

//Reads the history and shell variables from file
void MiniShell::read_data() {
    string cmd, var, val;
    string hist_file = (shell_vars["PWD"] + "/hist"), var_file = (shell_vars["PWD"] + "/shvar"); //sets the abs path for the files
    fstream hf(hist_file, ios::in | ios::out), vf(var_file, ios::in | ios::out);
    if (!hf.is_open() && !vf.is_open()) return; //if the files couldnt be open, return (first time running possibly)

    while(getline(hf, cmd)) this->_history.push_back(cmd); //read all the files from the history and store in _history
    hf.close(); //close history file
    //read all the shell variables and store in shell_vars
    while(getline(vf, var)) {
        tie(var, val) = get_cmd_with_args(var);
        this->shell_vars[var] = trim_str(val); 
    }
    vf.close(); //close shell_vars file
}

//Writes the history and shell data to the file to be read when shell is restarted at later time
void MiniShell::persist_data() {
     string hist_file = (shell_vars["PWD"] + "/hist"), var_file = (shell_vars["PWD"] + "/shvar");//sets the abs path for the files
    fstream hf(hist_file, ios::out), vf(var_file, ios::out);
    if (!hf.is_open() && !vf.is_open()) { // if the files cant be written to there is an error, return
        printf("cannot write to data files. check file path.\n");
        return;
    }

    for(auto cmd: this->_history) hf << cmd << endl; //write all the history commands to file
    hf.close(); //close history file

    rep_iter(it, this->shell_vars.begin(), this->shell_vars.end()) { //write all the shell variables to file
        vf << it->first << " " << it->second << endl;
    }
    vf.close(); //close shell_vars file
}

// MiniShell$ history : lists all the commands that were previously made since the program has been initiated
void MiniShell::list_history(int fd) {
    if(this->_history.empty()) return;
    int N = this->_history.size();
    for(auto i=0;i<N;i++) {
        //creating a string to store the possible 12 digits, space and current history command to be written
        int line_len = _history[i].size() + 14; //11 possible digits (for int) and 2 extra for space and null terminator.
        char line[line_len];
        sprintf(line, "%d %s\n", i+1, _history[i].c_str());
        string s(line);
        write(fd, s.c_str(), s.size());
    }
}

//Stores all commands entered into the _history vector
void MiniShell::record_command(const string& cmd) {
    this->_history.push_back(cmd);
    this->persist_data(); 
}

//Parses the shell commands and their arguments, returns a pair of strings representing the shell var and its values
// "PATH=/bin" -> {"PATH", "/bin"}
pair<string, string> MiniShell::parse_shell_var(string var) {
    int N = var.length();
    if(N < 3) throw runtime_error("incorrect shell arguments"); //if the string has o
    auto pos = var.find("=");
    if(pos == string::npos || pos < 1 || pos == N-1) throw runtime_error("incorrect shell arguments");
    return {var.substr(0, pos), var.substr(pos+1, N-pos-1)};
}

//MiniShell$ export : Lists all the currently exported shell variables
void MiniShell::list_shell_vars(int fd) {
    if(this->shell_vars.empty()) return;
    rep_iter(it, this->shell_vars.begin(), this->shell_vars.end()) {
        string vars = it->first + "=" + it->second+"\n";
        write(fd, vars.c_str(), vars.size());
    }
}

//MiniShell$ export shell=/path/to/folder: Adds a new shell variable or appends a path to the end of a shell variable
//i.e PATH=/bin ; export PATH=/usr/bin -> PATH=/bin:/usr/bin
void MiniShell::add_shell_var(const string& var) {
    string s = trim_str(var), shell, val;
    try {
        tie(shell, val) = this->parse_shell_var(s); //parsing the shell can throw an error if the command doesnt match expected
        if(this->shell_vars.find(shell) == this->shell_vars.end()) { //if the shell var was never exported before
            this->shell_vars[shell] = val; //add it as a new shell
        } else {
            if(this->shell_vars[shell] != "") this->shell_vars[shell] += ":"+val; //append new path if it was 
            else this->shell_vars[shell] = val;
        }
    } catch (const exception& e) {
        printf("Incorrect use of export command.\n"); //if error display error message
    }
}

// Handles the export command, if there is no arguments, list all the shell vars, else add a new shell var
void MiniShell::export_variable(const string& var, int fd) {
    if(!var.empty()) this->add_shell_var(var);
    else this->list_shell_vars(fd);
}

//sets the current working directory and updates the PWD shell variable
void MiniShell::set_cwd() {
    char buff[BUFF_SIZE];
    getcwd(buff, BUFF_SIZE);
    shell_vars["PWD"] = string(buff);
}

// Find all external commands that have been added to the PATH
void MiniShell::set_env() {
    //if PATH is empty nothign to set
    if(shell_vars.empty() || shell_vars["PATH"] == "") return;

    int argc = shell_vars.size() + 1; // adding extra for the NULL pointer terminator
    vector<string> s_argv;
    environ = new char *[argc];

    for(auto& it: shell_vars) {
        string env_var = it.first + "=" + it.second; // creating the shell variable of the format "VAR=/path/to/dir"
        s_argv.push_back(env_var);
    }
    get_cstring_array(s_argv, environ);
}

// MiniShell$ pwd : Prints the current working directory 
void MiniShell::pwd(int fd) {
    for(char c: shell_vars["PWD"])  write(fd, &c, 1);
    char c = '\n';
    write(fd, &c, 1);
}

// MiniShell$ cd /path/to/folder : Changes the directory from the current working directory to the path specified
void MiniShell::cd(const string& path) {
    if(path.empty()) {
        printf("Please enter a path.\n");
        return;
    }
    if(chdir(path.c_str()) == -1) printf("Unable to change directory. Ensure '%s' is a valid path.\n", path.c_str());
    else set_cwd();
}

// returns the number
string MiniShell::get_hist_num(string &str) {
    if(str.size() < 2) return "";
    return str.substr(1, str.size()-1);
}

// MiniShell$ !<cmd_num> : Executes a command in the history with the identifier <cmd_num>
string MiniShell::execute_history_command(string& cmd) {
    string cmd_num = get_hist_num(cmd);
    if(!is_number(cmd_num)) return printf("'%s' is not a valid identifier.\n", cmd_num.c_str()), ""; //check if cmd_num is a valid 
    int index = stoi(cmd_num) - 1, n = this->_history.size(); //if the number is valid, convert to an int and subtract one to get the index
    if(index < 0 || index >= n) return printf("Command with identifier %s not found.\n", cmd_num.c_str()), "";
    return this->_history[index]; // if the index is valid (within the range of _history) then return that commmand to be executed
}

//splits a string into a vector of substrings separated by a character `sep`
vector<string> MiniShell::split_by_sep(const string &str, char sep) {
    if (str.empty()) return {};

    stringstream ss(str);
    string tmp;
    vector<string> strings;

    //loops through the string, separating at each `sep` character and then trimming each subsequent substring
    //appends each trimmed separated substring to a vector and then returns it
    while(getline(ss, tmp, sep)) strings.push_back(trim_str(tmp));
    return strings;
}

void MiniShell::replace_hist_commands(string &command) {
    int pos, startp, endp, len;
    string cmd, hist_cmd;
    char op = '!';

    while((pos = command.find_first_of(op)) != string::npos) {
        startp = pos+1;
        if(!isalnum(command[startp])) continue;
        endp = command.find_first_of(' ', startp);
        len = endp - pos;
        cmd = command.substr(pos,  len);
        hist_cmd = execute_history_command(cmd);
        command.replace(command.begin()+pos, command.begin()+endp, hist_cmd);
    }
}

void MiniShell::expand_shell_vars(string &str) {
    int pos, startp, endp, len;
    string var, expanded;
    char op = '$';

    while((pos = str.find_first_of(op)) != string::npos){
        startp = pos+1;
        if(!isalnum(str[startp])) continue;
        endp = str.find_first_of(' ', startp);
        len = endp - startp;
        var = str.substr(startp,  len);
        auto it = shell_vars.find(var);
        if(it == shell_vars.end())  expanded = "";
        else expanded = it->second;
        str.replace(str.begin()+pos, str.begin()+endp, expanded);
    }
}

// gets the input or output file that is being redirected to, eg: out.txt in 'ls > out.txt'
string get_io_file(string& str, char op, bool& is_err_file){
    int pos, startp, endp, len;
    string file;

    pos = str.find_last_of(op); //find the > or < character and find parse the file from it
    if (pos == str.npos) return ""; //if there is no such character then there isnt a redirection
    if(op == '>' && pos > 0 && str[pos-1] == '2') is_err_file = true;
    startp = str.find_first_not_of(' ', pos+1); //find the first character of the character after the redirection operator
    endp = str.find_first_of(' ', startp+1);
    len = endp - pos;
    file = str.substr(startp,  endp-startp); //parse out the file from the command
    str.erase(pos, len); //erase the file and return just the command

    return file; //return the file that is being redirected to
}

//sets up the redirection for each process
//setts the input and output file descriptors
void MiniShell::setup_redirection(string& redirect, int& in, int& out, int& err){
    string input_file, output_file;
    bool err_redirect = false;

    input_file = get_io_file(redirect, '<', err_redirect); //get the inout file if there is one
    output_file = get_io_file(redirect, '>', err_redirect); //get the output file if there is one as well
    if (input_file.empty() && output_file.empty()) return;

    if(!input_file.empty()) { //read from the input files
        in = open(input_file.c_str(), O_RDONLY);
        if (in == -1) {
            printf("Could not open file: %s\n", input_file.c_str());
        }
    }

    if(!output_file.empty()) { // create the output files to write to
        if(err_redirect) err = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        else out = open(output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out == -1 || err == -1) {
            printf("Could not write file: '%s'\n", output_file.c_str());
        }
    }
}

//run the entire shell
void MiniShell::execute() {
    string line;

    while(true) {
        cout << ">> ";
        getline(cin, line); //read the current line of the shell
        line = trim_str(line); //trim the current command
        set_env(); //set up the environment


        if(line == "exit"){
            record_command(line); //save the command entered
            break; //exits
        }

        replace_hist_commands(line);

        vector<string> piped_commands = split_by_sep(line, '|'); //break the input line into strings separated by pipes
        int i=0, p[2], in = 0, out = 1, err = 2, cmd_len = piped_commands.size();
        pid_t pid;

        for(auto pcmd: piped_commands) { //loop through all the commands from left to right eg: cat x.txt | grep main | grep file
            expand_shell_vars(pcmd);
            setup_redirection(pcmd, in, out, err);
            vector<string> split_cmd = split_by_sep(pcmd, ' ');

            pid = 1; //set pid to 1 so that if its an internal command it can still close the pipe
            pipe(p); //open a pipe so the commands can communicate to each other from left to right

            if (is_internal_cmd(split_cmd[0])) { //if the command is an internal command, execute that command
                if(out == 1 && i < cmd_len-1) execute_internal_command(split_cmd, p[1]); //execute each command and write output to approp. file
                else execute_internal_command(split_cmd, out); //if the user is redirecting the output to a file, write to that file instead
            } else { //else its an external command
                char *argv[split_cmd.size()+1];
                get_cstring_array(split_cmd, argv); //get the C-string version of each command so that we can execute it
                pid = fork(); // fork the current process so that the child process runs the intened command without exiting this current process
                if (pid < 0) {
                    perror("fork failed\n");
                    std::exit(-1);
                } else if (pid == 0) { //if the child process is being run, we can run the intended command now
                    dup2(in, 0); // redirect the input from standard in to the appropriate file of the process to be run
                    if(out == 1 && i < cmd_len-1) dup2(p[1], 1); // redirect output if the user has requested to do so, eg: ls > ls.txt
                    else dup2(out, 1);
                    close(p[0]);
                    dup2(err, 2);
                    execvp(argv[0], &argv[0]); //execute the command
                    string message("could not execute command: ");
                    message += string(argv[0]);
                    perror(message.c_str());
                    std::exit(-1);
                }
            }

            if (pid > 0){ //if this is the parent process lets close the write end of the pipe
                wait(NULL);
                if (out != 1) close(out); //close an out file if it is open
                if(in != 0) close(in);
                close(p[1]);
                in = p[0]; //save the input for next command
            }
            i++;
        }
        record_command(line); //save the command entered
        close(p[0]);
        close(p[1]);
    }
}

//gets a cstring array from an array of strings to use in execvp or set environ
void MiniShell::get_cstring_array(vector<string>& s_argv, char ** argv) {
    int i=0, n = s_argv.size();
    for(;i<n;i++) argv[i] = const_cast<char *> (s_argv[i].c_str());
    argv[i] = nullptr;
}

//determines if the command entered is an internal ocmmand
bool MiniShell::is_internal_cmd(string cmd) {
    return (cmd == "cd" || cmd == "export" || cmd == "pwd" || cmd == "history" || cmd[0] == '!');
}

//if the command is internal execute it
void MiniShell::execute_internal_command(vector<string>& split_cmd, int fd){
    if (split_cmd.size() < 2) split_cmd.push_back("");
    if(split_cmd[0] == "cd") cd(split_cmd[1]);
    else if (split_cmd[0] == "history") list_history(fd);
    else if (split_cmd[0] == "export") export_variable(split_cmd[1], fd);
    else if (split_cmd[0] == "pwd") pwd(fd);
}

